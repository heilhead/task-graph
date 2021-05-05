#pragma once

#include <vector>
#include <thread>
#include "Worker.h"
#include "AtomicPoolAllocator.h"

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
    TaskCallback teardownFn;
    Task* parent;
    Task* next;
    std::atomic<size_t> childTaskCount;
    AtomicPoolAllocator<Task>* pool;

    static constexpr size_t TASK_METADATA_SIZE =
        sizeof(taskFn) + sizeof(teardownFn) + sizeof(parent) + sizeof(next) + sizeof(childTaskCount) + sizeof(pool);
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

    Task& submit();

private:
    template<typename T>
    Task& then(T inTaskFn, Task* parentTask) {
        if (next == nullptr) {
            next = &TaskGraph::allocate(inTaskFn, parentTask);
        } else {
            next->then(inTaskFn, parentTask);
        }

        return *this;
    }
};

static_assert(sizeof(Task) == std::hardware_destructive_interference_size, "invalid task size");

class TaskChainBuilder {
private:
    Task* wrapper;
    Task* first;
    Task* next;

public:
    explicit TaskChainBuilder(Task* parentTask = nullptr);

    template<typename T>
    TaskChainBuilder& add(T taskFn) {
        if (first == nullptr) {
            first = &TaskGraph::allocate(taskFn, wrapper);
            next = first;
        } else {
            next->then(taskFn, wrapper);
        }

        return *this;
    }

    Task& submit();
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
    static Task& allocate(T inTaskFn, Task* parentTask) {
        auto* pool = Worker::getTaskPool();
        auto* task = pool->obtain([](Task& task) {
            const auto& taskFn = task.template getData<T>();
            taskFn(task);
        }, parentTask);

        task->pool = pool;
        task->template constructData<T>(inTaskFn);
        task->setTeardownFunc([](Task& task) {
            task.template destroyData<T>();
        });

        return *task;
    }
};