project(st_graph_engine)

include_directories(${AIR_CODE_DIR}/tests/)
include_directories(${AIR_CODE_DIR}/tests/ut)
include_directories(${AIR_CODE_DIR}/tests/ut/ge)

MESSAGE("copy st run data from ${AIR_CODE_DIR}/tests/st/st_run_data to ${CMAKE_CURRENT_BINARY_DIR}/")
file(COPY ${AIR_CODE_DIR}/tests/st/st_run_data DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/)

set(PROTO_LIST
        "${METADEF_DIR}/proto/om.proto"
        "${METADEF_DIR}/proto/ge_ir.proto"
        "${METADEF_DIR}/proto/task.proto"
        "${METADEF_DIR}/proto/ge_api.proto"
        "${METADEF_DIR}/proto/insert_op.proto"
        "${METADEF_DIR}/proto/dump_task.proto"
        "${METADEF_DIR}/proto/fwk_adapter.proto"
        "${METADEF_DIR}/proto/op_mapping.proto"
        "${METADEF_DIR}/proto/optimizer_priority.proto"
        "${METADEF_DIR}/proto/ge_api.proto"
        "${METADEF_DIR}/proto/tensorflow/attr_value.proto"
        "${METADEF_DIR}/proto/tensorflow/tensor.proto"
        "${METADEF_DIR}/proto/tensorflow/resource_handle.proto"
        "${METADEF_DIR}/proto/tensorflow/tensor_shape.proto"
        "${METADEF_DIR}/proto/tensorflow/types.proto"
        "${METADEF_DIR}/proto/tensorflow/node_def.proto"
        "${METADEF_DIR}/proto/onnx/ge_onnx.proto"
        "${METADEF_DIR}/proto/var_manager.proto"
        )

protobuf_generate(ge PROTO_SRCS PROTO_HDRS ${PROTO_LIST})

################################################################################
# build graph_engine_test
add_executable(graph_engine_test
    "${AIR_CODE_DIR}/tests/ut/ge/ffts_plus_proto_tools.cc"
    "${AIR_CODE_DIR}/tests/st/testcase/test_tools_task_info.cc"
    "${AIR_CODE_DIR}/tests/st/testcase/test_main.cc"
    "${AIR_CODE_DIR}/tests/st/testcase/test_profiling_start_node.cc"
    "${AIR_CODE_DIR}/tests/st/testcase/test_iterator_flow_ctrl.cc"
    "${AIR_CODE_DIR}/tests/st/testcase/test_framework_dummy.cc"
    "${AIR_CODE_DIR}/tests/st/testcase/test_transdata_fusion_optimize.cc"
    "${AIR_CODE_DIR}/tests/st/testcase/test_ge_opt_info.cc"
    "${AIR_CODE_DIR}/tests/st/testcase/test_exception_dumper.cc"
    "${AIR_CODE_DIR}/tests/st/testcase/test_graph_compiler.cc"
    "${AIR_CODE_DIR}/tests/st/testcase/test_model_loader.cc"
    "${AIR_CODE_DIR}/tests/st/testcase/test_model_deploy_schedule.cc"
    "${AIR_CODE_DIR}/tests/st/testcase/test_ctrl_flow_execute.cc"
    "${AIR_CODE_DIR}/tests/st/testcase/test_davinci_model_execute.cc"
    "${AIR_CODE_DIR}/tests/st/testcase/test_ffts_plus_executor.cc"
    "${AIR_CODE_DIR}/tests/st/testcase/test_known_dynamic_execute.cc"
    "${AIR_CODE_DIR}/tests/st/testcase/test_online_infer_dynamic.cc"
    "${AIR_CODE_DIR}/tests/st/testcase/test_folding.cc"
    "${AIR_CODE_DIR}/tests/st/testcase/test_local_queue.cc"
    "${AIR_CODE_DIR}/tests/st/testcase/test_heterogeneous_runtime.cc"
    "${AIR_CODE_DIR}/tests/st/testcase/test_ffts_executor.cc"
    "${AIR_CODE_DIR}/tests/st/testcase/test_ctrl_flow_v2.cc"
    "${AIR_CODE_DIR}/tests/st/testcase/test_ctrl_flow_compile.cc"
    "${AIR_CODE_DIR}/tests/st/testcase/test_single_op.cc"
    "${AIR_CODE_DIR}/tests/st/testcase/test_stream_allocator.cc"
    "${AIR_CODE_DIR}/tests/st/testcase/test_ge_ir_build.cc"
    "${AIR_CODE_DIR}/tests/st/testcase/test_ge_api.cc"
    "${AIR_CODE_DIR}/tests/st/testcase/test_profiling_init.cc"
    "${AIR_CODE_DIR}/tests/st/testcase/test_variable_accelerate.cc"
    "${AIR_CODE_DIR}/tests/st/testcase/test_single_op_profiling.cc"
    "${AIR_CODE_DIR}/tests/st/testcase/test_dynamic_stack_execute.cc"
    "${AIR_CODE_DIR}/tests/st/testcase/test_dynamic_hccl_execute.cc"
    "${AIR_CODE_DIR}/tests/st/testcase/test_atc.cc"
    "${AIR_CODE_DIR}/tests/st/testcase/test_multi_dims.cc"
    "${AIR_CODE_DIR}/tests/st/testcase/test_aipp.cc"
    "${AIR_CODE_DIR}/tests/st/testcase/test_aoe.cc"
    "${AIR_CODE_DIR}/tests/st/testcase/test_engine.cc"
    "${AIR_CODE_DIR}/tests/st/testcase/test_dump.cc"
    "${AIR_CODE_DIR}/tests/st/testcase/test_summary_and_checkpoint.cc"
    "${AIR_CODE_DIR}/tests/st/testcase/test_local_engine.cc"
    "${AIR_CODE_DIR}/tests/st/testcase/test_dynamic_graph.cc"
    "${AIR_CODE_DIR}/tests/st/testcase/test_infershape_and_folding.cc"
    "${AIR_CODE_DIR}/tests/st/testcase/test_atc.cc"
    "${AIR_CODE_DIR}/tests/st/testcase/test_v1_loop.cc"
    "${AIR_CODE_DIR}/tests/st/testcase/test_merge_shape_n.cc"
    "${AIR_CODE_DIR}/tests/st/testcase/test_file_constant.cc"
    "${AIR_CODE_DIR}/tests/st/testcase/test_data_slice.cc"
    "${AIR_CODE_DIR}/tests/st/testcase/test_swap_pass.cc"
)

