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

#include "single_op/task/op_task.h"

#include <chrono>
#include <thread>

#include <google/protobuf/extension_set.h>
#include "aicpu/common/aicpu_task_struct.h"
#include "common/dump/dump_manager.h"
#include "common/dump/dump_op.h"
#include "common/profiling/profiling_manager.h"
#include "common/formats/formats.h"
#include "common/math/math_util.h"
#include "common/utils/executor_utils.h"
#include "framework/common/debug/log.h"
#include "framework/common/profiling_definitions.h"
#include "runtime/rt.h"
#include "single_op/task/build_task_utils.h"
#include "common/profiling_definitions.h"
#include "graph/def_types.h"

namespace ge {
namespace {
constexpr int32_t kLaunchRetryTimes = 1000;
constexpr size_t kMemcpyArgCount = 2U;
constexpr int32_t kSleepTime = 10;
constexpr size_t kCopyNum = 2U;


void FreeHbm(void *const var) {
  if (var != nullptr) {
    (void)rtFree(var);
  }
}
}  // namespace

Status OpTask::OpenDump(const rtStream_t stream) {
  if (DumpManager::GetInstance().GetDumpProperties(kInferSessionId).IsSingleOpNeedDump()) {
    GELOGI("Dump is open in single op, start to set dump info");
    std::vector<uint64_t> input_addrs;
    std::vector<uint64_t> output_adds;
    const auto input_size = op_desc_->GetInputsSize();
    const auto output_size = op_desc_->GetOutputsSize();
    uintptr_t *arg_base = nullptr;
    size_t arg_num = 0U;
    GetIoAddr(arg_base, arg_num);
    if (arg_num < (input_size + output_size)) {
      GELOGE(ACL_ERROR_GE_INTERNAL_ERROR,
          "[Check][Size]io_addrs_for_dump_ size %zu is not equal input and output size %zu",
          arg_num, input_size + output_size);
      REPORT_INNER_ERROR("E19999", "io_addrs_for_dump_ size %zu is not equal input and output size %zu",
          arg_num, input_size + output_size);
      return ACL_ERROR_GE_INTERNAL_ERROR;
    }

    for (size_t i = 0U; i < input_size; i++) {
      input_addrs.emplace_back(*arg_base);
      ++arg_base;
    }
    for (size_t j = 0U; j < output_size; j++) {
      output_adds.emplace_back(*arg_base);
      ++arg_base;
    }
    dump_op_.SetDumpInfo(DumpManager::GetInstance().GetDumpProperties(kInferSessionId),
                         op_desc_, input_addrs, output_adds, stream);
    const auto status = dump_op_.LaunchDumpOp();
    if (status != SUCCESS) {
      GELOGE(status, "[Launch][DumpOp] failed in single op.");
      return status;
    }
    return SUCCESS;
  }
  GELOGI("Dump is not open in single op");
  return SUCCESS;
}

Status OpTask::GetTaskIdAndStreamId() {
  if (ProfilingManager::Instance().ProfilingModelLoadOn()) {
    GE_CHK_RT_RET(rtGetTaskIdAndStreamID(&task_id_, &stream_id_));
  }
  return SUCCESS;
}

void OpTask::SetTaskTag() const {
  if (op_desc_ != nullptr) {
    const rtError_t rt_set_tag = rtSetTaskTag(op_desc_->GetName().c_str());
    if (rt_set_tag != RT_ERROR_NONE) {
      GELOGW("[Call][rtSetTaskTag] failed, ret:0x%X", rt_set_tag);
    }
  }
}

void TbeOpTask::SetStubFunc(const std::string &name, const void *const stub_func) {
  this->stub_name_ = name;
  this->stub_func_ = stub_func;
  this->task_name_ = name;
}

void TbeOpTask::SetKernelArgs(std::unique_ptr<uint8_t[]> &&args, const size_t arg_size,
                              const uint32_t block_dim, const OpDescPtr &op_desc) {
  args_ = std::move(args);
  arg_size_ = arg_size;
  block_dim_ = block_dim;
  op_desc_ = op_desc;
}

void TbeOpTask::SetKernelWithHandleArgs(std::unique_ptr<uint8_t[]> &&args, const size_t arg_size,
                                        const uint32_t block_dim, const OpDescPtr &op_desc,
                                        const domi::KernelDefWithHandle &kernel_def_with_handle) {
  SetKernelArgs(std::move(args), arg_size, block_dim, op_desc);
  node_info_ = kernel_def_with_handle.node_info();
}

void OpTask::SetModelArgs(const std::string &model_name, const uint32_t model_id) {
  model_name_ = model_name;
  model_id_ = model_id;
}

const std::string OpTask::GetOpType() const {
  if (op_desc_ == nullptr) {
    return "";
  } else {
    return op_desc_->GetType();
  }
}

Status OpTask::GetProfilingArgs(TaskDescInfo &task_desc_info) const {
  GE_CHECK_NOTNULL(op_desc_);
  const std::string op_name = op_desc_->GetName();
  GELOGD("Get profiling args of op [%s] end, task_id[%u], stream_id[%u].",
         op_name.c_str(), task_id_, stream_id_);

  task_desc_info.model_name = model_name_;
  task_desc_info.block_dim = block_dim_;
  task_desc_info.task_id = task_id_;
  task_desc_info.stream_id = stream_id_;
  task_desc_info.op_name = op_name;
  task_desc_info.op_type = GetOpType();
  auto &prof_mgr = ProfilingManager::Instance();
  prof_mgr.GetOpInputOutputInfo(op_desc_, task_desc_info);
  return SUCCESS;
}

Status OpTask::UpdateRunInfo() {
  return UNSUPPORTED;
}

Status OpTask::DoUpdateArgTable(const SingleOpModelParam &param, const bool keep_workspace) {
  const auto addresses = BuildTaskUtils::GetAddresses(op_desc_, param, keep_workspace);
  const auto all_addresses = BuildTaskUtils::JoinAddresses(addresses);
  uintptr_t *arg_base = nullptr;
  size_t arg_num = 0U;
  GetIoAddr(arg_base, arg_num);
  if (arg_num < all_addresses.size()) {
    GELOGE(ACL_ERROR_GE_INTERNAL_ERROR,
        "[Check][Size][%s] arg number mismatches, expect at least = %zu, but got = %zu.",
        op_desc_->GetName().c_str(), all_addresses.size(), arg_num);
    REPORT_INNER_ERROR("E19999", "%s arg number mismatches, expect at least = %zu, but got = %zu.",
        op_desc_->GetName().c_str(), all_addresses.size(), arg_num);
    return ACL_ERROR_GE_INTERNAL_ERROR;
  }

  for (void *const addr : all_addresses) {
    *arg_base = PtrToValue(addr);
    arg_base++;
  }
  return SUCCESS;
}

Status OpTask::UpdateArgTable(const SingleOpModelParam &param) {
  return DoUpdateArgTable(param, true);
}

Status OpTask::LaunchKernel(const std::vector<GeTensorDesc> &input_desc,
                            const std::vector<DataBuffer> &input_buffers,
                            std::vector<GeTensorDesc> &output_desc,
                            std::vector<DataBuffer> &output_buffers,
                            const rtStream_t stream) {
  (void)input_desc;
  (void)input_buffers;
  (void)output_desc;
  (void)output_buffers;
  (void)stream;
  return UNSUPPORTED;
}

const std::string &OpTask::GetTaskType() const { return kTaskTypeInvalid; }

void OpTask::SetNeedHostMemOpt(const bool need_host_mem_opt) {
  need_host_mem_opt_ = need_host_mem_opt;
}

void OpTask::SetHostMemInputFlag(const bool has_host_mem_input) {
  extend_args_for_host_input_ = has_host_mem_input;
}

bool OpTask::GetNeedTiling() const {
  return need_tiling_;
}

void OpTask::SetRuntimeContext(RuntimeInferenceContext *const context) {
  if (op_ != nullptr) {
    OpDescUtils::SetRuntimeContextToOperator(*op_, context);
  }
}

Status OpTask::UpdateHostMemInputArgs(const std::vector<DataBuffer> &inputs, const std::vector<DataBuffer> &outputs) {
  (void)inputs;
  (void)outputs;
  return SUCCESS;
}

TbeOpTask::~TbeOpTask() {
  if (sm_desc_ != nullptr) {
    (void)rtMemFreeManaged(sm_desc_);
  }
}

const std::string &TbeOpTask::GetTaskType() const { return kTaskTypeAicore; }

void TbeOpTask::SetHandle(void *const handle) {
  this->handle_ = handle;
}

Status TbeOpTask::LaunchKernel(const rtStream_t stream) {
  GELOGD("To invoke rtKernelLaunch. task = %s, block_dim = %u", this->stub_name_.c_str(), block_dim_);
  auto ret = DoLaunchKernel(stream);

  int32_t retry_times = 0;
  while ((ret != SUCCESS) && (retry_times < kLaunchRetryTimes)) {
    retry_times++;
    GELOGW("Retry after %d ms, retry_times: %d", kSleepTime, retry_times);
    std::this_thread::sleep_for(std::chrono::milliseconds(kSleepTime));
    ret = DoLaunchKernel(stream);
  }

  if (ret != SUCCESS) {
    GELOGE(ret, "[Invoke][RtKernelLaunch] failed. ret = %d, task = %s", ret, this->stub_name_.c_str());
    REPORT_INNER_ERROR("E19999", "invoke rtKernelLaunch failed, ret = %d, task = %s", ret, this->stub_name_.c_str());
    return ret;
  }
  GELOGI("[TASK_INFO] %s", this->stub_name_.c_str());

  return SUCCESS;
}

Status TbeOpTask::CalcTilingInfo() {
  GE_CHECK_NOTNULL(op_);
  const auto ret = optiling::OpParaCalculateV2(*op_, *run_info_);
  if (ret != GRAPH_SUCCESS) {
    GELOGE(ACL_ERROR_GE_INTERNAL_ERROR, "[Invoke][OpParaCalculate] failed, ret = %u.", ret);
    REPORT_INNER_ERROR("E19999", "invoke OpParaCalculate failed, ret = %u.", ret);
    return ACL_ERROR_GE_INTERNAL_ERROR;
  }
  return SUCCESS;
}

Status TbeOpTask::UpdateRunInfo() {
  PROFILING_SCOPE(-1, profiling::kTiling);
  // invoke OpParaCalculate
  GELOGD("Start to invoke OpParaCalculate.");
  run_info_->ResetWorkspace();
  const auto ptr_size = sizeof(void *);
  void *const tiling_data_addr = ValueToPtr(PtrToValue(args_.get()) + (ptr_size * tiling_data_idx_));
  run_info_->ResetAddrBase(tiling_data_addr, max_tiling_size_);
  GE_CHK_STATUS_RET(CalcTilingInfo(), "[Calc][TilingInfo]failed.");

  block_dim_ = run_info_->GetBlockDim();
  tiling_key_ = run_info_->GetTilingKey();
  clear_atomic_ = run_info_->GetClearAtomic();
  const auto workspaces = run_info_->GetAllWorkspaces();
  op_desc_->SetWorkspaceBytes(workspaces);
  GE_CHK_STATUS_RET(AllocateWorkspaces(workspaces), "[Allocate][Workspaces] failed.");
  GELOGD("Invoking OpParaCalculate successfully. block_dim = %u, tiling_key = %lu.",
         block_dim_, tiling_key_);
  return SUCCESS;
}

Status TbeOpTask::UpdateNodeByShape(const std::vector<GeTensorDesc> &input_desc,
                                    const std::vector<GeTensorDesc> &output_desc) const {
  PROFILING_SCOPE(-1, profiling::kUpdateShape);
  const auto op_desc = node_->GetOpDesc();
  GE_CHECK_NOTNULL(op_desc);
  // Set runtime shape to node
  for (size_t i = 0UL; i < input_desc.size(); ++i) {
    const auto tensor_desc = op_desc->MutableInputDesc(static_cast<uint32_t>(i));
    GE_CHECK_NOTNULL(tensor_desc);
    tensor_desc->SetShape(input_desc[i].GetShape());
    tensor_desc->SetOriginShape(input_desc[i].GetOriginShape());
  }

  for (size_t i = 0UL; i < output_desc.size(); ++i) {
    const auto tensor_desc = op_desc->MutableOutputDesc(static_cast<uint32_t>(i));
    GE_CHECK_NOTNULL(tensor_desc);
    tensor_desc->SetShape(output_desc[i].GetShape());
    tensor_desc->SetOriginShape(output_desc[i].GetOriginShape());
  }

  return SUCCESS;
}

void TbeOpTask::EnableDynamicSupport(const NodePtr &node, const uint32_t max_tiling_size) {
  node_ = node;
  max_tiling_size_ = (max_tiling_size + sizeof(uintptr_t) - 1U) / sizeof(uintptr_t) * sizeof(uintptr_t);
  need_tiling_ = max_tiling_size > 0U;
}

Status TbeOpTask::AllocateWorkspaces(const std::vector<int64_t> &workspace_sizes) {
  PROFILING_SCOPE(-1, profiling::kAllocMem);
  static const std::string kPurpose("malloc workspace memory for dynamic op.");
  workspaces_.clear();
  if (workspace_sizes.empty()) {
    GELOGD("No need to allocate workspace.");
    return SUCCESS;
  }
  int64_t total_size = 0;
  std::vector<int64_t> ws_offsets;
  for (const auto ws_size : workspace_sizes) {
    // alignment and padding should be done in OpParaCalculate
    if (CheckInt64AddOverflow(total_size, ws_size) != SUCCESS) {
      return ACL_ERROR_GE_INTERNAL_ERROR;
    }
    ws_offsets.emplace_back(total_size);
    total_size += ws_size;
  }

  GELOGD("Total workspace size is %ld", total_size);
  GE_CHECK_NOTNULL(stream_resource_);
  const auto ws_base = stream_resource_->MallocMemory(kPurpose, static_cast<size_t>(total_size));
  if (ws_base == nullptr) {
    GELOGE(ACL_ERROR_GE_MEMORY_ALLOCATION, "[Malloc][Memory] failed, size: %ld", total_size);
    REPORT_INNER_ERROR("E19999", "MallocMemory failed, size: %ld", total_size);
    return ACL_ERROR_GE_MEMORY_ALLOCATION;
  }
  GELOGD("Done allocating workspace memory successfully.");

  for (const auto ws_offset : ws_offsets) {
    workspaces_.emplace_back(ValueToPtr(PtrToValue(ws_base) + static_cast<uint64_t>(ws_offset)));
  }

  return SUCCESS;
}

Status TbeOpTask::CheckAndExecuteAtomic(const std::vector<GeTensorDesc> &input_desc,
                                        const std::vector<DataBuffer> &input_buffers,
                                        std::vector<GeTensorDesc> &output_desc,
                                        std::vector<DataBuffer> &output_buffers,
                                        const rtStream_t stream) {
  (void)stream;
  if (clear_atomic_ && (atomic_task_ != nullptr)) {
    PROFILING_SCOPE(profiling::kAtomic, profiling::kOpExecute);
    atomic_task_->SetWorkSpaceAddr(workspaces_);
    return atomic_task_->LaunchKernel(input_desc, input_buffers, output_desc, output_buffers, stream);
  }
  return SUCCESS;
}

Status TbeOpTask::UpdateArgsItem(const std::vector<DataBuffer> &inputs,
                                 const std::vector<DataBuffer> &outputs) {
  size_t new_size = 0U;
  UpdateArgsItemOffset((input_num_ + output_num_) * sizeof(uintptr_t),
                       workspaces_.size() * sizeof(uintptr_t), new_size);
  GE_CHK_STATUS_RET(ExtendArgSizeIfNeed(new_size), "[Extend][ArgSizeIfNeed] failed.");
  UpdateOverflowAddr();
  GE_CHK_STATUS_RET(UpdateTilingArgs(), "[Update][TilingArgs] failed.");
  // workspace must be after tiling updating
  UpdateWorkspaceArgs();
  GE_CHK_STATUS_RET(UpdateHostMemInputArgs(inputs, outputs),
                    "[Update][Args] failed of %s.", node_->GetName().c_str());
  return SUCCESS;
}

void TbeOpTask::UpdateArgsItemOffset(const size_t io_size, const size_t workspace_addr_size, size_t &arg_size) {
  arg_size = io_size;
  args_item_offsets_.overflow_addr_offset = 0U;
  args_item_offsets_.workspace_addr_offset = 0U;
  args_item_offsets_.tiling_addr_offset = 0U;
  args_item_offsets_.tiling_data_offset = 0U;
  args_item_offsets_.host_input_data_offset = 0U;

  if (workspace_addr_size != 0U) {
    args_item_offsets_.workspace_addr_offset = arg_size;
    arg_size +=  workspace_addr_size;
  }
  if (need_tiling_) {
    args_item_offsets_.tiling_addr_offset = arg_size;
    arg_size += sizeof(void *);
    if (overflow_addr_ != nullptr) {
      args_item_offsets_.overflow_addr_offset = arg_size;
      arg_size += sizeof(void *);
    }
    args_item_offsets_.tiling_data_offset = arg_size;
    arg_size += max_tiling_size_;
  } else {
    if (overflow_addr_ != nullptr) {
      args_item_offsets_.overflow_addr_offset = arg_size;
      arg_size += sizeof(void *);
    }
  }

  if (extend_args_for_host_input_) {
    args_item_offsets_.host_input_data_offset = arg_size;
    arg_size += kMaxHostMemInputLen;
  }

  if (arg_size > io_size) {
    GELOGD("args size is extended frome %zu to %zu. overflow flag = %u, tiling flag = %u, host mem flag = %u,"
        " max tiling size = %u, workspace_addr_size = %zu, host_input_data_offset = %zu", io_size, arg_size,
        ((overflow_addr_ != nullptr) ? 1U : 0U), (need_tiling_ ? 1U : 0U), (extend_args_for_host_input_ ? 1U : 0U),
        max_tiling_size_, workspace_addr_size, args_item_offsets_.host_input_data_offset);
  }
}

Status TbeOpTask::ExtendArgSizeIfNeed(size_t new_size) {
  if (arg_size_ >= new_size) {
    return SUCCESS;
  }
  GELOGD("Need to reset size of args_ from %zu to %zu.", arg_size_, new_size);
  std::unique_ptr<uint8_t[]> args = MakeUnique<uint8_t[]>(new_size);
  GE_CHECK_NOTNULL(args);
  if (memcpy_s(args.get(), new_size, args_.get(), arg_size_) != EOK) {
    GELOGE(ACL_ERROR_GE_MEMORY_OPERATE_FAILED, "[Update][KernelArgs] failed for [%s].", node_->GetName().c_str());
    REPORT_INNER_ERROR("E19999", "update kernel args failed for %s.", node_->GetName().c_str());
    return ACL_ERROR_GE_MEMORY_OPERATE_FAILED;
  }

  args_ = std::move(args);
  arg_size_ = new_size;
  args_ex_.args = args_.get();
  args_ex_.argsSize = arg_size_;
  return SUCCESS;
}

void TbeOpTask::UpdateOverflowAddr() const {
  if (overflow_addr_ != nullptr) {
    uintptr_t *const addr = PtrToPtr<void, uintptr_t>(ValueToPtr(PtrToValue(args_ex_.args) +
                                                         args_item_offsets_.overflow_addr_offset));
    *addr = PtrToValue(overflow_addr_);
  }
}

void TbeOpTask::UpdateWorkspaceArgs() {
  if (workspaces_.empty()) {
    return;
  }
  void *const arg_base = args_ex_.args;
  uintptr_t *arg_workspace = PtrToPtr<void, uintptr_t>(ValueToPtr(PtrToValue(arg_base) +
                                                                  args_item_offsets_.workspace_addr_offset));
  for (size_t i = 0UL; i < workspaces_.size(); ++i) {
    *arg_workspace = PtrToValue(workspaces_[i]);
    arg_workspace++;
  }
}

Status TbeOpTask::UpdateTilingArgs() {
  PROFILING_SCOPE(-1, profiling::kKernelLaunchPrepare);
  args_ex_.hasTiling = need_tiling_;
  if (!need_tiling_) {
    return SUCCESS;
  }

  const size_t tiling_data_index = args_item_offsets_.tiling_data_offset / sizeof(void *);
  if (tiling_data_index > tiling_data_idx_) {
    GELOGD("[%s] Start to copy tiling info.", node_->GetName().c_str());
    uintptr_t *const tiling_arg = PtrToPtr<void, uintptr_t>(ValueToPtr(PtrToValue(args_.get()) +
                                                                 args_item_offsets_.tiling_addr_offset));
    void *const tiling_data = ValueToPtr(PtrToValue(args_.get()) + args_item_offsets_.tiling_data_offset);
    void *const tiling_data_old = ValueToPtr(PtrToValue(args_.get()) + (tiling_data_idx_ * sizeof(void *)));
    if (memmove_s(tiling_data, static_cast<size_t>(max_tiling_size_),
                  tiling_data_old, static_cast<size_t>(max_tiling_size_)) != EOK) {
      GELOGE(ACL_ERROR_GE_MEMORY_OPERATE_FAILED, "[Update][Args] failed of %s.", node_->GetName().c_str());
      REPORT_INNER_ERROR("E19999", "update kernel args failed of %s.", node_->GetName().c_str());
      return ACL_ERROR_GE_MEMORY_OPERATE_FAILED;
    }
    // must after tiling data copy
    *tiling_arg = PtrToValue(tiling_data);

    tiling_data_idx_ = tiling_data_index;
    args_ex_.tilingAddrOffset = static_cast<uint16_t>(args_item_offsets_.tiling_addr_offset);
    args_ex_.tilingDataOffset = static_cast<uint16_t>(args_item_offsets_.tiling_data_offset);
  }
  return SUCCESS;
}

Status TbeOpTask::SetArgIndex() {
  const std::vector<bool> v_is_input_const = op_desc_->GetIsInputConst();
  size_t input_index = 0UL;
  for (size_t i = 0UL; i < op_desc_->GetAllInputsSize(); ++i) {
    const GeTensorDescPtr tensor_desc = op_desc_->MutableInputDesc(static_cast<uint32_t>(i));
    if (tensor_desc == nullptr) {
      GELOGD("SingleOp: %s, Index: %zu, has no input", op_desc_->GetName().c_str(), i);
      continue;
    }
    if ((i < v_is_input_const.size()) && v_is_input_const[i]) {
      GELOGD("SingleOp: %s, Index: %zu, input is const", op_desc_->GetName().c_str(), i);
      input_index++;
      continue;
    }
    arg_index_.emplace_back(input_index);
    input_index++;
  }
  return SUCCESS;
}

Status TbeOpTask::UpdateHostMemInputArgs(const std::vector<DataBuffer> &inputs,
                                         const std::vector<DataBuffer> &outputs) {
  (void)outputs;
  args_ex_.hostInputInfoPtr = nullptr;
  args_ex_.hostInputInfoNum = 0U;
  if (!need_host_mem_opt_) {
    return SUCCESS;
  }

  vector<rtHostInputInfo_t> host_inputs;
  GE_CHK_RT_RET(ExecutorUtils::UpdateHostMemInputArgs(inputs, *this, args_.get(), arg_size_, host_inputs));
  host_inputs_info_ = MakeUnique<rtHostInputInfo_t[]>(host_inputs.size());
  GE_CHECK_NOTNULL(host_inputs_info_);
  size_t idx = 0U;
  for (auto &host_input : host_inputs) {
    host_inputs_info_[idx++] = host_input;
  }
  args_ex_.hostInputInfoPtr = host_inputs_info_.get();
  args_ex_.hostInputInfoNum = static_cast<uint16_t>(host_inputs.size());
  return SUCCESS;
}

Status TbeOpTask::UpdateIoAddr(const std::vector<DataBuffer> &inputs, const std::vector<DataBuffer> &outputs) {
  PROFILING_SCOPE(-1, profiling::kKernelLaunchPrepare);
  if (arg_index_.size() != inputs.size()) {
    GELOGE(ACL_ERROR_GE_PARAM_INVALID, "[Check][Size] Args size is %zu, but get input size is %zu.",
           arg_index_.size(), inputs.size());
    REPORT_INNER_ERROR("E19999", "[Check][Size] Args size is %zu, but get input size is %zu.",
                       arg_index_.size(), inputs.size());
    return ACL_ERROR_GE_PARAM_INVALID;
  }

  uintptr_t *const arg_base = PtrToPtr<uint8_t, uintptr_t>(args_.get());
  const auto ptr_size = sizeof(uintptr_t);
  for (size_t i = 0UL; i < arg_index_.size(); ++i) {
    uintptr_t *const arg_tiling_pre = PtrToPtr<void, uintptr_t>(ValueToPtr(PtrToValue(arg_base) +
                                                                           (ptr_size * arg_index_[i])));
      *arg_tiling_pre = PtrToValue(inputs[i].data);
  }

  uintptr_t *arg_output = PtrToPtr<void, uintptr_t>(ValueToPtr(PtrToValue(arg_base) +
                                                                (ptr_size * input_num_)));
  for (size_t i = 0UL; i < op_desc_->GetOutputsSize(); ++i) {
    *arg_output = PtrToValue(outputs[i].data);
    ++arg_output;
  }

  return SUCCESS;
}

Status TbeOpTask::LaunchKernel(const std::vector<GeTensorDesc> &input_desc,
                               const std::vector<DataBuffer> &input_buffers,
                               std::vector<GeTensorDesc> &output_desc,
                               std::vector<DataBuffer> &output_buffers,
                               const rtStream_t stream) {
  GELOGD("[%s] Start to launch kernel", node_->GetName().c_str());
  GE_CHK_STATUS_RET(UpdateIoAddr(input_buffers, output_buffers), "[Update][IoAddr] failed.");
  GE_CHK_STATUS_RET_NOLOG(UpdateNodeByShape(input_desc, output_desc));
  RECORD_PROFILING_START(kTiling);
  GE_CHK_STATUS_RET_NOLOG(UpdateRunInfo());
  RECORD_PROFILING_END(profiling::kTiling, kTiling);
  GE_CHK_STATUS_RET(CheckAndExecuteAtomic(input_desc, input_buffers, output_desc, output_buffers, stream),
                    "[Execute][AtomicTask] failed.");
  GE_CHK_STATUS_RET(UpdateArgsItem(input_buffers, output_buffers),
                    "[Update][ArgsItem] failed of %s", node_->GetName().c_str());

  GELOGD("[%s] Start to invoke rtKernelLaunch", node_->GetName().c_str());
  RECORD_PROFILING_START(kLaunchTask);
  GE_CHK_STATUS_RET(DoLaunchKernel(stream), "Failed to do launch kernel.");
  RECORD_PROFILING_END(profiling::kTiling, kLaunchTask);
  return SUCCESS;
}

Status TbeOpTask::DoLaunchKernel(const rtStream_t stream) {
  PROFILING_SCOPE(-1, profiling::kRtKernelLaunch);
  GE_CHK_RT_RET(static_cast<rtError_t>(DoLaunchKernelWithArgsEx(stream)));
  GE_CHK_STATUS_RET_NOLOG(GetTaskIdAndStreamId());
  return SUCCESS;
}

Status TbeOpTask::DoLaunchKernelWithArgsEx(const rtStream_t stream) {
  auto *const sm_desc = PtrToPtr<void, rtSmDesc_t>(sm_desc_);

  if (handle_ == nullptr) {
    SetTaskTag();
    GE_CHK_RT_RET(rtKernelLaunchWithFlag(stub_func_, block_dim_, &args_ex_, sm_desc, stream, 0U));
  } else {
    const std::string dev_func = std::to_string(tiling_key_);
    SetTaskTag();
    GE_CHK_RT_RET(rtKernelLaunchWithHandle(handle_, dev_func.c_str(), block_dim_, &args_ex_,
                                           sm_desc, stream, node_info_.c_str()));
  }
  return SUCCESS;
}

void TbeOpTask::GetIoAddr(uintptr_t *&arg_base, size_t &arg_count) {
  arg_base = PtrToPtr<uint8_t, uintptr_t>(args_.get());
  arg_count = (arg_size_ - max_tiling_size_) / sizeof(void *);  // for tiling data
  if (need_tiling_) {
    --arg_count;  // for tiling arg
  }
}

Status AtomicAddrCleanOpTask::UpdateNodeByShape(const std::vector<GeTensorDesc> &input_desc,
                                                const std::vector<GeTensorDesc> &output_desc) const {
  (void)input_desc;
  (void)output_desc;
  return SUCCESS;
}

const std::string AtomicAddrCleanOpTask::GetOpType() const {
  return kAtomicOpType;
}

Status AtomicAddrCleanOpTask::UpdateIoAddr(const std::vector<DataBuffer> &inputs,
                                           const std::vector<DataBuffer> &outputs) {
  (void)inputs;
  uintptr_t *arg_base = PtrToPtr<uint8_t, uintptr_t>(args_.get());
  for (const auto atomic_output_index : atomic_output_indices_) {
    if (atomic_output_index >= static_cast<int32_t>(outputs.size())) {
      GELOGE(ACL_ERROR_GE_PARAM_INVALID, "[Update][Args] failed, atomic index must smaller then data size.");
      REPORT_INNER_ERROR("E19999", "[Update][Args] failed, atomic index must smaller then data size.");
      return ACL_ERROR_GE_PARAM_INVALID;
    }
    auto &output_buffer = outputs[static_cast<size_t>(atomic_output_index)];
    *arg_base = PtrToValue(output_buffer.data);
    ++arg_base;

    const auto tensor_desc = op_desc_->MutableOutputDesc(static_cast<uint32_t>(atomic_output_index));
    int64_t size = 0;
    const graphStatus graph_status = TensorUtils::GetTensorMemorySizeInBytes(*tensor_desc, size);
    if (graph_status != GRAPH_SUCCESS) {
      REPORT_CALL_ERROR("E19999", "Get tensor size in bytes failed!");
      GELOGE(graph_status, "[Get][TensorMemorySize] In Bytes failed!");
      return FAILED;
    }
    TensorUtils::SetSize(*tensor_desc, size);
  }
  for (const auto atomic_ws_index : atomic_workspace_indices_) {
    if (atomic_ws_index >= static_cast<int32_t>(workspaces_.size())) {
      GELOGE(ACL_ERROR_GE_PARAM_INVALID,
             "[Update][Args] failed, workspace atomic index must smaller then workspace size.");
      REPORT_INNER_ERROR("E19999", "[Update][Args] failed, workspace atomic index must smaller then workspace size.");
      return ACL_ERROR_GE_PARAM_INVALID;
    }
    *arg_base = PtrToValue(workspaces_[static_cast<size_t>(atomic_ws_index)]);
    ++arg_base;
  }
  return SUCCESS;
}

Status AtomicAddrCleanOpTask::UpdateTilingArgs() {
  return SUCCESS;
}

Status AtomicAddrCleanOpTask::CalcTilingInfo() {
  const auto ret = optiling::OpAtomicCalculateV2(*node_, *run_info_);
  if (ret != GRAPH_SUCCESS) {
    GELOGE(ACL_ERROR_GE_INTERNAL_ERROR, "[Invoke][OpAtomicCalculate] failed, ret = %u.", ret);
    REPORT_INNER_ERROR("E19999", "invoke OpAtomicCalculate failed, ret = %u.", ret);
    return ACL_ERROR_GE_INTERNAL_ERROR;
  }
  return SUCCESS;
}

Status AtomicAddrCleanOpTask::InitAtomicAddrCleanIndices() {
  GELOGD("[%s] Start to setup AtomicAddrClean task.", op_desc_->GetName().c_str());
  std::vector<int64_t> atomic_output_indices;
  (void)ge::AttrUtils::GetListInt(op_desc_, ATOMIC_ATTR_OUTPUT_INDEX, atomic_output_indices);
  const std::map<std::string, std::map<int64_t, int64_t>> workspace_info =
      op_desc_->TryGetExtAttr(EXT_ATTR_ATOMIC_WORKSPACE_INFO, std::map<std::string, std::map<int64_t, int64_t>>());
  if (atomic_output_indices.empty() && workspace_info.empty()) {
    GELOGE(INTERNAL_ERROR, "[Check][Size][%s] atomic_output_indices and atomic_workspace_info must not be both empty.",
           op_desc_->GetName().c_str());
    REPORT_INNER_ERROR("E19999", "[%s] atomic_output_indices and atomic_workspace_info must not be both empty.",
                       op_desc_->GetName().c_str());
    return INTERNAL_ERROR;
  }

  for (const auto &iter : workspace_info) {
    for (const auto &info_iter : iter.second) {
      const auto workspace_index = info_iter.first;
      GELOGD("[%s] Adding workspace index [%ld]", op_desc_->GetName().c_str(), workspace_index);
      GE_CHECK_GE(workspace_index, 0);
      GE_CHECK_LE(workspace_index, INT32_MAX);
      atomic_workspace_indices_.emplace_back(static_cast<int32_t>(workspace_index));
    }
  }

  for (const auto output_index : atomic_output_indices) {
    GELOGD("[%s] Adding output index [%ld]", op_desc_->GetName().c_str(), output_index);
    GE_CHECK_GE(output_index, 0);
    GE_CHECK_LE(output_index, INT32_MAX);
    atomic_output_indices_.emplace_back(static_cast<int32_t>(output_index));
  }
  const size_t arg_count = atomic_workspace_indices_.size() + atomic_output_indices_.size();
  uintptr_t *arg_base = nullptr;
  size_t max_arg_size = 0U;
  GetIoAddr(arg_base, max_arg_size);
  if (arg_count > max_arg_size) {
    GELOGE(INTERNAL_ERROR, "[Check][Size][%s] atomic_output_indices invalid. atomic_output_indices size is %zu,"
           "arg size is %zu.", op_desc_->GetName().c_str(), arg_count, arg_size_);
    REPORT_INNER_ERROR("E19999", "[%s] atomic_output_indices invalid. atomic_output_indices size is %zu,"
                       "arg size is %zu.", op_desc_->GetName().c_str(), arg_count, arg_size_);
    return INTERNAL_ERROR;
  }
  return SUCCESS;
}

AiCpuBaseTask::~AiCpuBaseTask() {
  if (ext_info_addr_dev_ != nullptr) {
    (void)rtFree(ext_info_addr_dev_);
  }
  if (rt_event_ != nullptr) {
    (void)rtEventDestroy(rt_event_);
  }
  FreeHbm(copy_input_release_flag_dev_);
  FreeHbm(copy_input_data_size_dev_);
  FreeHbm(copy_input_src_dev_);
  FreeHbm(copy_input_dst_dev_);
  for (const auto summary : output_summary_) {
    FreeHbm(summary);
  }
  for (const auto out_shape : out_shape_hbm_) {
    FreeHbm(out_shape);
  }
}

Status AiCpuBaseTask::UpdateEventIdForBlockingAicpuOp() {
  bool is_support = false;
  if (CheckDeviceSupportBlockingAicpuOpProcess(is_support) != SUCCESS) {
    GELOGE(FAILED, "[Call][CheckDeviceSupportBlockingAicpuOpProcess] Call CheckDeviceSupportBlockingAicpuOpProcess failed");
    return FAILED;
  }
  if (!is_support) {
    GELOGD("Device not support blocking aicpu op process");
    return SUCCESS;
  }
  uint32_t event_id = 0U;
  auto rt_ret = rtEventCreateWithFlag(&rt_event_, RT_EVENT_WITH_FLAG);
  if (rt_ret != RT_ERROR_NONE) {
    REPORT_CALL_ERROR("E19999", "Call rtEventCreateWithFlag failed, ret:0x%X", rt_ret);
    GELOGE(RT_FAILED, "[Call][rtEventCreateWithFlag] failed, ret:0x%X", rt_ret);
    return RT_ERROR_TO_GE_STATUS(rt_ret);
  }
  rt_ret = rtGetEventID(rt_event_, &event_id);
  if (rt_ret != RT_ERROR_NONE) {
    REPORT_CALL_ERROR("E19999", "Call rtGetEventID failed, ret:0x%X", rt_ret);
    GELOGE(RT_FAILED, "[Call][rtGetEventID] failed, ret:0x%X", rt_ret);
    return RT_ERROR_TO_GE_STATUS(rt_ret);
  }
  if (aicpu_ext_handle_->UpdateEventId(event_id) != SUCCESS) {
    REPORT_CALL_ERROR("E19999", "Update event id=%u failed.", event_id);
    GELOGE(FAILED, "[Update][EventId] Update event id=%u failed", event_id);
    return FAILED;
  }
  GELOGI("Update event_id=%u success", event_id);
  return SUCCESS;
}

Status AiCpuBaseTask::SetExtInfoAndType(const std::string &kernel_ext_info, const uint64_t kernel_id) {
  if (kernel_ext_info.empty()) {
    GELOGI("Kernel_ext_info is empty, no need copy to device.");
    return SUCCESS;
  }

  int32_t unknown_shape_type_val = 0;
  (void)AttrUtils::GetInt(op_desc_, ::ge::ATTR_NAME_UNKNOWN_SHAPE_TYPE, unknown_shape_type_val);
  GELOGD("Get unknown_type is %d.", unknown_shape_type_val);
  unknown_type_ = static_cast<UnknowShapeOpType>(unknown_shape_type_val);

  (void)AttrUtils::GetBool(op_desc_, ATTR_NAME_IS_BLOCKING_OP, is_blocking_aicpu_op_);
  GELOGD("Get op:%s attribute(is_blocking_op), value:%d", op_desc_->GetName().c_str(),
         static_cast<int32_t>(is_blocking_aicpu_op_));

  aicpu_ext_handle_ = MakeUnique<::ge::hybrid::AicpuExtInfoHandler>(op_desc_->GetName(),
                                                                    num_inputs_,
                                                                    num_outputs_,
                                                                    unknown_type_);
  GE_CHK_BOOL_RET_STATUS(aicpu_ext_handle_ != nullptr, ACL_ERROR_GE_MEMORY_ALLOCATION,
                         "[Malloc][Memory] failed for aicpu_ext_handle!");

  const Status ret = aicpu_ext_handle_->Parse(kernel_ext_info);
  if (ret != SUCCESS) {
    GELOGE(ret, "[Parse][Param:kernel_ext_info] failed, kernel_ext_info_size=%zu.", kernel_ext_info.size());
    REPORT_INNER_ERROR("E19999",
        "Parse Param:kernel_ext_info failed, kernel_ext_info_size=%zu.", kernel_ext_info.size());
    return ret;
  }

  GE_CHK_STATUS_RET(aicpu_ext_handle_->UpdateSessionInfo(std::numeric_limits<uint64_t>::max(), kernel_id, false),
                    "[Update][SessionInfo] failed.");

  if (is_blocking_aicpu_op_) {
    if (UpdateEventIdForBlockingAicpuOp() != SUCCESS) {
      GELOGE(FAILED, "[Call][UpdateEventIdForBlockingAicpuOp] Call UpdateEventIdForBlockingAicpuOp failed");
      return FAILED;
    }
  }

  GE_CHK_RT_RET(rtMalloc(&ext_info_addr_dev_, aicpu_ext_handle_->GetExtInfoLen(), RT_MEMORY_HBM));
  GE_CHK_RT_RET(rtMemcpy(ext_info_addr_dev_, aicpu_ext_handle_->GetExtInfoLen(),
                         aicpu_ext_handle_->GetExtInfo(), aicpu_ext_handle_->GetExtInfoLen(),
                         RT_MEMCPY_HOST_TO_DEVICE));
  return SUCCESS;
}

Status AiCpuBaseTask::SetInputConst() {
  input_is_const_.clear();
  const std::vector<bool> v_is_input_const = op_desc_->GetIsInputConst();
  for (size_t i = 0U; i < op_desc_->GetAllInputsSize(); ++i) {
    const GeTensorDescPtr tensor_desc = op_desc_->MutableInputDesc(static_cast<uint32_t>(i));
    if (tensor_desc == nullptr) {
      GELOGD("SingleOp: %s, Index: %zu, has no input", op_desc_->GetName().c_str(), i);
      continue;
    }
    if ((i < v_is_input_const.size()) && v_is_input_const[i]) {
      GELOGD("SingleOp: %s, Index: %zu, input is const", op_desc_->GetName().c_str(), i);
      input_is_const_.push_back(true);
      continue;
    }
    input_is_const_.push_back(false);
  }
  return SUCCESS;
}

Status AiCpuBaseTask::UpdateExtInfo(const std::vector<GeTensorDesc> &input_desc,
                                    const std::vector<GeTensorDesc> &output_desc,
                                    const rtStream_t stream) {
  GELOGI("Update ext info begin, unknown_type=%d.", unknown_type_);
  GE_CHECK_NOTNULL(aicpu_ext_handle_);
  GE_CHK_STATUS_RET(aicpu_ext_handle_->UpdateExecuteMode(false), "[Update][ExecuteMode] failed.");

  if ((num_inputs_ == 0U) && (num_outputs_ == 0U)) {
    GELOGI("No input and output, no need update ext info.");
    return SUCCESS;
  }

  size_t non_const_index = 0U;
  for (size_t input_index = 0U; input_index < num_inputs_; input_index++) {
    if ((input_index < input_is_const_.size()) && input_is_const_[input_index]) {
      // get input_desc from op_desc if const input, num_inputs_ is op_desc_ input_size
      const auto const_input_desc = op_desc_->MutableInputDesc(static_cast<uint32_t>(input_index));
      GE_CHECK_NOTNULL(const_input_desc);
      GE_CHK_STATUS_RET(aicpu_ext_handle_->UpdateInputShapeAndType(
          static_cast<uint32_t>(input_index), *const_input_desc),
          "[Update][InputShapeAndType] failed, input_index:%zu.", input_index);
      continue;
    }
    GE_CHK_BOOL_RET_STATUS(non_const_index < input_desc.size(), ACL_ERROR_GE_PARAM_INVALID,
        "[Check][Size]Input_desc size is %zu, but get non_const_index is %zu", input_desc.size(), non_const_index);
    GE_CHK_STATUS_RET(aicpu_ext_handle_->UpdateInputShapeAndType(static_cast<uint32_t>(input_index),
        input_desc[non_const_index]), "[Update][InputShapeAndType]failed, input_index:%zu.", input_index);
    if (DumpManager::GetInstance().GetDumpProperties(kInferSessionId).IsSingleOpNeedDump()) {
      GE_CHK_STATUS_RET(op_desc_->UpdateInputDesc(static_cast<uint32_t>(input_index),
          input_desc[non_const_index]), "AiCpuTask Update [%zu]th input desc failed.", input_index);
    }
    non_const_index++;
  }

  if (unknown_type_ != DEPEND_COMPUTE) {
    for (size_t j = 0U; j < num_outputs_; ++j) {
      GE_CHK_STATUS_RET(aicpu_ext_handle_->UpdateOutputShapeAndType(static_cast<uint32_t>(j),
          output_desc[j]), "[Update][OutputShapeAndType] failed, Output:%zu.", j);
      if (DumpManager::GetInstance().GetDumpProperties(kInferSessionId).IsSingleOpNeedDump()) {
        GE_CHK_STATUS_RET(op_desc_->UpdateOutputDesc(static_cast<uint32_t>(j), output_desc[j]),
                          "AiCpuTask Update [%zu]th output desc failed.", j);
    }
    }
  }

  GE_CHK_RT_RET(rtMemcpyAsync(ext_info_addr_dev_,
                              aicpu_ext_handle_->GetExtInfoLen(), // check size
                              aicpu_ext_handle_->GetExtInfo(),
                              aicpu_ext_handle_->GetExtInfoLen(),
                              RT_MEMCPY_HOST_TO_DEVICE_EX,
                              stream));

  GELOGI("Update ext info end.");
  return SUCCESS;
}

Status AiCpuBaseTask::UpdateOutputShape(std::vector<GeTensorDesc> &output_desc) {
  if (num_outputs_ == 0U) {
    GELOGD("AiCpuBaseTask output_num is 0, no need update output shape.");
    return SUCCESS;
  }
  GELOGD("Start to update DEPEND_SHAPE_RANGE AiCpuBaseTask outputshape.");

  GE_CHK_RT_RET(rtMemcpy(aicpu_ext_handle_->GetExtInfo(), aicpu_ext_handle_->GetExtInfoLen(), ext_info_addr_dev_,
                         aicpu_ext_handle_->GetExtInfoLen(), RT_MEMCPY_DEVICE_TO_HOST));

  for (size_t i = 0U; i < num_outputs_; ++i) {
    GeShape shape;
    DataType data_type;
    (void)aicpu_ext_handle_->GetOutputShapeAndType(static_cast<uint32_t>(i), shape, data_type);
    GE_CHK_STATUS_RET(UpdateShapeToOutputDesc(shape, output_desc[i]),
        "[Update][ShapeToOutputDesc] failed, output:%zu. datatype:%d.", i, data_type);
    if (DumpManager::GetInstance().GetDumpProperties(kInferSessionId).IsSingleOpNeedDump()) {
      GE_CHK_STATUS_RET(op_desc_->UpdateOutputDesc(static_cast<uint32_t>(i), output_desc[i]),
                        "[Update][OutputDesc] failed, output:%zu.", i);
    }
  }
  GELOGD("Update DEPEND_SHAPE_RANGE AiCpuBaseTask outputshape finished.");
  return SUCCESS;
}

Status AiCpuBaseTask::UpdateShapeToOutputDesc(const GeShape &shape_new, GeTensorDesc &output_desc) const {
  const auto shape_old = output_desc.GetShape();
  output_desc.SetShape(shape_new);
  GELOGD("Update AiCpuBaseTask shape from %s to %s", shape_old.ToString().c_str(), shape_new.ToString().c_str());

  const auto origin_shape_old = output_desc.GetOriginShape();
  const auto origin_format = output_desc.GetOriginFormat();
  const auto format = output_desc.GetFormat();
  if (origin_format == format) {
    output_desc.SetOriginShape(shape_new);
    return SUCCESS;
  }

  std::vector<int64_t> origin_dims_new;
  const auto trans_ret = formats::TransTensorShape(format, shape_new.GetDims(),
                                                   output_desc.GetDataType(), origin_format, origin_dims_new);
  GE_CHK_STATUS_RET(trans_ret,
                    "[Trans][Shape] failed, AiCpuTask originFormat[%d] is not same as format[%d], shape=%s.",
                    origin_format, format, shape_new.ToString().c_str());

  const auto origin_shape_new = GeShape(origin_dims_new);
  output_desc.SetOriginShape(origin_shape_new);
  GELOGD("AiCpuTask originFormat[%d] is not same as format[%d], need update from %s ro %s.",
         origin_format, format, origin_shape_old.ToString().c_str(), origin_shape_new.ToString().c_str());
  return SUCCESS;
}

Status AiCpuBaseTask::UpdateIoAddr(const std::vector<DataBuffer> &inputs, const std::vector<DataBuffer> &outputs) {
  uintptr_t *arg_base = nullptr;
  size_t arg_num = 0U;
  GetIoAddr(arg_base, arg_num);

  // input number and output number was check in ValidateParams
  size_t non_const_index = 0U;
  for (size_t input_index = 0U; input_index < num_inputs_; input_index++) {
    if ((input_index < input_is_const_.size()) && input_is_const_[input_index]) {
      // const input no need update addr
      GE_CHECK_NOTNULL(arg_base);
      GELOGD("AICpuTask input[%zu] addr = %lu", input_index, *arg_base);
      arg_base++;
      continue;
    }
    GE_CHK_BOOL_RET_STATUS(non_const_index < inputs.size(), ACL_ERROR_GE_PARAM_INVALID,
        "[Check][Size] Input size is %zu, but get non_const_index is %zu", inputs.size(), non_const_index);
    const auto addr = inputs[non_const_index].data;
    const uint64_t length = inputs[non_const_index].length;
    if ((length != 0U) && (addr == nullptr)) {
      GELOGE(PARAM_INVALID, "[Check][Addr]AiCpuTask input[%zu] addr is nullptr, length = %lu", input_index, length);
      return PARAM_INVALID;
    }
    GELOGD("AICpuTask input[%zu] addr = %p, length = %lu.", input_index, addr, length);
    *arg_base = PtrToValue(addr);
    ++arg_base;
    non_const_index++;
  }

  for (size_t i = 0U; i < outputs.size(); ++i) {
    const auto addr = outputs[i].data;
    const uint64_t length = outputs[i].length;
    if ((length != 0U) && (addr == nullptr)) {
      GELOGE(PARAM_INVALID, "[Check][Addr]AiCpuTask output[%zu] addr is nullptr, length = %lu", i, length);
      return PARAM_INVALID;
    }
    GELOGD("AICpuTask output[%zu] addr = %p, length = %lu.", i, addr, length);
    *arg_base = PtrToValue(addr);
    ++arg_base;
  }
  GE_CHK_STATUS_RET(UpdateHostMemInputArgs(inputs, outputs),
                    "[Update][Args] failed of %s.", op_desc_->GetName().c_str());
  return SUCCESS;
}

Status AiCpuBaseTask::CheckDeviceSupportBlockingAicpuOpProcess(bool &is_support) const {
  int32_t device_id = 0;
  auto rt_ret = rtGetDevice(&device_id);
  if (rt_ret != RT_ERROR_NONE) {
    REPORT_CALL_ERROR("E19999", "Call rtGetDevice failed, ret:0x%X", rt_ret);
    GELOGE(RT_FAILED, "[Call][rtGetDevice] failed, ret:0x%X", rt_ret);
    return RT_ERROR_TO_GE_STATUS(rt_ret);
  }
  int32_t value = 0;
  rt_ret = rtGetDeviceCapability(device_id, FEATURE_TYPE_BLOCKING_OPERATOR, RT_MODULE_TYPE_AICPU, &value);
  if (rt_ret != RT_ERROR_NONE) {
    REPORT_CALL_ERROR("E19999", "Call rtGetDeviceCapability failed, ret:0x%X", rt_ret);
    GELOGE(RT_FAILED, "[Call][rtGetDeviceCapability] failed, ret:0x%X", rt_ret);
    return RT_ERROR_TO_GE_STATUS(rt_ret);
  }
  if ((value != RT_AICPU_BLOCKING_OP_NOT_SUPPORT) && (value != RT_AICPU_BLOCKING_OP_SUPPORT)) {
    REPORT_INNER_ERROR("E19999", "Value should be %d or %d but %d",
                       RT_AICPU_BLOCKING_OP_NOT_SUPPORT, RT_AICPU_BLOCKING_OP_SUPPORT, value);
    GELOGE(FAILED, "[Check][Value] Value should be %d or %d but %d",
           RT_AICPU_BLOCKING_OP_NOT_SUPPORT, RT_AICPU_BLOCKING_OP_SUPPORT, value);
    return FAILED;
  }
  is_support = (value == RT_AICPU_BLOCKING_OP_SUPPORT);
  return SUCCESS;
}

Status AiCpuBaseTask::DistributeWaitTaskForAicpuBlockingOp(const rtStream_t stream) {
  bool is_support = false;
  if (CheckDeviceSupportBlockingAicpuOpProcess(is_support) != SUCCESS) {
    GELOGE(FAILED, "[Call][CheckDeviceSupportBlockingAicpuOpProcess] Call CheckDeviceSupportBlockingAicpuOpProcess failed");
    return FAILED;
  }
  if (!is_support) {
    GELOGD("Device not support blocking aicpu op process.");
    return SUCCESS;
  }
  GELOGI("Distribute queue task begin");
  if (rt_event_ == nullptr) {
    REPORT_INNER_ERROR("E19999", "rt_event_ is nullptr");
    GELOGE(FAILED, "[Check][rt_event_] rt_event_ is nullptr");
    return FAILED;
  }
  SetTaskTag();
  auto rt_ret = rtStreamWaitEvent(stream, rt_event_);
  if (rt_ret != RT_ERROR_NONE) {
    REPORT_CALL_ERROR("E19999", "Call rtStreamWaitEvent failed, ret:0x%X", rt_ret);
    GELOGE(RT_FAILED, "[Call][RtApi] failed, ret:0x%X", rt_ret);
    return RT_ERROR_TO_GE_STATUS(rt_ret);
  }
  SetTaskTag();
  rt_ret = rtEventReset(rt_event_, stream);
  if (rt_ret != RT_ERROR_NONE) {
    REPORT_CALL_ERROR("E19999", "Call rtEventReset failed, ret:0x%X", rt_ret);
    GELOGE(RT_FAILED, "[Call][RtApi] failed, ret:0x%X", rt_ret);
    return RT_ERROR_TO_GE_STATUS(rt_ret);
  }
  return SUCCESS;
}

AiCpuTask::~AiCpuTask() {
  FreeHbm(args_);
  FreeHbm(io_addr_);
  FreeHbm(workspace_addr_);
  FreeHbm(copy_workspace_buf_);
  FreeHbm(copy_ioaddr_dev_);
  FreeHbm(copy_task_args_buf_);
}

Status AiCpuTask::UpdateHostMemInputArgs(const std::vector<DataBuffer> &inputs,
                                         const std::vector<DataBuffer> &outputs) {
  (void)outputs;
  if (!need_host_mem_opt_) {
    return SUCCESS;
  }
  vector<rtHostInputInfo_t> host_inputs;
  GE_CHK_RT_RET(ExecutorUtils::UpdateHostMemInputArgs(inputs, *this, io_addr_host_.data(),
                                                      io_addr_host_.size() * sizeof(void *), host_inputs));
  for (auto &host_input : host_inputs) {
    const size_t index = host_input.addrOffset / sizeof(void *);
    io_addr_host_[index] = ValueToPtr(PtrToValue(io_addr_) + host_input.dataOffset);
  }
  return SUCCESS;
}

Status AiCpuTask::LaunchKernel(const rtStream_t stream) {
  GELOGD("Start to launch kernel. task = %s", this->op_type_.c_str());
  SetTaskTag();
  auto ret = rtMemcpyAsync(io_addr_,
                           io_addr_size_,
                           io_addr_host_.data(),
                           io_addr_host_.size() * sizeof(void *),
                           RT_MEMCPY_HOST_TO_DEVICE_EX,
                           stream);
  if (ret != RT_ERROR_NONE) {
    GELOGE(FAILED, "[MemcpyAsync][Date] failed. ret = %d, task = %s", ret, this->op_type_.c_str());
    REPORT_CALL_ERROR("E19999", "rtMemcpyAsync data failed, ret = %d, task = %s", ret, this->op_type_.c_str());
    return RT_ERROR_TO_GE_STATUS(ret);
  }

  GELOGI("To invoke rtKernelLaunchEx. task = %s", this->op_type_.c_str());
  SetTaskTag();
  ret = rtKernelLaunchEx(args_, static_cast<uint32_t>(arg_size_), 0U, stream);
  if (ret != RT_ERROR_NONE) {
    GELOGE(FAILED, "[Invoke][rtKernelLaunch] failed. ret = %d, task = %s", ret, this->op_type_.c_str());
    REPORT_CALL_ERROR("E19999", "invoke rtKernelLaunchEx failed, ret = %d, task = %s", ret, this->op_type_.c_str());
    return RT_ERROR_TO_GE_STATUS(ret);
  }
  GE_CHK_STATUS_RET_NOLOG(GetTaskIdAndStreamId());
  GELOGI("[TASK_INFO] %lu/%s", kernel_id_, op_type_.c_str());

  GELOGD("Done launch kernel successfully. task = %s", this->op_type_.c_str());

  if (is_blocking_aicpu_op_) {
    if (DistributeWaitTaskForAicpuBlockingOp(stream) != SUCCESS) {
      GELOGE(FAILED, "[Call][DistributeWaitTaskForAicpuBlockingOp] Call DistributeWaitTaskForAicpuBlockingOp failed");
      return FAILED;
    }
  }

  return SUCCESS;
}

Status AiCpuBaseTask::PrepareCopyInputs(const std::vector<DataBuffer> &outputs) {
  std::vector<uint64_t> copy_input_release_flag;
  std::vector<uint64_t> copy_input_data_size;
  std::vector<uint64_t> copy_input_src;
  std::vector<uint64_t> copy_input_dst;

  for (size_t i = 0U; i < num_outputs_; ++i) {
    const auto &summary = output_summary_host_[i];
    GELOGI("Node out[%zu] summary, shape data=0x%lx, shape data size=%lu, raw data=0x%lx, raw data size=%lu.",
           i, summary.shape_data_ptr, summary.shape_data_size,
           summary.raw_data_ptr, summary.raw_data_size);
    const auto output = outputs[i];
    copy_input_release_flag.emplace_back(kReleaseFlag);
    if (summary.raw_data_size != 0U) {
      copy_input_data_size.emplace_back(output.length);
    } else {
      copy_input_data_size.emplace_back(summary.raw_data_size);
    }
    copy_input_src.emplace_back(summary.raw_data_ptr);
    copy_input_dst.emplace_back(PtrToValue(output.data));

    const auto &shape_buffer = out_shape_hbm_[i];
    copy_input_release_flag.emplace_back(kReleaseFlag);
    copy_input_data_size.emplace_back(summary.shape_data_size);
    copy_input_src.emplace_back(summary.shape_data_ptr);
    copy_input_dst.emplace_back(PtrToValue(shape_buffer));
  }

  const size_t copy_input_buf_len = num_outputs_ * kCopyNum * sizeof(uint64_t);

  GE_CHK_RT_RET(rtMemcpy(copy_input_release_flag_dev_, copy_input_buf_len,
                         copy_input_release_flag.data(), copy_input_buf_len, RT_MEMCPY_HOST_TO_DEVICE));
  GE_CHK_RT_RET(rtMemcpy(copy_input_data_size_dev_, copy_input_buf_len,
                         copy_input_data_size.data(), copy_input_buf_len, RT_MEMCPY_HOST_TO_DEVICE));
  GE_CHK_RT_RET(rtMemcpy(copy_input_src_dev_, copy_input_buf_len,
                         copy_input_src.data(), copy_input_buf_len, RT_MEMCPY_HOST_TO_DEVICE));
  GE_CHK_RT_RET(rtMemcpy(copy_input_dst_dev_, copy_input_buf_len,
                         copy_input_dst.data(), copy_input_buf_len, RT_MEMCPY_HOST_TO_DEVICE));
  return SUCCESS;
}

Status AiCpuBaseTask::ReadResultSummaryAndPrepareMemory() {
  for (size_t i = 0U; i < num_outputs_; ++i) {
    auto &result_summary = output_summary_host_[i];

    GE_CHK_RT_RET(rtMemcpy(&result_summary, sizeof(aicpu::FWKAdapter::ResultSummary),
                           output_summary_[i], sizeof(aicpu::FWKAdapter::ResultSummary),
                           RT_MEMCPY_DEVICE_TO_HOST));
    const auto shape_data_size = result_summary.shape_data_size;
    void *shape_buffer = nullptr;
    if (shape_data_size > 0U) {
      GE_CHK_RT_RET(rtMalloc(&shape_buffer, shape_data_size, RT_MEMORY_HBM));
    }
    out_shape_hbm_.emplace_back(shape_buffer);
  }
  return SUCCESS;
}

Status AiCpuCCTask::CopyDataToHbm(std::vector<DataBuffer> &outputs,
                                  const rtStream_t stream) {
  GE_CHK_STATUS_RET_NOLOG(PrepareCopyInputs(outputs));
  rtArgsEx_t args_ex = {};
  args_ex.args = memcpy_args_.get();
  args_ex.argsSize = memcpy_args_size_;
  const auto ret = rtCpuKernelLaunchWithFlag(static_cast<const void *>(memcpy_so_name_.data()),
                                             static_cast<const void *>(memcpy_kernel_name_.data()),
                                             block_dim_, &args_ex,
                                             nullptr, stream, RT_KERNEL_DEFAULT);
  GE_CHK_RT_RET(ret);
  GE_CHK_RT_RET(rtStreamSynchronize(stream));
  return SUCCESS;
}

Status AiCpuTask::CopyDataToHbm(std::vector<DataBuffer> &outputs,
                                const rtStream_t stream) {
  GE_CHK_STATUS_RET_NOLOG(PrepareCopyInputs(outputs));

  GE_CHK_RT_RET(rtKernelLaunchEx(copy_task_args_buf_, sizeof(STR_FWK_OP_KERNEL),
                                 RT_KERNEL_DEFAULT, stream));
  GE_CHK_RT_RET(rtStreamSynchronize(stream));
  return SUCCESS;
}

Status AiCpuBaseTask::UpdateShapeByHbmBuffer(std::vector<GeTensorDesc> &output_desc) {
  for (size_t i = 0U; i < num_outputs_; ++i) {
    const auto &result_summary = output_summary_host_[i];
    std::vector<int64_t> shape_dims;
    if (result_summary.shape_data_size > 0U) {
      const auto &shape_hbm = out_shape_hbm_[i];

      const uint32_t dim_num = static_cast<uint32_t>(result_summary.shape_data_size / sizeof(int64_t));
      const std::unique_ptr<int64_t[]> shape_addr = MakeUnique<int64_t[]>(static_cast<size_t>(dim_num));
      GE_CHECK_NOTNULL(shape_addr);
      GE_CHK_RT_RET(rtMemcpy(shape_addr.get(), result_summary.shape_data_size, shape_hbm,
                             result_summary.shape_data_size, RT_MEMCPY_DEVICE_TO_HOST));

      for (size_t dim_idx = 0U; dim_idx < dim_num; ++dim_idx) {
        shape_dims.emplace_back(shape_addr[dim_idx]);
        GELOGD("Node [%zu]th output dim[%zu]=%ld.", i, dim_idx, shape_addr[dim_idx]);
      }
    }

    GE_CHK_STATUS_RET(UpdateShapeToOutputDesc(GeShape(shape_dims), output_desc[i]),
        "[Update][ShapeToOutputDesc] failed , output:%zu.", i);
    if (DumpManager::GetInstance().GetDumpProperties(kInferSessionId).IsSingleOpNeedDump()) {
      GE_CHK_STATUS_RET(op_desc_->UpdateOutputDesc(static_cast<uint32_t>(i), output_desc[i]),
          "[Update][OutputDesc] failed, output:%zu.", i);
    }
  }
  return SUCCESS;
}


Status AiCpuBaseTask::UpdateShapeAndDataByResultSummary(std::vector<GeTensorDesc> &output_desc,
                                                        std::vector<DataBuffer> &outputs,
                                                        const rtStream_t stream) {
  if (num_outputs_ == 0U) {
    GELOGI("Output num is 0, there is no need to update the output and size.");
    return SUCCESS;
  }

  GELOGI("Update shape and data by result summary begin.");

  for (const auto out_shape : out_shape_hbm_) {
    FreeHbm(out_shape);
  }
  out_shape_hbm_.clear();
  GE_CHK_STATUS_RET(ReadResultSummaryAndPrepareMemory(),
                    "[Read][ResultSummaryAndPrepareMemory] failed.");

  GE_CHK_STATUS_RET(CopyDataToHbm(outputs, stream),
                    "[Copy][DataToHbm] failed.");

  GE_CHK_STATUS_RET(UpdateShapeByHbmBuffer(output_desc),
                    "[Update][ShapeByHbmBuffer] failed.");

  for (const auto out_shape : out_shape_hbm_) {
    FreeHbm(out_shape);
  }
  out_shape_hbm_.clear();

  GELOGI("Update shape and data by result summary end.");
  return SUCCESS;
}

Status AiCpuTask::InitForSummaryAndCopy() {
  if ((unknown_type_ != DEPEND_COMPUTE) || (num_outputs_ == 0U)) {
    GELOGI("Unknown_type is %d, output num is %zu.", unknown_type_, num_outputs_);
    return SUCCESS;
  }

  output_summary_.resize(num_outputs_);
  for (size_t i = 0U; i < num_outputs_; ++i) {
    constexpr auto result_summary_size = sizeof(aicpu::FWKAdapter::ResultSummary);
    GE_CHK_RT_RET(rtMalloc(&output_summary_[i], result_summary_size, RT_MEMORY_HBM));
  }
  output_summary_host_.resize(num_outputs_);

  const size_t copy_input_buf_len = num_outputs_ * kCopyNum * sizeof(uint64_t);

  GE_CHK_RT_RET(rtMalloc(&copy_input_release_flag_dev_, copy_input_buf_len, RT_MEMORY_HBM));
  GE_CHK_RT_RET(rtMalloc(&copy_input_data_size_dev_, copy_input_buf_len, RT_MEMORY_HBM));
  GE_CHK_RT_RET(rtMalloc(&copy_input_src_dev_, copy_input_buf_len, RT_MEMORY_HBM));
  GE_CHK_RT_RET(rtMalloc(&copy_input_dst_dev_, copy_input_buf_len, RT_MEMORY_HBM));

  GE_CHK_RT_RET(rtMalloc(&copy_task_args_buf_, sizeof(STR_FWK_OP_KERNEL), RT_MEMORY_HBM));

  std::vector<uint64_t> copy_io_addr;
  copy_io_addr.emplace_back(PtrToValue(copy_input_release_flag_dev_));
  copy_io_addr.emplace_back(PtrToValue(copy_input_data_size_dev_));
  copy_io_addr.emplace_back(PtrToValue(copy_input_src_dev_));
  copy_io_addr.emplace_back(PtrToValue(copy_input_dst_dev_));

  const auto copy_io_addr_size = sizeof(uint64_t) * copy_io_addr.size();

  GE_CHK_RT_RET(rtMalloc(&copy_ioaddr_dev_, copy_io_addr_size, RT_MEMORY_HBM));

  GE_CHK_RT_RET(rtMemcpy(copy_ioaddr_dev_, copy_io_addr_size,
                         copy_io_addr.data(), copy_io_addr_size, RT_MEMCPY_HOST_TO_DEVICE));
  return SUCCESS;
}

Status AiCpuTask::SetMemCopyTask(const domi::KernelExDef &kernel_def) {
  if (kernel_def.args_size() > sizeof(STR_FWK_OP_KERNEL)) {
    GELOGE(ACL_ERROR_GE_PARAM_INVALID, "[Check][Size]sizeof STR_FWK_OP_KERNEL is: %lu, but args_size is: %d",
        sizeof(STR_FWK_OP_KERNEL), kernel_def.args_size());
    REPORT_INNER_ERROR("E19999", "[sizeof STR_FWK_OP_KERNEL is: %lu, but args_size is: %d",
        sizeof(STR_FWK_OP_KERNEL), kernel_def.args_size());
    return ACL_ERROR_GE_PARAM_INVALID;
  }
  GE_CHK_RT_RET(rtMalloc(&copy_workspace_buf_, static_cast<uint64_t>(kernel_def.task_info_size()),
                         RT_MEMORY_HBM));
  GE_CHK_RT_RET(rtMemcpy(copy_workspace_buf_, static_cast<uint64_t>(kernel_def.task_info_size()),
                         kernel_def.task_info().data(), static_cast<uint64_t>(kernel_def.task_info_size()),
                         RT_MEMCPY_HOST_TO_DEVICE));

  STR_FWK_OP_KERNEL aicpu_task = {};
  const auto sec_ret = memcpy_s(&aicpu_task, sizeof(STR_FWK_OP_KERNEL),
                                kernel_def.args().data(), kernel_def.args().size());
  if (sec_ret != EOK) {
    GELOGE(ACL_ERROR_GE_MEMORY_OPERATE_FAILED, "[Update][TaskArgs] failed, ret: %d", sec_ret);
    REPORT_INNER_ERROR("E19999", "update STR_FWK_OP_KERNEL args failed because memcpy_s return %d.", sec_ret);
    return ACL_ERROR_GE_MEMORY_OPERATE_FAILED;
  }

  aicpu_task.fwkKernelBase.fwk_kernel.inputOutputAddr = PtrToValue(copy_ioaddr_dev_);
  aicpu_task.fwkKernelBase.fwk_kernel.workspaceBaseAddr = PtrToValue(copy_workspace_buf_);
  aicpu_task.fwkKernelBase.fwk_kernel.extInfoAddr = 0U;
  aicpu_task.fwkKernelBase.fwk_kernel.extInfoLen = 0U;

  GE_CHK_RT_RET(rtMemcpy(copy_task_args_buf_, sizeof(STR_FWK_OP_KERNEL),
                         &aicpu_task, sizeof(STR_FWK_OP_KERNEL), RT_MEMCPY_HOST_TO_DEVICE));
  return SUCCESS;
}

Status AiCpuTask::LaunchKernel(const std::vector<GeTensorDesc> &input_desc,
                               const std::vector<DataBuffer> &input_buffers,
                               std::vector<GeTensorDesc> &output_desc,
                               std::vector<DataBuffer> &output_buffers,
                               const rtStream_t stream) {
  GE_CHK_STATUS_RET_NOLOG(UpdateExtInfo(input_desc, output_desc, stream));
  if (unknown_type_ == DEPEND_COMPUTE) {
    std::vector<DataBuffer> summary_buffers;
    for (size_t i = 0U; i < num_outputs_; ++i) {
      summary_buffers.emplace_back(output_summary_[i], sizeof(aicpu::FWKAdapter::ResultSummary), false);
    }
    GE_CHK_STATUS_RET_NOLOG(UpdateIoAddr(input_buffers, summary_buffers));
  } else {
    GE_CHK_STATUS_RET_NOLOG(UpdateIoAddr(input_buffers, output_buffers));
  }

  RECORD_PROFILING_START(kLaunchTask);
  GE_CHK_STATUS_RET_NOLOG(LaunchKernel(stream));
  RECORD_PROFILING_END(profiling::kLaunchTask, kLaunchTask);
  if (unknown_type_ == DEPEND_SHAPE_RANGE) {
    GE_CHK_RT_RET(rtStreamSynchronize(stream));
    GE_CHK_STATUS_RET_NOLOG(UpdateOutputShape(output_desc));
  } else if (unknown_type_ == DEPEND_COMPUTE) {
    GE_CHK_RT_RET(rtStreamSynchronize(stream));
    GE_CHK_STATUS_RET_NOLOG(UpdateShapeAndDataByResultSummary(output_desc, output_buffers, stream));
  } else {
    // something else
  }

  return SUCCESS;
}

Status AiCpuCCTask::LaunchKernel(const std::vector<GeTensorDesc> &input_desc,
                                 const std::vector<DataBuffer> &input_buffers,
                                 std::vector<GeTensorDesc> &output_desc,
                                 std::vector<DataBuffer> &output_buffers,
                                 const rtStream_t stream) {
  GE_CHK_STATUS_RET_NOLOG(UpdateExtInfo(input_desc, output_desc, stream));
  if (unknown_type_ == DEPEND_COMPUTE) {
    std::vector<DataBuffer> summary_buffers;
    for (size_t i = 0U; i < num_outputs_; ++i) {
      summary_buffers.emplace_back(output_summary_[i], sizeof(aicpu::FWKAdapter::ResultSummary), false);
    }
    GE_CHK_STATUS_RET_NOLOG(UpdateIoAddr(input_buffers, summary_buffers));
  } else {
    GE_CHK_STATUS_RET_NOLOG(UpdateIoAddr(input_buffers, output_buffers));
  }

  GE_CHK_STATUS_RET_NOLOG(LaunchKernel(stream));
  if (unknown_type_ == DEPEND_SHAPE_RANGE) {
    GE_CHK_RT_RET(rtStreamSynchronize(stream));
    GE_CHK_STATUS_RET_NOLOG(UpdateOutputShape(output_desc));
  } else if (unknown_type_ == DEPEND_COMPUTE) {
    GE_CHK_RT_RET(rtStreamSynchronize(stream));
    GE_CHK_STATUS_RET_NOLOG(UpdateShapeAndDataByResultSummary(output_desc, output_buffers, stream));
  } else {
    // something else
  }

  return SUCCESS;
}

Status AiCpuCCTask::InitForSummaryAndCopy() {
  if ((unknown_type_ != DEPEND_COMPUTE) || (num_outputs_ == 0U)) {
    GELOGI("Unknown_type is %d, output num is %zu.", unknown_type_, num_outputs_);
    return SUCCESS;
  }

  output_summary_.resize(num_outputs_);
  for (size_t i = 0U; i < num_outputs_; ++i) {
    constexpr auto result_summary_size = sizeof(aicpu::FWKAdapter::ResultSummary);
    GE_CHK_RT_RET(rtMalloc(&output_summary_[i], result_summary_size, RT_MEMORY_HBM));
  }
  output_summary_host_.resize(num_outputs_);

  const size_t copy_input_buf_len = num_outputs_ * kCopyNum * sizeof(uint64_t);

  GE_CHK_RT_RET(rtMalloc(&copy_input_release_flag_dev_, copy_input_buf_len, RT_MEMORY_HBM));
  GE_CHK_RT_RET(rtMalloc(&copy_input_data_size_dev_, copy_input_buf_len, RT_MEMORY_HBM));
  GE_CHK_RT_RET(rtMalloc(&copy_input_src_dev_, copy_input_buf_len, RT_MEMORY_HBM));
  GE_CHK_RT_RET(rtMalloc(&copy_input_dst_dev_, copy_input_buf_len, RT_MEMORY_HBM));

  copy_io_addr_.emplace_back(PtrToValue(copy_input_release_flag_dev_));
  copy_io_addr_.emplace_back(PtrToValue(copy_input_data_size_dev_));
  copy_io_addr_.emplace_back(PtrToValue(copy_input_src_dev_));
  copy_io_addr_.emplace_back(PtrToValue(copy_input_dst_dev_));
  return SUCCESS;
}

Status AiCpuCCTask::SetMemCopyTask(const domi::KernelDef &kernel_def) {
  auto &memcpy_args = kernel_def.args();
  memcpy_args_size_ = kernel_def.args_size();
  memcpy_so_name_ = kernel_def.so_name();
  memcpy_kernel_name_ = kernel_def.kernel_name();
  if (memcpy_args.size() != memcpy_args_size_) {
    REPORT_INNER_ERROR("E19999", "MemCopy task def args.size=%zu, but args_size=%u not equal.",
                       memcpy_args.size(), memcpy_args_size_);
    GELOGE(FAILED, "[Check][Size]MemCopy task def args.size=%zu, but args_size=%u not equal.",
           memcpy_args.size(), memcpy_args_size_);
    return FAILED;
  }
  if (memcpy_args_size_ < sizeof(aicpu::AicpuParamHead)) {
    REPORT_INNER_ERROR("E19999",
                       "Task def args_size=%u is less than aicpu param head len=%zu.",
                       memcpy_args_size_, sizeof(aicpu::AicpuParamHead));
    GELOGE(FAILED,
           "[Check][Size] Task def args_size=%u is less than aicpu param head len=%zu.",
           memcpy_args_size_, sizeof(aicpu::AicpuParamHead));
    return FAILED;
  }

  memcpy_args_ = MakeUnique<uint8_t[]>(static_cast<size_t>(memcpy_args_size_));
  if (memcpy_args_ == nullptr) {
    REPORT_INNER_ERROR("E19999", "new memory failed for Node[MemCopy], task_size[%u].",
                       memcpy_args_size_);
    GELOGE(FAILED, "[Malloc][Memory] failed for Node[MemCopy], task_size[%u].",
           memcpy_args_size_);
    return FAILED;
  }

  const errno_t sec_ret = memcpy_s(memcpy_args_.get(), static_cast<size_t>(memcpy_args_size_),
                                   memcpy_args.c_str(), memcpy_args.size());
  if (sec_ret != EOK) {
    REPORT_INNER_ERROR("E19999",
                       "memcpy_s argc_ failed for Node[MemCopy], ret: %d", sec_ret);
    GELOGE(INTERNAL_ERROR,
           "[Update][args] failed for Node[MemCopy], ret: %d", sec_ret);
    return FAILED;
  }
  const auto memcpy_param_head = PtrToPtr<uint8_t, aicpu::AicpuParamHead>(memcpy_args_.get());
  const uint32_t memcpy_io_num = memcpy_param_head->ioAddrNum;
  const auto memcpy_io_addr = PtrToPtr<void, uint8_t>(ValueToPtr(PtrToValue(memcpy_args_.get()) +
                                                                 sizeof(aicpu::AicpuParamHead)));
  // if has input and output, need copy to ioaddr
  const int32_t cpy_ret = memcpy_s(memcpy_io_addr,
                                   static_cast<size_t>(memcpy_args_size_ - sizeof(aicpu::AicpuParamHead)),
                                   &copy_io_addr_[0U], sizeof(uint64_t) * memcpy_io_num);
  if (cpy_ret != 0) {
    REPORT_INNER_ERROR("E19999", "Node[Memcpoy] memcpy io addr to AicpuParamHead failed,"
        "ret=%d, args_size=%u, io nums=%u.", cpy_ret, memcpy_args_size_, memcpy_io_num);
    GELOGE(INTERNAL_ERROR, "[Update][io_addr]Node[MemCopy] memcpy io addr to AicpuParamHead failed,"
        "ret=%d, args_size=%u, io nums=%u.", cpy_ret, memcpy_args_size_, memcpy_io_num);
    return INTERNAL_ERROR;
  }
  GELOGD("Set memcpy task for node[MemCopy] successfully.");
  return SUCCESS;
}

Status AiCpuBaseTask::UpdateArgTable(const SingleOpModelParam &param) {
  // aicpu do not have workspace, for now
  return DoUpdateArgTable(param, false);
}

const std::string &AiCpuBaseTask::GetTaskType() const { return kTaskTypeAicpu; }

void AiCpuTask::GetIoAddr(uintptr_t *&arg_base, size_t &arg_count) {
  arg_base = PtrToPtr<void *, uintptr_t>(io_addr_host_.data());
  arg_count = io_addr_host_.size();
}

void AiCpuCCTask::SetKernelArgs(std::unique_ptr<uint8_t[]> args, const size_t arg_size) {
  args_ = std::move(args);
  arg_size_ = arg_size;
  args_ex_.args = args_.get();
  args_ex_.argsSize = arg_size_;
}

void AiCpuCCTask::SetSoName(const std::string &so_name) { so_name_ = so_name; }

void AiCpuCCTask::SetkernelName(const std::string &kernel_Name) { kernel_name_ = kernel_Name; }

void AiCpuCCTask::SetIoAddr(uintptr_t *const io_addr) { io_addr_ = io_addr; }

AiCpuCCTask::~AiCpuCCTask() {
}

Status AiCpuCCTask::UpdateHostMemInputArgs(const std::vector<DataBuffer> &inputs,
                                           const std::vector<DataBuffer> &outputs) {
  (void)outputs;
  args_ex_.hostInputInfoPtr = nullptr;
  args_ex_.hostInputInfoNum = 0U;
  if (!need_host_mem_opt_) {
    return SUCCESS;
  }
  vector<rtHostInputInfo_t> host_inputs;
  GE_CHK_RT_RET(ExecutorUtils::UpdateHostMemInputArgs(inputs, *this, io_addr_,
                                                      arg_size_ - sizeof(aicpu::AicpuParamHead),
                                                      host_inputs));
  host_inputs_info_ = MakeUnique<rtHostInputInfo_t[]>(host_inputs.size());
  GE_CHECK_NOTNULL(host_inputs_info_);
  size_t idx = 0U;
  for (auto &host_input : host_inputs) {
    host_input.dataOffset += sizeof(aicpu::AicpuParamHead);
    host_input.addrOffset += sizeof(aicpu::AicpuParamHead);
    host_inputs_info_[idx++] = host_input;
  }
  args_ex_.hostInputInfoPtr = host_inputs_info_.get();
  args_ex_.hostInputInfoNum = static_cast<uint16_t>(host_inputs.size());
  return SUCCESS;
}

Status AiCpuCCTask::LaunchKernel(const rtStream_t stream) {
  GELOGI("To invoke rtCpuKernelLaunch. block_dim = %u, so_name is %s, kernel_name is %s", block_dim_, so_name_.data(),
         kernel_name_.data());
  // sm_desc is nullptr, because l2 buffer does not support
  auto *const sm_desc = PtrToPtr<void, rtSmDesc_t>(sm_desc_);
  SetTaskTag();
  const auto ret = rtCpuKernelLaunchWithFlag(static_cast<const void *>(so_name_.data()),
                                             static_cast<const void *>(kernel_name_.data()),
                                             block_dim_, &args_ex_,
                                             sm_desc, stream, dump_flag_);
  if (ret != RT_ERROR_NONE) {
    GELOGE(FAILED, "[Invoke][rtCpuKernelLaunchWithFlag] failed. ret = %d.", ret);
    REPORT_CALL_ERROR("E19999", "invoke rtCpuKernelLaunchWithFlag failed, ret:%d.", ret);
    return RT_ERROR_TO_GE_STATUS(ret);
  }
  GE_CHK_STATUS_RET_NOLOG(GetTaskIdAndStreamId());
  GELOGI("[TASK_INFO] %lu/%s", kernel_id_, op_type_.c_str());
  GELOGD("Invoke rtCpuKernelLaunch succeeded");

  if (is_blocking_aicpu_op_) {
    if (DistributeWaitTaskForAicpuBlockingOp(stream) != SUCCESS) {
      GELOGE(FAILED, "[Call][DistributeWaitTaskForAicpuBlockingOp] Call DistributeWaitTaskForAicpuBlockingOp failed");
      return FAILED;
    }
  }
  return SUCCESS;
}

void AiCpuCCTask::GetIoAddr(uintptr_t *&arg_base, size_t &arg_count) {
  arg_base = io_addr_;
  arg_count = io_addr_num_;
}

Status MemcpyAsyncTask::LaunchKernel(const rtStream_t stream) {
  const auto src_addr = ValueToPtr(addresses_[0]);
  const auto dst_addr = ValueToPtr(addresses_[1]);
  kind_ = (kind_ == RT_MEMCPY_ADDR_DEVICE_TO_DEVICE) ? RT_MEMCPY_DEVICE_TO_DEVICE : kind_;
  SetTaskTag();
  GE_CHK_RT_RET(rtMemcpyAsync(dst_addr, dst_max_, src_addr, count_, kind_, stream));
  GE_CHK_STATUS_RET_NOLOG(GetTaskIdAndStreamId());
  return SUCCESS;
}

void MemcpyAsyncTask::GetIoAddr(uintptr_t *&arg_base, size_t &arg_count) {
  arg_base = addresses_;
  arg_count = kMemcpyArgCount;
}
}  // namespace ge
