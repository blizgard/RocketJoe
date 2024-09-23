#include <catch2/catch.hpp>

#include <cstddef>
#include <memory_resource>
#include <random>

#include "core/buffer.hpp"

std::size_t gen_size() {
    std::default_random_engine generator;

    auto constexpr range_min{1000};
    auto constexpr range_max{100000};
    std::uniform_int_distribution<std::size_t> distribution(range_min, range_max);
    std::size_t size = distribution(generator);
    return size;
}

void sequence(core::buffer& buffer) {
    auto* ptr = static_cast<char*>(buffer.data());
    auto size = buffer.size();
    for (int i = 0; i < size; ++i) {
        ptr[i] = i;
    }
}

bool equal(core::buffer& buffer1, core::buffer& buffer2) {
    bool result = false;
    if (buffer1.data() == nullptr && buffer2.data() == nullptr && buffer1.size() != buffer2.size()) {
        return result;
    }

    result = std::equal(static_cast<char*>(buffer1.data()),
                        static_cast<char*>(buffer1.data()) + buffer1.size(),
                        static_cast<char*>(buffer2.data()),
                        static_cast<char*>(buffer2.data()) + buffer1.size());

    return result;
}

TEST_CASE("empty buffer") {
    auto mr = std::pmr::synchronized_pool_resource();
    core::buffer buff(&mr, 0);
    REQUIRE(buff.is_empty());
}

TEST_CASE("memory resource") {
    auto mr = std::pmr::synchronized_pool_resource();
    const auto size = gen_size();
    core::buffer buff(&mr, size);
    REQUIRE(nullptr != buff.data());
    REQUIRE(size == buff.size());
    REQUIRE(size == buff.ssize());
    REQUIRE(size == buff.capacity());
    REQUIRE(&mr == buff.memory_resource());
    REQUIRE(mr.is_equal(*buff.memory_resource()));
}

TEST_CASE("copy from raw pointer") {
    auto mr = std::pmr::synchronized_pool_resource();
    auto size = gen_size();
    void* device_memory = malloc(size);
    REQUIRE(device_memory != nullptr);
    core::buffer buff(&mr, device_memory, size);
    REQUIRE(nullptr != buff.data());
    REQUIRE(size == buff.size());
    REQUIRE(size == buff.capacity());
    REQUIRE(&mr == buff.memory_resource());
    free(device_memory);
}

TEST_CASE("copy from nullptr") {
    auto mr = std::pmr::synchronized_pool_resource();
    core::buffer buff(&mr, nullptr, 0);
    REQUIRE(nullptr == buff.data());
    REQUIRE(0 == buff.size());
    REQUIRE(0 == buff.capacity());
    REQUIRE(&mr == buff.memory_resource());
}

TEST_CASE("copy constructor") {
    auto mr = std::pmr::synchronized_pool_resource();
    const auto size = 200;
    core::buffer buff(&mr, size);

    sequence(buff);

    core::buffer buff_copy(&mr, buff); // uses default MR
    REQUIRE(nullptr != buff_copy.data());
    REQUIRE(buff.data() != buff_copy.data());
    REQUIRE(buff.size() == buff_copy.size());
    REQUIRE(buff.capacity() == buff_copy.capacity());
    REQUIRE(buff_copy.memory_resource() == &mr);
    REQUIRE(buff_copy.memory_resource()->is_equal(mr));

    REQUIRE(equal(buff, buff_copy));

    core::buffer buff_copy2(buff.memory_resource(), buff);
    REQUIRE(buff_copy2.memory_resource() == buff.memory_resource());
    REQUIRE(buff_copy2.memory_resource()->is_equal(*buff.memory_resource()));

    REQUIRE(equal(buff, buff_copy));
}

TEST_CASE("copy capacity larger than size") {
    auto mr = std::pmr::synchronized_pool_resource();
    auto size = 200;
    core::buffer buff(&mr, size);
    auto new_size = size - 1;
    buff.resize(new_size);

    sequence(buff);

    core::buffer buff_copy(&mr, buff);
    REQUIRE(nullptr != buff_copy.data());
    REQUIRE(buff.data() != buff_copy.data());
    REQUIRE(buff.size() == buff_copy.size());
    REQUIRE(new_size == buff_copy.capacity());
    REQUIRE(buff_copy.memory_resource() == &mr);
    REQUIRE(buff_copy.memory_resource()->is_equal(mr));
    REQUIRE(equal(buff, buff_copy));
}

TEST_CASE("copy constructor explicit memory resource") {
    auto mr = std::pmr::synchronized_pool_resource();
    auto size = 200;
    core::buffer buff(&mr, size);

    sequence(buff);

    core::buffer buff_copy(&mr, buff);
    REQUIRE(nullptr != buff_copy.data());
    REQUIRE(buff.data() != buff_copy.data());
    REQUIRE(buff.size() == buff_copy.size());
    REQUIRE(buff.capacity() == buff_copy.capacity());
    REQUIRE(buff.memory_resource() == buff_copy.memory_resource());
    REQUIRE(buff.memory_resource()->is_equal(*buff_copy.memory_resource()));
    REQUIRE(equal(buff, buff_copy));
}

TEST_CASE("copy capacity larger than size explicit memory resource") {
    auto mr = std::pmr::synchronized_pool_resource();
    auto size = 200;
    core::buffer buff(&mr, size);

    auto new_size = size - 1;
    buff.resize(new_size);

    sequence(buff);

    core::buffer buff_copy(&mr, buff);
    REQUIRE(nullptr != buff_copy.data());
    REQUIRE(buff.data() != buff_copy.data());
    REQUIRE(buff.size() == buff_copy.size());

    REQUIRE(new_size == buff_copy.capacity());
    REQUIRE(buff.capacity() != buff_copy.capacity());
    REQUIRE(buff.memory_resource() == buff_copy.memory_resource());
    REQUIRE(buff.memory_resource()->is_equal(*buff_copy.memory_resource()));

    REQUIRE(equal(buff, buff_copy));
}

