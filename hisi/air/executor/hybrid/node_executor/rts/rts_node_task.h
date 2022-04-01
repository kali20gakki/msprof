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

#ifndef GE_HYBRID_NODE_EXECUTOR_RTS_RTS_NODE_TASK_H_
#define GE_HYBRID_NODE_EXECUTOR_RTS_RTS_NODE_TASK_H_

#include "hybrid/node_executor/node_executor.h"
#include "proto/task.pb.h"

namespace ge {
namespace hybrid {
class RtsNodeTask : public NodeTask {
 public:
  RtsNodeTask() = default;
  ~RtsNodeTask() override = default;
  Status Init(TaskContext &context) override {
    (void)context;
    return SUCCESS;
  }

  virtual Status Init(const HybridModel &model, const NodePtr &node) {
    (void)model;
    GELOGD("[%s] Done initialization successfully.", node->GetName().c_str());
    return SUCCESS;
  }

  Status UpdateArgs(TaskContext &context) override {
    GELOGD("[%s] Done update args successfully.", context.GetNodeName());
    return SUCCESS;
  }

  static Status GetScalarIndexValue(const TaskContext &task_ctx, const int32_t idx, int64_t &val);
 private:
  RtsNodeTask& operator=(const RtsNodeTask&) = delete;
  RtsNodeTask(const RtsNodeTask&) = delete;
};

class StreamActiveNodeTask : public RtsNodeTask {
 public:
  Status ExecuteAsync(TaskContext &context, const std::function<void()> &done_callback) override;
};

class StreamSwitchNodeTask : public RtsNodeTask {
 public:
  Status Init(const HybridModel &model, const NodePtr &node) override;
  Status ExecuteAsync(TaskContext &context, const std::function<void()> &done_callback) override;

 private:
  std::function<bool(int64_t, int64_t)> comp_func_{nullptr};
};

class StreamMergeNodeTask : public RtsNodeTask {
 public:
  Status ExecuteAsync(TaskContext &context, const std::function<void()> &done_callback) override;
};

class PassThroughNodeTask : public RtsNodeTask {
 public:
  Status ExecuteAsync(TaskContext &context, const std::function<void()> &done_callback) override;
};

class LabelSetNodeTask : public RtsNodeTask {
 public:
  Status ExecuteAsync(TaskContext &context, const std::function<void()> &done_callback) override;
};

class LabelGotoNodeTask : public RtsNodeTask {
 public:
  Status ExecuteAsync(TaskContext &context, const std::function<void()> &done_callback) override;
};

class LabelSwitchNodeTask : public RtsNodeTask {
 public:
  Status ExecuteAsync(TaskContext &context, const std::function<void()> &done_callback) override;
};
}  // namespace hybrid
}  // namespace ge
#endif  // GE_HYBRID_NODE_EXECUTOR_RTS_RTS_NODE_TASK_H_
