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

#include "graph/load/model_manager/task_info/kernel_task_info.h"

#include "aicpu/common/aicpu_task_struct.h"
#include "graph/load/model_manager/model_manager.h"
#include "graph/load/model_manager/model_utils.h"
#include "graph/load/model_manager/task_info/super_kernel/super_kernel.h"
#include "graph/load/model_manager/task_info/super_kernel/super_kernel_factory.h"

namespace {
const uint8_t kL2LoadToDdr = 1U;
const uint8_t kL2NotLoadToDdr = 0U;
// for skt
constexpr int64_t kInvalidGroupKey = -1;
constexpr uint32_t kSKTSingleSize = 1U;
const std::string kIsLastNode = "is_last_node";
const std::string kAllShapeInAicpu = "_AllShape";
constexpr int64_t kCloseSkt = 100;
constexpr uint32_t kAddressLen = static_cast<uint32_t>(sizeof(void *));
constexpr int32_t kBaseInt = 10;
constexpr int32_t kStrtolFail = 0;
constexpr size_t kArgsInputDesc = 0U;
constexpr size_t kArgsInputAddr = 1U;
constexpr size_t kArgsOutputDesc = 2U;
constexpr size_t kArgsOutputAddr = 3U;
constexpr size_t kArgsAttrHandle = 4U;
}  // namespace

