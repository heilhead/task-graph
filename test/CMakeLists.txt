cmake_minimum_required(VERSION 3.10)
project(taskgraph_tests)

include_directories(lib)

set(SOURCE_FILES
        lib/catch2/catch.cpp
        lib/catch2/catch.hpp
        src/main.cpp
        src/PoolAllocator_tests.cpp
        src/TaskQueue_tests.cpp
        src/tasks_tests.cpp)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})
target_compile_definitions(${PROJECT_NAME} PUBLIC "$<$<CONFIG:DEBUG>:DEBUG>")

target_link_libraries(${PROJECT_NAME} taskgraph)
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
