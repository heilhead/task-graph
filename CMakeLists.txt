cmake_minimum_required(VERSION 3.10)
project(taskgraph)

option(TASKGRAPH_BuildTests OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR})

set(TASKGRAPH_INSTALL_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)
set(TASKGRAPH_INSTALL_BIN_DIR ${PROJECT_SOURCE_DIR}/bin)
set(TASKGRAPH_INSTALL_LIB_DIR ${PROJECT_SOURCE_DIR}/lib)
set(TASKGRAPH_HEADERS_DIR ${PROJECT_SOURCE_DIR}/src)

if (TASKGRAPH_BuildTests)
    include_directories(${TASKGRAPH_HEADERS_DIR})
    add_subdirectory(test)
else ()
    include_directories(${TASKGRAPH_INSTALL_INCLUDE_DIR})
endif ()

add_subdirectory(src)
