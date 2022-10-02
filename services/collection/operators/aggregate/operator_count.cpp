#include "operator_count.hpp"

namespace services::collection::operators::aggregate {

    constexpr auto key_ = "count";

    operator_count_t::operator_count_t(context_collection_t* context)
        : operator_aggregate_t(context) {
    }

    document_ptr operator_count_t::aggregate_impl() {
        if (left_ && left_->output()) {
            return components::document::make_document(key_, uint64_t(left_->output()->size()));
        }
        return components::document::make_document(key_, 0);
    }

    std::string operator_count_t::key_impl() const {
        return key_;
    }

} // namespace services::collection::operators::aggregate