#include "operator.hpp"

namespace services::collection::operators {

    operator_t::operator_t(context_collection_t* context)
        : context_(context) {
    }

    void operator_t::on_execute(const predicate_ptr& predicate, predicates::limit_t limit, components::cursor::sub_cursor_t* cursor) {
        return on_execute_impl(predicate, limit, cursor);
    }

} // namespace services::collection::operators
