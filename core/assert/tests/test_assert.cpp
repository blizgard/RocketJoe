#include <catch2/catch.hpp>

#include "core/assert/assert.hpp"

TEST_CASE("assert test ok") {
    //REQUIRE_NOTHROW([&]() { assertion_failed(true); }());
    //REQUIRE_NOTHROW([&]() { assertion_failed_msg(true, "ok"); }());
    REQUIRE_NOTHROW([&]() { assertion_exception_msg(true, "ok"); }());
    REQUIRE_NOTHROW([&]() { assertion_exception(true); }());
}

TEST_CASE("assert test string_view") {
    std::string_view message = "Testing";
    //REQUIRE_NOTHROW([&]() { assertion_failed_msg(true, message); }());
    REQUIRE_NOTHROW([&]() { assertion_exception_msg(true, message); }());
    REQUIRE_NOTHROW([&]() { assertion_exception(true); }());
}