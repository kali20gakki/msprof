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

#include "hybrid/node_executor/ffts_plus/ffts_plus_subgraph_executor.h"

#include "hybrid/node_executor/node_executor.h"
#include "framework/common/profiling_definitions.h"
#include "graph/ge_context.h"

namespace ge {
namespace hybrid {
Status FftsPlusSubgraphExecutor::PrepareNode(const NodeItem &node_item) {
  GELOGD("[%s] Start to prepare node [%s].", graph_item_->GetName().c_str(), node_item.NodeName().c_str());
  const auto p_node_state = subgraph_context_->GetNodeState(&node_item);
  GE_CHECK_NOTNULL(p_node_state);
  if (p_node_state->MaySkipSchedule()) {
    return SUCCESS;
  }
  if (node_item.node_type == NETOUTPUT) {
    return NodeEnqueue(p_node_state);
  }

  if (node_item.kernel_task == nullptr) {
    GELOGE(INTERNAL_ERROR, "[Get][KernelTask] failed for %s, NodeTask not set.", node_item.NodeName().c_str());
    REPORT_CALL_ERROR("E19999", "GetKernelTask failed for %s, NodeTask not set.", node_item.NodeName().c_str());
    return INTERNAL_ERROR;
  }
  p_node_state->SetKernelTask(node_item.kernel_task);

  // only do shape inference and compilation for nodes with dynamic shapes.
  if (node_item.is_dynamic) {
    PROFILING_START(p_node_state->GetProfilingIndex(), profiling::kFftsPlusInferShape);
    GE_CHK_STATUS_RET_NOLOG(shape_inference_engine_->InferShape(*p_node_state));
    PROFILING_END(p_node_state->GetProfilingIndex(), profiling::kFftsPlusInferShape);
  }

  PROFILING_START(p_node_state->GetProfilingIndex(), profiling::kAllocateOutputs);
  const auto shared_task_context = p_node_state->GetTaskContext();
  GE_CHECK_NOTNULL(shared_task_context);
  GE_CHK_STATUS_RET_NOLOG(shared_task_context->AllocateOutputs());
  GE_CHK_STATUS_RET_NOLOG(shared_task_context->PropagateOutputs());
  PROFILING_END(p_node_state->GetProfilingIndex(), profiling::kAllocateOutputs);

  PROFILING_START(p_node_state->GetProfilingIndex(), profiling::kCommitTilingTask);
  const auto prepare_func = [this, p_node_state](const error_message::Context &error_context) {
    ErrorManager::GetInstance().SetErrorContext(error_context);
    GetContext().SetSessionId(context_->session_id);
    GetContext().SetContextId(context_->context_id);

    GE_CHK_STATUS_RET_NOLOG(PrepareForExecution(context_, *p_node_state));
    return SUCCESS;
  };
  auto prepare_future = pre_run_pool_->commit(prepare_func, ErrorManager::GetInstance().GetErrorManagerContext());
  p_node_state->SetPrepareFuture(std::move(prepare_future));
  PROFILING_END(p_node_state->GetProfilingIndex(), profiling::kCommitTilingTask);

  return NodeEnqueue(p_node_state);
}

Status FftsPlusSubgraphExecutor::InitCallback(NodeState *const node_state, std::function<void()> &callback,
                                              const std::shared_ptr<ScopeGuard> tensor_guard) {
  (void)node_state;
  (void)tensor_guard;
  callback = nullptr;
  return SUCCESS;
}
}  // namespace hybrid
}  // namespace ge
