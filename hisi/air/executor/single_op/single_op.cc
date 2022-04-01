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

#include "single_op/single_op.h"

#include "framework/common/ge_types.h"
#include "common/math_util.h"
#include "common/math/math_util.h"
#include "common/profiling/profiling_manager.h"
#include "framework/common/debug/ge_log.h"
#include "framework/common/util.h"
#include "graph/load/model_manager/model_utils.h"
#include "runtime/mem.h"
#include "single_op/single_op_manager.h"
#include "single_op/task/build_task_utils.h"
#include "graph/load/model_manager/model_manager.h"
#include "common/profiling_definitions.h"

namespace ge {
namespace {
const size_t kDataMemAlignedSize = 32U;
const size_t kDataMemAlignUnit = 2U;
const int64_t kAlignBytes = 512;

size_t GetAlignedSizeFromDataBuffer(const size_t buf_size) {
  return (buf_size + (kDataMemAlignUnit * kDataMemAlignedSize) - 1U) / kDataMemAlignedSize * kDataMemAlignedSize;
}

Status ReportTaskInfo(const OpTask *const op_task, const std::string &shape_type) {
  auto &prof_mgr = ProfilingManager::Instance();
  TaskDescInfo op_task_desc_info;
  if (op_task->GetProfilingArgs(op_task_desc_info) != SUCCESS) {
    GELOGE(ACL_ERROR_GE_PARAM_INVALID, "[Get][ProfilingArgs] failed.");
    return ACL_ERROR_GE_PARAM_INVALID;
  }
  GELOGD("ProfilingReport of op [%s] model [%s] start.", op_task_desc_info.op_name.c_str(),
         op_task_desc_info.model_name.c_str());

  op_task_desc_info.shape_type = shape_type;
  op_task_desc_info.cur_iter_num = static_cast<int64_t>(prof_mgr.GetStepInfoIndex());
  op_task_desc_info.task_type = op_task->GetTaskType();

  std::vector<TaskDescInfo> task_desc_info;
  task_desc_info.emplace_back(op_task_desc_info);

  prof_mgr.ReportProfilingData(kInvalidModelId, task_desc_info);
  return SUCCESS;
}

Status ProfilingTaskInfo(const OpTask &op_task, const std::string &shape_type) {
  if (!ProfilingManager::Instance().ProfilingModelLoadOn()) {
    return SUCCESS;
  }
  if (op_task.NeedReportAtomicTask()) {
    GE_CHK_STATUS_RET_NOLOG(ReportTaskInfo(op_task.GetAtomicTask(), shape_type));
  }
  GE_CHK_STATUS_RET_NOLOG(ReportTaskInfo(&op_task, shape_type));
  return SUCCESS;
}

Status CalInputsHostMemSize(const std::vector<DataBuffer> &inputs,
                            std::vector<std::pair<size_t, uint64_t>> &inputs_size) {
  int64_t total_size = 0;
  size_t input_index = 0UL;
  for (auto &input_buffer : inputs) {
    if (input_buffer.placement == kHostMemType) {
      REQUIRE_COMPAT_INT64(input_buffer.length);
      auto input_size = static_cast<int64_t>(input_buffer.length);
      // input_size pad to 512
      GE_CHK_STATUS_RET(CheckInt64AddOverflow(input_size, (kAlignBytes - 1)), "Padding size is beyond the INT64_MAX.");
      input_size = ((input_size + kAlignBytes - 1) / kAlignBytes) * kAlignBytes;
      inputs_size.emplace_back(input_index, input_size);
      GE_CHK_STATUS_RET(CheckInt64AddOverflow(total_size, input_size), "Total size is beyond the INT64_MAX.");
      total_size += input_size;
      GELOGD("The [%zu]th input mem type is host, the tensor size is %ld.", input_index, input_size);
    }
    input_index++;
  }
  if (static_cast<uint64_t>(total_size) > kFuzzDeviceBufferSize) {
    GELOGE(FAILED, "[Check][Size]Total size is %ld, larger than 1M.", total_size);
    return FAILED;
  }
  return SUCCESS;
}

Status UpdateInputsBufferAddr(const StreamResource *const stream_resource, const rtStream_t stream,
                              const std::vector<std::pair<size_t, uint64_t>> &inputs_size,
                              std::vector<DataBuffer> &update_buffers) {
  PROFILING_SCOPE(-1, profiling::kCopyH2D);
  GE_CHECK_NOTNULL(stream_resource);
  auto dst_addr = PtrToPtr<void, uint8_t>(stream_resource->GetDeviceBufferAddr());
  // copy host mem from input_buffer to device mem of dst_addr
  for (const auto &input_size : inputs_size) {
    const auto input_index = input_size.first;
    const auto size = input_size.second;
    if (size <= 0U) {
      continue;
    }
    GELOGD("Do h2d for %zu input, dst size is %zu, src length is %lu.",
        input_index, size, update_buffers[input_index].length);
    GE_CHK_RT_RET(rtMemcpyAsync(dst_addr, size, update_buffers[input_index].data,
        update_buffers[input_index].length, RT_MEMCPY_HOST_TO_DEVICE_EX, stream));
    update_buffers[input_index].data = dst_addr;
    dst_addr = PtrToPtr<void, uint8_t>(ValueToPtr(PtrToValue(dst_addr) + size));
  }
  return SUCCESS;
}

Status InitHybridModelArgs(const std::vector<DataBuffer> &input_buffers,
                           const std::vector<DataBuffer> &output_buffers,
                           const std::vector<GeTensorDesc> &inputs_desc,
                           hybrid::HybridModelExecutor::ExecuteArgs &args) {
  PROFILING_SCOPE(-1, profiling::kInitHybridExecuteArgs);
  for (auto &input : input_buffers) {
    MemStorageType mem_type = input.placement == kHostMemType ? MemStorageType::HOST_DDR : MemStorageType::HBM;
    args.inputs.emplace_back(hybrid::TensorValue(input.data, input.length, mem_type));
  }
  for (auto &output : output_buffers) {
    MemStorageType mem_type = output.placement == kHostMemType ? MemStorageType::HOST_DDR : MemStorageType::HBM;
    args.outputs.emplace_back(hybrid::TensorValue(output.data, output.length, mem_type));
  }
  for (auto &tensor_desc : inputs_desc) {
    // tensor_desc will not be saved to use after execute finished
    args.input_desc.emplace_back(
        ConstGeTensorDescPtr(&tensor_desc, [](const GeTensorDesc *const point) { (void)point; }));
  }
  return SUCCESS;
}

bool CheckHostMemInputsLen(const std::vector<DataBuffer> &input_buffers,
                           const std::vector<std::unique_ptr<OpTask>> &tasks) {
  size_t align_size = 4U;
  for (auto &task : tasks) {
    const auto tmp_size = (task == nullptr) ? align_size : task->GetInputAddrAlignBytes();
    align_size = tmp_size > align_size ? tmp_size : align_size;
  }
  size_t total_size = 0U;
  for (auto &input_buffer : input_buffers) {
    if (input_buffer.placement == kHostMemType) {
      auto input_size = input_buffer.length;
      // unlikely
      if (CheckUint64AddOverflow(input_size, (align_size - 1)) != SUCCESS) {
        return false;
      }
      input_size = ((input_size + align_size - 1) / align_size) * align_size;
      // unlikely
      if (CheckUint64AddOverflow(input_size, total_size) != SUCCESS) {
        return false;
      }
      total_size += input_size;
    }
  }
  if ((total_size > kMaxHostMemInputLen) || (total_size == 0U)) {
    GELOGD("no optimization, the total host memory length is %zu, valid length range is (0, %zu].",
           total_size, kMaxHostMemInputLen);
    return false;
  }
  return true;
}

bool CheckIsSupportHostMemOpt(const std::unique_ptr<hybrid::HybridModel> &hybrid_model,
                              const std::vector<NodePtr> &node_with_hostmem,
                              const std::vector<std::unique_ptr<OpTask>> &tasks) {
  if (hybrid_model != nullptr) {
    if (!hybrid_model->CheckHostMemInputOptimization(node_with_hostmem)) {
      return false;
    }
  } else {
    if (tasks.empty()) {
      return false;
    }
    // As long as any one is not supported, no optimization will be done
    for (auto &task : tasks) {
      if (task == nullptr) {
        return false;
      }
      if (!task->IsArgsExtendedForHostMemInput()) {
        GELOGD("%s task does not extend args for host memory input", task->GetTaskName().c_str());
        return false;
      }
      if (!task->IsSupportHostMemInputOptimize()) {
        GELOGD("%s task does not support host memory input optimization", task->GetTaskName().c_str());
        return false;
      }
    }
  }
  return true;
}

void SetNeedHostMemOpt(const std::unique_ptr<hybrid::HybridModel> &hybrid_model,
                       const std::vector<NodePtr> &node_with_hostmem,
                       const std::vector<std::unique_ptr<OpTask>> &tasks,
                       const bool flag) {
  if (hybrid_model != nullptr) {
    hybrid_model->SetNeedHostMemOpt(node_with_hostmem, flag);
  } else {
    for (auto &task : tasks) {
      task->SetNeedHostMemOpt(flag);
    }
  }
}

bool CheckHostMemInputOpt(const std::vector<DataBuffer> &input_buffers,
                          const std::unique_ptr<hybrid::HybridModel> &hybrid_model,
                          const std::vector<NodePtr> &node_with_hostmem,
                          const std::vector<std::unique_ptr<OpTask>> &tasks) {
  if (!CheckIsSupportHostMemOpt(hybrid_model, node_with_hostmem, tasks)) {
    return false;
  }

  const std::function<void()> callback = [&hybrid_model, &node_with_hostmem, &tasks] () {
    SetNeedHostMemOpt(hybrid_model, node_with_hostmem, tasks, false);
  };
  GE_DISMISSABLE_GUARD(set_host_mem_input_opt_false, callback);

  if (!CheckHostMemInputsLen(input_buffers, tasks)) {
    GELOGD("no optimization");
    return false;
  }
  GE_DISMISS_GUARD(set_host_mem_input_opt_false);
  SetNeedHostMemOpt(hybrid_model, node_with_hostmem, tasks, true);

  GELOGD("host memory input optimization check return true");
  return true;
}
}  // namespace

SingleOp::SingleOp(StreamResource *const stream_resource, std::mutex *const stream_mutex, const rtStream_t stream)
    : stream_resource_(stream_resource), stream_mutex_(stream_mutex), stream_(stream) {
}

SingleOp::~SingleOp() {
}

Status SingleOp::ValidateArgs(const std::vector<DataBuffer> &inputs, const std::vector<DataBuffer> &outputs) {
  const auto num_inputs = inputs.size();
  if (num_inputs != input_sizes_.size()) {
    GELOGE(ACL_ERROR_GE_PARAM_INVALID,
           "[Check][Param:inputs]Input num mismatch. model expect %zu, but given %zu",
           input_addr_list_.size(), inputs.size());
    REPORT_INPUT_ERROR("E10401", std::vector<std::string>({"expect_num", "input_num"}),
        std::vector<std::string>({std::to_string(input_addr_list_.size()), std::to_string(num_inputs)}));
    return ACL_ERROR_GE_PARAM_INVALID;
  }

  for (size_t i = 0U; i < num_inputs; ++i) {
    // preventing from read out of bound
    const size_t aligned_size = GetAlignedSizeFromDataBuffer(inputs[i].length);
    GELOGI("Input [%zu], aligned_size:%zu, inputs.length:%lu, input_sizes_:%zu",
           i, aligned_size, inputs[i].length, input_sizes_[i]);
    if (aligned_size < input_sizes_[i]) {
      GELOGE(ACL_ERROR_GE_PARAM_INVALID,
             "[Check][Param:inputs]Input size mismatch. index = %zu, model expect %zu, but given %zu(after align)",
             i, input_sizes_[i], aligned_size);
      REPORT_INPUT_ERROR("E10402", std::vector<std::string>({"index", "expect_size", "input_size"}),
          std::vector<std::string>({std::to_string(i), std::to_string(input_sizes_[i]), std::to_string(aligned_size)}));
      return ACL_ERROR_GE_PARAM_INVALID;
    }
  }

  const auto num_outputs = outputs.size();
  if (num_outputs != output_sizes_.size()) {
    GELOGE(ACL_ERROR_GE_PARAM_INVALID, "[Check][Param:outputs]output num mismatch. model expect %zu, but given %zu",
           output_sizes_.size(), outputs.size());
    REPORT_INPUT_ERROR("E10403", std::vector<std::string>({"expect_num", "input_num"}),
        std::vector<std::string>({std::to_string(output_sizes_.size()), std::to_string(outputs.size())}));
    return ACL_ERROR_GE_PARAM_INVALID;
  }

  for (size_t i = 0U; i < num_outputs; ++i) {
    // preventing from write out of bound
    const size_t aligned_size = GetAlignedSizeFromDataBuffer(outputs[i].length);
    GELOGI("Output [%zu], aligned_size:%zu, outputs.length:%lu, output_sizes_:%zu",
           i, aligned_size, outputs[i].length, output_sizes_[i]);
    if (aligned_size < output_sizes_[i]) {
      GELOGE(ACL_ERROR_GE_PARAM_INVALID,
             "[Check][Param:outputs]Output size mismatch. index = %zu, model expect %zu, but given %zu(after align)",
             i, output_sizes_[i], aligned_size);
      REPORT_INPUT_ERROR("E10404", std::vector<std::string>({"index", "expect_size", "input_size"}),
          std::vector<std::string>({std::to_string(i), std::to_string(output_sizes_[i]), std::to_string(aligned_size)})
          );
      return ACL_ERROR_GE_PARAM_INVALID;
    }
  }

  return SUCCESS;
}

Status SingleOp::GetArgs(const std::vector<DataBuffer> &inputs, const std::vector<DataBuffer> &outputs) {
  size_t arg_index = 0U;
  for (auto &input : inputs) {
    args_[arg_index] = PtrToValue(input.data);
    arg_index++;
  }

  for (auto &output : outputs) {
    args_[arg_index] = PtrToValue(output.data);
    arg_index++;
  }
  return SUCCESS;
}

Status SingleOp::UpdateArgs(const std::vector<DataBuffer> &inputs, const std::vector<DataBuffer> &outputs) {
  const Status ret = GetArgs(inputs, outputs);
  if (ret != SUCCESS) {
    return ret;
  }
  // update tbe task args
  for (size_t i = 0U; i < arg_table_.size(); ++i) {
    std::vector<uintptr_t *> &ptr_to_arg_in_tasks = arg_table_[i];
    if (ptr_to_arg_in_tasks.empty()) {
      GELOGW("found NO arg address to update for arg[%lu]", i);
      continue;
    }

    for (uintptr_t *const arg_addr : ptr_to_arg_in_tasks) {
      *arg_addr = args_[i];
    }
  }

  // update host mem input args
  for (auto &task : tasks_) {
    GE_CHK_STATUS_RET(task->UpdateHostMemInputArgs(inputs, outputs), "%s update host memory input args failed.",
                      task->GetOpdesc()->GetName().c_str());
  }
  return SUCCESS;
}

Status SingleOp::ExecuteAsync(const std::vector<DataBuffer> &inputs, const std::vector<DataBuffer> &outputs) {
  GELOGD("Start SingleOp::ExecuteAsync.");
  Status ret = ValidateArgs(inputs, outputs);
  if (ret != SUCCESS) {
    return ret;
  }

  GE_CHECK_NOTNULL(stream_resource_);
  std::vector<std::pair<size_t, uint64_t>> inputs_size;
  GE_CHK_STATUS_RET_NOLOG(CalInputsHostMemSize(inputs, inputs_size));
  const std::lock_guard<std::mutex> lk(*stream_mutex_);
  const bool need_host_mem_optimize = CheckHostMemInputOptimization(inputs);
  std::vector<DataBuffer> update_buffers = inputs;
  if ((!inputs_size.empty()) && (!need_host_mem_optimize)) {
    GE_CHK_STATUS_RET_NOLOG(UpdateInputsBufferAddr(stream_resource_, stream_, inputs_size, update_buffers));
  }

  if (hybrid_model_executor_ != nullptr) {
    GELOGD("Execute multi-task single op by hybrid model executor");
    hybrid::HybridModelExecutor::ExecuteArgs args;
    GE_CHK_STATUS_RET_NOLOG(InitHybridModelArgs(update_buffers, outputs, inputs_desc_, args));
    return hybrid_model_executor_->Execute(args);
  }

  const auto current_mem_base = stream_resource_->GetMemoryBase();
  if (running_param_->mem_base != current_mem_base) {
    running_param_->mem_base = current_mem_base;
    GELOGD("Memory base changed, new memory base = %p", current_mem_base);
    for (auto &task : tasks_) {
      const auto new_address = BuildTaskUtils::GetAddresses(task->GetOpdesc(), *running_param_);
      GE_CHK_STATUS_RET(task->UpdateArgTable(*running_param_), "[Update][ArgTable] failed, single op:%s.",
          task->GetOpdesc()->GetName().c_str());
    }
  }
  ret = UpdateArgs(update_buffers, outputs);
  if (ret != SUCCESS) {
    return ret;
  }

  for (auto &task : tasks_) {
    ret = task->LaunchKernel(stream_);
    GELOGD("[DEBUG_TASK_INFO : Static Task] %s %s",
           task->GetTaskName().c_str(),
           BuildTaskUtils::GetTaskInfo(task->GetOpdesc(), inputs, outputs).c_str());
    if (ret != SUCCESS) {
      return ret;
    }
    GE_CHK_STATUS_RET(task->OpenDump(stream_), "[Open][Dump]failed, single op:%s.",
        task->GetOpdesc()->GetName().c_str());
    GE_CHK_STATUS_RET_NOLOG(ProfilingTaskInfo(*task, kShapeTypeStatic));
  }

  return ret;
}

bool SingleOp::CheckHostMemInputOptimization(const std::vector<DataBuffer> &input_buffers) const {
  return CheckHostMemInputOpt(input_buffers, hybrid_model_, node_with_hostmem_, tasks_);
}

int64_t SingleOp::GetProfilingNodeIndex() const noexcept {
  return profiling_node_type_index_;
}
DynamicSingleOp::DynamicSingleOp(ObjectPool<GeTensor> *const tensor_pool, const uintptr_t resource_id,
    std::mutex *const stream_mutex, const rtStream_t stream)
    : tensor_pool_(tensor_pool), resource_id_(resource_id), stream_mutex_(stream_mutex), stream_(stream) {}

Status DynamicSingleOp::ValidateParams(const std::vector<GeTensorDesc> &input_desc,
                                       const std::vector<DataBuffer> &inputs,
                                       const std::vector<GeTensorDesc> &output_desc,
                                       const std::vector<DataBuffer> &outputs) const {
  if (inputs.size() != input_desc.size()) {
    GELOGE(ACL_ERROR_GE_PARAM_INVALID,
        "[Check][Param:inputs]Input number mismatches input desc number. Input num = %zu, input desc num = %zu",
        inputs.size(), input_desc.size());
    REPORT_INPUT_ERROR("E10405", std::vector<std::string>({"input_num", "input_desc_num"}),
        std::vector<std::string>({std::to_string(inputs.size()), std::to_string(input_desc.size())}));
    return ACL_ERROR_GE_PARAM_INVALID;
  }

  if (outputs.size() != output_desc.size()) {
    GELOGE(ACL_ERROR_GE_PARAM_INVALID,
        "[Check][Param:outputs]Output number mismatches output desc number. Output num = %zu, output desc num = %zu",
        outputs.size(), output_desc.size());
    REPORT_INPUT_ERROR("E10406", std::vector<std::string>({"out_num", "out_desc_num"}),
        std::vector<std::string>({std::to_string(outputs.size()), std::to_string(output_desc.size())}));
    return ACL_ERROR_GE_PARAM_INVALID;
  }

  if (input_desc.size() != num_inputs_) {
    GELOGE(ACL_ERROR_GE_PARAM_INVALID, "[Check][Param:input_desc]Input number mismatches. expect %zu, but given %zu",
        num_inputs_, input_desc.size());
    REPORT_INPUT_ERROR("E10401", std::vector<std::string>({"expect_num", "input_num"}),
        std::vector<std::string>({std::to_string(num_inputs_), std::to_string(input_desc.size())}));
    return ACL_ERROR_GE_PARAM_INVALID;
  }

  if (output_desc.size() != num_outputs_) {
    GELOGE(ACL_ERROR_GE_PARAM_INVALID, "[Check][Param:output_desc]Output number mismatches. expect %zu, but given %zu",
        num_outputs_, output_desc.size());
    REPORT_INPUT_ERROR("E10403", std::vector<std::string>({"expect_num", "input_num"}),
        std::vector<std::string>({std::to_string(num_outputs_), std::to_string(output_desc.size())}));
    return ACL_ERROR_GE_PARAM_INVALID;
  }

  return SUCCESS;
}

Status DynamicSingleOp::SetHostTensorValue(const std::vector<std::pair<size_t, uint64_t>> &inputs_size,
                                           const std::vector<GeTensorDesc> &input_desc,
                                           const std::vector<DataBuffer> &input_buffers) {
  PROFILING_SCOPE(GetProfilingNodeIndex(), profiling::kConstPrepare);
  const auto op_desc = op_task_->GetOpdesc();
  GE_CHECK_NOTNULL(op_desc);
  GE_CHECK_NOTNULL(tensor_pool_);
  GELOGD("Start to update input tensors value for node %s.", op_desc->GetName().c_str());
  for (const auto &input_size : inputs_size) {
    const size_t input_index = input_size.first;
    const auto ge_tensor_desc = input_desc.at(input_index);
    // reconstruct GeTensor by DataBuffer
    auto ge_tensor = tensor_pool_->Acquire();
    GE_CHECK_NOTNULL(ge_tensor);
    ge_tensor->SetTensorDesc(ge_tensor_desc);
    GELOGD("The %zu tensor input type is host, desc data type is %d, input buffer addr is %p, size is %lu.",
           input_index, static_cast<int32_t>(ge_tensor_desc.GetDataType()), input_buffers[input_index].data,
           input_buffers[input_index].length);
    if (ge_tensor->SetData(PtrToPtr<void, uint8_t>(input_buffers[input_index].data),
                           static_cast<size_t>(input_buffers[input_index].length),
                           [](const uint8_t *const point){ (void)point; }) != SUCCESS) {
      GELOGE(INTERNAL_ERROR, "[Set][Data]Failed to set data of ge tensor.");
      return INTERNAL_ERROR;
    }

    const auto iter = input_node_anchor_map_[static_cast<int32_t>(input_index)];
    // cannot be deleted by shared_ptr
    const GeTensorPtr shared_tensor =
        std::shared_ptr<GeTensor>(ge_tensor.get(), [](const GeTensor *const point) { (void)point; });
    GE_CHECK_NOTNULL(shared_tensor);
    (void)runtime_context_.SetTensor(static_cast<int64_t>(iter.first), iter.second, shared_tensor);
    shared_tensors_.push(std::move(ge_tensor));
  }
  return SUCCESS;
}

Status DynamicSingleOp::SetHostTensorValue(const std::vector<GeTensorDesc> &input_desc,
                                           const std::vector<DataBuffer> &input_buffers) {
  PROFILING_SCOPE(GetProfilingNodeIndex(), profiling::kConstPrepare);
  GE_CHECK_NOTNULL(tensor_pool_);
  for (const auto &iter : hostmem_node_id_map_) {
    const auto index = static_cast<const size_t>(iter.first);
    if ((index >= input_desc.size()) || (index >= input_buffers.size())) {
      GELOGE(INTERNAL_ERROR, "[Check][Size]Index %zu should smaller then input desc size %zu "
             "and input buffers size %zu.", index, input_desc.size(), input_buffers.size());
      return INTERNAL_ERROR;
    }
    const auto ge_tensor_desc = input_desc[index];
    // reconstruct GeTensor by DataBuffer
    auto ge_tensor = tensor_pool_->Acquire();
    GE_CHECK_NOTNULL(ge_tensor);
    ge_tensor->SetTensorDesc(ge_tensor_desc);
    GELOGD("The %zu tensor input type is host, desc data type is %d, input buffer addr is %p, size is %ld.",
           index, ge_tensor_desc.GetDataType(), input_buffers[index].data, input_buffers[index].length);
    if (ge_tensor->SetData(PtrToPtr<void, uint8_t>(input_buffers[index].data),
                           static_cast<size_t>(input_buffers[index].length),
                           [](const uint8_t *const data) { (void)data; }) != SUCCESS) {
      GELOGE(INTERNAL_ERROR, "[Set][Data]Failed to set data of ge tensor.");
      return INTERNAL_ERROR;
    }
    const GeTensorPtr shared_tensor =
        std::shared_ptr<GeTensor>(ge_tensor.get(), [](const GeTensor *const point) { (void)point; });
    (void)hybrid_model_executor_->GetContext()->runtime_context_.SetTensor(iter.second, 0, shared_tensor);
    shared_tensors_.push(std::move(ge_tensor));
  }
  return SUCCESS;
}

bool DynamicSingleOp::CheckHostMemInputOptimization(const std::vector<DataBuffer> &input_buffers) {
  std::vector<std::unique_ptr<OpTask>> tasks;
  tasks.push_back(std::move(op_task_));
  const bool ret = CheckHostMemInputOpt(input_buffers, hybrid_model_, node_with_hostmem_, tasks);
  op_task_.reset(tasks[0U].release());
  return ret;
}

Status DynamicSingleOp::ExecuteAsync(const std::vector<GeTensorDesc> &input_desc,
                                     const std::vector<DataBuffer> &input_buffers,
                                     std::vector<GeTensorDesc> &output_desc,
                                     std::vector<DataBuffer> &output_buffers) {
  GELOGD("Start DynamicSingleOp::ExecuteAsync.");
  GE_CHK_STATUS_RET_NOLOG(ValidateParams(input_desc, input_buffers, output_desc, output_buffers));
  std::vector<std::pair<size_t, uint64_t>> inputs_size;
  GE_CHK_STATUS_RET_NOLOG(CalInputsHostMemSize(input_buffers, inputs_size));
  std::vector<DataBuffer> update_buffers = input_buffers;
  const std::lock_guard<std::mutex> lk(*stream_mutex_);
  const bool need_host_mem_opt = CheckHostMemInputOptimization(input_buffers);
  if ((!inputs_size.empty()) && (!need_host_mem_opt)) {
    StreamResource *const stream_resource  = SingleOpManager::GetInstance().GetResource(resource_id_, stream_);
    GE_CHK_STATUS_RET_NOLOG(UpdateInputsBufferAddr(stream_resource, stream_, inputs_size, update_buffers));
  }

  GE_MAKE_GUARD(tensor_gard, [this]() {
    while (!shared_tensors_.empty()) {
      tensor_pool_->Release(std::move(shared_tensors_.front()));
      shared_tensors_.pop();
    }
    runtime_context_.Release();
  });

  if (hybrid_model_executor_ != nullptr) {
    GELOGD("Execute multi-task dynamic single op by hybrid model executor");
    if (!inputs_size.empty()) {
      GE_CHK_STATUS_RET_NOLOG(SetHostTensorValue(input_desc, input_buffers));
    }
    hybrid::HybridModelExecutor::ExecuteArgs args;
    GE_CHK_STATUS_RET_NOLOG(InitHybridModelArgs(update_buffers, output_buffers, input_desc, args));
    return hybrid_model_executor_->ExecuteForSingleOp(args);
  }
  GE_CHECK_NOTNULL(op_task_);
  if (!inputs_size.empty()) {
    GE_CHK_STATUS_RET_NOLOG(SetHostTensorValue(inputs_size, input_desc, input_buffers));
    GE_CHK_STATUS_RET_NOLOG(op_task_->LaunchKernel(input_desc, update_buffers, output_desc, output_buffers, stream_));
  } else {
    GE_CHK_STATUS_RET_NOLOG(op_task_->LaunchKernel(input_desc, input_buffers, output_desc, output_buffers, stream_));
  }
  GELOGD("[DEBUG_TASK_INFO : Dynamic Task] %s",
         BuildTaskUtils::GetTaskInfo(op_task_->GetOpdesc(), input_buffers, output_buffers).c_str());
  GE_CHK_STATUS_RET_NOLOG(op_task_->OpenDump(stream_));
  GE_CHK_STATUS_RET_NOLOG(ProfilingTaskInfo(*op_task_, kShapeTypeDynamic));
  return SUCCESS;
}
void DynamicSingleOp::InjectRuntimeContext() {
  if (op_task_ != nullptr) {
    op_task_->SetRuntimeContext(&runtime_context_);
  }
}
int64_t DynamicSingleOp::GetProfilingNodeIndex() const noexcept {
  return profiling_node_type_index_;
}
}  // namespace ge
