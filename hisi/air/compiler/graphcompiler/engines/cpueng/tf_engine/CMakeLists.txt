cmake_minimum_required(VERSION 3.14)
project(aicpu_tf_engine)

if (BUILD_OPEN_PROJECT)
    get_filename_component(TOP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../../.." ABSOLUTE)
    set(PROTO_PATH ${TOP_DIR}/third_party/metadef)
else()
    get_filename_component(TOP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../.." ABSOLUTE)
    set(PROTO_PATH ${TOP_DIR}/metadef)
endif()

set(BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(proto_src_files
    ${PROTO_PATH}/proto/tensorflow/tensor.proto
    ${PROTO_PATH}/proto/tensorflow/resource_handle.proto
    ${PROTO_PATH}/proto/tensorflow/tensor_shape.proto
    ${PROTO_PATH}/proto/tensorflow/types.proto
    ${PROTO_PATH}/proto/tensorflow/function.proto
    ${PROTO_PATH}/proto/tensorflow/op_def.proto
    ${PROTO_PATH}/proto/tensorflow/versions.proto
    ${PROTO_PATH}/proto/task.proto
    ${PROTO_PATH}/proto/tensorflow/graph.proto
    ${PROTO_PATH}/proto/tensorflow/node_def.proto
    ${PROTO_PATH}/proto/tensorflow/attr_value.proto
    ${CMAKE_CURRENT_SOURCE_DIR}/proto/fwk_adapter.proto
)
protobuf_generate(aicpu PROTO_SRCS PROTO_HDRS ${proto_src_files})

set (local_lib_src_files
    ${CMAKE_CURRENT_SOURCE_DIR}/engine/tf_engine.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/tf_kernel_info/tf_kernel_info.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/tf_optimizer/tf_optimizer.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/tf_optimizer/tensorflow_util.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/tf_optimizer/tf_optimizer_utils.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/tf_optimizer/tf_function_builder.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/util/tf_util.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/ir2tf/ir2tf_parser_factory.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/ir2tf/ir2tf_base_parser.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/config/ir2tf_json_file.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/tf_kernel_builder/tf_kernel_builder.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/tf_kernel_builder/tf_ops_kernel_builder.cc
    ${PROTO_HDRS}
)

set (local_builder_acl_src_files
    ${CMAKE_CURRENT_SOURCE_DIR}/tf_kernel_builder/tf_kernel_builder.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/tf_kernel_builder/tf_ops_kernel_builder.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/util/tf_util.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/ir2tf/ir2tf_parser_factory.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/ir2tf/ir2tf_base_parser.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/config/ir2tf_json_file.cc
    ${PROTO_HDRS}
)

if (BUILD_OPEN_PROJECT)
  set(local_lib_inc_path
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${TOP_DIR}/compiler/graphcompiler/engines/cpueng
    ${TOP_DIR}/compiler/graphcompiler/engines/cpueng/common
    ${TOP_DIR}/third_party/metadef
    ${TOP_DIR}/third_party/metadef/inc
    ${TOP_DIR}/third_party/metadef/inc/external
    ${TOP_DIR}/third_party/inc/external
    ${TOP_DIR}/third_party/inc
    ${TOP_DIR}/third_party/inc/framework
    ${TOP_DIR}/third_party/inc/ops
    ${C_SEC_INCLUDE}
    ${JSON_INCLUDE}
    ${JSON_INCLUDE_DIR}
    ${Protobuf_INCLUDE}
    ${CMAKE_BINARY_DIR}/proto/aicpu
    ${CMAKE_BINARY_DIR}/proto/aicpu/proto
  )
else()
  set(local_lib_inc_path
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${TOP_DIR}/air/compiler/graphcompiler/engines/cpueng
    ${TOP_DIR}/air/compiler/graphcompiler/engines/cpueng/common
    ${TOP_DIR}/inc
    ${TOP_DIR}/metadef/inc
    ${TOP_DIR}/graphengine/inc
    ${TOP_DIR}/inc/external
    ${TOP_DIR}/libc_sec/include
    ${TOP_DIR}/abl/libc_sec/include
    ${TOP_DIR}/metadef/inc/external
    ${TOP_DIR}/graphengine/inc/external
    ${TOP_DIR}/graphengine/inc/framework
    ${TOP_DIR}/third_party/json/include
    ${TOP_DIR}/open_source/json/include
    ${TOP_DIR}/cann/ops/built-in/op_proto/inc
    ${TOP_DIR}/asl/ops/cann/ops/built-in/op_proto/inc
    ${CMAKE_BINARY_DIR}/proto/aicpu
    ${CMAKE_BINARY_DIR}/proto/aicpu/proto
  )
endif()

add_library(aicpu_tf_engine SHARED
    ${local_lib_src_files}
)
###aicpu_ms_engine
target_link_libraries(aicpu_tf_engine PRIVATE
    $<BUILD_INTERFACE:intf_pub>
    $<$<NOT:$<BOOL:${BUILD_OPEN_PROJECT}>>:$<BUILD_INTERFACE:msprof_headers>>
    $<$<NOT:$<BOOL:${BUILD_OPEN_PROJECT}>>:$<BUILD_INTERFACE:slog_headers>>
    $<$<NOT:$<BOOL:${BUILD_OPEN_PROJECT}>>:$<BUILD_INTERFACE:runtime_headers>>
    $<$<NOT:$<BOOL:${BUILD_OPEN_PROJECT}>>:$<BUILD_INTERFACE:cce_headers>>
    -Wl,--no-as-needed
    ascend_protobuf
    aicpu_engine_common
    slog
    graph
    error_manager
    register
    platform
    runtime
    -Wl,--as-needed
    -lrt
    -ldl
)

target_include_directories(aicpu_tf_engine PRIVATE
     ${local_lib_inc_path}
)

target_compile_definitions(aicpu_tf_engine PRIVATE
    AICPU_PLUGIN
    google=ascend_private
)

#------------atclib/aicpu_tf_engine

target_compile_options(aicpu_tf_engine PRIVATE
    -std=c++11
    -Wno-deprecated-declarations
    -fno-common
    -fno-strict-aliasing
    -Werror
)

set(INSTALL_LIBRARY_DIR lib)

install(TARGETS aicpu_tf_engine OPTIONAL
    EXPORT aicpu_tf_engine-targets
    LIBRARY DESTINATION ${INSTALL_LIBRARY_DIR}
    RUNTIME DESTINATION ${INSTALL_RUNTIME_DIR}
)

set(json_src_path   ${CMAKE_CURRENT_SOURCE_DIR}/config/ir2tf/ir2tf_op_mapping_lib.json)

install(FILES ${json_src_path}
    DESTINATION ${INSTALL_LIBRARY_DIR}
    OPTIONAL
)
