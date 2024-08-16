#include <catch2/catch.hpp>
#include <components/new_document/document.hpp>
#include <components/new_document/temp_generator/generaty.hpp>

using namespace components::new_document;

TEST_CASE("document_t::value from json") {
    auto json = R"(
{
  "_id": "000000000000000000000001",
  "count": 1,
  "countBool": true,
  "countDouble": 1.1,
  "countStr": "1",
  "countArray": [1, 2, 3, 4, 5],
  "countDict": {
    "even": false,
    "five": false,
    "odd": true,
    "three": false
  }
}
  )";
    auto allocator = std::pmr::new_delete_resource();
    auto doc = document_t::document_from_json(json, allocator);

    REQUIRE(doc->is_exists());
    REQUIRE(doc->is_exists("/count"));
    REQUIRE(doc->is_long("/count"));
    REQUIRE(doc->get_long("/count") == 1);
}

TEST_CASE("document_t::json") {
    auto allocator = std::pmr::new_delete_resource();
    auto doc1 = gen_doc(1, allocator);
    auto json = doc1->to_json();
    auto doc2 = document_t::document_from_json(std::string(json), allocator);

    REQUIRE(doc1->get_string("/_id") == doc2->get_string("/_id"));
    REQUIRE(doc1->get_ulong("/count") == doc2->get_ulong("/count"));
    REQUIRE(doc1->get_string("/countStr") == doc2->get_string("/countStr"));
    REQUIRE(doc1->get_double("/countDouble") == doc2->get_double("/countDouble"));
    REQUIRE(doc1->get_bool("/countBool") == doc2->get_bool("/countBool"));
    REQUIRE(doc1->get_array("/countArray")->count() == doc2->get_array("/countArray")->count());
    REQUIRE(doc1->get_array("/countArray")->get_as<uint64_t>("1") ==
            doc2->get_array("/countArray")->get_as<uint64_t>("1"));
    REQUIRE(doc1->get_dict("/countDict")->count() == doc2->get_dict("/countDict")->count());
    REQUIRE(doc1->get_dict("/countDict")->get_bool("/odd") == doc2->get_dict("/countDict")->get_bool("/odd"));
    REQUIRE(doc1->get_array("/nestedArray")->count() == doc2->get_array("/nestedArray")->count());
    REQUIRE(doc1->get_array("/dictArray")->count() == doc2->get_array("/dictArray")->count());
    REQUIRE(doc1->get_dict("/mixedDict")->count() == doc2->get_dict("/mixedDict")->count());
}

TEST_CASE("document_t::serialize") {
    auto allocator = std::pmr::new_delete_resource();
    auto doc1 = gen_doc(1, allocator);
    auto ser1 = doc1->to_json();
    auto ser2 = doc1->to_binary();

    //! for demonstration:
    REQUIRE(ser1.size() == 663);
    REQUIRE(ser2.size() == 37571);
}
