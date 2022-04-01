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

#ifndef GE_SINGLE_OP_TASK_OP_TASK_H_
#define GE_SINGLE_OP_TASK_OP_TASK_H_

#include <memory>
#include <string>

#include "common/dump/dump_op.h"
#include "common/dump/dump_properties.h"
#include "common/plugin/ge_util.h"
#include "common/profiling/profiling_properties.h"
#include "framework/common/ge_inner_error_codes.h"
#include "graph/op_kernel_bin.h"
#include "runtime/stream.h"
#include "runtime/kernel.h"
#include "runtime/mem.h"
#include "graph/node.h"
#include "graph/runtime_inference_context.h"
#include "graph/utils/op_desc_utils.h"
#include "cce/aicpu_engine_struct.h"
#include "hybrid/node_executor/aicpu/aicpu_ext_info.h"
#include "register/op_tiling.h"
#include "proto/task.pb.h"
#include "framework/common/ge_types.h"

namespace ge {
constexpr uint32_t kHostMemType = 1U;
constexpr size_t kAlignBytes4 = 4U;
constexpr size_t kAlignBytes64 = 64U;

class StreamResource;
struct SingleOpModelParam;
class AtomicAddrCleanOpTask;
class OpTask {
 public:
  OpTask() noexcept {};
  explicit OpTask(const NodePtr &node) : op_(MakeUnique<Operator>(
      OpDescUtils::CreateOperatorFromNode(node->shared_from_this()))) {}
  virtual ~OpTask() = default;
  virtual Status LaunchKernel(const rtStream_t stream) = 0;
  virtual Status UpdateRunInfo();
  virtual Status UpdateArgTable(const SingleOpModelParam &param);
  void SetModelArgs(const std::string &model_name, const uint32_t model_id);
  Status GetTaskIdAndStreamId();
  Status GetProfilingArgs(TaskDescInfo &task_desc_info) const;
  const std::string &GetTaskName() const {return task_name_;}
  void SetOpDesc(const OpDescPtr &op_desc) {
    op_desc_ = op_desc;
  }
  const OpDescPtr &GetOpdesc() const {return op_desc_;}
  Status OpenDump(const rtStream_t stream);
  virtual void GetIoAddr(uintptr_t *&arg_base, size_t &arg_count) = 0;
  virtual Status LaunchKernel(const std::vector<GeTensorDesc> &input_desc,
                              const std::vector<DataBuffer> &input_buffers,
                              std::vector<GeTensorDesc> &output_desc,
                              std::vector<DataBuffer> &output_buffers,
                              const rtStream_t stream);
  virtual const std::string &GetTaskType() const;
  bool NeedReportAtomicTask() const { return clear_atomic_ && (atomic_task_ != nullptr); }
  AtomicAddrCleanOpTask *GetAtomicTask() const { return atomic_task_.get(); }
  virtual const std::string GetOpType() const;
  void SetNeedHostMemOpt(const bool need_host_mem_opt);
  void SetHostMemInputFlag(const bool has_host_mem_input);
  bool GetNeedTiling() const;
  void SetRuntimeContext(RuntimeInferenceContext *const context);
  virtual Status UpdateHostMemInputArgs(const std::vector<DataBuffer> &inputs, const std::vector<DataBuffer> &outputs);
  virtual bool IsSupportHostMemInputOptimize() const { return false; }
  virtual size_t GetHostMemInputDataOffsetInIoAddr() const { return 0U; }
  virtual size_t GetInputAddrAlignBytes() const { return kAlignBytes4; }
  bool IsArgsExtendedForHostMemInput() const { return extend_args_for_host_input_; }
 protected:
  Status DoUpdateArgTable(const SingleOpModelParam &param, const bool keep_workspace);
  void SetTaskTag() const;

 private:
  OpTask(const OpTask &) = delete;
  OpTask &operator=(const OpTask &) = delete;

  friend class AiCpuTaskBuilder;
  friend class AiCpuCCTaskBuilder;
  friend class TbeTaskBuilder;
  friend class SingleOpModel;
  friend class TbeOpTask;
  friend class AiCpuBaseTask;
  friend class AiCpuCCTask;
  friend class AiCpuTask;
  friend class AtomicAddrCleanOpTask;

