# Copyright 2019-2020 Huawei Technologies Co., Ltd
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# ============================================================================

project(ut_ge)

set(CMAKE_CXX_STANDARD 11)

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

set(DEPLOYER_PROTO_LIST
    "${AIR_CODE_DIR}/runtime/heterogeneous/proto/deployer.proto"
    "${AIR_CODE_DIR}/runtime/heterogeneous/proto/cluster.proto"
)

protobuf_generate(ge PROTO_SRCS PROTO_HDRS ${PROTO_LIST} "--proto_path=${METADEF_DIR}/proto")
protobuf_generate(ge DEPLOYER_PROTO_SRCS DEPLOYER_PROTO_HDRS ${DEPLOYER_PROTO_LIST} "--proto_path=${METADEF_DIR}/proto")

# include directories
include_directories(${CMAKE_CURRENT_LIST_DIR})
include_directories(${CMAKE_BINARY_DIR})
include_directories(${CMAKE_BINARY_DIR}/proto/ge)
include_directories(${CMAKE_BINARY_DIR}/proto/ge/proto)

include_directories(${AIR_CODE_DIR}/base)
include_directories(${AIR_CODE_DIR}/compiler)
include_directories(${AIR_CODE_DIR}/executor)
include_directories(${AIR_CODE_DIR}/runner)
include_directories(${AIR_CODE_DIR}/inc)
include_directories(${AIR_CODE_DIR}/inc/external)
include_directories(${AIR_CODE_DIR}/inc/framework)
include_directories(${AIR_CODE_DIR}/third_party/inc)
include_directories(${AIR_CODE_DIR}/third_party/inc/ops)
include_directories(${AIR_CODE_DIR}/third_party/inc/toolchain)
include_directories(${AIR_CODE_DIR}/third_party/inc/opt_info)
include_directories(${AIR_CODE_DIR}/third_party/inc/mmpa)

include_directories(${PARSER_DIR})
include_directories(${PARSER_DIR}/parser)
include_directories(${PARSER_DIR}/inc)
include_directories(${PARSER_DIR}/inc/external)

include_directories(${METADEF_DIR})
include_directories(${METADEF_DIR}/inc)
include_directories(${METADEF_DIR}/graph)
include_directories(${METADEF_DIR}/inc/external)
include_directories(${METADEF_DIR}/inc/external/graph)
include_directories(${METADEF_DIR}/inc/register)
include_directories(${METADEF_DIR}/inc/graph)
include_directories(${METADEF_DIR}/inc/common)
include_directories(${METADEF_DIR}/third_party)
include_directories(${METADEF_DIR}/third_party/transformer)
include_directories(${METADEF_DIR}/third_party/transformer/inc)

include_directories(${AIR_CODE_DIR}/tests/)
include_directories(${AIR_CODE_DIR}/tests/ut/ge)
include_directories(${AIR_CODE_DIR}/tests/ut/ge/runtime/stub)
include_directories(${AIR_CODE_DIR}/tests/ut/ge/runtime/stub/grpc++)
include_directories(${AIR_CODE_DIR}/tests/ut/common)


set(PARSER_SRC_FILES
    "${PARSER_DIR}/parser/common/op_map.cc"
    "${PARSER_DIR}/parser/common/pre_checker.cc"
    "${PARSER_DIR}/parser/common/convert/pb2json.cc"
    "${PARSER_DIR}/parser/common/parser_factory.cc"
    "${PARSER_DIR}/parser/common/model_saver.cc"
    "${PARSER_DIR}/parser/common/parser_types.cc"
    "${PARSER_DIR}/parser/common/parser_utils.cc"
    "${PARSER_DIR}/parser/common/parser_inner_ctx.cc"
    "${PARSER_DIR}/parser/tensorflow/iterator_fusion_pass.cc"
    "${PARSER_DIR}/parser/common/register_tbe.cc"
    "${PARSER_DIR}/parser/common/op_parser_factory.cc"
    "${PARSER_DIR}/parser/tensorflow/tensorflow_custom_parser_adapter.cc"
    "${PARSER_DIR}/parser/tensorflow/tensorflow_fusion_op_parser.cc"
    "${PARSER_DIR}/parser/tensorflow/tensorflow_fusion_custom_parser_adapter.cc"
    "${PARSER_DIR}/parser/tensorflow/tensorflow_util.cc"
    "${PARSER_DIR}/parser/common/parser_fp16_t.cc"
    "${PARSER_DIR}/parser/common/op_def/operator.cc"
)

set(COMMON_SRC_FILES
    "${AIR_CODE_DIR}/base/common/auth/file_saver.cc"
    "${AIR_CODE_DIR}/base/common/bcast.cc"
    "${AIR_CODE_DIR}/base/common/context/ctx.cc"
    "${AIR_CODE_DIR}/base/common/cust_aicpu_kernel_store.cc"
    "${AIR_CODE_DIR}/base/common/debug/memory_dumper.cc"
    "${AIR_CODE_DIR}/base/common/dump/dump_manager.cc"
    "${AIR_CODE_DIR}/base/common/dump/dump_properties.cc"
    "${AIR_CODE_DIR}/base/common/file_constant_util.cc"
    "${AIR_CODE_DIR}/base/common/fmk_error_codes.cc"
    "${AIR_CODE_DIR}/base/common/ge_inner_attrs.cc"
    "${AIR_CODE_DIR}/base/common/helper/model_helper.cc"
    "${AIR_CODE_DIR}/base/common/helper/om_file_helper.cc"
    "${AIR_CODE_DIR}/base/common/kernel_store.cc"
    "${AIR_CODE_DIR}/base/common/local_context.cc"
    "${AIR_CODE_DIR}/base/common/model/ge_model.cc"
    "${AIR_CODE_DIR}/base/common/model/ge_root_model.cc"
    "${AIR_CODE_DIR}/base/pne/model/flow_model.cc"
    "${AIR_CODE_DIR}/base/common/model/model_relation.cc"
    "${AIR_CODE_DIR}/base/common/model_parser/model_parser.cc"
    "${AIR_CODE_DIR}/base/common/model_saver.cc"
    "${AIR_CODE_DIR}/base/common/omg_util.cc"
    "${AIR_CODE_DIR}/base/common/op/ge_op_utils.cc"
    "${AIR_CODE_DIR}/base/common/plugin/datatype_util.cc"
    "${AIR_CODE_DIR}/base/common/plugin/op_tiling_manager.cc"
    "${AIR_CODE_DIR}/base/common/plugin/plugin_manager.cc"
    "${AIR_CODE_DIR}/base/common/profiling/profiling_properties.cc"
    "${AIR_CODE_DIR}/base/common/profiling_definitions.cc"
    "${AIR_CODE_DIR}/base/common/properties_manager.cc"
    "${AIR_CODE_DIR}/base/common/tbe_handle_store/bin_register_utils.cc"
    "${AIR_CODE_DIR}/base/common/tbe_handle_store/tbe_handle_store.cc"
    "${AIR_CODE_DIR}/base/common/tbe_kernel_store.cc"
    "${AIR_CODE_DIR}/base/common/thread_pool.cc"
    "${AIR_CODE_DIR}/base/common/transop_util.cc"
    "${AIR_CODE_DIR}/base/common/types.cc"
    "${AIR_CODE_DIR}/base/common/util.cc"
    "${AIR_CODE_DIR}/base/exec_runtime/deploy/deploy_planner.cc"
    "${AIR_CODE_DIR}/base/exec_runtime/execution_runtime.cc"
    "${AIR_CODE_DIR}/base/graph/bin_cache/node_bin_selector_factory.cc"
    "${AIR_CODE_DIR}/base/graph/bin_cache/node_compile_cache_module.cc"
    "${AIR_CODE_DIR}/base/graph/bin_cache/op_binary_cache.cc"
    "${AIR_CODE_DIR}/base/graph/build/memory/var_mem_assign_util.cc"
    "${AIR_CODE_DIR}/base/graph/manager/graph_manager_utils.cc"
    "${AIR_CODE_DIR}/base/graph/manager/graph_var_manager.cc"
    "${AIR_CODE_DIR}/base/slice/data_slice_default_impl.cc"
    "${AIR_CODE_DIR}/base/slice/data_slice_elementwise_impl.cc"
    "${AIR_CODE_DIR}/base/slice/data_slice_factory.cc"
    "${AIR_CODE_DIR}/base/slice/data_slice_helper.cc"
)

