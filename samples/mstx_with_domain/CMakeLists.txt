#Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.

# CMake lowest version requirement
cmake_minimum_required(VERSION 3.14)

project(MSTX_WITH_DOMAIN)

# Compile options
add_compile_options(-std=c++11)
add_compile_options(-D_FORTIFY_SOURCE=2 -O2)

# 设置编译选项
set(CMAKE_SKIP_RPATH TRUE)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY  "./")
set(CMAKE_CXX_FLAGS_DEBUG "-fPIC -O0 -g -Wall")
set(CMAKE_CXX_FLAGS_RELEASE "-fPIC -O2 -Wall")

add_executable(
    mstx_with_domain
    mstx_with_domain.cpp
)

if(DEFINED ENV{ASCEND_HOME_PATH})
    set(ASCEND_PATH $ENV{ASCEND_HOME_PATH})
else()
    set(ASCEND_PATH "/usr/local/Ascend/ascend-toolkit/latest")
endif()

set(INCLUDE_BASE_DIR ${CMAKE_SOURCE_DIR}/..)
set(ASCEND_INCLUDE_BASE_DIR "${ASCEND_PATH}/include")
include_directories(
    ${INCLUDE_BASE_DIR}
    ${INCLUDE_BASE_DIR}/common
    ${ASCEND_INCLUDE_BASE_DIR}
    ${ASCEND_INCLUDE_BASE_DIR}/acl
    ${ASCEND_INCLUDE_BASE_DIR}/aclnn
)

# 设置链接的库文件路径
target_link_libraries(
    mstx_with_domain PRIVATE
    ${ASCEND_PATH}/lib64/libascendcl.so
    ${ASCEND_PATH}/lib64/libnnopbase.so
    ${ASCEND_PATH}/lib64/libmsprofiler.so
    ${ASCEND_PATH}/lib64/libopapi.so
    ${ASCEND_PATH}/lib64/libc_sec.so
    dl
)

target_compile_options(
    mstx_with_domain PRIVATE
    -fPIC
    -fstack-protector-all
    -ftrapv
)

target_link_options(
    mstx_with_domain PRIVATE
    -Wl,-z,relro,-z,now,-z,noexecstack
    -s
)

install(TARGETS mstx_with_domain DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
