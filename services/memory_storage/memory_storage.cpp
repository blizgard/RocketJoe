#include "memory_storage.hpp"
#include "route.hpp"
#include <cassert>
#include <components/physical_plan_generator/create_plan.hpp>
#include <components/ql/statements/create_collection.hpp>
#include <components/ql/statements/create_database.hpp>
#include <components/ql/statements/drop_collection.hpp>
#include <components/ql/statements/drop_database.hpp>
#include <core/system_command.hpp>
#include <core/tracy/tracy.hpp>
#include <services/collection/collection.hpp>

using namespace components::cursor;

namespace services {

    using namespace services::memory_storage;
    using namespace core::pmr;

    memory_storage_t::load_buffer_t::load_buffer_t(std::pmr::memory_resource* resource)
        : collections(resource) {}

    memory_storage_t::memory_storage_t(std::pmr::memory_resource* resource,
                                       actor_zeta::scheduler_raw scheduler,
                                       log_t& log)
        : actor_zeta::cooperative_supervisor<memory_storage_t>(resource, "memory_storage")
        , executor_(nullptr, [&](collection::executor::executor_t* agent) { mr_delete(this->resource(), agent); })
        , log_(log.clone())
        , e_(scheduler)
        , databases_(resource)
        , collections_(resource) {
        ZoneScoped;
        trace(log_, "memory_storage start thread pool");
        executor_address_ = spawn_actor<services::collection::executor::executor_t>(
            [this](services::collection::executor::executor_t* ptr) { executor_.reset(ptr); },
            resource,
            std::move(log_.clone()));
        add_handler(core::handler_id(core::route::sync), &memory_storage_t::sync);
        add_handler(handler_id(route::execute_plan), &memory_storage_t::execute_plan);
        add_handler(handler_id(route::load), &memory_storage_t::load);

        add_handler(collection::handler_id(collection::route::create_documents_finish),
                    &memory_storage_t::create_documents_finish_);
        add_handler(collection::handler_id(collection::route::execute_plan_finish),
                    &memory_storage_t::execute_plan_finish_);
        add_handler(collection::handler_id(collection::route::size), &memory_storage_t::size);
        add_handler(collection::handler_id(collection::route::close_cursor), &memory_storage_t::close_cursor);
    }

    memory_storage_t::~memory_storage_t() {
        ZoneScoped;
        trace(log_, "delete memory_storage");
    }

    void memory_storage_t::sync(const address_pack& pack) {
        manager_dispatcher_ = std::get<static_cast<uint64_t>(unpack_rules::manager_dispatcher)>(pack);
        manager_disk_ = std::get<static_cast<uint64_t>(unpack_rules::manager_disk)>(pack);
    }

    void memory_storage_t::execute_plan(components::session::session_id_t& session,
                                        components::logical_plan::node_ptr logical_plan,
                                        components::ql::storage_parameters parameters) {
        using components::logical_plan::node_type;

        switch (logical_plan->type()) {
            case node_type::create_database_t:
                create_database_(session, std::move(logical_plan));
                break;
            case node_type::drop_database_t:
                drop_database_(session, std::move(logical_plan));
                break;
            case node_type::create_collection_t:
                create_collection_(session, std::move(logical_plan));
                break;
            case node_type::drop_collection_t:
                drop_collection_(session, std::move(logical_plan));
                break;
            default:
                execute_plan_(session, std::move(logical_plan), std::move(parameters));
                break;
        }
    }

    void memory_storage_t::size(components::session::session_id_t& session, collection_full_name_t&& name) {
        trace(log_, "collection {}::{}::size", name.database, name.collection);
        if (!check_collection_(session, name)) {
            return;
        }
        auto collection = collections_.at(name).get();
        if (collection->dropped()) {
            actor_zeta::send(current_message()->sender(),
                             address(),
                             handler_id(collection::route::size_finish),
                             session,
                             make_cursor(resource(), error_code_t::collection_dropped));
        } else {
            auto* sub_cursor = new sub_cursor_t(collection->resource(), collection->name());
            for (const auto& doc : collection->storage()) {
                sub_cursor->append(doc.second);
            }
            auto cursor = make_cursor(collection->resource());
            cursor->push(sub_cursor);
            actor_zeta::send(current_message()->sender(),
                             address(),
                             handler_id(collection::route::size_finish),
                             session,
                             std::move(cursor));
        }
    }

