#include "taskgraph/TaskQueue.h"

void TaskQueue::push(Task* task) {
    int b = bottom;
    tasks[b & TASK_LOOKUP_MASK] = task;
    bottom = b + 1;
}

Task* TaskQueue::pop() {
    int b = std::max(0, bottom - 1);
    bottom = b;
    int t = top;

    if (t <= b) {
        auto* task = tasks[b & TASK_LOOKUP_MASK];
        if (t != b) {
            return task;
        }

        auto tExpected = t;
        auto tDesired = t + 1;

        if (!top.compare_exchange_strong(tExpected, tDesired)) {
            task = nullptr;
        }

        bottom = t + 1;
        return task;
    }

    bottom = t;
    return nullptr;
}

Task* TaskQueue::steal() {
    int t = top;
    int b = bottom;
    if (t < b) {
        auto* task = tasks[t & TASK_LOOKUP_MASK];

        auto tExpected = t;
        auto tDesired = t + 1;

        if (!top.compare_exchange_strong(tExpected, tDesired)) {
            return nullptr;
        }

        return task;
    }

    return nullptr;
}

int TaskQueue::size() {
    return std::max(0, bottom - top);
}
