project (executor)


##################################################################
set(EXECUTOR_SRC_LIST
    "common/dump/dump_op.cc"
    "common/dump/exception_dumper.cc"
    "common/dump/opdebug_register.cc"
    "common/profiling/ge_profiling.cc"
    "common/profiling/profiling_manager.cc"
    "common/profiling/command_handle.cc"
    "common/utils/executor_utils.cc"
    "executor/ge_executor.cc"
    "graph/execute/graph_execute.cc"
    "graph/execute/model_executor.cc"
    "graph/load/graph_loader.cc"
    "graph/load/model_manager/aipp_utils.cc"
    "graph/load/model_manager/cpu_queue_schedule.cc"
    "common/dump/data_dumper.cc"
    "graph/load/model_manager/data_inputer.cc"
    "graph/load/model_manager/davinci_model.cc"
    "graph/load/model_manager/heterogeneous_model_executor.cc"
    "graph/load/model_manager/aicpu_resources.cc"
    "graph/load/model_manager/model_manager.cc"
    "graph/load/model_manager/model_utils.cc"
    "graph/load/model_manager/task_info/cmo_barrier_task_info.cc"
    "graph/load/model_manager/task_info/cmo_task_info.cc"
    "graph/load/model_manager/task_info/dsa_task_info.cc"
    "graph/load/model_manager/task_info/end_graph_task_info.cc"
    "graph/load/model_manager/task_info/event_record_task_info.cc"
    "graph/load/model_manager/task_info/event_wait_task_info.cc"
    "graph/load/model_manager/task_info/ffts_task_info.cc"
    "graph/load/model_manager/task_info/ffts_plus_task_info.cc"
    "graph/load/model_manager/task_info/ffts_plus_proto_transfer.cc"
    "graph/load/model_manager/task_info/fusion_start_task_info.cc"
    "graph/load/model_manager/task_info/fusion_stop_task_info.cc"
    "graph/load/model_manager/task_info/hccl_task_info.cc"
    "graph/load/model_manager/task_info/kernel_ex_task_info.cc"
    "graph/load/model_manager/task_info/kernel_task_info.cc"
    "graph/load/model_manager/task_info/label_goto_ex_task_info.cc"
    "graph/load/model_manager/task_info/label_set_task_info.cc"
    "graph/load/model_manager/task_info/label_switch_by_index_task_info.cc"
    "graph/load/model_manager/task_info/memcpy_addr_async_task_info.cc"
    "graph/load/model_manager/task_info/memcpy_async_task_info.cc"
    "graph/load/model_manager/task_info/model_exit_task_info.cc"
    "graph/load/model_manager/task_info/profiler_trace_task_info.cc"
    "graph/load/model_manager/task_info/stream_active_task_info.cc"
    "graph/load/model_manager/task_info/stream_switch_task_info.cc"
    "graph/load/model_manager/task_info/super_kernel/super_kernel.cc"
    "graph/load/model_manager/task_info/super_kernel/super_kernel_factory.cc"
    "graph/load/model_manager/task_info/task_info.cc"
    "graph/load/model_manager/tbe_kernel_handle.cc"
    "graph/load/model_manager/zero_copy_offset.cc"
    "graph/load/model_manager/zero_copy_task.cc"
    "graph/manager/graph_caching_allocator.cc"
    "graph/manager/graph_mem_allocator.cc"
    "graph/manager/graph_mem_manager.cc"
    "graph/manager/host_mem_allocator.cc"
    "graph/manager/host_mem_manager.cc"
    "graph/manager/memory_api.cc"
    "graph/manager/rdma_pool_allocator.cc"
    "graph/manager/session_scope_mem_allocator.cc"
    "graph/manager/trans_var_data_utils.cc"
    "graph/manager/util/hcom_util.cc"
    "hybrid/common/npu_memory_allocator.cc"
    "hybrid/common/tensor_value.cc"
    "hybrid/common/bin_cache/one_node_single_bin_selector.cc"
    "hybrid/executor/hybrid_execution_context.cc"
    "hybrid/executor/hybrid_model_async_executor.cc"
    "hybrid/executor/hybrid_model_executor.cc"
    "hybrid/executor/hybrid_model_pipeline_executor.cc"
    "hybrid/executor/hybrid_profiler.cc"
    "hybrid/executor/node_done_manager.cc"
    "hybrid/executor/node_state.cc"
    "hybrid/executor/rt_callback_manager.cc"
    "hybrid/executor/host_cpu_callback_manager.cc"
    "hybrid/executor/subgraph_context.cc"
    "hybrid/executor/subgraph_executor.cc"
    "hybrid/executor/hybrid_data_flow.cc"
    "hybrid/executor/hybrid_resource_manager.cc"
    "hybrid/executor/worker/execution_engine.cc"
    "hybrid/executor/worker/shape_inference_engine.cc"
    "hybrid/hybrid_davinci_model.cc"
    "hybrid/model/graph_item.cc"
    "hybrid/model/hybrid_model.cc"
    "hybrid/model/hybrid_model_builder.cc"
    "hybrid/model/node_item.cc"
    "hybrid/model/infer/graph_stage_cache.cc"
    "hybrid/model/infer/node_shape_infer.cc"
    "hybrid/model/infer/node_shape_propagator.cc"
    "hybrid/model/infer/shape_utils.cc"
    "hybrid/model/infer/shape_propagator.cc"
    "hybrid/model/infer/tensor_desc_observer.cc"
    "hybrid/model/infer/tensor_desc_holder.cc"
    "hybrid/node_executor/aicore/aicore_node_executor.cc"
    "hybrid/node_executor/aicore/aicore_op_task.cc"
    "hybrid/node_executor/aicore/aicore_task_builder.cc"
    "hybrid/node_executor/aicpu/aicpu_ext_info.cc"
    "hybrid/node_executor/aicpu/aicpu_node_executor.cc"
    "hybrid/node_executor/compiledsubgraph/known_node_executor.cc"
    "hybrid/node_executor/controlop/control_op_executor.cc"
    "hybrid/node_executor/ge_local/data_flow_kernels.cc"
    "hybrid/node_executor/ge_local/ge_local_node_executor.cc"
    "hybrid/node_executor/hccl/hccl_node_executor.cc"
    "hybrid/node_executor/host_cpu/host_cpu_node_executor.cc"
    "hybrid/node_executor/node_executor.cc"
    "hybrid/node_executor/partitioned_call/partitioned_call_node_executor.cc"
    "hybrid/node_executor/ffts_plus/ffts_plus_node_task.cc"
    "hybrid/node_executor/ffts_plus/ffts_plus_subgraph_executor.cc"
    "hybrid/node_executor/ffts_plus/ffts_plus_sub_node_executor.cc"
    "hybrid/node_executor/ffts_plus/ffts_plus_sub_task.cc"
    "hybrid/node_executor/ffts_plus/ffts_plus_sub_task_factory.cc"
    "hybrid/node_executor/rts/rts_node_executor.cc"
    "hybrid/node_executor/rts/rts_node_task.cc"
    "hybrid/node_executor/rts/rts_task_factory.cc"
    "hybrid/node_executor/task_context.cc"
    "single_op/single_op.cc"
    "single_op/single_op_manager.cc"
    "single_op/single_op_model.cc"
    "single_op/stream_resource.cc"
    "single_op/task/aicpu_kernel_task_builder.cc"
    "single_op/task/aicpu_task_builder.cc"
    "single_op/task/build_task_utils.cc"
    "single_op/task/op_task.cc"
    "single_op/task/rts_kernel_task_builder.cc"
    "single_op/task/tbe_task_builder.cc"
    "runtime/local_execution_runtime.cc"
    "runtime/deploy/local_model_deployer.cc"
    "runtime/deploy/local_exchange_deployer.cc"
    "runtime/deploy/local_exchange_service.cc"
    "opskernel_executor/ops_kernel_executor_manager.cc"
    "${AIR_CODE_DIR}/compiler/local_engine/engine/host_cpu_engine.cc"
    "${AIR_CODE_DIR}/compiler/host_kernels/kernel_utils.cc"
    "${AIR_CODE_DIR}/compiler/host_kernels/rank_kernel.cc"
    "${AIR_CODE_DIR}/compiler/host_kernels/shape_kernel.cc"
    "${AIR_CODE_DIR}/compiler/host_kernels/shape_n_kernel.cc"
    "${AIR_CODE_DIR}/compiler/host_kernels/size_kernel.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/mds_kernels/mds_utils.cc"
    "${AIR_CODE_DIR}/compiler/host_kernels/gathershapes_kernel.cc"
        )

