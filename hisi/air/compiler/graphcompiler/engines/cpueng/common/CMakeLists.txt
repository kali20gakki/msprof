cmake_minimum_required(VERSION 3.14)
project(aicpu_engine_common)
if (BUILD_OPEN_PROJECT)
    get_filename_component(TOP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../../.." ABSOLUTE)
    set(PROTO_PATH ${TOP_DIR}/third_party/metadef)
else()
    get_filename_component(TOP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../.." ABSOLUTE)
    set(PROTO_PATH ${TOP_DIR}/metadef)
endif()
set(BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(proto_src_files
    ${PROTO_PATH}/proto/task.proto
)

set(proto_nodedef_src_files
    ${CMAKE_CURRENT_SOURCE_DIR}/../cpu_engine/common/proto/cpu_attr.proto
    ${CMAKE_CURRENT_SOURCE_DIR}/../cpu_engine/common/proto/cpu_node_def.proto
    ${CMAKE_CURRENT_SOURCE_DIR}/../cpu_engine/common/proto/cpu_tensor.proto
    ${CMAKE_CURRENT_SOURCE_DIR}/../cpu_engine/common/proto/cpu_tensor_shape.proto
)

protobuf_generate(aicpu PROTO_SRCS PROTO_HDRS ${proto_src_files})
protobuf_generate(aicpu PROTO_NODEDEF_SRCS PROTO_NODEDEF_HDRS ${proto_nodedef_src_files})

set(local_lib_src_files
    ${CMAKE_CURRENT_SOURCE_DIR}/engine/base_engine.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/aicpu_ops_kernel_info_store/aicpu_ops_kernel_info_store.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/aicpu_ops_kernel_info_store/kernel_info.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/aicpu_graph_optimizer/aicpu_graph_optimizer.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/aicpu_graph_optimizer/optimizer.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/aicpu_graph_optimizer/graph_optimizer_utils.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/config/config_file.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/aicpu_ops_kernel_builder/aicpu_ops_kernel_builder.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/aicpu_ops_kernel_builder/kernel_builder.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/config/ops_json_file.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/error_code/error_code.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/../cpu_engine/common/kernel_builder/cpu_kernel_builder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../cpu_engine/common/optimizer/cpu_optimizer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../cpu_engine/common/util/cpu_engine_util.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/util/util.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/config/ops_parallel_json_file.cpp
    ${PROTO_HDRS}
    ${PROTO_NODEDEF_SRCS}
)

if (BUILD_OPEN_PROJECT)
  set(local_lib_inc_path
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${TOP_DIR}/compiler/graphcompiler/engines/cpueng
    ${TOP_DIR}/compiler/graphcompiler/engines/nneng/inc
    ${TOP_DIR}/compiler/graphcompiler/engines/cpueng/cpu_engine/common/util
    ${TOP_DIR}/third_party/metadef
    ${TOP_DIR}/third_party/metadef/inc
    ${TOP_DIR}/third_party/metadef/inc/external
    ${TOP_DIR}/third_party/inc/external
    ${TOP_DIR}/third_party/inc
    ${TOP_DIR}/third_party/inc/framework
    ${TOP_DIR}/third_party/inc/ops
    ${C_SEC_INCLUDE}
    ${Protobuf_INCLUDE}
    ${AIR_ROOT}/third_party/metadef/cpu_context/inc
    ${CMAKE_BINARY_DIR}/proto/aicpu
    ${CMAKE_BINARY_DIR}/proto/aicpu/proto
  )
else()
  set(local_lib_inc_path
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${TOP_DIR}/air/compiler/graphcompiler/engines/cpueng
    ${TOP_DIR}/air/compiler/graphcompiler/engines/nneng/inc
    ${TOP_DIR}/air/compiler/graphcompiler/engines/cpueng/cpu_engine/common/util
    ${TOP_DIR}/inc
    ${TOP_DIR}/metadef
    ${TOP_DIR}/metadef/inc
    ${TOP_DIR}/graphengine/inc
    ${TOP_DIR}/inc/external
    ${TOP_DIR}/libc_sec/include
    ${TOP_DIR}/metadef/inc/external
    ${TOP_DIR}/graphengine/inc/external
    ${TOP_DIR}/graphengine/inc/framework
    ${TOP_DIR}/cann/ops/built-in/op_proto/inc
    ${TOP_DIR}/asl/ops/cann/ops/built-in/op_proto/inc
    ${CMAKE_BINARY_DIR}/proto/aicpu
    ${CMAKE_BINARY_DIR}/proto/aicpu/proto
  )
endif()


add_library(aicpu_engine_common SHARED
    ${local_lib_src_files}
)

##-----aicpu_engine_common

if (BUILD_OPEN_PROJECT)
target_link_libraries(aicpu_engine_common PRIVATE
    c_sec
    slog
    error_manager
    graph
    platform
    ascend_protobuf
    aicore_utils
    -lrt
    -ldl
    $<BUILD_INTERFACE:intf_pub>
    json
)
else()
target_link_libraries(aicpu_engine_common PRIVATE
    -Wl,--no-as-needed
    -Wl,--as-needed
    c_sec
    slog
    error_manager
    ascend_protobuf
    aicore_utils
    graph
    platform
    -lrt
    -ldl
    $<BUILD_INTERFACE:intf_pub>
    $<BUILD_INTERFACE:msprof_headers>
    $<BUILD_INTERFACE:slog_headers>
    $<BUILD_INTERFACE:runtime_headers>
    $<BUILD_INTERFACE:cce_headers>
    json
)
endif()

##### target atclib/aicpu_engine_common
target_include_directories(aicpu_engine_common PRIVATE
     ${local_lib_inc_path}
)

target_compile_definitions(aicpu_engine_common PRIVATE
    AICPU_PLUGIN
    google=ascend_private
    $<$<STREQUAL:${BUILD_OPEN_PROJECT},True>:ONLY_COMPILE_OPEN_SRC>
)

target_compile_options(aicpu_engine_common PRIVATE
    -std=c++11
    -Wno-deprecated-declarations
    -fno-common
    -fno-strict-aliasing
    -Werror=format
)

set(INSTALL_LIBRARY_DIR lib)


install(TARGETS aicpu_engine_common OPTIONAL
    EXPORT aicpu_engine_common-targets
    LIBRARY DESTINATION ${INSTALL_LIBRARY_DIR}
    RUNTIME DESTINATION ${INSTALL_RUNTIME_DIR}
)

set(init.conf_src_path  ${CMAKE_CURRENT_SOURCE_DIR}/config/init.conf)
install(FILES ${init.conf_src_path}
    DESTINATION ${INSTALL_LIBRARY_DIR}
    OPTIONAL
)

set(ops_parallel_json_src_path   ${CMAKE_CURRENT_SOURCE_DIR}/config/aicpu_ops_parallel_rule.json)
install(FILES ${ops_parallel_json_src_path}
    DESTINATION ${INSTALL_LIBRARY_DIR}
    OPTIONAL
)