namespace ge {
Status KernelTaskInfo::Init(const domi::TaskDef &task_def, DavinciModel *const davinci_model) {
  GE_CHECK_NOTNULL(davinci_model);
  davinci_model_ = davinci_model;
  GE_CHK_STATUS_RET_NOLOG(SetStream(task_def.stream_id(), davinci_model_->GetStreamList()));

  fusion_op_info_.clear();
  is_l1_fusion_enable_ = davinci_model_->GetL1FusionEnableOption();
  GELOGD("KernelTaskInfo init start, ge.enableL1Fusion in davinci model is %d.",
         static_cast<int32_t>(is_l1_fusion_enable_));

  const domi::KernelDef &kernel_def = task_def.kernel();
  // Old model will not take this value, its default value is 0,need to convert to the real default value 1.
  block_dim_ = (kernel_def.block_dim() == 0U) ? 1U : kernel_def.block_dim();
  args_size_ = static_cast<uint32_t>(kernel_def.args().size());

  const domi::KernelContext &context = kernel_def.context();
  kernel_type_ = static_cast<ccKernelType>(context.kernel_type());
  op_desc_ = davinci_model_->GetOpByIndex(context.op_index());
  GE_CHECK_NOTNULL(op_desc_);
  (void)AttrUtils::GetBool(op_desc_, ATTR_N_BATCH_SPILT, is_n_batch_spilt_);
  GELOGD("node[%s] is_n_batch_spilt %d", op_desc_->GetName().c_str(), static_cast<int32_t>(is_n_batch_spilt_));
  (void)AttrUtils::GetInt(op_desc_, ATTR_NAME_FUSION_GROUP_KEY, group_key_);
  has_group_key_ = (group_key_ != kInvalidGroupKey);
  GELOGD("node[%s] has_group_key_ %d, group key is [%ld]", op_desc_->GetName().c_str(),
         static_cast<int32_t>(has_group_key_), group_key_);

  // fusion_op_info
  std::vector<std::string> original_op_names;
  (void)AttrUtils::GetListStr(op_desc_, ATTR_NAME_DATA_DUMP_ORIGIN_OP_NAMES, original_op_names);
  if (!original_op_names.empty()) {
    FusionOpInfo fusion_op_info;
    fusion_op_info.stream_id = task_def.stream_id();
    fusion_op_info.op_index = context.op_index();
    fusion_op_info.original_op_names = original_op_names;
    fusion_op_info.op_name = op_desc_->GetName();
    fusion_op_info_.emplace_back(fusion_op_info);
  }

  if (context.origin_op_index_size() > CC_FUSION_OP_MAX) {
    REPORT_INNER_ERROR("E19999", "context.origin_op_index_size():%d is more than CC_FUSION_OP_MAX(%d), op:%s(%s), "
                       "check invalid", context.origin_op_index_size(), CC_FUSION_OP_MAX,
                       op_desc_->GetName().c_str(), op_desc_->GetType().c_str());
    GELOGE(PARAM_INVALID, "[Check][Param] context.origin_op_index_size():%d is more than CC_FUSION_OP_MAX(%d), "
           "op:%s(%s)", context.origin_op_index_size(), CC_FUSION_OP_MAX,
           op_desc_->GetName().c_str(), op_desc_->GetType().c_str());
    return PARAM_INVALID;
  }

  ctx_.opIndex = context.op_index();
  ctx_.opCount = static_cast<uint32_t>(context.origin_op_index_size());
  for (int32_t i = 0; i < context.origin_op_index_size(); ++i) {
    ctx_.opIndex2[static_cast<size_t>(i)] = context.origin_op_index(i);
  }

  InitDumpFlag();
  Status ret = FAILED;
  if (kernel_type_ == ccKernelType::TE) {
    ret = InitTVMTask(op_desc_, kernel_def);
  } else if (kernel_type_ == ccKernelType::CUSTOMIZED) {
    ret = InitAICPUCustomTask(op_desc_, kernel_def);
  } else if ((kernel_type_ == ccKernelType::AI_CPU) || (kernel_type_ == ccKernelType::CUST_AI_CPU)) {
    ret = InitAicpuTask(op_desc_, kernel_def);
  } else {
    REPORT_INNER_ERROR("E19999", "Node op:%s(%s) kernel type invalid", op_desc_->GetName().c_str(),
                       op_desc_->GetType().c_str());
    GELOGE(FAILED, "[Check][Param] Node op:%s(%s) kernel type invalid", op_desc_->GetName().c_str(),
           op_desc_->GetType().c_str());
    return ret;
  }

  globalworkspace_overflow_addr_ = davinci_model_->GetOverflowAddr();
  SetIoAddrs(op_desc_);
  GELOGD("KernelTaskInfo init finish, result=%u.", ret);
  return ret;
}

Status KernelTaskInfo::SaveSKTDumpInfo() {
  GE_CHECK_NOTNULL(davinci_model_);
  if (skt_dump_flag_ == RT_KERNEL_DEFAULT) {
    GELOGD("no need save skt dump info");
    return SUCCESS;
  }
  // all op in super kernel share one taskid and streamid
  const SuperKernelTaskInfo &skt_info = davinci_model_->GetSuperKernelTaskInfo();
  for (size_t i = 0U; i < skt_info.op_desc_list.size(); i++) {
    davinci_model_->SaveDumpTask(skt_info.last_task_id, skt_info.last_stream_id, skt_info.op_desc_list[i],
                                 skt_info.dump_args_list[i]);
  }
  return SUCCESS;
}

void KernelTaskInfo::UpdateSKTTaskId() {
  if (davinci_model_ != nullptr) {
    davinci_model_->SuperKernelUpdateTaskId(skt_id_);
  }
}

void KernelTaskInfo::UpdateTaskId() {
  if (davinci_model_ != nullptr) {
    GE_CHK_RT_EXEC(rtModelGetTaskId(davinci_model_->GetRtModelHandle(), &task_id_, &stream_id_), return);
    GELOGD("UpdateTaskId:UpdateTaskId [%u], stream id [%u]:", task_id_, stream_id_);
  }
}

Status KernelTaskInfo::SKTFinalize() {
  UpdateSKTTaskId();
  GE_CHK_STATUS_RET(SaveSKTDumpInfo(), "[Save][SKTDumpInfo] failed");
  GELOGI("SuperKernel Distribute [skt_id:%u]", skt_id_);
  davinci_model_->SuperKernelFinalize(sm_desc_, kInvalidGroupKey);
  return SUCCESS;
}

uint32_t KernelTaskInfo::GetDumpFlag() const {
  const SuperKernelTaskInfo &skt_info = davinci_model_->GetSuperKernelTaskInfo();
  for (const auto flag : skt_info.dump_flag_list) {
    if (flag == RT_KERNEL_DUMPFLAG) {
      return RT_KERNEL_DUMPFLAG;
    }
  }
  return RT_KERNEL_DEFAULT;
}

Status KernelTaskInfo::SuperKernelLaunch() {
  const SuperKernelTaskInfo &skt_info = davinci_model_->GetSuperKernelTaskInfo();
  if (skt_info.kernel_list.empty()) {
    GELOGI("SuperKernelLaunch: Skt_kernel_list has no task, just return");
    return SUCCESS;
  }

  auto &skt_kernel_list = skt_info.kernel_list;
  auto &skt_arg_list = skt_info.arg_list;
  GELOGI("SuperKernelLaunch: Skt_kernel_list size[%zu] skt_arg_list[%zu]", skt_kernel_list.size(), skt_arg_list.size());
  if ((skt_kernel_list.size() == kSKTSingleSize) && (skt_arg_list.size() == kSKTSingleSize)) {
    SetTaskTag(op_desc_->GetName().c_str());
    rtArgsEx_t args_ex = {};
    args_ex.args = skt_info.arg_list.front();
    args_ex.argsSize = skt_info.last_args_size;
    GE_CHK_RT_RET(rtKernelLaunchWithFlag(
        skt_info.kernel_list.front(), static_cast<uint32_t>(skt_info.last_block_dim), &args_ex,
        PtrToPtr<void, rtSmDesc_t>(skt_info.last_sm_desc), skt_info.last_stream, skt_info.last_dump_flag));
    call_save_dump_ = true;
    GE_CHK_STATUS_RET(SKTFinalize(), "[Call][SKTFinalize] failed");
    return SUCCESS;
  }
  // Create super kernel factory
  skt::SuperKernelFactory *const factory = &skt::SuperKernelFactory::GetInstance();
  // Init super kernel factory
  Status ge_ret = factory->Init();
  if (ge_ret != SUCCESS) {
    REPORT_CALL_ERROR("E19999", "Call SuperKernelFactory init fail, ret:0x%X", ge_ret);
    GELOGE(ge_ret, "[Init][SuperKernelFactory] failed, ret:0x%X", ge_ret);
    return ge_ret;
  }
  // Call the fuse API
  std::unique_ptr<skt::SuperKernel> superKernel = nullptr;
  ge_ret = factory->FuseKernels(skt_kernel_list, skt_arg_list, skt_info.last_block_dim, superKernel);
  if (ge_ret != SUCCESS) {
    REPORT_CALL_ERROR("E19999", "Call SuperKernelFactory FuseKernels fail, ret:0x%X", ge_ret);
    GELOGE(ge_ret, "[Call][FuseKernels] failed, ret:0x%X", ge_ret);
    return ge_ret;
  }
  // Launch a super kernel
  skt_dump_flag_ = GetDumpFlag();
  ge_ret = superKernel->Launch(skt_info.last_stream, skt_dump_flag_, op_desc_->GetName().c_str());
  if (ge_ret != SUCCESS) {
    REPORT_CALL_ERROR("E19999", "Call SuperKernelFactory Launch fail, ret:0x%X", ge_ret);
    GELOGE(ge_ret, "[Call][Launch] failed, ret:0x%X", ge_ret);
    return ge_ret;
  }
  GELOGI("SuperKernelLaunch: success[skt_kernel_list size[%zu] skt_arg_list[%zu]]", skt_kernel_list.size(),
         skt_arg_list.size());
  // record skt addr for release
  superkernel_dev_nav_table_ = superKernel->GetNavTablePtr();
  superkernel_device_args_addr_ = superKernel->GetDeviceArgsPtr();
  GE_CHK_STATUS_RET(SKTFinalize(), "[Call][SKTFinalize] failed");
  return SUCCESS;
}

Status KernelTaskInfo::SaveSuperKernelInfo() {
  const SktTaskInfo task_info {
      stream_, sm_desc_, stub_func_, args_, args_size_, block_dim_, group_key_, dump_flag_, PtrToValue(skt_dump_args_)};
  davinci_model_->SuperKernelSaveTask(op_desc_, task_info);

  // last node in a stream, just launch
  if (IsMarkedLastNode()) {
    return SuperKernelLaunch();
  }

  return SUCCESS;
}

bool KernelTaskInfo::IsMarkedLastNode() const {
  if (davinci_model_ == nullptr) {
    REPORT_INNER_ERROR("E19999", "Check param davinci_model nullptr");
    GELOGE(PARAM_INVALID, "[Check][Param] davinci_model is null!");
    return false;
  }
  const OpDescPtr op_desc = davinci_model_->GetOpByIndex(ctx_.opIndex);
  if (op_desc == nullptr) {
    REPORT_INNER_ERROR("E19999", "Can't get op_desc from davinci_model by index:%u", ctx_.opIndex);
    GELOGE(INTERNAL_ERROR, "[Get][Op] by index failed, index:%u is out of range!", ctx_.opIndex);
    return false;
  }
  bool is_last_node = false;
  (void)AttrUtils::GetBool(op_desc, kIsLastNode, is_last_node);
  return is_last_node;
}

// current task 's block dim and stream and grouo key (if have) must same with last task,
// then may be saved to skt task list; else
// call skt launch those saved tasks before
bool KernelTaskInfo::FirstCallSKTLaunchCheck() const {
  const SuperKernelTaskInfo &skt_info = davinci_model_->GetSuperKernelTaskInfo();
  return ((block_dim_ != skt_info.last_block_dim) || (stream_ != skt_info.last_stream) ||
          (has_group_key_ && (group_key_ != skt_info.last_group_key)));
}

// current task has group_id or has n ATTR_N_BATCH_SPLIT then save it to skt task list; else
// call skt launch those saved tasks and call rtlaunch for current task
bool KernelTaskInfo::DoubleCallSKTSaveCheck() const {
  return ((!is_n_batch_spilt_) && (!has_group_key_));
}

Status KernelTaskInfo::SuperKernelDistribute() {
  Status ret;
  if (FirstCallSKTLaunchCheck()) {
    ret = SuperKernelLaunch();
    if (ret != SUCCESS) {
      GELOGE(FAILED, "[Call][SuperKernelLaunch] failed, taskid:%u", task_id_);
      return FAILED;
    }
  }
  if (DoubleCallSKTSaveCheck()) {
    // 1.launch before
    ret = SuperKernelLaunch();
    if (ret != SUCCESS) {
      GELOGE(ret, "[Call][SuperKernelLaunch] failed, taskid:%u", task_id_);
      return ret;
    }
    // 2.launch current
    SetTaskTag(op_desc_->GetName().c_str());
    rtArgsEx_t args_ex = {};
    args_ex.args = args_;
    args_ex.argsSize = args_size_;
    GE_CHK_RT_RET(rtKernelLaunchWithFlag(stub_func_, block_dim_, &args_ex,
                                         PtrToPtr<void, rtSmDesc_t>(sm_desc_), stream_, dump_flag_));
    call_save_dump_ = true;
    UpdateTaskId();
    GELOGI("Current Common Task Distribute [taskid:%u]", task_id_);
  } else {
    ret = SaveSuperKernelInfo();
    if (ret != SUCCESS) {
      GELOGE(ret, "[Call][SaveSuperKernelInfo] failed, taskid:%u", task_id_);
      return ret;
    }
  }
  return SUCCESS;
}

void KernelTaskInfo::SetArgs() {
  if (davinci_model_->IsKnownNode()) {
    if (kernel_type_ == ccKernelType::TE) {
      args_ = l2_buffer_on_ ? davinci_model_->GetCurrentHybridArgsAddr(hybrid_args_offset_)
                            : davinci_model_->GetCurrentArgsAddr(args_offset_);
    } else {
      if ((kernel_type_ == ccKernelType::AI_CPU) || (kernel_type_ == ccKernelType::CUST_AI_CPU)) {
        args_ = davinci_model_->GetCurrentHybridArgsAddr(hybrid_args_offset_);
      }
    }
    GELOGI("Known node %s args addr %p, offset %u.", op_desc_->GetName().c_str(), args_, args_offset_);
  }
}

Status KernelTaskInfo::Distribute() {
  GELOGD("KernelTaskInfo Distribute Start.");
  SetArgs();

  ge::char_t skt_enable_env[MMPA_MAX_PATH] = {};
  const INT32 res = mmGetEnv("SKT_ENABLE", &skt_enable_env[0], static_cast<UINT32>(MMPA_MAX_PATH));
  const int64_t env_flag = (res == EN_OK) ? strtol(&skt_enable_env[0], nullptr, kBaseInt) : kStrtolFail;
  const bool call_skt = (env_flag != 0) || is_l1_fusion_enable_;
  const bool is_skt = (kernel_type_ == ccKernelType::AI_CPU) || (kernel_type_ == ccKernelType::CUST_AI_CPU);
  if (is_skt) {
    if (topic_type_flag_ > 0) {
      // Use the fifth and sixth bits of dump_flag_ indicate the value of topic_type.
      // xxxxxxxx xxxxxxxx xxxxxxxx xx00xxxx: DEVICE_ONLY
      // xxxxxxxx xxxxxxxx xxxxxxxx xx01xxxx: DEVICE_FIRST
      // xxxxxxxx xxxxxxxx xxxxxxxx xx10xxxx: HOST_ONLY
      // xxxxxxxx xxxxxxxx xxxxxxxx xx11xxxx: HOST_FIRST
      dump_flag_ = dump_flag_ | static_cast<uint32_t>(topic_type_flag_);
    }
    GELOGI("distribute task info kernel_type: %d, flag: %u", static_cast<int32_t>(kernel_type_), dump_flag_);
    const auto ret = SuperKernelLaunch();
    if (ret != SUCCESS) {
      GELOGE(ret, "[Call][SuperKernelLaunch] failed, taskid:%u", task_id_);
      return ret;
    }
    GELOGI("firstly, superkernel launch before taskes.");
    // blockDim is reserved parameter, set to 1
    const std::string op_name(op_desc_->GetName());
    const rtKernelLaunchNames_t launch_name = {so_name_.c_str(), kernel_name_.c_str(), op_name.c_str()};
    SetTaskTag(op_name.c_str());
    rtArgsEx_t args_ex = {};
    args_ex.args = args_;
    args_ex.argsSize = args_size_;
    GE_CHK_RT_RET(rtAicpuKernelLaunchWithFlag(&launch_name, block_dim_, &args_ex, nullptr, stream_,
                                              dump_flag_));
    call_save_dump_ = true;
  } else {
    /* default: not skt launch */
    const SuperKernelTaskInfo &skt_info = davinci_model_->GetSuperKernelTaskInfo();
    GELOGD("KernelTaskInfo Distribute Start, sktenable:%d taskid:%u sktid:%u last_sktid:%u stubfunc_name:%s "
           "stubfunc:%p blockdim:%u stream:%p",
           static_cast<int32_t>(call_skt), task_id_, skt_id_, skt_info.last_task_id, stub_func_name_.c_str(),
           stub_func_, block_dim_, stream_);
    // l1 fusion enable and env flag open (kCloseSkt for skt debug)
    bool open_dump = false;
    if (davinci_model_->ModelNeedDump()) {
      open_dump = true;
    }
    if (call_skt && (env_flag != kCloseSkt) && (!open_dump)) {
      GE_RETURN_IF_ERROR(SuperKernelDistribute());
    } else {
      // call rtKernelLaunch for current task
      SetTaskTag((op_desc_ != nullptr) ? op_desc_->GetName().c_str() : "");
      rtArgsEx_t args_ex = {};
      args_ex.args = args_;
      args_ex.argsSize = args_size_;
      GE_CHK_RT_RET(rtKernelLaunchWithFlag(stub_func_, block_dim_, &args_ex, PtrToPtr<void, rtSmDesc_t>(sm_desc_),
                                           stream_, dump_flag_));
      call_save_dump_ = true;
    }
  }

  // set for task_id_
  UpdateTaskId();
  if (is_blocking_aicpu_op_) {
    if (DistributeWaitTaskForAicpuBlockingOp() != SUCCESS) {
      GELOGE(FAILED, "[Call][DistributeWaitTaskForAicpuBlockingOp] Call DistributeWaitTaskForAicpuBlockingOp failed");
      return FAILED;
    }
  }
  GELOGD("KernelTaskInfo Distribute Success. sktenable:%d taskid:%d sktid:%d stubfunc_name:%s stubfunc:%p "
         "blockdim:%d stream:%p",
         static_cast<int32_t>(call_skt), task_id_, skt_id_, stub_func_name_.c_str(), stub_func_, block_dim_, stream_);
  op_desc_.reset(); // Not hold OpDesc after distribute.
  return SUCCESS;
}

void KernelTaskInfo::PostProcess(const domi::TaskDef &task_def) {
  const auto &context_def = task_def.kernel().context();
  davinci_model_->SaveProfilingTask(context_def.op_index(), task_def, *this);
}

Status KernelTaskInfo::CheckDeviceSupportBlockingAicpuOpProcess(bool &is_support) const {
  int32_t device_id = 0;
  GE_CHK_RT_RET(rtGetDevice(&device_id));

  int32_t val = 0;
  GE_CHK_RT_RET(rtGetDeviceCapability(device_id, FEATURE_TYPE_BLOCKING_OPERATOR, RT_MODULE_TYPE_AICPU, &val));
  if ((val != RT_AICPU_BLOCKING_OP_NOT_SUPPORT) && (val != RT_AICPU_BLOCKING_OP_SUPPORT)) {
    REPORT_INNER_ERROR("E19999", "Value should be %d or %d but %d", RT_AICPU_BLOCKING_OP_NOT_SUPPORT,
                       RT_AICPU_BLOCKING_OP_SUPPORT, val);
    GELOGE(FAILED, "[Check][Value] Value should be %d or %d but %d", RT_AICPU_BLOCKING_OP_NOT_SUPPORT,
           RT_AICPU_BLOCKING_OP_SUPPORT, val);
    return FAILED;
  }
  is_support = (val == RT_AICPU_BLOCKING_OP_SUPPORT);
  return SUCCESS;
}

Status KernelTaskInfo::UpdateEventIdForAicpuBlockingOp(hybrid::AicpuExtInfoHandler &ext_handle) const {
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
    if (davinci_model_->GetEventIdForBlockingAicpuOp(op_desc_, stream_, event_id) != SUCCESS) {
      GELOGE(FAILED, "[Get][EventId] Get event id failed for op:%s(%s)", op_desc_->GetName().c_str(),
             op_desc_->GetType().c_str());
      return FAILED;
    }
    if (ext_handle.UpdateEventId(event_id) != SUCCESS) {
      GELOGE(FAILED, "[Update][EventId] Update event id failed for op:%s(%s)", op_desc_->GetName().c_str(),
             op_desc_->GetType().c_str());
      return FAILED;
    }
    GELOGI("Update event_id=%u success", event_id);
  }
  return SUCCESS;
}