set(COMPILER_COMMON_SRC_FILES
    "${AIR_CODE_DIR}/compiler/analyzer/analyzer.cc"
    "${AIR_CODE_DIR}/compiler/engine_manager/dnnengine_manager.cc"
    "${AIR_CODE_DIR}/compiler/generator/ge_generator.cc"
    "${AIR_CODE_DIR}/compiler/graph/build/graph_builder.cc"
    "${AIR_CODE_DIR}/compiler/graph/build/label_allocator.cc"
    "${AIR_CODE_DIR}/compiler/graph/build/logical_stream_allocator.cc"
    "${AIR_CODE_DIR}/compiler/graph/build/memory/binary_block_mem_assigner.cc"
    "${AIR_CODE_DIR}/compiler/graph/build/memory/block_mem_assigner.cc"
    "${AIR_CODE_DIR}/compiler/graph/build/memory/buffer_pool_mem_assigner.cc"
    "${AIR_CODE_DIR}/compiler/graph/build/memory/graph_mem_assigner.cc"
    "${AIR_CODE_DIR}/compiler/graph/build/memory/hybrid_mem_assigner.cc"
    "${AIR_CODE_DIR}/compiler/graph/build/memory/max_block_mem_assigner.cc"
    "${AIR_CODE_DIR}/compiler/graph/build/memory/memory_assigner.cc"
    "${AIR_CODE_DIR}/compiler/graph/build/model_builder.cc"
    "${AIR_CODE_DIR}/compiler/graph/build/run_context.cc"
    "${AIR_CODE_DIR}/compiler/graph/build/stream_allocator.cc"
    "${AIR_CODE_DIR}/compiler/graph/build/stream_graph_optimizer.cc"
    "${AIR_CODE_DIR}/compiler/graph/build/task_generator.cc"
    "${AIR_CODE_DIR}/compiler/graph/common/trans_op_creator.cc"
    "${AIR_CODE_DIR}/compiler/graph/fuzz_compile/fuzz_compile_bin_selector.cc"
    "${AIR_CODE_DIR}/compiler/graph/manager/graph_context.cc"
    "${AIR_CODE_DIR}/compiler/graph/manager/graph_manager.cc"
    "${AIR_CODE_DIR}/compiler/graph/manager/session_id_manager.cc"
    "${AIR_CODE_DIR}/compiler/graph/manager/util/graph_optimize_utility.cc"
    "${AIR_CODE_DIR}/compiler/graph/manager/util/graph_rebuild_state_ctrl.cc"
    "${AIR_CODE_DIR}/compiler/graph/manager/util/rt_context_util.cc"
    "${AIR_CODE_DIR}/compiler/graph/optimize/graph_optimize.cc"
    "${AIR_CODE_DIR}/compiler/graph/optimize/mem_rw_conflict_optimize.cc"
    "${AIR_CODE_DIR}/compiler/graph/optimize/summary_optimize.cc"
    "${AIR_CODE_DIR}/compiler/graph/partition/base_cluster.cc"
    "${AIR_CODE_DIR}/compiler/graph/partition/base_partition.cc"
    "${AIR_CODE_DIR}/compiler/graph/partition/dynamic_shape_partition.cc"
    "${AIR_CODE_DIR}/compiler/graph/partition/engine_place.cc"
    "${AIR_CODE_DIR}/compiler/graph/partition/graph_partition.cc"
    "${AIR_CODE_DIR}/compiler/graph/partition/optimizer/dynamic_data_flow_engine_reassign_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/partition/optimizer/dynamic_data_flow_partitioner_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/partition/pipeline_partition.cc"
    "${AIR_CODE_DIR}/compiler/graph/partition/stage_partition.cc"
    "${AIR_CODE_DIR}/compiler/graph/preprocess/multi_batch_copy_graph.cc"
    "${AIR_CODE_DIR}/compiler/init/gelib.cc"
    "${AIR_CODE_DIR}/compiler/ir_build/attr_options/keep_dtype_option.cc"
    "${AIR_CODE_DIR}/compiler/ir_build/attr_options/utils.cc"
    "${AIR_CODE_DIR}/compiler/ir_build/attr_options/weight_compress_option.cc"
    "${AIR_CODE_DIR}/compiler/ir_build/ge_ir_build.cc"
    "${AIR_CODE_DIR}/compiler/ir_build/option_utils.cc"
    "${AIR_CODE_DIR}/compiler/local_engine/engine/ge_local_engine.cc"
    "${AIR_CODE_DIR}/compiler/local_engine/engine/host_cpu_engine.cc"
    "${AIR_CODE_DIR}/compiler/local_engine/ops_kernel_store/ge_local_ops_kernel_builder.cc"
    "${AIR_CODE_DIR}/compiler/local_engine/ops_kernel_store/ge_local_ops_kernel_info.cc"
    "${AIR_CODE_DIR}/compiler/local_engine/ops_kernel_store/op/ge_deleted_op.cc"
    "${AIR_CODE_DIR}/compiler/local_engine/ops_kernel_store/op/no_op.cc"
    "${AIR_CODE_DIR}/compiler/local_engine/ops_kernel_store/op/op.cc"
    "${AIR_CODE_DIR}/compiler/local_engine/ops_kernel_store/op/op_factory.cc"
    "${AIR_CODE_DIR}/compiler/offline/main_impl.cc"
    "${AIR_CODE_DIR}/compiler/offline/omg.cc"
    "${AIR_CODE_DIR}/compiler/offline/single_op_parser.cc"
    "${AIR_CODE_DIR}/compiler/opskernel_manager/ops_kernel_builder_manager.cc"
    "${AIR_CODE_DIR}/compiler/opskernel_manager/ops_kernel_manager.cc"
    "${AIR_CODE_DIR}/compiler/opt_info/ge_opt_info.cc"
    "${AIR_CODE_DIR}/compiler/plugin/engine/dnnengines.cc"
    "${AIR_CODE_DIR}/compiler/plugin/engine/engine_manage.cc"
    "${AIR_CODE_DIR}/compiler/pne/npu/npu_process_node_engine.cc"
    "${AIR_CODE_DIR}/compiler/pne/cpu/cpu_process_node_engine.cc"
)

set(EXECUTOR_COMMON_SRC_FILES
    "${AIR_CODE_DIR}/executor/common/dump/dump_op.cc"
    "${AIR_CODE_DIR}/executor/common/dump/exception_dumper.cc"
    "${AIR_CODE_DIR}/executor/common/dump/opdebug_register.cc"
    "${AIR_CODE_DIR}/executor/common/profiling/command_handle.cc"
    "${AIR_CODE_DIR}/executor/common/utils/executor_utils.cc"
    "${AIR_CODE_DIR}/executor/common/profiling/ge_profiling.cc"
    "${AIR_CODE_DIR}/executor/common/profiling/profiling_manager.cc"
    "${AIR_CODE_DIR}/executor/graph/execute/graph_execute.cc"
    "${AIR_CODE_DIR}/executor/graph/execute/model_executor.cc"
    "${AIR_CODE_DIR}/executor/graph/load/graph_loader.cc"
    "${AIR_CODE_DIR}/executor/graph/manager/graph_caching_allocator.cc"
    "${AIR_CODE_DIR}/executor/graph/manager/graph_mem_allocator.cc"
    "${AIR_CODE_DIR}/executor/graph/manager/graph_mem_manager.cc"
    "${AIR_CODE_DIR}/executor/graph/manager/host_mem_allocator.cc"
    "${AIR_CODE_DIR}/executor/graph/manager/host_mem_manager.cc"
    "${AIR_CODE_DIR}/executor/graph/manager/memory_api.cc"
    "${AIR_CODE_DIR}/executor/graph/manager/rdma_pool_allocator.cc"
    "${AIR_CODE_DIR}/executor/graph/manager/session_scope_mem_allocator.cc"
    "${AIR_CODE_DIR}/executor/graph/manager/trans_var_data_utils.cc"
    "${AIR_CODE_DIR}/executor/graph/manager/util/hcom_util.cc"
    "${AIR_CODE_DIR}/executor/hybrid/common/bin_cache/one_node_single_bin_selector.cc"
)

set(COMMON_FORMAT_SRC_FILES
    "${AIR_CODE_DIR}/base/common/fp16_t.cc"
    "${AIR_CODE_DIR}/base/common/ge_format_util.cc"
    "${AIR_CODE_DIR}/base/common/formats/formats.cc"
    "${AIR_CODE_DIR}/base/common/formats/format_transfers/datatype_transfer.cc"
    "${AIR_CODE_DIR}/base/common/formats/format_transfers/format_transfer_transpose.cc"
    "${AIR_CODE_DIR}/base/common/formats/format_transfers/format_transfer_nchw_nc1hwc0.cc"
    "${AIR_CODE_DIR}/base/common/formats/format_transfers/format_transfer_fractal_z.cc"
    "${AIR_CODE_DIR}/base/common/formats/format_transfers/format_transfer_fractal_nz.cc"
    "${AIR_CODE_DIR}/base/common/formats/format_transfers/format_transfer_fractal_zz.cc"
    "${AIR_CODE_DIR}/base/common/formats/format_transfers/format_transfer_fractal_z_tbe.cc"
    "${AIR_CODE_DIR}/base/common/formats/format_transfers/format_transfer_nhwc_nc1hwc0.cc"
    "${AIR_CODE_DIR}/base/common/formats/format_transfers/format_transfer_nc1hwc0_nchw.cc"
    "${AIR_CODE_DIR}/base/common/formats/format_transfers/format_transfer_nc1hwc0_nhwc.cc"
    "${AIR_CODE_DIR}/base/common/formats/format_transfers/format_transfer_hwcn_c1hwncoc0.cc"
    "${AIR_CODE_DIR}/base/common/formats/format_transfers/format_transfer_c1hwncoc0_hwcn.cc"
    "${AIR_CODE_DIR}/base/common/formats/format_transfers/format_transfer_fracz_nchw.cc"
    "${AIR_CODE_DIR}/base/common/formats/format_transfers/format_transfer_fracz_nhwc.cc"
    "${AIR_CODE_DIR}/base/common/formats/format_transfers/format_transfer_fracz_hwcn.cc"
    "${AIR_CODE_DIR}/base/common/formats/format_transfers/format_transfer_dhwcn_fracz3D.cc"
    "${AIR_CODE_DIR}/base/common/formats/format_transfers/format_transfer_dhwnc_fracz3D_transpose.cc"
    "${AIR_CODE_DIR}/base/common/formats/format_transfers/format_transfer_nchw_fz_c04.cc"
    "${AIR_CODE_DIR}/base/common/formats/register_format_transfer.cc"
    "${AIR_CODE_DIR}/base/common/formats/utils/formats_trans_utils.cc"
)

set(GRAPH_PREPARE_COMMON_SRC_FILES
    "${AIR_CODE_DIR}/compiler/graph/preprocess/graph_preprocess.cc"
    "${AIR_CODE_DIR}/compiler/graph/preprocess/insert_op/util_insert_aipp_op.cc"
    "${AIR_CODE_DIR}/compiler/graph/preprocess/insert_op/ge_aipp_op.cc"
    "${AIR_CODE_DIR}/compiler/graph/preprocess/multi_batch_options.cc"
)

