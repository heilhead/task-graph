#pragma once

#include <array>
#include <atomic>

class Task;

class TaskQueue {
private:
    static constexpr uint32_t MAX_TASK_COUNT = 4096u;
    static constexpr uint32_t TASK_LOOKUP_MASK = MAX_TASK_COUNT - 1u;

    static_assert((MAX_TASK_COUNT != 0) && ((MAX_TASK_COUNT & (MAX_TASK_COUNT - 1)) == 0));

    std::array<Task*, MAX_TASK_COUNT> tasks;
    std::atomic<int> top;
    std::atomic<int> bottom;

public:
    TaskQueue()
        :top { 0 }, bottom { 0 }, tasks { nullptr } { }

    void push(Task* task);
    Task* pop();
    Task* steal();
};