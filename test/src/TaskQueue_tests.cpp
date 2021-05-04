#include <catch2/catch.hpp>
#include <TaskGraph.h>
#include <TaskQueue.h>

static constexpr size_t MAX_ITEMS = 128;
struct TestStruct {
    std::array<uint8_t, 64> data;
};

TEST_CASE("Deque functionality", "[TaskQueueTest]") {
    TaskQueue queue;
    std::vector<Task> tasks { 3 };

    REQUIRE(queue.size() == 0);

    queue.push(&tasks[0]);
    queue.push(&tasks[1]);

    REQUIRE(queue.size() == 2);

    queue.push(&tasks[2]);

    // N.B. LIFO for pop(), FIFO for steal()
    REQUIRE(queue.pop() == &tasks[2]);
    REQUIRE(queue.steal() == &tasks[0]);
    REQUIRE(queue.pop() == &tasks[1]);

    REQUIRE(queue.size() == 0);

    REQUIRE(queue.pop() == nullptr);
    REQUIRE(queue.steal() == nullptr);

    queue.push(&tasks[0]);

    REQUIRE(queue.size() == 1);
    REQUIRE(queue.steal() == &tasks[0]);
}
