#include <cassert>
#include "Worker.h"
#include "TaskGraph.h"

Worker::Worker()
    :pool(TASK_POOL_SIZE), queue {}, id { std::this_thread::get_id() }, mode { Mode::Foreground },
     state { State::Idle }, stealIndex { 0 }, workerCount { 0 } {
}

Worker::~Worker() {
    if (mode == Mode::Foreground) {
        gThreadWorker = nullptr;
    }
}

void Worker::start(size_t inIndex, size_t inWorkerCount, Mode inMode) {
    mode = inMode;
    stealIndex = inIndex;
    workerCount = inWorkerCount;

    if (mode == Mode::Foreground) {
        assert(gThreadWorker == nullptr);
        gThreadWorker = this;
    } else {
        thread = std::move(std::thread(&Worker::run, this));
    }
}

void Worker::stop() {
    state = State::Stopping;
}

void Worker::join() {
    if (thread.joinable()) {
        thread.join();
    }
}

void Worker::submit(PoolItemHandle<Task>& task) {
    queue.push(*task);
}

void Worker::wait(PoolItemHandle<Task>& task) {
    assert(mode == Mode::Foreground);

    state = State::Running;
    while (task.valid()) {
        if (Task* nextTask = fetchTask()) {
            nextTask->run();

#if DEBUG
            tasksCompleted++;
#endif
        } else {
            std::this_thread::yield();
        }
    }
    state = State::Idle;
}

void Worker::clear() {
    assert(mode == Mode::Foreground);
    assert(gThreadWorker == this);

    while(auto* task = fetchTask()) {
        task->finish();
    }
}

void Worker::run() {
    gThreadWorker = this;
    id = thread.get_id();

    if (state == State::Idle) {
        state = State::Running;
    }

    while (state == State::Running) {
        if (auto* nextTask = fetchTask()) {
            nextTask->run();

#if DEBUG
            tasksCompleted++;
#endif
        } else {
            std::this_thread::yield();
        }
    }

    state = State::Idle;
    gThreadWorker = nullptr;
}

Task* Worker::fetchTask() {
    Task* task = queue.pop();
    if (task != nullptr) {
        return task;
    }

    auto* taskGraph = TaskGraph::get();
    if (taskGraph != nullptr) {
        auto& workers = taskGraph->workers;
        for (auto i = 0u; i < workerCount; i++) {
            auto idx = (i + stealIndex) % workerCount;
            task = workers[idx].queue.steal();
            if (task != nullptr) {
                stealIndex = idx;
                return task;
            }
        }
    }

    return nullptr;
}

Worker* Worker::getThreadWorker() {
    return gThreadWorker;
}

PoolAllocator<Task>* Worker::getTaskPool() {
    assert(gThreadWorker != nullptr);
    return &gThreadWorker->pool;
}
