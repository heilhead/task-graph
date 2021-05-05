#pragma once

#include <thread>
#include "AtomicPoolAllocator.h"
#include "TaskQueue.h"

class Task;

class Worker {
public:
    static constexpr size_t TASK_POOL_SIZE = 4096u;

    enum class Mode {
        Background,
        Foreground
    };

    enum class State {
        Idle = 0,
        Running = 1,
        Stopping = 2
    };

    std::thread::id id;
    std::thread thread;
    std::atomic<State> state;

#if DEBUG
    uint32_t tasksCompleted = 0;
#endif

private:
    TaskQueue queue;
    std::atomic<Mode> mode;
    AtomicPoolAllocator<Task> pool;
    size_t stealIndex;
    size_t workerCount;

public:
    Worker();
    ~Worker();

    void start(size_t inIndex, size_t inWorkerCount, Mode inMode);
    void stop();
    bool running() const;
    void join();
    void submit(Task* task);
    void wait(Task* task);

    static Worker* getThreadWorker();
    static AtomicPoolAllocator<Task>* getTaskPool();

private:
    void run();
    Task* fetchTask();
};

namespace {
    thread_local Worker* gThreadWorker = nullptr;
}