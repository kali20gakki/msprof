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

#ifndef GE_GRAPH_LOAD_NEW_MODEL_MANAGER_TASK_INFO_KERNEL_EX_TASK_INFO_H_
#define GE_GRAPH_LOAD_NEW_MODEL_MANAGER_TASK_INFO_KERNEL_EX_TASK_INFO_H_

#include "graph/op_desc.h"
#include "graph/def_types.h"
#include "graph/load/model_manager/task_info/task_info.h"
#include "hybrid/node_executor/aicpu/aicpu_ext_info.h"

namespace ge {
class KernelExTaskInfo : public TaskInfo {
 public:
  KernelExTaskInfo() = default;

  ~KernelExTaskInfo() override {}

  Status Init(const domi::TaskDef &task_def, DavinciModel *const davinci_model) override;

  Status Distribute() override;

  Status Release() override;

  Status UpdateArgs() override;

  Status CalculateArgs(const domi::TaskDef &task_def, DavinciModel *const davinci_model) override;

  uint32_t GetTaskID() const override { return task_id_; }

  uint32_t GetStreamId() const override { return stream_id_; }

  uintptr_t GetDumpArgs() const override {
    return static_cast<uintptr_t>(PtrToValue(dump_args_));
  }

  bool CallSaveDumpInfo() const override {
    return true;
  }

  void PostProcess(const domi::TaskDef &task_def) override;

 private:
  static Status CopyTaskInfo(const domi::KernelExDef &kernel_def, const RuntimeParam &rts_param,
                             const OpDescPtr &op_desc);
  void SetIoAddrs(const OpDescPtr &op_desc);

  void InitDumpFlag(const OpDescPtr &op_desc);
  void InitDumpArgs(void *const addr, const OpDescPtr &op_desc);
  bool NeedUpdateAddr(const OpDescPtr &op_desc) const;
  Status InitTaskExtInfo(const std::string &ext_info, const OpDescPtr &op_desc);

  // for blocking aicpu op
  Status DistributeWaitTaskForAicpuBlockingOp() const;
  Status CheckDeviceSupportBlockingAicpuOpProcess(bool &is_support) const;
  Status UpdateEventIdForAicpuBlockingOp(const OpDescPtr &op_desc, hybrid::AicpuExtInfoHandler &ext_handle) const;

  uint32_t task_id_{0U};
  uint32_t stream_id_{0U};
  uint32_t dump_flag_{RT_KERNEL_DEFAULT};
  uint32_t kernel_buf_size_{0U};
  DavinciModel *davinci_model_{nullptr};
  OpDescPtr op_desc_;
  void *kernel_buf_{nullptr};
  void *input_output_addr_{nullptr};
  void *ext_info_addr_{nullptr};
  void *dump_args_{nullptr};
  std::vector<void *> io_addrs_;
  uint32_t args_offset_{0U};
  int64_t fixed_addr_offset_{0};
  int32_t topic_type_flag_{-1};
  bool is_blocking_aicpu_op_{false};
  std::vector<void *> ext_args_;
  bool own_args_memory_ = true;
};
}  // namespace ge
#endif  // GE_GRAPH_LOAD_NEW_MODEL_MANAGER_TASK_INFO_KERNEL_EX_TASK_INFO_H_