Status KernelTaskInfo::DistributeWaitTaskForAicpuBlockingOp() const {
  bool is_support = false;
  if (CheckDeviceSupportBlockingAicpuOpProcess(is_support) != SUCCESS) {
    GELOGE(FAILED, "[Call][CheckDeviceSupportBlockingAicpuOpProcess] Call CheckDeviceSupportBlockingAicpuOp failed");
    return FAILED;
  }
  if (!is_support) {
    GELOGD("device not support blocking aicpu op process.");
    return SUCCESS;
  }
  GELOGD("Distribute wait task begin");
  rtEvent_t rt_event = nullptr;
  if (davinci_model_->GetEventByStream(stream_, rt_event) != SUCCESS) {
    REPORT_CALL_ERROR("E19999", "Call GetEventByStream failed");
    GELOGE(FAILED, "[Call][GetEventByStream] Call GetEventByStream failed");
    return FAILED;
  }

  GE_CHK_RT_RET(rtStreamWaitEvent(stream_, rt_event));
  GE_CHK_RT_RET(rtEventReset(rt_event, stream_));

  return SUCCESS;
}

void KernelTaskInfo::SetIoAddrs(const OpDescPtr &op_desc) {
  const RuntimeParam &rts_param = davinci_model_->GetRuntimeParam();
  std::vector<void *> input_data_addrs = ModelUtils::GetInputAddrs(rts_param, op_desc);
  std::vector<void *> output_data_addrs = ModelUtils::GetOutputAddrs(rts_param, op_desc);

  (void)io_addrs_.insert(io_addrs_.cend(), input_data_addrs.cbegin(), input_data_addrs.cend());
  (void)io_addrs_.insert(io_addrs_.cend(), output_data_addrs.cbegin(), output_data_addrs.cend());

  if (kernel_type_ == ccKernelType::TE) {
    std::vector<void *> workspace_data_addrs = ModelUtils::GetWorkspaceDataAddrs(rts_param, op_desc);
    (void)io_addrs_.insert(io_addrs_.cend(), workspace_data_addrs.cbegin(), workspace_data_addrs.cend());
  }

  // Refresh the address for overflow detection
  if (globalworkspace_overflow_addr_ != nullptr) {
    io_addrs_.push_back(globalworkspace_overflow_addr_);
  }
}

