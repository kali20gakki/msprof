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

#ifndef HYBRID_KNOWN_NODE_EXECUTOR_H_
#define HYBRID_KNOWN_NODE_EXECUTOR_H_
#include "hybrid/node_executor/node_executor.h"
#include "hybrid/model/hybrid_model.h"
#include "graph/op_desc.h"
#include "graph/load/model_manager/davinci_model.h"

namespace ge {
namespace hybrid {

class KnownNodeTask : public NodeTask {
 public:
  explicit KnownNodeTask(std::shared_ptr<DavinciModel> davinci_model)
      : NodeTask(), davinci_model_(davinci_model)
    {}

  ~KnownNodeTask() = default;

  Status UpdateArgs(TaskContext &context) override;
  Status ExecuteAsync(TaskContext &context, const std::function<void()> &done_callback) override;
  Status Init(TaskContext &context) override;
  Status InitDavinciModel(const HybridModel &model, const TensorBuffer *const weight_buffer);
  Status ReportProfilingData() override;
 protected:
  virtual Status DoInitDavinciModel(const uintptr_t weight, const size_t weight_size);
 private:
  std::shared_ptr<DavinciModel> davinci_model_ = nullptr;
  bool load_flag_ = false;
};

class KnownNodeExecutor : public NodeExecutor {
 public:
  Status LoadTask(const HybridModel &model, const NodePtr &node, shared_ptr<NodeTask> &task) const override;
  Status PrepareTask(NodeTask &task, TaskContext &context) const override;
  Status ExecuteTask(NodeTask &task, TaskContext &context, const std::function<void()> &callback) const override;
  Status ReportProfilingData(const NodeItem &node_item) const override;
  ~KnownNodeExecutor() override {}

 private:
  static Status ParseAttrForAllocatingOutputs(NodeItem &node_item, const ComputeGraph &graph);
  static Status GetDataNodes(const ComputeGraph &graph, std::map<NodePtr, int32_t> &data_indices);
  static Status GetModelAndGraph(const HybridModel &model,
                                 const NodePtr &node,
                                 GeModelPtr &ge_model,
                                 ComputeGraphPtr &graph);
  Status SetDaviciModel(const HybridModel &model, const NodePtr &node,
                        std::shared_ptr<DavinciModel> &davinci_model) const;
};
}  // namespace hybrid
}  // namespace ge

#endif //HYBRID_KNOWN_NODE_EXECUTOR_H_
