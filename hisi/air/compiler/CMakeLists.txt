project (compiler)

add_subdirectory(graphcompiler)

add_subdirectory(plugin/engine)
add_subdirectory(local_engine)
add_subdirectory(offline)

set(GRAPHENGINE_PROTO_LIST
    "${METADEF_DIR}/proto/om.proto"
    "${METADEF_DIR}/proto/task.proto"
    "${METADEF_DIR}/proto/insert_op.proto"
    "${METADEF_DIR}/proto/ge_ir.proto"
    "${METADEF_DIR}/proto/ge_api.proto"
    "${METADEF_DIR}/proto/fusion_model.proto"
    "${METADEF_DIR}/proto/optimizer_priority.proto"
    "${METADEF_DIR}/proto/fwk_adapter.proto"
    "${METADEF_DIR}/proto/op_mapping.proto"
    "${METADEF_DIR}/proto/dump_task.proto"
    "${METADEF_DIR}/proto/tensorflow/graph_library.proto"
    "${METADEF_DIR}/proto/tensorflow/graph.proto"
    "${METADEF_DIR}/proto/tensorflow/node_def.proto"
    "${METADEF_DIR}/proto/tensorflow/function.proto"
    "${METADEF_DIR}/proto/tensorflow/versions.proto"
    "${METADEF_DIR}/proto/tensorflow/attr_value.proto"
    "${METADEF_DIR}/proto/tensorflow/op_def.proto"
    "${METADEF_DIR}/proto/tensorflow/tensor.proto"
    "${METADEF_DIR}/proto/tensorflow/tensor_shape.proto"
    "${METADEF_DIR}/proto/tensorflow/types.proto"
    "${METADEF_DIR}/proto/tensorflow/resource_handle.proto"
    "${METADEF_DIR}/proto/var_manager.proto"
)

protobuf_generate(graphengine_protos GRAPHENGINE_PROTO_SRCS GRAPHENGINE_PROTO_HDRS ${GRAPHENGINE_PROTO_LIST} TARGET)

set(GE_FUSION_MODEL_PROTO_SRCS
    "${CMAKE_BINARY_DIR}/proto/graphengine_protos/proto/fusion_model.pb.cc"
    "${CMAKE_BINARY_DIR}/proto/graphengine_protos/proto/optimizer_priority.pb.cc"
)


add_library(ge_fusion_model_protos_obj OBJECT ${GE_FUSION_MODEL_PROTO_SRCS})

add_dependencies(ge_fusion_model_protos_obj graphengine_protos)

target_include_directories(ge_fusion_model_protos_obj PRIVATE
    #### blue zone ####
    ${PROTOBUF_SHARED_PKG_DIR}/include
    #### yellow zone ####
    ${ASCEND_PROTOBUF_SHARED_PKG_DIR}/include
)

target_compile_definitions(ge_fusion_model_protos_obj PRIVATE
    google=ascend_private
)

target_link_libraries(ge_fusion_model_protos_obj PRIVATE ascend_protobuf $<BUILD_INTERFACE:intf_pub>)

target_compile_options(ge_fusion_model_protos_obj PRIVATE
    -Werror
    -Wextra
    -Wfloat-equal
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Linux>:-O2 -fPIC>
    $<$<OR:$<STREQUAL:${PRODUCT_SIDE},host>,$<STREQUAL:${ENABLE_OPEN_SRC},True>>:-fexceptions>
    $<$<OR:$<STREQUAL:${TARGET_SYSTEM_NAME},Linux>,$<STREQUAL:${TARGET_SYSTEM_NAME},Android>>: -fno-common>
    $<$<AND:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>,$<STREQUAL:${CMAKE_CONFIGURATION_TYPES},Debug>>:/MTd>
    $<$<AND:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>,$<STREQUAL:${CMAKE_CONFIGURATION_TYPES},Release>>:/MT>
)

if (NOT ENABLE_D AND NOT ENABLE_ACL AND NOT ENABLE_MS_TESTCASES)
############ libge_proto_common.a ############
add_library(ge_proto_common STATIC
    $<TARGET_OBJECTS:ge_fusion_model_protos_obj>
)

