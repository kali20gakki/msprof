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

#ifndef GE_HYBRID_NODE_EXECUTOR_PARTITIONED_CALL_NODE_EXECUTOR_H_
#define GE_HYBRID_NODE_EXECUTOR_PARTITIONED_CALL_NODE_EXECUTOR_H_

#include "hybrid/node_executor/node_executor.h"
#include "hybrid/model/hybrid_model.h"
#include "hybrid/executor/node_state.h"
#include "hybrid/executor/subgraph_executor.h"
#include "common/thread_pool.h"

namespace ge {
namespace hybrid {
class PartitionedCallNodeTask : public NodeTask {
 public:
  explicit PartitionedCallNodeTask(const GraphItem * const graph_item);
  ~PartitionedCallNodeTask() override;

  Status Init(TaskContext &context) override;

  Status UpdateArgs(TaskContext &context) override;

  Status ExecuteAsync(TaskContext &context, const std::function<void()> &done_callback) override;

 private:
  Status Callback(const std::function<void()> &done_callback);

  const GraphItem *graph_item_;
  std::unique_ptr<SubgraphExecutor> subgraph_executor_;
};

class PartitionedCallNodeExecutor : public NodeExecutor {
 public:
  PartitionedCallNodeExecutor() noexcept : NodeExecutor() {}
  ~PartitionedCallNodeExecutor() override = default;
  Status LoadTask(const HybridModel &model, const NodePtr &node, shared_ptr<NodeTask> &task) const override;
  Status PrepareTask(NodeTask &task, TaskContext &context) const override;
};
}  // namespace hybrid
}  // namespace ge
#endif // GE_HYBRID_NODE_EXECUTOR_PARTITIONED_CALL_NODE_EXECUTOR_H_