Status KernelTaskInfo::CopyNoncontinuousArgs(const uint16_t offset) {
  GE_CHECK_NOTNULL(davinci_model_);
  // copy new io addrs
  std::vector<void *> io_addrs = io_addrs_;
  (void)davinci_model_->UpdateRunAddr(io_addrs);
  const auto addr_size = kAddressLen * io_addrs.size();

  // copy io addr
  const errno_t sec_ret = memcpy_s(&args_addr_[static_cast<size_t>(offset)], addr_size, io_addrs.data(), addr_size);
  if (sec_ret != EOK) {
    REPORT_CALL_ERROR("E19999", "Call memcpy_s fail, size:%zu, ret:0x%X", addr_size, sec_ret);
    GELOGE(FAILED, "[Call][Memcpy] failed, size:%zu, ret:%d", addr_size, sec_ret);
    return FAILED;
  }

  // copy args to device
  GE_CHK_RT_RET(rtMemcpy(args_, static_cast<uint64_t>(args_size_),
                         args_addr_.data(), static_cast<uint64_t>(args_size_), RT_MEMCPY_HOST_TO_DEVICE));

  GELOGD("Copy noncontinuous args success, kernel type %d.", static_cast<int32_t>(kernel_type_));
  return SUCCESS;
}

Status KernelTaskInfo::UpdateArgs() {
  GELOGI("KernelTaskInfo::UpdateArgs in.");
  GE_CHECK_NOTNULL(davinci_model_);
  if (kernel_type_ == ccKernelType::TE) {
    if (l2_buffer_on_) {
      return CopyNoncontinuousArgs(io_addr_offset_);
    }
    davinci_model_->SetTotalIOAddrs(io_addrs_);
    davinci_model_->UpdateOpIOAddrs(task_id_, stream_id_, io_addrs_);
  } else {
    if ((kernel_type_ == ccKernelType::AI_CPU) || (kernel_type_ == ccKernelType::CUST_AI_CPU)) {
      return CopyNoncontinuousArgs(static_cast<uint16_t>(sizeof(aicpu::AicpuParamHead)));
    }
  }
  return SUCCESS;
}

Status KernelTaskInfo::Release() {
  if (sm_desc_ != nullptr) {
    GE_CHK_RT(rtMemFreeManaged(sm_desc_));
    sm_desc_ = nullptr;
  }

  rtContext_t ctx = nullptr;
  GE_CHK_RT(rtCtxGetCurrent(&ctx));

  if (own_args_memory_) {
    GE_FREE_RT_LOG(args_);
  }
  args_ = nullptr;

  GE_FREE_RT_LOG(superkernel_device_args_addr_);
  GE_FREE_RT_LOG(superkernel_dev_nav_table_);
  GE_FREE_RT_LOG(flowtable_);
  GE_FREE_RT_LOG(custom_info_.input_descs);
  GE_FREE_RT_LOG(custom_info_.input_addrs);
  GE_FREE_RT_LOG(custom_info_.output_descs);
  GE_FREE_RT_LOG(custom_info_.output_addrs);
  GE_FREE_RT_LOG(custom_info_.attr_handle);
  GE_FREE_RT_LOG(aicpu_ext_info_addr_);

  ctx_.argsOffset.clear();

  return SUCCESS;
}

Status KernelTaskInfo::UpdateL2Data(const domi::KernelDef &kernel_def) {
  if (kernel_def.sm_desc().empty()) {
    return SUCCESS;
  }
  std::vector<uint8_t> sm_desc(kernel_def.sm_desc().size());
  (void)memcpy_s(sm_desc.data(), kernel_def.sm_desc().size(), kernel_def.sm_desc().data(), kernel_def.sm_desc().size());

  auto *const l2_ctrl_info = PtrToPtr<uint8_t, rtL2Ctrl_t>(sm_desc.data());

  // There is no weight for te op now. Update L2_mirror_addr by data memory base.
  const uint64_t data_base_addr = davinci_model_->FeatureMapBase() - davinci_model_->GetRtBaseAddr();
  const uint32_t l2_ctrl_info_data_count = 8U;
  for (uint32_t data_index = 0U; data_index < l2_ctrl_info_data_count; ++data_index) {
    if (l2_ctrl_info->data[data_index].L2_mirror_addr != 0U) {
      l2_ctrl_info->data[data_index].L2_mirror_addr += data_base_addr;
      l2_ctrl_info->data[data_index].L2_load_to_ddr = IsL2CpToDDR(l2_ctrl_info->data[data_index].L2_load_to_ddr);
    }
  }

  GE_CHK_RT_RET(rtMemAllocManaged(&sm_desc_, sm_desc.size(), RT_MEMORY_SPM));
  GE_CHK_RT_RET(rtMemcpy(sm_desc_, sm_desc.size(), sm_desc.data(), sm_desc.size(), RT_MEMCPY_HOST_TO_DEVICE));

  return SUCCESS;
}

void KernelTaskInfo::SetContinuousArgs(DavinciModel *const davinci_model) {
  args_offset_ = davinci_model->GetTotalArgsSize();
  davinci_model->SetTotalArgsSize(args_size_);
}

void KernelTaskInfo::SetNoncontinuousArgs(DavinciModel *const davinci_model) {
  hybrid_args_offset_ = davinci_model->GetHybridArgsSize();
  davinci_model->SetHybridArgsSize(args_size_);
}