target_compile_definitions(ge_proto_common PRIVATE
    PROTOBUF_INLINE_NOT_IN_HEADERS=0
    google=ascend_private
)

target_compile_options(ge_proto_common PRIVATE
    -O2
    -fno-common
    -Wextra
    -Wfloat-equal
)

target_link_libraries(ge_proto_common PRIVATE
    $<BUILD_INTERFACE:intf_pub>
    ascend_protobuf
)
endif ()

##################################################################
set(COMPILER_SRC_LIST
    "analyzer/analyzer.cc"
    "engine_manager/dnnengine_manager.cc"
    "local_engine/engine/host_cpu_engine.cc"
    "opt_info/ge_opt_info.cc"
    "generator/ge_generator.cc"
    "graph/build/graph_builder.cc"
    "graph/build/label_allocator.cc"
    "graph/build/logical_stream_allocator.cc"
    "graph/build/memory/binary_block_mem_assigner.cc"
    "graph/build/memory/block_mem_assigner.cc"
    "graph/build/memory/buffer_pool_mem_assigner.cc"
    "graph/build/memory/graph_mem_assigner.cc"
    "graph/build/memory/hybrid_mem_assigner.cc"
    "graph/build/memory/max_block_mem_assigner.cc"
    "graph/build/memory/memory_assigner.cc"
    "graph/build/model_builder.cc"
    "graph/build/run_context.cc"
    "graph/build/stream_allocator.cc"
    "graph/build/stream_graph_optimizer.cc"
    "graph/build/task_generator.cc"
    "graph/common/trans_op_creator.cc"
    "graph/fuzz_compile/fuzz_compile_bin_selector.cc"
    "graph/label/case_label_maker.cc"
    "graph/label/if_label_maker.cc"
    "graph/label/label_maker.cc"
    "graph/label/partitioned_call_label_maker.cc"
    "graph/label/while_label_maker.cc"
    "graph/manager/graph_context.cc"
    "graph/manager/graph_manager.cc"
    "graph/manager/util/rt_context_util.cc"
    "graph/manager/session_id_manager.cc"
    "graph/manager/util/graph_rebuild_state_ctrl.cc"
    "graph/manager/util/graph_optimize_utility.cc"
    "graph/optimize/graph_optimize.cc"
    "graph/optimize/mem_rw_conflict_optimize.cc"
    "graph/optimize/summary_optimize.cc"
    "graph/partition/base_partition.cc"
    "graph/partition/pipeline_partition.cc"
    "graph/partition/dynamic_shape_partition.cc"
    "graph/partition/base_cluster.cc"
    "graph/partition/engine_place.cc"
    "graph/partition/graph_partition.cc"
    "graph/partition/stage_partition.cc"
    "graph/partition/optimizer/dynamic_data_flow_engine_reassign_pass.cc"
    "graph/partition/optimizer/dynamic_data_flow_partitioner_pass.cc"
    "graph/passes/set_ffts_plus_attr_pass.cc"
    "graph/passes/addn_pass.cc"
    "graph/passes/aicpu_constant_folding_pass.cc"
    "graph/passes/assert_pass.cc"
    "graph/passes/assign_remove_pass.cc"
    "graph/passes/atomic_addr_clean_pass.cc"
    "graph/passes/attach_stream_label_pass.cc"
    "graph/passes/base_pass.cc"
    "graph/passes/bitcast_pass.cc"
    "graph/passes/buffer_pool_memory_pass.cc"
    "graph/passes/cast_remove_pass.cc"
    "graph/passes/common_subexpression_elimination_pass.cc"
    "graph/passes/compile_nodes_pass.cc"
    "graph/passes/cond_pass.cc"
    "graph/passes/cond_remove_pass.cc"
    "graph/passes/constant_folding_pass.cc"
    "graph/passes/constant_fuse_same_pass.cc"
    "graph/passes/control_trigger_pass.cc"
    "graph/passes/create_subgraph_with_scope_pass.cc"
    "graph/passes/ctrl_edge_transfer_pass.cc"
    "graph/passes/data_pass.cc"
    "graph/passes/split_shape_n_pass.cc"
    "graph/passes/merge_unknown_shape_n_pass.cc"
    "graph/passes/dimension_adjust_pass.cc"
    "graph/passes/dimension_compute_pass.cc"
    "graph/passes/dropout_pass.cc"
    "graph/passes/end_of_sequence_add_control_pass.cc"
    "graph/passes/enter_pass.cc"
    "graph/passes/flow_ctrl_pass.cc"
    "graph/passes/folding_pass.cc"
    "graph/passes/for_pass.cc"
    "graph/passes/fuse_data_nodes_with_common_input_pass.cc"
    "graph/passes/get_original_format_pass.cc"
    "graph/passes/global_step_insert_pass.cc"
    "graph/passes/guarantee_const_pass.cc"
    "graph/passes/hccl_continuous_memcpy_pass.cc"
    "graph/passes/hccl_group_pass.cc"
    "graph/passes/hccl_memcpy_pass.cc"
    "graph/passes/hccl_tailing_optimization_pass.cc"
    "graph/passes/identity_pass.cc"
    "graph/passes/infer_base_pass.cc"
    "graph/passes/infer_value_range_pass.cc"
    "graph/passes/infershape_pass.cc"
    "graph/passes/inplace_support_check_pass.cc"
    "graph/passes/input_output_connection_identify_pass.cc"
    "graph/passes/iterator_op_pass.cc"
    "graph/passes/link_gen_mask_nodes_pass.cc"
    "graph/passes/mark_agnostic_pass.cc"
    "graph/passes/mark_force_unknown_for_cond_pass.cc"
    "graph/passes/mark_graph_unknown_status_pass.cc"
    "graph/passes/mark_same_addr_pass.cc"
    "graph/passes/memcpy_addr_async_pass.cc"
    "graph/passes/merge_input_memcpy_pass.cc"
    "graph/passes/merge_pass.cc"
    "graph/passes/merge_to_stream_merge_pass.cc"
    "graph/passes/multi_batch_clone_pass.cc"
    "graph/passes/multi_batch_pass.cc"
    "graph/passes/net_output_pass.cc"
    "graph/passes/next_iteration_pass.cc"
    "graph/passes/no_use_reshape_remove_pass.cc"
    "graph/passes/parallel_concat_start_op_pass.cc"
    "graph/passes/parallel_group_pass.cc"
    "graph/passes/pass_manager.cc"
    "graph/passes/pass_utils.cc"
    "graph/passes/permute_pass.cc"
    "graph/passes/placeholder_with_default_pass.cc"
    "graph/passes/prevent_gradient_pass.cc"
    "graph/passes/print_op_pass.cc"
    "graph/passes/prune_pass.cc"
    "graph/passes/ref_identity_delete_op_pass.cc"
    "graph/passes/remove_same_const_pass.cc"
    "graph/passes/replace_transshape_pass.cc"
    "graph/passes/replace_with_empty_const_pass.cc"
    "graph/passes/reshape_recovery_pass.cc"
    "graph/passes/reshape_remove_pass.cc"
    "graph/passes/same_transdata_breadth_fusion_pass.cc"
    "graph/passes/save_pass.cc"
    "graph/passes/set_input_output_offset_pass.cc"
    "graph/passes/shape_operate_op_remove_pass.cc"
    "graph/passes/snapshot_pass.cc"
    "graph/passes/stop_gradient_pass.cc"
    "graph/passes/subexpression_migration_pass.cc"
    "graph/passes/subgraph_const_migration_pass.cc"
    "graph/passes/subgraph_multi_dims_pass.cc"
    "graph/passes/subgraph_multi_dims_clone_pass.cc"
    "graph/passes/subgraph_pass.cc"
    "graph/passes/switch_data_edges_bypass.cc"
    "graph/passes/switch_dead_branch_elimination.cc"
    "graph/passes/switch_logic_remove_pass.cc"
    "graph/passes/switch_to_stream_switch_pass.cc"
    "graph/passes/start_of_sequence_pass.cc"
    "graph/passes/transop_breadth_fusion_pass.cc"
    "graph/passes/transop_nearby_allreduce_fusion_pass.cc"
    "graph/passes/transop_symmetry_elimination_pass.cc"
    "graph/passes/transop_without_reshape_fusion_pass.cc"
    "graph/passes/transpose_transdata_pass.cc"
    "graph/passes/unused_args_clean_pass.cc"
    "graph/passes/unused_const_pass.cc"
    "graph/passes/useless_control_out_remove_pass.cc"
    "graph/passes/var_is_initialized_op_pass.cc"
    "graph/passes/variable_op_pass.cc"
    "graph/passes/variable_prepare_op_pass.cc"
    "graph/passes/variable_ref_delete_op_pass.cc"
    "graph/passes/variable_ref_useless_control_out_delete_pass.cc"
    "graph/passes/mds_pass.cc"
    "graph/passes/swap_space_pass.cc"
    "graph/passes/mds_kernels/base_mds_kernel.cc"
    "graph/passes/mds_kernels/mds_utils.cc"
    "graph/passes/data_flow_prepare_pass.cc"
    "graph/passes/potential_folding_pass.cc"
    "graph/passes/potential_const_taken_effect_pass.cc"
    "graph/preprocess/graph_preprocess.cc"
    "graph/preprocess/insert_op/ge_aipp_op.cc"
    "graph/preprocess/insert_op/util_insert_aipp_op.cc"
    "graph/preprocess/multi_batch_copy_graph.cc"
    "graph/preprocess/multi_batch_options.cc"
    "host_kernels/add_kernel.cc"
    "host_kernels/gathershapes_kernel.cc"
    "host_kernels/flattenv2_kernel.cc"
    "host_kernels/broadcast_args_kernel.cc"
    "host_kernels/broadcast_gradient_args_kernel.cc"
    "host_kernels/cast_kernel.cc"
    "host_kernels/concat_offset_kernel.cc"
    "host_kernels/concat_v2_kernel.cc"
    "host_kernels/dynamic_stitch_kernel.cc"
    "host_kernels/empty_kernel.cc"
    "host_kernels/expanddims_kernel.cc"
    "host_kernels/fill_kernel.cc"
    "host_kernels/floordiv_kernel.cc"
    "host_kernels/floormod_kernel.cc"
    "host_kernels/gather_v2_kernel.cc"
    "host_kernels/greater_kernel.cc"
    "host_kernels/identity_kernel.cc"
    "host_kernels/kernel_utils.cc"
    "host_kernels/maximum_kernel.cc"
    "host_kernels/mul_kernel.cc"
    "host_kernels/pack_kernel.cc"
    "host_kernels/permute_kernel.cc"
    "host_kernels/range_kernel.cc"
    "host_kernels/rank_kernel.cc"
    "host_kernels/reduce_prod_kernel.cc"
    "host_kernels/reformat_kernel.cc"
    "host_kernels/reshape_kernel.cc"
    "host_kernels/rsqrt_kernel.cc"
    "host_kernels/shape_kernel.cc"
    "host_kernels/shape_n_kernel.cc"
    "host_kernels/size_kernel.cc"
    "host_kernels/slice_d_kernel.cc"
    "host_kernels/slice_kernel.cc"
    "host_kernels/squeeze_kernel.cc"
    "host_kernels/squeezev3_kernel.cc"
    "host_kernels/ssd_prior_box_kernel.cc"
    "host_kernels/strided_slice_kernel.cc"
    "host_kernels/sub_kernel.cc"
    "host_kernels/transdata_kernel.cc"
    "host_kernels/transpose_kernel.cc"
    "host_kernels/unpack_kernel.cc"
    "host_kernels/unsqueeze_kernel.cc"
    "host_kernels/unsqueezev3_kernel.cc"
    "init/gelib.cc"
    "ir_build/attr_options/keep_dtype_option.cc"
    "ir_build/attr_options/utils.cc"
    "ir_build/attr_options/weight_compress_option.cc"
    "ir_build/ge_ir_build.cc"
    "ir_build/option_utils.cc"
    "opskernel_manager/ops_kernel_builder_manager.cc"
    "opskernel_manager/ops_kernel_manager.cc"
    "pne/npu/npu_process_node_engine.cc"
    "pne/cpu/cpu_process_node_engine.cc"
)

