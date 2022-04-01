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

#ifndef GE_HYBRID_KERNEL_AICORE_NODE_EXECUTOR_H_
#define GE_HYBRID_KERNEL_AICORE_NODE_EXECUTOR_H_

#include <map>
#include <mutex>
#include "hybrid/node_executor/aicore/aicore_task_builder.h"
#include "hybrid/node_executor/node_executor.h"
#include "graph/bin_cache/node_bin_selector.h"

namespace ge {
namespace hybrid {
class AiCoreNodeTask : public NodeTask {
 public:
  AiCoreNodeTask();
  explicit AiCoreNodeTask(std::vector<std::unique_ptr<AiCoreOpTask>> &&tasks);
  ~AiCoreNodeTask() override = default;
  bool IsSupportDynamicShape() override;
  bool IsSupportHostMemInputOpt() const override;
  bool IsArgsExtendedForHostMemInput() const override;
  void SetNeedHostMemOpt(const bool need_host_mem_opt) override;
  Status UpdateTilingData(TaskContext &context) override;
  bool IsNeedTilling() override { return true; };
  Status SelectBin(TaskContext &task_context, const GraphExecutionContext *const ctx) override;

  Status UpdateArgs(TaskContext &context) override;
  Status ExecuteAsync(TaskContext &context, const std::function<void()> &done_callback) override;

  NodeBinSelector *GetBinSelector() const {
    return bin_selector_;
  }
  void SetBinSelector(NodeBinSelector *bin_selector) {
    bin_selector_ = bin_selector;
  }

 private:
  friend class AiCoreTaskBuilder;
  Status CheckOverflow(TaskContext &context) const;
  Status UpdateOpTasks(const TaskContext &task_context, const HybridModel &model, const NodeCompileCacheItem *cci,
                       const std::vector<domi::TaskDef> &task_defs);

  std::vector<std::unique_ptr<AiCoreOpTask>> tasks_;

  std::unique_ptr<NodeTask> aicpu_task_;
  bool aicpu_exec_ = false;

  std::unique_ptr<NodeTask> fused_task_;
  bool origin_fused_graph_exec_ = false;

  NodeBinSelector *bin_selector_{nullptr};
  uint64_t last_bin_{UINT64_MAX};
};

class AiCoreNodeExecutor : public NodeExecutor {
 public:
  Status LoadTask(const HybridModel &model, const NodePtr &node, shared_ptr<NodeTask> &task) const override;
};

}  // namespace hybrid
}  // namespace ge

#endif //GE_HYBRID_KERNEL_AICORE_NODE_EXECUTOR_H_