Status KernelTaskInfo::CalculateArgs(const domi::TaskDef &task_def, DavinciModel *const davinci_model) {
  // Called before Init. Assign necessary vaiables.
  GE_CHECK_NOTNULL(davinci_model);
  const domi::KernelDef &kernel_def = task_def.kernel();
  const domi::KernelContext &context = kernel_def.context();
  kernel_type_ = static_cast<ccKernelType>(context.kernel_type());
  args_size_ = static_cast<uint32_t>(kernel_def.args().size());
  if (kernel_type_ == ccKernelType::TE) {
    if (kernel_def.sm_desc().empty()) {
      SetContinuousArgs(davinci_model);
      return SUCCESS;
    }
    l2_buffer_on_ = true;
    SetNoncontinuousArgs(davinci_model);
  } else {
    if ((kernel_type_ == ccKernelType::AI_CPU) || (kernel_type_ == ccKernelType::CUST_AI_CPU)) {
      SetNoncontinuousArgs(davinci_model);
    }
  }
  return SUCCESS;
}

Status KernelTaskInfo::InitTVMContext(const domi::KernelContext &context) {
  // get bin_file_key
  std::string session_graph_model_id;
  davinci_model_->GetUniqueId(op_desc_, session_graph_model_id);
  const char_t *const bin_file_key = davinci_model_->GetRegisterStub(op_desc_->GetName(), session_graph_model_id);
  void *tmp_stub_func = nullptr;
  const rtError_t rt_ret = rtGetFunctionByName(bin_file_key, &tmp_stub_func);
  if (rt_ret != RT_ERROR_NONE) {
        REPORT_CALL_ERROR("E19999", "Call rtGetFunctionByName failed op:%s(%s), bin_file_key:%s, ret:0x%X",
                          op_desc_->GetName().c_str(), op_desc_->GetType().c_str(), bin_file_key, rt_ret);
    GELOGE(RT_FAILED, "[Execute][RtGetFunctionByName] failed for op:%s(%s), bin_file_key:%s",
           op_desc_->GetName().c_str(), op_desc_->GetType().c_str(), bin_file_key);
    return RT_ERROR_TO_GE_STATUS(rt_ret);
  }
  stub_func_ = tmp_stub_func;

  if ((context.args_offset().size() / sizeof(uint16_t)) < 1U) {
    REPORT_INNER_ERROR("E19999", "context.args_offset().size():%zu / sizeof(uint16_t) less than 1, op:%s(%s), "
                                 "check invalid", context.args_offset().size(),
                       op_desc_->GetName().c_str(), op_desc_->GetType().c_str());
    GELOGE(FAILED, "[Check][Param] context.args_offset().size() / sizeof(uint16_t) less than 1, op:%s(%s)",
           op_desc_->GetName().c_str(), op_desc_->GetType().c_str());
    return FAILED;
  }

  (void)memcpy_s(&io_addr_offset_, sizeof(uint16_t), context.args_offset().data(), sizeof(uint16_t));
  return SUCCESS;
}

Status KernelTaskInfo::InitTVMTask(const OpDescPtr &op_desc, const domi::KernelDef &kernel_def) {
  GELOGD("Do InitTVMTask.");
  GE_CHECK_NOTNULL(davinci_model_);
  GE_CHK_STATUS_RET_NOLOG(InitTVMContext(kernel_def.context()));

  args_addr_.resize(static_cast<size_t>(args_size_));
  errno_t sec_ret = memcpy_s(args_addr_.data(), static_cast<size_t>(args_size_),
                             kernel_def.args().data(), static_cast<size_t>(args_size_));
  if (sec_ret != EOK) {
    REPORT_CALL_ERROR("E19999", "Call memcpy_s fail, size:%u, ret:0x%X", args_size_, sec_ret);
    GELOGE(FAILED, "[Call][Memcpy] failed, size:%u, ret:0x%X", args_size_, sec_ret);
    return FAILED;
  }

  GE_CHK_STATUS_RET_NOLOG(UpdateL2Data(kernel_def));
  if (davinci_model_->IsKnownNode()) {
    args_ = l2_buffer_on_ ? davinci_model_->GetCurrentHybridArgsAddr(hybrid_args_offset_)
                          : davinci_model_->GetCurrentArgsAddr(args_offset_);
    InitDumpArgs(static_cast<uint32_t>(io_addr_offset_));
    return SUCCESS;
  }

  // Update Stub
  // When training, when the the second call to DavinciModel::init() comes here, stub_func_ is already valid,
  // and does not need to be modified.
  // When inferencing, stub_func_ is different from dynamic-registration to runtime, and needs to be modified.
  std::string session_graph_model_id;
  davinci_model_->GetUniqueId(op_desc, session_graph_model_id);
  const auto *const bin_file_key = davinci_model_->GetRegisterStub(op_desc->GetName(), session_graph_model_id);
  const rtError_t rt_ret = rtQueryFunctionRegistered(bin_file_key);
  if (rt_ret != RT_ERROR_NONE) {
    stub_func_ = bin_file_key;
  }

  const RuntimeParam &rts_param = davinci_model_->GetRuntimeParam();
  const std::vector<void *> input_data_addrs = ModelUtils::GetInputAddrs(rts_param, op_desc);
  const std::vector<void *> output_data_addrs = ModelUtils::GetOutputAddrs(rts_param, op_desc);
  const std::vector<void *> workspace_data_addrs = ModelUtils::GetWorkspaceDataAddrs(rts_param, op_desc);

  std::vector<void *> tensor_device_addrs;
  (void)tensor_device_addrs.insert(tensor_device_addrs.cend(), input_data_addrs.cbegin(), input_data_addrs.cend());
  (void)tensor_device_addrs.insert(tensor_device_addrs.cend(), output_data_addrs.cbegin(), output_data_addrs.cend());
  (void)tensor_device_addrs.insert(tensor_device_addrs.cend(), workspace_data_addrs.cbegin(),
                                   workspace_data_addrs.cend());

  if ((args_size_ <= io_addr_offset_) ||
      (static_cast<size_t>(args_size_ - io_addr_offset_) < (kAddressLen * tensor_device_addrs.size()))) {
    REPORT_INNER_ERROR("E19999",
                       "offset:%u >= kernelInfo.argsSize:%u or copy content:%zu beyond applied memory:%u, "
                       "check invalid",
                       static_cast<uint32_t>(io_addr_offset_), args_size_, kAddressLen * tensor_device_addrs.size(),
                       args_size_ - static_cast<uint32_t>(io_addr_offset_));
    GELOGE(FAILED,
           "[Check][Param] offset:%u >= kernelInfo.argsSize:%u or copy content:%zu beyond applied memory:%u, "
           "check invalid",
           static_cast<uint32_t>(io_addr_offset_), args_size_, kAddressLen * tensor_device_addrs.size(),
           args_size_ - static_cast<uint32_t>(io_addr_offset_));
    return FAILED;
  }

  sec_ret = memcpy_s(&args_addr_[static_cast<size_t>(io_addr_offset_)],
                     (static_cast<size_t>(args_size_) - static_cast<size_t>(io_addr_offset_)),
                     tensor_device_addrs.data(), kAddressLen * tensor_device_addrs.size());
  if (sec_ret != EOK) {
    REPORT_CALL_ERROR("E19999", "Call memcpy_s failed, size:%u, ret:0x%X", args_size_ - io_addr_offset_, sec_ret);
    GELOGE(FAILED, "[Call][Memcpy] failed, size:%u, ret:0x%X", args_size_ - io_addr_offset_, sec_ret);
    return FAILED;
  }

  std::vector<void *> virtual_io_addrs;  // use virtual address for zero copy key.
  (void)virtual_io_addrs.insert(virtual_io_addrs.cend(), input_data_addrs.cbegin(), input_data_addrs.cend());
  (void)virtual_io_addrs.insert(virtual_io_addrs.cend(), output_data_addrs.cbegin(), output_data_addrs.cend());
  if (op_desc->GetType() == ATOMICADDRCLEAN) {
    (void)virtual_io_addrs.insert(virtual_io_addrs.cend(), workspace_data_addrs.cbegin(), workspace_data_addrs.cend());
  }

  const auto zero_copy_args_index = davinci_model_->GetZeroCopyArgsIndex(virtual_io_addrs);
  if (zero_copy_args_index.empty()) {
    // malloc args memory
    own_args_memory_ = true;
    GE_CHK_RT_RET(rtMalloc(&args_, static_cast<uint64_t>(args_size_), RT_MEMORY_HBM));
    // copy orign args
    GE_CHK_RT_RET(rtMemcpy(args_, static_cast<uint64_t>(args_size_),
                           args_addr_.data(), static_cast<uint64_t>(args_size_), RT_MEMCPY_HOST_TO_DEVICE));
  } else {
    std::map<uintptr_t, std::set<size_t>> zero_copy_args_offset;
    for (const auto &args_index : zero_copy_args_index) {
      const auto args_offset_tmp = (args_index * kAddressLen) + static_cast<size_t>(io_addr_offset_);
      (void)zero_copy_args_offset[PtrToValue(virtual_io_addrs[args_index])].insert(args_offset_tmp);
    }
    GE_CHK_STATUS_RET(
        davinci_model_->Mapping2BundleZeroCopy(op_desc, zero_copy_args_offset, static_cast<size_t>(args_size_),
                                               args_addr_.data(), args_, own_args_memory_),
        "Failed mapping zero copy task for %s to bundle task", op_desc->GetName().c_str());
  }

  skt_dump_args_ =
      PtrAdd(PtrToPtr<void, uint8_t>(args_), static_cast<size_t>(args_size_), static_cast<size_t>(io_addr_offset_));
  InitDumpArgs(static_cast<uint32_t>(io_addr_offset_));

  GELOGD("Do InitTVMTask end");
  return SUCCESS;
}

