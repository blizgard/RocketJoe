#pragma once

#include <components/session/session.hpp>

namespace services::disk {

    struct command_append_database_t {
        database_name_t database;
    };

    struct command_remove_database_t {
        database_name_t database;
    };

    struct command_append_collection_t {
        database_name_t database;
        collection_name_t collection;
    };

    struct command_remove_collection_t {
        database_name_t database;
        collection_name_t collection;
    };

    struct command_write_documents_t {
        database_name_t database;
        collection_name_t collection;
        std::vector<components::document::document_ptr> documents;
    };

    struct command_remove_documents_t {
        database_name_t database;
        collection_name_t collection;
        std::vector<components::document::document_id_t> documents;
    };

    class command_t {
    public:
        using command_name_t = uint64_t;

        template<class T>
        explicit command_t(const T command)
            : command_(command) {}

        template<class T>
        const T& get() const {
            return std::get<T>(command_);
        }

        command_name_t name() const;

    private:
        std::variant<command_append_database_t,
                     command_remove_database_t,
                     command_append_collection_t,
                     command_remove_collection_t,
                     command_write_documents_t,
                     command_remove_documents_t>
            command_;
    };

    using command_storage_t = std::unordered_map<components::session::session_id_t, std::vector<command_t>>;

    void append_command(command_storage_t &storage, const components::session::session_id_t &session, const command_t &command);

} //namespace services::disk