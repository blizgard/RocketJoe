#pragma once
#include <memory>
#include <unordered_map>

#include <goblin-engineer/core.hpp>
#include <pybind11/pytypes.h>

#include "log/log.hpp"

#include "protocol/base.hpp"
#include "protocol/forward.hpp"

#include "storage/conditional_expression.hpp"
#include "storage/document.hpp"

#include "forward.hpp"
#include "route.hpp"
#include "query.hpp"

namespace services::storage {
    using document_t  = components::storage::document_t;
    class collection_t final : public goblin_engineer::abstract_service {
    public:
        using storage_t = std::unordered_map<std::string, document_t>;
        using iterator = typename storage_t::iterator;

        collection_t(database_ptr database, log_t& log);
        void insert(session_t& session_t,std::string& collection,document_t& document);
        auto get(components::storage::conditional_expression& cond) -> void;
        auto search(const session_t &session, const std::string &collection, query_ptr cond) -> void;
        auto find(const session_t& session, const std::string &collection, const document_t &cond) -> void;
        auto all() -> void;
       /// void insert_many(py::iterable iterable);
        auto size(session_t& session, std::string& collection) -> void;
        void update(document_t& fields, components::storage::conditional_expression& cond);
        void remove(components::storage::conditional_expression& cond);
        void drop();

    private:
        void insert_(const std::string& uid, document_t&& document);
        document_t* get_(const std::string& uid);
        std::size_t size_() const;
        auto begin() -> iterator;
        auto end() -> iterator;
        auto remove_(const std::string& key);
        void drop_();
        std::vector<document_t *> search_(query_ptr cond);

        log_t log_;
        storage_t storage_;

#ifdef DEV_MODE
    public:
        void dummy_insert(document_t&& document);
        std::vector<components::document::document_t *> search_test(query_ptr cond);
        std::vector<components::document::document_t *> find_test(const document_t &cond);
#endif
    };

    using collection_ptr = goblin_engineer::intrusive_ptr<collection_t>;

} // namespace services::document