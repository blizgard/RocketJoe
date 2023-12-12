#include "wrapper_collection.hpp"

#include "convert.hpp"
#include <components/cursor/cursor.hpp>
#include <components/document/document.hpp>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>
#include "wrapper_database.hpp"

// The bug related to the use of RTTI by the pybind11 library has been fixed: a
// declaration should be in each translation unit.
PYBIND11_DECLARE_HOLDER_TYPE(T, boost::intrusive_ptr<T>)
namespace otterbrix {

    using components::document::document_id_t;

    void generate_document_id_if_not_exists(components::document::document_ptr& document) {
        if (!document_view_t(document).is_exists(std::string_view("_id"))) {
            document->set("_id", document_id_t().to_string());
        }
    }

    wrapper_collection::wrapper_collection(const std::string& name, const std::string& database, wrapper_dispatcher_t* ptr, log_t& log)
        : name_(name)
        , database_(database)
        , ptr_(ptr)
        , log_(log.clone()) {
        trace(log_, "wrapper_collection");
    }

    wrapper_collection::~wrapper_collection() {
        trace(log_, "delete wrapper_collection");
    }

    std::string wrapper_collection::print() {
        return name_;
    }

    std::size_t wrapper_collection::size() {
        trace(log_, "wrapper_collection::size");
        auto session_tmp = otterbrix::session_id_t();
        return ptr_->size(session_tmp, database_, name_)->size();
    }

    pybind11::list wrapper_collection::insert(const py::handle& documents) {
        trace(log_, "wrapper_collection::insert");
        if (py::isinstance<py::dict>(documents)) {
            py::list result;
            auto id = insert_one(documents);
            if (!id.empty()) {
                result.append(id);
            }
            return result;
        }
        if (py::isinstance<py::list>(documents)) {
            return insert_many(documents);
        }
        return py::list();
    }

    std::string wrapper_collection::insert_one(const py::handle& document) {
        trace(log_, "wrapper_collection::insert_one");
        if (py::isinstance<py::dict>(document)) {
            auto doc = to_document(document);
            generate_document_id_if_not_exists(doc);
            auto session_tmp = otterbrix::session_id_t();
            auto cur = ptr_->insert_one(session_tmp, database_, name_, doc);
            debug(log_, "wrapper_collection::insert_one {} inserted", cur->size());
            if (cur->is_error()) {
                debug(log_, "wrapper_collection::insert_one has result error while insert");
                throw std::runtime_error("wrapper_collection::insert_one error_result");
            }
            debug(log_, "wrapper_collection::insert_one {} inserted", cur->size());
            return cur->size() > 0 ? cur->get()->id().to_string() : std::string();
        }
        throw std::runtime_error("wrapper_collection::insert_one");
        return std::string();
    }

    pybind11::list wrapper_collection::insert_many(const py::handle& documents) {
        trace(log_, "wrapper_collection::insert_many");
        if (py::isinstance<py::list>(documents)) {
            std::pmr::vector<components::document::document_ptr> docs;
            for (const auto document : documents) {
                auto doc = to_document(document);
                generate_document_id_if_not_exists(doc);
                docs.push_back(std::move(doc));
            }
            auto session_tmp = otterbrix::session_id_t();
            auto cur = ptr_->insert_many(session_tmp, database_, name_, docs);
            if(cur->is_error()) {
                debug(log_, "wrapper_collection::insert_many has result error while insert");
                throw std::runtime_error("wrapper_collection::insert_many error_result");
            }
            debug(log_, "wrapper_collection::insert_many {} inserted", cur->size());
            py::list list;
            for (const auto& sub_cursor : *cur) {
                for(const auto& doc : sub_cursor->data()) {
                    list.append(doc.id().to_string());
                }
            }
            return list;
        }
        throw std::runtime_error("wrapper_collection::insert_many");
        return py::list();
    }

    wrapper_cursor_ptr wrapper_collection::update_one(py::object cond, py::object fields, bool upsert) {
        trace(log_, "wrapper_collection::update_one");
        if (py::isinstance<py::dict>(cond) && py::isinstance<py::dict>(fields)) {
            auto statement = components::ql::make_aggregate_statement(database_, name_);
            to_statement(pack_to_match(cond), statement.get());
            auto update = to_document(fields);
            generate_document_id_if_not_exists(update);
            auto session_tmp = otterbrix::session_id_t();
            auto cur = ptr_->update_one(session_tmp, statement.release(), std::move(update), upsert);
            if (cur->is_error()) {
                debug(log_, "wrapper_collection::update_one has result error while update");
                throw std::runtime_error("wrapper_collection::update_one error_result");
            }
            debug(log_, "wrapper_collection::update_one {} modified, upsert id {}", cur->size(), cur->size() == 0 ? "none" : cur->get()->id().to_string());
            return wrapper_cursor_ptr{new wrapper_cursor{otterbrix::session_id_t(), cur}};
        }
        return wrapper_cursor_ptr{new wrapper_cursor{otterbrix::session_id_t(), new components::cursor::cursor_t(ptr_->resource())}};
    }