bool KernelTaskInfo::IsL1FusionOp(const OpDescPtr &op_desc) const {
  std::vector<int64_t> input_memory_type;
  (void)AttrUtils::GetListInt(op_desc, ATTR_NAME_INPUT_MEM_TYPE_LIST, input_memory_type);
  for (const auto i : input_memory_type) {
    if (i == static_cast<int64_t>(RT_MEMORY_L1)) {
      return true;
    }
  }

  std::vector<int64_t> output_memory_type;
  (void)AttrUtils::GetListInt(op_desc, ATTR_NAME_OUTPUT_MEM_TYPE_LIST, output_memory_type);
  for (const auto type : output_memory_type) {
    if (type == static_cast<int64_t>(RT_MEMORY_L1)) {
      return true;
    }
  }
  return false;
}

Status KernelTaskInfo::InitAICPUCustomTask(const OpDescPtr &op_desc, const domi::KernelDef &kernel_def) {
  GELOGI("Do InitAICPUCustomTask");
  const domi::KernelContext &context = kernel_def.context();
  const uint32_t kCustomAicpuArgsLen = 5U;
  ctx_.argsOffset.resize(kCustomAicpuArgsLen);

  if ((context.args_offset().size() / sizeof(uint16_t)) < kCustomAicpuArgsLen) {
    REPORT_INNER_ERROR("E19999", "context.args_offset().size():%zu / sizeof(uint16_t) is less than "
                       "kCustomAicpuArgsLen:%u, op:%s(%s), check invalid", context.args_offset().size(),
                       kCustomAicpuArgsLen, op_desc->GetName().c_str(), op_desc->GetType().c_str());
    GELOGE(PARAM_INVALID, "[Check][Param] context.args_offset().size():%zu / sizeof(uint16_t) is less than "
           "kCustomAicpuArgsLen:%u, op:%s(%s)", context.args_offset().size(), kCustomAicpuArgsLen,
           op_desc->GetName().c_str(), op_desc->GetType().c_str());
    return PARAM_INVALID;
  }

  (void)memcpy_s(ctx_.argsOffset.data(), (sizeof(uint16_t) * kCustomAicpuArgsLen), context.args_offset().data(),
                 (sizeof(uint16_t) * kCustomAicpuArgsLen));

  const RuntimeParam &rts_param = davinci_model_->GetRuntimeParam();
  const std::vector<void *> input_data_addrs = ModelUtils::GetInputAddrs(rts_param, op_desc);
  const std::vector<void *> output_data_addrs = ModelUtils::GetOutputAddrs(rts_param, op_desc);
  const Status ret = StoreInputOutputTensor(input_data_addrs, output_data_addrs, ModelUtils::GetInputDescs(op_desc),
                                            ModelUtils::GetOutputDescs(op_desc));
  if (ret != SUCCESS) {
    GELOGE(ret, "[Store][InputOutputTensor] Failed, op:%s(%s)", op_desc->GetName().c_str(), op_desc->GetType().c_str());
    return ret;
  }

  // attrHandle
  Buffer buffer;
  if (!AttrUtils::GetBytes(op_desc, ATTR_NAME_OPATTR, buffer)) {
    REPORT_INNER_ERROR("E19999", "Get Attr:%s in op:%s(%s) fail", ATTR_NAME_OPATTR.c_str(),
                       op_desc->GetName().c_str(), op_desc->GetType().c_str());
    GELOGE(FAILED, "[Get][Attr] %s in op:%s(%s) fail", ATTR_NAME_OPATTR.c_str(),
           op_desc->GetName().c_str(), op_desc->GetType().c_str());
    return FAILED;
  }

  const uint64_t op_attr_size = static_cast<uint64_t>(buffer.GetSize());
  if (op_attr_size == 0U) {
    REPORT_INNER_ERROR("E19999", "Attr:%s in op:%s(%s) size is 0, check invalid",
                       ATTR_NAME_OPATTR.c_str(), op_desc->GetName().c_str(), op_desc->GetType().c_str());
    GELOGE(PARAM_INVALID, "[Check][Param] param op_attr_size is out of range, op:%s", op_desc->GetName().c_str());
    return PARAM_INVALID;
  }

  GE_CHK_RT_RET(rtMalloc(&custom_info_.attr_handle, op_attr_size, RT_MEMORY_HBM));
  GE_CHK_RT_RET(rtMemcpy(custom_info_.attr_handle, op_attr_size, buffer.GetData(), op_attr_size,
                         RT_MEMCPY_HOST_TO_DEVICE));

  // args
  std::vector<uint8_t> args(kernel_def.args().size());
  const errno_t sec_ret =
      memcpy_s(args.data(), kernel_def.args().size(), kernel_def.args().data(), kernel_def.args().size());
  if (sec_ret != EOK) {
    REPORT_CALL_ERROR("E19999", "Call memcpy_s fail, size:%zu, ret:0x%X", kernel_def.args().size(), sec_ret);
    GELOGE(FAILED, "[Call][Memcpy] failed, size:%zu, ret:0x%X", kernel_def.args().size(), sec_ret);
    return FAILED;
  }

  for (uint32_t i = 0U; i < kCustomAicpuArgsLen; ++i) {
    if (kernel_def.args().size() < (static_cast<size_t>(ctx_.argsOffset[static_cast<size_t>(i)]) + sizeof(uint64_t))) {
      REPORT_INNER_ERROR("E19999", "ctx.argsOffset[%u]: %u + sizeof(uint64_t): %zu >= kernelDef.args().size():%zu, "
                         "op:%s(%s) check invalid", i, static_cast<uint32_t>(ctx_.argsOffset[static_cast<size_t>(i)]),
                         sizeof(uint64_t), kernel_def.args().size(),
                         op_desc->GetName().c_str(), op_desc->GetType().c_str());
      GELOGE(FAILED, "[Check][Param] ctx.argsOffset[%u]:%u + sizeof(uint64_t):%zu >= kernelDef.args().size():%zu", i,
             static_cast<uint32_t>(ctx_.argsOffset[static_cast<size_t>(i)]), sizeof(uint64_t),
             kernel_def.args().size());
      return FAILED;
    }
  }
  *(PtrToPtr<uint8_t, uint64_t>(&args[static_cast<size_t>(ctx_.argsOffset[kArgsInputDesc])])) =
      PtrToValue(custom_info_.input_descs);  // arg 0
  *(PtrToPtr<uint8_t, uint64_t>(&args[static_cast<size_t>(ctx_.argsOffset[kArgsInputAddr])])) =
      PtrToValue(custom_info_.input_addrs);  // arg 1
  *(PtrToPtr<uint8_t, uint64_t>(&args[static_cast<size_t>(ctx_.argsOffset[kArgsOutputDesc])])) =
      PtrToValue(custom_info_.output_descs);  // arg 2
  *(PtrToPtr<uint8_t, uint64_t>(&args[static_cast<size_t>(ctx_.argsOffset[kArgsOutputAddr])])) =
      PtrToValue(custom_info_.output_addrs);  // arg 3
  *(PtrToPtr<uint8_t, uint64_t>(&args[static_cast<size_t>(ctx_.argsOffset[kArgsAttrHandle])])) =
      PtrToValue(custom_info_.attr_handle);  // arg 4

  own_args_memory_ = true;
  GE_CHK_RT_RET(rtMalloc(&args_, static_cast<uint64_t>(args_size_), RT_MEMORY_HBM));
  GE_CHK_RT_RET(rtMemcpy(args_, static_cast<uint64_t>(args_size_), args.data(), static_cast<uint64_t>(args_size_),
                         RT_MEMCPY_HOST_TO_DEVICE));

  davinci_model_->SetZeroCopyAddr(op_desc, input_data_addrs, input_data_addrs.data(),
                                  PtrToValue(custom_info_.input_addrs), input_data_addrs.size() * kAddressLen, 0U);
  davinci_model_->SetZeroCopyAddr(op_desc, output_data_addrs, output_data_addrs.data(),
                                  PtrToValue(custom_info_.output_addrs), output_data_addrs.size() * kAddressLen, 0U);
  return SUCCESS;
}

