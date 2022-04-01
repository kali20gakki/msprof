project(aicpu_engine_stest)
get_filename_component(TOP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../.." ABSOLUTE)
set(BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(PROTO_PATH ${TOP_DIR}/third_party/metadef)

execute_process(
    COMMAND cp -rf ${CMAKE_CURRENT_SOURCE_DIR}/../stub/config ${CMAKE_CURRENT_BINARY_DIR}/../st
    COMMAND cp -rf ${CMAKE_CURRENT_SOURCE_DIR}/../stub/config ${CMAKE_CURRENT_BINARY_DIR}/../stub
)

set(proto_src_files
    ${PROTO_PATH}/proto/tensorflow/types.proto
    ${PROTO_PATH}/proto/tensorflow/graph.proto
    ${PROTO_PATH}/proto/tensorflow/node_def.proto
    ${PROTO_PATH}/proto/tensorflow/attr_value.proto
    ${PROTO_PATH}/proto/tensorflow/tensor.proto
    ${PROTO_PATH}/proto/tensorflow/resource_handle.proto
    ${PROTO_PATH}/proto/tensorflow/tensor_shape.proto
    ${PROTO_PATH}/proto/tensorflow/function.proto
    ${PROTO_PATH}/proto/tensorflow/op_def.proto
    ${PROTO_PATH}/proto/tensorflow/versions.proto
    ${PROTO_PATH}/proto/task.proto
    ${TOP_DIR}/compiler/graphcompiler/engines/cpueng/tf_engine/proto/fwk_adapter.proto
)

set(proto_context_src_files
    ${TOP_DIR}/compiler/graphcompiler/engines/cpueng/cpu_engine/common/proto/cpu_attr.proto
    ${TOP_DIR}/compiler/graphcompiler/engines/cpueng/cpu_engine/common/proto/cpu_attr.proto
    ${TOP_DIR}/compiler/graphcompiler/engines/cpueng/cpu_engine/common/proto/cpu_node_def.proto
    ${TOP_DIR}/compiler/graphcompiler/engines/cpueng/cpu_engine/common/proto/cpu_tensor.proto
    ${TOP_DIR}/compiler/graphcompiler/engines/cpueng/cpu_engine/common/proto/cpu_tensor_shape.proto
)

protobuf_generate(aicpu PROTO_SRCS PROTO_HDRS ${proto_src_files})
protobuf_generate(aicpu PROTO_CONTEXT_SRCS PROTO_CONTEXT_HDRS ${proto_context_src_files})

set(engine_common_src_path ${TOP_DIR}/compiler/graphcompiler/engines/cpueng/common)

set(engine_common_src_files
    ${engine_common_src_path}/aicpu_ops_kernel_info_store/aicpu_ops_kernel_info_store.cc
    ${engine_common_src_path}/aicpu_ops_kernel_info_store/kernel_info.cc
    ${engine_common_src_path}/aicpu_graph_optimizer/aicpu_graph_optimizer.cc
    ${engine_common_src_path}/aicpu_graph_optimizer/graph_optimizer_utils.cc
    ${engine_common_src_path}/aicpu_graph_optimizer/optimizer.cc
    ${engine_common_src_path}/aicpu_ops_kernel_builder/aicpu_ops_kernel_builder.cc
    ${engine_common_src_path}/aicpu_ops_kernel_builder/kernel_builder.cc
    ${engine_common_src_path}/config/config_file.cc
    ${engine_common_src_path}/config/ops_json_file.cc
    ${engine_common_src_path}/error_code/error_code.cc
    ${engine_common_src_path}/engine/base_engine.cc
    ${engine_common_src_path}/util/util.cc
    ${TOP_DIR}/compiler/graphcompiler/engines/cpueng/cpu_engine/common/util/cpu_engine_util.cc
    ${engine_common_src_path}/config/ops_parallel_json_file.cpp
    ${PROTO_HDRS}
    ${PROTO_CONTEXT_SRCS}
)

set(engine_common_stub_src_path "${CMAKE_CURRENT_SOURCE_DIR}/..")
set(engine_common_stub_src_files
    ${engine_common_stub_src_path}/stub/platform_info_stub.cpp
    ${engine_common_stub_src_path}/stub/ops_kernel_builder_registry_stub.cpp
)

set(engine_common_src_test_files
    st_main.cpp
)

set(engine_common_includes
    ${TOP_DIR}/test/engines/cpueng/stub/
    ${TOP_DIR}/compiler/graphcompiler/engines/cpueng
    ${TOP_DIR}/compiler/graphcompiler/engines/cpueng/common
    ${TOP_DIR}/compiler/graphcompiler/engines/nneng/inc
    ${TOP_DIR}/third_party/metadef
    ${TOP_DIR}/third_party/metadef/inc
    ${TOP_DIR}/third_party/metadef/inc/external
    ${TOP_DIR}/third_party/metadef/inc/graph
    ${TOP_DIR}/third_party/inc/external
    ${TOP_DIR}/third_party/inc
    ${TOP_DIR}/third_party/inc/framework
    ${TOP_DIR}/third_party/inc/framework/common/
    ${TOP_DIR}/compiler/graphcompiler/engines/cpueng/cpu_engine/common/util
    ${C_SEC_INCLUDE}
    ${JSON_INCLUDE}
    ${JSON_INCLUDE_DIR}
    ${Protobuf_INCLUDE}
    ${CMAKE_BINARY_DIR}/proto/aicpu
    ${CMAKE_BINARY_DIR}/proto/aicpu/proto
)

set(cpueng_src_path ${TOP_DIR}/compiler/graphcompiler/engines/cpueng)

set(tf_engine_src_files
    ${cpueng_src_path}/tf_engine/engine/tf_engine.cc
    ${cpueng_src_path}/tf_engine/tf_kernel_info/tf_kernel_info.cc
    ${cpueng_src_path}/tf_engine/tf_optimizer/tf_optimizer.cc
    ${cpueng_src_path}/tf_engine/tf_optimizer/tf_optimizer_utils.cc
    ${cpueng_src_path}/tf_engine/tf_optimizer/tensorflow_util.cc
    ${cpueng_src_path}/tf_engine/tf_optimizer/tf_function_builder.cc
    ${cpueng_src_path}/tf_engine/tf_kernel_builder/tf_kernel_builder.cc
    ${cpueng_src_path}/tf_engine/tf_kernel_builder/tf_ops_kernel_builder.cc
    ${cpueng_src_path}/tf_engine/util/tf_util.cc
    ${cpueng_src_path}/tf_engine/ir2tf/ir2tf_parser_factory.cc
    ${cpueng_src_path}/tf_engine/ir2tf/ir2tf_base_parser.cc
    ${cpueng_src_path}/tf_engine/config/ir2tf_json_file.cc
    ${PROTO_HDRS}
)

set(tf_engine_src_test_files
    tf_engine/tf_engine_st.cpp
    tf_engine/tf_kernel_info_st.cpp
    tf_engine/tf_optimizer_st.cpp
    tf_engine/tf_kernel_builder_st.cpp
    tf_engine/tf_ops_kernel_builder_st.cpp
    tf_engine/ir2tf_st.cpp
)

set(tf_engine_includes
    ${TOP_DIR}/compiler/graphcompiler/engines/cpueng/tf_engine/
    ${TOP_DIR}/compiler/graphcompiler/engines/nneng/inc
    ${CMAKE_BINARY_DIR}/proto/aicpu/proto
    ${CMAKE_BINARY_DIR}/proto/aicpu/
)

set(cpu_engine_src_files
    ${cpueng_src_path}/cpu_engine/aicpu_engine/engine/aicpu_engine.cpp
    ${cpueng_src_path}/cpu_engine/aicpu_engine/kernel_info/aicpu_kernel_info.cpp
    ${cpueng_src_path}/cpu_engine/aicpu_engine/kernel_info/aicpu_cust_kernel_info.cpp
    ${cpueng_src_path}/cpu_engine/aicpu_engine/optimizer/aicpu_optimizer.cpp
    ${cpueng_src_path}/cpu_engine/aicpu_engine/kernel_builder/aicpu_kernel_builder.cpp
    ${cpueng_src_path}/cpu_engine/aicpu_engine/kernel_builder/aicpu_ascend_ops_kernel_builder.cpp
    ${cpueng_src_path}/cpu_engine/common/kernel_builder/cpu_kernel_builder.cpp
    ${cpueng_src_path}/cpu_engine/common/optimizer/cpu_optimizer.cpp
)

set(engine_common_src_test_files st_main.cpp)

set(cpu_engine_includes
    ${TOP_DIR}/test/engines/cpueng/stub/
    ${TOP_DIR}/compiler/graphcompiler/engines/cpueng/cpu_engine/
    ${TOP_DIR}/compiler/graphcompiler/engines/cpueng
    ${TOP_DIR}/compiler/graphcompiler/engines/cpueng/common
    ${TOP_DIR}/compiler/graphcompiler/engines/nneng/inc
    ${TOP_DIR}/third_party/inc/external
    ${TOP_DIR}/third_party/inc
    ${TOP_DIR}/third_party/inc/framework
    ${TOP_DIR}/third_party/inc/framework/common/
    ${TOP_DIR}/third_party/metadef
    ${TOP_DIR}/third_party/metadef/inc
    ${TOP_DIR}/third_party/metadef/inc/external
    ${TOP_DIR}/third_party/metadef/inc/graph
    ${TOP_DIR}/compiler/graphcompiler/engines/cpueng/cpu_engine/common/util
    ${C_SEC_INCLUDE}
    ${JSON_INCLUDE}
    ${JSON_INCLUDE_DIR}
    ${Protobuf_INCLUDE}
    ${GTEST_INCLUDE}
    ${TOP_DIR}/third_party/inc/ops
    ${CMAKE_BINARY_DIR}/proto/aicpu
    ${CMAKE_BINARY_DIR}/proto/aicpu/proto
)

set(cpu_engine_src_test_files
    cpu_engine/aicpu_engine_st.cpp
    cpu_engine/aicpu_kernel_info_st.cpp
    cpu_engine/aicpu_cust_kernel_builder_st.cpp
    cpu_engine/aicpu_optimizer_st.cpp
    cpu_engine/aicpu_kernel_builder_st.cpp
)

set(host_engine_src_files
    ${cpueng_src_path}/cpu_engine/hostcpu_engine/engine/hostcpu_engine.cpp
    ${cpueng_src_path}/cpu_engine/hostcpu_engine/kernel_info/hostcpu_kernel_info.cpp
    ${cpueng_src_path}/cpu_engine/hostcpu_engine/optimizer/hostcpu_optimizer.cpp
    ${cpueng_src_path}/cpu_engine/hostcpu_engine/kernel_builder/hostcpu_kernel_builder.cpp
    ${cpueng_src_path}/cpu_engine/hostcpu_engine/kernel_builder/hostcpu_ops_kernel_builder.cpp
    ${cpueng_src_path}/cpu_engine/common/kernel_builder/cpu_kernel_builder.cpp
    ${cpueng_src_path}/cpu_engine/common/optimizer/cpu_optimizer.cpp
)

set(host_engine_src_test_files
    cpu_engine/hostcpu_engine_st.cpp
    cpu_engine/hostcpu_kernel_builder_st.cpp
    cpu_engine/hostcpu_kernel_info_st.cpp
)

add_executable(tf_engine_stest
    ${engine_common_src_files}
    ${engine_common_src_test_files}
    ${tf_engine_src_files}
    ${tf_engine_src_test_files}
    ${engine_common_stub_src_files}
)

add_executable(cpu_engine_stest
    ${engine_common_src_files}
    ${engine_common_src_test_files}
    ${cpu_engine_src_files}
    ${cpu_engine_src_test_files}
    ${engine_common_stub_src_files}
)

add_executable(host_engine_stest
    ${engine_common_src_files}
    ${engine_common_src_test_files}
    ${host_engine_src_files}
    ${host_engine_src_test_files}
    ${engine_common_stub_src_files}
)

target_include_directories(tf_engine_stest PRIVATE
    ${engine_common_includes}
    ${tf_engine_includes}
    ${cpu_engine_includes}
)

target_compile_definitions(tf_engine_stest PRIVATE
    RUN_TEST
    google=ascend_private
    $<$<STREQUAL:${BUILD_OPEN_PROJECT},True>:ONLY_COMPILE_OPEN_SRC>
)

target_compile_options(tf_engine_stest PRIVATE
    -std=c++11
)

target_link_options(tf_engine_stest PRIVATE
    -Wl,-rpath,${ASCEND_ATC_DIR}
)

target_link_options(cpu_engine_stest PRIVATE
    -Wl,-rpath,${ASCEND_ATC_DIR}
)

target_link_options(host_engine_stest PRIVATE
    -Wl,-rpath,${ASCEND_ATC_DIR}
)

target_link_libraries(tf_engine_stest PRIVATE
    $<BUILD_INTERFACE:intf_pub>
    -Wl,--as-needed
    gtest
    stub_for_aicpu
    graph
    c_sec
    error_manager
    register
    ascend_protobuf
    json
    runtime_aicpu_stub
    -Wl,--no-as-needed
    slog
    -ldl
    -lrt
    -lpthread
    -lgcov
)

target_include_directories(cpu_engine_stest PRIVATE
    ${engine_common_includes}
    ${cpu_engine_includes}
)

target_compile_definitions(cpu_engine_stest PRIVATE
    RUN_TEST
    google=ascend_private
    $<$<STREQUAL:${BUILD_OPEN_PROJECT},True>:ONLY_COMPILE_OPEN_SRC>
)

target_compile_options(cpu_engine_stest PRIVATE
    -std=c++11
)

target_link_libraries(cpu_engine_stest PRIVATE
    $<BUILD_INTERFACE:intf_pub>
    -Wl,--as-needed
    stub_for_aicpu
    error_manager
    graph
    register
    gtest
    ascend_protobuf
    c_sec
    json
    runtime_aicpu_stub
    -Wl,--no-as-needed
    slog
    -ldl
    -lrt
    -lpthread
    -lgcov
)

target_include_directories(host_engine_stest PRIVATE
    ${engine_common_includes}
    ${cpu_engine_includes}
)

target_compile_definitions(host_engine_stest PRIVATE
    RUN_TEST
    google=ascend_private
    $<$<STREQUAL:${BUILD_OPEN_PROJECT},True>:ONLY_COMPILE_OPEN_SRC>
)

target_compile_options(host_engine_stest PRIVATE
    -std=c++11
)

target_link_libraries(host_engine_stest PRIVATE
    $<BUILD_INTERFACE:intf_pub>
    -Wl,--as-needed
    stub_for_aicpu
    error_manager
    graph
    register
    gtest
    ascend_protobuf
    c_sec
    json
    runtime_aicpu_stub
    -Wl,--no-as-needed
    slog
    -ldl
    -lrt
    -lpthread
    -lgcov
)

add_custom_command(
  TARGET  cpu_engine_stest POST_BUILD
  COMMAND cpu_engine_stest
  COMMENT "Run cpu_engine_stest"
)

add_custom_command(
  TARGET  tf_engine_stest POST_BUILD
  COMMAND tf_engine_stest
  COMMENT "Run tf_engine_stest"
)

add_custom_command(
  TARGET  host_engine_stest POST_BUILD
  COMMAND host_engine_stest
  COMMENT "Run host_engine_stest"
)
