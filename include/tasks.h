#pragma once

#include <thread>
#include "taskgraph/TaskGraph.h"

namespace tasks {
    using TaskHandle = PoolItemHandle<Task>;

    TaskGraph* getGraph();
    void init(uint32_t numThreads = std::thread::hardware_concurrency());
    void shutdown();
    void wait(TaskHandle& task);

    template<typename T>
    [[nodiscard]]
    inline TaskHandle create(T taskFn) {
        return TaskGraph::template allocate<T>(taskFn, nullptr);
    }

    template<typename T>
    [[nodiscard]]
    inline TaskHandle create(TaskHandle& parent, T taskFn) {
        return TaskGraph::template allocate<T>(taskFn, &parent);
    }

    template<typename T>
    [[nodiscard]]
    inline TaskHandle create(Task& parent, T taskFn) {
        auto handle = TaskHandle(&parent);
        return TaskGraph::template allocate<T>(taskFn, &handle);
    }

    template<typename T>
    inline TaskHandle add(T taskFn) {
        return create<T>(taskFn)->submit();
    }

    template<typename T>
    inline TaskHandle add(TaskHandle& parent, T taskFn) {
        return create<T>(parent, taskFn)->submit();
    }

    template<typename T>
    inline TaskHandle add(Task& parent, T taskFn) {
        auto handle = TaskHandle(&parent);
        return create<T>(handle, taskFn)->submit();
    }

    [[nodiscard]]
    inline TaskChainBuilder chain() {
        return TaskChainBuilder();
    }

    [[nodiscard]]
    inline TaskChainBuilder chain(TaskHandle& parent) {
        return TaskChainBuilder(parent);
    }

    [[nodiscard]]
    inline TaskChainBuilder chain(Task& parent) {
        return TaskChainBuilder(TaskHandle(&parent));
    }
}