    wrapper_cursor_ptr wrapper_collection::update_many(py::object cond, py::object fields, bool upsert) {
        trace(log_, "wrapper_collection::update_many");
        if (py::isinstance<py::dict>(cond) && py::isinstance<py::dict>(fields)) {
            auto statement = components::ql::make_aggregate_statement(database_, name_);
            to_statement(pack_to_match(cond), statement.get());
            auto update = to_document(fields);
            generate_document_id_if_not_exists(update);
            auto session_tmp = otterbrix::session_id_t();
            auto cur = ptr_->update_many(session_tmp, statement.release(), std::move(update), upsert);
            if (cur->is_error()) {
                debug(log_, "wrapper_collection::update_many has result error while update");
                throw std::runtime_error("wrapper_collection::update_many error_result");
            }
            debug(log_, "wrapper_collection::update_one {} modified, upsert id {}", cur->size(), cur->size() == 0 ? "none" : cur->get()->id().to_string());
            return wrapper_cursor_ptr{new wrapper_cursor{otterbrix::session_id_t(), cur}};
        }
        return wrapper_cursor_ptr{new wrapper_cursor{otterbrix::session_id_t(), new components::cursor::cursor_t(ptr_->resource())}};
    }

    auto wrapper_collection::find(py::object cond) -> wrapper_cursor_ptr {
        trace(log_, "wrapper_collection::find");
        if (py::isinstance<py::dict>(cond)) {
            auto statement = components::ql::make_aggregate_statement(database_, name_);
            to_statement(pack_to_match(cond), statement.get());
            auto session_tmp = otterbrix::session_id_t();
            auto cur = ptr_->find(session_tmp, statement.release());
            debug(log_, "wrapper_collection::find {} records", cur->size());
            return wrapper_cursor_ptr(new wrapper_cursor(session_tmp, cur));
        }
        throw std::runtime_error("wrapper_collection::find");
        return wrapper_cursor_ptr();
    }

    auto wrapper_collection::find_one(py::object cond) -> py::dict {
        trace(log_, "wrapper_collection::find_one");
        if (py::isinstance<py::dict>(cond)) {
            auto statement = components::ql::make_aggregate_statement(database_, name_);
            to_statement(pack_to_match(cond), statement.get());
            auto session_tmp = otterbrix::session_id_t();
            auto cur = ptr_->find_one(session_tmp, statement.release());
            debug(log_, "wrapper_collection::find_one {}", cur->size() > 0);
            if (cur->size() > 0) {
                return from_document(*cur->next());
            }
            return py::dict();
        }
        throw std::runtime_error("wrapper_collection::find_one");
        return py::dict();
    }

    wrapper_cursor_ptr wrapper_collection::delete_one(py::object cond) {
        trace(log_, "wrapper_collection::delete_one");
        if (py::isinstance<py::dict>(cond)) {
            auto statement = components::ql::make_aggregate_statement(database_, name_);
            to_statement(pack_to_match(cond), statement.get());
            auto session_tmp = otterbrix::session_id_t();
            auto cur = ptr_->delete_one(session_tmp, statement.release());
            if(cur->is_error()){
                debug(log_, "wrapper_collection::delete_one has result error while delete");
                throw std::runtime_error("wrapper_collection::delete_one error_result");
            }
            debug(log_, "wrapper_collection::delete_one {} deleted", cur->size());
            return wrapper_cursor_ptr{new wrapper_cursor{otterbrix::session_id_t(), cur}};
        }
        return wrapper_cursor_ptr{new wrapper_cursor{otterbrix::session_id_t(), new components::cursor::cursor_t(ptr_->resource())}};
    }

    wrapper_cursor_ptr wrapper_collection::delete_many(py::object cond) {
        trace(log_, "wrapper_collection::delete_many");
        if (py::isinstance<py::dict>(cond)) {
            auto statement = components::ql::make_aggregate_statement(database_, name_);
            to_statement(pack_to_match(cond), statement.get());
            auto session_tmp = otterbrix::session_id_t();
            auto cur = ptr_->delete_many(session_tmp, statement.release());
            if(cur->is_error()){
                debug(log_, "wrapper_collection::delete_many has result error while delete");
                throw std::runtime_error("wrapper_collection::delete_many error_result");
            }
            debug(log_, "wrapper_collection::delete_many {} deleted", cur->size());
            return wrapper_cursor_ptr{new wrapper_cursor{otterbrix::session_id_t(), cur}};
        }
        return wrapper_cursor_ptr{new wrapper_cursor{otterbrix::session_id_t(), new components::cursor::cursor_t(ptr_->resource())}};
    }

    bool wrapper_collection::drop() {
        trace(log_, "wrapper_collection::drop: {}", name_);
        auto session_tmp = otterbrix::session_id_t();
        auto cur = ptr_->drop_collection(session_tmp, database_, name_);
        debug(log_, "wrapper_collection::drop {}", cur->is_success());
        return cur->is_success();
    }
/*
    auto wrapper_collection::aggregate(const py::sequence& it) -> wrapper_cursor_ptr {
        trace(log_, "wrapper_collection::aggregate");
        if (py::isinstance<py::sequence>(it)) {

           /// auto condition = experimental::to_statement(it);
            //auto session_tmp = otterbrix::session_id_t();
            //auto cur = ptr_->find(session_tmp, database_, name_, std::move(condition));
            ///trace(log_, "wrapper_collection::find {} records", cur->size());
            ///return wrapper_cursor_ptr(new wrapper_cursor(session_tmp, cur));
        }
        throw std::runtime_error("wrapper_collection::find");
    }
    */
    bool wrapper_collection::create_index(const py::list& keys, index_type type, index_compare compare) {
        debug(log_, "wrapper_collection::create_index: {}", name_);
        auto session_tmp = otterbrix::session_id_t();
        components::ql::create_index_t index(database_, name_, type, compare);
        for (const auto &key : keys) {
            index.keys_.emplace_back(key.cast<std::string>());
        }
        auto cur = ptr_->create_index(session_tmp, index);
        debug(log_, "wrapper_collection::create_index {}", cur->is_success());
        return cur->is_success();
    }

} // namespace otterbrix