TEST_CASE("move constructor") {
    auto mr_tmp = std::pmr::synchronized_pool_resource();
    const auto size_tmp = gen_size();

    core::buffer buff(&mr_tmp, size_tmp);
    auto* ptr = buff.data();
    auto size = buff.size();
    auto capacity = buff.capacity();
    auto* mr = buff.memory_resource();

    core::buffer buff_new(std::move(buff));
    REQUIRE(nullptr != buff_new.data());
    REQUIRE(ptr == buff_new.data());
    REQUIRE(size == buff_new.size());
    REQUIRE(capacity == buff_new.capacity());
    REQUIRE(mr == buff_new.memory_resource());

    REQUIRE(nullptr == buff.data());
    REQUIRE(0 == buff.size());
    REQUIRE(0 == buff.capacity());
    REQUIRE(nullptr != buff.memory_resource());
}

TEST_CASE("move assignment to default") {
    auto mr_tmp = std::pmr::synchronized_pool_resource();
    const auto size_tmp = gen_size();

    core::buffer src(&mr_tmp, size_tmp);
    auto* ptr = src.data();
    auto size = src.size();
    auto capacity = src.capacity();
    auto* mr = src.memory_resource();

    core::buffer dest(&mr_tmp);
    dest = std::move(src);

    REQUIRE(nullptr != dest.data());
    REQUIRE(ptr == dest.data());
    REQUIRE(size == dest.size());
    REQUIRE(capacity == dest.capacity());
    REQUIRE(mr == dest.memory_resource());

    REQUIRE(nullptr == src.data());
    REQUIRE(0 == src.size());
    REQUIRE(0 == src.capacity());
    REQUIRE(nullptr != src.memory_resource());
}

TEST_CASE("move assignment") {
    auto mr_tmp = std::pmr::synchronized_pool_resource();
    const auto size_tmp = gen_size();

    core::buffer src(&mr_tmp, size_tmp);
    auto* ptr = src.data();
    auto size = src.size();
    auto capacity = src.capacity();
    auto* mr = src.memory_resource();

    core::buffer dest(mr, size - 1);
    dest = std::move(src);

    REQUIRE(nullptr != dest.data());
    REQUIRE(ptr == dest.data());
    REQUIRE(size == dest.size());
    REQUIRE(capacity == dest.capacity());
    REQUIRE(mr == dest.memory_resource());

    REQUIRE(nullptr == src.data());
    REQUIRE(0 == src.size());
    REQUIRE(0 == src.capacity());
    REQUIRE(nullptr != src.memory_resource());
}

TEST_CASE("self move assignment") {
    auto mr_tmp = std::pmr::synchronized_pool_resource();
    const auto size_tmp = gen_size();

    core::buffer buff(&mr_tmp, size_tmp);
    auto* ptr = buff.data();
    auto size = buff.size();
    auto capacity = buff.capacity();
    auto* mr = buff.memory_resource();

    buff = std::move(buff);
    REQUIRE(nullptr != buff.data());
    REQUIRE(ptr == buff.data());
    REQUIRE(size == buff.size());
    REQUIRE(capacity == buff.capacity());
    REQUIRE(mr == buff.memory_resource());
}

TEST_CASE("resize smaller") {
    auto mr = std::pmr::synchronized_pool_resource();
    const auto size = 200;

    core::buffer buff(&mr, size);

    sequence(buff);

    auto* old_data = buff.data();
    core::buffer old_content(&mr, old_data, buff.size());

    auto new_size = size - 1;
    buff.resize(new_size);
    REQUIRE(new_size == buff.size());
    REQUIRE(size == buff.capacity());
    REQUIRE(old_data == buff.data());

    buff.shrink_to_fit();
    REQUIRE(nullptr != buff.data());
    // A reallocation should have occured
    REQUIRE(old_data != buff.data());
    REQUIRE(new_size == buff.size());
    REQUIRE(buff.capacity() == buff.size());
    REQUIRE(equal(buff, old_content));
}

TEST_CASE("resize bigger") {
    auto mr_tmp = std::pmr::synchronized_pool_resource();
    const auto size_tmp = gen_size();

    core::buffer buff(&mr_tmp, size_tmp);
    auto* old_data = buff.data();
    auto new_size = size_tmp + 1;
    buff.resize(new_size);
    REQUIRE(new_size == buff.size());
    REQUIRE(new_size == buff.capacity());
    REQUIRE(old_data != buff.data());
}

TEST_CASE("reserve smaller") {
    auto mr_tmp = std::pmr::synchronized_pool_resource();
    const auto size_tmp = gen_size();

    core::buffer buff(&mr_tmp, size_tmp);
    auto* const old_data = buff.data();
    auto const old_capacity = buff.capacity();
    auto const new_capacity = buff.capacity() - 1;
    buff.reserve(new_capacity);
    REQUIRE(size_tmp == buff.size());
    REQUIRE(old_capacity == buff.capacity());
    REQUIRE(old_data == buff.data());
}

TEST_CASE("reserve bigger") {
    auto mr_tmp = std::pmr::synchronized_pool_resource();
    const auto size_tmp = gen_size();

    core::buffer buff(&mr_tmp, size_tmp);
    auto* const old_data = buff.data();
    auto const new_capacity = buff.capacity() + 1;
    buff.reserve(new_capacity);
    REQUIRE(size_tmp == buff.size());
    REQUIRE(new_capacity == buff.capacity());
    REQUIRE(old_data != buff.data());
}
