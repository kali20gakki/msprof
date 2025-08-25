#Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.

# CMake lowest version requirement
cmake_minimum_required(VERSION 3.14)

project(MSPTI_HCCL_ACTIVITY)

# Compile options
add_compile_options(-std=c++11)

# 设置编译选项
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY  "./")
set(CMAKE_CXX_FLAGS_DEBUG "-fPIC -O0 -g -Wall")
set(CMAKE_CXX_FLAGS_RELEASE "-fPIC -O2 -Wall")

add_executable(mspti_hccl_activity_test
               mspti_hccl_activity.cpp) 

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
    ${ASCEND_INCLUDE_BASE_DIR}/mspti
)

# 设置链接的库文件路径
target_link_libraries(mspti_hccl_activity_test PRIVATE
                      ${ASCEND_PATH}/lib64/libascendcl.so
                      ${ASCEND_PATH}/lib64/libnnopbase.so
                      ${ASCEND_PATH}/lib64/libopapi.so
                      ${ASCEND_PATH}/lib64/libhccl.so
                      ${ASCEND_PATH}/lib64/libmspti.so
                      ${ASCEND_PATH}/lib64/libc_sec.so
                      dl
                      -lpthread)

target_compile_options(
    mspti_hccl_activity_test PRIVATE
    -fPIC
    -fstack-protector-all
    -ftrapv
    -Werror=uninitialized
    -Werror=return-type
    -Wall
)

target_link_options(
    mspti_hccl_activity_test PRIVATE
    -Wl,-z,relro,-z,now,-z,noexecstack
    -s
)

install(TARGETS mspti_hccl_activity_test DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