if (NOT ENABLE_D AND NOT ENABLE_ACL AND NOT ENABLE_MS_TESTCASES)
message("CMAKE_CXX_COMPILER_VERSION = ${CMAKE_CXX_COMPILER_VERSION}")
############ libge_compiler.so ############
add_library(ge_compiler SHARED
    ${COMPILER_SRC_LIST}
)

add_dependencies(ge_compiler
    graphengine_protos
)

target_compile_definitions(ge_compiler PRIVATE
    PROTOBUF_INLINE_NOT_IN_HEADERS=0
    REUSE_MEMORY=1
    FMK_SUPPORT_DUMP
    google=ascend_private
    $<$<STREQUAL:${ENABLE_OPEN_SRC},True>:ONLY_COMPILE_OPEN_SRC>
)

target_compile_options(ge_compiler PRIVATE
    -O2
    -fno-common
    -Werror
    -Wextra
    -Wfloat-equal
    -fvisibility=default
)

target_include_directories(ge_compiler SYSTEM PRIVATE
    ${PROJECT_SOURCE_DIR}
    ${AIR_CODE_DIR}/base
    ${AIR_CODE_DIR}/inc
    ${AIR_CODE_DIR}/inc/external
    ${AIR_CODE_DIR}/inc/framework
    ${METADEF_DIR}
    ${METADEF_DIR}/inc
    ${METADEF_DIR}/inc/external
    ${METADEF_DIR}/third_party/transformer
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/proto/graphengine_protos
    #### yellow zone ####
    ${GE_DEPEND_DIR}/toolchain/ide/ide-daemon/external
    ${GE_DEPEND_DIR}/abl/adump/external
    ${GE_DEPEND_DIR}/abl/licctrl
    ${GE_DEPEND_DIR}/ace/comop/inc
    ${GE_DEPEND_DIR}/ace/comop/inc/external
    $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:${GE_DEPEND_DIR}/inc>
    $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:$<TARGET_PROPERTY:runtime_headers,INTERFACE_INCLUDE_DIRECTORIES>>
    $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:$<TARGET_PROPERTY:cce_headers,INTERFACE_INCLUDE_DIRECTORIES>>
    #### blue zone ####
    ${ASCEND_DIR}/driver/include
    ${ASCEND_DIR}/fwkacllib/include
    $<$<BOOL:${ENABLE_OPEN_SRC}>:${AIR_CODE_DIR}/third_party/inc>
    $<$<BOOL:${ENABLE_OPEN_SRC}>:${AIR_CODE_DIR}/third_party/inc/opt_info>
)

