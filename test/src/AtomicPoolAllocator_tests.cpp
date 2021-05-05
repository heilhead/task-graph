#include <catch2/catch.hpp>
#include <array>
#include <AtomicPoolAllocator.h>

static constexpr size_t MAX_ITEMS = 128;

struct alignas(std::hardware_destructive_interference_size) TestStruct {
    int x;
};

TEST_CASE("Initial fill", "[AtomicPoolAllocator]") {
    AtomicPoolAllocator<TestStruct> pool(MAX_ITEMS);

    REQUIRE(pool.size() == MAX_ITEMS);
}

TEST_CASE("Obtain & release", "[AtomicPoolAllocator]") {
    AtomicPoolAllocator<TestStruct> pool(MAX_ITEMS);
    std::vector<TestStruct*> vec;

    while (auto* ptr = pool.obtain()) {
        vec.push_back(ptr);
    }

    REQUIRE(pool.size() == 0);

    for (auto* ptr : vec) {
        pool.release(ptr);
    }

    REQUIRE(pool.size() == MAX_ITEMS);
}

TEST_CASE("Constructors & destructor", "[AtomicPoolAllocator]") {
    static int alive = 0;

    struct CDTestStruct {
        std::array<uint8_t, 64> data;

        CDTestStruct()
            :data {} {
            ++alive;
        }

        ~CDTestStruct() {
            --alive;
        }
    };

    AtomicPoolAllocator<CDTestStruct> pool(MAX_ITEMS);
    std::vector<CDTestStruct*> vec;

    while (auto* ptr = pool.obtain()) {
        vec.push_back(ptr);
    }

    REQUIRE(alive == MAX_ITEMS);

    for (auto* ptr : vec) {
        pool.release(ptr);
    }

    REQUIRE(alive == 0);
}