if (NOT ENABLE_D AND NOT ENABLE_ACL AND NOT ENABLE_MS_TESTCASES)
message("CMAKE_CXX_COMPILER_VERSION = ${CMAKE_CXX_COMPILER_VERSION}")
######## mini_acllib_win: libge_executor.a ########
add_library(ge_executor STATIC
    ${EXECUTOR_SRC_LIST})

add_dependencies(ge_executor
    graphengine_protos
)

target_compile_options(ge_executor PRIVATE
    $<$<OR:$<STREQUAL:${TARGET_SYSTEM_NAME},Linux>,$<STREQUAL:${TARGET_SYSTEM_NAME},Android>>:-fvisibility=hidden -O2 -Werror -fno-common -Wextra -Wfloat-equal>
    $<$<AND:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>,$<STREQUAL:${CMAKE_CONFIGURATION_TYPES},Debug>>:/MTd>
    $<$<AND:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>,$<STREQUAL:${CMAKE_CONFIGURATION_TYPES},Release>>:/MT>
)

target_compile_definitions(ge_executor PRIVATE
    PROTOBUF_INLINE_NOT_IN_HEADERS=0
    google=ascend_private
    $<IF:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>,OS_TYPE=WIN,OS_TYPE=0>
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>:SECUREC_USING_STD_SECURE_LIB=0 NOMINMAX>
    $<$<STREQUAL:${ENABLE_OPEN_SRC},True>:ONLY_COMPILE_OPEN_SRC>
    LOG_CPP
)

