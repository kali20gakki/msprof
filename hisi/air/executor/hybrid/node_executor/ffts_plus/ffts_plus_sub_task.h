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

#ifndef GE_HYBRID_NODE_EXECUTOR_FFTS_PLUS_SUB_TASK_H_
#define GE_HYBRID_NODE_EXECUTOR_FFTS_PLUS_SUB_TASK_H_

#include "hybrid/node_executor/node_executor.h"
#include "hybrid/node_executor/ffts_plus/ffts_plus_node_task.h"
#include "register/ffts_plus_update_manager.h"
#include "register/op_tiling_registry.h"

namespace ge {
namespace hybrid {
class FftsPlusSubTask : public NodeTask {
 public:
  FftsPlusSubTask() = default;
  GE_DELETE_ASSIGN_AND_COPY(FftsPlusSubTask);

  virtual Status Load(const HybridModel &model, const NodePtr &node);

  /**
   * @brief Init FFTS Plus Node Task for Update.
   * @param context: instance of TaskContext
   * @return SUCCESS on success, error code otherwise
   */
  Status Init(TaskContext &context) override;

  /**
   * @brief Update tiling data and ffts context.
   * @param context: instance of TaskContext
   * @return SUCCESS on success, error code otherwise
   */
  Status UpdateTilingData(TaskContext &context) override;

  bool IsNeedTilling() override {
    return true;
  }

  Status UpdateArgs(TaskContext &context) override {
    GELOGD("[%s] Done update args successfully.", context.GetNodeName());
    return SUCCESS;
  }

  Status ExecuteAsync(TaskContext &context, const std::function<void()> &done_callback) override;

  virtual Status InitThreadRunInfo(TaskContext &context) = 0;

  virtual Status InitThreadRunParam(TaskContext &context) = 0;

  virtual Status UpdateSubTaskAndCache(const TaskContext &context);

 private:
  friend class FftsPlusAiCoreTask;
  friend class FftsPlusAicAivTask;
  friend class FftsPlusMixAicAivTask;
  friend class FftsPlusAiCpuTask;
  friend class FftsPlusMixL2Task;
  const NodeItem *ffts_node_item_{nullptr};
  FftsPlusNodeTask *ffts_node_task_{nullptr};
  FftsCtxUpdatePtr ffts_plus_ctx_update_;
  AutoThreadParam task_param_{};
  AutoThreadSubTaskFlush task_flush_{};

  bool need_tiling_{true};
  size_t args_addr_base_{0U};
  std::vector<uintptr_t> args_host_data_;
  void *task_args_base_{nullptr};
  size_t task_args_size_{0U}; // Current task used.
};

class FftsPlusAiCoreTask : public FftsPlusSubTask {
 public:
  Status InitThreadRunInfo(TaskContext &context) override;

  Status InitThreadRunParam(TaskContext &context) override;

  /**
   * @ingroup ge
   * @brief Get runtime task Start PC and prefetch count.
   * @return SUCCESS / other error code.
   */
  virtual Status UpdateAddrAndPrefCnt(const OpDesc &op_desc) {
    (void)op_desc;
    return SUCCESS;
  }

  virtual void InitOpTiling(const size_t ctx_idx, const uintptr_t data_base) {
    (void)ctx_idx;
    (void)data_base;
  }

 private:
  /**
   * @brief Update tiling data and ffts context.
   * @param context: instance of TaskContext
   * @return SUCCESS on success, error code otherwise
   */
  void InitCtxIoAddrs(const size_t ctx_idx, const uintptr_t data_base);

  /**
   * @brief Update tiling data and ffts context.
   * @param context: instance of TaskContext
   * @return SUCCESS on success, error code otherwise
   */
  virtual Status InitTaskAddrs(const TaskContext &context);

  /**
   * @brief Set workspace and copy Tiling data.
   * @return SUCCESS on success, error code otherwise
   */
  Status InitOpRunInfo();

 private:
  friend class FftsPlusAicAivTask;
  friend class FftsPlusMixAicAivTask;
  friend class FftsPlusMixL2Task;
  size_t tiling_num_{0U};
  size_t tiling_data_len_{0U};
  std::vector<size_t> tiling_offset_;
};

class FftsPlusAicAivTask : public FftsPlusAiCoreTask {
 public:
  Status UpdateAddrAndPrefCnt(const OpDesc &op_desc) override;
  void InitOpTiling(const size_t ctx_idx, const uintptr_t data_base) override;
};

class FftsPlusMixAicAivTask : public FftsPlusAiCoreTask {
 public:
  Status UpdateAddrAndPrefCnt(const OpDesc &op_desc) override;
  void InitOpTiling(const size_t ctx_idx, const uintptr_t data_base) override;
};

class FftsPlusAiCpuTask : public FftsPlusSubTask {
 public:
  Status ExecuteAsync(TaskContext &context, const std::function<void()> &done_callback) override;

  Status InitThreadRunInfo(TaskContext &context) override;

  Status InitThreadRunParam(TaskContext &context) override;
};

class FftsPlusMixL2Task : public FftsPlusAiCoreTask {
 public:
  ~FftsPlusMixL2Task() override;
  Status Load(const HybridModel &model, const NodePtr &node) override;
  Status UpdateTilingData(TaskContext &context) override;
  Status ExecuteAsync(TaskContext &context, const function<void()> &done_callback) override;
  Status UpdateAddrAndPrefCnt(const OpDesc &op_desc) override;

 private:
  static Status GetTilingRunInfo(const ge::Operator &op, optiling::OpRunInfoV2 &op_run_info);
  Status InitTaskAddrs(const TaskContext &context) override;
  void InitMixL2Addrs(const size_t io_index, const uintptr_t data_base);

  std::vector<uintptr_t> io_addrs_from_taskdef_;
  std::set<size_t> mode_addr_idx_;
  std::vector<void *> ext_args_;
  TBEKernelHandle bin_kernel_handle_;
  rtFftsPlusTaskInfo_t ffts_plus_task_info_{nullptr, nullptr, 0U};
};
} // namespace hybrid
} // namespace ge
#endif // GE_HYBRID_NODE_EXECUTOR_FFTS_PLUS_SUB_TASK_H_