set(GRAPH_DAVINCI_MODEL_SRC_FILES
    "${AIR_CODE_DIR}/tests/ut/ge/ffts_plus_proto_tools.cc"
    "${AIR_CODE_DIR}/executor/opskernel_executor/ops_kernel_executor_manager.cc"
    "${AIR_CODE_DIR}/executor/graph/load/model_manager/aipp_utils.cc"
    "${AIR_CODE_DIR}/executor/graph/load/model_manager/cpu_queue_schedule.cc"
    "${AIR_CODE_DIR}/executor/common/dump/data_dumper.cc"
    "${AIR_CODE_DIR}/executor/graph/load/model_manager/data_inputer.cc"
    "${AIR_CODE_DIR}/executor/graph/load/model_manager/davinci_model.cc"
    "${AIR_CODE_DIR}/executor/graph/load/model_manager/aicpu_resources.cc"
    "${AIR_CODE_DIR}/executor/graph/load/model_manager/model_manager.cc"
    "${AIR_CODE_DIR}/executor/graph/load/model_manager/model_utils.cc"
    "${AIR_CODE_DIR}/executor/graph/load/model_manager/zero_copy_offset.cc"
    "${AIR_CODE_DIR}/executor/graph/load/model_manager/zero_copy_task.cc"
    "${AIR_CODE_DIR}/executor/graph/load/model_manager/tbe_kernel_handle.cc"
    "${AIR_CODE_DIR}/executor/graph/load/model_manager/task_info/task_info.cc"
    "${AIR_CODE_DIR}/executor/graph/load/model_manager/task_info/end_graph_task_info.cc"
    "${AIR_CODE_DIR}/executor/graph/load/model_manager/task_info/cmo_task_info.cc"
    "${AIR_CODE_DIR}/executor/graph/load/model_manager/task_info/cmo_barrier_task_info.cc"
    "${AIR_CODE_DIR}/executor/graph/load/model_manager/task_info/event_record_task_info.cc"
    "${AIR_CODE_DIR}/executor/graph/load/model_manager/task_info/event_wait_task_info.cc"
    "${AIR_CODE_DIR}/executor/graph/load/model_manager/task_info/ffts_plus_proto_transfer.cc"
    "${AIR_CODE_DIR}/executor/graph/load/model_manager/task_info/ffts_plus_task_info.cc"
    "${AIR_CODE_DIR}/executor/graph/load/model_manager/task_info/ffts_task_info.cc"
    "${AIR_CODE_DIR}/executor/graph/load/model_manager/task_info/fusion_start_task_info.cc"
    "${AIR_CODE_DIR}/executor/graph/load/model_manager/task_info/fusion_stop_task_info.cc"
    "${AIR_CODE_DIR}/executor/graph/load/model_manager/task_info/hccl_task_info.cc"
    "${AIR_CODE_DIR}/executor/graph/load/model_manager/task_info/kernel_ex_task_info.cc"
    "${AIR_CODE_DIR}/executor/graph/load/model_manager/task_info/kernel_task_info.cc"
    "${AIR_CODE_DIR}/executor/graph/load/model_manager/task_info/label_goto_ex_task_info.cc"
    "${AIR_CODE_DIR}/executor/graph/load/model_manager/task_info/label_set_task_info.cc"
    "${AIR_CODE_DIR}/executor/graph/load/model_manager/task_info/label_switch_by_index_task_info.cc"
    "${AIR_CODE_DIR}/executor/graph/load/model_manager/task_info/memcpy_addr_async_task_info.cc"
    "${AIR_CODE_DIR}/executor/graph/load/model_manager/task_info/memcpy_async_task_info.cc"
    "${AIR_CODE_DIR}/executor/graph/load/model_manager/task_info/model_exit_task_info.cc"
    "${AIR_CODE_DIR}/executor/graph/load/model_manager/task_info/profiler_trace_task_info.cc"
    "${AIR_CODE_DIR}/executor/graph/load/model_manager/task_info/stream_active_task_info.cc"
    "${AIR_CODE_DIR}/executor/graph/load/model_manager/task_info/stream_switch_task_info.cc"
    "${AIR_CODE_DIR}/executor/graph/load/model_manager/task_info/dsa_task_info.cc"
    "${AIR_CODE_DIR}/executor/graph/load/model_manager/task_info/super_kernel/super_kernel.cc"
    "${AIR_CODE_DIR}/executor/graph/load/model_manager/task_info/super_kernel/super_kernel_factory.cc"
    "${AIR_CODE_DIR}/executor/hybrid/node_executor/aicpu/aicpu_ext_info.cc"
    "${AIR_CODE_DIR}/executor/runtime/deploy/local_model_deployer.cc"
    "${AIR_CODE_DIR}/executor/runtime/deploy/local_exchange_service.cc"
    "${AIR_CODE_DIR}/executor/runtime/deploy/local_exchange_deployer.cc"
    "${AIR_CODE_DIR}/executor/graph/load/model_manager/heterogeneous_model_executor.cc"
    "${AIR_CODE_DIR}/executor/runtime/local_execution_runtime.cc"
)

set(GRAPH_BUILD_COMMON_SRC_FILES
    "${AIR_CODE_DIR}/runner/client/ge_api.cc"
    "${AIR_CODE_DIR}/runner/common/plugin/tbe_plugin_manager.cc"
    "${AIR_CODE_DIR}/runner/common/profiling/ge_runner_profiling.cc"
    "${AIR_CODE_DIR}/runner/common/profiling/profiling_init.cc"
    "${AIR_CODE_DIR}/runner/session/inner_session.cc"
    "${AIR_CODE_DIR}/runner/session/session_manager.cc"
    "${AIR_CODE_DIR}/base/pne/manager/process_node_engine_manager.cc"
)

set(GRAPH_PASS_COMMON_SRC_FILES
    "${AIR_CODE_DIR}/compiler/graph/passes/pass_manager.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/base_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/variable_prepare_op_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/variable_ref_delete_op_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/atomic_addr_clean_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/set_ffts_plus_attr_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/constant_folding_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/iterator_op_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/net_output_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/print_op_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/dimension_compute_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/dimension_adjust_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/get_original_format_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/stop_gradient_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/guarantee_const_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/prevent_gradient_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/identity_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/placeholder_with_default_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/snapshot_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/shape_operate_op_remove_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/permute_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/var_is_initialized_op_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/prune_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/enter_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/next_iteration_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/pass_utils.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/addn_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/save_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/merge_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/switch_logic_remove_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/assert_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/split_shape_n_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/merge_unknown_shape_n_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/dropout_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/unused_const_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/reshape_remove_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/remove_same_const_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/useless_control_out_remove_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/transop_breadth_fusion_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/transop_without_reshape_fusion_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/transop_nearby_allreduce_fusion_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/same_transdata_breadth_fusion_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/compile_nodes_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/flow_ctrl_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/parallel_group_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/folding_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/variable_op_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/transpose_transdata_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/hccl_memcpy_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/no_use_reshape_remove_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/infer_base_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/infershape_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/infer_value_range_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/pass_utils.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/base_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/bitcast_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/constant_folding_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/aicpu_constant_folding_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/reshape_remove_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/reshape_recovery_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/transop_breadth_fusion_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/transop_nearby_allreduce_fusion_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/transop_without_reshape_fusion_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/compile_nodes_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/variable_prepare_op_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/variable_ref_delete_op_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/variable_ref_useless_control_out_delete_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/subgraph_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/data_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/net_output_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/replace_transshape_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/constant_fuse_same_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/fuse_data_nodes_with_common_input_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/print_op_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/no_use_reshape_remove_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/iterator_op_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/input_output_connection_identify_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/atomic_addr_clean_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/mark_same_addr_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/mark_graph_unknown_status_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/mark_agnostic_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/dimension_compute_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/dimension_adjust_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/get_original_format_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/shape_operate_op_remove_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/assert_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/dropout_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/infer_base_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/infershape_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/infer_value_range_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/unused_const_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/permute_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/ctrl_edge_transfer_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/end_of_sequence_add_control_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/start_of_sequence_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/stop_gradient_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/prevent_gradient_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/identity_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/ref_identity_delete_op_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/placeholder_with_default_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/snapshot_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/guarantee_const_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/var_is_initialized_op_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/parallel_concat_start_op_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/folding_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/prune_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/merge_to_stream_merge_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/merge_input_memcpy_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/switch_to_stream_switch_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/mark_force_unknown_for_cond_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/attach_stream_label_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/create_subgraph_with_scope_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/multi_batch_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/subgraph_multi_dims_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/multi_batch_clone_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/subgraph_multi_dims_clone_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/subexpression_migration_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/subgraph_const_migration_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/unused_args_clean_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/next_iteration_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/control_trigger_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/cond_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/cond_remove_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/for_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/enter_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/assign_remove_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/inplace_support_check_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/addn_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/common_subexpression_elimination_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/transop_symmetry_elimination_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/save_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/switch_dead_branch_elimination.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/switch_logic_remove_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/switch_data_edges_bypass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/merge_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/variable_op_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/cast_remove_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/transpose_transdata_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/hccl_continuous_memcpy_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/flow_ctrl_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/global_step_insert_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/link_gen_mask_nodes_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/replace_with_empty_const_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/hccl_group_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/hccl_tailing_optimization_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/memcpy_addr_async_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/set_input_output_offset_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/remove_same_const_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/useless_control_out_remove_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/parallel_group_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/buffer_pool_memory_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/mds_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/mds_kernels/base_mds_kernel.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/mds_kernels/mds_utils.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/data_flow_prepare_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/potential_const_taken_effect_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/potential_folding_pass.cc"
    "${AIR_CODE_DIR}/compiler/graph/passes/swap_space_pass.cc"
)

set(KERNEL_SRC_FILES
    "${AIR_CODE_DIR}/compiler/host_kernels/broadcast_gradient_args_kernel.cc"
    "${AIR_CODE_DIR}/compiler/host_kernels/broadcast_args_kernel.cc"
    "${AIR_CODE_DIR}/compiler/host_kernels/greater_kernel.cc"
    "${AIR_CODE_DIR}/compiler/host_kernels/gathershapes_kernel.cc"
    "${AIR_CODE_DIR}/compiler/host_kernels/flattenv2_kernel.cc"
    "${AIR_CODE_DIR}/compiler/host_kernels/gather_v2_kernel.cc"
    "${AIR_CODE_DIR}/compiler/host_kernels/maximum_kernel.cc"
    "${AIR_CODE_DIR}/compiler/host_kernels/floormod_kernel.cc"
    "${AIR_CODE_DIR}/compiler/host_kernels/floordiv_kernel.cc"
    "${AIR_CODE_DIR}/compiler/host_kernels/range_kernel.cc"
    "${AIR_CODE_DIR}/compiler/host_kernels/mul_kernel.cc"
    "${AIR_CODE_DIR}/compiler/host_kernels/shape_kernel.cc"
    "${AIR_CODE_DIR}/compiler/host_kernels/shape_n_kernel.cc"
    "${AIR_CODE_DIR}/compiler/host_kernels/size_kernel.cc"
    "${AIR_CODE_DIR}/compiler/host_kernels/rank_kernel.cc"
    "${AIR_CODE_DIR}/compiler/host_kernels/fill_kernel.cc"
    "${AIR_CODE_DIR}/compiler/host_kernels/empty_kernel.cc"
    "${AIR_CODE_DIR}/compiler/host_kernels/cast_kernel.cc"
    "${AIR_CODE_DIR}/compiler/host_kernels/transdata_kernel.cc"
    "${AIR_CODE_DIR}/compiler/host_kernels/permute_kernel.cc"
    "${AIR_CODE_DIR}/compiler/host_kernels/squeeze_kernel.cc"
    "${AIR_CODE_DIR}/compiler/host_kernels/squeezev3_kernel.cc"
    "${AIR_CODE_DIR}/compiler/host_kernels/unsqueezev3_kernel.cc"
    "${AIR_CODE_DIR}/compiler/host_kernels/reshape_kernel.cc"
    "${AIR_CODE_DIR}/compiler/host_kernels/reformat_kernel.cc"
    "${AIR_CODE_DIR}/compiler/host_kernels/kernel_utils.cc"
    "${AIR_CODE_DIR}/compiler/host_kernels/expanddims_kernel.cc"
    "${AIR_CODE_DIR}/compiler/host_kernels/ssd_prior_box_kernel.cc"
    "${AIR_CODE_DIR}/compiler/host_kernels/pack_kernel.cc"
    "${AIR_CODE_DIR}/compiler/host_kernels/strided_slice_kernel.cc"
    "${AIR_CODE_DIR}/compiler/host_kernels/concat_v2_kernel.cc"
    "${AIR_CODE_DIR}/compiler/host_kernels/add_kernel.cc"
    "${AIR_CODE_DIR}/compiler/host_kernels/sub_kernel.cc"
    "${AIR_CODE_DIR}/compiler/host_kernels/reduce_prod_kernel.cc"
    "${AIR_CODE_DIR}/compiler/host_kernels/rsqrt_kernel.cc"
    "${AIR_CODE_DIR}/compiler/host_kernels/concat_offset_kernel.cc"
    "${AIR_CODE_DIR}/compiler/host_kernels/slice_kernel.cc"
    "${AIR_CODE_DIR}/compiler/host_kernels/dynamic_stitch_kernel.cc"
    "${AIR_CODE_DIR}/compiler/host_kernels/identity_kernel.cc"
    "${AIR_CODE_DIR}/compiler/host_kernels/slice_d_kernel.cc"
    "${AIR_CODE_DIR}/compiler/host_kernels/transpose_kernel.cc"
    "${AIR_CODE_DIR}/compiler/host_kernels/unpack_kernel.cc"
    "${AIR_CODE_DIR}/compiler/host_kernels/unsqueeze_kernel.cc"
)

