#include <iostream>
#include <mutex>
#include <cstdio>
#include "../src/tasks.h"

std::mutex stdOutMutex;

template<typename ...Args>
void msg(Args&& ...args) {
    std::lock_guard lg(stdOutMutex);
    (std::cout << ... << args) << std::endl;
}

void poolAllocTests() {
    struct Test {
        int x, y, z;

        explicit Test(int i) {
            x = i;
//            msg("constructor: ", i);
        }

        ~Test() {
//            msg("destructor: ", x);
        }
    };

    constexpr int ELEMENT_COUNT = 4;

    AtomicPoolAllocator<Test> fl(ELEMENT_COUNT);

    int i = 0;
    std::vector<Test*> elements;
    while (auto* el = fl.obtain(i)) {
        elements.push_back(el);
        i++;
    }
    assert(elements.size() == ELEMENT_COUNT);

    for (auto el : elements) {
        fl.release(el);
    }
    elements.clear();

    while (auto* el = fl.obtain(0)) {
        elements.push_back(el);
    }
    assert(elements.size() == ELEMENT_COUNT);

    msg("freelist tests completed!");
}

void taskGraphTests() {
//    msg("task payload size: ", Task::TASK_PAYLOAD_SIZE);

    tasks::init();

    msg("task graph workers: ", tasks::getGraph().workers.size());

    Task* task;

    {
        struct Test {
            int i;

            Test() {
                i = 0;
            }

            ~Test() {
                msg("test destroyed");
            }
        };

        auto test = std::make_shared<Test>();

        task = &tasks::chain()
            .add([test](auto&) {
                msg("task1: i=", ++test->i);
            })
            .add([test](auto&) {
                msg("task2: i=", ++test->i);
            })
            .add([test](auto&) {
                msg("task3: i=", ++test->i);
            })
            .add([test](auto& task) {
                msg("task4: i=", ++test->i);

                for (auto i = 0u; i < 1000u; i++) {
                    tasks::add(task, [](auto& task) {
//                            msg("task=", (size_t)&task, " thread=", std::this_thread::get_id());
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    });
                }
            })
            .submit();
    }

    tasks::wait(*task);

    auto& graph = tasks::getGraph();
    for (auto& worker : graph.workers) {
        msg("worker=", worker.id, " tasks_completed=", worker.tasksCompleted);
    }

    tasks::shutdown();

    msg("worker tests completed!");
}

void tests::runTests() {
    poolAllocTests();
    taskGraphTests();

    msg("tests completed!");
}
