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

#ifndef HYBRID_HCCL_NODE_EXECUTOR_H_
#define HYBRID_HCCL_NODE_EXECUTOR_H_
#include "hccl/base.h"
#include "common/opskernel/ge_task_info.h"
#include "graph/op_desc.h"
#include "graph/runtime_inference_context.h"
#include "hybrid/model/hybrid_model.h"
#include "hybrid/node_executor/node_executor.h"

namespace ge {
namespace hybrid {

class HcclNodeTask : public NodeTask {
 public:
  HcclNodeTask() noexcept : NodeTask() {}

  ~HcclNodeTask() override {}

  Status UpdateArgs(TaskContext &context) override;
  Status ExecuteAsync(TaskContext &context, const std::function<void()> &done_callback) override;
  Status Init(TaskContext &context) override;

 private:
  static Status FillHcomOpInfo(const TaskContext &context, const OpDescPtr op_desc,
                               const std::vector<void *> &inputs,
                               const std::vector<void *> &outputs,
                               HcomOperation &hcom_op_info);
  static Status GetInputsOutPuts(const TaskContext &context, std::vector<void *> &inputs,
                                 std::vector<void *> &outputs);
  std::shared_ptr<DavinciModel> davinci_model_ = nullptr;
  std::mutex hccl_mutex_;
  std::condition_variable cond_;
};

class RdmaNodeTask : public NodeTask {
 public:
  RdmaNodeTask() = default;

  ~RdmaNodeTask() override {}

  Status UpdateArgs(TaskContext &context) override;
  Status ExecuteAsync(TaskContext &context, const std::function<void()> &done_callback) override;
  Status Init(TaskContext &context) override;

 private:
  Status SetAddrInfo(const TaskContext &context, const RuntimeInferenceContext &rt_ctx, const uint64_t * const data,
                     const size_t row_num, std::vector<HcomRemoteAccessAddrInfo> &addr_infos) const;
  Status GetOffsetTensor(const TaskContext &context, const RuntimeInferenceContext &rt_ctx,
                         const size_t row_num, GeTensorPtr &offset_tensor) const;
  Status ExtractTensor(const TaskContext &context, std::vector<HcomRemoteAccessAddrInfo> &addr_infos) const;
  std::pair<int64_t, int64_t> remote_index_;
  std::pair<int64_t, int64_t> offset_index_;
  int32_t local_index_ = 0;
  std::mutex hccl_mutex_;
  std::condition_variable cond_;
  bool skip_flag_ = false;
};


class AllToAllNodeTask : public NodeTask {
 public:
  AllToAllNodeTask() = default;

  ~AllToAllNodeTask() override = default;

  Status UpdateArgs(TaskContext &context) override {
    (void)context;
    return SUCCESS;
  }
  Status ExecuteAsync(TaskContext &context, const std::function<void()> &done_callback) override;
  Status Init(TaskContext &context) override {
    (void)context;
    return SUCCESS;
  }

 private:
  std::mutex hccl_mutex_;
  std::condition_variable cond_;
};

class HcclNodeExecutor : public NodeExecutor {
 public:
  Status LoadTask(const HybridModel &model, const NodePtr &node, shared_ptr<NodeTask> &task) const override;
  Status PrepareTask(NodeTask &task, TaskContext &context) const override;
  Status ExecuteTask(NodeTask &task, TaskContext &context, const std::function<void()> &callback) const override;
  Status Initialize() override;
  Status Finalize() override;
  HcclNodeExecutor() noexcept : NodeExecutor(), handle_(nullptr) {}
  ~HcclNodeExecutor() override = default;

 private:
  void *handle_;
};
}  // namespace hybrid
}  // namespace ge

#endif  // HYBRID_HCCL_NODE_EXECUTOR_H_