set(SINGLE_OP_SRC_FILES
    "${AIR_CODE_DIR}/executor/executor/ge_executor.cc"
    "${AIR_CODE_DIR}/executor/single_op/task/build_task_utils.cc"
    "${AIR_CODE_DIR}/executor/single_op/task/op_task.cc"
    "${AIR_CODE_DIR}/executor/single_op/task/tbe_task_builder.cc"
    "${AIR_CODE_DIR}/executor/single_op/single_op.cc"
    "${AIR_CODE_DIR}/executor/single_op/single_op_model.cc"
    "${AIR_CODE_DIR}/executor/single_op/stream_resource.cc"
    "${AIR_CODE_DIR}/executor/single_op/single_op_manager.cc"
    "${AIR_CODE_DIR}/executor/single_op/task/aicpu_task_builder.cc"
    "${AIR_CODE_DIR}/executor/single_op/task/aicpu_kernel_task_builder.cc"
    "${AIR_CODE_DIR}/executor/single_op/task/rts_kernel_task_builder.cc"
    "${AIR_CODE_DIR}/executor/hybrid/common/tensor_value.cc"
    "${AIR_CODE_DIR}/executor/hybrid/common/npu_memory_allocator.cc"
    "${AIR_CODE_DIR}/executor/hybrid/executor/rt_callback_manager.cc"
    "${AIR_CODE_DIR}/executor/hybrid/executor/host_cpu_callback_manager.cc"
    "${AIR_CODE_DIR}/executor/hybrid/executor/node_state.cc"
    "${AIR_CODE_DIR}/executor/hybrid/executor/node_done_manager.cc"
    "${AIR_CODE_DIR}/executor/hybrid/executor/hybrid_profiler.cc"
    "${AIR_CODE_DIR}/executor/hybrid/executor/hybrid_model_executor.cc"
    "${AIR_CODE_DIR}/executor/hybrid/executor/hybrid_model_async_executor.cc"
    "${AIR_CODE_DIR}/executor/hybrid/executor/hybrid_execution_context.cc"
    "${AIR_CODE_DIR}/executor/hybrid/executor/hybrid_model_pipeline_executor.cc"
    "${AIR_CODE_DIR}/executor/hybrid/executor/subgraph_context.cc"
    "${AIR_CODE_DIR}/executor/hybrid/executor/subgraph_executor.cc"
    "${AIR_CODE_DIR}/executor/hybrid/executor/hybrid_data_flow.cc"
    "${AIR_CODE_DIR}/executor/hybrid/executor//hybrid_resource_manager.cc"
    "${AIR_CODE_DIR}/executor/hybrid/executor/worker/shape_inference_engine.cc"
    "${AIR_CODE_DIR}/executor/hybrid/executor/worker/execution_engine.cc"
    "${AIR_CODE_DIR}/executor/hybrid/model/infer/graph_stage_cache.cc"
    "${AIR_CODE_DIR}/executor/hybrid/model/infer/node_shape_infer.cc"
    "${AIR_CODE_DIR}/executor/hybrid/model/infer/node_shape_propagator.cc"
    "${AIR_CODE_DIR}/executor/hybrid/model/infer/shape_utils.cc"
    "${AIR_CODE_DIR}/executor/hybrid/model/infer/shape_propagator.cc"
    "${AIR_CODE_DIR}/executor/hybrid/model/infer/tensor_desc_observer.cc"
    "${AIR_CODE_DIR}/executor/hybrid/model/infer/tensor_desc_holder.cc"
    "${AIR_CODE_DIR}/executor/hybrid/model/hybrid_model.cc"
    "${AIR_CODE_DIR}/executor/hybrid/model/hybrid_model_builder.cc"
    "${AIR_CODE_DIR}/executor/hybrid/model/node_item.cc"
    "${AIR_CODE_DIR}/executor/hybrid/model/graph_item.cc"
    "${AIR_CODE_DIR}/executor/hybrid/node_executor/aicore/aicore_node_executor.cc"
    "${AIR_CODE_DIR}/executor/hybrid/node_executor/aicore/aicore_op_task.cc"
    "${AIR_CODE_DIR}/executor/hybrid/node_executor/aicore/aicore_task_builder.cc"
    "${AIR_CODE_DIR}/executor/hybrid/node_executor/aicpu/aicpu_node_executor.cc"
    "${AIR_CODE_DIR}/executor/hybrid/node_executor/compiledsubgraph/known_node_executor.cc"
    "${AIR_CODE_DIR}/executor/hybrid/node_executor/ge_local/data_flow_kernels.cc"
    "${AIR_CODE_DIR}/executor/hybrid/node_executor/ge_local/ge_local_node_executor.cc"
    "${AIR_CODE_DIR}/executor/hybrid/node_executor/host_cpu/host_cpu_node_executor.cc"
    "${AIR_CODE_DIR}/executor/hybrid/node_executor/controlop/control_op_executor.cc"
    "${AIR_CODE_DIR}/executor/hybrid/node_executor/partitioned_call/partitioned_call_node_executor.cc"
    "${AIR_CODE_DIR}/executor/hybrid/node_executor/ffts_plus/ffts_plus_node_task.cc"
    "${AIR_CODE_DIR}/executor/hybrid/node_executor/ffts_plus/ffts_plus_subgraph_executor.cc"
    "${AIR_CODE_DIR}/executor/hybrid/node_executor/ffts_plus/ffts_plus_sub_node_executor.cc"
    "${AIR_CODE_DIR}/executor/hybrid/node_executor/ffts_plus/ffts_plus_sub_task.cc"
    "${AIR_CODE_DIR}/executor/hybrid/node_executor/ffts_plus/ffts_plus_sub_task_factory.cc"
    "${AIR_CODE_DIR}/executor/hybrid/node_executor/hccl/hccl_node_executor.cc"
    "${AIR_CODE_DIR}/executor/hybrid/node_executor/rts/rts_node_executor.cc"
    "${AIR_CODE_DIR}/executor/hybrid/node_executor/rts/rts_node_task.cc"
    "${AIR_CODE_DIR}/executor/hybrid/node_executor/rts/rts_task_factory.cc"
    "${AIR_CODE_DIR}/executor/hybrid/node_executor/node_executor.cc"
    "${AIR_CODE_DIR}/executor/hybrid/node_executor/task_context.cc"
    "${AIR_CODE_DIR}/executor/hybrid/hybrid_davinci_model.cc"
)

set(HELPER_RUNTIME_SRC_FILES
        "${AIR_CODE_DIR}/runtime/heterogeneous/common/config/configurations.cc"
        "${AIR_CODE_DIR}/runtime/heterogeneous/common/config/device_debug_config.cc"
        "${AIR_CODE_DIR}/runtime/heterogeneous/common/config/json_parser.cc"
        "${AIR_CODE_DIR}/runtime/heterogeneous/common/data_flow/queue/helper_exchange_service.cc"
        "${AIR_CODE_DIR}/runtime/heterogeneous/common/mem_grp/memory_group_manager.cc"
        "${AIR_CODE_DIR}/runtime/heterogeneous/daemon/daemon_client_manager.cc"
        "${AIR_CODE_DIR}/runtime/heterogeneous/daemon/daemon_service.cc"
        "${AIR_CODE_DIR}/runtime/heterogeneous/daemon/deployer_daemon_client.cc"
        "${AIR_CODE_DIR}/runtime/heterogeneous/deploy/helper_execution_runtime.cc"
        "${AIR_CODE_DIR}/runtime/heterogeneous/deploy/deployer/deploy_context.cc"
        "${AIR_CODE_DIR}/runtime/heterogeneous/deploy/deployer/deployer.cc"
        "${AIR_CODE_DIR}/runtime/heterogeneous/deploy/deployer/deployer_service_impl.cc"
        "${AIR_CODE_DIR}/runtime/heterogeneous/deploy/deployer/deployer_proxy.cc"
        "${AIR_CODE_DIR}/runtime/heterogeneous/deploy/deployer/helper_deploy_planner.cc"
        "${AIR_CODE_DIR}/runtime/heterogeneous/deploy/deployer/master_model_deployer.cc"
        "${AIR_CODE_DIR}/runtime/heterogeneous/deploy/flowrm/datagw_manager.cc"
        "${AIR_CODE_DIR}/runtime/heterogeneous/deploy/flowrm/flow_route_manager.cc"
        "${AIR_CODE_DIR}/runtime/heterogeneous/deploy/flowrm/helper_exchange_deployer.cc"
        "${AIR_CODE_DIR}/runtime/heterogeneous/deploy/flowrm/network_manager.cc"
        "${AIR_CODE_DIR}/runtime/heterogeneous/deploy/hcom/cluster/cluster_client.cc"
        "${AIR_CODE_DIR}/runtime/heterogeneous/deploy/hcom/cluster/cluster_data.cc"
        "${AIR_CODE_DIR}/runtime/heterogeneous/deploy/hcom/cluster/cluster_manager.cc"
        "${AIR_CODE_DIR}/runtime/heterogeneous/deploy/hcom/cluster/cluster_parser.cc"
        "${AIR_CODE_DIR}/runtime/heterogeneous/deploy/hcom/cluster/cluster_service_impl.cc"
        "${AIR_CODE_DIR}/runtime/heterogeneous/deploy/hcom/communication_domain.cc"
        "${AIR_CODE_DIR}/runtime/heterogeneous/deploy/hcom/rank_parser.cc"
        "${AIR_CODE_DIR}/runtime/heterogeneous/deploy/resource/device_info.cc"
        "${AIR_CODE_DIR}/runtime/heterogeneous/deploy/resource/node_info.cc"
        "${AIR_CODE_DIR}/runtime/heterogeneous/deploy/resource/resource_manager.cc"
        "${AIR_CODE_DIR}/runtime/heterogeneous/executor/cpu_sched_event_dispatcher.cc"
        "${AIR_CODE_DIR}/runtime/heterogeneous/executor/cpu_sched_model.cc"
        "${AIR_CODE_DIR}/runtime/heterogeneous/executor/dynamic_model_executor.cc"
        "${AIR_CODE_DIR}/runtime/heterogeneous/executor/engine_daemon.cc"
        "${AIR_CODE_DIR}/runtime/heterogeneous/executor/event_handler.cc"
        "${AIR_CODE_DIR}/runtime/heterogeneous/executor/executor_context.cc"
        "${AIR_CODE_DIR}/runtime/heterogeneous/executor/incremental_model_parser.cc"
        #   "${AIR_CODE_DIR}/runtime/deploy/daemon/executor_manager.cc"
        #   "${AIR_CODE_DIR}/runtime/deploy/daemon/grpc_server.cc"
        #   "${AIR_CODE_DIR}/runtime/deploy/daemon/model_deployer_daemon.cc"
        #   "${AIR_CODE_DIR}/runtime/deploy/executor/npu_executor.cc"
        "${DEPLOYER_PROTO_SRCS}"
        )