    void memory_storage_t::close_cursor(components::session::session_id_t& session,
                                        std::set<collection_full_name_t>&& collections) {
        for (const auto& name : collections) {
            if (check_collection_(session, name)) {
                collections_.at(name)->cursor_storage().erase(session);
            }
        }
    }

    void memory_storage_t::load(components::session::session_id_t& session, const disk::result_load_t& result) {
        trace(log_, "memory_storage_t:load");
        load_buffer_ = std::make_unique<load_buffer_t>(resource());
        auto count_collections = std::accumulate(
            (*result).begin(),
            (*result).end(),
            0ul,
            [](size_t sum, const disk::result_database_t& database) { return sum + database.collections.size(); });
        if (count_collections > 0) {
            sessions_.emplace(session, session_t{nullptr, current_message()->sender(), count_collections});
        }
        for (const auto& database : (*result)) {
            debug(log_, "memory_storage_t:load:create_database: {}", database.name);
            databases_.insert(database.name);
            for (const auto& collection : database.collections) {
                debug(log_, "memory_storage_t:load:create_collection: {}", collection.name);
                collection_full_name_t name(database.name, collection.name);
                auto context = new collection::context_collection_t(resource(), name, manager_disk_, log_.clone());
                collections_.emplace(name, context);
                load_buffer_->collections.emplace_back(name);
                debug(log_, "memory_storage_t:load:fill_documents: {}", collection.documents.size());
                actor_zeta::send(executor_address_,
                                 address(),
                                 collection::handler_id(collection::route::create_documents),
                                 session,
                                 context,
                                 collection.documents);
            }
        }
        if (count_collections == 0) {
            actor_zeta::send(current_message()->sender(), address(), handler_id(route::load_finish), session);
            load_buffer_.reset();
        }
    }

    actor_zeta::scheduler::scheduler_abstract_t* memory_storage_t::scheduler_impl() noexcept { return e_; }

    void memory_storage_t::enqueue_impl(actor_zeta::message_ptr msg, actor_zeta::execution_unit*) {
        ZoneScoped;
        std::unique_lock<spin_lock> _(lock_);
        set_current_message(std::move(msg));
        execute(this, current_message());
    }

    bool memory_storage_t::is_exists_database_(const database_name_t& name) const {
        return databases_.find(name) != databases_.end();
    }

    bool memory_storage_t::is_exists_collection_(const collection_full_name_t& name) const {
        return collections_.contains(name);
    }

    bool memory_storage_t::check_database_(components::session::session_id_t& session, const database_name_t& name) {
        if (!is_exists_database_(name)) {
            actor_zeta::send(current_message()->sender(),
                             this->address(),
                             handler_id(route::execute_plan_finish),
                             session,
                             make_cursor(resource(), error_code_t::database_not_exists, "database not exists"));
            return false;
        }
        return true;
    }

    bool memory_storage_t::check_collection_(components::session::session_id_t& session,
                                             const collection_full_name_t& name) {
        if (check_database_(session, name.database)) {
            if (!is_exists_collection_(name)) {
                actor_zeta::send(current_message()->sender(),
                                 this->address(),
                                 handler_id(route::execute_plan_finish),
                                 session,
                                 make_cursor(resource(), error_code_t::collection_not_exists, "collection not exists"));
                return false;
            }
            return true;
        }
        return false;
    }

    void memory_storage_t::create_database_(components::session::session_id_t& session,
                                            components::logical_plan::node_ptr logical_plan) {
        trace(log_, "memory_storage_t:create_database {}", logical_plan->database_name());
        if (is_exists_database_(logical_plan->database_name())) {
            actor_zeta::send(current_message()->sender(),
                             this->address(),
                             handler_id(route::execute_plan_finish),
                             session,
                             make_cursor(resource(), error_code_t::database_already_exists, "database already exists"));
            return;
        }
        databases_.insert(logical_plan->database_name());
        actor_zeta::send(current_message()->sender(),
                         this->address(),
                         handler_id(route::execute_plan_finish),
                         session,
                         make_cursor(resource(), operation_status_t::success));
    }

    void memory_storage_t::drop_database_(components::session::session_id_t& session,
                                          components::logical_plan::node_ptr logical_plan) {
        trace(log_, "memory_storage_t:drop_database {}", logical_plan->database_name());
        if (check_database_(session, logical_plan->database_name())) {
            databases_.erase(logical_plan->database_name());
            actor_zeta::send(current_message()->sender(),
                             this->address(),
                             handler_id(route::execute_plan_finish),
                             session,
                             make_cursor(resource(), operation_status_t::success));
        }
    }