  std::unique_ptr<Operator> op_{nullptr};
  DumpProperties dump_properties_;
  DumpOp dump_op_;
  OpDescPtr op_desc_{nullptr};
  std::string model_name_;
  uint32_t model_id_ = 0U;
  uint32_t block_dim_ = 1U;
  std::string task_name_;
  bool need_tiling_ = false;
  bool need_host_mem_opt_ = false;
  bool extend_args_for_host_input_ = false;
  bool clear_atomic_ = false;
  std::unique_ptr<AtomicAddrCleanOpTask> atomic_task_;
  uint32_t task_id_ = 0U;
  uint32_t stream_id_ = 0U;
};

struct ArgItemOffset {
  size_t overflow_addr_offset;
  size_t workspace_addr_offset;
  size_t tiling_addr_offset;
  size_t tiling_data_offset;
  size_t host_input_data_offset;
};

class TbeOpTask : public OpTask {
 public:
  TbeOpTask() = default;
  explicit TbeOpTask(const NodePtr &node) : OpTask(node) {}
  ~TbeOpTask() override;
  Status LaunchKernel(const rtStream_t stream) override;
  Status LaunchKernel(const std::vector<GeTensorDesc> &input_desc,
                      const std::vector<DataBuffer> &input_buffers,
                      std::vector<GeTensorDesc> &output_desc,
                      std::vector<DataBuffer> &output_buffers,
                      const rtStream_t stream) override;
  void GetIoAddr(uintptr_t *&arg_base, size_t &arg_count) override;
  void SetStubFunc(const std::string &name, const void *const stub_func);
  void SetKernelArgs(std::unique_ptr<uint8_t[]> &&args, const size_t arg_size,
                     const uint32_t block_dim, const OpDescPtr &op_desc);
  void SetKernelWithHandleArgs(std::unique_ptr<uint8_t[]> &&args, const size_t arg_size,
                               const uint32_t block_dim, const OpDescPtr &op_desc,
                               const domi::KernelDefWithHandle& kernel_def_with_handle);
  void SetAtomicAddrCleanTask(AtomicAddrCleanOpTask *const task) { atomic_task_.reset(task); }

  Status UpdateRunInfo() override;
  Status SetArgIndex();

  void EnableDynamicSupport(const NodePtr &node, const uint32_t max_tiling_size);
  const std::string &GetTaskType() const override;
  void SetHandle(void *const handle);

  void SetOverflowAddr(void *addr) {
    overflow_addr_ = addr;
  }
  Status UpdateHostMemInputArgs(const std::vector<DataBuffer> &inputs, const std::vector<DataBuffer> &outputs) override;
  bool IsSupportHostMemInputOptimize() const override { return true; }
  size_t GetHostMemInputDataOffsetInIoAddr() const override { return args_item_offsets_.host_input_data_offset; }
  void UpdateArgsItemOffset(const size_t io_size, const size_t workspace_addr_size, size_t &arg_size);
 private:
  NodePtr node_;
  // |input addrs|output addrs|overflow_addr|workspace addrs|tiling addr|tiling data|host mem data|
  std::unique_ptr<uint8_t[]> args_;
  size_t arg_size_ = 0U;
  rtArgsEx_t args_ex_ = {};
  std::unique_ptr<rtHostInputInfo_t[]> host_inputs_info_;
  ArgItemOffset args_item_offsets_;
  uint32_t arg_num = 0U;
  uint32_t max_tiling_size_ = 0U;
  size_t input_num_ = 0U; // include const input
  size_t output_num_ = 0U;

  friend class SingleOpModel;
  friend class TbeTaskBuilder;
  friend class AtomicAddrCleanOpTask;
  Status AllocateWorkspaces(const std::vector<int64_t> &workspace_sizes);
  Status DoLaunchKernel(const rtStream_t stream);
  Status DoLaunchKernelWithArgsEx(const rtStream_t stream);
  Status CheckAndExecuteAtomic(const std::vector<GeTensorDesc> &input_desc,
                               const std::vector<DataBuffer> &input_buffers,
                               std::vector<GeTensorDesc> &output_desc,
                               std::vector<DataBuffer> &output_buffers,
                               const rtStream_t stream);
  virtual Status UpdateNodeByShape(const std::vector<GeTensorDesc> &input_desc,
                                   const std::vector<GeTensorDesc> &output_desc) const;
  virtual Status UpdateTilingArgs();
  virtual Status UpdateIoAddr(const std::vector<DataBuffer> &inputs, const std::vector<DataBuffer> &outputs);
  virtual Status CalcTilingInfo();

  Status UpdateArgsItem(const std::vector<DataBuffer> &inputs, const std::vector<DataBuffer> &outputs);
  Status ExtendArgSizeIfNeed(size_t new_size);
  void UpdateOverflowAddr() const;
  void UpdateWorkspaceArgs();
  const void *stub_func_ = nullptr;
  void *sm_desc_ = nullptr;
  std::string stub_name_;
  StreamResource *stream_resource_ = nullptr;

  std::vector<void *> workspaces_;

