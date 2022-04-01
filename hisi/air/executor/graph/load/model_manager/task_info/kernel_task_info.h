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

#ifndef GE_GRAPH_LOAD_NEW_MODEL_MANAGER_TASK_INFO_KERNEL_TASK_INFO_H_
#define GE_GRAPH_LOAD_NEW_MODEL_MANAGER_TASK_INFO_KERNEL_TASK_INFO_H_

#include "graph/op_desc.h"
#include "graph/def_types.h"
#include "graph/load/model_manager/task_info/task_info.h"
#include "hybrid/node_executor/aicpu/aicpu_ext_info.h"

namespace ge {
class KernelTaskInfo : public TaskInfo {
 public:
  KernelTaskInfo() : TaskInfo(), ctx_() {}

  ~KernelTaskInfo() override {
    davinci_model_ = nullptr;
    stub_func_ = nullptr;
    sm_desc_ = nullptr;
    flowtable_ = nullptr;
    args_ = nullptr;
    superkernel_device_args_addr_ = nullptr;
    superkernel_dev_nav_table_ = nullptr;
  }

  Status Init(const domi::TaskDef &task_def, DavinciModel *const davinci_model) override;

  Status Distribute() override;

  Status UpdateArgs() override;

  Status CalculateArgs(const domi::TaskDef &task_def, DavinciModel *const davinci_model) override;

  Status Release() override;

  const std::vector<FusionOpInfo> &GetAllFusionOpInfo() const override { return fusion_op_info_; }

  uint32_t GetTaskID() const override { return task_id_; }

  uint32_t GetStreamId() const override { return stream_id_; }

  uintptr_t GetDumpArgs() const override {
    return PtrToValue(dump_args_);
  }

  uintptr_t GetArgs() const override {
    return PtrToValue(args_) + io_addr_offset_;
  }

  void PostProcess(const domi::TaskDef &task_def) override;

  uint32_t GetSktTaskID() const override { return skt_id_; }

  bool CallSaveDumpInfo() const override  { return call_save_dump_; }

 private:
  struct tagOpContext {
   private:
    friend class KernelTaskInfo;
    uint32_t opIndex = 0U;
    uint32_t opCount = 0U;
    uint32_t opIndex2[CC_FUSION_OP_MAX] = {};
    std::vector<uint16_t> argsOffset;
  } ctx_;

  std::vector<FusionOpInfo> fusion_op_info_;

  Status InitTVMTask(const OpDescPtr &op_desc, const domi::KernelDef &kernel_def);

  Status InitAICPUCustomTask(const OpDescPtr &op_desc, const domi::KernelDef &kernel_def);

  Status InitAicpuTask(const OpDescPtr &op_desc, const domi::KernelDef &kernel_def);

  Status InitTVMContext(const domi::KernelContext &context);

  Status InitAicpuTaskExtInfo(const std::string &ext_info);

  Status StoreInputOutputTensor(const std::vector<void *> &input_data_addrs,
                                const std::vector<void *> &output_data_addrs,
                                const std::vector<ccAICPUTensor> &input_descs,
                                const std::vector<ccAICPUTensor> &output_descs);

  Status UpdateL2Data(const domi::KernelDef &kernel_def);

  uint8_t IsL2CpToDDR(const uint8_t origain_L2_load_to_ddr) const;

  Status SuperKernelDistribute();
  bool IsL1FusionOp(const OpDescPtr &op_desc) const;
  void SetIoAddrs(const OpDescPtr &op_desc);
  void InitDumpFlag();
  void InitDumpArgs(const uint32_t offset);
  void SetContinuousArgs(DavinciModel *const davinci_model);
  void SetNoncontinuousArgs(DavinciModel *const davinci_model);
  Status CopyNoncontinuousArgs(const uint16_t offset);

  // For super kernel
  Status SaveSKTDumpInfo();
  void UpdateTaskId();
  void UpdateSKTTaskId();
  Status SKTFinalize();
  Status SuperKernelLaunch();
  uint32_t GetDumpFlag() const;
  Status SaveSuperKernelInfo();
  bool IsMarkedLastNode() const;
  bool FirstCallSKTLaunchCheck() const;
  bool DoubleCallSKTSaveCheck() const;
  void SetArgs();

  // for blocking aicpu op
  Status DistributeWaitTaskForAicpuBlockingOp() const;
  Status CheckDeviceSupportBlockingAicpuOpProcess(bool &is_support) const;
  Status UpdateEventIdForAicpuBlockingOp(hybrid::AicpuExtInfoHandler &ext_handle) const;

  const void *stub_func_{nullptr};
  void *args_{nullptr};
  void *sm_desc_{nullptr};
  void *flowtable_{nullptr};
  uint32_t block_dim_{0U};
  uint32_t args_size_{0U};
  uint32_t task_id_{0U};
  uint32_t stream_id_{0U};
  std::string so_name_;
  std::string kernel_name_;
  ccKernelType kernel_type_{ccKernelType::CCE_AI_CORE};
  uint32_t dump_flag_{RT_KERNEL_DEFAULT};
  void *dump_args_{nullptr};
  OpDescPtr op_desc_;   // Clear after distribute.
  std::vector<void *> io_addrs_;
  DavinciModel *davinci_model_{nullptr};
  uint32_t args_offset_ = 0U;
  uint32_t hybrid_args_offset_ = 0U;
  std::vector<uint8_t> args_addr_;
  uint16_t io_addr_offset_{0U};
  bool l2_buffer_on_ = false;
  bool call_save_dump_ = false;
  int32_t topic_type_flag_ = -1;

  // aicpu ext_info device mem
  void *aicpu_ext_info_addr_ = nullptr;

  // For super kernel
  void *skt_dump_args_ = nullptr;
  uint32_t skt_id_{0U};
  std::string stub_func_name_;
  bool is_l1_fusion_enable_{false};
  bool is_n_batch_spilt_{false};
  int64_t group_key_{-1};
  bool has_group_key_{false};
  uint32_t skt_dump_flag_ = RT_KERNEL_DEFAULT;
  void *superkernel_device_args_addr_ = nullptr;
  void *superkernel_dev_nav_table_ = nullptr;
  bool is_blocking_aicpu_op_ = false;
  bool own_args_memory_ = false;

  // for support overflow detection
  void *globalworkspace_overflow_addr_ = nullptr;

  struct AICPUCustomInfo {
   private:
    friend class KernelTaskInfo;
    void *input_descs = nullptr;
    void *input_addrs = nullptr;
    void *output_descs = nullptr;
    void *output_addrs = nullptr;
    void *attr_handle = nullptr;
  } custom_info_;
};
}  // namespace ge
#endif  // GE_GRAPH_LOAD_NEW_MODEL_MANAGER_TASK_INFO_KERNEL_TASK_INFO_H_
