cmake_minimum_required(VERSION 3.10)
project(taskgraph_tests)

include_directories(${TASKGRAPH_HEADERS_DIR})
include_directories(lib)

set(SOURCE_FILES src/main.cpp
        lib/catch2/catch.hpp lib/catch2/catch.cpp
        src/PoolAllocator_tests.cpp
        src/TaskQueue_tests.cpp
        src/tasks_tests.cpp)

add_executable(taskgraph_tests ${SOURCE_FILES})
target_compile_definitions(taskgraph_tests PUBLIC "$<$<CONFIG:DEBUG>:DEBUG>")

target_link_libraries(taskgraph_tests taskgraph)
install(TARGETS taskgraph_tests DESTINATION bin)