set(LABEL_MAKER_SRC_FILES
    "${AIR_CODE_DIR}/compiler/graph/label/label_maker.cc"
    "${AIR_CODE_DIR}/compiler/graph/label/case_label_maker.cc"
    "${AIR_CODE_DIR}/compiler/graph/label/if_label_maker.cc"
    "${AIR_CODE_DIR}/compiler/graph/label/while_label_maker.cc"
    "${AIR_CODE_DIR}/compiler/graph/label/partitioned_call_label_maker.cc"
)

# test files
set(COMMON_TEST_FILES
    "graph/passes/graph_builder_utils.cc"
    "graph/utils/buffer_pool_graph_builder.cc"
    "test.cc"
    "graph/bin_cache/node_compile_cache_module_unittest.cc"
)

set(HETEROGENEOUS_MODEL_TEST_FILES
    "common/execution_runtime_unittest.cc"
    "common/model_relation_unittest.cc"
    "common/deploy_planner_unittest.cc"
    "runtime/deploy/stub_models.cc"
    "runtime/deploy/local_model_deployer_unittest.cc"
    "runtime/deploy/local_exchange_service_unittest.cc"
    "runtime/deploy/local_exchange_deployer_unittest.cc"
    "graph/load/heterogeneous_model_executor_unittest.cc"
)

set(DISTINCT_GRAPH_LOAD_TEST_FILES
    "graph/load/aipp_utils_unittest.cc"
    "graph/load/data_dumper_unittest.cc"
    "graph/load/model_manager_unittest.cc"
    "graph/load/new_model_manager_model_manager_aicpu_unittest.cc"
    "graph/load/end_graph_task_unittest.cc"
    "graph/load/event_record_task_unittest.cc"
    "graph/load/event_wait_task_unittest.cc"
    "graph/load/davinci_model_unittest.cc"
    "graph/load/kernel_handle_unittest.cc"
    "graph/load/tbe_handle_store_unittest.cc"
    "graph/load/super_kernel_factory_unittest.cc"
    "graph/load/hccl_task_info_unittest.cc"
    "graph/load/kernel_ex_task_info_unittest.cc"
    "graph/load/kernel_task_info_unittest.cc"
    "graph/load/cmo_task_info_unittest.cc"
    "graph/load/cmo_barrier_task_info_unittest.cc"
    "graph/load/ffts_task_info_unittest.cc"
    "graph/load/ffts_plus_task_info_unittest.cc"
    "graph/load/dsa_task_info_unittest.cc"
    "graph/load/memcpy_addr_async_task_info_unittest.cc"
    "graph/load/memcpy_async_task_info_unittest.cc"
    "graph/load/cpu_queue_schedule_unittest.cc"
    "executor/ge_executor_unittest.cc"
    "executor/executor_utils_unittest.cc"
    "graph/load/model_helper_unittest.cc"
    "graph/load/om_file_helper_unittest.cc"
    "hybrid/hybrid_davinci_model_unittest.cc"
    "graph/load/model_utils_unittest.cc"
    "graph/load/profiler_trace_task_info_unittest.cc"
    "graph/load/super_kernel_unittest.cc"
    "graph/load/graph_loader_unittest.cc"
    "graph/load/stream_switch_task_info_unittest.cc"
    "graph/load/scattered_collection_unittest.cc"
)

set(PASS_TEST_FILES
    "graph/passes/addn_pass_unittest.cc"
    "graph/passes/assign_remove_pass_unittest.cc"
    "graph/passes/atomic_addr_clean_pass_unittest.cc"
    "graph/passes/bitcast_pass_unittest.cc"
    "graph/passes/compile_nodes_pass_unittest.cc"
    "graph/passes/cond_pass_unittest.cc"
    "graph/passes/cond_remove_pass_unittest.cc"
    "graph/passes/aicpu_constant_folding_pass_unittest.cc"
    "graph/passes/control_trigger_pass_unittest.cc"
    "graph/passes/transop_without_reshape_fusion_pass_unittest.cc"
    "graph/passes/infer_value_range_pass_unittest.cc"
    "graph/passes/infer_base_pass_unittest.cc"
    "graph/passes/prune_pass_unittest.cc"
    "graph/passes/enter_pass_unittest.cc"
    "graph/passes/for_pass_unittest.cc"
    "graph/passes/switch_op_pass_unittest.cc"
    "graph/passes/get_original_format_pass_unittest.cc"
    "graph/passes/pass_manager_unittest.cc"
    "graph/passes/permute_pass_unittest.cc"
    "graph/passes/print_op_pass_unittest.cc"
    "graph/passes/shape_operate_op_remove_pass_unittest.cc"
    "graph/passes/variable_op_pass_unittest.cc"
    "graph/passes/base_pass_unittest.cc"
    "graph/passes/save_pass_unittest.cc"
    "graph/passes/merge_pass_unittest.cc"
    "graph/passes/multi_batch_pass_unittest.cc"
    "graph/passes/switch_logic_remove_pass_unittest.cc"
    "graph/passes/cond_branch_v1_unittest.cc"
    "graph/passes/loop_branch_v1_unittest.cc"
    "graph/passes/switch_dead_branch_elimination_unittest.cc"
    "graph/passes/subexpression_migration_pass_unittest.cc"
    "graph/passes/subgraph_pass_unittest.cc"
    "graph/passes/switch_to_stream_switch_pass_unittest.cc"
    "graph/passes/assert_pass_unittest.cc"
    "graph/passes/dropout_pass_unittest.cc"
    "graph/passes/end_of_sequence_add_control_pass_unittest.cc"
    "graph/passes/ctrl_edge_transfer_pass_unittest.cc"
    "graph/passes/common_subexpression_elimination_pass_unittest.cc"
    "graph/passes/unused_const_pass_unittest.cc"
    "graph/passes/reshape_remove_pass_unittest.cc"
    "graph/passes/ref_identity_delete_op_pass_unittest.cc"
    "graph/passes/trans_op_breadth_fusion_pass_unittest.cc"
    "graph/passes/transop_nearby_allreduce_fusion_pass_unittest.cc"
    "graph/passes/constant_folding_pass_unittest.cc"
    "graph/passes/potential_const_taken_effect_pass_unittest.cc"
    "graph/passes/fuse_data_nodes_with_common_input_pass_unittest.cc"
    "graph/passes/stop_gradient_pass_unittest.cc"
    "graph/passes/prevent_gradient_pass_unittest.cc"
    "graph/passes/identity_pass_unittest.cc"
    "graph/passes/global_step_insert_pass_unittest.cc"
    "graph/passes/placeholder_with_default_pass_unittest.cc"
    "graph/passes/snapshot_pass_unittest.cc"
    "graph/passes/guarantee_const_pass_unittest.cc"
    "graph/passes/flow_ctrl_pass_unittest.cc"
    "graph/passes/dimension_compute_pass_unittest.cc"
    "graph/passes/variable_prepare_pass_unittest.cc"
    "graph/passes/hccl_tailing_optimization_pass_unittest.cc"
    "graph/passes/inplace_support_check_pass_unittest.cc"
    "graph/passes/next_iteration_pass_unittest.cc"
    "graph/passes/variable_ref_delete_pass_unittest.cc"
    "graph/passes/dimension_adjust_pass_unittest.cc"
    "graph/passes/pass_utils_unittest.cc"
    "graph/passes/net_output_pass_unittest.cc"
    "graph/passes/no_use_reshape_remove_pass_unittest.cc"
    "graph/passes/infershape_pass_unittest.cc"
    "graph/passes/mark_force_unknown_for_cond_pass_unittest.cc"
    "graph/passes/multi_batch_clone_pass_unittest.cc"
    "graph/passes/constant_fuse_same_pass_unittest.cc"
    "graph/passes/subgraph_multi_dims_clone_pass_unittest.cc"
    "graph/passes/create_subgraph_with_scope_pass_unittest.cc"
    "graph/passes/subgraph_const_migration_pass_unittest.cc"
    "graph/passes/start_of_sequence_pass_unittest.cc"
    "graph/passes/replace_with_empty_const_pass_unittest.cc"
    "graph/passes/link_gen_mask_nodes_pass_unittest.cc"
    "graph/passes/transpose_transdata_pass_unittest.cc"
    "graph/passes/parallel_group_pass_unittest.cc"
    "graph/passes/buffer_pool_memory_pass_unittest.cc"
    "graph/passes/reshape_recovery_pass_unittest.cc"
    "graph/passes/cast_remove_pass_unittest.cc"
    "graph/passes/memcpy_addr_async_unittest.cc"
    "graph/passes/hccl_continuous_pass_unittest.cc"
    "graph/passes/hccl_memcpy_pass_unittest.cc"
    "graph/passes/same_transdata_breadth_fusion_pass_unittest.cc"
    "graph/passes/set_ffts_plus_attr_pass_unittest.cc"
    "graph/passes/mds_pass_unittest.cc"
    "graph/passes/transop_symmetry_elimination_pass_unittest.cc"
    "graph/passes/data_flow_prepare_pass_unittest.cc"
    "graph/passes/data_pass_unittest.cc"
    "graph/passes/merge_shape_n_pass_unittest.cc"
    "graph/passes/parallel_concat_start_op_pass_unittest.cc"
    "graph/passes/switch_data_edges_bypass_unittest.cc"
    "graph/passes/set_input_output_offset_pass_unittest.cc"
    "graph/passes/hccl_group_pass_unittest.cc"
    "graph/passes/mark_same_addr_pass_unittest.cc"
    "graph/passes/mark_graph_unknown_status_pass_unittest.cc"
    "graph/passes/mark_agnostic_pass_unittest.cc"
    "graph/passes/iterator_op_pass_unittest.cc"
    "graph/passes/unused_args_clean_pass_unittest.cc"
    "graph/passes/var_is_initialized_op_pass_unittest.cc"
    "graph/passes/replace_transshape_pass_unittest.cc"
    "graph/passes/remove_same_const_pass_unittest.cc"
    "graph/passes/input_output_connection_identify_pass_unittest.cc"
    "graph/passes/variable_ref_useless_control_out_delete_pass_unittest.cc"
    "graph/passes/split_shape_n_pass_unittest.cc"
    "graph/passes/swap_space_pass_unittest.cc"
)

