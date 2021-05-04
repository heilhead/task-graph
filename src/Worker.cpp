#include <cassert>
#include "Worker.h"
#include "TaskGraph.h"

void Worker::start() {
    thread = std::move(std::thread(&Worker::run, this));
    id = thread.get_id();
    mode = Mode::Background;
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
            tasksCompleted++;
        } else {
            std::this_thread::yield();
        }
    }

    state = State::Idle;
}

void Worker::run() {
    if (state == State::Idle) {
        state = State::Running;
    }

    while (state == State::Running) {
        if (Task* nextTask = fetchTask()) {
            nextTask->run();
            tasksCompleted++;
        } else {
            std::this_thread::yield();
        }
    }

    state = State::Idle;
}

Task* Worker::fetchTask() {
    if (auto* task = queue.pop()) {
        return task;
    }

    auto* worker = TaskGraph::get().getRandomWorker();
    if (worker != nullptr && worker != this) {
        return worker->queue.steal();
    }

    return nullptr;
}