#pragma once

#include <thread>
#include "TaskQueue.h"

class Task;

class Worker {
public:
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

    uint32_t tasksCompleted = 0;

private:
    TaskQueue queue;
    std::atomic<Mode> mode;

public:
    explicit Worker()
        :queue {}, id { std::this_thread::get_id() }, mode { Mode::Foreground }, state { State::Idle } {
    }

    void start();
    void stop();
    bool running() const;
    void join();
    void submit(Task* task);
    void wait(Task* task);

private:
    void run();
    Task* fetchTask();
};