target_link_options(ge_compiler PRIVATE
    -Wl,-Bsymbolic
)

target_link_libraries(ge_compiler PRIVATE
    $<BUILD_INTERFACE:intf_pub>
    $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:$<BUILD_INTERFACE:slog_headers>>
    $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:$<BUILD_INTERFACE:msprof_headers>>
    $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:$<BUILD_INTERFACE:mmpa_headers>>
    static_mmpa
    ge_proto_common
    -Wl,--no-as-needed
    graph
    ge_common
    ascend_protobuf
    register
    c_sec
    error_manager
    slog
    runtime
    opt_feature
    -Wl,--as-needed
    json
    -lrt
    -ldl
)

###############################################################
add_custom_target(
    engine_conf.json ALL
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/engine_conf.json
)
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/engine_conf.json
    COMMAND cp ${CMAKE_CURRENT_LIST_DIR}/engine_manager/engine_conf.json ${CMAKE_CURRENT_BINARY_DIR}/
)

###############################################################
add_custom_target(
    optimizer_priority.pbtxt ALL
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/optimizer_priority.pbtxt
)
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/optimizer_priority.pbtxt
    COMMAND cp ${CMAKE_CURRENT_LIST_DIR}/opskernel_manager/optimizer_priority.pbtxt ${CMAKE_CURRENT_BINARY_DIR}/
)

###############################################################

############ install ############
set(INSTALL_BASE_DIR "")
set(INSTALL_LIBRARY_DIR lib)

install(TARGETS ge_compiler OPTIONAL
    LIBRARY DESTINATION ${INSTALL_LIBRARY_DIR}
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/engine_conf.json
    ${CMAKE_CURRENT_BINARY_DIR}/optimizer_priority.pbtxt OPTIONAL
    DESTINATION ${INSTALL_LIBRARY_DIR}
)
endif()