    void memory_storage_t::create_collection_(components::session::session_id_t& session,
                                              components::logical_plan::node_ptr logical_plan) {
        trace(log_, "memory_storage_t:create_collection {}", logical_plan->collection_full_name().to_string());
        if (check_database_(session, logical_plan->database_name())) {
            if (is_exists_collection_(logical_plan->collection_full_name())) {
                actor_zeta::send(
                    current_message()->sender(),
                    this->address(),
                    handler_id(route::execute_plan_finish),
                    session,
                    make_cursor(resource(), error_code_t::collection_already_exists, "collection already exists"));
                return;
            }
            collections_.emplace(logical_plan->collection_full_name(),
                                 new collection::context_collection_t(resource(),
                                                                      logical_plan->collection_full_name(),
                                                                      manager_disk_,
                                                                      log_.clone()));
            auto cursor = make_cursor(resource(), operation_status_t::success);
            actor_zeta::send(current_message()->sender(),
                             this->address(),
                             handler_id(route::execute_plan_finish),
                             session,
                             cursor);
        }
    }

    void memory_storage_t::drop_collection_(components::session::session_id_t& session,
                                            components::logical_plan::node_ptr logical_plan) {
        trace(log_, "memory_storage_t:drop_collection {}", logical_plan->collection_full_name().to_string());
        if (check_collection_(session, logical_plan->collection_full_name())) {
            sessions_.emplace(session, session_t{logical_plan, current_message()->sender(), 1});
            actor_zeta::send(current_message()->sender(),
                             address(),
                             handler_id(route::execute_plan_finish),
                             session,
                             collections_.at(logical_plan->collection_full_name())->drop()
                                 ? make_cursor(resource(), operation_status_t::success)
                                 : make_cursor(resource(), error_code_t::other_error, "collection not dropped"));
            sessions_.erase(session);
            collections_.erase(logical_plan->collection_full_name());
            trace(log_, "memory_storage_t:drop_collection_finish {}", logical_plan->collection_full_name().to_string());
        }
    }

    void memory_storage_t::execute_plan_(components::session::session_id_t& session,
                                         components::logical_plan::node_ptr logical_plan,
                                         components::ql::storage_parameters parameters) {
        trace(log_,
              "memory_storage_t:execute_plan_: collection: {}, sesion: {}",
              logical_plan->collection_full_name().to_string(),
              session.data());
        auto dependency_tree_collections_names = logical_plan->collection_dependencies();
        context_storage_t collections_context_storage;
        while (!dependency_tree_collections_names.empty()) {
            collection_full_name_t name =
                dependency_tree_collections_names.extract(dependency_tree_collections_names.begin()).value();
            if (!check_collection_(session, name)) {
                trace(log_,
                      "memory_storage_t:execute_plan_: collection not found {}, sesion: {}",
                      name.to_string(),
                      session.data());
                return;
            }
            collections_context_storage.emplace(std::move(name), collections_.at(name).get());
        }
        sessions_.emplace(session, session_t{logical_plan, current_message()->sender(), 1});
        actor_zeta::send(executor_address_,
                         address(),
                         collection::handler_id(collection::route::execute_plan),
                         session,
                         logical_plan,
                         parameters,
                         std::move(collections_context_storage));
    }

    void memory_storage_t::execute_plan_finish_(components::session::session_id_t& session, cursor_t_ptr result) {
        auto& s = sessions_.at(session);
        debug(log_,
              "memory_storage_t:execute_plan_finish: session: {}, success: {}",
              session.data(),
              result->is_success());
        actor_zeta::send(s.sender, address(), handler_id(route::execute_plan_finish), session, std::move(result));
        sessions_.erase(session);
    }

    void memory_storage_t::create_documents_finish_(components::session::session_id_t& session) {
        if (!sessions_.contains(session)) {
            return;
        }
        auto& s = sessions_.at(session);
        --s.count_answers;
        debug(log_, "memory_storage_t:create_documents_finish: {}, rest: {}", session.data(), s.count_answers);
        if (s.count_answers == 0) {
            actor_zeta::send(s.sender, address(), handler_id(route::load_finish), session);
            load_buffer_.reset();
            sessions_.erase(session);
        }
    }

} // namespace services