  uint64_t tiling_key_ = 0U;
  void* handle_ = nullptr;
  std::string node_info_;
  std::vector<size_t> arg_index_; // data index in args
  void *overflow_addr_ = nullptr;
  std::unique_ptr<optiling::utils::OpRunInfo> run_info_;
  uint32_t tiling_data_idx_ = 0U;
};

class AtomicAddrCleanOpTask : public TbeOpTask {
 public:
  AtomicAddrCleanOpTask() = default;
  explicit AtomicAddrCleanOpTask(const NodePtr &node) : TbeOpTask(node) {}
  ~AtomicAddrCleanOpTask() = default;
  Status InitAtomicAddrCleanIndices();
  void SetWorkSpaceAddr(const std::vector<void *> &workspaces) { workspaces_ = workspaces;}
  const std::string GetOpType() const override;
  bool IsSupportHostMemInputOptimize() const override { return false; }
 private:
  Status UpdateNodeByShape(const std::vector<GeTensorDesc> &input_desc,
                           const std::vector<GeTensorDesc> &output_desc) const override;
  Status UpdateIoAddr(const std::vector<DataBuffer> &inputs, const std::vector<DataBuffer> &outputs) override;
  Status UpdateTilingArgs() override;
  Status CalcTilingInfo() override;

  std::vector<int32_t> atomic_output_indices_;
  std::vector<int32_t> atomic_workspace_indices_;
  std::vector<void *> workspaces_;
};

class AiCpuBaseTask : public OpTask {
 public:
  AiCpuBaseTask() = default;
  ~AiCpuBaseTask() override;
  UnknowShapeOpType GetUnknownType() const { return unknown_type_; }
  Status UpdateArgTable(const SingleOpModelParam &param) override;
  const std::string &GetTaskType() const override;
 protected:
  Status UpdateIoAddr(const std::vector<DataBuffer> &inputs, const std::vector<DataBuffer> &outputs);
  Status SetInputConst();
  Status SetExtInfoAndType(const std::string &kernel_ext_info, const uint64_t kernel_id);

  Status UpdateExtInfo(const std::vector<GeTensorDesc> &input_desc,
                       const std::vector<GeTensorDesc> &output_desc,
                       const rtStream_t stream);
  Status UpdateOutputShape(std::vector<GeTensorDesc> &output_desc);
  Status UpdateShapeToOutputDesc(const GeShape &shape_new, GeTensorDesc &output_desc) const;
  Status UpdateShapeAndDataByResultSummary(std::vector<GeTensorDesc> &output_desc,
                                           std::vector<DataBuffer> &outputs,
                                           const rtStream_t stream);
  Status ReadResultSummaryAndPrepareMemory();

  Status PrepareCopyInputs(const std::vector<DataBuffer> &outputs);

  Status UpdateShapeByHbmBuffer(std::vector<GeTensorDesc> &output_desc);

  virtual Status CopyDataToHbm(std::vector<DataBuffer> &outputs, rtStream_t stream) = 0;
  // for blocking aicpu op
  Status DistributeWaitTaskForAicpuBlockingOp(const rtStream_t stream);
  Status UpdateEventIdForBlockingAicpuOp();
  Status CheckDeviceSupportBlockingAicpuOpProcess(bool &is_support) const;

 private:
  AiCpuBaseTask(const AiCpuBaseTask &) = delete;
  AiCpuBaseTask &operator=(const AiCpuBaseTask &) = delete;

  friend class AiCpuTaskBuilder;
  friend class AiCpuCCTaskBuilder;
  friend class AiCpuTask;
  friend class AiCpuCCTask;

  size_t num_inputs_ = 0U;
  size_t num_outputs_ = 0U;
  UnknowShapeOpType unknown_type_ = DEPEND_IN_SHAPE;
  std::unique_ptr<ge::hybrid::AicpuExtInfoHandler> aicpu_ext_handle_;
  void *ext_info_addr_dev_ = nullptr;
  std::vector<bool> input_is_const_;
  // for blocking aicpu op
  bool is_blocking_aicpu_op_ = false;
  rtEvent_t rt_event_ = nullptr;
  std::vector<void *> output_summary_;
  std::vector<aicpu::FWKAdapter::ResultSummary> output_summary_host_;

  void *copy_input_release_flag_dev_ = nullptr;
  void *copy_input_data_size_dev_ = nullptr;
  void *copy_input_src_dev_ = nullptr;
  void *copy_input_dst_dev_ = nullptr;

  std::vector<void *> out_shape_hbm_;
};

class AiCpuTask : public AiCpuBaseTask {
 public:
  AiCpuTask() = default;
  ~AiCpuTask() override;

  Status LaunchKernel(const rtStream_t stream) override;
  void GetIoAddr(uintptr_t *&arg_base, size_t &arg_count) override;

