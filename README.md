# TaskGraph

A lambda-based lock-free work-stealing task graph.

## Installation

TaskGraph's CMake build exports an interface target `taskgraph`.

If TaskGraph has been installed on the system:

```cmake
find_package(taskgraph REQUIRED)
target_link_libraries(your-app taskgraph)
```

This target is also provided when TaskGraph is used as a subdirectory.
Assuming that TaskGraph has been cloned to `lib/task-graph`:

```cmake
add_subdirectory(lib/task-graph)
target_link_libraries(your-app taskgraph)
```

## Usage

The task graph needs to be initialized before adding tasks and shut down
when there's no more work to keep worker threads from spinning:

```cpp
#include <iostream>
#include <tasks.h>

int main() {
    tasks::init();
    
    // Create a task and submit it for execution.
    auto task = tasks::add([](auto&) {
        std::cout << "hello from task graph!" << std::endl;
    });
    
    // Block the main thread until the task has executed.
    tasks::wait(task);

    tasks::shutdown();
}
```

Task handling lambda receives the current task as parameter so that subtasks
can be added, in which case the task will only be considered finished once all
of its subtasks are executed:

```cpp
auto task = tasks::add([](auto& task) {
    tasks::add(task, [](auto& task) {
        tasks::add(task, [](auto&) {
            // ...
        });
    });      
});
```

It's possible to create task chains where each next task is submitted for execution
as soon as the previous has executed:

```cpp
auto task = tasks::chain()
    ->add([](auto&) {
        // ...
    })
    ->add([](auto& task) {
        // Chains can also be added as subtasks.
        tasks::chain(task)
            ->add([](auto&) {
                // ...
            })
            ->submit();
    })
    ->submit();

// Block until the entire chain has executed.
tasks::wait(task);
```

`tasks::TaskHandle` returned is safe to copy and pass around, as well as query
for task completion:

```cpp
tasks::TaskHandle handle = tasks::add([](auto&) {
    // ...
});

// Use implicit (bool) conversion or `handle.valid()` method to check whether
// the task has executed.
while (handle) {
    // Perform work on the foregroud thread while waiting for the task to finish. 
}
```

## Issues

The library was developed as a research project and may contain issues and bugs,
which will likely never be fixed.

## License

MIT