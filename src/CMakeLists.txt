cmake_minimum_required(VERSION 3.10)
project(taskgraph CXX)

set(SOURCE_FILES
        taskgraph/PoolAllocator.h
        taskgraph/TaskGraph.cpp taskgraph/TaskGraph.h
        taskgraph/TaskQueue.cpp taskgraph/TaskQueue.h
        taskgraph/Worker.cpp taskgraph/Worker.h
        taskgraph/utils.h
        tasks.cpp tasks.h)

add_library(taskgraph SHARED STATIC ${SOURCE_FILES})
target_compile_definitions(taskgraph PUBLIC "$<$<CONFIG:DEBUG>:DEBUG>")

install(TARGETS taskgraph DESTINATION ${TASKGRAPH_INSTALL_LIB_DIR})
install(FILES
        taskgraph/PoolAllocator.h
        taskgraph/TaskGraph.h
        taskgraph/TaskQueue.h
        taskgraph/Worker.h
        taskgraph/utils.h
        DESTINATION
        ${TASKGRAPH_INSTALL_INCLUDE_DIR}/taskgraph)
install(FILES tasks.h DESTINATION ${TASKGRAPH_INSTALL_INCLUDE_DIR})
