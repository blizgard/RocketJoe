#include <catch2/catch.hpp>

#include <iostream>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#include <components/serialization/deserialization.hpp>
#include <components/serialization/serialization.hpp>

using components::serialization::deserialize;
using components::serialization::deserialize_array;
using components::serialization::serialize;
using components::serialization::serialize_array;
using components::serialization::context::json_context;

TEST_CASE("base container") {
    std::string json_str;

    {
        json_context json;
        auto boolean = true;
        std::vector<int64_t> vector{1, 2, 3};
        std::map<int64_t, int64_t> map = {{1, 2}, {3, 4}};
        std::string str("42");
        std::string_view str1("42");
        std::int64_t number = 42;
        serialize_array(json, 6);
        serialize(json, boolean);
        serialize(json, number);
        serialize(json, str);
        serialize(json, str1);
        serialize(json, vector);
        serialize(json, map);
        std::cerr << json.data() << std::endl;
        json_str = json.data();
    }

    {
        json_context json(json_str);
        auto boolean = false;
        std::vector<int64_t> vector;
        std::map<int64_t, int64_t> map;
        std::string str;
        std::string str1;
        std::int64_t number = 0;
        deserialize_array(json, 6);
        deserialize(json, boolean);
        deserialize(json, number);
        deserialize(json, str);
        deserialize(json, str1);
        deserialize(json, vector);
        deserialize(json, map);
        REQUIRE(true == boolean);
        REQUIRE(number == 42);
        std::cerr << "asd" << str << std::endl;
        REQUIRE(str1 == "42");
        REQUIRE(str == "42");
        REQUIRE(vector == std::vector<int64_t>{1, 2, 3});
        REQUIRE(map == std::map<int64_t, int64_t>{{1, 2}, {3, 4}});
    }
}