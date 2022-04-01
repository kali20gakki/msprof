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

#include "hybrid/node_executor/partitioned_call/partitioned_call_node_executor.h"
#include <memory>
#include "hybrid/node_executor/ffts_plus/ffts_plus_node_task.h"

namespace ge {
namespace hybrid {
REGISTER_NODE_EXECUTOR_BUILDER(NodeExecutorManager::ExecutorType::DYNAMIC_SUBGRAPH, PartitionedCallNodeExecutor);

PartitionedCallNodeTask::PartitionedCallNodeTask(const GraphItem * const graph_item)
    : NodeTask(), graph_item_(graph_item) {
}

PartitionedCallNodeTask::~PartitionedCallNodeTask() {
  GELOGD("[%s] PartitionedCallNodeTask destroyed.", graph_item_->GetName().c_str());
}

Status PartitionedCallNodeTask::Init(TaskContext &context) {
  const auto execution_context = context.GetExecutionContext();
  subgraph_executor_.reset();
  subgraph_executor_ = MakeUnique<SubgraphExecutor>(graph_item_, execution_context);
  GE_CHECK_NOTNULL(subgraph_executor_);
  GE_CHK_STATUS_RET(subgraph_executor_->Init(), "[Init][SubgraphExecutor]Failed.");
  return SUCCESS;
}

Status PartitionedCallNodeTask::ExecuteAsync(TaskContext &context, const std::function<void()> &done_callback) {
  GE_CHK_STATUS_RET(subgraph_executor_->ExecuteAsync(context),
                    "[Invoke][ExecuteAsync] failed for[%s]", graph_item_->GetName().c_str());

  const auto callback = [this, done_callback]() {
    Callback(done_callback);
  };

  GE_CHK_STATUS_RET(context.RegisterCallback(callback),
                    "[Register][Callback] failed for [%s]", graph_item_->GetName().c_str());
  GELOGD("[%s] Done executing subgraph successfully.", graph_item_->GetName().c_str());
  return SUCCESS;
}

Status PartitionedCallNodeTask::Callback(const std::function<void()> &done_callback) {
  GELOGD("[%s] On subgraph callback", graph_item_->GetName().c_str());
  if (done_callback != nullptr) {
    done_callback();
  }

  GELOGD("[%s] To release sub graph tensors.", graph_item_->GetName().c_str());
  subgraph_executor_.reset();
  GELOGD("[%s] Done releasing sub graph tensors.", graph_item_->GetName().c_str());
  return SUCCESS;
}

Status PartitionedCallNodeTask::UpdateArgs(TaskContext &context) {
  (void)context;
  return SUCCESS;
}

Status PartitionedCallNodeExecutor::LoadTask(const ge::hybrid::HybridModel &model,
                                             const ge::NodePtr &node,
                                             std::shared_ptr<NodeTask> &task) const {
  GELOGD("Load dynamic partitioned call: [%s]", node->GetName().c_str());
  const auto &subgraph = NodeUtils::GetSubgraph(*node, 0U);
  GE_CHECK_NOTNULL(subgraph);
  const auto partitioned_call = model.GetSubgraphItem(subgraph);
  GE_CHECK_NOTNULL(partitioned_call);

  if (!node->GetOpDesc()->HasAttr(ATTR_NAME_FFTS_PLUS_SUB_GRAPH)) {
    task.reset();
    task = MakeShared<PartitionedCallNodeTask>(partitioned_call);
    GE_CHECK_NOTNULL(task);
  } else {
    const std::shared_ptr<FftsPlusNodeTask> ffts_plus_task = MakeShared<FftsPlusNodeTask>(partitioned_call);
    GE_CHECK_NOTNULL(ffts_plus_task);
    GE_CHK_STATUS_RET(ffts_plus_task->Load(model, node, subgraph), "[%s] Load task failed.", node->GetName().c_str());
    task = ffts_plus_task;
  }

  GELOGD("Done loading dynamic partitioned call: [%s]", node->GetName().c_str());
  return SUCCESS;
}

Status PartitionedCallNodeExecutor::PrepareTask(NodeTask &task, TaskContext &context) const {
  RECORD_EXECUTION_EVENT(context.GetExecutionContext(), context.GetNodeName(), "[PartitionedCallPrepareTask] Start");
  GE_CHK_STATUS_RET(task.Init(context), "[Init][Task] failed for [%s].", context.GetNodeName());
  RECORD_EXECUTION_EVENT(context.GetExecutionContext(), context.GetNodeName(), "[PartitionedCallPrepareTask] End");
  return SUCCESS;
}
}  // namespace hybrid
}  // namespace ge