set(KERNEL_TEST_FILES
    "graph/passes/folding_kernel/greater_kernel_unittest.cc"
    "graph/passes/folding_kernel/maximum_kernel_unittest.cc"
    "graph/passes/folding_kernel/floormod_kernel_unittest.cc"
    "graph/passes/folding_kernel/floordiv_kernel_unittest.cc"
    "graph/passes/folding_kernel/range_kernel_unittest.cc"
    "graph/passes/folding_kernel/mul_kernel_unittest.cc"
    "graph/passes/folding_kernel/shape_kernel_unittest.cc"
    "graph/passes/folding_kernel/shape_n_kernel_unittest.cc"
    "graph/passes/folding_kernel/size_kernel_unittest.cc"
    "graph/passes/folding_kernel/broadcast_args_kernel_unittest.cc"
    "graph/passes/folding_kernel/broadcast_gradient_args_kernel_unittest.cc"
    "graph/passes/folding_kernel/fill_kernel_unittest.cc"
    "graph/passes/folding_kernel/empty_kernel_unittest.cc"
    "graph/passes/folding_kernel/rank_kernel_unittest.cc"
    "graph/passes/folding_kernel/expanddims_kernel_unittest.cc"
    "graph/passes/folding_kernel/reshape_kernel_unittest.cc"
    "graph/passes/folding_kernel/reformat_kernel_unittest.cc"
    "graph/passes/folding_kernel/squeeze_kernel_unittest.cc"
    "graph/passes/folding_kernel/squeezev3_kernel_unittest.cc"
    "graph/passes/folding_kernel/transdata_kernel_unittest.cc"
    "graph/passes/folding_kernel/permute_kernel_unittest.cc"
    "graph/passes/folding_kernel/cast_kernel_unittest.cc"
    "graph/passes/folding_kernel/ssd_prior_box_kernel_unittest.cc"
    "graph/passes/folding_kernel/strided_slice_kernel_unittest.cc"
    "graph/passes/folding_kernel/pack_kernel_unittest.cc"
    "graph/passes/folding_kernel/concat_v2_kernel_unittest.cc"
    "graph/passes/folding_kernel/add_kernel_unittest.cc"
    "graph/passes/folding_kernel/sub_kernel_unittest.cc"
    "graph/passes/folding_kernel/reduce_prod_kernel_unittest.cc"
    "graph/passes/folding_kernel/rsqrt_kernel_unittest.cc"
    "graph/passes/folding_kernel/concat_offset_kernel_unittest.cc"
    "graph/passes/folding_kernel/gather_v2_kernel_unittest.cc"
    "graph/passes/folding_kernel/slice_kernel_unittest.cc"
    "graph/passes/folding_kernel/dynamic_stitch_kernel_unittest.cc"
    "graph/passes/folding_kernel/gathershapes_kernel_unittest.cc"
    "graph/passes/folding_kernel/flattenv2_kernel_unittest.cc"
    "graph/passes/folding_kernel/identity_kernel_unittest.cc"
    "graph/passes/folding_kernel/slice_d_kernel_unittest.cc"
    "graph/passes/folding_kernel/transpose_kernel_unittest.cc"
    "graph/passes/folding_kernel/unpack_kernel_unittest.cc"
    "graph/passes/folding_kernel/unsqueeze_kernel_unittest.cc"
    "graph/passes/folding_kernel/unsqueezev3_kernel_unittest.cc"
    "graph/passes/folding_kernel/kernel_utils_unittest.cc"
)

set(FORMAT_TRANSFERS_TEST_FILES
    "common/datatype_transfer_unittest.cc"
    "common/format_transfer_unittest.cc"
    "common/format_transfer_transpose_unittest.cc"
    "common/format_transfer_nchw_5d_unittest.cc"
    "common/format_transfer_nchw_fractalz_unittest.cc"
    "common/format_transfer_hwcn_fractalz_unittest.cc"
    "common/format_transfer_nhwc_fractalz_unittest.cc"
    "common/format_transfer_fractal_nz_unittest.cc"
    "common/format_transfer_fractal_zz_unittest.cc"
    "common/format_transfer_fractal_z_tbe_unittest.cc"
    "common/format_transfer_nhwc_5d_unittest.cc"
    "common/format_transfer_5d_nchw_unittest.cc"
    "common/format_transfer_5d_nhwc_unittest.cc"
    "common/format_transfer_hwcn_c1hwncoc0_unittest.cc"
    "common/format_transfer_c1hwncoc0_hwcn_unittest.cc"
    "common/format_transfer_fracz_nchw_unittest.cc"
    "common/format_transfer_fracz_nhwc_unittest.cc"
    "common/format_transfer_fracz_hwcn_unittest.cc"
    "common/format_transfer_dhwnc_fracz3D_transpose_unittest.cc"
    "common/format_transfer_dhwnc_fracz3D_unittest.cc"
    "common/format_transfer_nchw_fzc04_unittest.cc"
    "common/ge_format_util_unittest.cc"
    "common/register_format_transfer_unittest.cc"
)

set(BASE_COMMON_TEST_FILES
    "common/cust_aicpu_kernel_store_unittest.cc"
    "common/util_unittest.cc"
    "common/fp16_unittest.cc"
    "common/dump_manager_unittest.cc"
    "common/dump_properties_unittest.cc"
    "common/memory_dumper_unittest.cc"
    "common/model_saver_unittest.cc"
    "common/properties_manager_unittest.cc"
    "common/kernel_store_unittest.cc"
    "common/ge_auth_file_saver_unittest.cc"
    "common/tbe_plugin_manager_unittest.cc"
    "common/file_constant_unittest.cc"
    "common/profiling_definitions_unittest.cc"
    "common/dataslice_unittest.cc"
)

set(MULTI_PARTS_TEST_FILES
    "graph/common/trans_op_creator_unittest.cc"
    "graph/build/binary_block_mem_assigner_unittest.cc"
    "graph/build/buffer_pool_mem_assigner_unittest.cc"
    "graph/build/graph_mem_assigner_unittest.cc"
    "graph/build/block_mem_assigner_unittest.cc"
    "graph/build/hybrid_mem_assigner_unittest.cc"
    "graph/build/max_block_mem_assigner_unittest.cc"
    "graph/build/memory_assigner_unittest.cc"
    "common/analyzer_unittest.cc"
    "common/dnnengine_manager_unittest.cc"
    "graph/optimize/graph_optimize_unittest.cc"
    "graph/optimize/mem_rw_conflict_optimize_unittest.cc"
    "graph/preprocess/insert_op/ge_aipp_op_unittest.cc"
    "graph/preprocess/insert_op/util_insert_aipp_op_unittest.cc"
    "common/gelib_unittest.cc"
    "common/ge_local_engine_unittest.cc"
    "common/host_cpu_engine_unittest.cc"
    "common/ge_local_ops_kernel_builder_unittest.cc"
    "common/ge_local_ops_kernel_info_unittest.cc"
    "common/op_unittest.cc"
    "common/op_factory_unittest.cc"
    "common/no_op_unittest.cc"
    "common/ge_deleted_op_unittest.cc"
    "common/ops_kernel_builder_manager_unittest.cc"
    "common/ops_kernel_manager_unittest.cc"
    "common/dnnengines_unittest.cc"
    "common/engine_manage_unittest.cc"
    "graph_ir/ge_operator_factory_unittest.cc"
    "graph_ir/ge_ir_build_unittest.cc"
    "graph/transop_util_unittest.cc"
    "common/dump_op_unittest.cc"
    "common/dump_exception_unittest.cc"
    "common/opdebug_register_unittest.cc"
    "graph/graph_rebuild_state_ctrl_unittest.cc"
    "graph/build/logical_stream_allocator_unittest.cc"
    "graph/build/stream_allocator_unittest.cc"
    "graph/build/model_builder_unittest.cc"
    "graph/build/mem_assigner_unittest.cc"
    "graph/build/graph_mem_assigner_unittest.cc"
    "graph/build/task_generator_unittest.cc"
    "graph/build/buffer_pool_mem_assigner_unittest.cc"
    "graph/build/graph_builder_unittest.cc"
    "graph/build/label_allocator_unittest.cc"
    "graph/build/run_context_unittest.cc"
    "graph/execute/graph_execute_unittest.cc"
    "graph/execute/model_executor_unittest.cc"
    "graph/preprocess/graph_preprocess_unittest.cc"
    "graph/preprocess/multi_batch_copy_graph_unittest.cc"
    "graph/preprocess/insert_op/ge_aipp_op_unittest.cc"
    "graph/manager/hcom_util_unittest.cc"
    "graph/manager/graph_caching_allocator_unittest.cc"
    "graph/manager/host_mem_allocator_unittest.cc"
    "graph/manager/host_mem_manager_unittest.cc"
    "graph/manager/memory_api_unittest.cc"
    "graph/manager/rdma_pool_allocator_unittest.cc"
    "graph/manager/trans_var_data_utils_unittest.cc"
    "graph/manager/session_scope_mem_allocator_unittest.cc"
    "graph/manager/run_graph_unittest.cc"
    "graph/partition/pipeline_partition_unittest.cc"
    "graph/partition/dynamic_shape_partition_unittest.cc"
    "graph/partition/engine_place_unittest.cc"
    "graph/partition/graph_partition_unittest.cc"
    "graph/partition/stage_partition_unittest.cc"
    "graph/partition/optimizer/dynamic_data_flow_engine_reassign_unittest.cc"
    "graph/manager/graph_manager_unittest.cc"
    "graph/manager/graph_var_manager_unittest.cc"
    "graph/optimize/mem_rw_conflict_optimize_unittest.cc"
    "graph/optimize/graph_optimize_unittest.cc"
    "graph/optimize/summary_optimize_unittest.cc"
    "graph/fuzz_compile/fuzz_compile_bin_selector_unittest.cc"
    "graph/fuzz_compile/one_node_single_bin_selector_unittest.cc"
    "session/omg_omg_unittest.cc"
    "session/main_unittest.cc"
    "session/single_op_parser_unittest.cc"
    "session/ge_api_unittest.cc"
    "session/inner_session_unittest.cc"
    "session/session_manager_unittest.cc"
)

set(GE_OPT_INFO_TEST_FILES
    "opt_info/opt_info_unittest.cc"
)

set(GENERATOR_TEST_FILES
    "generator/ge_generator_unittest.cc"
)

set(SINGLE_OP_TEST_FILES
    "single_op/single_op_model_unittest.cc"
    "single_op/single_op_manager_unittest.cc"
    "single_op/stream_resource_unittest.cc"
    "single_op/single_op_task_unittest.cc"
    "single_op/single_op_unittest.cc"
    "single_op/task/op_task_unittest.cc"
    "single_op/task/tbe_task_builder_unittest.cc"
)

