/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2021. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "hybrid/node_executor/ffts_plus/ffts_plus_sub_task.h"

#include "hybrid/node_executor/ffts_plus/ffts_plus_sub_task_factory.h"
#include "graph/load/model_manager/task_info/ffts_plus_proto_transfer.h"
#include "graph/utils/op_desc_utils.h"
#include "framework/common/op/ge_op_utils.h"
#include "framework/common/profiling_definitions.h"

namespace ge {
namespace {
constexpr uint8_t kTaskAutoAicCtxIndex = 0U;
constexpr uint8_t kTaskAutoTailAicCtxIndex = 1U;
constexpr uint8_t kTaskAutoAivCtxIndex = 2U;
constexpr uint8_t kTaskAutoTailAivCtxIndex = 3U;
constexpr uint8_t kTilingPos = 0U;
constexpr uint8_t kTilingTailPos = 1U;

const std::string kTaskCoreTypeAIC = "AIC";
const std::string kTaskCoreTypeAIV = "AIV";
const std::string kTaskCoreTypeMixAIC = "MIX_AIC";
const std::string kTaskCoreTypeMixAIV = "MIX_AIV";
const std::string kTaskCoreTypeAiCpu = "AICPU";
const std::string kTaskCoreTypeMixL2 = "MIX_L2";

const std::string kTaskCubeTBEKernelPrefixAic = "_mix_aic";
const std::string kTaskVectorTBEKernelPrefixAiv = "_mix_aiv";
const std::string kAttrOpParamSize = "op_para_size";

inline size_t MemSizeAlign(const size_t bytes) {
  constexpr uint32_t kAlignSize = 32U;
  return (((bytes + kAlignSize) - 1U) / kAlignSize) * kAlignSize;
}

inline void ResetSubTaskParam(AutoThreadParam &task_param) {
  task_param.thread_dim = 0U;
  task_param.input_output_num = 0U;
  task_param.task_addr_offset.clear();

  task_param.args_size = 0U;
  task_param.extinfo_size = 0U;
}

inline void ResetSubTaskFlush(AutoThreadSubTaskFlush &task_flush, const int32_t device_id) {
  task_flush.device_id = device_id;
  task_flush.args_base = nullptr;

  task_flush.aic_non_tail_task_start_pc = 0U;
  task_flush.aic_tail_task_start_pc = 0U;
  task_flush.aic_icache_prefetch_cnt = 0U;

  task_flush.aiv_non_tail_task_start_pc = 0U;
  task_flush.aiv_tail_task_start_pc = 0U;
  task_flush.aiv_icache_prefetch_cnt = 0U;

  task_flush.extinfo_base = nullptr;
}

// stub for tune
Status FFTSNodeThread(const ge::ComputeGraph &compute_graph, const ge::NodePtr &node) {
  (void)node;
  return compute_graph.GetAllNodes().empty() ? ge::FAILED : ge::SUCCESS;
}
}

namespace hybrid {
REGISTER_FFTS_PLUS_SUB_TASK_CREATOR(kTaskCoreTypeAIC, FftsPlusAicAivTask);
REGISTER_FFTS_PLUS_SUB_TASK_CREATOR(kTaskCoreTypeAIV, FftsPlusAicAivTask);
REGISTER_FFTS_PLUS_SUB_TASK_CREATOR(kTaskCoreTypeMixAIC, FftsPlusMixAicAivTask);
REGISTER_FFTS_PLUS_SUB_TASK_CREATOR(kTaskCoreTypeMixAIV, FftsPlusMixAicAivTask);
REGISTER_FFTS_PLUS_SUB_TASK_CREATOR(kTaskCoreTypeAiCpu, FftsPlusAiCpuTask);
REGISTER_FFTS_PLUS_SUB_TASK_CREATOR(kTaskCoreTypeMixL2, FftsPlusMixL2Task);

Status FftsPlusSubTask::Load(const HybridModel &model, const NodePtr &node) {
  GE_CHECK_NOTNULL(node);
  // Get FftsPlusUpdate instance by CORE_TYPE.
  std::string op_core_type;
  (void)AttrUtils::GetStr(node->GetOpDesc(), ATTR_NAME_CUBE_VECTOR_CORE_TYPE, op_core_type);
  ffts_plus_ctx_update_ = FftsPlusUpdateManager::Instance().GetUpdater(op_core_type);
  GE_CHECK_NOTNULL(ffts_plus_ctx_update_);

  // Get FftsPlusNodeTask of PartitionedCall.
  const auto &owner_graph = node->GetOwnerComputeGraph();
  GE_CHECK_NOTNULL(owner_graph);
  const auto &parent_node = owner_graph->GetParentNode();
  GE_CHECK_NOTNULL(parent_node);
  ffts_node_item_ = model.GetNodeItem(parent_node);
  GE_CHECK_NOTNULL(ffts_node_item_);

  GELOGD("[%s] Set FftsPlus Node success: %s.", node->GetName().c_str(), ffts_node_item_->NodeName().c_str());
  return SUCCESS;
}

/**
 * @brief Init FFTS Plus Node Task for Update.
 * @param context: instance of TaskContext
 * @return SUCCESS on success, error code otherwise
 */
Status FftsPlusSubTask::Init(TaskContext &context) {
  if (ffts_node_task_ != nullptr) {
    return SUCCESS;
  }

  GE_CHECK_NOTNULL(ffts_node_item_);
  GE_CHECK_NOTNULL(ffts_node_item_->kernel_task);
  ffts_node_task_ = dynamic_cast<FftsPlusNodeTask *>(ffts_node_item_->kernel_task.get());
  GE_CHECK_NOTNULL(ffts_node_task_);

  GELOGD("[%s] Get FftsPlusNodeTask success.", context.GetNodeName());
  return SUCCESS;
}

/**
 * @brief Update tiling data and ffts context.
 * @param context: instance of TaskContext
 * @return SUCCESS on success, error code otherwise
 */
Status FftsPlusSubTask::UpdateTilingData(TaskContext &context) {
  // Step1: Get FftsPlusNodeTask for first time (Task create not in topological order)
  GELOGD("[%s] Start to Update FFTS Plus context.", context.GetNodeName());
  GE_CHK_STATUS_RET_NOLOG(Init(context));
  GE_CHECK_NOTNULL(ffts_node_task_);

  // Step2: Thread slice for Node.
  PROFILING_START(context.GetNodeState()->GetProfilingIndex(), profiling::kFftsPlusNodeThread);
  const auto &node_item = context.GetNodeItem();
  const auto &subgraph = node_item.node->GetOwnerComputeGraph();
  GE_CHECK_NOTNULL(subgraph);
  const Status status = FFTSNodeThread(*subgraph, node_item.node);
  if (status != SUCCESS) {
    REPORT_INNER_ERROR("E19999", "[%s] FFTS Node-Thread failed: %u.", context.GetNodeName(), status);
    GELOGE(INTERNAL_ERROR, "[%s] FFTS Node-Thread failed: %u.", context.GetNodeName(), status);
    return INTERNAL_ERROR;
  }
  PROFILING_END(context.GetNodeState()->GetProfilingIndex(), profiling::kFftsPlusNodeThread);

  // Step3: Op Tiling for Node.
  PROFILING_START(context.GetNodeState()->GetProfilingIndex(), profiling::kInitThreadRunInfo);
  ResetSubTaskParam(task_param_);
  ResetSubTaskFlush(task_flush_, ffts_node_task_->device_id_);
  GE_CHK_STATUS_RET_NOLOG(InitThreadRunInfo(context));
  PROFILING_END(context.GetNodeState()->GetProfilingIndex(), profiling::kInitThreadRunInfo);

  // Step4: Init Thread slice parameters.
  PROFILING_START(context.GetNodeState()->GetProfilingIndex(), profiling::kInitThreadRunParam);
  GE_CHK_STATUS_RET_NOLOG(InitThreadRunParam(context));
  PROFILING_END(context.GetNodeState()->GetProfilingIndex(), profiling::kInitThreadRunParam);

  // Step5: Update task slice context.
  PROFILING_START(context.GetNodeState()->GetProfilingIndex(), profiling::kUpdateTaskAndCache);
  GE_CHK_STATUS_RET_NOLOG(UpdateSubTaskAndCache(context));
  PROFILING_END(context.GetNodeState()->GetProfilingIndex(), profiling::kUpdateTaskAndCache);

  GELOGD("[%s] Done Update FFTS Plus context successfully.", context.GetNodeName());
  return SUCCESS;
}

Status FftsPlusSubTask::UpdateSubTaskAndCache(const TaskContext &context) {
  const auto &node_item = context.GetNodeItem();
  const auto status = ffts_plus_ctx_update_->UpdateSubTaskAndCache(node_item.node, task_flush_,
                                                                   ffts_node_task_->ffts_plus_task_info_);
  if (status != SUCCESS) {
    REPORT_INNER_ERROR("E19999", "[%s] FFTS UpdateSubTaskAndCache failed: %u.", context.GetNodeName(), status);
    GELOGE(INTERNAL_ERROR, "[%s] FFTS UpdateSubTaskAndCache failed: %u.", context.GetNodeName(), status);
    return INTERNAL_ERROR;
  }

  return SUCCESS;
}

Status FftsPlusSubTask::ExecuteAsync(TaskContext &context, const std::function<void()> &done_callback) {
  (void)done_callback;
  GELOGD("[%s] Start to execute.", context.GetNodeName());
  if ((task_args_base_ != nullptr) && (!args_host_data_.empty())) {
    GELOGD("[%s]Copy, mem size: %zu, data size: %zu", context.GetNodeName(), task_args_size_, args_host_data_.size());
    GE_CHK_RT_RET(rtMemcpyAsync(task_args_base_, task_args_size_, args_host_data_.data(), task_args_size_,
                                RT_MEMCPY_HOST_TO_DEVICE_EX, context.GetStream()));
  } else {
    GELOGW("[%s]Empty, mem size: %zu, data size: %zu", context.GetNodeName(), task_args_size_, args_host_data_.size());
  }

  GELOGD("[%s] Done executing successfully.", context.GetNodeName());
  return SUCCESS;
}

Status FftsPlusAiCoreTask::InitThreadRunInfo(TaskContext &context) {
  PROFILING_START(context.GetNodeState()->GetProfilingIndex(), profiling::kOpFftsCalculateV2);
  const auto &node_item = context.GetNodeItem();
  const auto status = optiling::OpFftsCalculateV2(*node_item.node, task_flush_.op_run_info);
  if (status != SUCCESS) {
    REPORT_INNER_ERROR("E19999", "[%s] FFTS Calculate tiling failed: %u.", context.GetNodeName(), status);
    GELOGE(INTERNAL_ERROR, "[%s] FFTS Calculate tiling failed: %u.", context.GetNodeName(), status);
    return INTERNAL_ERROR;
  }
  PROFILING_END(context.GetNodeState()->GetProfilingIndex(), profiling::kOpFftsCalculateV2);

  // Step4: Get Start PC and prefetch count.
  PROFILING_START(context.GetNodeState()->GetProfilingIndex(), profiling::kUpdateAddrAndPrefCnt);
  GE_CHK_STATUS_RET_NOLOG(UpdateAddrAndPrefCnt(*node_item.op_desc));
  PROFILING_END(context.GetNodeState()->GetProfilingIndex(), profiling::kUpdateAddrAndPrefCnt);

  tiling_data_len_ = 0U;
  tiling_num_ = task_flush_.op_run_info.size() / 2U; // Non-Tail + Tail
  std::vector<int64_t> workspaces;
  for (auto &run_info : task_flush_.op_run_info) {
    const std::vector<int64_t> &workspace = run_info.GetAllWorkspaces();
    (void)workspaces.insert(workspaces.cend(), workspace.cbegin(), workspace.cend());
    tiling_data_len_ += MemSizeAlign(run_info.GetAllTilingData().tellp());
  }
  node_item.op_desc->SetWorkspaceBytes(workspaces);

  PROFILING_START(context.GetNodeState()->GetProfilingIndex(), profiling::kGetAutoThreadParam);
  GE_CHK_STATUS_RET(ffts_plus_ctx_update_->GetAutoThreadParam(node_item.node, task_flush_.op_run_info, task_param_),
                    "[%s] FFTS GetAutoThreadParam for AICORE failed", context.GetNodeName());
  if (task_param_.thread_dim == 0U) {
    REPORT_INNER_ERROR("E19999", "[%s] FFTS GetAutoThreadParam failed: thread dim is zero", context.GetNodeName());
    GELOGE(FAILED, "[%s] FFTS GetAutoThreadParam failed: thread dim is zero", context.GetNodeName());
    return FAILED;
  }

  const int32_t io_addr_size = node_item.num_inputs + node_item.num_outputs;
  const size_t thrd_addr_size = static_cast<size_t>(io_addr_size) + workspaces.size();
  if (thrd_addr_size != task_param_.task_addr_offset.size()) {
    REPORT_INNER_ERROR("E19999", "[%s] Invalid task addr size: %zu, inputs: %d, outputs: %d, workspaces: %zu",
                       context.GetNodeName(), task_param_.task_addr_offset.size(),
                       node_item.num_inputs, node_item.num_outputs, workspaces.size());
    GELOGE(INTERNAL_ERROR, "[%s] Invalid task addr size: %zu, inputs: %d, outputs: %d, workspaces: %zu",
           context.GetNodeName(), task_param_.task_addr_offset.size(), node_item.num_inputs, node_item.num_outputs,
           workspaces.size());
    return FAILED;
  }
  PROFILING_END(context.GetNodeState()->GetProfilingIndex(), profiling::kGetAutoThreadParam);

  // Step5: Prepare workspace and Tiling data.
  PROFILING_START(context.GetNodeState()->GetProfilingIndex(), profiling::kInitOpRunInfo);
  args_addr_base_ = tiling_data_len_ / sizeof(uintptr_t);
  const size_t task_addr_size = (thrd_addr_size + tiling_num_) * task_param_.thread_dim;
  args_host_data_.resize(args_addr_base_ + task_addr_size);

  GE_CHK_STATUS_RET_NOLOG(InitOpRunInfo());
  PROFILING_END(context.GetNodeState()->GetProfilingIndex(), profiling::kInitOpRunInfo);

  return SUCCESS;
}

Status FftsPlusAiCoreTask::InitThreadRunParam(TaskContext &context) {
  PROFILING_START(context.GetNodeState()->GetProfilingIndex(), profiling::kAllocateWorkspaces);
  // Step7: Allocate workspace and memory for tiling and args.
  GE_CHK_STATUS_RET_NOLOG(context.AllocateWorkspaces());

  // Addrs Num: Input / Output / Workspace / Tiling.
  const NodeItem &node_item = context.GetNodeItem();
  const std::vector<int64_t> &workspaces = node_item.op_desc->GetWorkspaceBytes();
  const int32_t io_addr_size = node_item.num_inputs + node_item.num_outputs;
  const size_t thrd_addr_size = static_cast<size_t>(io_addr_size) + workspaces.size();
  const size_t task_addr_size = (thrd_addr_size + tiling_num_) * task_param_.thread_dim;

  task_args_size_ = tiling_data_len_ + (task_addr_size * sizeof(uintptr_t));
  GE_CHK_STATUS_RET_NOLOG(context.AllocateWorkspace(task_args_size_, task_args_base_));
  task_flush_.args_base = ValueToPtr(PtrToValue(task_args_base_) + tiling_data_len_);
  GELOGD("[%s] Addr offset size: %zu, inputs: %d, outputs: %d, workspaces: %zu, tiling data len: %zu, args base: %p",
         context.GetNodeName(), task_param_.task_addr_offset.size(), node_item.num_inputs, node_item.num_outputs,
         workspaces.size(), tiling_data_len_, task_flush_.args_base);
  PROFILING_END(context.GetNodeState()->GetProfilingIndex(), profiling::kAllocateWorkspaces);

  // Step8: Arrange context args and Flush Task.
  PROFILING_START(context.GetNodeState()->GetProfilingIndex(), profiling::kInitTaskAddrs);
  GE_CHK_STATUS_RET_NOLOG(InitTaskAddrs(context));
  PROFILING_END(context.GetNodeState()->GetProfilingIndex(), profiling::kInitTaskAddrs);

  return SUCCESS;
}

/**
 * @brief Set workspace and copy Tiling data.
 * @return SUCCESS on success, error code otherwise
 */
Status FftsPlusAiCoreTask::InitOpRunInfo() {
  size_t tiling_data_pos = 0U;
  tiling_offset_.resize(task_flush_.op_run_info.size());
  for (size_t i = 0U; i < task_flush_.op_run_info.size(); ++i) {
    tiling_offset_[i] = tiling_data_pos;
    std::stringstream &tiling_stream = task_flush_.op_run_info[i].GetAllTilingData();
    const size_t cur_data_len = tiling_stream.tellp();
    if (cur_data_len == 0U) {
      continue;
    }

    const auto data_pos = tiling_data_pos / sizeof(uintptr_t);
    GELOGD("Tiling data size: %zu, Total size: %zu, host data size: %zu, copy offset: %zu",
           cur_data_len, tiling_data_len_, args_host_data_.size(), data_pos);
    auto *const cur_data_buf = PtrToPtr<uintptr_t, char_t>(&args_host_data_[data_pos]);
    (void)tiling_stream.rdbuf()->pubseekoff(0U, std::ios_base::beg); // rewind
    const auto rd_len = tiling_stream.rdbuf()->sgetn(cur_data_buf, cur_data_len);
    if (static_cast<size_t>(rd_len) != cur_data_len) {
      GELOGE(INTERNAL_ERROR, "Copy tiling data failed, data size=%zu, data pos: %zu, tiling size:%zu, rd_len: %ld.",
             args_host_data_.size(), data_pos, cur_data_len, rd_len);
      return INTERNAL_ERROR;
    }
    tiling_data_pos += MemSizeAlign(cur_data_len);
  }

  return SUCCESS;
}

void FftsPlusAiCoreTask::InitCtxIoAddrs(const size_t ctx_idx, const uintptr_t data_base) {
  const size_t thrd_addr_size = (args_host_data_.size() - args_addr_base_) / task_param_.thread_dim;
  for (uint32_t i = 0U; i < task_param_.thread_dim; ++i) {
    const size_t ctx_io_idx = (thrd_addr_size * i) + ctx_idx;
    GELOGD("addr base: 0x%lx, addr index: %zu, thread index: %u, thread addr offset: 0x%lx, args index: %zu",
           data_base, ctx_idx, i, task_param_.task_addr_offset[ctx_idx], ctx_io_idx);
    args_host_data_[args_addr_base_ + ctx_io_idx] = (data_base + (task_param_.task_addr_offset[ctx_idx] * i));
  }
}

Status FftsPlusAiCoreTask::InitTaskAddrs(const TaskContext &context) {
  size_t io_index = 0U;
  const auto &node_item = context.GetNodeItem();
  task_flush_.input_addr_base.resize(static_cast<size_t>(node_item.num_inputs));
  for (int32_t i = 0; i < node_item.num_inputs; ++i) {
    const TensorValue *const tensor = context.GetInput(i);
    GE_CHECK_NOTNULL(tensor);
    const uintptr_t data_base = PtrToValue(tensor->GetData());
    InitCtxIoAddrs(io_index + static_cast<uint32_t>(i), data_base);
    task_flush_.input_addr_base[i] = data_base;
  }
  io_index += static_cast<uint32_t>(node_item.num_inputs);

  task_flush_.output_addr_base.resize(static_cast<size_t>(node_item.num_outputs));
  for (int32_t i = 0; i < node_item.num_outputs; ++i) {
    const TensorValue *const tensor = context.GetOutput(i);
    GE_CHECK_NOTNULL(tensor);
    const uintptr_t data_base = PtrToValue(tensor->GetData());
    InitCtxIoAddrs(io_index + static_cast<uint32_t>(i), data_base);
    task_flush_.output_addr_base[i] = data_base;
  }
  io_index += static_cast<uint32_t>(node_item.num_outputs);

  const auto &workspaces = node_item.op_desc->GetWorkspaceBytes();
  for (size_t i = 0U; i < workspaces.size(); ++i) {
    const void *const workspace = context.MutableWorkspace(static_cast<int32_t>(i));
    GE_CHECK_NOTNULL(workspace);
    InitCtxIoAddrs(io_index + i, PtrToValue(workspace));
  }
  io_index += workspaces.size();

  InitOpTiling(io_index, PtrToValue(task_args_base_));

  return SUCCESS;
}

void FftsPlusAicAivTask::InitOpTiling(const size_t ctx_idx, const uintptr_t data_base) {
  const auto thread_dims = task_param_.thread_dim;
  const size_t thrd_addr_size = (args_host_data_.size() - args_addr_base_) / thread_dims;
  for (uint32_t i = 0U; i < thread_dims; ++i) {
    const auto data_pos = (i < (thread_dims - 1U)) ? tiling_offset_[kTilingPos] : tiling_offset_[kTilingTailPos];
    GELOGD("addr base: 0x%lx, addr index: %zu, thread index: %u, thread addr offset: 0x%lx, args index: %zu",
           data_base, ctx_idx, i, data_pos, (thrd_addr_size * i) + ctx_idx);
    args_host_data_[args_addr_base_ + (thrd_addr_size * i) + ctx_idx] = (data_base + data_pos);
  }
}

Status FftsPlusAicAivTask::UpdateAddrAndPrefCnt(const OpDesc &op_desc) {
  const auto &run_info = task_flush_.op_run_info;
  if (run_info.size() <= kTaskAutoTailAicCtxIndex) {
    REPORT_INNER_ERROR("E19999", "[%s] Run info invalid, size is: %zu", op_desc.GetName().c_str(), run_info.size());
    GELOGE(INTERNAL_ERROR, "[%s] Run info invalid, size is: %zu", op_desc.GetName().c_str(), run_info.size());
    return INTERNAL_ERROR;
  }

  uint32_t prefetch_cnt = 0U;
  Status status = ffts_node_task_->GetAddrAndPrefCnt(op_desc, run_info[kTaskAutoAicCtxIndex].GetTilingKey(),
                                                     task_flush_.aic_non_tail_task_start_pc, prefetch_cnt);
  GE_CHK_STATUS_RET(status, "[%s] Get AIC task start PC failed.", op_desc.GetName().c_str());
  if (run_info[kTaskAutoAicCtxIndex].GetTilingKey() != run_info[kTaskAutoTailAicCtxIndex].GetTilingKey()) {
    uint32_t tail_prefetch_cnt = 0U;
    status = ffts_node_task_->GetAddrAndPrefCnt(op_desc, run_info[kTaskAutoTailAicCtxIndex].GetTilingKey(),
                                                task_flush_.aic_tail_task_start_pc, tail_prefetch_cnt);
    GE_CHK_STATUS_RET(status, "[%s] Get tail AIC task start PC failed.", op_desc.GetName().c_str());
    task_flush_.aic_icache_prefetch_cnt = std::min(prefetch_cnt, tail_prefetch_cnt);
  } else {
    task_flush_.aic_tail_task_start_pc = task_flush_.aic_non_tail_task_start_pc;
    task_flush_.aic_icache_prefetch_cnt = prefetch_cnt;
  }

  return SUCCESS;
}

Status FftsPlusMixAicAivTask::UpdateAddrAndPrefCnt(const OpDesc &op_desc) {
  const auto &run_info = task_flush_.op_run_info;
  if (run_info.size() <= kTaskAutoTailAivCtxIndex) {
    REPORT_INNER_ERROR("E19999", "[%s] Tiling info invalid, size is: %zu", op_desc.GetName().c_str(), run_info.size());
    GELOGE(INTERNAL_ERROR, "[%s] Tiling info invalid, size is: %zu", op_desc.GetName().c_str(), run_info.size());
    return SUCCESS; // Test Feature: OpFftsCalculateV2 not Ready.
  }

  uint32_t aic_prefetch_cnt = 0U;
  Status status = ffts_node_task_->GetAddrAndPrefCnt(op_desc, run_info[kTaskAutoAicCtxIndex].GetTilingKey(),
                                                     task_flush_.aic_non_tail_task_start_pc, aic_prefetch_cnt);
  GE_CHK_STATUS_RET(status, "[%s] Get MIX AIC task start PC failed.", op_desc.GetName().c_str());
  if (run_info[kTaskAutoAicCtxIndex].GetTilingKey() != run_info[kTaskAutoTailAicCtxIndex].GetTilingKey()) {
    uint32_t tail_aic_prefetch_cnt = 0U;
    status = ffts_node_task_->GetAddrAndPrefCnt(op_desc, run_info[kTaskAutoTailAicCtxIndex].GetTilingKey(),
                                                task_flush_.aic_tail_task_start_pc, tail_aic_prefetch_cnt);
    GE_CHK_STATUS_RET(status, "[%s] Get Tail MIX AIC task start PC failed.", op_desc.GetName().c_str());
    task_flush_.aic_icache_prefetch_cnt = std::min(aic_prefetch_cnt, tail_aic_prefetch_cnt);
  } else {
    task_flush_.aic_tail_task_start_pc = task_flush_.aic_non_tail_task_start_pc;
    task_flush_.aic_icache_prefetch_cnt = aic_prefetch_cnt;
  }

  uint32_t aiv_prefetch_cnt = 0U;
  status = ffts_node_task_->GetAddrAndPrefCnt(op_desc, run_info[kTaskAutoAivCtxIndex].GetTilingKey(),
                                              task_flush_.aiv_non_tail_task_start_pc, aiv_prefetch_cnt);
  GE_CHK_STATUS_RET(status, "[%s] Get MIX AIV task start PC failed.", op_desc.GetName().c_str());
  if (run_info[kTaskAutoAivCtxIndex].GetTilingKey() != run_info[kTaskAutoTailAivCtxIndex].GetTilingKey()) {
    uint32_t tail_aiv_prefetch_cnt = 0U;
    status = ffts_node_task_->GetAddrAndPrefCnt(op_desc, run_info[kTaskAutoTailAivCtxIndex].GetTilingKey(),
                                                task_flush_.aiv_tail_task_start_pc, tail_aiv_prefetch_cnt);
    GE_CHK_STATUS_RET(status, "[%s] Get Tail MIX AIV task start PC failed.", op_desc.GetName().c_str());
    task_flush_.aiv_icache_prefetch_cnt = std::min(aiv_prefetch_cnt, tail_aiv_prefetch_cnt);
  } else {
    task_flush_.aiv_tail_task_start_pc = task_flush_.aiv_non_tail_task_start_pc;
    task_flush_.aiv_icache_prefetch_cnt = aiv_prefetch_cnt;
  }

  return SUCCESS;
}

void FftsPlusMixAicAivTask::InitOpTiling(const size_t ctx_idx, const uintptr_t data_base) {
  const auto thread_dims = task_param_.thread_dim;
  const size_t thrd_addr_size = (args_host_data_.size() - args_addr_base_) / task_param_.thread_dim;
  for (uint32_t i = 0U; i < thread_dims; ++i) {
    const auto data_pos = (i < (thread_dims - 1U)) ? tiling_offset_[kTilingPos] : tiling_offset_[kTilingTailPos];
    GELOGD("addr base is 0x%lx, addr index is %zu, thread index is %u, tiling addr offset is 0x%lx",
           data_base, ctx_idx, i, data_pos);
    args_host_data_[args_addr_base_ + (thrd_addr_size * i) + ctx_idx] = (data_base + data_pos);
  }
}

Status FftsPlusAiCpuTask::ExecuteAsync(TaskContext &context, const std::function<void()> &done_callback) {
  (void)context;
  (void)done_callback;
  return SUCCESS;
}

Status FftsPlusAiCpuTask::InitThreadRunInfo(TaskContext &context) {
  PROFILING_START(context.GetNodeState()->GetProfilingIndex(), profiling::kGetAutoThreadParam);
  const NodeItem &node_item = context.GetNodeItem();
  GE_CHK_STATUS_RET(ffts_plus_ctx_update_->GetAutoThreadParam(node_item.node, task_flush_.op_run_info, task_param_),
                    "[%s] FFTS GetAutoThreadParam for AICPU failed", context.GetNodeName());
  PROFILING_END(context.GetNodeState()->GetProfilingIndex(), profiling::kGetAutoThreadParam);
  return SUCCESS;
}

Status FftsPlusAiCpuTask::InitThreadRunParam(TaskContext &context) {
  GELOGD("[%s] args size:%u, extinfo size:%u", context.GetNodeName(), task_param_.args_size, task_param_.extinfo_size);
  size_t alloc_size = MemSizeAlign(static_cast<size_t>(task_param_.args_size));
  const size_t extinfo_pos = alloc_size;
  alloc_size += MemSizeAlign(static_cast<size_t>(task_param_.extinfo_size));
  if (alloc_size > 0U) {
    GE_CHK_STATUS_RET_NOLOG(context.AllocateWorkspace(alloc_size, task_flush_.args_base));
    task_flush_.extinfo_base = ValueToPtr(PtrToValue(task_flush_.args_base) + extinfo_pos);
  }

  const NodeItem &node_item = context.GetNodeItem();
  for (int32_t i = 0; i < node_item.num_inputs; ++i) {
    const TensorValue *const tensor = context.GetInput(i);
    GE_CHECK_NOTNULL(tensor);
    task_flush_.input_addr_base.emplace_back(PtrToValue(tensor->GetData()));
  }

  for (int32_t i = 0; i < node_item.num_outputs; ++i) {
    const TensorValue *const tensor = context.GetOutput(i);
    GE_CHECK_NOTNULL(tensor);
    task_flush_.output_addr_base.emplace_back(PtrToValue(tensor->GetData()));
  }

  return SUCCESS;
}

Status FftsPlusMixL2Task::Load(const HybridModel &model, const NodePtr &node) {
  const auto &op_desc = node->GetOpDesc();  // node is valid
  GE_CHECK_NOTNULL(op_desc);

  // Get FftsPlusUpdate instance by CORE_TYPE.
  std::string op_core_type;
  (void)AttrUtils::GetStr(op_desc, ATTR_NAME_CUBE_VECTOR_CORE_TYPE, op_core_type);
  ffts_plus_ctx_update_ = FftsPlusUpdateManager::Instance().GetUpdater(op_core_type);
  GE_CHECK_NOTNULL(ffts_plus_ctx_update_);

  GE_CHK_STATUS_RET_NOLOG(bin_kernel_handle_.Register(op_desc, kTaskCubeTBEKernelPrefixAic));
  GE_CHK_STATUS_RET_NOLOG(bin_kernel_handle_.Register(op_desc, kTaskVectorTBEKernelPrefixAiv));

  GELOGD("[%s] start to transfer taskdef", node->GetName().c_str());
  const auto *const task_defs = model.GetTaskDefs(node);
  if ((task_defs == nullptr) || (task_defs->size() != 1U)) {
    REPORT_INNER_ERROR("E19999", "[%s] has no taskdef", op_desc->GetName().c_str());
    GELOGE(INTERNAL_ERROR, "[%s] has no taskdef", op_desc->GetName().c_str());
    return INTERNAL_ERROR;
  }

  const domi::TaskDef &task_def = task_defs->at(0U);
  const domi::FftsPlusTaskDef &ffts_plus_task_def = task_def.ffts_plus_task();

  const RuntimeParam runtime_param;
  FftsPlusProtoTransfer ffts_proto_transfer(0U, io_addrs_from_taskdef_, runtime_param, ext_args_, mode_addr_idx_);
  const auto find_node_handle = [op_desc](const uint32_t index_object) -> OpDescPtr {
    (void)index_object;
    return op_desc;
  };
  ffts_proto_transfer.SetFindNodeHandle(find_node_handle);
  const auto ffts_addr_pref_handle = [this](const std::string &kernel_name, void *&addr, uint32_t &pref_cnt) {
    return bin_kernel_handle_.GetAddrAndPrefCnt(kernel_name, addr, pref_cnt);
  };
  // Figure out need tiling or not
  int64_t max_size = -1;
  if (AttrUtils::GetInt(op_desc, kAttrOpParamSize, max_size)) {
    GELOGD("Got op %s param size by key: %s, value = %ld", op_desc->GetName().c_str(), kAttrOpParamSize.c_str(),
           max_size);
    if (max_size < 0) {
      GELOGE(PARAM_INVALID, "[Check][Size][%s(%s)] Invalid op_param_size: %ld.", op_desc->GetName().c_str(),
             op_desc->GetType().c_str(), max_size);
      REPORT_INNER_ERROR("E19999", "[%s(%s)] Invalid op_param_size: %ld.", op_desc->GetName().c_str(),
                         op_desc->GetType().c_str(), max_size);
      return PARAM_INVALID;
    }
    need_tiling_ = max_size > 0;
  }
  if (!need_tiling_) {
    ffts_proto_transfer.SetAddrPrefHandle(ffts_addr_pref_handle);
  }
  GE_CHK_STATUS_RET_NOLOG(ffts_proto_transfer.Transfer(op_desc, ffts_plus_task_def, ffts_plus_task_info_));
  GELOGD("[%s] Done initialization successfully.", node->GetName().c_str());
  return SUCCESS;
}

Status FftsPlusMixL2Task::UpdateTilingData(TaskContext &context) {
  const auto &node_name = context.GetNodeName();
  GELOGD("[%s] start to update context", node_name);
  const auto &node_item = context.GetNodeItem();
  // Op Tiling for Node.
  ResetSubTaskFlush(task_flush_, 0);
  const auto op = context.GetNodeState()->GetOperator(context.GetExecutionContext()->stage_id);
  GE_CHECK_NOTNULL(op);
  if (need_tiling_) {
    tiling_num_ = 1U;  // if need tiling, only cube need
    task_flush_.op_run_info.resize(tiling_num_);
    PROFILING_START(context.GetNodeState()->GetProfilingIndex(), profiling::kTiling);
    GE_CHK_STATUS_RET(GetTilingRunInfo(*op, task_flush_.op_run_info.front()), "[%s] calculate tiling failed",
                      node_name);
    PROFILING_END(context.GetNodeState()->GetProfilingIndex(), profiling::kTiling);

    // Get Start PC and prefetch count.
    PROFILING_START(context.GetNodeState()->GetProfilingIndex(), profiling::kUpdateAddrAndPrefCnt);
    GE_CHK_STATUS_RET(UpdateAddrAndPrefCnt(*node_item.op_desc), "[%s] update pc and prefetch count failed", node_name);
    PROFILING_END(context.GetNodeState()->GetProfilingIndex(), profiling::kUpdateAddrAndPrefCnt);

    tiling_data_len_ = 0U;
    std::vector<int64_t> workspaces;
    for (const auto &run_info : task_flush_.op_run_info) {
      const std::vector<int64_t> &workspace = run_info.GetAllWorkspaces();
      (void)workspaces.insert(workspaces.cend(), workspace.cbegin(), workspace.cend());
      tiling_data_len_ += MemSizeAlign(run_info.GetAllTilingData().str().size());
    }
    node_item.op_desc->SetWorkspaceBytes(workspaces);
  }

  // Prepare workspace and Tiling data.
  PROFILING_START(context.GetNodeState()->GetProfilingIndex(), profiling::kInitOpRunInfo);
  // Addrs Num: Mode / Input / Output / Workspace / Tiling.
  const size_t mode_addr_size = mode_addr_idx_.size();
  const int32_t io_addr_size = node_item.num_inputs + node_item.num_outputs;
  const size_t work_spaces_size = node_item.op_desc->GetWorkspaceBytes().size();
  const size_t addr_size = static_cast<size_t>(io_addr_size) + work_spaces_size;
  const size_t task_addr_size = mode_addr_size + addr_size + tiling_num_;
  args_addr_base_ = tiling_data_len_ / sizeof(uintptr_t);
  args_host_data_.resize(args_addr_base_ + task_addr_size);

  GE_CHK_STATUS_RET(InitOpRunInfo(), "[%s] handle tiling result failed", node_name);
  PROFILING_END(context.GetNodeState()->GetProfilingIndex(), profiling::kInitOpRunInfo);

  // Allocate workspace and memory for tiling and args.
  PROFILING_START(context.GetNodeState()->GetProfilingIndex(), profiling::kAllocateWorkspaces);
  GE_CHK_STATUS_RET(context.AllocateOutputs(), "[%s] malloc failed", node_name);
  GE_CHK_STATUS_RET(context.AllocateWorkspaces(), "[%s] malloc failed", node_name);

  task_args_size_ = tiling_data_len_ + (sizeof(uintptr_t) * task_addr_size);
  GE_CHK_STATUS_RET(context.AllocateWorkspace(task_args_size_, task_args_base_), "[%s] malloc failed", node_name);
  task_flush_.args_base = ValueToPtr(PtrToValue(task_args_base_) + tiling_data_len_);
  GELOGD("[%s] mode_addr_size: %zu, inputs: %d, outputs: %d, workspaces: %zu, tiling data len: %zu, args base: %p",
         node_name, mode_addr_size, node_item.num_inputs, node_item.num_outputs, work_spaces_size, tiling_data_len_,
         task_flush_.args_base);
  PROFILING_END(context.GetNodeState()->GetProfilingIndex(), profiling::kAllocateWorkspaces);
  // Arrange context args and Flush Task.
  PROFILING_START(context.GetNodeState()->GetProfilingIndex(), profiling::kInitTaskAddrs);
  GE_CHK_STATUS_RET(InitTaskAddrs(context), "[%s] init task addr failed", node_name);
  PROFILING_END(context.GetNodeState()->GetProfilingIndex(), profiling::kInitTaskAddrs);
  // Update rts ffts_plus_task_info_ info
  PROFILING_START(context.GetNodeState()->GetProfilingIndex(), profiling::kUpdateTaskAndCache);
  GE_CHK_STATUS_RET(ffts_plus_ctx_update_->UpdateSubTaskAndCache(node_item.node, task_flush_, ffts_plus_task_info_),
                    "[%s] handle tiling result failed", node_name);
  PROFILING_END(context.GetNodeState()->GetProfilingIndex(), profiling::kUpdateTaskAndCache);
  GELOGD("[%s] Done Update FFTS Plus MixL2 context successfully.", context.GetNodeName());
  return SUCCESS;
}

Status FftsPlusMixL2Task::UpdateAddrAndPrefCnt(const OpDesc &op_desc) {
  const auto &run_info = task_flush_.op_run_info;
  if (run_info.empty()) {
    REPORT_INNER_ERROR("E19999", "[%s] Run info invalid, size is: %zu", op_desc.GetName().c_str(), run_info.size());
    GELOGE(INTERNAL_ERROR, "[%s] Run info invalid, size is: %zu", op_desc.GetName().c_str(), run_info.size());
    return INTERNAL_ERROR;
  }

  // One tiling key for both aic and aiv
  GE_CHK_STATUS_RET(
      bin_kernel_handle_.GetAddrAndPrefCnt(op_desc, run_info[kTaskAutoAicCtxIndex].GetTilingKey(),
                                           task_flush_.aic_non_tail_task_start_pc,
                                           task_flush_.aic_icache_prefetch_cnt, kTaskCubeTBEKernelPrefixAic),
      "Get cube pc and prefetch count failed for %s", op_desc.GetName().c_str());
  GE_CHK_STATUS_RET(
      bin_kernel_handle_.GetAddrAndPrefCnt(op_desc, run_info[kTaskAutoAicCtxIndex].GetTilingKey(),
                                           task_flush_.aiv_non_tail_task_start_pc,
                                           task_flush_.aiv_icache_prefetch_cnt, kTaskVectorTBEKernelPrefixAiv),
      "Get vector pc and prefetch count failed for %s", op_desc.GetName().c_str());
  return SUCCESS;
}

Status FftsPlusMixL2Task::GetTilingRunInfo(const ge::Operator &op, optiling::OpRunInfoV2 &op_run_info) {
  return optiling::OpParaCalculateV2(op, op_run_info);
}

Status FftsPlusMixL2Task::ExecuteAsync(TaskContext &context, const function<void()> &done_callback) {
  const auto &node_item = context.GetNodeItem();
  const auto &op_desc = node_item.node->GetOpDesc();
  GE_CHECK_NOTNULL(op_desc);
  GELOGI("[%s] FFTS Plus MixL2 launch start.", op_desc->GetName().c_str());
  if ((task_args_base_ != nullptr) && (!args_host_data_.empty())) {
    GELOGD("[%s] start copy: mem size: %zu, data size: %zu", op_desc->GetName().c_str(), task_args_size_,
           args_host_data_.size());
    GE_CHK_RT_RET(rtMemcpyAsync(task_args_base_, task_args_size_,
                                args_host_data_.data(), args_host_data_.size() * sizeof(uintptr_t),
                                RT_MEMCPY_HOST_TO_DEVICE_EX, context.GetStream()));
  } else {
    GELOGW("[%s] no need copy mem size: %zu, data size: %zu", op_desc->GetName().c_str(), task_args_size_,
           args_host_data_.size());
  }
  const rtError_t rt_ret = rtFftsPlusTaskLaunch(&ffts_plus_task_info_, context.GetStream());
  if (rt_ret != RT_ERROR_NONE) {
    REPORT_INNER_ERROR("E19999", "[%s] Call rtFftsPlusTaskLaunch failed: 0x%X", context.GetNodeName(), rt_ret);
    GELOGE(RT_FAILED, "[Check][RT][%s] Call rtFftsPlusTaskLaunch failed: 0x%X", context.GetNodeName(), rt_ret);
    return RT_ERROR_TO_GE_STATUS(rt_ret);
  }

  const auto callback_func = [op_desc, done_callback]() {
    GELOGD("[%s] On FFTS Plus MixL2 callback", op_desc->GetName().c_str());
    if (done_callback != nullptr) {
      done_callback();
    }
    GELOGD("[%s] DoneFFTS Plus MixL2 callback.", op_desc->GetName().c_str());
    return SUCCESS;
  };

  GE_CHK_STATUS_RET(context.RegisterCallback(callback_func), "[Register][Callback] failed for [%s]",
                    context.GetNodeName());
  GELOGD("[%s] Done executing FFTS Plus MixL2 successfully.", context.GetNodeName());
  return SUCCESS;
}

Status FftsPlusMixL2Task::InitTaskAddrs(const TaskContext &context) {
  size_t io_index = 0U;
  for (const auto &addr_idx : mode_addr_idx_) {
    if (addr_idx >= io_addrs_from_taskdef_.size()) {
      GELOGE(FAILED, "Invalid index %zu, which should be less than %zu.", addr_idx, io_addrs_from_taskdef_.size());
      return FAILED;
    }
    InitMixL2Addrs(io_index, io_addrs_from_taskdef_[addr_idx]);
    io_index++;
  }
  const auto &node_item = context.GetNodeItem();
  for (int32_t i = 0; i < node_item.num_inputs; ++i) {
    const TensorValue *const tensor = context.GetInput(i);
    GE_CHECK_NOTNULL(tensor);
    const uintptr_t data_base = PtrToValue(tensor->GetData());
    (void)task_flush_.input_addr_base.emplace_back(data_base);
    InitMixL2Addrs(io_index + static_cast<uint32_t>(i), data_base);
  }
  io_index += static_cast<uint32_t>(node_item.num_inputs);

  for (int32_t i = 0; i < node_item.num_outputs; ++i) {
    const TensorValue *const tensor = context.GetOutput(i);
    GE_CHECK_NOTNULL(tensor);
    const uintptr_t data_base = PtrToValue(tensor->GetData());
    (void)task_flush_.output_addr_base.emplace_back(data_base);
    InitMixL2Addrs(io_index + static_cast<uint32_t>(i), data_base);
  }
  io_index += static_cast<uint32_t>(node_item.num_outputs);

  const auto &workspaces = node_item.op_desc->GetWorkspaceBytes();
  for (size_t i = 0U; i < workspaces.size(); ++i) {
    const void *const workspace = context.MutableWorkspace(static_cast<int32_t>(i));
    GE_CHECK_NOTNULL(workspace);
    InitMixL2Addrs(io_index + i, PtrToValue(workspace));
  }
  io_index += workspaces.size();
  // tiling addr
  if (need_tiling_) {
    InitMixL2Addrs(io_index, PtrToValue(task_args_base_));
  }

  return SUCCESS;
}

void FftsPlusMixL2Task::InitMixL2Addrs(const size_t io_index, const uintptr_t data_base) {
  GELOGD("addr base: 0x%lx, args index: %zu", data_base, io_index);
  args_host_data_[args_addr_base_ + io_index] = data_base; // io_index is valid
}

FftsPlusMixL2Task::~FftsPlusMixL2Task() {
  for (auto &addr : ext_args_) {
    GE_FREE_RT_LOG(addr);
  }
  bin_kernel_handle_.CleanTbeHandle();
  CleanRtFftsPlusTask(ffts_plus_task_info_);
}
} // namespace hybrid
} // namespace ge