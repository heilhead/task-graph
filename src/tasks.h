#pragma once

#include <thread>
#include "TaskGraph.h"

namespace tasks {
    TaskGraph& getGraph();
    void init(uint32_t numThreads = std::thread::hardware_concurrency());
    void shutdown();
    void wait(Task& task);

    template<typename T>
    inline Task& create(T taskFn) {
        return TaskGraph::template allocate<T>(taskFn, nullptr);
    }

    template<typename T>
    inline Task& create(Task& parent, T taskFn) {
        return TaskGraph::template allocate<T>(taskFn, &parent);
    }

    template<typename T>
    inline Task& add(T taskFn) {
        return create(taskFn).submit();
    }

    template<typename T>
    inline Task& add(Task& parent, T taskFn) {
        return create(parent, taskFn).submit();
    }

    inline TaskChainBuilder chain() {
        return TaskChainBuilder(nullptr);
    }

    inline TaskChainBuilder chain(Task& parent) {
        return TaskChainBuilder(&parent);
    }
}