Status KernelTaskInfo::InitAicpuTask(const OpDescPtr &op_desc, const domi::KernelDef &kernel_def) {
  GELOGI("Do InitAicpuTask");
  so_name_ = kernel_def.so_name();
  kernel_name_ = kernel_def.kernel_name();
  io_addr_offset_ = static_cast<uint16_t>(sizeof(aicpu::AicpuParamHead));
  GELOGI("node[%s] so name %s, kernel name %s", op_desc->GetName().c_str(), so_name_.c_str(), kernel_name_.c_str());

  if (kernel_type_ == ccKernelType::CUST_AI_CPU) {
    bool loaded = false;
    auto &model_mgr = ModelManager::GetInstance();
    GE_CHK_STATUS_RET(model_mgr.LoadCustAicpuSo(davinci_model_->GetCustAICPUKernel(op_desc), so_name_, loaded),
                      "[Launch][CustAicpuSo] failed, node: %s", op_desc->GetName().c_str());
  }

  // copy args to new host memory
  args_addr_.resize(static_cast<size_t>(args_size_));
  GE_PRINT_DYNAMIC_MEMORY(new, "cce task physical memory.", sizeof(uint8_t) * args_size_);
  errno_t sec_ret = memcpy_s(args_addr_.data(), static_cast<size_t>(args_size_), kernel_def.args().data(),
                             static_cast<size_t>(args_size_));
  if (sec_ret != EOK) {
    REPORT_CALL_ERROR("E19999", "Call memcpy_s fail, size:%u, ret:0x%X", args_size_, sec_ret);
    GELOGE(FAILED, "[Call][Memcpy] failed, size:%u, ret:0x%X", args_size_, sec_ret);
    return FAILED;
  }

  const auto aicpu_param_head = PtrToPtr<uint8_t, aicpu::AicpuParamHead>(args_addr_.data());
  const auto &ext_info = kernel_def.kernel_ext_info();
  const auto init_ret = InitAicpuTaskExtInfo(ext_info);
  if (init_ret != SUCCESS) {
    GELOGE(init_ret, "[Init][AicpuTaskExtInfo] failed, ext_info size=%zu", ext_info.size());
    return init_ret;
  }
  GELOGI("Node[%s] type[%s] kernel_ext_info size=%zu, aicpu_ext_info_addr_=%p", op_desc->GetName().c_str(),
         op_desc->GetType().c_str(), ext_info.size(), aicpu_ext_info_addr_);

  aicpu_param_head->extInfoAddr = PtrToValue(aicpu_ext_info_addr_);
  aicpu_param_head->extInfoLength = static_cast<uint32_t>(ext_info.size());

  if (davinci_model_->IsKnownNode()) {
    args_ = davinci_model_->GetCurrentHybridArgsAddr(hybrid_args_offset_);
    InitDumpArgs(static_cast<uint32_t>(io_addr_offset_));
    return SUCCESS;
  }
  const RuntimeParam &rts_param = davinci_model_->GetRuntimeParam();
  std::vector<void *> input_addrs = ModelUtils::GetInputAddrs(rts_param, op_desc);
  std::vector<void *> output_addrs = ModelUtils::GetOutputAddrs(rts_param, op_desc);
  std::vector<void *> io_addrs;
  (void)io_addrs.insert(io_addrs.cend(), input_addrs.cbegin(), input_addrs.cend());
  (void)io_addrs.insert(io_addrs.cend(), output_addrs.cbegin(), output_addrs.cend());
  if (!io_addrs.empty()) {
    // refresh io addrs
    const auto addrs_size = sizeof(uint64_t) * io_addrs.size();
    sec_ret = memcpy_s(&(args_addr_[static_cast<size_t>(io_addr_offset_)]), addrs_size, io_addrs.data(), addrs_size);
    if (sec_ret != EOK) {
      REPORT_CALL_ERROR("E19999", "Call memcpy_s fail, size:%lu, ret:0x%X", addrs_size, sec_ret);
      GELOGE(FAILED, "[Call][Memcpy] failed, size:%lu, ret:0x%X", addrs_size, sec_ret);
      return FAILED;
    }
  }

  // malloc device memory for args
  own_args_memory_ = true;
  GE_CHK_RT_RET(rtMalloc(&args_, static_cast<uint64_t>(args_size_), RT_MEMORY_HBM));
  GE_PRINT_DYNAMIC_MEMORY(rtMalloc, "cce task physical memory.", args_size_);

  // copy args to device
  GE_CHK_RT_RET(rtMemcpy(args_, static_cast<uint64_t>(args_size_), args_addr_.data(), static_cast<uint64_t>(args_size_),
                         RT_MEMCPY_HOST_TO_DEVICE));

  InitDumpArgs(static_cast<uint32_t>(io_addr_offset_));

  davinci_model_->SetZeroCopyAddr(op_desc, io_addrs, args_addr_.data(), PtrToValue(args_),
                                  static_cast<size_t>(args_size_), static_cast<size_t>(io_addr_offset_));

  return SUCCESS;
}

void KernelTaskInfo::InitDumpFlag() {
  if (davinci_model_->OpNeedDump(op_desc_->GetName())) {
    GELOGD("Op %s init dump flag", op_desc_->GetName().c_str());
    if (IsL1FusionOp(op_desc_)) {
      dump_flag_ = RT_FUSION_KERNEL_DUMPFLAG;
    } else {
      dump_flag_ = RT_KERNEL_DUMPFLAG;
    }
  }
}

