#include <cassert>
#include <random>
#include "AtomicPoolAllocator.h"
#include "TaskGraph.h"

Task::Task(Task::TaskCallback inTaskFn, Task* parentTask, Task* nextTask)
    :taskFn { inTaskFn }, teardownFn { nullptr }, parent { parentTask }, next { nextTask }, childTaskCount { 1 },
     pool { nullptr }, payload {} {
    if (parent != nullptr) {
        parent->childTaskCount++;
    }
}

void Task::run() {
    if (taskFn != nullptr) {
        taskFn(*this);
    }

    finish();
}

void Task::finish() {
    size_t remaining = --childTaskCount;
    if (!remaining) {
        if (parent != nullptr) {
            parent->finish();
        }

        if (teardownFn != nullptr) {
            teardownFn(*this);
        }

        if (next != nullptr) {
            next->submit();
        }

        assert(pool != nullptr);
        pool->release(this);
    }
}

bool Task::finished() {
    return childTaskCount == 0;
}

void Task::setTeardownFunc(TaskCallback inTeardownFn) {
    teardownFn = inTeardownFn;
}

Task& Task::submit() {
    TaskGraph::get().getThreadWorker()->submit(this);

    return *this;
}

TaskChainBuilder::TaskChainBuilder(Task* parent) {
    wrapper = gTaskPool.obtain(nullptr, parent);
    wrapper->pool = &gTaskPool;

    next = nullptr;
    first = nullptr;
}

Task& TaskChainBuilder::submit() {
    wrapper->submit();

    if (first != nullptr) {
        first->submit();
    }

    return *wrapper;
}

TaskGraph::TaskGraph(uint32_t numThreads)
    :workers(numThreads) {
    assert(numThreads > 0);

    for (auto i = 1u; i < numThreads; i++) {
        workers[i].start();
    }
}

void TaskGraph::stop() {
    for (auto& worker : workers) {
        worker.stop();
    }

    for (auto& worker : workers) {
        worker.join();
    }
}

Worker* TaskGraph::getRandomWorker() {
    std::uniform_int_distribution<std::size_t> dist { 0, workers.size() - 1 };
    std::default_random_engine randomEngine { std::random_device()() };

    auto idx = dist(randomEngine);
    auto* worker = &workers[idx];
    if (worker->running()) {
        return worker;
    }

    return nullptr;
}

Worker* TaskGraph::getThreadWorker() {
    return getWorker(std::this_thread::get_id());
}

Worker* TaskGraph::getWorker(std::thread::id id) {
    for (auto& worker : workers) {
        if (worker.id == id) {
            return &worker;
        }
    }

    return nullptr;
}

TaskGraph& TaskGraph::get() {
    assert(gInstance);
    return *gInstance;
}

void TaskGraph::init(uint32_t numThreads) {
    assert(!gInstance);
    gInstance = std::make_unique<TaskGraph>(numThreads);
}

void TaskGraph::shutdown() {
    assert(gInstance);
    gInstance->stop();
    gInstance = nullptr;
}

AtomicPoolAllocator<Task>& TaskGraph::getTaskPool() {
    return gTaskPool;
}