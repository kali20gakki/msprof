cmake_minimum_required(VERSION 3.14.1)
project(collector_fuzz)
add_definitions(-DCOLLECTOR_FUZZ)
set(CMAKE_SKIP_RPATH TRUE)
set(TOP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../..)
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

add_compile_options(
    -g
    -fPIC
    -Wall
    -Winvalid-pch
    -fms-extensions
    -std=c++11
    -O0
    -fno-omit-frame-pointer
    -ftest-coverage
    -fprofile-arcs
    -fdump-rtl-expand
    -Wno-pmf-conversions
    -D_GLIBCXX_USE_CXX11_ABI=0
    # for asan 
    -fsanitize=address 
    -fsanitize=undefined
    -fsanitize-coverage=trace-pc,trace-cmp
)

add_link_options(
    -fsanitize=address 
    -fsanitize=undefined
    -fsanitize-coverage=trace-pc,trace-cmp 
)

set(SECODEFUZZ_HEADER_DIR ${TOP_DIR}/test/opensource/secodefuzz/Secodefuzz)
set(LIB_SECODE_FUZZ ${TOP_DIR}/test/opensource/secodefuzz/examples/out-bin-x64/libSecodefuzz.a)

include_directories(
    ${SECODEFUZZ_HEADER_DIR}
    ${TOP_DIR}/opensource/json/include
    ${TOP_DIR}/platform/securec/include
    ${CMAKE_BINARY_DIR}/proto/profiler
)

link_libraries(asan ubsan gcov pthread c_sec_static ${GTEST_STATIC_LIBRARY})

add_subdirectory(prof_api)
