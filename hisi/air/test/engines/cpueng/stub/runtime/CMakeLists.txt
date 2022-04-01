cmake_minimum_required(VERSION 3.14.1)
project(runtime_aicpu_stub)
get_filename_component(TOP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../../.." ABSOLUTE)
set(LOCAL_SRC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/runtime_stub.cpp
)

add_library(runtime_aicpu_stub SHARED
    ${LOCAL_SRC_FILES}
)

#[[set(engine_common_includes
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${TOP_DIR}/compiler/graphcompiler/engines/cpueng
    ${TOP_DIR}/compiler/graphcompiler/engines/cpueng/common
    ${TOP_DIR}/third_party/metadef
    ${TOP_DIR}/third_party/metadef/inc
    ${TOP_DIR}/third_party/metadef/inc/external
    ${TOP_DIR}/third_party/metadef/inc/graph
    ${TOP_DIR}/third_party/inc/external
    ${TOP_DIR}/third_party/inc
    ${TOP_DIR}/third_party/inc/framework
    ${TOP_DIR}/third_party/inc/framework/common/
    ${TOP_DIR}/third_party/canndev/ops/built-in/op_proto/inc
    #${TOP_DIR}/output/c_sec/include
    ${C_SEC_INCLUDE}
    #${TOP_DIR}/output/json
    ${JSON_INCLUDE}
    #${TOP_DIR}/output/protobuf/include
    ${Protobuf_INCLUDE}
    #${AICPU_OPP_PATH}/inc
    ${TOP_DIR}/third_party/metadef/cpu_context/inc
    ${CMAKE_BINARY_DIR}/proto/aicpu
    ${CMAKE_BINARY_DIR}/proto/aicpu/proto
  )]]


set(LOCAL_C_INCLUDES
    ${TOP_DIR}/third_party/inc
    ${TOP_DIR}/third_party/inc/external
    ${TOP_DIR}/third_party/inc/framework
    ${TOP_DIR}/third_party/inc/cce
    #${TOP_DIR}/libc_sec/include
    ${C_SEC_INCLUDE}
    ${TOP_DIR}/third_party/inc/framework/common
    ${TOP_DIR}/third_party/metadef/third_party/fwkacllib/inc/
    #${TOP_DIR}/framework/domi
    #${TOP_DIR}/framework/domi/common

)

target_compile_options(runtime_aicpu_stub PRIVATE
    -std=c++11
    -fPIC
)

target_include_directories(runtime_aicpu_stub PRIVATE
    ${LOCAL_C_INCLUDES}
)

target_link_libraries(runtime_aicpu_stub PRIVATE
    c_sec
    -ldl
    -lrt
    $<BUILD_INTERFACE:intf_pub>

)
