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

#ifndef GE_GRAPH_LOAD_MODEL_MANAGER_TASK_INFO_FFTS_PLUS_TASK_INFO_H_
#define GE_GRAPH_LOAD_MODEL_MANAGER_TASK_INFO_FFTS_PLUS_TASK_INFO_H_

#include "graph/op_desc.h"
#include "graph/load/model_manager/task_info/task_info.h"
#include "hybrid/node_executor/aicpu/aicpu_ext_info.h"

namespace ge {
class FftsPlusTaskInfo : public TaskInfo {
 public:
  FftsPlusTaskInfo() = default;
  ~FftsPlusTaskInfo() override;

  Status Init(const domi::TaskDef &task_def, DavinciModel *const davinci_model) override;
  Status Distribute() override;
  Status Release() override;
  Status UpdateArgs() override;
  Status CalculateArgs(const domi::TaskDef &task_def, DavinciModel *const davinci_model) override;
  uint32_t GetTaskID() const override { return task_id_; }
  uint32_t GetStreamId() const override { return stream_id_; }

  bool CallSaveDumpInfo() const override { return true; }

  void PostProcess(const domi::TaskDef &task_def) override;

  const std::vector<FusionOpInfo> &GetAllFusionOpInfo() const override { return fusion_op_info_; }

 private:
  Status SetCachePersistentWay(const OpDescPtr &op_desc) const;
  void InitDumpArgs(const OpDescPtr &op_desc, const size_t args_offset);
  uintptr_t FindDumpArgs(const std::string &op_name) const;

  Status CreateAicpuSession(STR_FWK_OP_KERNEL &fwk_op_kernel) const;
  Status LoadCustAicpuSo(const OpDescPtr &op_desc, const domi::FftsPlusAicpuCtxDef &ctx_def) const;

  DavinciModel *davinci_model_{nullptr};
  rtFftsPlusTaskInfo_t ffts_plus_task_info_{nullptr, nullptr, 0U};
  void *args_{nullptr};   // runtime args memory
  uint64_t args_size_{0U}; // runtime args memory length
  std::vector<uintptr_t> io_addrs_;

  uint32_t dump_flag_{RT_KERNEL_DEFAULT};
  std::map<std::string, size_t> dump_args_offset_;
  uint32_t task_id_{0xFFFFFFFFU};
  uint32_t stream_id_{0xFFFFFFFFU};

  std::set<size_t> mode_addr_idx_; //mode addr at pos of io_addrs_
  std::vector<void *> ext_info_addrs_;
  std::vector<FusionOpInfo> fusion_op_info_;
};
}  // namespace ge
#endif  // GE_GRAPH_LOAD_MODEL_MANAGER_TASK_INFO_FFTS_PLUS_TASK_INFO_H_
