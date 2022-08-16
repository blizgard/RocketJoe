#pragma once

#include <map>
#include <memory>
#include <scoped_allocator>
#include <string>
#include <utility>
#include <vector>

#include "forward.hpp"
#include "core/pmr_unique.hpp"

namespace components::index {

    struct index_engine_t final {
    public:
        using value_t = index_ptr;

        explicit index_engine_t(actor_zeta::detail::pmr::memory_resource* resource);
        auto find(id_index id) -> index_raw_ptr;
        auto find(const keys_base_t& query) -> index_raw_ptr;
        auto emplace(const keys_base_t&, value_t) -> uint32_t;
        [[nodiscard]] auto size() const -> std::size_t;
        actor_zeta::detail::pmr::memory_resource* resource() noexcept;

    private:
        using comparator_t = std::less<keys_base_t>;
        using base_storgae = std::pmr::list<index_ptr>;

        using keys_to_doc_t = std::pmr::map<keys_base_t, base_storgae::iterator, comparator_t>;
        using index_to_doc_t = std::pmr::unordered_map<id_index, base_storgae::iterator>;

        actor_zeta::detail::pmr::memory_resource* resource_;
        keys_to_doc_t mapper_;
        index_to_doc_t index_to_mapper_;
        base_storgae storage_;
    };

    using index_engine_ptr = core::pmr::unique_ptr<index_engine_t>;

    auto make_index_engine(actor_zeta::detail::pmr::memory_resource* resource) -> index_engine_ptr;
    auto search_index(const index_engine_ptr& ptr, id_index id) -> index_t*;
    auto search_index(const index_engine_ptr& ptr, const query_t& query) -> index_t*;

    template<class Target, class... Args>
    auto make_index(index_engine_ptr& ptr, const keys_base_t& keys, Args&&... args) -> uint32_t {
        return ptr->emplace(keys, std::make_unique<Target>(ptr->resource(), keys, std::forward<Args>(args)...));
    }

    void insert(const index_engine_ptr& ptr, id_index id, std::pmr::vector<document_ptr>& docs);
    void insert_one(const index_engine_ptr& ptr, id_index id, document_ptr docs);
    /// void find(const index_engine_ptr& index, id_index id , result_set_t* );
    /// void find(const index_engine_ptr& index, query_t query,result_set_t* );

} // namespace components::index