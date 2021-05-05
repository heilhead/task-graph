#include <cassert>
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

bool Task::finished() const {
    // @FIXME calling this from foreground thread may attempt to read child task count after the task has been returned to the pool and possibly reused.
    return childTaskCount == 0;
}

void Task::setTeardownFunc(TaskCallback inTeardownFn) {
    teardownFn = inTeardownFn;
}

Task& Task::submit() {
    auto* worker = TaskGraph::getThreadWorker();
    assert(worker != nullptr);
    worker->submit(this);
    return *this;
}

TaskChainBuilder::TaskChainBuilder(Task* parent) {
    auto* pool = Worker::getTaskPool();
    wrapper = pool->obtain(nullptr, parent);
    wrapper->pool = pool;

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

    workers[0].start(0, numThreads, Worker::Mode::Foreground);
    for (auto i = 1u; i < numThreads; i++) {
        workers[i].start(i, numThreads, Worker::Mode::Background);
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

Worker* TaskGraph::getWorker(std::thread::id id) {
    for (auto& worker : workers) {
        if (worker.id == id) {
            return &worker;
        }
    }

    return nullptr;
}

Worker* TaskGraph::getThreadWorker() {
    return Worker::getThreadWorker();
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
