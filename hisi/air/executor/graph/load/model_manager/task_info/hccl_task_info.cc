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

#include "graph/load/model_manager/task_info/hccl_task_info.h"

#include "graph/load/model_manager/davinci_model.h"
#include "graph/load/model_manager/model_utils.h"
#include "opskernel_executor/ops_kernel_executor_manager.h"

namespace ge {
std::mutex HcclTaskInfo::hccl_follow_stream_mutex_;

HcclTaskInfo::~HcclTaskInfo() {
  if (private_def_ != nullptr) {
    const rtError_t ret = rtFreeHost(private_def_);
    if (ret != RT_ERROR_NONE) {
      REPORT_CALL_ERROR("E19999", "Call rtFreeHost failed, ret:0x%X", ret);
      GELOGE(RT_FAILED, "[Call][RtFree] Fail, ret = 0x%X.", ret);
    }
    private_def_ = nullptr;
  }
  davinci_model_ = nullptr;
  ops_kernel_store_ = nullptr;
  args_ = nullptr;
}

Status HcclTaskInfo::Init(const domi::TaskDef &task_def, DavinciModel *const davinci_model) {
  GELOGI("HcclTaskInfo Init Start.");
  GE_CHECK_NOTNULL(davinci_model);
  davinci_model_ = davinci_model;
  GE_CHK_STATUS_RET_NOLOG(SetStream(task_def.stream_id(), davinci_model_->GetStreamList()));

  static std::atomic<uint32_t> hccl_task_id(0U);
  id_ = hccl_task_id.fetch_add(1U);

  const auto &hccl_def = task_def.kernel_hccl();
  const uint32_t op_index = hccl_def.op_index();
  GELOGI("HcclTaskInfo Init, op_index is: %u", op_index);

  // Get HCCL op
  const auto op_desc = davinci_model_->GetOpByIndex(op_index);
  GE_CHECK_NOTNULL(op_desc);
  GetPrivateDefByTaskDef(op_desc, task_def);

  // Create the kernel hccl infos
  CreateKernelHcclInfo(op_desc);

  // Initialize the hccl_type of all kernel hccl info
  HcomOmeUtil::GetHcclType(task_def, kernel_hccl_infos_);

  // Only in Horovod scenario should get the inputName and GeShape
  auto ret = HcomOmeUtil::GetHorovodInputs(op_desc, kernel_hccl_infos_);
  if (ret != SUCCESS) {
    REPORT_CALL_ERROR("E19999", "Call GetHorovodInputs fail for op:%s(%s)",
                      op_desc->GetName().c_str(), op_desc->GetType().c_str());
    GELOGE(ret, "[Get][HorovodInputs] fail for op:%s(%s)", op_desc->GetName().c_str(), op_desc->GetType().c_str());
    return ret;
  }
  Status dmrt = HcomOmeUtil::GetHcclDataType(op_desc, kernel_hccl_infos_);
  if (dmrt != SUCCESS) {
    REPORT_CALL_ERROR("E19999", "Call GetHcclDataType fail for op:%s(%s)",
                      op_desc->GetName().c_str(), op_desc->GetType().c_str());
    GELOGE(dmrt, "[Get][HcomDataType] fail for op:%s(%s)", op_desc->GetName().c_str(), op_desc->GetType().c_str());
    return dmrt;
  }
  dmrt = HcomOmeUtil::GetHcclCount(op_desc, kernel_hccl_infos_);
  if (dmrt != SUCCESS) {
    REPORT_CALL_ERROR("E19999", "Call GetHcclCount fail for op:%s(%s)",
                      op_desc->GetName().c_str(), op_desc->GetType().c_str());
    GELOGE(dmrt, "[Get][HcomCount] fail for op:%s(%s)", op_desc->GetName().c_str(), op_desc->GetType().c_str());
    return dmrt;
  }
  // Only HCOMBROADCAST and HVDCALLBACKBROADCAST need to get the rootId
  dmrt = HcomOmeUtil::GetAllRootId(op_desc, kernel_hccl_infos_);
  if (dmrt != SUCCESS) {
    REPORT_CALL_ERROR("E19999", "Call GetAllRootId fail for op:%s(%s)",
                      op_desc->GetName().c_str(), op_desc->GetType().c_str());
    GELOGE(dmrt, "[Get][RootId] fail for op:%s(%s)", op_desc->GetName().c_str(), op_desc->GetType().c_str());
    return dmrt;
  }

  // GE's new process: hccl declares the number of streams required, creates a stream by GE, and sends it to hccl
  ret = SetFollowStream(op_desc);
  if (ret != SUCCESS) {
    GELOGE(ret, "[Set][Stream] Fail for op:%s(%s)", op_desc->GetName().c_str(), op_desc->GetType().c_str());
    return ret;
  }

  if (davinci_model_->IsKnownNode()) {
    args_ = davinci_model_->GetCurrentArgsAddr(args_offset_);
    GELOGI("Known node %s args addr %p, offset %u.", op_desc->GetName().c_str(), args_, args_offset_);
  }

  ret = SetAddrs(op_desc, kernel_hccl_infos_);
  if (ret != SUCCESS) {
    GELOGE(ret, "[Set][Addrs] Fail for op:%s(%s)", op_desc->GetName().c_str(), op_desc->GetType().c_str());
    return ret;
  }
  // GE's new process: hccl declares the need for Workspace size, and GE allocates Workspace
  ret = SetWorkspace(op_desc, kernel_hccl_infos_);
  if (ret != SUCCESS) {
    GELOGE(ret, "[Set][Workspace] Fail for op:%s(%s)", op_desc->GetName().c_str(), op_desc->GetType().c_str());
    return ret;
  }

  SetIoAddrs(op_desc);
  GELOGI("HcclTaskInfo Init Success");
  return SUCCESS;
}

Status HcclTaskInfo::SetFollowStream(const ConstOpDescPtr &op_desc) {
  if (!HcomOmeUtil::IsHCOMOp(op_desc->GetType())) {
    GELOGI("Node %s Optye %s no need to create slave streams.", op_desc->GetName().c_str(), op_desc->GetType().c_str());
    return SUCCESS;
  }

  int64_t hccl_stream_num = 0;
  if ((!AttrUtils::GetInt(op_desc, "used_stream_num", hccl_stream_num)) || (hccl_stream_num < 0)) {
    GELOGI("op_desc has no attr used_stream_num or is invalid!");
  }

  const std::lock_guard<std::mutex> lock(hccl_follow_stream_mutex_);
  const int64_t main_stream_id = op_desc->GetStreamId();
  const std::map<int64_t, std::vector<rtStream_t>> &main_follow_stream_mapping = davinci_model_->GetHcclFolowStream();

  if (main_follow_stream_mapping.find(main_stream_id) != main_follow_stream_mapping.end()) {
    const std::vector<rtStream_t> &follow_stream_usage = main_follow_stream_mapping.at(main_stream_id);
    if (static_cast<size_t>(hccl_stream_num) <= follow_stream_usage.size()) {
      GELOGI("capacity of follow stream is enough to be reused.");
      for (size_t i = 0UL; i < static_cast<size_t>(hccl_stream_num); i++) {
        hccl_stream_list_.emplace_back(follow_stream_usage.at(i));
      }
    } else {
      GELOGI("need to reuse follow stream and create new follow stream.");
      const size_t created_stream_num = follow_stream_usage.size();
      for (const auto &stream : follow_stream_usage) {
        hccl_stream_list_.emplace_back(stream);
      }
      const auto ret = CreateStream(hccl_stream_num - static_cast<int64_t>(created_stream_num), main_stream_id);
      if (ret != SUCCESS) {
        GELOGE(RT_FAILED, "[Create][Stream] for %s failed, stream id:%ld, stream num:%lu.",
               op_desc->GetName().c_str(), main_stream_id, static_cast<uint64_t>(hccl_stream_num) - created_stream_num);
        return RT_ERROR_TO_GE_STATUS(ret);
      }
    }
    GELOGI("Initialize hccl slave stream success, hcclStreamNum =%ld", hccl_stream_num);
  } else {
    GELOGI("need to create follow stream for %s with new mainstream %ld.", op_desc->GetName().c_str(), main_stream_id);
    const auto ret = CreateStream(hccl_stream_num, main_stream_id);
    if (ret != SUCCESS) {
      GELOGE(RT_FAILED, "[Create][Stream] for %s failed, stream id:%ld, stream num:%ld.",
             op_desc->GetName().c_str(), main_stream_id, hccl_stream_num);
      return RT_ERROR_TO_GE_STATUS(ret);
    }
  }
  return SUCCESS;
}

Status HcclTaskInfo::CreateStream(const int64_t stream_num, const int64_t main_stream_id) {
  GELOGI("Start to create %ld hccl stream.", stream_num);
  for (int64_t i = 0; i < stream_num; ++i) {
    rtStream_t stream = nullptr;
    const auto stream_flags = static_cast<uint32_t>(RT_STREAM_PERSISTENT) | static_cast<uint32_t>(RT_STREAM_FORCE_COPY);
    GE_CHK_RT_RET(rtStreamCreateWithFlags(&stream, davinci_model_->Priority(), stream_flags));
    hccl_stream_list_.emplace_back(stream);
    davinci_model_->PushHcclStream(stream);

    // Create slave stream, inactive by default, activated by hccl
    GE_CHK_RT_RET(rtModelBindStream(davinci_model_->GetRtModelHandle(), stream,
                                    static_cast<uint32_t>(RT_MODEL_WAIT_ACTIVE_STREAM)));
    GELOGD("hccl_stream addr is=%p", stream);
    davinci_model_->SaveHcclFollowStream(main_stream_id, stream);
  }
  GELOGI("CreateStream success.");
  return SUCCESS;
}

Status HcclTaskInfo::Distribute() {
  GELOGI("HcclTaskInfo Distribute Start. begin to call function LoadTask in hccl.");
  if (ops_kernel_store_ == nullptr) {
    REPORT_INNER_ERROR("E19999", "Check param ops_kernel_store_ nullptr");
    GELOGE(INTERNAL_ERROR, "[Check][Param] ops kernel store is null.");
    return INTERNAL_ERROR;
  }
  GETaskInfo ge_task;
  TransToGETaskInfo(ge_task);
  const auto result = ops_kernel_store_->LoadTask(ge_task);
  if (result != HCCL_SUCCESS) {
    REPORT_CALL_ERROR("E19999", "Call ops_kernel_info_store LoadTask fail");
    GELOGE(INTERNAL_ERROR, "[Load][Task] fail, return ret:%u", result);
    return INTERNAL_ERROR;
  }
  GELOGI("HcclTaskInfo Distribute Success.");
  return SUCCESS;
}

Status HcclTaskInfo::CalculateArgs(const domi::TaskDef &task_def, DavinciModel *const davinci_model) {
  GE_CHECK_NOTNULL(davinci_model);
  const auto &hccl_def = task_def.kernel_hccl();
  const uint32_t op_index = hccl_def.op_index();
  GELOGI("HcclTaskInfo Init, op_index is: %u", op_index);
  // Get HCCL op
  const auto op_desc = davinci_model->GetOpByIndex(op_index);
  GE_CHECK_NOTNULL(op_desc);
  GELOGI("Calc opType[%s] args size. Node name is [%s]", op_desc->GetType().c_str(), op_desc->GetName().c_str());
  // Only need the number of addr to allocate args memory
  const auto input_size = op_desc->GetInputsSize();
  const auto output_size = op_desc->GetOutputsSize();
  const auto workspace_size = op_desc->GetWorkspaceBytes().size();
  const uint32_t args_size = static_cast<uint32_t>(sizeof(void *) * (input_size + output_size + workspace_size));
  args_offset_ = davinci_model->GetTotalArgsSize();
  davinci_model->SetTotalArgsSize(args_size);
  GELOGI("Calculate hccl task args , args_size %u, args_offset %u", args_size, args_offset_);
  return SUCCESS;
}

void HcclTaskInfo::SetIoAddrs(const OpDescPtr &op_desc) {
  const RuntimeParam &rts_param = davinci_model_->GetRuntimeParam();
  const auto input_data_addrs = ModelUtils::GetInputAddrs(rts_param, op_desc);
  const auto output_data_addrs = ModelUtils::GetOutputAddrs(rts_param, op_desc);
  const auto workspace_data_addrs = ModelUtils::GetWorkspaceDataAddrs(rts_param, op_desc);
  (void)io_addrs_.insert(io_addrs_.cend(), input_data_addrs.cbegin(), input_data_addrs.cend());
  (void)io_addrs_.insert(io_addrs_.cend(), output_data_addrs.cbegin(), output_data_addrs.cend());
  (void)io_addrs_.insert(io_addrs_.cend(), workspace_data_addrs.cbegin(), workspace_data_addrs.cend());
}

Status HcclTaskInfo::UpdateArgs() {
  GELOGI("HcclTaskInfo::UpdateArgs in.");
  davinci_model_->SetTotalIOAddrs(io_addrs_);
  GELOGI("HcclTaskInfo::UpdateArgs success.");
  return SUCCESS;
}

Status HcclTaskInfo::SetAddrs(const std::shared_ptr<OpDesc> &op_desc,
                              std::vector<GETaskKernelHcclInfo> &kernel_hccl_infos) {
  GE_CHECK_NOTNULL(op_desc);
  GE_CHK_STATUS_RET(HcomOmeUtil::CheckKernelHcclInfo(op_desc, kernel_hccl_infos),
                    "[Check][Param] HcomOmeUtil:: the number of GETaskKernelHcclInfo is invalid, node:%s(%s).",
                    op_desc->GetName().c_str(), op_desc->GetType().c_str());
  GELOGI("Set hccl task input output address, node[%s], type[%s] kernel_hccl_infos.size[%zu].",
         op_desc->GetName().c_str(), op_desc->GetType().c_str(), kernel_hccl_infos.size());
  if (op_desc->GetType() == HVDWAIT) {
    return SUCCESS;
  }

  HcclReduceOp op_type = HCCL_REDUCE_SUM;
  GE_CHECK_NOTNULL(davinci_model_);
  GELOGI("Calc opType[%s] input address before. Node name[%s]", op_desc->GetType().c_str(), op_desc->GetName().c_str());
  std::vector<void *> input_data_addrs;
  std::vector<void *> output_data_addrs;
  if (!davinci_model_->IsKnownNode()) {
    input_data_addrs = ModelUtils::GetInputAddrs(davinci_model_->GetRuntimeParam(), op_desc);
    output_data_addrs = ModelUtils::GetOutputAddrs(davinci_model_->GetRuntimeParam(), op_desc);
  }
  void *input_data_addr = nullptr;
  void *output_data_addr = nullptr;
  // initialize every kernel_hccl_info inputDataAddr
  for (size_t i = 0U; i < kernel_hccl_infos.size(); i++) {
    const std::string hccl_type = kernel_hccl_infos[i].hccl_type;
    if (davinci_model_->IsKnownNode()) {
      input_data_addr = ValueToPtr(PtrToValue(args_) + (i * sizeof(uint64_t)));
      output_data_addr = ValueToPtr(PtrToValue(args_) + ((op_desc->GetInputsSize() + i) * sizeof(uint64_t)));
      GELOGI("Hccl task info known input addr %p, output addr %p.", input_data_addr, output_data_addr);
    } else {
      input_data_addr = input_data_addrs.empty() ? nullptr : input_data_addrs[i];
      output_data_addr = output_data_addrs.empty() ? nullptr : output_data_addrs[i];
    }
    kernel_hccl_infos[i].inputDataAddr = input_data_addr;
    const bool reduceFlag = ((hccl_type == HCOMALLREDUCE) || (hccl_type == HCOMREDUCESCATTER) ||
                            (hccl_type == HVDCALLBACKALLREDUCE) || (hccl_type == HCOMREDUCE));
    if ((hccl_type == HCOMALLGATHER) || (hccl_type == HCOMRECEIVE) ||
        (hccl_type == HVDCALLBACKALLGATHER) || (hccl_type == HCOMALLTOALLV)) {
      kernel_hccl_infos[i].outputDataAddr = output_data_addr;
    } else if (reduceFlag) {
      GE_CHK_STATUS_RET(HcomOmeUtil::GetHcclOperationType(op_desc, op_type),
                        "[Get][HcomOperationType] fail! op:%s", op_desc->GetName().c_str());
      kernel_hccl_infos[i].outputDataAddr = output_data_addr;
      kernel_hccl_infos[i].opType = op_type;
    } else {
      // do nothing
    }
    davinci_model_->DisableZeroCopy(input_data_addr);
  }
  return SUCCESS;
}

void HcclTaskInfo::TransToGETaskInfo(GETaskInfo &ge_task) const {
  ge_task.id = id_;
  ge_task.type = static_cast<uint16_t>(RT_MODEL_TASK_HCCL);
  ge_task.stream = stream_;
  ge_task.kernelHcclInfo = kernel_hccl_infos_;
  ge_task.privateDef = private_def_;
  ge_task.privateDefLen = private_def_len_;
  ge_task.opsKernelStorePtr = ops_kernel_store_;
  for (size_t i = 0U; i < ge_task.kernelHcclInfo.size(); i++) {
    ge_task.kernelHcclInfo[i].hcclStreamList = hccl_stream_list_;
  }
}

void HcclTaskInfo::GetPrivateDefByTaskDef(const OpDescPtr &op_desc, const domi::TaskDef &task) {
  // Get privateDef and opsKernelStorePtr from taskDef and save them in taskInfo
  GELOGI("get custom info in modelTaskDef.");
  ops_kernel_store_ = op_desc->TryGetExtAttr<OpsKernelExecutor *>("OpsKernelInfoStorePtr", nullptr);
  if ((ops_kernel_store_ == nullptr) &&
      (OpsKernelExecutorManager::GetInstance().GetExecutor(op_desc->GetOpKernelLibName(), ops_kernel_store_) !=
          SUCCESS)) {
    return;
  }
  const std::string &private_def_temp = task.private_def();
  if ((!private_def_temp.empty()) && (private_def_temp.size() <= static_cast<size_t>(UINT32_MAX))) {
    private_def_len_ = static_cast<uint32_t>(private_def_temp.size());
    GE_CHK_RT_EXEC(rtMallocHost(&private_def_, static_cast<uint64_t>(private_def_len_)), return);
    GE_CHK_RT_EXEC(rtMemcpy(private_def_, static_cast<uint64_t>(private_def_len_), task.private_def().c_str(),
                   static_cast<uint64_t>(private_def_len_), RT_MEMCPY_HOST_TO_HOST), return);
    GELOGI("The first address of the custom info, privateDef=%p.", private_def_);
  }
}

void HcclTaskInfo::CreateKernelHcclInfo(const ConstOpDescPtr &op_desc) {
  GE_CHECK_NOTNULL_JUST_RETURN(op_desc);
  if (HcomOmeUtil::IsHCOMOp(op_desc->GetType())) {
    GETaskKernelHcclInfo kernel_hccl_info;
    kernel_hccl_infos_.emplace_back(kernel_hccl_info);
  } else if (HcomOmeUtil::IsHorovodOp(op_desc->GetType())) {
    // Horovod wait do not have any input, but create a GETaskKernelHcclInfo to record hccl_type.
    // Other Operator need to check that the number of GETaskKernelHcclInfo must equals to number of inputs
    if (op_desc->GetType() == HVDWAIT) {
      GETaskKernelHcclInfo kernel_hccl_info;
      kernel_hccl_infos_.emplace_back(kernel_hccl_info);
      return;
    }
    for (size_t i = 0U; i < op_desc->GetInputsSize(); i++) {
      GETaskKernelHcclInfo kernel_hccl_info;
      kernel_hccl_infos_.emplace_back(kernel_hccl_info);
    }
  } else {
    // do nothing
  }
}

Status HcclTaskInfo::SetWorkspace(const std::shared_ptr<OpDesc> &op_desc,
                                  std::vector<GETaskKernelHcclInfo> &kernel_hccl_infos) {
  GE_CHECK_NOTNULL(op_desc);
  GE_CHECK_NOTNULL(davinci_model_);
  GELOGI("SetWorkspace Node[%s] opType[%s] set workspace.", op_desc->GetName().c_str(), op_desc->GetType().c_str());
  uint64_t workspace_mem_size = 0U;
  void *workspace_addr = nullptr;
  const auto workspace_bytes = op_desc->GetWorkspaceBytes();
  if (!workspace_bytes.empty()) {
    const uint64_t workspace_mem_size_tmp = static_cast<uint64_t>(workspace_bytes[0U]);
    GELOGI("hccl need workSpaceMemSize=%lu", workspace_mem_size_tmp);
    if (workspace_mem_size_tmp != 0U) {
      workspace_mem_size = workspace_mem_size_tmp;
      if (davinci_model_->IsKnownNode()) {
        workspace_addr = ValueToPtr(PtrToValue(args_) +
                                    ((op_desc->GetInputsSize() + op_desc->GetOutputsSize()) * sizeof(uint64_t)));
      } else {
        const auto workspace_data_addrs = ModelUtils::GetWorkspaceDataAddrs(davinci_model_->GetRuntimeParam(), op_desc);
        workspace_addr = workspace_data_addrs.empty() ? nullptr : workspace_data_addrs[0U];
      }
    }
  }
  for (size_t i = 0U; i < kernel_hccl_infos.size(); i++) {
    kernel_hccl_infos[i].workSpaceMemSize = workspace_mem_size;
    kernel_hccl_infos[i].workSpaceAddr = workspace_addr;
  }
  return SUCCESS;
}

Status HcclTaskInfo::Release() {
#ifndef ONLY_COMPILE_OPEN_SRC
  GELOGI("HcclTaskInfo unload Start. begin to call function unloadTask in hccl.");
  if (ops_kernel_store_ == nullptr) {
    REPORT_INNER_ERROR("E19999", "Check param ops_kernel_store_ nullptr");
    GELOGE(INTERNAL_ERROR, "[Check][Param] ops kernel store is null.");
    return INTERNAL_ERROR;
  }
  GETaskInfo ge_task;
  TransToGETaskInfo(ge_task);
  const auto result = ops_kernel_store_->UnloadTask(ge_task);
  if (result != HCCL_SUCCESS) {
    REPORT_CALL_ERROR("E19999", "Call ops_kernel_info_store unloadTask fail");
    GELOGE(INTERNAL_ERROR, "[UnLoad][Task] fail, return ret:%u", result);
    return INTERNAL_ERROR;
  }
#endif
  GELOGI("HcclTaskInfo unload Success.");
  return SUCCESS;
}

REGISTER_TASK_INFO(RT_MODEL_TASK_HCCL, HcclTaskInfo);
}  // namespace ge