void KernelTaskInfo::InitDumpArgs(const uint32_t offset) {
  if (davinci_model_->OpNeedDump(op_desc_->GetName())) {
    GELOGD("Op %s need dump in task info", op_desc_->GetName().c_str());
    dump_args_ = PtrAdd(PtrToPtr<void, uint8_t>(args_), static_cast<size_t>(args_size_), static_cast<size_t>(offset));
  }
  if (davinci_model_->GetOpDugReg()) {
    GELOGD("Op debug is open in kernel task info");
    dump_args_ = PtrAdd(PtrToPtr<void, uint8_t>(args_), static_cast<size_t>(args_size_), static_cast<size_t>(offset));
  }
  if (kernel_type_ == ccKernelType::CUST_AI_CPU) {
    dump_flag_ |= RT_KERNEL_CUSTOM_AICPU;
  }
}

Status KernelTaskInfo::InitAicpuTaskExtInfo(const std::string &ext_info) {
  if (ext_info.empty()) {
    return SUCCESS;
  }

  int32_t unknown_shape_type_val = 0;
  (void)AttrUtils::GetInt(op_desc_, ATTR_NAME_UNKNOWN_SHAPE_TYPE, unknown_shape_type_val);
  const auto unknown_type = static_cast<UnknowShapeOpType>(unknown_shape_type_val);
  const uint32_t num_inputs = static_cast<uint32_t>(op_desc_->GetInputsSize());
  const uint32_t num_outputs = static_cast<uint32_t>(op_desc_->GetOutputsSize());
  hybrid::AicpuExtInfoHandler ext_handle(op_desc_->GetName(), num_inputs, num_outputs, unknown_type);
  GE_CHK_STATUS_RET(ext_handle.Parse(ext_info), "[Parse][KernelExtInfo] failed, kernel_ext_info_size=%zu, op:%s.",
                    ext_info.size(), op_desc_->GetName().c_str());
  GE_CHK_STATUS_RET(ext_handle.UpdateSessionInfoId(davinci_model_->GetSessionId()),
                    "[Update][SessionInfoSessionId] failed, op:%s", op_desc_->GetName().c_str());
  GELOGD("Update aicpu_task ext_info session_info session_id is %lu", davinci_model_->GetSessionId());
  GE_CHK_STATUS_RET(ext_handle.UpdateExecuteMode(true), "[Update][ExecuteMode] failed, op:%s",
                    op_desc_->GetName().c_str());
  GELOGD("Update aicpu_task ext_info bit_map execute mode to 1.");
  topic_type_flag_ = ext_handle.GetTopicTypeFlag();

  bool all_shape = false;
  (void)AttrUtils::GetBool(op_desc_, kAllShapeInAicpu, all_shape);
  if (all_shape) {
    GELOGD("Aicpu all_shape kernel need to update io shape.");
    for (uint32_t i = 0U; i < num_inputs; i++) {
      const auto input_desc = op_desc_->MutableInputDesc(i);
      GE_CHECK_NOTNULL(input_desc);
      GE_CHK_STATUS_RET(ext_handle.UpdateInputShapeAndType(i, *input_desc),
                        "[Call][UpdateInputShapeAndType] Input[%u] update input shape failed, op:%s.",
                        i, op_desc_->GetName().c_str());
    }
    for (uint32_t j = 0U; j < num_outputs; j++) {
      const auto output_desc = op_desc_->MutableOutputDesc(j);
      GE_CHECK_NOTNULL(output_desc);
      GE_CHK_STATUS_RET(ext_handle.UpdateOutputShapeAndType(j, *output_desc),
                        "[Call][UpdateOutputShapeAndType] Output[%u] update output shape failed, op:%s.",
                        j, op_desc_->GetName().c_str());
    }
  }

  (void)AttrUtils::GetBool(op_desc_, ATTR_NAME_IS_BLOCKING_OP, is_blocking_aicpu_op_);
  GELOGD("Get op:%s attribute(is_blocking_op), value:%d", op_desc_->GetName().c_str(),
         static_cast<int32_t>(is_blocking_aicpu_op_));

  if (UpdateEventIdForAicpuBlockingOp(ext_handle) != SUCCESS) {
    GELOGE(FAILED, "[Call][UpdateEventIdForAicpuBlockingOp] failed for op:%s(%s)",
           op_desc_->GetName().c_str(), op_desc_->GetType().c_str());
    return FAILED;
  }

  GE_CHK_RT_RET(rtMalloc(&aicpu_ext_info_addr_, ext_handle.GetExtInfoLen(), RT_MEMORY_HBM));
  GE_CHK_RT_RET(rtMemcpy(aicpu_ext_info_addr_, ext_handle.GetExtInfoLen(), ext_handle.GetExtInfo(),
                         ext_handle.GetExtInfoLen(), RT_MEMCPY_HOST_TO_DEVICE));

  return SUCCESS;
}

Status KernelTaskInfo::StoreInputOutputTensor(const std::vector<void *> &input_data_addrs,
                                              const std::vector<void *> &output_data_addrs,
                                              const std::vector<ccAICPUTensor> &input_descs,
                                              const std::vector<ccAICPUTensor> &output_descs) {
  if (!input_data_addrs.empty()) {
    const auto input_size = input_descs.size();
    const size_t total_desc_size = sizeof(ccAICPUTensor) * input_size;
    const size_t total_addr_size = sizeof(void *) * input_size;
    // inputDescs
    GE_CHK_RT_RET(rtMalloc(&custom_info_.input_descs, total_desc_size, RT_MEMORY_HBM));
    GE_CHK_RT_RET(rtMemcpy(custom_info_.input_descs, total_desc_size, input_descs.data(), total_desc_size,
                           RT_MEMCPY_HOST_TO_DEVICE));

    // inputAddrs
    GE_CHK_RT_RET(rtMalloc(&custom_info_.input_addrs, total_addr_size, RT_MEMORY_HBM));
    GE_CHK_RT_RET(rtMemcpy(custom_info_.input_addrs, total_addr_size, input_data_addrs.data(), total_addr_size,
                           RT_MEMCPY_HOST_TO_DEVICE));
  }

  if (!output_data_addrs.empty()) {
    const auto output_size = output_descs.size();
    const size_t total_desc_size = sizeof(ccAICPUTensor) * output_size;
    const size_t total_addr_size = sizeof(void *) * output_size;
    // outputDescs
    GE_CHK_RT_RET(rtMalloc(&custom_info_.output_descs, total_desc_size, RT_MEMORY_HBM));
    GE_CHK_RT_RET(rtMemcpy(custom_info_.output_descs, total_desc_size, output_descs.data(),
                           sizeof(ccAICPUTensor) * output_size, RT_MEMCPY_HOST_TO_DEVICE));

    // outputAddrs
    GE_CHK_RT_RET(rtMalloc(&custom_info_.output_addrs, total_addr_size, RT_MEMORY_HBM));
    GE_CHK_RT_RET(rtMemcpy(custom_info_.output_addrs, total_addr_size, output_data_addrs.data(), total_addr_size,
                           RT_MEMCPY_HOST_TO_DEVICE));
  }

  return SUCCESS;
}

uint8_t KernelTaskInfo::IsL2CpToDDR(const uint8_t origain_L2_load_to_ddr) const {
  if (static_cast<int32_t>(origain_L2_load_to_ddr) == static_cast<int32_t>(kL2LoadToDdr)) {
    return kL2LoadToDdr;
  }

  if (dump_flag_ == RT_KERNEL_DUMPFLAG) {
    return kL2LoadToDdr;
  }
  return kL2NotLoadToDdr;
}

REGISTER_TASK_INFO(RT_MODEL_TASK_KERNEL, KernelTaskInfo);
}  // namespace ge
