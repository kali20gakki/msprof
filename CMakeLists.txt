# msprofbin libmsprofiler.so stub/libmsprofiler.so
cmake_minimum_required(VERSION 3.14.1)
project(msprof)

set(CMAKE_SKIP_RPATH TRUE)
set(TOP_DIR ${CMAKE_CURRENT_LIST_DIR})

set(CMAKE_MODULE_PATH
    ${TOP_DIR}/cmake/modules
)

find_package(protoc)
find_package(ascend_protobuf_shared)
find_package(ascend_protobuf_static)

########################## securec ###################################
add_subdirectory(collector)
