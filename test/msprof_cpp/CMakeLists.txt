cmake_minimum_required(VERSION 3.14.1)
project(msprof_llt)
add_definitions(-DMSPORF_LLT)
set(CMAKE_SKIP_RPATH TRUE)
set(TOP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../..)
set(MSPROF_SOURCE_DIR ${TOP_DIR}/collector)
set(CMAKE_MODULE_PATH
    ${TOP_DIR}/cmake/modules
    ${TOP_DIR}/test/cmake/modules
)

find_package(protoc)
find_package(ascend_protobuf_shared)
find_package(ascend_protobuf_static)
find_package(gtest)
find_package(mockcpp)

include(${TOP_DIR}/test/cmake/depend.cmake)

protobuf_generate(profiler
    CppPath
    HeaderPath
    ${MSPROF_SOURCE_DIR}/dvvp/proto/profiler.proto
    ${MSPROF_SOURCE_DIR}/dvvp/proto/profiler_ext.proto
)

if ((NOT DEFINED PACKAGE) OR (PACKAGE STREQUAL "ut"))
    add_subdirectory(ut)
else()
    add_subdirectory(st)
endif()