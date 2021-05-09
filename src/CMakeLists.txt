cmake_minimum_required(VERSION 3.10)
project(taskgraph C CXX)

set(SOURCE_FILES
        PoolAllocator.h
        TaskGraph.cpp TaskGraph.h
        TaskQueue.cpp TaskQueue.h
        tasks.cpp tasks.h
        Worker.cpp Worker.h
        )

add_library(taskgraph SHARED STATIC ${SOURCE_FILES})
target_compile_definitions(taskgraph PUBLIC "$<$<CONFIG:DEBUG>:DEBUG>")

install(TARGETS taskgraph DESTINATION ${TASKGRAPH_INSTALL_LIB_DIR})
install(FILES PoolAllocator.h TaskGraph.h TaskQueue.h tasks.h Worker.h DESTINATION ${TASKGRAPH_INSTALL_INCLUDE_DIR})