set(HELPER_RUNTIME_TEST_FILES
        "${AIR_CODE_DIR}/tests/ut/ge/runtime/common/device_debug_config_unittest.cc"
        "${AIR_CODE_DIR}/tests/ut/ge/runtime/common/json_parser_unittest.cc"
        "${AIR_CODE_DIR}/tests/ut/ge/runtime/communication/cluster_client_unittest.cc"
        "${AIR_CODE_DIR}/tests/ut/ge/runtime/communication/cluster_data_unittest.cc"
        "${AIR_CODE_DIR}/tests/ut/ge/runtime/communication/cluster_parser_unittest.cc"
        "${AIR_CODE_DIR}/tests/ut/ge/runtime/communication/comm_manager_unittest.cc"
        "${AIR_CODE_DIR}/tests/ut/ge/runtime/communication/comm_rank_parser_unittest.cc"
        "${AIR_CODE_DIR}/tests/ut/ge/runtime/deploy/deploy_context_unittest.cc"
        "${AIR_CODE_DIR}/tests/ut/ge/runtime/deploy/deployer_proxy_unittest.cc"
        "${AIR_CODE_DIR}/tests/ut/ge/runtime/deploy/dynamic_model_executor_unittest.cc"
        "${AIR_CODE_DIR}/tests/ut/ge/runtime/deploy/engine_daemon_unittest.cc"
        "${AIR_CODE_DIR}/tests/ut/ge/runtime/deploy/event_handler_unittest.cc"
        "${AIR_CODE_DIR}/tests/ut/ge/runtime/deploy/helper_deploy_planner_unittest.cc"
        "${AIR_CODE_DIR}/tests/ut/ge/runtime/deploy/helper_exchange_deployer_unittest.cc"
        "${AIR_CODE_DIR}/tests/ut/ge/runtime/deploy/helper_exchange_service_unittest.cc"
        "${AIR_CODE_DIR}/tests/ut/ge/runtime/deploy/incremental_model_parser_unittest.cc"
        "${AIR_CODE_DIR}/tests/ut/ge/runtime/deploy/master_model_deployer_unittest.cc"
        "${AIR_CODE_DIR}/tests/ut/ge/runtime/deploy/resource_manager_unittest.cc"
        "${AIR_CODE_DIR}/tests/ut/ge/runtime/deploy/stub_models.cc"
        "${AIR_CODE_DIR}/tests/ut/ge/runtime/device/deployer_client_unittest.cc"
        "${AIR_CODE_DIR}/tests/ut/ge/runtime/queue/datagw_manager_unittest.cc"
        "${AIR_CODE_DIR}/tests/ut/ge/runtime/queue/memory_group_manager_unittest.cc"
        "${AIR_CODE_DIR}/tests/ut/ge/runtime/worker/deployer_service_unittest.cc"
        "${AIR_CODE_DIR}/tests/ut/ge/runtime/worker/client_manager_unittest.cc"
        "${AIR_CODE_DIR}/tests/ut/ge/runtime/worker/daemon_service_unittest.cc"
        "${AIR_CODE_DIR}/tests/ut/ge/runtime/stub/queue/dgw_client_stub.cc"
        "${AIR_CODE_DIR}/tests/ut/ge/runtime/stub/queue/re_mem_group_stub.cc"
        "${AIR_CODE_DIR}/tests/depends/helper_runtime/src/cluster_grpc_client_stub.cc"
        "${AIR_CODE_DIR}/tests/depends/helper_runtime/src/cluster_server_stub.cc"
        "${AIR_CODE_DIR}/tests/depends/helper_runtime/src/deployer_client_stub.cc"
        "${AIR_CODE_DIR}/tests/depends/helper_runtime/src/executor_manager_stub.cc"
#    "${AIR_CODE_DIR}/tests/depends/helper_runtime/src/aicpusd_interface_stub.cc"
        )

set(PROFILING_MNG_TEST_FILES
    "profiling/ge_profiling_manager_unittest.cc"
    "profiling/profiling_properties_unittest.cc"
    "profiling/profiling_init_unittest.cc"
    "profiling/ge_runner_profilling_unittest.cc"
)

set(HYBRID_TEST_FILES
    "hybrid/ge_hybrid_unittest.cc"
    "hybrid/executor/node_state_unittest.cc"
    "hybrid/executor/subgraph_executor_unittest.cc"
    "hybrid/executor/worker/execution_engine_unittest.cc"
    "hybrid/model/hybrid_model_builder_unittest.cc"
    "hybrid/model/test_infer_shape_output.cc"
    "hybrid/node_executor/node_executor_unittest.cc"
    "hybrid/node_executor/task_context_unittest.cc"
    "hybrid/node_executor/rts/rts_node_task_unittest.cc"
    "hybrid/node_executor/host_cpu/host_cpu_node_task_unittest.cc"
    "hybrid/node_executor/ge_local/ge_local_node_executor_unittest.cc"
    "hybrid/node_executor/controlop/control_op_executor_unittest.cc"
    "hybrid/node_executor/compiledsubgraph/known_node_executor_unittest.cc"
    "hybrid/node_executor/hccl/hccl_node_executor_unittest.cc"
    "hybrid/node_executor/aicpu/aicpu_node_executor_unittest.cc"
    "hybrid/executor/hybrid_model_async_executor_unittest.cc"
    "hybrid/executor/hybrid_profiler_unittest.cc"
    "hybrid/executor/hybrid_model_executor_unittest.cc"
    "hybrid/executor/hybrid_model_pipeline_executor_unittest.cc"
    "hybrid/node_executor/aicore/aicore_op_task_unittest.cc"
    "hybrid/node_executor/aicore/aicore_node_executor_unittest.cc"
    "hybrid/node_executor/aicore/aicore_task_builder_unittest.cc"
    "hybrid/node_executor/ffts_plus/ffts_plus_subgraph_executor_unittest.cc"
    "hybrid/node_executor/ffts_plus/ffts_plus_sub_task_factory_unittest.cc"
    "hybrid/node_executor/ffts_plus/ffts_plus_sub_task_unittest.cc"
    "hybrid/node_executor/ffts_plus/ffts_plus_node_task_unittest.cc"
)

set(OTHERS_TEST_FILES
    "plugin_manager/ge_util_unittest.cc"
    "plugin_manager/plugin_manager_unittest.cc"
    "engine_manager/engine_manager_unittest.cc"
)

set(LABEL_MAKER_TEST_FILES
    "label_maker/label_maker_unittest.cc"
)

list(APPEND COMMON_SHARED_LIBRARIES
    c_sec
    slog_stub
    runtime_stub
    #tunner_stub
    profiler_stub
    mmpa_stub
    hccl_stub
    error_manager_stub
    opt_feature_stub
    ascend_protobuf
    json
)

# build graph
add_library(ge_ut_parser STATIC ${PARSER_SRC_FILES})

target_compile_definitions(ge_ut_parser PRIVATE
    google=ascend_private
    FMK_SUPPORT_DUMP
)

target_compile_options(ge_ut_parser PRIVATE
    -g
)

target_link_libraries(ge_ut_parser PRIVATE
    $<BUILD_INTERFACE:intf_pub>
    c_sec
    ascend_protobuf
    json
)

# build common
add_library(ge_ut_common STATIC ${COMMON_SRC_FILES} ${COMPILER_COMMON_SRC_FILES} ${EXECUTOR_COMMON_SRC_FILES} ${PROTO_HDRS})

target_compile_definitions(ge_ut_common PRIVATE
    google=ascend_private
    $<$<STREQUAL:${ENABLE_OPEN_SRC},True>:ONLY_COMPILE_OPEN_SRC>
)

target_compile_options(ge_ut_common PRIVATE
    -g --coverage -fprofile-arcs -ftest-coverage
    -Werror=format
)

target_link_libraries(ge_ut_common PRIVATE
    $<BUILD_INTERFACE:intf_pub>
    c_sec
    ascend_protobuf
    ge_ut_parser
    json
    runtime_stub
    gflags
)

target_link_libraries(ge_ut_common PUBLIC ge_graph_dsl)

# build common format
add_library(ge_ut_common_format STATIC ${COMMON_FORMAT_SRC_FILES} ${PROTO_HDRS})

target_compile_definitions(ge_ut_common_format PRIVATE
    google=ascend_private
    $<$<STREQUAL:${ENABLE_OPEN_SRC},True>:ONLY_COMPILE_OPEN_SRC>
)

target_compile_options(ge_ut_common_format PRIVATE
    -g --coverage -fprofile-arcs -ftest-coverage
    -Werror=format
)

target_link_libraries(ge_ut_common_format PRIVATE
    $<BUILD_INTERFACE:intf_pub>
    c_sec
    ascend_protobuf
    json
)

# build graph prepare common
add_library(ge_prepare_common STATIC ${GRAPH_PREPARE_COMMON_SRC_FILES} ${PROTO_HDRS})

target_compile_definitions(ge_prepare_common PRIVATE
    google=ascend_private
)

target_compile_options(ge_prepare_common PRIVATE
    -g --coverage -fprofile-arcs -ftest-coverage
    -Werror=format
)

target_link_libraries(ge_prepare_common PRIVATE
    $<BUILD_INTERFACE:intf_pub>
    c_sec
    ascend_protobuf
    json
)

# build build graph load common
add_library(ge_davinci_model STATIC ${GRAPH_DAVINCI_MODEL_SRC_FILES} ${PROTO_HDRS})

target_compile_definitions(ge_davinci_model PRIVATE
    google=ascend_private
    $<$<STREQUAL:${ENABLE_OPEN_SRC},True>:ONLY_COMPILE_OPEN_SRC>
)

target_compile_options(ge_davinci_model PRIVATE
    -g --coverage -fprofile-arcs -ftest-coverage
    -Werror=format
)

target_link_libraries(ge_davinci_model PRIVATE
    $<BUILD_INTERFACE:intf_pub>
    c_sec
    ascend_protobuf
    json
)

# build graph build common
add_library(ge_build_common STATIC ${GRAPH_BUILD_COMMON_SRC_FILES} ${PROTO_HDRS})

target_compile_definitions(ge_build_common PRIVATE
    google=ascend_private
)

target_compile_options(ge_build_common PRIVATE
    -g --coverage -fprofile-arcs -ftest-coverage
    -Werror=format
)

target_link_libraries(ge_build_common PRIVATE
    $<BUILD_INTERFACE:intf_pub>
    c_sec
    ascend_protobuf
    json
)

# build graph pass common
add_library(ge_pass_common STATIC ${GRAPH_PASS_COMMON_SRC_FILES} ${PROTO_HDRS})

target_compile_definitions(ge_pass_common PRIVATE
    google=ascend_private
)

target_compile_options(ge_pass_common PRIVATE
    -g --coverage -fprofile-arcs -ftest-coverage
    -Werror=format
)

target_link_libraries(ge_pass_common PRIVATE
    $<BUILD_INTERFACE:intf_pub>
    ascend_protobuf
    c_sec
    json
)

