#include "serializer.hpp"

#include <boost/json.hpp>

#include <components/document/mutable/mutable_array.h>
#include <components/document/mutable/mutable_dict.h>
#include <components/document/document_view.hpp>


namespace components::document {

    std::vector<std::uint8_t> to_msgpack(const document_t& j) {
        std::vector<std::uint8_t> result;
        to_msgpack(j, result);
        return result;
    }

    void to_msgpack(const document_t& j, output_adapter<std::uint8_t> o) {
        binary_writer<std::uint8_t>(o).write_msgpack(j);
    }

    void to_msgpack(const document_t& j, output_adapter<char> o) {
        binary_writer<char>(o).write_msgpack(j);
    }

    static document_const_value_t json2value(const boost::json::value &item) {
        if (item.is_bool()) {
            return ::document::impl::new_value(item.get_bool());
        } else if (item.is_uint64()) {
            return ::document::impl::new_value(item.get_uint64());
        } else if (item.is_int64()) {
            return ::document::impl::new_value(item.get_int64());
        } else if (item.is_double()) {
            return ::document::impl::new_value(item.get_double());
        } else if (item.is_string()) {
            return ::document::impl::new_value(std::string(item.get_string().c_str()));
        } else if (item.is_array()) {
            auto array = ::document::impl::mutable_array_t::new_array();
            for (const auto &child : item.get_array()) {
                array->append(json2value(child));
            }
            return array->as_array();
        } else if (item.is_object()) {
            auto dict = ::document::impl::mutable_dict_t::new_dict();
            for (const auto &child : item.get_object()) {
                dict->set(std::string(child.key()), json2value(child.value()));
            }
            return dict->as_dict();
        }
        return ::document::impl::value_t::null_value;
    }

    document_ptr from_json(const std::string &json) {
        auto doc = make_document();
        auto tree = boost::json::parse(json);
        for (const auto &item : tree.as_object()) {
            doc->set(std::string(item.key()), json2value(item.value()));
        }
        return doc;
    }

    std::string to_json(const document_ptr &doc) {
        return document_view_t(doc).to_json();
    }

    std::string serialize_document(const document_ptr &document) {
        return to_json(document);
    }

    document_ptr deserialize_document(const std::string &text) {
        return from_json(text);
    }
}