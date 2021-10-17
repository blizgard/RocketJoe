#pragma once

namespace services::storage {

    namespace manager_database {
        static constexpr auto create_database = "create_database";
    }

    namespace dispatcher {
        static constexpr auto create_collection = "create_collection";
        static constexpr auto create_database = "create_database";
        static constexpr auto select = "select";
        static constexpr auto insert = "insert";
        static constexpr auto erase = "erase";
        static constexpr auto ws_dispatch = "ws_dispatch";
    } // namespace dispatcher

    namespace database {
        static constexpr auto create_collection = "create_collection";
    }

    namespace collection {
        static constexpr auto select = "select";
        static constexpr auto insert = "insert";
        static constexpr auto erase = "erase";
        static constexpr auto search = "search";
        static constexpr auto find = "find";
        static constexpr auto size = "size";
    } // namespace collection

} // namespace kv