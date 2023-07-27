#include "generaty.hpp"

document::retained_t<document::impl::array_t> gen_array(int num) {
    auto array = document::impl::mutable_array_t::new_array();
    for (int i = 0; i < 5; ++i) {
        array->append(num + i);
    }
    return array;
}

document::retained_t<document::impl::dict_t> gen_dict(int num) {
    auto dict = document::impl::mutable_dict_t::new_dict();
    dict->set("odd", num % 2 != 0);
    dict->set("even", num % 2 == 0);
    dict->set("three", num % 3 == 0);
    dict->set("five", num % 5 == 0);
    return dict;
}

std::string gen_id(int num) {
    auto res = std::to_string(num);
    while (res.size() < 24) {
        res = "0" + res;
    }
    return res;
}

document_ptr gen_doc(int num) {
    auto doc = document::impl::mutable_dict_t::new_dict();
    doc->set("_id", gen_id(num));
    doc->set("count", num);
    doc->set("countStr", std::to_string(num));
    doc->set("countDouble", float(num) + 0.1);
    doc->set("countBool", num % 2 != 0);
    doc->set("countArray", gen_array(num));
    doc->set("countDict", gen_dict(num));
    auto array = document::impl::mutable_array_t::new_array();
    for (int i = 0; i < 5; ++i) {
        array->append(gen_array(num + i));
    }
    doc->set("nestedArray", array);
    array = document::impl::mutable_array_t::new_array();
    for (int i = 0; i < 5; ++i) {
        auto dict = document::impl::mutable_dict_t::new_dict();
        dict->set("number", num + i);
        array->append(dict);
    }
    doc->set("dictArray", array);
    auto dict = document::impl::mutable_dict_t::new_dict();
    for (int i = 0; i < 5; ++i) {
        auto number = std::to_string(num + i);
        dict->set(number, gen_dict(num + i));
    }
    doc->set("mixedDict", dict);
    return make_document(doc);
}

document_ptr gen_number_doc(int64_t num) {
    auto array = document::impl::mutable_array_t::new_array().detach();
    array->append(num);
    return make_document(array);
}


document_ptr gen_special_doc(char type) {
    auto array = document::impl::mutable_array_t::new_array().detach();
    switch (type){
        case 't': //true
            array->append(document::impl::value_t::true_value);
            break;
        case 'f': //false
            array->append(document::impl::value_t::false_value);
            break;
        case 'u': //undefined - abandoned
            array->append(document::impl::value_t::undefined_value);
            break;
        case 'n': //null
        default:
            array->append(document::impl::value_t::null_value);
    }
    return make_document(array);
}

