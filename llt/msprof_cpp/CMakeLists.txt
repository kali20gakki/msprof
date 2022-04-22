cmake_minimum_required(VERSION 3.14.1)
project(msprof_llt)

set(CMAKE_SKIP_RPATH TRUE)
set(TOP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../..)
set(MSPROF_SOURCE_DIR ${TOP_DIR}/collector)
set(CMAKE_MODULE_PATH
    ${TOP_DIR}/cmake/modules
    ${TOP_DIR}/llt/cmake/modules
)

find_package(protoc)
find_package(ascend_protobuf_shared)
find_package(ascend_protobuf_static)
find_package(gtest)
find_package(mockcpp)

include(${TOP_DIR}/llt/cmake/depend.cmake)

protobuf_generate(profiler
    CppPath
    HeaderPath
    ${MSPROF_SOURCE_DIR}/dvvp/proto/profiler.proto
    ${MSPROF_SOURCE_DIR}/dvvp/proto/profiler_ext.proto
)

# add_subdirectory(ut)
add_subdirectory(st)