  Status LaunchKernel(const std::vector<GeTensorDesc> &input_desc,
                      const std::vector<DataBuffer> &input_buffers,
                      std::vector<GeTensorDesc> &output_desc,
                      std::vector<DataBuffer> &output_buffers,
                      const rtStream_t stream) override;
  Status SetMemCopyTask(const domi::KernelExDef &kernel_def);
  Status UpdateHostMemInputArgs(const std::vector<DataBuffer> &inputs, const std::vector<DataBuffer> &outputs) override;
  bool IsSupportHostMemInputOptimize() const override { return true; }
  size_t GetInputAddrAlignBytes() const override { return kAlignBytes64; }
  size_t GetHostMemInputDataOffsetInIoAddr() const override { return host_mem_input_data_offset_; }
 private:
  // for copy task.
  Status InitForSummaryAndCopy();
  Status CopyDataToHbm(std::vector<DataBuffer> &outputs, const rtStream_t stream) override;

  friend class AiCpuTaskBuilder;
  void *workspace_addr_ = nullptr;
  std::string task_info_;
  // device addr
  void *args_ = nullptr;
  size_t arg_size_ = 0U;
  std::string op_type_;
  // device addr
  void *io_addr_ = nullptr;
  size_t io_addr_size_ = 0U;

  // host addr
  std::vector<void *> io_addr_host_;
  size_t host_mem_input_data_offset_ = 0U;

  // for copy task
  void *copy_task_args_buf_ = nullptr;
  void *copy_workspace_buf_ = nullptr;

  void *copy_ioaddr_dev_ = nullptr;

  uint64_t kernel_id_ = 0U;
};

class AiCpuCCTask : public AiCpuBaseTask {
 public:
  AiCpuCCTask() = default;
  ~AiCpuCCTask() override;
  AiCpuCCTask(const AiCpuCCTask &) = delete;
  AiCpuCCTask &operator=(const AiCpuCCTask &) = delete;
  Status SetMemCopyTask(const domi::KernelDef &kernel_def);
  Status LaunchKernel(const rtStream_t stream) override;
  Status LaunchKernel(const std::vector<GeTensorDesc> &input_desc,
                      const std::vector<DataBuffer> &input_buffers,
                      std::vector<GeTensorDesc> &output_desc,
                      std::vector<DataBuffer> &output_buffers,
                      const rtStream_t stream) override;
  void GetIoAddr(uintptr_t *&arg_base, size_t &arg_count) override;
  void SetKernelArgs(std::unique_ptr<uint8_t[]> args, const size_t arg_size);
  void SetSoName(const std::string &so_name);
  void SetkernelName(const std::string &kernel_Name);
  void SetIoAddr(uintptr_t *const io_addr);
  bool IsSupportHostMemInputOptimize() const override { return true; }
  size_t GetHostMemInputDataOffsetInIoAddr() const override { return host_mem_input_data_offset_; }
  Status UpdateHostMemInputArgs(const std::vector<DataBuffer> &inputs, const std::vector<DataBuffer> &outputs) override;
 private:
  Status InitForSummaryAndCopy();
  Status CopyDataToHbm(std::vector<DataBuffer> &outputs, const rtStream_t stream) override;

 private:
  friend class AiCpuCCTaskBuilder;
  std::string so_name_;
  std::string kernel_name_;
  std::unique_ptr<uint8_t[]> args_;
  std::unique_ptr<rtHostInputInfo_t[]> host_inputs_info_;
  rtArgsEx_t args_ex_ = {};
  size_t arg_size_ = 0U;
  void *sm_desc_ = nullptr;
  uintptr_t *io_addr_ = nullptr;
  size_t io_addr_num_ = 0U;
  size_t host_mem_input_data_offset_ = 0U;
  bool is_custom_ = false;
  uint32_t dump_flag_ = RT_KERNEL_DEFAULT;
  std::string op_type_;
  uint64_t kernel_id_ = 0U;
  // host memcpy mem
  std::unique_ptr<uint8_t[]> memcpy_args_;
  std::string memcpy_so_name_;
  std::string memcpy_kernel_name_;
  std::vector<uint64_t> copy_io_addr_;
  // args size
  uint32_t memcpy_args_size_ = 0U;
};

class MemcpyAsyncTask : public OpTask {
 public:
  Status LaunchKernel(const rtStream_t stream) override;
  void GetIoAddr(uintptr_t *&arg_base, size_t &arg_count) override;

 private:
  friend class SingleOpModel;
  friend class RtsKernelTaskBuilder;

  uintptr_t addresses_[2] = {0U, 0U}; // src address and dst address
  size_t dst_max_;
  size_t count_;
  rtMemcpyKind_t kind_;
  NodePtr node_;
};
}  // namespace ge

#endif  // GE_SINGLE_OP_TASK_OP_TASK_H_
