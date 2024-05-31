#include "parser_index_drop.hpp"
#include <components/sql/parser/base/parser_mask.hpp>

using components::sql::impl::mask_element_t;
using components::sql::impl::mask_t;

namespace components::sql::index::impl {

    constexpr uint64_t database_name = 4;
    constexpr uint64_t collection_name = 6;
    constexpr uint64_t name = 8;

    components::sql::impl::parser_result
    parse_drop(std::pmr::memory_resource*, std::string_view query, ql::variant_statement_t& statement) {
        static mask_t mask({mask_element_t(token_type::bare_word, "drop"),
                            mask_element_t(token_type::whitespace, ""),
                            mask_element_t(token_type::bare_word, "index"),
                            mask_element_t(token_type::whitespace, ""),
                            mask_element_t::create_value_mask_element(),
                            mask_element_t(token_type::dot, "."),
                            mask_element_t::create_value_mask_element(),
                            mask_element_t(token_type::dot, "."),
                            mask_element_t::create_value_mask_element(),
                            mask_element_t(token_type::semicolon, ";", true)});

        lexer_t lexer(query);
        if (mask.match(lexer)) {
            statement =
                components::ql::drop_index_t{mask.cap(database_name), mask.cap(collection_name), mask.cap(name)};
            return true;
        }
        return false;
    }

} // namespace components::sql::index::impl
