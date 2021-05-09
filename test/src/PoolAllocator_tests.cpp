#include <array>
#include <catch2/catch.hpp>
#include <taskgraph/PoolAllocator.h>

static constexpr size_t MAX_ITEMS = 128;

struct alignas(std::hardware_destructive_interference_size) TestStruct {
    int x;
};

TEST_CASE("Pool item pointer arithmetics", "[PoolAllocator]") {
    PoolItem<uint64_t> poolItem;
    *poolItem.data() = 123;

    REQUIRE(PoolItem<uint64_t>::fromData(poolItem.data()) == &poolItem);
    REQUIRE(PoolItem<uint64_t>::fromData(poolItem.data())->data() == poolItem.data());
    REQUIRE(*PoolItem<uint64_t>::fromData(poolItem.data())->data() == 123);
}

TEST_CASE("Initial fill", "[AtomicPoolAllocator]") {
    PoolAllocator<TestStruct> pool(MAX_ITEMS);

    REQUIRE(pool.size() == MAX_ITEMS);
    REQUIRE(pool.capacity() == MAX_ITEMS);
}

TEST_CASE("Obtain & release", "[PoolAllocator]") {
    PoolAllocator<TestStruct> pool(MAX_ITEMS);
    std::vector<PoolItem<TestStruct>*> vec;

    PoolItem<TestStruct>* handle { nullptr };
    while (handle = pool.obtain()) {
        vec.push_back(handle);
    }

    REQUIRE(pool.size() == 0);

    for (auto& h : vec) {
        pool.release(h);
    }

    REQUIRE(pool.size() == MAX_ITEMS);
}

TEST_CASE("Constructors & destructor", "[PoolAllocator]") {
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

    PoolAllocator<CDTestStruct> pool(MAX_ITEMS);
    std::vector<PoolItem<CDTestStruct>*> vec;

    while (PoolItem<CDTestStruct>* handle = pool.obtain()) {
        vec.push_back(handle);
    }

    REQUIRE(alive == MAX_ITEMS);

    for (auto& handle : vec) {
        pool.release(handle);
    }

    REQUIRE(alive == 0);
}
