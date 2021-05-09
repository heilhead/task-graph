#pragma once

#include <vector>
#include <thread>
#include "Worker.h"
#include "PoolAllocator.h"

class TaskGraph;

namespace {
    std::unique_ptr<TaskGraph> gInstance;
}

class Task {
    friend class TaskChainBuilder;
    friend class TaskGraph;

public:
    using TaskCallback = void (*)(Task&);

private:
    TaskCallback taskFn;
    TaskCallback teardownFn = nullptr;
    Task* parent;
    Task* next;
    std::atomic<size_t> childTaskCount;

    static constexpr size_t TASK_METADATA_SIZE =
        sizeof(taskFn) + sizeof(teardownFn) + sizeof(parent) + sizeof(next) + sizeof(childTaskCount);
    static constexpr size_t TASK_PAYLOAD_SIZE = std::hardware_destructive_interference_size - TASK_METADATA_SIZE;

    std::array<uint8_t, TASK_PAYLOAD_SIZE> payload;

public:
    explicit Task(TaskCallback inTaskFn = nullptr, Task* parentTask = nullptr, Task* nextTask = nullptr);

    void run();
    void finish();
    bool finished() const;
    void setTeardownFunc(TaskCallback inTeardownFn);

    template<typename T, typename... Args>
    void constructData(Args&& ... args) {
        constexpr auto size = sizeof(T);

        if constexpr (size <= TASK_PAYLOAD_SIZE) {
            new(payload.data())T(std::forward<Args>(args)...);
        } else {
            *(T**)(payload.data()) = new T(std::forward<Args>(args)...);
        }
    }

    template<typename T>
    void destroyData() {
        constexpr auto size = sizeof(T);

        if constexpr (size <= TASK_PAYLOAD_SIZE) {
            T* ptr = (T*)payload.data();
            ptr->~T();
        } else {
            T* ptr = *(T**)payload.data();
            ptr->~T();
        }
    }

    template<typename T>
    std::enable_if_t<!std::is_trivially_copyable_v<T>, const T&> getData() {
        constexpr auto size = sizeof(T);

        if constexpr (size <= TASK_PAYLOAD_SIZE) {
            return *(T*)payload.data();
        } else {
            return *(T**)payload.data();
        }
    }

    template<typename T>
    std::enable_if_t<std::is_trivially_copyable_v<T> && (sizeof(T) <= TASK_PAYLOAD_SIZE)>
    setData(const T& data) {
        memcpy(payload.data(), &data, sizeof(data));
    }

    template<typename T>
    std::enable_if_t<std::is_trivially_copyable_v<T> && (sizeof(T) <= TASK_PAYLOAD_SIZE), const T&> getData() {
        return *(T*)payload.data();
    }

    PoolItemHandle<Task> submit();

private:
    template<typename T>
    Task* then(T inTaskFn, PoolItemHandle<Task>* parentTask) {
        if (next == nullptr) {
            next = *TaskGraph::allocate(inTaskFn, parentTask);
        } else {
            next->then(inTaskFn, parentTask);
        }

        return this;
    }
};

static_assert(sizeof(Task) == std::hardware_destructive_interference_size, "invalid task size");

class TaskChainBuilder {
private:
    PoolItemHandle<Task> wrapper;
    PoolItemHandle<Task> first;
    PoolItemHandle<Task> next;

public:
    TaskChainBuilder();
    explicit TaskChainBuilder(PoolItemHandle<Task> parentTask);

    template<typename T>
    TaskChainBuilder* add(T taskFn) {
        if (!first) {
            first = TaskGraph::allocate(taskFn, &wrapper);
            next = first;
        } else {
            next->then(taskFn, &wrapper);
        }

        return this;
    }

    PoolItemHandle<Task> submit();

    TaskChainBuilder* operator->() {
        return this;
    }
};

class TaskGraph {
public:
    std::vector<Worker> workers;

public:
    explicit TaskGraph(uint32_t numThreads);

    void stop();

    Worker* getWorker(std::thread::id id);

    static Worker* getThreadWorker();
    static TaskGraph& get();
    static void init(uint32_t numThreads);
    static void shutdown();

    template<typename T>
    static PoolItemHandle<Task> allocate(T inTaskFn, PoolItemHandle<Task>* parentTaskHandle) {
        auto* pool = Worker::getTaskPool();
        auto* parentTask = parentTaskHandle != nullptr ? parentTaskHandle->data() : nullptr;
        auto* item = pool->obtain([](Task& task) {
            const auto& taskFn = task.template getData<T>();
            taskFn(task);
        }, parentTask);

        // @TODO Throw if `item == nullptr` (pool is empty).

        auto* task = item->data();
        task->template constructData<T>(inTaskFn);
        task->setTeardownFunc([](Task& task) {
            task.template destroyData<T>();
        });

        return PoolItemHandle<Task>(item);
    }
};