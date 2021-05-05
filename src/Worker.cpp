#include <cassert>
#include "Worker.h"
#include "TaskGraph.h"

Worker::Worker()
    :pool(TASK_POOL_SIZE), queue {}, id { std::this_thread::get_id() }, mode { Mode::Foreground },
     state { State::Idle }, stealIndex { 0 }, workerCount { 0 } {
}

Worker::~Worker() {
    if (mode == Mode::Foreground) {
        assert(gThreadWorker != nullptr);
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

bool Worker::running() const {
    return state == State::Running;
}

void Worker::join() {
    if (thread.joinable()) {
        thread.join();
    }
}

void Worker::submit(Task* task) {
    queue.push(task);
}

void Worker::wait(Task* task) {
    assert(mode == Mode::Foreground);

    state = State::Running;
    while (!task->finished()) {
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

void Worker::run() {
    gThreadWorker = this;
    id = thread.get_id();

    if (state == State::Idle) {
        state = State::Running;
    }

    while (state == State::Running) {
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
    gThreadWorker = nullptr;
}

Task* Worker::fetchTask() {
    Task* task = queue.pop();
    if (task != nullptr) {
        return task;
    }

    auto& workers = TaskGraph::get().workers;
    for (auto i = 0u; i < workerCount; i++) {
        auto idx = (i + stealIndex) % workerCount;
        auto& worker = workers[idx];
        if (worker.running()) {
            task = worker.queue.steal();
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

AtomicPoolAllocator<Task>* Worker::getTaskPool() {
    assert(gThreadWorker != nullptr);
    return &gThreadWorker->pool;
}
