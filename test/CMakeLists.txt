cmake_minimum_required(VERSION 3.10)
project(taskgraph_tests)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")

add_subdirectory(lib/googletest)

include_directories(${TASKGRAPH_HEADERS_DIR})
include_directories(lib/googletest/googletest/include)

set(SOURCE_FILES main.cpp src/taskgraph_tests.cpp)

add_executable(taskgraph_tests ${SOURCE_FILES})

target_link_libraries(taskgraph_tests taskgraph gtest)
install(TARGETS taskgraph_tests DESTINATION bin)
