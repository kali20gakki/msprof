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

#include "graph/load/model_manager/task_info/kernel_ex_task_info.h"

#include "common/math_util.h"
#include "graph/load/model_manager/davinci_model.h"
#include "graph/load/model_manager/model_manager.h"
#include "graph/load/model_manager/model_utils.h"

namespace {
const std::string kAttrAicpuAllshape = "_AllShape";
}  // namespace

namespace ge {
bool KernelExTaskInfo::NeedUpdateAddr(const OpDescPtr &op_desc) const {
  return davinci_model_->IsKnownNode() || davinci_model_->HasZeroCopyAddr(op_desc);
}

Status KernelExTaskInfo::InitTaskExtInfo(const std::string &ext_info, const OpDescPtr &op_desc) {
  if (ext_info.empty()) {
    return SUCCESS;
  }
  int32_t unknown_shape_type_val = 0;
  (void)AttrUtils::GetInt(op_desc, ATTR_NAME_UNKNOWN_SHAPE_TYPE, unknown_shape_type_val);
  const auto unknown_type = static_cast<UnknowShapeOpType>(unknown_shape_type_val);
  const uint32_t num_inputs = op_desc->GetInputsSize();
  const uint32_t num_outputs = op_desc->GetOutputsSize();

  hybrid::AicpuExtInfoHandler ext_handle(op_desc->GetName(), num_inputs, num_outputs, unknown_type);
  GE_CHK_STATUS_RET(ext_handle.Parse(ext_info), "[Parse][KernelExtInfo] failed, ext_info_size=%zu", ext_info.size());
  const bool need_update = NeedUpdateAddr(op_desc);
  // 1 means static(no need update), 0 means dynamic(need_update)
  GE_CHK_STATUS_RET(ext_handle.UpdateExecuteMode(!need_update), "[Update][ExecuteMode] failed.");
  GELOGD("Update aicpu_task ext_info bit_map execute mode to %d.", static_cast<int32_t>(!need_update));
  topic_type_flag_ = ext_handle.GetTopicTypeFlag();

  bool all_shape = false;
  (void)AttrUtils::GetBool(op_desc, kAttrAicpuAllshape, all_shape);
  if (all_shape) {
    GELOGD("Aicpu all_shape kernel need to update io shape.");
    for (uint32_t i = 0U; i < num_inputs; i++) {
      const auto input_desc = op_desc->MutableInputDesc(i);
      GE_CHECK_NOTNULL(input_desc);
      GE_CHK_STATUS_RET(ext_handle.UpdateInputShapeAndType(i, *input_desc),
                        "[Call][UpdateInputShapeAndType] Input[%u] update input shape failed, op:%s.",
                        i, op_desc->GetName().c_str());
    }
    if (unknown_type != DEPEND_COMPUTE) {
      for (uint32_t i = 0U; i < num_outputs; ++i) {
        const auto output_desc = op_desc->MutableOutputDesc(i);
        GE_CHECK_NOTNULL(output_desc);
        GE_CHK_STATUS_RET(ext_handle.UpdateOutputShapeAndType(i, *output_desc),
                          "[Call][UpdateOutputShapeAndType] Output[%u] update output shape failed, op:%s.",
                          i, op_desc->GetName().c_str());
      }
    }
  }

  (void)AttrUtils::GetBool(op_desc, ATTR_NAME_IS_BLOCKING_OP, is_blocking_aicpu_op_);
  GELOGD("Get op:%s attribute(is_blocking_op), value:%d", op_desc->GetName().c_str(),
         static_cast<int32_t>(is_blocking_aicpu_op_));

  if (UpdateEventIdForAicpuBlockingOp(op_desc, ext_handle) != SUCCESS) {
    GELOGE(FAILED, "[Call][UpdateEventIdForAicpuBlockingOp] failed for op:%s(%s)",
           op_desc->GetName().c_str(), op_desc->GetType().c_str());
    return FAILED;
  }

  GE_CHK_RT_RET(rtMalloc(&ext_info_addr_, ext_handle.GetExtInfoLen(), RT_MEMORY_HBM));
  GE_CHK_RT_RET(rtMemcpy(ext_info_addr_, ext_handle.GetExtInfoLen(), ext_handle.GetExtInfo(),
                         ext_handle.GetExtInfoLen(), RT_MEMCPY_HOST_TO_DEVICE));

  return SUCCESS;
}

Status KernelExTaskInfo::Init(const domi::TaskDef &task_def, DavinciModel *const davinci_model) {
  GELOGI("KernelExTaskInfo Init Start.");
  GE_CHECK_NOTNULL(davinci_model);
  davinci_model_ = davinci_model;
  GE_CHK_STATUS_RET_NOLOG(SetStream(task_def.stream_id(), davinci_model_->GetStreamList()));

  // 1. Copy context from kernelExDef.private to workspace
  const auto &kernel_ex_def = task_def.kernel_ex();
  const uint32_t op_index = kernel_ex_def.op_index();
  const OpDescPtr op_desc = davinci_model_->GetOpByIndex(op_index);
  op_desc_ = op_desc;
  if (op_desc == nullptr) {
    REPORT_INNER_ERROR("E19999", "Can't get op_desc from davinci_model by index:%u", op_index);
    GELOGE(INTERNAL_ERROR, "[Get][Op] by index failed, index:%u is out of range!", op_index);
    return INTERNAL_ERROR;
  }

  // 2. Reconstruct kernelExDef.args to STR_FWK_OP_KERNEL
  STR_FWK_OP_KERNEL fwk_op_kernel = {};
  const size_t args_size = kernel_ex_def.args().size();
  if (args_size > sizeof(STR_FWK_OP_KERNEL)) {
    REPORT_INNER_ERROR("E19999", "Param kernel_ex_def.args().size():%zu > sizeof(STR_FWK_OP_KERNEL):%zu, check invalid",
                       args_size, sizeof(STR_FWK_OP_KERNEL));
    GELOGE(FAILED, "[Check][Param] kernel_ex_def.args().size():%zu > sizeof(STR_FWK_OP_KERNEL):%zu",
           args_size, sizeof(STR_FWK_OP_KERNEL));
    return FAILED;
  }
  const errno_t sec_ret = memcpy_s(&fwk_op_kernel, sizeof(STR_FWK_OP_KERNEL), kernel_ex_def.args().data(), args_size);
  if (sec_ret != EOK) {
    REPORT_CALL_ERROR("E19999", "Call memcpy_s fail, size:%zu, ret:0x%X", sizeof(STR_FWK_OP_KERNEL), sec_ret);
    GELOGE(FAILED, "[Call][Memcpy] failed, size:%zu, ret: %d", sizeof(STR_FWK_OP_KERNEL), sec_ret);
    return FAILED;
  }

  const auto &ext_info = kernel_ex_def.kernel_ext_info();
  GE_CHK_STATUS_RET(InitTaskExtInfo(ext_info, op_desc),
                    "[Init][TaskExtInfo] failed, ext_info size=%zu, op:%s",
                    ext_info.size(), op_desc->GetName().c_str());

  GELOGI("Node[%s] type[%s] kernel_ext_info size=%zu, ext_info_addr_=%p", op_desc->GetName().c_str(),
         op_desc->GetType().c_str(), ext_info.size(), ext_info_addr_);

  // 2.1 get SessionId and loop variable for tensor array write
  const uint64_t session_id = davinci_model_->GetSessionId();
  fwk_op_kernel.fwkKernelBase.fwk_kernel.sessionID = session_id;

  // 2.2 Collect aicpu kernel
  const uint64_t kernel_id = hybrid::AicpuExtInfoHandler::GenerateKernelId();
  fwk_op_kernel.fwkKernelBase.fwk_kernel.kernelID = kernel_id;
  ModelManager::GetInstance().CreateAicpuKernel(session_id, davinci_model->Id(),
                                                davinci_model->SubModelId(), kernel_id);

  // 2.3 Create session
  GE_CHK_STATUS_RET_NOLOG(ModelManager::GetInstance().CreateAicpuSession(session_id));

  kernel_buf_size_ = sizeof(STR_FWK_OP_KERNEL);
  GE_CHK_RT_RET(rtMalloc(&kernel_buf_, sizeof(STR_FWK_OP_KERNEL), RT_MEMORY_HBM));

  if (davinci_model_->IsKnownNode()) {
    void *const input_output_addr = davinci_model_->GetCurrentArgsAddr(args_offset_);
    fwk_op_kernel.fwkKernelBase.fwk_kernel.inputOutputAddr = PtrToValue(input_output_addr);
    void *workspace_base_addr = nullptr;
    GE_CHK_RT_RET(rtMalloc(&workspace_base_addr, kernel_ex_def.task_info().size(), RT_MEMORY_HBM));
    ext_args_.emplace_back(workspace_base_addr);
    GE_CHK_RT_RET(rtMemcpy(workspace_base_addr, kernel_ex_def.task_info().size(),
                           kernel_ex_def.task_info().data(), kernel_ex_def.task_info().size(),
                           RT_MEMCPY_HOST_TO_DEVICE));

    fwk_op_kernel.fwkKernelBase.fwk_kernel.workspaceBaseAddr = PtrToValue(workspace_base_addr);
    fwk_op_kernel.fwkKernelBase.fwk_kernel.stepIDAddr = davinci_model_->GetGlobalStep();
    fwk_op_kernel.fwkKernelBase.fwk_kernel.extInfoLen = ext_info.size();
    fwk_op_kernel.fwkKernelBase.fwk_kernel.extInfoAddr = PtrToValue(ext_info_addr_);

    GE_CHK_RT_RET(rtMemcpy(kernel_buf_, static_cast<uint64_t>(kernel_buf_size_), static_cast<void *>(&fwk_op_kernel),
                           static_cast<uint64_t>(kernel_buf_size_), RT_MEMCPY_HOST_TO_DEVICE));

    SetIoAddrs(op_desc);
    InitDumpFlag(op_desc);
    InitDumpArgs(input_output_addr, op_desc);
    GELOGI("KernelExTaskInfo knonw node Init Success.");
    return SUCCESS;
  }

  // 3. Set workspaceaddr, inputOutputDataAddr
  const RuntimeParam &rts_param = davinci_model_->GetRuntimeParam();
  const Status ge_ret = CopyTaskInfo(kernel_ex_def, rts_param, op_desc);
  if (ge_ret != SUCCESS) {
    GELOGE(ge_ret, "[Copy][TaskInfo] to workspace failed, op:%s.", op_desc->GetName().c_str());
    return ge_ret;
  }

  const std::vector<void *> workspace_data_addrs = ModelUtils::GetWorkspaceDataAddrs(rts_param, op_desc);
  if (workspace_data_addrs.empty()) {
    REPORT_CALL_ERROR("E19999", "workspace_data_addrs is empty in op:%s(%s), check invalid",
                      op_desc->GetName().c_str(), op_desc->GetType().c_str());
    GELOGE(FAILED, "[Check][Param] workspace_data_addrs is empty in op:%s(%s).",
           op_desc->GetName().c_str(), op_desc->GetType().c_str());
    return FAILED;
  }

  const uint64_t workspace_base_addr = PtrToValue(workspace_data_addrs[0U]);
  const std::vector<void *> input_addrs = ModelUtils::GetInputAddrs(rts_param, op_desc);
  const std::vector<void *> output_addrs = ModelUtils::GetOutputAddrs(rts_param, op_desc);
  std::vector<void *> io_addrs;
  (void)io_addrs.insert(io_addrs.cend(), input_addrs.cbegin(), input_addrs.cend());
  (void)io_addrs.insert(io_addrs.cend(), output_addrs.cbegin(), output_addrs.cend());

  const auto addrs_size = sizeof(uint64_t) * (io_addrs.size());
  if (addrs_size > 0UL) {
    const auto zero_copy_args_index = davinci_model_->GetZeroCopyArgsIndex(io_addrs);
    if (zero_copy_args_index.empty()) {
      GE_CHK_RT_RET(rtMalloc(&input_output_addr_, addrs_size, RT_MEMORY_HBM));
      GE_CHK_RT_RET(rtMemcpy(input_output_addr_, addrs_size, io_addrs.data(), addrs_size, RT_MEMCPY_HOST_TO_DEVICE));
    } else {
      std::map<uintptr_t, std::set<size_t>> zero_copy_args_offset;
      for (const auto &args_index : zero_copy_args_index) {
        (void)zero_copy_args_offset[PtrToValue(io_addrs[args_index])].insert(args_index * sizeof(uint64_t));
      }
      GE_CHK_STATUS_RET(davinci_model_->Mapping2BundleZeroCopy(op_desc, zero_copy_args_offset, addrs_size,
                                                               io_addrs.data(), input_output_addr_, own_args_memory_),
                        "Failed mapping zero copy task for %s to bundle task", op_desc->GetName().c_str());
    }

    InitDumpFlag(op_desc);
    InitDumpArgs(input_output_addr_, op_desc);
  }

  fwk_op_kernel.fwkKernelBase.fwk_kernel.workspaceBaseAddr = workspace_base_addr;
  fwk_op_kernel.fwkKernelBase.fwk_kernel.inputOutputAddr = PtrToValue(input_output_addr_);
  fwk_op_kernel.fwkKernelBase.fwk_kernel.stepIDAddr = davinci_model_->GetGlobalStep();
  fwk_op_kernel.fwkKernelBase.fwk_kernel.extInfoLen = ext_info.size();
  fwk_op_kernel.fwkKernelBase.fwk_kernel.extInfoAddr = PtrToValue(ext_info_addr_);

  // 4. Return result
  GE_CHK_RT_RET(rtMemcpy(kernel_buf_, sizeof(STR_FWK_OP_KERNEL), static_cast<void *>(&fwk_op_kernel),
                         sizeof(STR_FWK_OP_KERNEL), RT_MEMCPY_HOST_TO_DEVICE));

  SetIoAddrs(op_desc);
  GELOGI("KernelExTaskInfo Init Success. session id: %lu", session_id);
  return SUCCESS;
}

void KernelExTaskInfo::InitDumpFlag(const OpDescPtr &op_desc) {
  if (davinci_model_->OpNeedDump(op_desc->GetName())) {
    GELOGD("Op %s need init dump flag in kernel ex task info", op_desc->GetName().c_str());
    dump_flag_ = RT_KERNEL_DUMPFLAG;
  }
}

void KernelExTaskInfo::InitDumpArgs(void *const addr, const OpDescPtr &op_desc) {
  if (davinci_model_->OpNeedDump(op_desc->GetName())) {
    GELOGD("Op %s need dump in kernel ex task info", op_desc->GetName().c_str());
    dump_args_ = addr;
  }
  if (davinci_model_->GetOpDugReg()) {
    GELOGD("Op debug is open in kernel ex task info");
    dump_args_ = addr;
  }
}

Status KernelExTaskInfo::CalculateArgs(const domi::TaskDef &task_def, DavinciModel *const davinci_model) {
  const auto &kernel_ex_def = task_def.kernel_ex();
  const uint32_t op_index = kernel_ex_def.op_index();
  const OpDescPtr op_desc = davinci_model->GetOpByIndex(op_index);
  if (op_desc == nullptr) {
    REPORT_INNER_ERROR("E19999", "Can't get op_desc from davinci_model by index:%u", op_index);
    GELOGE(INTERNAL_ERROR, "[Get][Op] By Index, index:%u is out of range!", op_index);
    return INTERNAL_ERROR;
  }
  args_offset_ = davinci_model->GetTotalArgsSize();
  const size_t inputs_size = op_desc->GetInputsSize();
  const size_t outputs_size = op_desc->GetOutputsSize();
  // aicpu kernel input/output size
  REQUIRE_COMPAT_UINT32(sizeof(uint64_t) * (inputs_size + outputs_size));
  const uint32_t mem_size = static_cast<uint32_t>(sizeof(uint64_t) * (inputs_size + outputs_size));
  davinci_model->SetTotalArgsSize(mem_size);
  GELOGI("kernel task name %s, args_size %u, args_offset %u", op_desc->GetName().c_str(), mem_size, args_offset_);

  // alloc fixed addr
  std::string peer_input_name;
  if (AttrUtils::GetStr(op_desc, ATTR_DYNAMIC_SHAPE_FIXED_ADDR, peer_input_name) && (!peer_input_name.empty())) {
    const uint32_t output_index = davinci_model->GetFixedAddrOutputIndex(peer_input_name);
    if (output_index > outputs_size) {
      REPORT_INNER_ERROR("E19999", "The output size[%zu] and output index[%u] in op:%s(%s) are inconsistent, "
                         "check invalid", outputs_size, output_index,
                         op_desc->GetName().c_str(), op_desc->GetType().c_str());
      GELOGE(FAILED, "[Check][Param] The output size[%zu] and output index[%u] in op:%s(%s) are inconsistent.",
             outputs_size, output_index, op_desc->GetName().c_str(), op_desc->GetType().c_str());
      return FAILED;
    }
    fixed_addr_offset_ = davinci_model->GetFixedAddrsSize(peer_input_name);
    int64_t tensor_size = 0;
    GE_CHK_STATUS(TensorUtils::GetSize(op_desc->GetOutputDesc(output_index), tensor_size));
    davinci_model->SetTotalFixedAddrsSize(peer_input_name, tensor_size);
    GELOGI("Calculate stream switch task args , tensor size is %ld, fixed addr offset %ld", tensor_size,
           fixed_addr_offset_);
  }
  return SUCCESS;
}

void KernelExTaskInfo::SetIoAddrs(const OpDescPtr &op_desc) {
  const RuntimeParam &rts_param = davinci_model_->GetRuntimeParam();
  std::vector<void *> input_data_addrs = ModelUtils::GetInputAddrs(rts_param, op_desc);
  std::vector<void *> output_data_addrs = ModelUtils::GetOutputAddrs(rts_param, op_desc);
  if (!op_desc->HasAttr(ATTR_DYNAMIC_SHAPE_FIXED_ADDR)) {
    (void)io_addrs_.insert(io_addrs_.cend(), input_data_addrs.cbegin(), input_data_addrs.cend());
    (void)io_addrs_.insert(io_addrs_.cend(), output_data_addrs.cbegin(), output_data_addrs.cend());
  } else {
    std::string peer_input_name;
    if (AttrUtils::GetStr(op_desc, ATTR_DYNAMIC_SHAPE_FIXED_ADDR, peer_input_name)) {
      const uint32_t output_index = davinci_model_->GetFixedAddrOutputIndex(peer_input_name);
      if (output_index > output_data_addrs.size()) {
        REPORT_INNER_ERROR("E19999", "The output data addr size[%zu] and output index[%u] in op:%s(%s) "
                           "are inconsistent, check invalid", output_data_addrs.size(), output_index,
                           op_desc->GetName().c_str(), op_desc->GetType().c_str());
        GELOGE(FAILED, "[Check][Param] The output data addr size[%zu] and output index[%u] in op:%s(%s) "
               "are inconsistent.", output_data_addrs.size(), output_index,
               op_desc->GetName().c_str(), op_desc->GetType().c_str());
        return;
      }
      (void)io_addrs_.insert(io_addrs_.cend(), input_data_addrs.cbegin(), input_data_addrs.cend());
      for (size_t i = 0U; i < output_data_addrs.size(); ++i) {
        if (i == output_index) {
          void *fixed_addr = davinci_model_->GetCurrentFixedAddr(fixed_addr_offset_);
          io_addrs_.emplace_back(fixed_addr);
          continue;
        }
        io_addrs_.emplace_back(output_data_addrs[i]);
      }
    }
  }
}

Status KernelExTaskInfo::UpdateArgs() {
  GELOGI("KernelExTaskInfo::UpdateArgs in.");
  davinci_model_->SetTotalIOAddrs(io_addrs_);
  davinci_model_->UpdateOpIOAddrs(task_id_, stream_id_, io_addrs_);
  GELOGI("KernelExTaskInfo::UpdateArgs success.");
  return SUCCESS;
}

Status KernelExTaskInfo::CopyTaskInfo(const domi::KernelExDef &kernel_def, const RuntimeParam &rts_param,
                                      const OpDescPtr &op_desc) {
  // Userspace copy need virtual address.
  const std::vector<int64_t> workspace_data_sizes = ModelUtils::GetWorkspaceSize(op_desc);
  const std::vector<void *> workspace_data_addrs = ModelUtils::GetWorkspaceDataAddrs(rts_param, op_desc);
  if (workspace_data_addrs.empty() || workspace_data_sizes.empty()) {
    REPORT_INNER_ERROR("E19999", "Node:%s(%s) workspace addr:%zu or size:%zu empty, check invalid",
                       op_desc->GetName().c_str(), op_desc->GetType().c_str(),
                       workspace_data_addrs.size(), workspace_data_sizes.size());
    GELOGE(FAILED, "[Check][Param] Node:%s invalid workspace, addrs is %zu, size is %zu.", op_desc->GetName().c_str(),
           workspace_data_addrs.size(), workspace_data_sizes.size());
    return FAILED;
  }

  if (workspace_data_addrs.front() == nullptr) {
    REPORT_INNER_ERROR("E19999", "Node:%s(%s) workspace addr is nullptr, check invalid",
                       op_desc->GetName().c_str(), op_desc->GetType().c_str());
    GELOGE(FAILED, "[Check][Param] Node:%s workspace addrs is null.", op_desc->GetName().c_str());
    return FAILED;
  }

  if (workspace_data_sizes.front() < static_cast<int64_t>(kernel_def.task_info().size())) {
    REPORT_INNER_ERROR("E19999", "Node:%s(%s) workspace size:%ld < task info size:%zu, check invalid",
                       op_desc->GetName().c_str(), op_desc->GetType().c_str(),
                       workspace_data_sizes.front(), kernel_def.task_info().size());
    GELOGE(FAILED, "[Check][Param] Node:%s workspace size is %ld, task info size is %zu.", op_desc->GetName().c_str(),
           workspace_data_sizes.front(), kernel_def.task_info().size());
    return FAILED;
  }

  GE_CHK_RT_RET(rtMemcpy(workspace_data_addrs.front(), kernel_def.task_info().size(),
                         kernel_def.task_info().data(), kernel_def.task_info().size(), RT_MEMCPY_HOST_TO_DEVICE));

  return SUCCESS;
}

Status KernelExTaskInfo::Distribute() {
  GELOGI("KernelExTaskInfo Distribute Start.");
  // Use the fifth and sixth bits of dump_flag_ indicate the value of topic_type.
  // xxxxxxxx xxxxxxxx xxxxxxxx xx00xxxx: DEVICE_ONLY
  // xxxxxxxx xxxxxxxx xxxxxxxx xx01xxxx: DEVICE_FIRST
  // xxxxxxxx xxxxxxxx xxxxxxxx xx10xxxx: HOST_ONLY
  // xxxxxxxx xxxxxxxx xxxxxxxx xx11xxxx: HOST_FIRST
  if (topic_type_flag_ > 0) {
    dump_flag_ = dump_flag_ | static_cast<uint32_t>(topic_type_flag_);
  }
  SetTaskTag(op_desc_->GetName().c_str());
  GE_CHK_RT_RET(rtKernelLaunchFwk(op_desc_->GetName().c_str(), kernel_buf_, kernel_buf_size_, dump_flag_, stream_));

  GE_CHECK_NOTNULL(davinci_model_);
  GE_CHK_RT_RET(rtModelGetTaskId(davinci_model_->GetRtModelHandle(), &task_id_, &stream_id_));

  GELOGI("KernelExTaskInfo Distribute Success. task id: %u, stream id: %u", task_id_, stream_id_);
  if (is_blocking_aicpu_op_) {
    if (DistributeWaitTaskForAicpuBlockingOp() != SUCCESS) {
      GELOGE(FAILED, "[Call][DistributeWaitTaskForAicpuBlockingOp] Call DistributeWaitTaskForAicpuBlockingOp failed");
      return FAILED;
    }
  }

  op_desc_.reset(); // Release OpDesc after Distribute.
  return SUCCESS;
}

void KernelExTaskInfo::PostProcess(const domi::TaskDef &task_def) {
  const auto &kernel_ex_def = task_def.kernel_ex();
  davinci_model_->SaveProfilingTask(kernel_ex_def.op_index(), task_def, *this);
}

Status KernelExTaskInfo::CheckDeviceSupportBlockingAicpuOpProcess(bool &is_support) const {
  int32_t device_id = 0;
  GE_CHK_RT_RET(rtGetDevice(&device_id));
  int32_t value = 0;
  GE_CHK_RT_RET(rtGetDeviceCapability(device_id, FEATURE_TYPE_BLOCKING_OPERATOR, RT_MODULE_TYPE_AICPU, &value));

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

Status KernelExTaskInfo::UpdateEventIdForAicpuBlockingOp(const OpDescPtr &op_desc,
                                                         hybrid::AicpuExtInfoHandler &ext_handle) const {
  if (is_blocking_aicpu_op_) {
    bool is_support = false;
    if (CheckDeviceSupportBlockingAicpuOpProcess(is_support) != SUCCESS) {
      GELOGE(FAILED, "[Call][CheckDeviceSupportBlockingAicpuOpProcess] Call CheckDeviceSupportBlockingAicpuOp failed");
      return FAILED;
    }
    if (!is_support) {
      GELOGD("Device not support blocking aicpu op process");
      return SUCCESS;
    }
    uint32_t event_id = 0U;
    if (davinci_model_->GetEventIdForBlockingAicpuOp(op_desc, stream_, event_id) != SUCCESS) {
      REPORT_CALL_ERROR("E19999", "Get event id failed for op:%s(%s).", op_desc->GetName().c_str(),
                        op_desc->GetType().c_str());
      GELOGE(FAILED, "[Get][EventId] Get event id failed for op:%s(%s)", op_desc->GetName().c_str(),
             op_desc->GetType().c_str());
      return FAILED;
    }
    if (ext_handle.UpdateEventId(event_id) != SUCCESS) {
      REPORT_CALL_ERROR("E19999", "Update event id failed for op:%s(%s).", op_desc->GetName().c_str(),
                        op_desc->GetType().c_str());
      GELOGE(FAILED, "[Update][EventId] Update event id failed for op:%s(%s)", op_desc->GetName().c_str(),
             op_desc->GetType().c_str());
      return FAILED;
    }
    GELOGI("Update event_id=%u success", event_id);
  }
  return SUCCESS;
}

Status KernelExTaskInfo::DistributeWaitTaskForAicpuBlockingOp() const {
  bool is_support = false;
  if (CheckDeviceSupportBlockingAicpuOpProcess(is_support) != SUCCESS) {
    GELOGE(FAILED, "[Call][CheckDeviceSupportBlockingAicpuOpProcess] Call CheckDeviceSupportBlockingAicpuOp failed");
    return FAILED;
  }
  if (!is_support) {
    GELOGD("Device not support blocking aicpu op process.");
    return SUCCESS;
  }
  GELOGD("Distribute wait task begin");
  rtEvent_t rt_event = nullptr;
  if (davinci_model_->GetEventByStream(stream_, rt_event) != SUCCESS) {
    GELOGE(FAILED, "[Call][GetEventByStream] Call GetEventByStream failed");
    return FAILED;
  }

  GE_CHK_RT_RET(rtStreamWaitEvent(stream_, rt_event));
  GE_CHK_RT_RET(rtEventReset(rt_event, stream_));

  return SUCCESS;
}

Status KernelExTaskInfo::Release() {
  GE_FREE_RT_LOG(kernel_buf_);
  if (own_args_memory_) {
    GE_FREE_RT_LOG(input_output_addr_);
  } else {
    input_output_addr_ = nullptr;
  }
  GE_FREE_RT_LOG(ext_info_addr_);
  for (auto &addr : ext_args_) {
    GE_FREE_RT_LOG(addr);
  }
  return SUCCESS;
}

REGISTER_TASK_INFO(RT_MODEL_TASK_KERNEL_EX, KernelExTaskInfo);
}  // namespace ge