# build single_op common
add_library(ge_single_op STATIC ${SINGLE_OP_SRC_FILES} ${PROTO_HDRS})

target_compile_definitions(ge_single_op PRIVATE
    google=ascend_private
    $<$<STREQUAL:${ENABLE_OPEN_SRC},True>:ONLY_COMPILE_OPEN_SRC>
)

target_compile_options(ge_single_op PRIVATE
    -g --coverage -fprofile-arcs -ftest-coverage
    -Werror=format
)

target_link_libraries(ge_single_op PRIVATE
    $<BUILD_INTERFACE:intf_pub>
    ascend_protobuf
    json
    c_sec
    runtime_stub
)

#build runtime
add_library(helper_ut_runtime STATIC ${HELPER_RUNTIME_SRC_FILES} ${PROTO_HDRS} ${DEPLOYER_PROTO_HDRS})

target_compile_definitions(helper_ut_runtime PRIVATE
    google=ascend_private
    $<$<STREQUAL:${ENABLE_OPEN_SRC},True>:ONLY_COMPILE_OPEN_SRC>
)

target_compile_options(helper_ut_runtime PRIVATE
    -g --coverage -fprofile-arcs -ftest-coverage
    -Werror=format
)

target_link_libraries(helper_ut_runtime PRIVATE
    $<BUILD_INTERFACE:intf_pub>
    ascend_protobuf
    json
    c_sec
    runtime_stub
)

target_include_directories(helper_ut_runtime PRIVATE
    ${AIR_CODE_DIR}/runtime/device/remote
    ${AIR_CODE_DIR}/runtime/communication
    ${AIR_CODE_DIR}/runtime
    ${AIR_CODE_DIR}/runtime/heterogeneous
    ${AIR_CODE_DIR}/base
    ${AIR_CODE_DIR}/inc/framework
    ${AIR_CODE_DIR}/air/third_party/inc/runtime
    ${AIR_CODE_DIR}/inc/framework
    ${AIR_CODE_DIR}/runtime/deploy/deployer/device_mgr
    ${AIR_CODE_DIR}/tests/ut/ge/runtime/stub
)

# ut binary

# libge_mutiparts_utest
add_executable(ut_libge_multiparts_utest
    ${COMMON_TEST_FILES}
    ${MULTI_PARTS_TEST_FILES}
)

target_compile_options(ut_libge_multiparts_utest PRIVATE
    -g --coverage -fprofile-arcs -ftest-coverage
    -fsanitize=address -fsanitize=leak -fsanitize-recover=address
    -Werror=format
)

target_compile_definitions(ut_libge_multiparts_utest PRIVATE
    google=ascend_private
)

target_link_libraries(ut_libge_multiparts_utest
    $<BUILD_INTERFACE:intf_pub>
    -fsanitize=address -fsanitize=leak -fsanitize-recover=address
    -Wl,--whole-archive
    ge_davinci_model ge_build_common ge_prepare_common ge_single_op ge_pass_common ge_ut_common_format ge_ut_common gflags
    -Wl,--no-whole-archive
    ge_running_env gtest gtest_main gmock gmock_main ${COMMON_SHARED_LIBRARIES} -lrt -ldl -lgcov
)

set_target_properties(ut_libge_multiparts_utest PROPERTIES CXX_STANDARD 17)

# libge_common_utest
add_executable(ut_libge_common_utest
    ${COMMON_TEST_FILES}
    ${BASE_COMMON_TEST_FILES}
)

target_compile_options(ut_libge_common_utest PRIVATE
    -g --coverage -fprofile-arcs -ftest-coverage
    -Werror=format
)

target_compile_definitions(ut_libge_common_utest PRIVATE
    google=ascend_private
)

target_link_libraries(ut_libge_common_utest
    $<BUILD_INTERFACE:intf_pub>
    -Wl,--whole-archive
    ge_davinci_model ge_build_common ge_prepare_common ge_single_op ge_pass_common ge_ut_common_format ge_ut_common
    -Wl,--no-whole-archive
    ge_running_env gtest gtest_main gmock gmock_main ${COMMON_SHARED_LIBRARIES} -lrt -ldl -lgcov
)

set_target_properties(ut_libge_common_utest PROPERTIES CXX_STANDARD 17)

# libge_others_utest
add_executable(ut_libge_others_utest
    ${COMMON_TEST_FILES}
    ${PASS_TEST_FILES}
    ${EXECUTE_TEST_FILES}
    ${OTHERS_TEST_FILES}
    ${GE_OPT_INFO_TEST_FILES}
)

target_compile_options(ut_libge_others_utest PRIVATE
    -g --coverage -fprofile-arcs -ftest-coverage
    #-fsanitize=address -fsanitize=leak -fsanitize-recover=address
    -Werror=format
)

target_link_libraries(ut_libge_others_utest
    $<BUILD_INTERFACE:intf_pub>
    #-fsanitize=address -fsanitize=leak -fsanitize-recover=address
    -Wl,--whole-archive
    ge_davinci_model ge_build_common ge_prepare_common ge_pass_common ge_single_op ge_ut_common ge_ut_common_format
    -Wl,--no-whole-archive
    ge_running_env gtest gtest_main gmock gmock_main ${COMMON_SHARED_LIBRARIES} -lrt -ldl -lgcov
)

set_target_properties(ut_libge_others_utest PROPERTIES CXX_STANDARD 17)

# libge_label_maker_utest
add_executable(ut_libge_label_maker_utest
    ${COMMON_TEST_FILES}
    ${LABEL_MAKER_SRC_FILES}
    ${LABEL_MAKER_TEST_FILES}
)

target_compile_options(ut_libge_label_maker_utest PRIVATE
    -g --coverage -fprofile-arcs -ftest-coverage
    -Werror=format
)
# TODO:ge_ut_common need shrink; it has too many depends so
target_link_libraries(ut_libge_label_maker_utest
    $<BUILD_INTERFACE:intf_pub>
    -Wl,--whole-archive
    ge_davinci_model ge_build_common ge_prepare_common ge_pass_common ge_single_op ge_ut_common ge_ut_common_format
    -Wl,--no-whole-archive
    ge_running_env gtest gtest_main gmock gmock_main ${COMMON_SHARED_LIBRARIES} -lrt -ldl -lgcov
)

set_target_properties(ut_libge_label_maker_utest PROPERTIES CXX_STANDARD 17)

# libge_kernel_utest
add_executable(ut_libge_kernel_utest
        ${COMMON_TEST_FILES}
        ${KERNEL_TEST_FILES}
        ${FORMAT_TRANSFERS_TEST_FILES}
        ${KERNEL_SRC_FILES}
)

target_compile_options(ut_libge_kernel_utest PRIVATE
        -g --coverage -fprofile-arcs -ftest-coverage
        -Werror=format
)

target_link_libraries(ut_libge_kernel_utest
        $<BUILD_INTERFACE:intf_pub>
        -Wl,--whole-archive
        ge_davinci_model ge_build_common ge_prepare_common ge_pass_common ge_single_op ge_ut_common ge_ut_common_format
        -Wl,--no-whole-archive
        ge_running_env gtest gtest_main gmock gmock_main ${COMMON_SHARED_LIBRARIES} -lrt -ldl -lgcov
)

set_target_properties(ut_libge_kernel_utest PROPERTIES CXX_STANDARD 17)

# libge_distinct_load_utest
add_executable(ut_libge_distinct_load_utest
        ${COMMON_TEST_FILES}
        ${GENERATOR_TEST_FILES}
        ${DISTINCT_GRAPH_LOAD_TEST_FILES}
        ${HETEROGENEOUS_MODEL_TEST_FILES}
        ${SINGLE_OP_TEST_FILES}
        ${PROFILING_MNG_TEST_FILES}
        ${HYBRID_TEST_FILES}
)

target_compile_options(ut_libge_distinct_load_utest PRIVATE
    -g --coverage -fprofile-arcs -ftest-coverage
    -fsanitize=address -fsanitize=leak -fsanitize-recover=address
    -Werror=format
)

target_compile_definitions(ut_libge_distinct_load_utest PRIVATE
    google=ascend_private
    $<$<STREQUAL:${ENABLE_OPEN_SRC},True>:ONLY_COMPILE_OPEN_SRC>
)

target_include_directories(ut_libge_distinct_load_utest PRIVATE
    ${AIR_CODE_DIR}/base
    ${AIR_CODE_DIR}/runtime
    ${AIR_CODE_DIR}/runtime/device/remote
    ${AIR_CODE_DIR}/runtime/communication/cluster
    ${AIR_CODE_DIR}/runtime/communication/
)

target_link_libraries(ut_libge_distinct_load_utest
    $<BUILD_INTERFACE:intf_pub>
    -fsanitize=address -fsanitize=leak -fsanitize-recover=address
    -Wl,--whole-archive
    ge_single_op ge_davinci_model ge_build_common ge_prepare_common ge_pass_common ge_ut_common ge_ut_common_format
    -Wl,--no-whole-archive
    ge_running_env gtest gtest_main gmock gmock_main ${COMMON_SHARED_LIBRARIES} -lrt -ldl -lpthread -lgcov
)

set_target_properties(ut_libge_distinct_load_utest PROPERTIES CXX_STANDARD 17)

# helper ut
add_executable(ut_libge_helper_utest
    ${COMMON_TEST_FILES}
    ${HELPER_RUNTIME_TEST_FILES}
)

target_include_directories(ut_libge_helper_utest PRIVATE
        ${AIR_CODE_DIR}/runtime/heterogeneous/
        ${AIR_CODE_DIR}/runtime
        ${AIR_CODE_DIR}/third_party/inc/aicpu/
        ${AIR_CODE_DIR}/third_party/inc/runtime/
        ${AIR_CODE_DIR}/inc/framework
        ${AIR_CODE_DIR}/base
        ${AIR_CODE_DIR}/runtime/deploy/deployer/device_mgr
        )

target_compile_options(ut_libge_helper_utest PRIVATE
    -g --coverage -fprofile-arcs -ftest-coverage
    -Werror=format
)

target_compile_definitions(ut_libge_helper_utest PRIVATE
    google=ascend_private
    $<$<STREQUAL:${ENABLE_OPEN_SRC},True>:ONLY_COMPILE_OPEN_SRC>
)

target_link_libraries(ut_libge_helper_utest
    $<BUILD_INTERFACE:intf_pub>
    -Wl,--whole-archive
    ge_single_op ge_davinci_model ge_build_common ge_prepare_common ge_pass_common ge_ut_common ge_ut_common_format
    helper_ut_runtime
    -Wl,--no-whole-archive
    gtest gtest_main gmock gmock_main ${COMMON_SHARED_LIBRARIES} -lrt -ldl -lpthread -lgcov
)

set_target_properties(ut_libge_helper_utest PROPERTIES CXX_STANDARD 17)
