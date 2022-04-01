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

#ifndef GE_HYBRID_CONTROLOP_CONTROL_OP_EXECUTOR_H_
#define GE_HYBRID_CONTROLOP_CONTROL_OP_EXECUTOR_H_

#include <vector>
#include "hybrid/node_executor/node_executor.h"
#include "hybrid/model/graph_item.h"
#include "hybrid/executor/subgraph_executor.h"

namespace ge {
namespace hybrid {
class ExecutorCache {
 public:
  ExecutorCache() = default;
  explicit ExecutorCache(const GraphItem * const graph_item): graph_item_(graph_item) {}
  GE_DELETE_ASSIGN_AND_COPY(ExecutorCache);
  ExecutorCache(ExecutorCache &&exeuctor_cache) noexcept = default;
  ExecutorCache &operator=(ExecutorCache &&exeuctor_cache) noexcept = default;

  ~ExecutorCache() = default;
  Status ExecuteSubgraph(const TaskContext &task_context, const std::function<void()> &done_callback);
  Status ExecuteSubgraph(const TaskContext &task_context, std::vector<TensorValue> &outputs,
                         std::vector<ConstGeTensorDescPtr> &output_desc_list);

 private:
  const GraphItem *graph_item_ = nullptr;
  std::unique_ptr<SubgraphExecutor> graph_executor_;
};

class ControlOpNodeTask : public NodeTask {
 public:
  ControlOpNodeTask() = default;
  ~ControlOpNodeTask() override = default;
  GE_DELETE_ASSIGN_AND_COPY(ControlOpNodeTask);
  using NodeTask::Init;
  virtual Status Init(const NodePtr &node, const HybridModel &model) = 0;
  Status UpdateArgs(TaskContext &context) override;

  Status ExecuteAsync(TaskContext &context, const std::function<void()> &done_callback) override;

 protected:
  virtual Status DoExecuteAsync(TaskContext &task_context, const std::function<void()> &done_callback) = 0;
  static Status ToBool(const TensorValue &tensor, const DataType data_type, bool &value);
};

class IfOpNodeTask : public ControlOpNodeTask {
 public:
  Status Init(const NodePtr &node, const HybridModel &model) override;

 protected:
  Status DoExecuteAsync(TaskContext &task_context, const std::function<void()> &done_callback) override;

 private:
  static constexpr int32_t kIfCondIndex = 0;
  static constexpr uint32_t kThenBranchIndex = 0U;
  static constexpr uint32_t kElseBranchIndex = 1U;

  ExecutorCache then_;
  ExecutorCache else_;
};

class CaseOpNodeTask : public ControlOpNodeTask {
 public:
  Status Init(const NodePtr &node, const HybridModel &model) override;

 protected:
  ExecutorCache &SelectBranch(size_t branch_index);
  Status DoExecuteAsync(TaskContext &task_context, const std::function<void()> &done_callback) override;

 private:
  static constexpr int32_t kCaseBranchIndex = 0;
  static constexpr size_t kMaxBranchNum = SIZE_MAX;
  static constexpr size_t kMinBranchNum = 1U;

  std::vector<ExecutorCache> executors_;
};

class WhileOpNodeTask : public ControlOpNodeTask {
 public:
  Status Init(const NodePtr &node, const HybridModel &model) override;

 protected:
  Status DoExecuteAsync(TaskContext &task_context, const std::function<void()> &done_callback) override;
  Status ExecuteCond(const TaskContext &task_context, bool &is_continue);

  static Status MoveOutputs2Inputs(const TaskContext &task_context);
  Status ExecuteOneLoop(const TaskContext &task_context, bool &is_continue);

 private:
  static constexpr uint32_t kCondBranchIndex = 0U;
  static constexpr uint32_t kBodyBranchIndex = 1U;
  static constexpr size_t kCondOutputSize = 1U;

  ExecutorCache cond_;
  ExecutorCache body_;
};

class FusedOpNodeTask : public ControlOpNodeTask {
 public:
  Status Init(const NodePtr &node, const HybridModel &model) override;

 protected:
  Status DoExecuteAsync(TaskContext &task_context, const std::function<void()> &done_callback) override;

 private:
  ExecutorCache origin_;
};

class ControlOpNodeExecutor : public NodeExecutor {
 public:
  Status LoadTask(const HybridModel &model, const NodePtr &node, shared_ptr<NodeTask> &task) const override;
  Status PrepareTask(NodeTask &task, TaskContext &context) const override;
};
}  // namespace hybrid
}  // namespace ge
#endif // GE_HYBRID_CONTROLOP_CONTROL_OP_EXECUTOR_H_
