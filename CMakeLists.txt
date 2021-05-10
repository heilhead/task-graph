cmake_minimum_required(VERSION 3.10)
project(taskgraph)

option(TASKGRAPH_BuildTests OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR})

set(TASKGRAPH_INSTALL_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)
set(TASKGRAPH_INSTALL_BIN_DIR ${PROJECT_SOURCE_DIR}/bin)
set(TASKGRAPH_INSTALL_LIB_DIR ${PROJECT_SOURCE_DIR}/lib)

set(SOURCE_FILES
        include/taskgraph/PoolAllocator.h
        include/taskgraph/TaskGraph.h
        include/taskgraph/TaskQueue.h
        include/taskgraph/utils.h
        include/taskgraph/Worker.h
        include/tasks.h
        src/taskgraph/TaskGraph.cpp
        src/taskgraph/TaskQueue.cpp
        src/taskgraph/Worker.cpp
        src/tasks.cpp)

add_library(${PROJECT_NAME} SHARED STATIC ${SOURCE_FILES})

target_compile_definitions(${PROJECT_NAME} PUBLIC "$<$<CONFIG:DEBUG>:DEBUG>")
target_include_directories(${PROJECT_NAME} PUBLIC ${TASKGRAPH_INSTALL_INCLUDE_DIR})

if (TASKGRAPH_BuildTests)
    add_subdirectory(test)
endif ()