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

#include "graph/load/model_manager/task_info/ffts_plus_task_info.h"

#include "graph/load/model_manager/task_info/ffts_plus_proto_transfer.h"
#include "graph/load/model_manager/model_manager.h"
#include "graph/load/model_manager/model_utils.h"
#include "aicpu/common/aicpu_task_struct.h"

namespace {
const std::string kAttrNameCachePst = "_cache_persist"; // temp code, wait for metadef code merge
}

namespace ge {
FftsPlusTaskInfo::~FftsPlusTaskInfo() {
}

Status FftsPlusTaskInfo::Init(const domi::TaskDef &task_def, DavinciModel *const davinci_model) {
  GELOGI("Init FftsPlusTaskInfo Start");
  fusion_op_info_.clear();
  GE_CHECK_NOTNULL(davinci_model);
  davinci_model_ = davinci_model;
  GE_CHK_STATUS_RET_NOLOG(SetStream(task_def.stream_id(), davinci_model_->GetStreamList()));

  const domi::FftsPlusTaskDef &ffts_plus_task_def = task_def.ffts_plus_task();
  const auto op_desc = davinci_model_->GetOpByIndex(ffts_plus_task_def.op_index());
  GE_CHECK_NOTNULL(op_desc);
  const int32_t ctx_num = ffts_plus_task_def.ffts_plus_ctx_size();
  for (int32_t i = 0; i < ctx_num; ++i) {
    const domi::FftsPlusCtxDef &ctx_def = ffts_plus_task_def.ffts_plus_ctx(i);
    const OpDescPtr sub_op_desc = davinci_model_->GetOpByIndex(ctx_def.op_index());
    if (sub_op_desc != nullptr) {
      GE_CHK_STATUS_RET(SetCachePersistentWay(sub_op_desc), "[Call][SetCachePersistentWay] failed, node:%s.",
                        sub_op_desc->GetName().c_str());
    }
  }

  const auto &rts_param = davinci_model_->GetRuntimeParam();
  const auto ffts_run_addr_handle = [&rts_param](const uintptr_t logic_addr, uint8_t *&mem_addr) -> Status {
    return ModelUtils::GetRtAddress(rts_param, logic_addr, mem_addr);
  };

  const auto ffts_addr_pref_handle = [&davinci_model](const std::string &kernel_name, void *&addr, uint32_t &pref_cnt) {
    return davinci_model->GetAddrAndPrefCnt(kernel_name, addr, pref_cnt);
  };

  const auto ffts_find_node_handle = [&davinci_model](const uint32_t index) {
    return davinci_model->GetOpByIndex(index);
  };

  const auto ffts_save_ctx_args_handle = [this](const OpDescPtr &descriptor, const size_t args_offset) {
    InitDumpArgs(descriptor, args_offset);
  };

  const auto ffts_get_session_id = [this]() {
    return davinci_model_->GetSessionId();
  };

  const auto ffts_create_aicpu_session = [this](STR_FWK_OP_KERNEL &fwk_op_kernel) {
    return this->CreateAicpuSession(fwk_op_kernel);
  };

  const auto ffts_load_cust_aicpu_so = [this](const OpDescPtr &op_desc, const domi::FftsPlusAicpuCtxDef &ctx_def) {
    return this->LoadCustAicpuSo(op_desc, ctx_def);
  };

  args_size_ = sizeof(void *) * ffts_plus_task_def.addr_size();
  if (args_size_ > 0U) {
    GE_CHK_RT_RET(rtMalloc(&args_, args_size_, RT_MEMORY_HBM));
    GE_CHECK_NOTNULL(args_);
  }

  FftsPlusProtoTransfer ffts_proto_transfer(PtrToValue(args_), io_addrs_, davinci_model_->GetRuntimeParam(),
                                            ext_info_addrs_, mode_addr_idx_);
  ffts_proto_transfer.SetRunAddrHandle(ffts_run_addr_handle);
  ffts_proto_transfer.SetAddrPrefHandle(ffts_addr_pref_handle);
  ffts_proto_transfer.SetFindNodeHandle(ffts_find_node_handle);
  ffts_proto_transfer.SetSaveCtxArgsHandle(ffts_save_ctx_args_handle);
  ffts_proto_transfer.SetGetSessionId(ffts_get_session_id);
  ffts_proto_transfer.SetCreateAicpuSession(ffts_create_aicpu_session);
  ffts_proto_transfer.SetLoadCustAicpuSo(ffts_load_cust_aicpu_so);

  GE_CHK_STATUS_RET_NOLOG(ffts_proto_transfer.Transfer(op_desc, ffts_plus_task_def, ffts_plus_task_info_));
  fusion_op_info_ = ffts_proto_transfer.GetAllFusionOpInfo();

  if (args_size_ < (sizeof(void *) * io_addrs_.size())) {
    REPORT_INNER_ERROR("E19999", "addr_size %d of FftsPlusTaskInfo is less than number of task_addr %zu",
                       ffts_plus_task_def.addr_size(), io_addrs_.size());
    GELOGE(FAILED, "[Check][Param] addr_size %d of FftsPlusTaskInfo is less than number of task_addr %zu",
           ffts_plus_task_def.addr_size(), io_addrs_.size());
    return FAILED;
  }

  GE_CHK_STATUS_RET_NOLOG(UpdateArgs());
  GELOGI("Init FftsPlusTaskInfo success, node: %s", op_desc->GetName().c_str());
  return SUCCESS;
}

Status FftsPlusTaskInfo::CreateAicpuSession(STR_FWK_OP_KERNEL &fwk_op_kernel) const {
  // 0 Global Step
  fwk_op_kernel.fwkKernelBase.fwk_kernel.stepIDAddr = davinci_model_->GetGlobalStep();

  // 1 Session Id
  const uint64_t session_id = davinci_model_->GetSessionId();
  fwk_op_kernel.fwkKernelBase.fwk_kernel.sessionID = session_id;

  // 2 Collect aicpu kernel
  fwk_op_kernel.fwkKernelBase.fwk_kernel.kernelID = hybrid::AicpuExtInfoHandler::GenerateKernelId();
  ModelManager::GetInstance().CreateAicpuKernel(session_id, davinci_model_->Id(), davinci_model_->SubModelId(),
                                                fwk_op_kernel.fwkKernelBase.fwk_kernel.kernelID);

  // 3 Create session
  const auto ret = ModelManager::GetInstance().CreateAicpuSession(session_id);
  if (ret != SUCCESS) {
    REPORT_CALL_ERROR("E19999", "CreateAicpuSession fail, session_id:%lu", session_id);
    GELOGE(ret, "[Create][AicpuSession] error. session id:%lu", session_id);
    return ret;
  }
  return SUCCESS;
}

Status FftsPlusTaskInfo::LoadCustAicpuSo(const OpDescPtr &op_desc, const domi::FftsPlusAicpuCtxDef &ctx_def) const {
  bool loaded = false;
  const auto &kernel = ctx_def.kernel();
  auto &model_mgr = ModelManager::GetInstance();
  GE_CHK_STATUS_RET(model_mgr.LoadCustAicpuSo(davinci_model_->GetCustAICPUKernel(op_desc), kernel.so_name(), loaded),
                    "[Launch][CustAicpuSo] failed.");
  return SUCCESS;
}

Status FftsPlusTaskInfo::CalculateArgs(const domi::TaskDef &task_def, DavinciModel *const davinci_model) {
  (void)task_def;
  (void)davinci_model;
  return SUCCESS;
}

Status FftsPlusTaskInfo::UpdateArgs() {
  GE_CHECK_NOTNULL(davinci_model_);
  if (io_addrs_.empty()) {
    GELOGI("Init FftsPlusTaskInfo success, do not have io addrs");
    return SUCCESS;
  }

  if ((args_ == nullptr) || (args_size_ < (sizeof(void *) * io_addrs_.size()))) {
    REPORT_INNER_ERROR("E19999", "Invalid agrs: args size: %lu, adds size: %zu", args_size_, io_addrs_.size());
    GELOGE(FAILED, "[Check][Param] Invalid agrs: args size: %lu, adds size: %zu", args_size_, io_addrs_.size());
    return INTERNAL_ERROR;
  }

  std::vector<void *> io_addrs(io_addrs_.size(), nullptr);
  const auto &rts_param = davinci_model_->GetRuntimeParam();
  const auto addr_size = sizeof(void *) * io_addrs_.size();
  for (size_t i = 0U; i < io_addrs_.size(); ++i) {
    if (mode_addr_idx_.count(i) > 0U) {
      io_addrs[i] = ValueToPtr(io_addrs_[i]);
      GELOGD("addr at idx:%zu is real addr", i);
      continue;
    }
    uint8_t *mem_addr = nullptr;
    if (ModelUtils::GetRtAddress(rts_param, io_addrs_[i], mem_addr) != SUCCESS) {
      GELOGE(INTERNAL_ERROR, "[Check][GetRtAddress] failed, logic addr is 0x%lx.", io_addrs_[i]);
      return INTERNAL_ERROR;
    }
    io_addrs[i] = mem_addr;
  }

  GELOGI("Memcpy to addr %p, addr_size=%lu, len=%lu", args_, args_size_, addr_size);
  GE_CHK_RT_RET(rtMemcpy(args_, args_size_, io_addrs.data(), addr_size, RT_MEMCPY_HOST_TO_DEVICE));
  return SUCCESS;
}

Status FftsPlusTaskInfo::Distribute() {
  GELOGI("FftsPlusTaskInfo Distribute Start.");
  GE_CHK_RT_RET(rtFftsPlusTaskLaunchWithFlag(&ffts_plus_task_info_, stream_, dump_flag_));
  GE_CHECK_NOTNULL(davinci_model_);
  GE_CHK_RT_RET(rtModelGetTaskId(davinci_model_->GetRtModelHandle(), &task_id_, &stream_id_));
  GELOGI("FftsPlusTaskInfo Distribute Success. task id is %u, stream id is %u.", task_id_, stream_id_);
  return SUCCESS;
}

Status FftsPlusTaskInfo::Release() {
  GE_FREE_RT_LOG(args_);
  for (auto &ext_info_addr : ext_info_addrs_) {
    GE_FREE_RT_LOG(ext_info_addr);
  }

  CleanRtFftsPlusTask(ffts_plus_task_info_);
  return SUCCESS;
}

Status FftsPlusTaskInfo::SetCachePersistentWay(const OpDescPtr &op_desc) const {
  const vector<void *> input_addrs = ModelUtils::GetInputDataAddrs(davinci_model_->GetRuntimeParam(), op_desc);
  for (size_t i = 0U; i < op_desc->GetAllInputsSize(); ++i) {
    const GeTensorDescPtr tensor_desc = op_desc->MutableInputDesc(static_cast<uint32_t>(i));
    if (tensor_desc == nullptr) {
      continue;
    }
    uint32_t persistent_id = static_cast<uint32_t>(UINT_MAX);
    (void)AttrUtils::GetInt(tensor_desc, kAttrNameCachePst, persistent_id);
    if (persistent_id != static_cast<uint32_t>(UINT_MAX)) {
      int64_t tensor_size = 0;
      (void)TensorUtils::GetSize(*tensor_desc, tensor_size);
      const uint32_t advise = 0U;
      GE_CHK_RT(rtMemAdvise(input_addrs[i], static_cast<uint64_t>(tensor_size), advise));
    }
  }
  return SUCCESS;
}

void FftsPlusTaskInfo::InitDumpArgs(const OpDescPtr &op_desc, const size_t args_offset) {
  if (davinci_model_->OpNeedDump(op_desc->GetName())) {
    GELOGD("Op:%s(%s) need dump in ffts plus task info", op_desc->GetName().c_str(), op_desc->GetType().c_str());
    dump_flag_ = RT_KERNEL_DUMPFLAG;
  }

  if (davinci_model_->GetOpDugReg() || davinci_model_->OpNeedDump(op_desc->GetName())) {
    if ((dump_args_offset_.count(op_desc->GetName()) == 0U)) {
      (void)dump_args_offset_.emplace(op_desc->GetName(), args_offset);
    }
  }
}

uintptr_t FftsPlusTaskInfo::FindDumpArgs(const std::string &op_name) const {
  const std::map<std::string, size_t>::const_iterator iter = dump_args_offset_.find(op_name);
  if (iter != dump_args_offset_.end()) {
    return (PtrToValue(args_) + (sizeof(void *) * iter->second));
  }
  GELOGD("op:%s not save args", op_name.c_str());
  return 0U;
}

void FftsPlusTaskInfo::PostProcess(const domi::TaskDef &task_def) {
  const domi::FftsPlusTaskDef &ffts_plus_task_def = task_def.ffts_plus_task();
  const int32_t ctx_num = ffts_plus_task_def.ffts_plus_ctx_size();
  // Dump times of the same operator, only one time is allowed
  std::set<std::string> dump_op_set;
  for (int32_t i = 0; i < ctx_num; ++i) {
    const domi::FftsPlusCtxDef &ctx_def = ffts_plus_task_def.ffts_plus_ctx(i);
    const OpDescPtr op_desc = davinci_model_->GetOpByIndex(ctx_def.op_index());
    if (op_desc == nullptr) {
      GELOGD("ctx op is nullptr, ctx id:%u, ctx type:%u, op index:%u",
             ctx_def.context_id(), ctx_def.context_type(), ctx_def.op_index());
      continue;
    }
    // Dump times of the same op, only one time is allowed
    if (dump_op_set.count(op_desc->GetName()) > 0U) {
      GELOGD("ctx op is save, ctx id:%u, ctx type:%u, op index:%u",
             ctx_def.context_id(), ctx_def.context_type(), ctx_def.op_index());
      continue;
    }
    if (FindDumpArgs(op_desc->GetName()) != 0U) {
      const bool call_dump = davinci_model_->OpNeedDump(op_desc->GetName()) && CallSaveDumpInfo();
      if (call_dump || davinci_model_->GetOpDugReg()) {
        davinci_model_->SaveDumpTask(task_id_, stream_id_, op_desc, FindDumpArgs(op_desc->GetName()));
        GELOGD("save ctx op, ctx id:%u, ctx type:%u, op index:%u, task id:%u, stream id:%u", ctx_def.context_id(),
               ctx_def.context_type(), ctx_def.op_index(), task_id_, stream_id_);
        (void)dump_op_set.emplace(op_desc->GetName());
      }
    }
    davinci_model_->SaveDumpOpInfo(op_desc, task_id_, stream_id_);
  }

  davinci_model_->SaveFftsPlusProfilingTask(task_def, *this);
}

REGISTER_TASK_INFO(RT_MODEL_TASK_FFTS_PLUS_TASK, FftsPlusTaskInfo);
}  // namespace ge
