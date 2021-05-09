#include <cassert>
#include "PoolAllocator.h"
#include "TaskGraph.h"

Task::Task(Task::TaskCallback inTaskFn, Task* parentTask, Task* nextTask)
    :taskFn { inTaskFn }, parent { parentTask }, next { nextTask }, childTaskCount { 1 }, payload {} {
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

        auto* poolItem = PoolItem<Task>::fromData(this);
        auto* pool = PoolAllocator<Task>::fromItem(poolItem);
        pool->release(poolItem);
    }
}

bool Task::finished() const {
    // @FIXME calling this from foreground thread may attempt to read child task count after the task has been returned to the pool and possibly reused.
    return childTaskCount == 0;
}

void Task::setTeardownFunc(TaskCallback inTeardownFn) {
    teardownFn = inTeardownFn;
}

PoolItemHandle<Task> Task::submit() {
    auto* worker = TaskGraph::getThreadWorker();
    assert(worker != nullptr);
    PoolItemHandle<Task> handle(this);
    worker->submit(handle);
    return handle;
}


TaskChainBuilder::TaskChainBuilder() {
    auto* item = Worker::getTaskPool()->obtain();
    wrapper = PoolItemHandle<Task>(item);
}

TaskChainBuilder::TaskChainBuilder(PoolItemHandle<Task> parent) {
    auto* item = Worker::getTaskPool()->obtain(nullptr, *parent);
    wrapper = PoolItemHandle<Task>(item);
}

PoolItemHandle<Task> TaskChainBuilder::submit() {
    wrapper->submit();

    if (first) {
        first->submit();
    }

    return wrapper;
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
