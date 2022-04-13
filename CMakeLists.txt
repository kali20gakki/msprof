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

########################## securec ###################################
set(SECUREC_DIR ${TOP_DIR}/opensource/securec)
file(GLOB_RECURSE SECUREC_SRC ${SECUREC_DIR}/src/*.c)

add_library(c_sec SHARED
    ${SECUREC_SRC}
)

target_include_directories(c_sec PRIVATE
    ${SECUREC_DIR}/include
)

target_compile_options(c_sec PRIVATE
    -fPIC
    -fstack-protector-all
    -fno-common
    -fno-strict-aliasing
    -Wfloat-equal
    -Wextra
)

target_link_options(c_sec PRIVATE
    -Wl,-z,relro,-z,now,-z,noexecstack
    -s
)

########################## securec ###################################

add_subdirectory(collector)


