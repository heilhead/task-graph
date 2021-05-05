#include <catch2/catch.hpp>
#include <tasks.h>
#include <iostream>

TEST_CASE("Task graph", "[tasks]") {
    static bool bTestAlive = false;

    struct Test {
        std::atomic<int> x;
        std::atomic<int> y;

        Test()
            :x { 0 }, y { 0 } {
            bTestAlive = true;
        }

        ~Test() {
            bTestAlive = false;
        }
    };

    {
        auto test = std::make_shared<Test>();

        {
            tasks::init();

            REQUIRE(TaskGraph::getThreadWorker() != nullptr);

            auto& task = tasks::chain()
                .add([test](auto&) {
                    REQUIRE(++test->x == 1);
                })
                .add([test](auto&) {
                    REQUIRE(++test->x == 2);
                })
                .add([test](auto&) {
                    REQUIRE(++test->x == 3);
                })
                .add([test](auto& task) {
                    REQUIRE(++test->x == 4);

                    for (auto i = 0u; i < 1000u; i++) {
                        tasks::add(task, [test](auto&) {
                            ++test->y;
                            std::this_thread::sleep_for(std::chrono::milliseconds(1));
                        });
                    }
                })
                .submit();

            tasks::wait(task);

            for (auto& worker : tasks::getGraph().workers) {
                REQUIRE(worker.getTaskPool()->size() == Worker::TASK_POOL_SIZE);
            }

            tasks::shutdown();
        }

        REQUIRE(test->x == 4);
        REQUIRE(test->y == 1000);
    }

    REQUIRE(!bTestAlive);
}