target_include_directories(ge_executor PRIVATE
    ${PROJECT_SOURCE_DIR}
    ${AIR_CODE_DIR}/base
    ${AIR_CODE_DIR}/compiler
    ${AIR_CODE_DIR}/inc
    ${AIR_CODE_DIR}/inc/external
    ${AIR_CODE_DIR}/inc/framework
    ${METADEF_DIR}
    ${METADEF_DIR}/inc
    ${METADEF_DIR}/inc/external
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/proto/graphengine_protos
    #### yellow zone ####
    $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:${GE_DEPEND_DIR}/inc>
    $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:${GE_DEPEND_DIR}/ace/comop/inc>
    $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:${GE_DEPEND_DIR}/ace/comop/inc/external>
    $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:${GE_DEPEND_DIR}/asl/aoetools/inc/graph_tuner>
    $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:$<TARGET_PROPERTY:runtime_headers,INTERFACE_INCLUDE_DIRECTORIES>>
    $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:$<TARGET_PROPERTY:cce_headers,INTERFACE_INCLUDE_DIRECTORIES>>
    #### blue zone ####
    ${ASCEND_DIR}/driver/include
    ${ASCEND_DIR}/fwkacllib/include
    $<$<BOOL:${ENABLE_OPEN_SRC}>:${AIR_CODE_DIR}/third_party/inc>
)

target_link_libraries(ge_executor PRIVATE
    $<BUILD_INTERFACE:intf_pub>
    $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:$<BUILD_INTERFACE:slog_headers>>
    $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:$<BUILD_INTERFACE:msprof_headers>>
    $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:$<BUILD_INTERFACE:mmpa_headers>>
    #$<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:$<BUILD_INTERFACE:graph_tuner_headers>>
    json
    ascend_protobuf_static
    c_sec
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Android>>:-lrt>
    -ldl
)

######## libge_executor.so ########
add_library(ge_executor_shared SHARED
    ${EXECUTOR_SRC_LIST}
)

add_dependencies(ge_executor_shared
    graphengine_protos
)

target_compile_options(ge_executor_shared PRIVATE
    -fno-common
    -Werror
    -O2
    -Wextra
    -Wfloat-equal
    -fvisibility=default
)

target_compile_definitions(ge_executor_shared PRIVATE
    PROTOBUF_INLINE_NOT_IN_HEADERS=0
    google=ascend_private
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>:SECUREC_USING_STD_SECURE_LIB=0 NOMINMAX>
    $<$<STREQUAL:${ENABLE_OPEN_SRC},True>:ONLY_COMPILE_OPEN_SRC>
)

target_include_directories(ge_executor_shared PRIVATE
    ${PROJECT_SOURCE_DIR}
    ${AIR_CODE_DIR}/base
    ${AIR_CODE_DIR}/compiler
    ${AIR_CODE_DIR}/inc
    ${AIR_CODE_DIR}/inc/external
    ${AIR_CODE_DIR}/inc/framework
    ${METADEF_DIR}
    ${METADEF_DIR}/inc
    ${METADEF_DIR}/inc/external
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/proto/graphengine_protos
    #### yellow zone ####
    $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:${GE_DEPEND_DIR}/inc>
    $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:${GE_DEPEND_DIR}/ace/comop/inc>
    $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:${GE_DEPEND_DIR}/ace/comop/inc/external>
    $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:${GE_DEPEND_DIR}/asl/aoetools/inc/graph_tuner>
    $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:$<TARGET_PROPERTY:runtime_headers,INTERFACE_INCLUDE_DIRECTORIES>>
    $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:$<TARGET_PROPERTY:cce_headers,INTERFACE_INCLUDE_DIRECTORIES>>
    #### blue zone ####
    ${ASCEND_DIR}/driver/include
    ${ASCEND_DIR}/fwkacllib/include
    $<$<BOOL:${ENABLE_OPEN_SRC}>:${AIR_CODE_DIR}/third_party/inc>
)

target_link_options(ge_executor_shared PRIVATE
    -Wl,-Bsymbolic
    -Wl,--exclude-libs,ALL
)

target_link_libraries(ge_executor_shared PRIVATE
    $<BUILD_INTERFACE:intf_pub>
    $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:$<BUILD_INTERFACE:slog_headers>>
    $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:$<BUILD_INTERFACE:msprof_headers>>
    $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:$<BUILD_INTERFACE:mmpa_headers>>
    #$<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:$<BUILD_INTERFACE:graph_tuner_headers>>
    -Wl,--no-as-needed
    ge_common
    runtime
    slog
    graph
    register
    #graph_tuner
    error_manager
    ascend_protobuf
    c_sec
    -Wl,--as-needed
    json
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Android>>:-lrt>
    -ldl
)

set_target_properties(ge_executor_shared PROPERTIES
    OUTPUT_NAME ge_executor
)

############ install ############
set(INSTALL_BASE_DIR "")
set(INSTALL_LIBRARY_DIR lib)

install(TARGETS ge_executor_shared OPTIONAL
    LIBRARY DESTINATION ${INSTALL_LIBRARY_DIR}
)

endif()