target_compile_definitions(graph_engine_test PRIVATE
    $<$<STREQUAL:${ENABLE_OPEN_SRC},True>:ONLY_COMPILE_OPEN_SRC>
    google=ascend_private
)

target_include_directories(graph_engine_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${AIR_CODE_DIR}
)

set_target_properties(graph_engine_test PROPERTIES CXX_STANDARD 17)

target_link_libraries(graph_engine_test
    $<BUILD_INTERFACE:intf_pub>
    -Wl,--whole-archive
    graphengine
    -Wl,--no-whole-archive
    gtest ge_graph_dsl ge_running_env st_stubs gmock gmock_main
)

################################################################################
# build helper_runtime_test
add_executable(helper_runtime_test
    "test_main.cc"
    "test_helper_runtime.cc"
    "test_helper_cluster.cc"
)

target_compile_definitions(helper_runtime_test PRIVATE
    $<$<STREQUAL:${ENABLE_OPEN_SRC},True>:ONLY_COMPILE_OPEN_SRC>
    google=ascend_private
)

target_include_directories(helper_runtime_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${AIR_CODE_DIR}/third_party/inc/aicpu
    ${AIR_CODE_DIR}/third_party/inc/runtime
    ${AIR_CODE_DIR}
    ${AIR_CODE_DIR}/runtime/heterogeneous
    ${AIR_CODE_DIR}/runtime/deploy/deployer/device_mgr
    ${CMAKE_BINARY_DIR}/proto/ge
    ${CMAKE_BINARY_DIR}/proto/ge/proto
)

set_target_properties(helper_runtime_test PROPERTIES CXX_STANDARD 17)

target_link_libraries(helper_runtime_test
    $<BUILD_INTERFACE:intf_pub>
    #-Wl,--whole-archive
    graphengine helper_runtime
    #-Wl,--no-whole-archive
    gtest ge_graph_dsl ge_running_env
    gtest gtest_main gmock gmock_main
)

include(CTest)
enable_testing()
add_test(NAME test COMMAND graph_engine_test)



