#include <catch2/catch.hpp>
#include <taskgraph/TaskGraph.h>

TEST_CASE("Deque functionality", "[TaskQueue]") {
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
