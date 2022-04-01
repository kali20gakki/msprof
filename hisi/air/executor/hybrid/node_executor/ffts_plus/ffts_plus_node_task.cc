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

#include "hybrid/node_executor/ffts_plus/ffts_plus_node_task.h"
#include "graph/load/model_manager/task_info/ffts_plus_proto_transfer.h"
#include "graph/load/model_manager/model_manager.h"
#include "framework/common/profiling_definitions.h"

namespace ge {
namespace {
// Stub for tune
Status FFTSGraphPreThread(const ComputeGraph &compute_graph) {
  return compute_graph.GetAllNodes().empty() ? ge::FAILED : ge::SUCCESS;
}
}

namespace hybrid {
FftsPlusNodeTask::~FftsPlusNodeTask() {
  for (auto &addr : ext_args_) {
    GE_FREE_RT_LOG(addr);
  }

  bin_kernel_handle_.CleanTbeHandle();
  CleanRtFftsPlusTask(ffts_plus_task_info_);
}

Status FftsPlusNodeTask::Init(TaskContext &context) {
  subgraph_executor_ = MakeUnique<FftsPlusSubgraphExecutor>(graph_item_, context.GetExecutionContext());
  GE_CHECK_NOTNULL(subgraph_executor_);
  GE_CHK_STATUS_RET(subgraph_executor_->Init(), "[Init][FftsSubgraphExecutor]Failed.");
  return SUCCESS;
}

Status FftsPlusNodeTask::ExecuteAsync(TaskContext &context, const std::function<void()> &done_callback) {
  GELOGD("[%s] Start call thread slice.", context.GetNodeName());
  PROFILING_START(context.GetNodeState()->GetProfilingIndex(), profiling::kFftsPlusPreThread);
  const auto status = FFTSGraphPreThread(*subgraph_);
  if (status != SUCCESS) {
    REPORT_INNER_ERROR("E19999", "[%s] FFTS Pre-Thread failed: %u.", context.GetNodeName(), status);
    GELOGE(INTERNAL_ERROR, "[Check][tune][%s] FFTS Pre-Thread failed: %u.", context.GetNodeName(), status);
    return INTERNAL_ERROR;
  }
  PROFILING_END(context.GetNodeState()->GetProfilingIndex(), profiling::kFftsPlusPreThread);

  PROFILING_START(context.GetNodeState()->GetProfilingIndex(), profiling::kFftsPlusGraphSchedule);
  GELOGD("[%s] Start to execute.", context.GetNodeName());
  GE_CHK_STATUS_RET(subgraph_executor_->ExecuteAsync(context),  // For InferShape / Alloc and Update.
                    "[Invoke][ExecuteAsync] failed for[%s]", context.GetNodeName());
  PROFILING_END(context.GetNodeState()->GetProfilingIndex(), profiling::kFftsPlusGraphSchedule);

  PROFILING_START(context.GetNodeState()->GetProfilingIndex(), profiling::kFftsPlusTaskLaunch);
  GELOGI("[%s] FFTS Graph task launch start.", graph_item_->GetName().c_str());
  const rtError_t rt_ret = rtFftsPlusTaskLaunch(&ffts_plus_task_info_, context.GetStream());
  if (rt_ret != RT_ERROR_NONE) {
    REPORT_INNER_ERROR("E19999", "[%s] Call rtFftsPlusTaskLaunch failed: 0x%X", context.GetNodeName(), rt_ret);
    GELOGE(RT_FAILED, "[Check][RT][%s] Call rtFftsPlusTaskLaunch failed: 0x%X", context.GetNodeName(), rt_ret);
    return RT_ERROR_TO_GE_STATUS(rt_ret);
  }
  PROFILING_END(context.GetNodeState()->GetProfilingIndex(), profiling::kFftsPlusTaskLaunch);

  const auto callback_func = [this, done_callback]() {
    Callback(done_callback);
  };

  GE_CHK_STATUS_RET(context.RegisterCallback(callback_func), "[Register][Callback] failed[%s]", context.GetNodeName());
  GELOGD("[%s] Done executing FFTS Graph successfully.", context.GetNodeName());
  return SUCCESS;
}

Status FftsPlusNodeTask::Callback(const std::function<void()> &done_callback) {
  GELOGD("[%s] On FFTS Graph callback", graph_item_->GetName().c_str());
  if (done_callback != nullptr) {
    done_callback();
  }

  GELOGD("[%s] To release sub graph tensors.", graph_item_->GetName().c_str());
  subgraph_executor_.reset();
  GELOGD("[%s] Done releasing sub graph tensors.", graph_item_->GetName().c_str());
  return SUCCESS;
}

/**
 * @ingroup ge
 * @brief Load dynamic shape model for FFTS Plus Graph.
 * @return SUCCESS / other error code.
 */
Status FftsPlusNodeTask::Load(const HybridModel &model, const NodePtr &node, const ComputeGraphPtr &subgraph) {
  GE_CHECK_NOTNULL(node);
  GE_CHECK_NOTNULL(subgraph);
  GELOGD("[%s] Load dynamic shape Model for FFTS Plus.", node->GetName().c_str());
  // node ---> PartitionedCall
  //               |
  // node --->     |---- PartitionedCall          <--- HasAttr(ATTR_NAME_FFTS_PLUS_SUB_GRAPH)
  //                         |
  // subgraph -->            |------{ Node --> Node --> Node --> Node }      <--- HasAttr(ATTR_NAME_THREAD_SCOPE_ID)
  const auto &op_desc = node->GetOpDesc();
  GE_CHECK_NOTNULL(op_desc);
  subgraph_ = subgraph;
  const auto *const task_defs = model.GetTaskDefs(node);
  if ((task_defs == nullptr) || (task_defs->size() != 1U)) {
    REPORT_INNER_ERROR("E19999", "[%s] No TaskDef found for FFTS Plus PartitionedCall", graph_item_->GetName().c_str());
    GELOGE(INTERNAL_ERROR, "[%s] No TaskDef found for FFTS Plus PartitionedCall", graph_item_->GetName().c_str());
    return INTERNAL_ERROR;
  }
  const domi::TaskDef &task_def = task_defs->at(0U);
  const domi::FftsPlusTaskDef &ffts_plus_task_def = task_def.ffts_plus_task();

  std::map<uint32_t, OpDescPtr> op_by_index;
  for (const auto &sub_node : subgraph_->GetDirectNode()) {
    const auto &sub_desc = sub_node->GetOpDesc();
    GELOGD("[%s] Init Sgt node[%s] Id:%ld", node->GetName().c_str(), sub_desc->GetName().c_str(), sub_desc->GetId());
    op_by_index[static_cast<uint32_t>(sub_desc->GetId())] = sub_desc;
    if (IsTbeTask(sub_desc)) {
      GE_CHK_STATUS_RET_NOLOG(bin_kernel_handle_.Register(sub_desc));
    }
  }

  const auto find_node_handle = [&op_by_index](const uint32_t op_index) -> OpDescPtr {
    const std::map<uint32_t, OpDescPtr>::const_iterator it = op_by_index.find(op_index);
    return (it == op_by_index.cend()) ? nullptr : it->second;
  };

  const auto ffts_save_aicpu_ctx = [this](const OpDescPtr &op_desc, const domi::aicpuKernelDef &kernel_def) {
    return this->SetSaveAicpuCtxHandle(op_desc, kernel_def);
  };

  std::vector<uintptr_t> io_addrs;
  std::set<size_t> mode_addr_idx;
  const RuntimeParam runtime_param;
  FftsPlusProtoTransfer ffts_proto_transfer(0U, io_addrs, runtime_param, ext_args_, mode_addr_idx);
  ffts_proto_transfer.SetFindNodeHandle(find_node_handle);
  ffts_proto_transfer.SetSaveAicpuCtxHandle(ffts_save_aicpu_ctx);
  GE_CHK_STATUS_RET_NOLOG(ffts_proto_transfer.Transfer(op_desc, ffts_plus_task_def, ffts_plus_task_info_));

  GE_CHK_RT_RET(rtGetDevice(&device_id_));
  GELOGD("[%s] Done initialization successfully.", node->GetName().c_str());
  return SUCCESS;
}

Status FftsPlusNodeTask::SetSaveAicpuCtxHandle(const OpDescPtr &op_desc, const domi::aicpuKernelDef &kernel_def) {
  GE_CHECK_NOTNULL(op_desc);
  aicpu_arg_data_[op_desc->GetId()] = kernel_def.args();
  aicpu_ext_info_[op_desc->GetId()] = kernel_def.kernel_ext_info();
  return SUCCESS;
}
} // namespace hybrid
} // namespace ge
