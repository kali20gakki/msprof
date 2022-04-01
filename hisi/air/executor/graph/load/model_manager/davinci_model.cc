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

#include "graph/load/model_manager/davinci_model.h"

#include "common/ge_call_wrapper.h"
#include "common/omg_util.h"
#include "common/profiling/profiling_manager.h"
#include "common/utils/executor_utils.h"
#include "exec_runtime/runtime_tensor_desc.h"
#include "external/runtime/rt_error_codes.h"
#include "framework/common/file_constant_util.h"
#include "framework/common/op/ge_op_utils.h"
#include "graph/ge_context.h"
#include "graph/load/model_manager/model_utils.h"
#include "graph/load/model_manager/model_manager.h"
#include "graph/manager/graph_mem_manager.h"
#include "graph/manager/trans_var_data_utils.h"
#include "graph/manager/util/hcom_util.h"
#include "graph/model_serialize.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/type_utils.h"

namespace ge {
namespace {
constexpr uint32_t kDataIndex = 0U;
constexpr uint32_t kTrueBranchStreamCount = 1U;
constexpr uint32_t kGetDynamicDimsCount = 1U;
constexpr uint32_t kAddrSize = sizeof(void *);
constexpr int32_t kDecimalRadix = 10;
constexpr int64_t kDataMemAlignSizeCompare = 64;
constexpr uint32_t kModelL1FusionOpMByteSize = 2097152U;   // 2 * 1024 * 1024
constexpr uint32_t kModelFlagOfL1Fusion = 0U;
constexpr int32_t kInvalidStream = -1;
constexpr int32_t kSinkModelEndOfSequence = 0x0704000a;
constexpr int32_t kSinkModelEndOfSequenceNew = 507005;
constexpr int32_t kSinkModelAbortNormal = 0x0704000e;
constexpr int32_t kSinkModelAbortNormalNew = 507024;
constexpr uint32_t kStringBytes = 8U;
constexpr uint32_t kStringHeadElems = 2U;
constexpr size_t kMemAlignment = 64U;
constexpr uint64_t kSessionScopeMemory = 0x100000000U;
constexpr uint64_t kMemoryTypeMask = 0xffffffffU;
constexpr uint32_t kManualThreadMode = 0U;
constexpr uint32_t kInValidThreadMode = 3U;
constexpr uint64_t kMsProfFusionOpNum = static_cast<uint64_t>(MSPROF_GE_FUSION_OP_NUM);
constexpr int32_t kTimeNanoRadix = 1000000000;  // 1000 ^ 3 converts second to nanosecond
constexpr size_t kMemTypeIndexHbm = 0U;
constexpr size_t kMemTypeIndexTs = 1U;

const std::string kDefaultBatchLabel = "Batch_default";
const std::string kGetDynamicDimsName = "ascend_mbatch_get_dynamic_dims_node";
const std::string kMultiBatchNodePostfix = "_ascend_mbatch_batch_";
const std::string kMixCubeCoreTypeAic = "MIX_AIC";
const std::string kMixVectorCoreTypeAiv = "MIX_AIV";
const std::string kMixCubeTBEKernelPrefix = "_mix_aic";
const std::string kMixVectorTBEKernelPrefix = "_mix_aiv";
const std::string kStaticShape = "static";

const std::set<std::string> kDataOpTypes { DATA, AIPPDATA, ANN_DATA };

const std::set<std::string> kIoNodeTypes { DATA, AIPPDATA, ANN_DATA, QUEUE_DATA, NETOUTPUT };

inline bool IsNoTaskAndDumpNeeded(const OpDescPtr &op_desc) {
  bool save_dump_info = false;
  (void)AttrUtils::GetBool(op_desc, ATTR_NO_TASK_AND_DUMP_NEEDED, save_dump_info);
  return save_dump_info;
}

bool IsInputOfNetoutputCanZeroCopy(const NodePtr &node, const int32_t anchor_idx) {
  if ((node->GetInDataAnchor(anchor_idx) == nullptr) ||
      (node->GetInDataAnchor(anchor_idx)->GetPeerOutAnchor() == nullptr) ||
      (node->GetInDataAnchor(anchor_idx)->GetPeerOutAnchor()->GetOwnerNode() == nullptr) ||
      (node->GetInDataAnchor(anchor_idx)->GetPeerOutAnchor()->GetOwnerNode()->GetOpDesc() == nullptr)) {
    GELOGE(PARAM_INVALID, "Peer node of net-output %s input %d is invalid", node->GetName().c_str(), anchor_idx);
    return false;
  }

  const auto src_node = node->GetInDataAnchor(anchor_idx)->GetPeerOutAnchor()->GetOwnerNode();
  const auto src_output_index = node->GetInDataAnchor(anchor_idx)->GetPeerOutAnchor()->GetIdx();
  const auto output_desc = src_node->GetOpDesc()->GetOutputDescPtr(static_cast<uint32_t>(src_output_index));

  bool is_zero_copy_block = false;
  const bool determinate =
      (output_desc != nullptr) && AttrUtils::GetBool(output_desc, ATTR_IS_ZERO_COPY_BLOCK, is_zero_copy_block);

  GELOGI("Net-output %s input %d from %s output %d can zero copy: %s", node->GetName().c_str(), anchor_idx,
         src_node->GetName().c_str(), src_output_index,
         (determinate ? (is_zero_copy_block ? "true" : "false") : "indeterminate"));

  return is_zero_copy_block;
}

void InitMemoryInfo(const OpDescPtr &op_desc, uint64_t &input_mem_size, uint64_t &output_mem_size,
                    uint64_t &workspace_mem_size, uint64_t &weight_mem_size) {
  const auto input_size = ModelUtils::GetInputSize(op_desc);
  const auto output_size = ModelUtils::GetOutputSize(op_desc);
  const auto workspace_size = ModelUtils::GetWorkspaceSize(op_desc);
  const auto weight_size = ModelUtils::GetWeightSize(op_desc);
  input_mem_size = static_cast<uint64_t>(std::accumulate(input_size.begin(), input_size.end(), 0));
  output_mem_size = static_cast<uint64_t>(std::accumulate(output_size.begin(), output_size.end(), 0));
  workspace_mem_size = static_cast<uint64_t>(std::accumulate(workspace_size.begin(), workspace_size.end(), 0));
  weight_mem_size = static_cast<uint64_t>(std::accumulate(weight_size.begin(), weight_size.end(), 0));
}

void FillProfFusionOp(const ProfileInfo &profile, const uint64_t origin_op_index, const uint64_t op_index,
                      MsprofGeProfFusionData &prof_fusion_data) {
  uint64_t hash_id = 0UL;
  const auto &prof_mgr = ProfilingManager::Instance();
  const auto ret = prof_mgr.QueryHashId(profile.fusion_info.original_op_names[origin_op_index], hash_id);
  if (ret == SUCCESS) {
    prof_fusion_data.fusionOp[op_index] = hash_id;
  }
}
}  // namespace

DavinciModel::DavinciModel(const int32_t priority, const std::shared_ptr<ModelListener> &listener)
    : listener_(listener), priority_(priority), data_dumper_(&runtime_param_) {
  op_list_.clear();
  skt_info_ = {0U, 0U, 0U, 0U, nullptr, nullptr, {}, {}, {}, {}, {}, RT_KERNEL_DEFAULT, -1, 0U, nullptr};
}

DavinciModel::~DavinciModel() {
  GE_CHK_STATUS(ModelRunStop());
  if (GetDieId() != std::numeric_limits<int64_t>::max()) {
    GE_CHK_RT(rtSetDie(static_cast<int32_t>(GetDieId())));
  }
  data_dumper_.UnloadDumpInfo();

  ClearTaskAddrs();

  op_list_.clear();
  tensor_name_to_fixed_addr_size_.clear();
  tensor_name_to_peer_output_index_.clear();
  // check rt ctx is exist. rt api call will cause error log when ctx not exist
  rtContext_t current_ctx = nullptr;
  if (rtCtxGetCurrent(&current_ctx) == RT_ERROR_NONE) {
    UnbindTaskSinkStream();
    for (size_t i = 0U; i < label_list_.size(); ++i) {
      if (label_list_[i] != nullptr) {
        GE_LOGW_IF(rtLabelDestroy(label_list_[i]) != RT_ERROR_NONE, "Destroy label failed, index:%zu.", i);
      }
    }

    for (size_t i = 0U; i < stream_list_.size(); ++i) {
      GE_LOGW_IF(rtStreamDestroy(stream_list_[i]) != RT_ERROR_NONE, "Destroy stream failed, index:%zu.", i);
    }

    for (size_t i = 0U; i < event_list_.size(); ++i) {
      GE_LOGW_IF(rtEventDestroy(event_list_[i]) != RT_ERROR_NONE, "Destroy event failed, index: %zu", i);
    }

    for (const auto &it : stream_2_event_) {
      GE_LOGW_IF(rtEventDestroy(it.second) != RT_ERROR_NONE, "Destroy event failed");
    }

    ReleaseTask();

    FreeWeightsMem();

    FreeFeatureMapMem();

    FreeExMem();

    OpDebugUnRegister();

    GE_FREE_RT_LOG(l1_fusion_addr_);

    // Release overflow detection mem
    if ((globalworkspace_overflow_addr_ != nullptr) && (!known_node_)) {
      GE_CHK_RT(rtFree(globalworkspace_overflow_addr_));
      globalworkspace_overflow_addr_ = nullptr;
    }

    if (rt_model_handle_ != nullptr) {
      GE_CHK_RT(rtModelDestroy(rt_model_handle_));
      rt_model_handle_ = nullptr;
    }
  }

  bin_kernel_handle_.CleanTbeHandle();
  aicpu_resources_.ReleaseResources();

  var_mem_base_ = 0U;
  total_io_addrs_.clear();

  const auto free_mem_by_value = [](uintptr_t &ptr_value) {
    if (ptr_value != 0U) {
      GE_CHK_RT(rtFree(ValueToPtr(ptr_value)));
      ptr_value = 0U;
    }
  };
  for (auto &addr : known_args_bases_) {
    free_mem_by_value(addr);
  }
  free_mem_by_value(fixed_addrs_base_);
  free_mem_by_value(hybrid_addrs_base_);
}

void DavinciModel::ClearTaskAddrs() {
  for (auto &op_and_addr : saved_task_addrs_) {
    GE_FREE_RT_LOG(op_and_addr.second);
  }
  saved_task_addrs_.clear();
}

void DavinciModel::UnbindHcomStream() {
  for (size_t i = 0U; i < all_hccl_stream_list_.size(); ++i) {
    GE_LOGW_IF(rtModelUnbindStream(rt_model_handle_, all_hccl_stream_list_[i]) != RT_ERROR_NONE,
               "Unbind hccl stream from model failed, Index: %zu", i);
    GE_LOGW_IF(rtStreamDestroy(all_hccl_stream_list_[i]) != RT_ERROR_NONE, "Destroy hccl stream for rt_model failed")
  }
}

void DavinciModel::ReleaseTask() {
  for (const auto &task : cpu_task_list_) {
    if (task != nullptr) {
      GE_CHK_STATUS(task->Release(), "[Release][Task] failed, model id:%u.", model_id_);
    }
  }
  cpu_task_list_.clear();

  for (const auto &task : task_list_) {
    if (task != nullptr) {
      GE_CHK_STATUS(task->Release(), "[Release][Task] failed, model id:%u.", model_id_);
    }
  }
  task_list_.clear();

  for (auto &item : label_goto_args_) {
    GE_FREE_RT_LOG(item.second.first);
  }
  label_goto_args_.clear();
}

void DavinciModel::Assign(const GeModelPtr &ge_model) {
  if (ge_model == nullptr) {
    GELOGW("Assign null to ge_model");
  }
  ge_model_ = ge_model;
}

///
/// @ingroup ge
/// @brief Reduce memory usage after task sink.
/// @return: void
///
void DavinciModel::Shrink() {
  skt_info_ = {0U, 0U, 0U, 0U, nullptr, nullptr, {}, {}, {}, {}, {}, RT_KERNEL_DEFAULT, -1, 0U, nullptr};
  DumperShrink();
  ge_model_.reset();  // delete object.
  op_list_.clear();
  ClearTaskAddrs();
}

Status DavinciModel::InitWeightMem(const uintptr_t mem_ptr, const uintptr_t weight_ptr, const size_t weight_size) {
  if (is_weight_mem_has_inited_) {
    REPORT_INNER_ERROR("E19999", "Call InitWeightMem more than once, model_id:%u, check invalid", model_id_);
    GELOGE(FAILED, "[Check][Param] call InitWeightMem more than once, model id:%u.", model_id_);
    return FAILED;
  }
  is_weight_mem_has_inited_ = true;

  const Buffer &weights = ge_model_->GetWeight();
  const auto weights_size = weights.GetSize();
  GE_CHECK_LE(weights_size, ALLOC_MEMORY_MAX_SIZE);

  if ((weight_ptr != 0U) && (weight_size < weights_size)) {
    REPORT_INNER_ERROR("E19999", "Param weight_ptr is nullptr or ge_model.weight.size:%zu < param weights_size:%zu, "
                       "model_id:%u, check invalid", weight_size, weights_size, model_id_);
    GELOGE(FAILED, "[Check][Param] Invalid mem param: weight_size=%zu totalsize=%zu, model_id:%u.",
           weight_size, weights_size, model_id_);
    return FAILED;
  }

  weights_mem_base_ = mem_ptr;
  is_inner_weight_base_ = false;

  if (weights_size != 0U) {
    weights_mem_base_ = weight_ptr;
    is_inner_weight_base_ = false;
    if (weight_ptr == 0U) {
      weights_mem_base_ = PtrToValue(MallocWeightsMem(weights_size));
      if (weights_mem_base_ == 0U) {
        REPORT_CALL_ERROR("E19999", "MallocWeightsMem fail, weights_size:%zu, model_id:%u, check invalid",
                          weights_size, model_id_);
        GELOGE(ACL_ERROR_GE_MEMORY_ALLOCATION, "[Alloc][Memory] for weight failed. size:%zu, model_id:%u",
               weights_size, model_id_);
        return ACL_ERROR_GE_MEMORY_ALLOCATION;
      }
      is_inner_weight_base_ = true;
    }
    GELOGI("[IMAS]InitWeightMem graph_%u MallocMemory type[W] memaddr[%lx] mem_size[%zu]", runtime_param_.graph_id,
           weights_mem_base_, weights_size);
    GE_CHK_RT_RET(rtMemcpy(ValueToPtr(weights_mem_base_), weights_size, weights.GetData(), weights_size,
                           RT_MEMCPY_HOST_TO_DEVICE));
    GELOGI("copy weights data to device");
  }

  runtime_param_.weight_base = weights_mem_base_;
  return SUCCESS;
}

Status DavinciModel::InitFeatureMapAndP2PMem(const uintptr_t mem_ptr, const size_t mem_size) {
  if (is_feature_map_mem_has_inited_) {
    REPORT_INNER_ERROR("E19999", "InitFeatureMapMem is called more than once, model_id:%u, check invalid", model_id_);
    GELOGE(PARAM_INVALID, "[Check][Param] InitFeatureMapMem is called more than once, model_id:%u", model_id_);
    return PARAM_INVALID;
  }
  is_feature_map_mem_has_inited_ = true;

  size_t data_size = TotalMemSize();

  if (ExecutorUtils::IsReuseZeroCopyMemory()) {
    int64_t non_zero_copy_memory_size = 0;
    GE_CHK_STATUS_RET_NOLOG(GetTotalMemSizeExcludeZeroCopy(non_zero_copy_memory_size));
    data_size = static_cast<size_t>(non_zero_copy_memory_size);
    GELOGI("Model %u need %zu/%zu for feature-map without zero-copyable memory", model_id_, data_size, TotalMemSize());
  }

  if ((mem_ptr != 0U) && (mem_size < data_size)) {
    REPORT_INNER_ERROR("E19999", "Param mem_ptr is nullptr or mem_size:%zu < ge_model.mem_size:%zu, "
                       "model_id:%u, check invalid", mem_size, data_size, model_id_);
    GELOGE(PARAM_INVALID, "[Check][Param] Invalid mem param: mem_size=%zu totalsize=%zu, model_id:%u.",
           mem_size, data_size, model_id_);
    return PARAM_INVALID;
  }

  mem_base_ = mem_ptr;
  is_inner_mem_base_ = false;

  if ((data_size != 0U) && (mem_base_ == 0U)) {
    mem_base_ = PtrToValue(MallocFeatureMapMem(data_size));
    if (mem_base_ == 0U) {
      REPORT_CALL_ERROR("E19999", "MallocFeatureMapMem fail, data_size:%zu, model_id:%u, check invalid",
                        data_size, model_id_);
      GELOGE(ACL_ERROR_GE_MEMORY_ALLOCATION, "[Alloc][Memory] for feature map failed. size:%zu, model_id:%u",
             data_size, model_id_);
      return ACL_ERROR_GE_MEMORY_ALLOCATION;
    }
    GEEVENT("[IMAS]InitFeatureMapAndP2PMem graph_%u MallocMemory type[F] memaddr[%lx] mem_size[%zu]",
            runtime_param_.graph_id, mem_base_, data_size);

    if (!is_inner_weight_base_) {
      weights_mem_base_ = mem_base_;
      is_inner_weight_base_ = true;
    }
    is_inner_mem_base_ = true;
  }

  if (!runtime_param_.memory_infos.empty()) {
    GE_CHK_STATUS_RET(MallocExMem(), "MallocExMem failed.");
  }

  GE_CHK_STATUS_RET(InitVariableMem(), "[Init][VariableMemory] failed, model_id:%u", model_id_);
  runtime_param_.mem_base = mem_base_;
  runtime_param_.weight_base = weights_mem_base_;
  return SUCCESS;
}

Status DavinciModel::InitVariableMem() {
  // malloc variable memory base
  var_mem_base_ = PtrToValue(VarManager::Instance(session_id_)->GetVarMemoryBase(RT_MEMORY_HBM, GetDeviceId()));
  if ((TotalVarMemSize() != 0U) && (var_mem_base_ == 0U)) {
    const Status ret = VarManager::Instance(session_id_)->MallocVarMemory(TotalVarMemSize(), GetDeviceId());
    if (ret != SUCCESS) {
      REPORT_CALL_ERROR("E19999", "MallocVarMemory fail, var_size:%zu, model_id:%u, check invalid",
                        TotalVarMemSize(), model_id_);
      GELOGE(ret, "[Malloc][VarMemory] failed, var_size:%zu, model_id:%u", TotalVarMemSize(), model_id_);
      return ret;
    }
    var_mem_base_ = PtrToValue(VarManager::Instance(session_id_)->GetVarMemoryBase(RT_MEMORY_HBM, GetDeviceId()));
    GEEVENT("[IMAS]InitVariableMem graph_%u MallocMemory type[V] memaddr[%lx] mem_size[%zu]", runtime_param_.graph_id,
            var_mem_base_, TotalVarMemSize());
  }
  runtime_param_.var_base = var_mem_base_;
  return SUCCESS;
}

void DavinciModel::InitRuntimeParams() {
  (void)AttrUtils::GetInt(ge_model_, ATTR_MODEL_MEMORY_SIZE, runtime_param_.mem_size);
  (void)AttrUtils::GetInt(ge_model_, ATTR_MODEL_WEIGHT_SIZE, runtime_param_.weight_size);
  (void)AttrUtils::GetInt(ge_model_, ATTR_MODEL_STREAM_NUM, runtime_param_.stream_num);
  (void)AttrUtils::GetInt(ge_model_, ATTR_MODEL_EVENT_NUM, runtime_param_.event_num);
  (void)AttrUtils::GetInt(ge_model_, ATTR_MODEL_LABEL_NUM, runtime_param_.label_num);
  (void)AttrUtils::GetInt(ge_model_, ATTR_MODEL_BATCH_NUM, runtime_param_.batch_num);
  (void)AttrUtils::GetInt(ge_model_, MODEL_ATTR_TASK_GEN_BASE_ADDR, runtime_param_.logic_mem_base);
  (void)AttrUtils::GetInt(ge_model_, MODEL_ATTR_TASK_GEN_WEIGHT_ADDR, runtime_param_.logic_weight_base);
  (void)AttrUtils::GetInt(ge_model_, MODEL_ATTR_SESSION_ID, runtime_param_.session_id);
  (void)AttrUtils::GetInt(ge_model_, ATTR_MODEL_TASK_GEN_VAR_ADDR, runtime_param_.logic_var_base);
  (void)AttrUtils::GetInt(ge_model_, ATTR_MODEL_VAR_SIZE, runtime_param_.var_size);
  session_id_ = runtime_param_.session_id;

  MemInfo p2p_mem_info{};
  (void)AttrUtils::GetInt(ge_model_, ATTR_MODEL_P2P_MEMORY_SIZE, p2p_mem_info.memory_size);
  p2p_mem_info.memory_type = RT_MEMORY_P2P_DDR;
  p2p_mem_info.memory_key = "_p";
  runtime_param_.memory_infos[RT_MEMORY_P2P_DDR] = std::move(p2p_mem_info);

  MemInfo session_scope_mem_info{};
  (void)AttrUtils::GetInt(ge_model_, ATTR_MODEL_SESSION_SCOPE_MEMORY_SIZE, session_scope_mem_info.memory_size);
  runtime_param_.memory_infos[kSessionScopeMemory | RT_MEMORY_HBM] = std::move(session_scope_mem_info);

  (void)AttrUtils::GetInt(ge_model_, MODEL_ATTR_HOST_MEMORY_SIZE, runtime_param_.host_mem_size);
  (void)AttrUtils::GetInt(ge_model_, MODEL_ATTR_TASK_GEN_HOST_BASE_ADDR, runtime_param_.host_logic_mem_base);

  MemInfo host_mem_info{};
  host_mem_info.memory_size = runtime_param_.host_mem_size;
  host_mem_info.logic_memory_base = runtime_param_.host_logic_mem_base;
  host_mem_info.memory_type = RT_MEMORY_HOST;
  host_mem_info.memory_key = "_h";
  runtime_param_.memory_infos[RT_MEMORY_HOST] = std::move(host_mem_info);

  (void)AttrUtils::GetInt(ge_model_, ATTR_MODEL_ZERO_COPY_MEMORY_SIZE, runtime_param_.zero_copy_size);

  VarManager::Instance(session_id_)->SetMemManager(&MemManager::Instance());
  GELOGI("InitRuntimeParams: %s.", runtime_param_.ToString().c_str());
}

///
/// @ingroup ge
/// @brief Make active stream list and bind to model.
/// @return: 0 for success / others for fail
///
Status DavinciModel::BindModelStream() {
  // Stream not in active_stream_indication_ is active stream.
  if (((!input_queue_ids_.empty()) || (!output_queue_ids_.empty())) || (deploy_type_ == AICPU_DEPLOY_CROSS_THREAD)) {
    for (size_t i = 0U; i < stream_list_.size(); ++i) {
      if (active_stream_indication_.count(i) == 0U) {
        active_stream_list_.push_back(stream_list_[i]);
        (void)active_stream_indication_.insert(i);  // deactive all model stream.
      }
    }
  }

  for (size_t i = 0U; i < stream_list_.size(); ++i) {
    const auto bind_flag = (active_stream_indication_.count(i) == 0U) ? RT_HEAD_STREAM : RT_INVALID_FLAG;
    GELOGI("rtModelBindStream[%zu] stream: %p, flag: %#x", i, stream_list_[i], static_cast<uint32_t>(bind_flag));
    GE_CHK_RT_RET(rtModelBindStream(rt_model_handle_, stream_list_[i], static_cast<uint32_t>(bind_flag)));
  }
  is_stream_list_bind_ = true;
  return SUCCESS;
}

Status DavinciModel::DoTaskSink() {
  // task sink is supported as model_task_def is set
  const auto &model_task_def = ge_model_->GetModelTaskDefPtr();
  if (model_task_def == nullptr) {
    return SUCCESS;
  }

  GE_CHK_RT_RET(rtGetAicpuDeploy(&deploy_type_));
  GELOGI("Do task sink. AiCpu deploy type is: %x.", deploy_type_);

  GE_CHK_STATUS_RET(BindModelStream(), "[Bind][ModelStream] failed, model_id:%u.", model_id_);

  GE_CHK_STATUS_RET(InitL1DataDumperArgs(), "[Init][L1DataDumperArgs] failed, model_id:%u.", model_id_);

  GE_CHK_STATUS_RET(InitTaskInfo(*model_task_def), "[Init][TaskInfo] failed, model_id:%u.", model_id_);

  GE_CHK_STATUS_RET(InitEntryTask(), "[Init][EntryTask] failed, model_id:%u.", model_id_);

  auto &model_mgr = ModelManager::GetInstance();
  GE_CHK_STATUS_RET(model_mgr.LaunchCustAicpuSo(), "[Launch][CustAicpuSo] failed, model_id:%u.", model_id_);
  GE_CHK_STATUS_RET(model_mgr.CheckAicpuOpList(ge_model_), "[Check][AicpuOpList] failed, model_id:%u.", model_id_);

  GE_CHK_STATUS_RET(DistributeTask(*model_task_def), "[Distribute][Task] failed, model_id:%u.", model_id_);

  GE_CHK_RT_RET(rtModelLoadComplete(rt_model_handle_));

  SetCopyOnlyOutput();
  return SUCCESS;
}

// set device use aicore(0) or vectorcore(1)
Status DavinciModel::SetTSDevice() {
  int64_t value = 0;
  const bool ret = AttrUtils::GetInt(ge_model_, ATTR_MODEL_CORE_TYPE, value);
  const uint32_t core_type = ret ? static_cast<uint32_t>(value) : 0U;
  GELOGD("Set TSDevice: %u.", core_type);
  GE_CHK_RT_RET(rtSetTSDevice(core_type));
  return SUCCESS;
}

Status DavinciModel::OpDebugRegister() {
  if (GetDumpProperties().IsOpDebugOpen()) {
    const uint32_t op_debug_mode = GetDumpProperties().GetOpDebugMode();
    const auto ret = opdebug_register_.RegisterDebugForModel(rt_model_handle_, op_debug_mode, data_dumper_);
    if (ret != SUCCESS) {
      GELOGE(ret, "[Call][RegisterDebugForModel] Register known shape op debug failed, ret: 0x%X", ret);
      return ret;
    }
    is_op_debug_reg_ = true;
  }
  return SUCCESS;
}

void DavinciModel::OpDebugUnRegister() {
  if (is_op_debug_reg_) {
    opdebug_register_.UnregisterDebugForModel(rt_model_handle_);
    is_op_debug_reg_ = false;
  }
  return;
}

// initialize op sequence and call initialization function of each op respectively
Status DavinciModel::Init(const uintptr_t mem_ptr, const size_t mem_size,
                          const uintptr_t weight_ptr, const size_t weight_size) {
  // validating params
  GELOGI("Priority is %d.", priority_);
  GE_CHK_BOOL_TRUE_EXEC_WITH_LOG((priority_ < 0) || (priority_ > 7), return PARAM_INVALID,
                                 "[Check][Param] Priority must between 0-7, now is %d.", priority_);
  GE_CHK_BOOL_RET_STATUS(ge_model_ != nullptr, PARAM_INVALID, "[Check][Param] GeModel is null.");
  const Graph &graph = ge_model_->GetGraph();
  const ComputeGraphPtr compute_graph = GraphUtils::GetComputeGraph(graph);
  GE_CHK_BOOL_RET_STATUS(compute_graph != nullptr, INTERNAL_ERROR, "[Get][ComputeGraph] failed, ret is nullptr.");

  // Initializing runtime_param_
  InitRuntimeParams();

  GE_CHK_STATUS_RET(GetFilePathFromOption(file_id_and_path_map_), "Failed to get file path.");

  // RTS set aicore or vectorcore
  GE_CHK_STATUS_RET(SetTSDevice(), "[Set][TSDevice] failed, graph:%s.", compute_graph->GetName().c_str());

  version_ = ge_model_->GetVersion();
  name_ = ge_model_->GetName();
  (void)AttrUtils::GetBool(ge_model_, ATTR_NAME_SWITCH_FOR_L1_FUSION, is_l1_fusion_enable_);
  GELOGD("The value of ge.l1Fusion in ge_model is %d.", static_cast<int32_t>(is_l1_fusion_enable_));

  GE_TIMESTAMP_START(InitModelMem);
  GELOGD("Known node is %d.", static_cast<int32_t>(known_node_));
  GE_CHK_STATUS_RET_NOLOG(InitWeightMem(mem_ptr, weight_ptr, weight_size));
  if (!known_node_) {
    GE_CHK_STATUS_RET_NOLOG(InitFeatureMapAndP2PMem(mem_ptr, mem_size));
  }
  fixed_mem_base_ = mem_base_;
  GE_TIMESTAMP_END(InitModelMem, "GraphLoader::InitModelMem");
  std::vector<NodePtr> variable_nodes;
  GE_CHK_STATUS_RET(InitIoNodes(compute_graph, variable_nodes), "[Init][InitIoNodes] failed, name: %s", name_.c_str());

  std::vector<int64_t> huge_stream_list;
  (void)AttrUtils::GetListInt(ge_model_, ATTR_MODEL_HUGE_STREAM_LIST, huge_stream_list);
  const std::set<int64_t> huge_streams(huge_stream_list.begin(), huge_stream_list.end());

  for (uint32_t i = 0U; i < runtime_param_.stream_num; ++i) {
    uint32_t stream_flags = RT_STREAM_PERSISTENT;
    if (huge_streams.count(static_cast<int32_t>(i)) > 0U) {
      GELOGI("Stream %u is huge stream.", i);
      stream_flags |= RT_STREAM_HUGE;
    }
    if (hcom_streams_.count(i) > 0U) {
      stream_flags |= RT_STREAM_FORCE_COPY;
    }

    rtStream_t stream = nullptr;
    GE_CHK_RT_RET(rtStreamCreateWithFlags(&stream, priority_, stream_flags));

    stream_list_.push_back(stream);
    int32_t rt_stream_id = kInvalidStream;
    (void)rtGetStreamId(stream, &rt_stream_id);
    GELOGI("Logical stream index:%u, rtstream: %d, model: %u.", i, rt_stream_id, model_id_);
  }

  const auto create_flag = (runtime_param_.event_num > kEventReuseThreshold) ? RT_EVENT_WITH_FLAG : RT_EVENT_DEFAULT;
  for (uint32_t i = 0U; i < runtime_param_.event_num; ++i) {
    rtEvent_t rt_event = nullptr;
    GE_CHK_RT_RET(rtEventCreateWithFlag(&rt_event, static_cast<uint32_t>(create_flag)));
    event_list_.push_back(rt_event);
  }

  label_list_.resize(static_cast<size_t>(runtime_param_.label_num), nullptr);

  // create model_handle to load model
  GE_CHK_RT_RET(rtModelCreate(&rt_model_handle_, 0U));
  if (ProfilingManager::Instance().ProfilingModelExecuteOn() && (!CheckModelNoInputAndOutput())) {
    GE_CHK_RT_RET(rtModelSetExtId(rt_model_handle_, model_id_));
  }
  GE_CHK_RT_RET(rtModelGetId(rt_model_handle_, &runtime_model_id_));

  // inference will use default graph_id 0;
  runtime_param_.graph_id = compute_graph->GetGraphID();

  // op debug register
  GE_CHK_STATUS_RET(OpDebugRegister(), "[Call][OpDebugRegister] failed, model_id:%u.", model_id_);

  GE_TIMESTAMP_START(TransAllVarData);
  GE_CHK_STATUS_RET_NOLOG(TransAllVarData(compute_graph, variable_nodes));
  GE_TIMESTAMP_END(TransAllVarData, "GraphLoader::TransAllVarData");

  GE_CHK_STATUS_RET(InitNodes(compute_graph), "[Init][Nodes] failed, graph:%s.", compute_graph->GetName().c_str());

  // malloc mem for overflow detetcion
  int64_t global_workspace = 0;
  (void)AttrUtils::GetInt(compute_graph, "globleworkspace_status_bytes", global_workspace);
  if (global_workspace > 0) {
    GE_CHK_RT_RET(rtMalloc(&globalworkspace_overflow_addr_, static_cast<uint64_t>(global_workspace), RT_MEMORY_HBM));
  }

  GE_TIMESTAMP_START(DoTaskSink);
  GE_CHK_STATUS_RET(DoTaskSink(), "[Call][DoTaskSink] failed, model_id:%u.", model_id_);
  GE_TIMESTAMP_END(DoTaskSink, "GraphLoader::DoTaskSink");

  /// In zero copy model, if a aicpu operator is connected to the first or last layer, before model execution,
  /// the aicpu opertor needs to destroy history record, and update operator memory address.
  /// The model with specified aicpu operators is only marked here, and destruction is in ModelManager::ExecuteModel().
  need_destroy_aicpu_kernel_ = IsAicpuKernelConnectSpecifiedLayer();

  std::string fp_ceiling_mode;
  if (AttrUtils::GetStr(ge_model_, ATTR_FP_CEILING_MODE, fp_ceiling_mode)) {
    GELOGI("Get attr ATTR_FP_CEILING_MODE from model, value is %s.", fp_ceiling_mode.c_str());
    // mode 0: Do not perform saturation processing. By default, IEEE754 is used.
    GE_CHK_RT_RET(rtSetCtxINFMode((fp_ceiling_mode != "0")));
  }

  SetProfileTime(ModelProcStage::MODEL_LOAD_END);
  // collect profiling for ge
  auto &prof_mgr = ProfilingManager::Instance();
  if (prof_mgr.ProfilingModelLoadOn()) {
    GE_CHK_STATUS_RET(ReportProfilingData(), "[Report][ProfilingData] failed, model_id:%u.", model_id_);
  } else {
    prof_mgr.RecordUnReportedModelId(model_id_);
  }

  Shrink();
  return SUCCESS;
}

// save specify attr values of op, such as ATTR_NAME_DATA_DUMP_ORIGIN_OP_NAMES
// it will save more attr values in the future
void DavinciModel::SaveSpecifyAttrValues(const OpDescPtr &op_desc) {
  std::vector<std::string> value;
  if (AttrUtils::GetListStr(op_desc, ATTR_NAME_DATA_DUMP_ORIGIN_OP_NAMES, value)) {
    op_name_to_attrs_[op_desc->GetName()] = { {ATTR_NAME_DATA_DUMP_ORIGIN_OP_NAMES, value} };
    GELOGD("Get op:%s attr:%s success.", op_desc->GetName().c_str(), ATTR_NAME_DATA_DUMP_ORIGIN_OP_NAMES.c_str());
  }
}

///
/// @ingroup ge
/// @brief Travel all nodes and determine if destruction is required.
/// @return bool
///
bool DavinciModel::IsAicpuKernelConnectSpecifiedLayer() const {
  const Graph &graph = ge_model_->GetGraph();
  const ComputeGraphPtr &compute_graph = GraphUtils::GetComputeGraph(graph);
  const auto all_nodes = compute_graph->GetAllNodes();
  for (const auto &node : all_nodes) {
    GE_IF_BOOL_EXEC(node == nullptr, continue);
    const OpDescPtr op_desc = node->GetOpDesc();
    GE_IF_BOOL_EXEC(op_desc == nullptr, continue);

    int64_t imply_type = -1;
    (void)AttrUtils::GetInt(op_desc, ATTR_NAME_IMPLY_TYPE, imply_type);
    if (imply_type != static_cast<int64_t>(domi::ImplyType::AI_CPU)) {
      continue;
    }
    GELOGD("Current operator imply type is %ld, name is %s.", imply_type, op_desc->GetName().c_str());

    for (const auto &in_data_anchor : node->GetAllInDataAnchors()) {
      GE_IF_BOOL_EXEC(in_data_anchor == nullptr, continue);
      const auto peer_out_data_anchor = in_data_anchor->GetPeerOutAnchor();
      GE_IF_BOOL_EXEC(peer_out_data_anchor == nullptr, continue);
      const auto peer_node = peer_out_data_anchor->GetOwnerNode();
      GE_IF_BOOL_EXEC(peer_node == nullptr, continue);
      const auto peer_op_desc = peer_node->GetOpDesc();
      GE_IF_BOOL_EXEC(peer_op_desc == nullptr, continue);
      if (kDataOpTypes.count(peer_op_desc->GetType()) > 0U) {
        GELOGI("Mark specified aicpu operator connected to data.");
        return true;
      }
    }
    for (const auto &out_data_anchor : node->GetAllOutDataAnchors()) {
      GE_IF_BOOL_EXEC(out_data_anchor == nullptr, continue);
      const auto peer_in_data_anchors = out_data_anchor->GetPeerInDataAnchors();
      for (const auto &peer_in_data_anchor : peer_in_data_anchors) {
        GE_IF_BOOL_EXEC(peer_in_data_anchor == nullptr, continue);
        const auto peer_node = peer_in_data_anchor->GetOwnerNode();
        GE_IF_BOOL_EXEC(peer_node == nullptr, continue);
        const auto peer_op_desc = peer_node->GetOpDesc();
        GE_IF_BOOL_EXEC(peer_op_desc == nullptr, continue);
        if (peer_op_desc->GetType() == NETOUTPUT) {
          GELOGI("Mark specified aicpu operator connected to netoutput.");
          return true;
        }
      }
    }
  }

  return false;
}

Status DavinciModel::UpdateSessionId(const uint64_t session_id) {
  GE_CHECK_NOTNULL(ge_model_);
  if (!AttrUtils::SetInt(ge_model_, MODEL_ATTR_SESSION_ID, static_cast<int64_t>(session_id))) {
    GELOGW("Set attr[%s] failed in updating session_id.", MODEL_ATTR_SESSION_ID.c_str());
  }

  GELOGD("Update session id: %lu.", session_id);
  return SUCCESS;
}

Status DavinciModel::InitIoNodes(const ComputeGraphPtr &compute_graph, std::vector<NodePtr> &variable_nodes) {
  uint32_t data_op_index = 0U;
  std::map<uint32_t, OpDescPtr> data_by_index;
  std::vector<OpDescPtr> output_op_list;
  std::set<const void *> input_outside_addrs;
  std::set<const void *> output_outside_addrs;
  std::vector<NodePtr> queue_data_nodes;
  for (const auto &node : compute_graph->GetAllNodes()) {
    const auto &op_desc = node->GetOpDesc();
    GE_CHECK_NOTNULL(op_desc);
    if (kDataOpTypes.count(op_desc->GetType()) > 0U) {
      GE_CHK_STATUS_RET_NOLOG(InitDataOp(compute_graph, node, data_op_index, data_by_index, input_outside_addrs));
      data_dumper_.SaveDumpInput(node);
      continue;
    }

    if (op_desc->GetType() == NETOUTPUT) {
      GE_CHK_STATUS_RET_NOLOG(InitNetOutput(compute_graph, node, output_op_list, output_outside_addrs));
      GE_CHK_STATUS_RET_NOLOG(InitRealSizeAndShapeInfo(compute_graph, node));
      continue;
    }

    if (op_desc->GetType() == VARIABLE) {
      variable_nodes.emplace_back(node);
      continue;
    }

    if (op_desc->GetType() == QUEUE_DATA) {
      queue_data_nodes.emplace_back(node);
      continue;
    }

    // CheckHasHcomOp
    if (HcomOmeUtil::IsHCOMOp(op_desc->GetType()) || HcomOmeUtil::IsHorovodOp(op_desc->GetType())) {
      const uint32_t stream_id = static_cast<uint32_t>(op_desc->GetStreamId());
      (void)hcom_streams_.emplace(stream_id);
      GELOGD("hcom stream: %u.", stream_id);
      continue;
    }
  }

  GE_CHK_STATUS_RET_NOLOG(GenInputOutputInfo(data_by_index, output_op_list));
  GE_CHK_STATUS_RET_NOLOG(InitQueueDataNodes(queue_data_nodes, data_op_index, input_outside_addrs));
  GE_CHK_STATUS_RET_NOLOG(GenInputOutputIndex());
  return SUCCESS;
}

///
/// @ingroup ge
/// @brief Travel all nodes and do some init.
/// @param [in] compute_graph: ComputeGraph to load.
/// @return Status
///
Status DavinciModel::InitNodes(const ComputeGraphPtr &compute_graph) {
  GE_TIMESTAMP_CALLNUM_START(InitTbeHandle);

  using OpDescCall = std::function<Status(DavinciModel *, const OpDescPtr &)>;
  static std::map<std::string, OpDescCall> op_desc_handle = {
      {CONSTANTOP, &DavinciModel::InitConstant},
      {STREAMACTIVE, &DavinciModel::InitStreamActive},
      {STREAMSWITCH, &DavinciModel::InitStreamSwitch},
      {LABELSET, &DavinciModel::InitLabelSet},
      {CASE, &DavinciModel::InitCase},
  };

  std::map<std::string, OpDescPtr> variable_by_name;
  const auto &nodes = compute_graph->GetAllNodes();
  for (size_t i = 0U; i < nodes.size(); ++i) {
    const auto &node = nodes.at(i);
    const auto &op_desc = node->GetOpDesc();
    GE_CHECK_NOTNULL(op_desc);
    SaveSpecifyAttrValues(op_desc);
    DisableZeroCopyNode(op_desc);
    op_list_[op_desc->GetId()] = op_desc;

    if (kIoNodeTypes.count(op_desc->GetType()) > 0U) {
      continue;
    }

    if (op_desc->GetType() == VARIABLE) {
      GE_CHK_STATUS_RET_NOLOG(InitVariable(op_desc, variable_by_name));
      continue;
    }

    if (op_desc->GetType() == FILECONSTANT) {
      GE_CHK_STATUS_RET_NOLOG(InitFileConstant(node));
      continue;
    }

    GE_CHK_STATUS_RET_NOLOG(AllocateResource(*node));

    // for dynamic shape with control flow
    SetLabelForDynamic(node);

    const auto it = op_desc_handle.find(op_desc->GetType());
    if (it != op_desc_handle.end()) {
      GE_CHK_BOOL_TRUE_EXEC_WITH_LOG((it->second)(this, op_desc) != SUCCESS, return PARAM_INVALID,
                                     "[Init][Node] failed, Name:%s", op_desc->GetName().c_str());
      continue;
    }

    if (IsNoTaskAndDumpNeeded(op_desc)) {
      GELOGD("node[%s] without task, and save op_desc and addr for dump", op_desc->GetName().c_str());
      const std::vector<void *> input_data_addrs = ModelUtils::GetInputAddrs(runtime_param_, op_desc);
      const std::vector<void *> output_data_addrs = ModelUtils::GetOutputAddrs(runtime_param_, op_desc);
      const std::vector<void *> workspace_data_addrs = ModelUtils::GetWorkspaceDataAddrs(runtime_param_, op_desc);
      std::vector<void *> device_addrs;
      (void)device_addrs.insert(device_addrs.cend(), input_data_addrs.cbegin(), input_data_addrs.cend());
      (void)device_addrs.insert(device_addrs.cend(), output_data_addrs.cbegin(), output_data_addrs.cend());
      (void)device_addrs.insert(device_addrs.cend(), workspace_data_addrs.cbegin(), workspace_data_addrs.cend());
      void *addr = nullptr;
      const auto addr_size = kAddrSize * device_addrs.size();
      GE_CHK_RT_RET(rtMalloc(&addr, addr_size, RT_MEMORY_HBM));
      saved_task_addrs_[op_desc] = addr;

      GE_CHK_RT_RET(rtMemcpy(addr, addr_size, device_addrs.data(), addr_size, RT_MEMCPY_HOST_TO_DEVICE));
    }

    GE_TIMESTAMP_RESTART(InitTbeHandle);
    GE_CHK_STATUS_RET(InitTbeHandle(op_desc), "[Init][TbeHandle] failed. op:%s", op_desc->GetName().c_str());
    GE_TIMESTAMP_ADD(InitTbeHandle);
  }

  SetDataDumperArgs(compute_graph, variable_by_name);
  GE_TIMESTAMP_CALLNUM_END(InitTbeHandle, "GraphLoader::InitTbeHandle.");
  return SUCCESS;
}

void DavinciModel::SetLabelForDynamic(const NodePtr &node) {
  if (known_node_ && ((node->GetType() == LABELSWITCHBYINDEX) || (node->GetType() == STREAMSWITCH))) {
    for (auto &in_data_anchor : node->GetAllInDataAnchors()) {
      const auto peer_out_data_anchor = in_data_anchor->GetPeerOutAnchor();
      if (peer_out_data_anchor != nullptr) {
        // name+index as the label of switch input
        const std::string tensor_name = node->GetName() + std::to_string(in_data_anchor->GetIdx());
        const auto peer_node = peer_out_data_anchor->GetOwnerNode();
        (void)AttrUtils::SetStr(peer_node->GetOpDesc(), ATTR_DYNAMIC_SHAPE_FIXED_ADDR, tensor_name);
        (void)AttrUtils::SetInt(peer_node->GetOpDesc(), ATTR_DYNAMIC_SHAPE_FIXED_ADDR_INDEX, 0);
        tensor_name_to_peer_output_index_[tensor_name] = 0;
      }
    }
  }
}

///
/// @ingroup ge
/// @brief Data Op Initialize.
/// @param [in] ComputeGraphPtr: root graph of the model.
/// @param [in] NodePtr: Data Op.
/// @param [in/out] data_op_index: index of courrent count.
/// @param [in/out] data_by_index: Data ordered by index.
/// @return Status
///
Status DavinciModel::InitDataOp(const ComputeGraphPtr &graph, const NodePtr &node, uint32_t &data_op_index,
                                std::map<uint32_t, OpDescPtr> &data_by_index,
                                std::set<const void *> &input_outside_addrs) {
  // op_desc Checked by Init: Data, valid.
  const auto op_desc = node->GetOpDesc();
  if (node->GetOwnerComputeGraph() != graph) {
    GELOGI("Skip Data node: %s in subgraph.", op_desc->GetName().c_str());
    return SUCCESS;
  }

  auto data_index = data_op_index++;
  const auto &index_attr = (GraphUtils::FindRootGraph(graph) == graph) ? ATTR_NAME_INDEX : ATTR_NAME_PARENT_NODE_INDEX;
  if (AttrUtils::GetInt(op_desc, index_attr, data_index)) {
    GELOGD("Get new index %u, old %u", data_index, data_op_index - 1U);
  }
  GELOGI("Init data node: %s, index: %u.", op_desc->GetName().c_str(), data_index);

  const auto &anchor = node->GetOutDataAnchor(0);
  if ((anchor != nullptr) && (anchor->GetFirstPeerAnchor() != nullptr) &&
      (anchor->GetFirstPeerAnchor()->GetOwnerNode() != nullptr)) {
    const auto &node_desc = anchor->GetFirstPeerAnchor()->GetOwnerNode()->GetOpDesc();
    const auto anchor_idx = static_cast<size_t>(anchor->GetFirstPeerAnchor()->GetIdx());
    std::vector<int64_t> op_max_size;
    if (AttrUtils::GetListInt(node_desc, "_op_max_size", op_max_size) && (op_max_size.size() > anchor_idx)) {
      (void)AttrUtils::SetInt(op_desc, "_op_max_size", op_max_size[anchor_idx]);
    }
  }

  data_by_index[data_index] = op_desc;
  if (known_node_) {
    return SUCCESS;
  }

  GE_CHK_STATUS_RET_NOLOG(InitInputZeroCopy(op_desc, data_index, input_outside_addrs));
  return SUCCESS;
}

Status DavinciModel::InitInputZeroCopy(const OpDescPtr &op_desc,
                                       const uint32_t data_index,
                                       std::set<const void *> &input_outside_addrs) {
  // Make information for copy input data.
  const std::vector<int64_t> output_size_list = ModelUtils::GetOutputSize(op_desc);
  const std::vector<void *> virtual_addr_list = ModelUtils::GetOutputAddrs(runtime_param_, op_desc);
  const std::vector<int64_t> output_offset_list = op_desc->GetOutputOffset();
  if (output_size_list.empty() || virtual_addr_list.empty() || (output_size_list.size() != virtual_addr_list.size()) ||
      (output_offset_list.size() != virtual_addr_list.size())) {
    REPORT_INNER_ERROR(
        "E19999", "Check data fail in op:%s(%s), output_desc size:%zu output addr size:%zu output offset size:%zu "
        "not equal or has empty, model_id:%u",
        op_desc->GetName().c_str(), op_desc->GetType().c_str(),
        output_size_list.size(), virtual_addr_list.size(), output_offset_list.size(), model_id_);
    GELOGE(PARAM_INVALID, "[Check][Param] Data[%s] init failed: output size is %zu, "
           "virtual_addr size is %zu, offset size is %zu.", op_desc->GetName().c_str(), output_size_list.size(),
           virtual_addr_list.size(), output_offset_list.size());
    return PARAM_INVALID;
  }

  bool fusion_flag = false;
  ZeroCopyOffset zero_copy_offset;
  const int64_t data_size = output_size_list[kDataIndex];
  const auto ret = zero_copy_offset.InitInputDataInfo(data_size, virtual_addr_list[kDataIndex], op_desc, fusion_flag);
  if (ret != SUCCESS) {
    GELOGE(PARAM_INVALID, "[Init][DataInfo] of input_info %s failed.", op_desc->GetName().c_str());
    return PARAM_INVALID;
  }

  const void *const virtual_addr = virtual_addr_list[kDataIndex];
  if (input_outside_addrs.count(virtual_addr) == 0U) {
    const int64_t output_offset = output_offset_list.at(kDataIndex);
    zero_copy_offset.SetInputOutsideAddrs(output_offset, PtrToValue(virtual_addr), fusion_flag, real_virtual_addrs_);
    (void)input_outside_addrs.insert(virtual_addr);
  }
  input_data_info_[data_index] = zero_copy_offset;

  if (ExecutorUtils::IsReuseZeroCopyMemory()) {
    bool is_zero_copy_block = false;
    (void)ge::AttrUtils::GetBool(op_desc->GetOutputDescPtr(kDataIndex), ATTR_IS_ZERO_COPY_BLOCK, is_zero_copy_block);
    if (!is_zero_copy_block) {
      GELOGI("Disable zero copy for %p of data %s", virtual_addr, op_desc->GetName().c_str());
      const std::lock_guard<std::mutex> lk(outside_addrs_mutex_);
      (void)copy_only_addrs_.insert(virtual_addr);
    }
  }

  bool is_no_tiling = false;
  (void)AttrUtils::GetBool(op_desc->GetOutputDescPtr(kDataIndex), ATTR_NAME_TENSOR_NO_TILING_MEM_TYPE, is_no_tiling);
  input_no_tiling_flag_.push_back(is_no_tiling);
  if (is_no_tiling) {
    has_no_tiling_input_ = true;
  }
  return SUCCESS;
}

///
/// @ingroup ge
/// @brief Sort Data op list by index.
/// @param [in] data_by_index: map of Data Op.
/// @param [in] output_op_list: list of NetOutput op.
/// @return Status
///
Status DavinciModel::GenInputOutputInfo(const std::map<uint32_t, OpDescPtr> &data_by_index,
                                        const std::vector<OpDescPtr> &output_op_list) {
  GELOGD("Data node size: %zu, NetOutput node size: %zu", data_by_index.size(), output_op_list.size());
  for (auto &item : data_by_index) {
    const auto output_addrs = ModelUtils::GetOutputAddrs(runtime_param_, item.second);
    GELOGD("Data node is: %s, output addr size: %zu", item.second->GetName().c_str(), output_addrs.size());
    input_addrs_list_.emplace_back(output_addrs);

    GE_CHK_STATUS_RET(InitAippInfo(item.first, item.second),
                      "[Init][AippInfo] failed, node:%s", item.second->GetName().c_str());
    GE_CHK_STATUS_RET(InitAippType(item.first, item.second, data_by_index),
                      "[Init][AippType] failed, node:%s", item.second->GetName().c_str());
    GE_CHK_STATUS_RET(InitOrigInputInfo(item.first, item.second),
                      "[Init][OrigInputInfo] failed, node:%s", item.second->GetName().c_str());
    GE_CHK_STATUS_RET(InitAippInputOutputDims(item.first, item.second),
                      "[Init][AippInputOutputDims] failed, node:%s", item.second->GetName().c_str());
    GE_CHK_STATUS_RET(InitInputDescInfo(item.second),
                      "[Init][InputDescInfo] failed, node:%s", item.second->GetName().c_str());
    if (item.second->GetType() == AIPP_DATA_TYPE) {
      GELOGI("This is dynamic aipp model, Node: %s", item.second->GetName().c_str());
      is_dynamic_aipp_ = true;
    }
  }

  std::vector<std::string> out_node_name;
  (void)AttrUtils::GetListStr(ge_model_, ATTR_MODEL_OUT_NODES_NAME, out_node_name);
  GELOGD("Output node size: %zu, out nodes name is: %zu", output_op_list.size(), out_node_name.size());
  for (const auto &op_desc : output_op_list) {
    const auto input_addrs = ModelUtils::GetInputAddrs(runtime_param_, op_desc);
    GELOGD("NetOutput node is: %s, input addr size: %zu", op_desc->GetName().c_str(), input_addrs.size());
    output_addrs_list_.emplace_back(input_addrs);

    bool getnext_sink_dynamic = false;
    if (AttrUtils::GetBool(op_desc, ATTR_GETNEXT_SINK_DYNMAIC, getnext_sink_dynamic) && getnext_sink_dynamic) {
      GELOGI("ATTR_GETNEXT_SINK_DYNMAIC has been set and is true, node: %s", op_desc->GetName().c_str());
      is_getnext_sink_dynamic_ = true;
    }

    std::vector<std::string> shape_info;
    if (AttrUtils::GetListStr(op_desc, ATTR_NAME_DYNAMIC_OUTPUT_DIMS, shape_info)) {
      (void)dynamic_output_shape_info_.insert(dynamic_output_shape_info_.cend(), shape_info.cbegin(),
                                              shape_info.cend());
    }

    if (InitOutputTensorInfo(op_desc) != SUCCESS) {
      return INTERNAL_ERROR;
    }

    GE_CHK_STATUS_RET(InitOutputDescInfo(op_desc, out_node_name),
                      "[Init][OutputDescInfo] failed, node:%s", op_desc->GetName().c_str());
  }

  return SUCCESS;
}

Status DavinciModel::GenInputOutputIndex() {
  for (size_t i = 0U; i < input_addrs_list_.size(); ++i) {
    const std::vector<void *> &addr_list = input_addrs_list_[i];
    if (!addr_list.empty()) {
      const void *const addr = addr_list[kDataIndex];
      known_input_idx_[addr] = i;
      GELOGI("input %zu, v addr %p.", i, addr);
    }
  }

  if (!output_addrs_list_.empty()) {
    const std::vector<void *> &addr_list = output_addrs_list_.front();
    for (size_t i = 0U; i < addr_list.size(); ++i) {
      const void *const addr = addr_list[i];
      known_output_idx_[addr] = i;
      GELOGI("output %zu, v addr %p.", i, addr);
    }
  }

  return SUCCESS;
}

bool DavinciModel::IsGetNextSinkDynamic(const OpDescPtr &op_desc) const {
  bool getnext_sink_dynamic = false;
  if (AttrUtils::GetBool(op_desc, ATTR_GETNEXT_SINK_DYNMAIC, getnext_sink_dynamic) && getnext_sink_dynamic) {
    GELOGI("ATTR_GETNEXT_SINK_DYNMAIC has been set and is true.");
    return true;
  }
  return false;
}

/// @ingroup ge
/// @brief NetOutput Op Initialize.
/// @param [in] ComputeGraphPtr: root graph of the model.
/// @param [in] NodePtr: NetOutput Op.
/// @param [in/out] std::vector<OpDescPtr>: All NetOutput node in model.
/// @return Status
Status DavinciModel::InitNetOutput(const ComputeGraphPtr &graph, const NodePtr &node,
                                   std::vector<OpDescPtr> &output_op_list,
                                   std::set<const void *> &output_outside_addrs) {
  // node->GetOpDesc Checked by Init: NetOutput, valid.
  const auto op_desc = node->GetOpDesc();
  // excludes the function op sub graph, e.g. case,if
  if (node->GetOwnerComputeGraph() != graph) {
    GELOGI("Skip subgraph NetOutput node: %s.", op_desc->GetName().c_str());
    (void)op_list_.erase(op_desc->GetId());
    return SUCCESS;
  }

  GELOGI("Init NetOutput node: %s.", op_desc->GetName().c_str());
  output_op_list.push_back(op_desc);
  has_output_node_ = true;
  if (known_node_) {
    return SUCCESS;
  }

  // Make information for copy output data.
  const std::vector<int64_t> input_size_list = ModelUtils::GetInputSize(op_desc);
  const std::vector<void *> virtual_addr_list = ModelUtils::GetInputAddrs(runtime_param_, op_desc);
  const std::vector<int64_t> input_offset_list = op_desc->GetInputOffset();
  GE_IF_BOOL_EXEC(input_offset_list.size() != virtual_addr_list.size(),
                  REPORT_INNER_ERROR("E19999", "Check data fail in op:%s(%s), input addr size:%zu "
                                     "input offset size:%zu not equal, model_id:%u", op_desc->GetName().c_str(),
                                     op_desc->GetType().c_str(), virtual_addr_list.size(), input_offset_list.size(),
                                     model_id_);
                  GELOGE(PARAM_INVALID, "[Check][Param] virtual_addr size:%zu should be equal to offset size:%zu, "
                         "op:%s(%s), model id:%u", virtual_addr_list.size(), input_offset_list.size(),
                         op_desc->GetName().c_str(), op_desc->GetType().c_str(), model_id_);
                  return PARAM_INVALID;);
  if (input_size_list.empty() && virtual_addr_list.empty()) {
    GELOGI("NetOutput[%s] is empty.", op_desc->GetName().c_str());
    return SUCCESS;
  }
  if (input_size_list.empty() || (input_size_list.size() != virtual_addr_list.size())) {
    REPORT_INNER_ERROR("E19999", "Check data fail in op:%s(%s), input_desc size:%zu input addr size:%zu "
                       "not equal or has empty, model_id:%u", op_desc->GetName().c_str(), op_desc->GetType().c_str(),
                       input_size_list.size(), virtual_addr_list.size(), model_id_);
    GELOGE(PARAM_INVALID, "[Check][Param] NetOutput[%s] init failed: Input size is %zu, Input addr is %zu",
           op_desc->GetName().c_str(), input_size_list.size(), virtual_addr_list.size());
    return PARAM_INVALID;
  }

  const size_t num = output_data_info_.size();
  size_t input_count = input_size_list.size();
  is_getnext_sink_dynamic_ = false;
  if (IsGetNextSinkDynamic(op_desc)) {
    input_count = input_size_list.size() - kGetDynamicDimsCount;
    is_getnext_sink_dynamic_ = true;
  }
  const bool is_reuse_zero_copy_memory = ExecutorUtils::IsReuseZeroCopyMemory();
  for (size_t idx = 0U; idx < input_count; ++idx) {
    ZeroCopyOffset zero_copy_offset;
    bool fusion_flag = false;
    const auto ret = zero_copy_offset.InitOutputDataInfo(input_size_list, virtual_addr_list, op_desc, idx, fusion_flag);
    GE_IF_BOOL_EXEC(ret != SUCCESS,
                    GELOGE(PARAM_INVALID, "[Init][DataInfo] of input_info %s failed.", op_desc->GetName().c_str());
                    return PARAM_INVALID;);
    const void *const addr = virtual_addr_list.at(idx);
    const int64_t input_offset = input_offset_list.at(idx);
    if (output_outside_addrs.count(addr) != 0U) {
      GELOGI("same output_tensor_addr %p to different input_tensor of %s", addr, op_desc->GetName().c_str());
      DisableZeroCopy(addr);
    } else {
      std::vector<const void *> tensor_addrs;
      zero_copy_offset.SetOutputOutsideAddrs(input_offset, fusion_flag, PtrToValue(addr), tensor_addrs);
      (void)output_outside_addrs.insert(addr);

      if (is_reuse_zero_copy_memory) {
        DisableZeroCopyInReuseMemoryMode(node, idx, addr);
      } else {
        for (const auto &real_addr : tensor_addrs) {
          DisableZeroCopy(real_addr);
          (void)real_virtual_addrs_.insert(real_addr);
        }
      }
    }
    output_data_info_[num + idx] = zero_copy_offset;
  }
  return SUCCESS;
}

Status DavinciModel::InitRealSizeAndShapeInfo(const ComputeGraphPtr &compute_graph, const NodePtr &node) {
  if (node->GetName().find(kMultiBatchNodePostfix) != std::string::npos) {
    GELOGD("No need to get size and shape of netoutput in subgraph.");
    return SUCCESS;
  }
  GELOGD("Start to initialize real size and shape info of %s.", node->GetName().c_str());
  GetAllGearsInfo(node);
  if (is_getnext_sink_dynamic_) {
    if (GetDynamicDimsNodeInfo(node) != SUCCESS) {
      GELOGE(PARAM_INVALID, "[Get][Info] of getdynamicdims node:%s failed.", node->GetName().c_str());
      return PARAM_INVALID;
    }
  }
  if (is_online_infer_dynamic_) {
    if (GetGearAndRealOutSizeInfo(compute_graph, node) != SUCCESS) {
      GELOGE(PARAM_INVALID, "[Call][GetGearAndRealOutSizeInfo] failed, node:%s.", node->GetName().c_str());
      return PARAM_INVALID;
    }
    if (GetGearAndRealOutShapeInfo(node) != SUCCESS) {
      GELOGE(PARAM_INVALID, "[Call][GetGearAndRealOutShapeInfo] failed, node:%s.", node->GetName().c_str());
      return PARAM_INVALID;
    }
  }

  return SUCCESS;
}

void DavinciModel::GetAllGearsInfo(const NodePtr &node) {
  is_online_infer_dynamic_ = false;
  all_gears_info_.clear();
  std::string shapes;
  (void)AttrUtils::GetStr(node->GetOpDesc(), ATTR_ALL_GEARS_INFO, shapes);
  if (!shapes.empty()) {
    is_online_infer_dynamic_ = true;
    const auto shape_strs = StringUtils::Split(shapes, ';');
    for (const auto &shape_str : shape_strs) {
      if (shape_str.empty()) {
        continue;
      }
      std::vector<int32_t> gear_info;
      const auto dims = StringUtils::Split(shape_str, ',');
      for (const auto &dim : dims) {
        if (dim.empty()) {
          continue;
        }
        gear_info.emplace_back(std::strtol(dim.c_str(), nullptr, kDecimalRadix));
      }
      if (!gear_info.empty()) {
        all_gears_info_.emplace_back(gear_info);
        GELOGD("Init all gears info from %s, gear info is %s", node->GetName().c_str(), ToString(gear_info).c_str());
      }
    }
  }
}

Status DavinciModel::GetDynamicDimsNodeInfo(const NodePtr &node) {
  GE_CHECK_NOTNULL(node->GetOpDesc());
  const size_t input_count = node->GetAllInDataAnchors().size();
  GELOGI("input_anchor count of %s is %zu.", node->GetName().c_str(), input_count);
  const size_t get_dynamic_dims_index = input_count - kGetDynamicDimsCount;
  const auto in_anchor = node->GetAllInDataAnchors().at(get_dynamic_dims_index);
  const auto peer_out_anchor = in_anchor->GetPeerOutAnchor();
  GE_CHECK_NOTNULL(peer_out_anchor);

  const auto peer_node = peer_out_anchor->GetOwnerNode();
  const auto op_desc = peer_node->GetOpDesc();
  GE_CHECK_NOTNULL(op_desc);
  if ((op_desc->GetName() == kGetDynamicDimsName) && (op_desc->GetType() == GETDYNAMICDIMS)) {
    GELOGD("Start get info of %s.", op_desc->GetName().c_str());
    const auto input_addr = ModelUtils::GetInputAddrs(runtime_param_, node->GetOpDesc());
    const auto input_size = ModelUtils::GetInputSize(node->GetOpDesc());
    if (input_addr.empty() || input_size.empty()) {
      REPORT_INNER_ERROR("E19999", "input addr size:%zu or input size:%zu in op:%s(%s) has empty, model_id:%u "
                         "check invalid", input_addr.size(), input_size.size(),
                         node->GetName().c_str(), node->GetType().c_str(), model_id_);
      GELOGE(PARAM_INVALID, "[Check][Param] input addr size:%zu or input size:%zu in op:%s(%s) is empty, model_id:%u",
             input_addr.size(), input_size.size(), node->GetName().c_str(), node->GetType().c_str(), model_id_);
      return PARAM_INVALID;
    }
    const auto input_desc = node->GetOpDesc()->GetInputDescPtr(static_cast<uint32_t>(get_dynamic_dims_index));
    GE_CHECK_NOTNULL(input_desc);
    if (input_desc->GetShape().GetDims().empty()) {
      REPORT_INNER_ERROR("E19999", "input_desc_index:%zu in op:%s(%s) shape dim is empty, model_id:%u, check invalid",
                         get_dynamic_dims_index, node->GetName().c_str(), node->GetType().c_str(), model_id_);
      GELOGE(PARAM_INVALID, "[Check][Param] input_desc_index:%zu in op:%s(%s) shape dim is empty, model_id:%u",
             get_dynamic_dims_index, node->GetName().c_str(), node->GetType().c_str(), model_id_);
      return PARAM_INVALID;
    }
    netoutput_last_input_addr_ = input_addr[get_dynamic_dims_index];
    netoutput_last_input_size_ = input_size[get_dynamic_dims_index];
    shape_of_cur_dynamic_dims_ = static_cast<size_t>(input_desc->GetShape().GetDims().at(0U));
    GELOGD("Shape of cur dynamic dims is %zu, size is %ld, addr is %p.", shape_of_cur_dynamic_dims_,
           netoutput_last_input_size_, netoutput_last_input_addr_);
  }
  return SUCCESS;
}

Status DavinciModel::GetGearAndRealOutSizeInfo(const ComputeGraphPtr &graph, const NodePtr &node) {
  GELOGD("Start get gear and real output size info of %s.", node->GetName().c_str());
  merge_nodes_gear_and_real_out_size_info_.clear();
  size_t idx = 0U;
  for (const auto &in_anchor : node->GetAllInDataAnchors()) {
    const auto peer_out_anchor = in_anchor->GetPeerOutAnchor();
    if (peer_out_anchor == nullptr) {
      continue;
    }
    const auto peer_node = peer_out_anchor->GetOwnerNode();
    const auto op_desc = peer_node->GetOpDesc();
    GE_CHECK_NOTNULL(op_desc);
    if ((peer_node->GetType() == CASE) && (op_desc->HasAttr(ATTR_INSERT_BY_MBATCH))) {
      if (GetRealOutputSizeOfCase(graph, idx, peer_node) != SUCCESS) {
        GELOGE(PARAM_INVALID, "[Get][RealOutputSizeOfCase] %s failed.", peer_node->GetName().c_str());
        return PARAM_INVALID;
      }
    }
    idx++;
  }
  return SUCCESS;
}

Status DavinciModel::GetRealOutputSizeOfCase(const ComputeGraphPtr &graph, const size_t input_index,
                                             const NodePtr &case_node) {
  GELOGD("Start to get output size of %s, which is %zu input to netoutput", case_node->GetName().c_str(), input_index);
  const auto &func_desc = case_node->GetOpDesc();
  GE_CHECK_NOTNULL(func_desc);
  auto &gear_and_real_out_size_info = merge_nodes_gear_and_real_out_size_info_[input_index];
  for (const auto &name : func_desc->GetSubgraphInstanceNames()) {
    const auto &subgraph = graph->GetSubgraph(name);
    if (subgraph == nullptr) {
      REPORT_INNER_ERROR("E19999", "Get name:%s subgraph in graph:%s fail, model_id:%u, check invalid",
                         name.c_str(), graph->GetName().c_str(), model_id_);
      GELOGE(GE_GRAPH_EMPTY_SUBGRAPH, "[Get][Subgraph] %s in graph:%s failed, model_id:%u.",
             name.c_str(), graph->GetName().c_str(), model_id_);
      return GE_GRAPH_EMPTY_SUBGRAPH;
    }
    for (auto &node : subgraph->GetDirectNode()) {
      if (node->GetType() == NETOUTPUT) {
        const auto op_desc = node->GetOpDesc();
        GE_CHECK_NOTNULL(op_desc);
        std::string batch_label;
        if (AttrUtils::GetStr(op_desc, ATTR_NAME_BATCH_LABEL, batch_label)) {
          const size_t batch_index = static_cast<size_t>(stoi(batch_label.substr(batch_label.rfind('_') + 1U)));
          GELOGD("Batch index of %s is %zu.", op_desc->GetName().c_str(), batch_index);
          if (batch_index > all_gears_info_.size()) {
            REPORT_INNER_ERROR("E19999", "Batch_index:%zu in op:%s(%s) > all_gears_info.size:%zu, model_id:%u, "
                               "check invalid", batch_index, op_desc->GetName().c_str(), op_desc->GetType().c_str(),
                               all_gears_info_.size(), model_id_);
            GELOGE(PARAM_INVALID, "[Check][Param] Batch_index:%zu in op:%s(%s) > all_gears_info.size:%zu, model_id:%u.",
                   batch_index, op_desc->GetName().c_str(), op_desc->GetType().c_str(),
                   all_gears_info_.size(), model_id_);
            return PARAM_INVALID;
          }

          const std::vector<int64_t> input_size_list = ModelUtils::GetInputSize(op_desc);
          const auto tensor_desc = op_desc->GetInputDescPtr(static_cast<uint32_t>(input_index));
          GE_CHECK_NOTNULL(tensor_desc);
          int64_t data_size = 0;
          if (TensorUtils::GetTensorSizeInBytes(*tensor_desc, data_size) != GRAPH_SUCCESS) {
            REPORT_INNER_ERROR("E19999", "Get input TensorSize in op:%s(%s) failed, input_index:%zu, model_id:%u",
                               op_desc->GetName().c_str(), op_desc->GetType().c_str(), input_index, model_id_);
            GELOGE(FAILED, "[Get][TensorSize] in op:%s(%s) failed, input_index:%zu, model_id:%u",
                   op_desc->GetName().c_str(), op_desc->GetType().c_str(), input_index, model_id_);
            return FAILED;
          }
          gear_and_real_out_size_info[all_gears_info_[batch_index]] = data_size;
          GELOGD("Get real gear index is: %zu, gear info is %s, size is %ld, tensor size is %ld",
                 batch_index, ToString(all_gears_info_[batch_index]).c_str(), input_size_list[input_index], data_size);
        }
        break;
      }
    }
  }
  return SUCCESS;
}

Status DavinciModel::GetGearAndRealOutShapeInfo(const NodePtr &node) {
  GELOGD("Start to get dynamic output dims of %s", node->GetName().c_str());
  merge_nodes_gear_and_real_out_shape_info_.clear();
  size_t idx = 0U;
  for (const auto &in_anchor : node->GetAllInDataAnchors()) {
    const auto peer_out_anchor = in_anchor->GetPeerOutAnchor();
    if (peer_out_anchor == nullptr) {
      continue;
    }
    const auto peer_node = peer_out_anchor->GetOwnerNode();
    const auto op_desc = peer_node->GetOpDesc();
    GE_CHECK_NOTNULL(op_desc);
    if ((peer_node->GetType() == CASE) && (op_desc->HasAttr(ATTR_INSERT_BY_MBATCH))) {
      std::vector<std::string> dynamic_output_shape_info;
      if (!AttrUtils::GetListStr(node->GetOpDesc(), ATTR_NAME_DYNAMIC_OUTPUT_DIMS, dynamic_output_shape_info)) {
        GELOGD("Can not get dynamic output dims attr from %s", node->GetName().c_str());
        return SUCCESS;
      }
      GELOGI("Dynamic output shape info is %s", ToString(dynamic_output_shape_info).c_str());
      std::vector<std::vector<int64_t>> dynamic_output_shape;
      ParseDynamicOutShape(dynamic_output_shape_info, dynamic_output_shape);
      auto &gear_and_real_out_shape_info = merge_nodes_gear_and_real_out_shape_info_[idx];
      for (auto &it : dynamic_output_shape) {
        const auto gear_index = static_cast<size_t>(it[0U]);
        if (gear_index > all_gears_info_.size()) {
          REPORT_INNER_ERROR("E19999", "gear index:%zu in op:%s(%s) > all_gears_info.size:%zu in model:%u "
                             "check invalid", gear_index, op_desc->GetName().c_str(), op_desc->GetType().c_str(),
                             all_gears_info_.size(), model_id_);
          GELOGE(PARAM_INVALID, "[Check][Param] gear index:%zu in op:%s(%s) > all_gears_info.size:%zu in model:%u.",
                 gear_index, op_desc->GetName().c_str(), op_desc->GetType().c_str(), all_gears_info_.size(), model_id_);
          return PARAM_INVALID;
        }

        if (static_cast<size_t>(it[1U]) == idx) {
          std::vector<int64_t> output_shape;
          for (size_t i = 2U; i < it.size(); ++i) {
            output_shape.emplace_back(it[i]);
          }
          gear_and_real_out_shape_info[all_gears_info_[gear_index]] = output_shape;
          GELOGD("Get real gear index is: %zu, gear info is %s, output shape is %s",
                 gear_index, ToString(all_gears_info_[gear_index]).c_str(), ToString(output_shape).c_str());
        }
      }
    }
    idx++;
  }
  return SUCCESS;
}

void DavinciModel::ParseDynamicOutShape(const std::vector<std::string> &str_info,
                                        std::vector<std::vector<int64_t>> &vec_info) const {
  for (size_t i = 0U; i < str_info.size(); ++i) {
    std::vector<int64_t> shape;
    const auto dims = StringUtils::Split(str_info[i], ',');
    for (const auto &dim : dims) {
      if (dim.empty()) {
        continue;
      }
      shape.emplace_back(std::strtol(dim.c_str(), nullptr, kDecimalRadix));
    }
    GELOGI("Shape from attr is %s", ToString(shape).c_str());
    vec_info.emplace_back(shape);
  }
}

Status DavinciModel::GetLabelGotoAddr(const uint32_t label_index, const rtMemType_t mem_type,
                                      void *&arg_addr, uint32_t &arg_size) {
  const std::lock_guard<std::mutex> lk(label_args_mutex_);
  const std::map<uint32_t, std::pair<void *, uint32_t>>::const_iterator it = label_goto_args_.find(label_index);
  if (it != label_goto_args_.cend()) {
    arg_addr = it->second.first;
    arg_size = it->second.second;
    return SUCCESS;
  }

  if (label_index >= label_list_.size()) {
    REPORT_INNER_ERROR("E19999", "Param label index:%u >= label_list_.size:%zu in model:%u, check invalid",
                       label_index, label_list_.size(), model_id_);
    GELOGE(INTERNAL_ERROR, "[Check][Param] Param label index:%u >= label_list_.size:%zu in model:%u",
           label_index, label_list_.size(), model_id_);
    return INTERNAL_ERROR;
  }
  GE_CHECK_NOTNULL(label_list_[static_cast<size_t>(label_index)]);
  std::vector<rtLabel_t> label_used = { label_list_[static_cast<size_t>(label_index)] };

  arg_size = static_cast<uint32_t>(label_used.size() * sizeof(rtLabelDevInfo));
  GE_CHK_RT_RET(rtMalloc(&arg_addr, static_cast<uint64_t>(arg_size), mem_type));
  label_goto_args_[label_index] = { arg_addr, arg_size };

  GE_CHK_RT_RET(rtLabelListCpy(label_used.data(), static_cast<uint32_t>(label_used.size()), arg_addr, arg_size));

  return SUCCESS;
}

void DavinciModel::SetGlobalStep(const uintptr_t step_addr, const uint64_t step_size) {
  global_step_addr_ = step_addr;
  global_step_size_ = step_size;
}

/// @ingroup ge
/// @brief Get Op rtStream.
/// @param [in] op_desc: Op descriptor.
/// @param [in] stream_id: Logical stream id.
/// @param [out] stream: rt stream.
/// @return Status
Status DavinciModel::GetOpStream(const OpDescPtr &op_desc, const size_t stream_id, rtStream_t &stream) {
  if (stream_list_.size() == 1U) {
    stream = stream_list_[0U];
  } else if (stream_list_.size() > stream_id) {
    stream = stream_list_[stream_id];
  } else {
    REPORT_INNER_ERROR("E19999", "stream_id:%zu in op:%s(%s) >= stream size:%zu in model:%u, check invalid",
        stream_id, op_desc->GetName().c_str(), op_desc->GetType().c_str(), stream_list_.size(), model_id_);
    GELOGE(INTERNAL_ERROR, "[Check][Param] stream_id:%zu in op:%s(%s) >= stream size:%zu in model:%u",
           stream_id, op_desc->GetName().c_str(), op_desc->GetType().c_str(), stream_list_.size(), model_id_);
    return INTERNAL_ERROR;
  }
  return SUCCESS;
}

/// @ingroup ge
/// @brief LabelSet Op Initialize.
/// @param [in] op_desc: LabelSet Op descriptor.
/// @return Status
Status DavinciModel::InitLabelSet(const OpDescPtr &op_desc) {
  uint32_t label_index = 0U;
  if (!AttrUtils::GetInt(op_desc, ATTR_NAME_LABEL_SWITCH_INDEX, label_index)) {
    REPORT_INNER_ERROR("E19999", "Get Attr:%s in op:%s(%s) fail, model_id:%u, check invalid",
                       ATTR_NAME_LABEL_SWITCH_INDEX.c_str(), op_desc->GetName().c_str(), op_desc->GetType().c_str(),
                       model_id_);
    GELOGE(INTERNAL_ERROR, "[Get][Attr] %s in op:%s(%s) fail, model_id:%u",
           ATTR_NAME_LABEL_SWITCH_INDEX.c_str(), op_desc->GetName().c_str(), op_desc->GetType().c_str(), model_id_);
    return INTERNAL_ERROR;
  }
  if (label_index >= runtime_param_.label_num) {
    REPORT_INNER_ERROR("E19999", "label_switch_index:%u in Node:%s >= label_num:%u in model:%u, check invalid",
                       label_index, op_desc->GetName().c_str(), runtime_param_.label_num, model_id_);
    GELOGE(INTERNAL_ERROR, "[Check][Param] label_switch_index:%u in Node:%s >= label_num:%u in model:%u",
           label_index, op_desc->GetName().c_str(), runtime_param_.label_num, model_id_);
    return INTERNAL_ERROR;
  }
  if (label_id_indication_.count(label_index) > 0U) {
    REPORT_INNER_ERROR("E19999", "label_switch_index:%u in op:%s(%s) is already used  in model:%u, check invalid",
                       label_index, op_desc->GetName().c_str(), op_desc->GetType().c_str(), model_id_);
    GELOGE(INTERNAL_ERROR, "[Check][Param] label_switch_index:%u in op:%s(%s) is already used  in model:%u",
           label_index, op_desc->GetName().c_str(), op_desc->GetType().c_str(), model_id_);
    return INTERNAL_ERROR;
  }

  rtStream_t stream = nullptr;
  const size_t stream_id = static_cast<size_t>(op_desc->GetStreamId());
  GE_CHK_STATUS_RET_NOLOG(GetOpStream(op_desc, stream_id, stream));

  rtLabel_t rt_label = nullptr;
  GE_CHK_RT_RET(rtLabelCreateExV2(&rt_label, rt_model_handle_, stream));

  GELOGI("InitLabelSet: label[%u]=%p stream[%zu]=%p", label_index, rt_label, stream_id, stream);
  (void)label_id_indication_.insert(label_index);
  label_list_[static_cast<size_t>(label_index)] = rt_label;
  return SUCCESS;
}

Status DavinciModel::InitVariable(const OpDescPtr &op_desc, std::map<std::string, OpDescPtr> &variable_by_name) {
  if (!known_node_) {
    if (op_desc->GetName() == NODE_NAME_GLOBAL_STEP) {
      const auto output_sizes = ModelUtils::GetOutputSize(op_desc);
      if (!output_sizes.empty()) {
        global_step_size_ = static_cast<uint64_t>(output_sizes[0U]);
      }
      const auto output_addrs = ModelUtils::GetOutputAddrs(runtime_param_, op_desc);
      if (!output_addrs.empty()) {
        global_step_addr_ = PtrToValue(output_addrs[0U]);
      }
    }
  }

  variable_by_name[op_desc->GetName()] = op_desc;
  return SUCCESS;
}

/// @ingroup ge
/// @brief ACL case, Load task list with queue.
/// @param [in] input_queue_ids: input queue ids from user, nums equal Data Op.
/// @param [in] output_queue_ids: input queue ids from user, nums equal NetOutput Op.
/// @return: 0 for success / others for failed
Status DavinciModel::SetQueIds(const std::vector<uint32_t> &input_queue_ids,
                               const std::vector<uint32_t> &output_queue_ids) {
  if (input_queue_ids.empty() && output_queue_ids.empty()) {
    REPORT_INNER_ERROR("E19999", "Param input_queue_ids.size:%zu and output_queue_ids.size:%zu is empty, model_id:%u,"
                       "check invalid", input_queue_ids.size(), output_queue_ids.size(),
                       model_id_);
    GELOGE(ACL_ERROR_GE_EXEC_MODEL_QUEUE_ID_INVALID, "[Check][Param] Param is empty, model_id:%u", model_id_);
    return ACL_ERROR_GE_EXEC_MODEL_QUEUE_ID_INVALID;
  }

  input_queue_ids_ = input_queue_ids;
  output_queue_ids_ = output_queue_ids;
  return SUCCESS;
}

///
/// @ingroup ge
/// @brief ACL case, Load task list with queue.
/// @param [in] input_que_ids: input queue ids from user, nums equal Data Op.
/// @param [in] output_que_ids: input queue ids from user, nums equal NetOutput Op.
/// @return: 0 for success / others for failed
///
Status DavinciModel::LoadWithQueue() {
  if (input_queue_ids_.empty() && output_queue_ids_.empty()) {
    return SUCCESS;
  }

  use_control_input_queue_ = input_data_info_.empty() && (input_queue_ids_.size() == 1U);
  if ((!use_control_input_queue_) && (input_queue_ids_.size() != input_data_info_.size())) {
    REPORT_INNER_ERROR("E19999", "Param input_queue_ids_.size:%zu != input_data_info_.size:%zu, model_id:%u,"
                       "check invalid", input_queue_ids_.size(), input_data_info_.size(),
                       model_id_);
    GELOGE(ACL_ERROR_GE_EXEC_MODEL_QUEUE_ID_INVALID, "[Check][Param] Input queue ids not match model: "
           "input_queue=%zu input_data=%zu, model_id:%u", input_queue_ids_.size(), input_data_info_.size(), model_id_);
    return ACL_ERROR_GE_EXEC_MODEL_QUEUE_ID_INVALID;
  }

  if (output_queue_ids_.size() != output_data_info_.size()) {
    REPORT_INNER_ERROR("E19999", "Param output_queue_ids_.size:%zu != output_data_info_.size:%zu, model_id:%u,"
                       "check invalid", output_queue_ids_.size(), output_data_info_.size(), model_id_);
    GELOGE(ACL_ERROR_GE_EXEC_MODEL_QUEUE_ID_INVALID,
           "[Check][Param] Output queue ids not match model: output_queue=%zu output_data=%zu, model_id:%u",
           output_queue_ids_.size(), output_data_info_.size(), model_id_);
    return ACL_ERROR_GE_EXEC_MODEL_QUEUE_ID_INVALID;
  }

  GE_CHK_STATUS_RET(AddHeadStream(), "[Add][HeadStream] failed, model_id:%u", model_id_);
  if (use_control_input_queue_) {
    GE_CHK_STATUS_RET(BindControlInputQueue(), "[Bind][ControlInputQueue] failed, model_id:%u", model_id_);
  } else {
    // Binding input_queue and Data Op.
    GE_CHK_STATUS_RET(BindInputQueue(), "[Bind][InputQueue] failed, model_id:%u", model_id_);
    ZeroCpyArgs input_args{};
    input_args.cpy_type = ZeroCpyType::kMixedCpy;
    input_args.has_no_tiling = has_no_tiling_input_;
    input_args.need_distribute = true;
    GE_CHK_STATUS_RET(CpuTaskModelZeroCopy(input_mbuf_list_, input_data_info_, input_no_tiling_flag_, input_args),
                      "[Call][CpuTaskModelZeroCopy] failed, model_id:%u", model_id_);
  }

  // Binding output_queue and NetOutput Op.
  GE_CHK_STATUS_RET(BindOutputQueue(), "[Bind][OutputQueue] failed, model_id:%u", model_id_);
  ZeroCpyArgs output_args{};
  output_args.cpy_type = ZeroCpyType::kAllStatic;
  output_args.has_no_tiling = has_no_tiling_output_;
  output_args.need_distribute = true;
  GE_CHK_STATUS_RET(CpuTaskModelZeroCopy(output_mbuf_list_, output_data_info_, output_no_tiling_flag_, output_args),
                    "[Call][CpuTaskModelZeroCopy] failed, model_id:%u", model_id_);

  GE_CHK_STATUS_RET(CpuActiveStream(), "[Call][CpuActiveStream] failed, model_id:%u", model_id_);
  GE_CHK_STATUS_RET(CpuWaitEndGraph(), "[Call][CpuWaitEndGraph] failed, model_id:%u", model_id_);
  GE_CHK_STATUS_RET(CpuPostProcess(), "[Call][CpuPostProcess] failed, model_id:%u", model_id_);
  GE_CHK_STATUS_RET(BindEnqueue(), "[Call][BindEnqueue] failed, model_id:%u", model_id_);
  GE_CHK_STATUS_RET(CpuModelRepeat(), "[Call][CpuModelRepeat] failed, model_id:%u", model_id_);

  return SUCCESS;
}

Status DavinciModel::BindControlInputQueue() {
  const uint32_t queue_id = input_queue_ids_[0U];
  GELOGI("using control input queue, queue_id = %u", queue_id);
  GE_CHK_RT_RET(rtModelBindQueue(rt_model_handle_, queue_id, RT_MODEL_INPUT_QUEUE));
  GE_CHK_STATUS_RET_NOLOG(CpuModelDequeue(queue_id));
  return SUCCESS;
}

/// @ingroup ge
/// @brief queue schedule, Bind  input queue to Data output address.
/// @return: 0 for success / others for failed
Status DavinciModel::BindInputQueue() {
  // Caller checked: input_queue_ids_.size() == input_size_list_.size() != input_addr_list_.size()
  for (size_t i = 0U; i < input_queue_ids_.size(); ++i) {
    const std::map<uint32_t, ZeroCopyOffset>::const_iterator it = input_data_info_.find(i);
    if (it == input_data_info_.cend()) {
      GELOGE(FAILED, "[Check][Param] Input not match: tensor num=%zu, Queue id index=%zu", input_data_info_.size(), i);
      return FAILED;
    }

    const uint32_t queue_id = input_queue_ids_[i];
    if (it->second.GetDataInfo().empty()) {
      GELOGE(INTERNAL_ERROR, "[Check][Param] the %zu input_queue not set data_info.", i);
      return INTERNAL_ERROR;
    }
    const uint32_t data_size = static_cast<uint32_t>(it->second.GetDataInfo().at(0U).first);
    const uintptr_t data_addr = PtrToValue(it->second.GetDataInfo().at(0U).second);
    GELOGI("BindInputToQueue: graph_%u index[%zu] queue id[%u] output addr[0x%lx] output size[%u]",
           runtime_param_.graph_id, i, queue_id, data_addr, data_size);

    GE_CHK_RT_RET(rtModelBindQueue(rt_model_handle_, queue_id, RT_MODEL_INPUT_QUEUE));

    if (CpuModelDequeue(queue_id) != SUCCESS) {
      return INTERNAL_ERROR;
    }
  }

  return SUCCESS;
}

/// @ingroup ge
/// @brief definiteness queue schedule, bind input queue to task.
/// @param [in] queue_id: input queue id from user.
/// @return: 0 for success / others for failed
Status DavinciModel::CpuModelDequeue(const uint32_t queue_id) {
  GELOGI("Set CpuKernel model dequeue task enter.");
  const auto dequeue_task = MakeShared<CpuTaskModelDequeue>(rt_entry_stream_);
  GE_CHECK_NOTNULL(dequeue_task);

  // Get DataOp Output address and bind to queue.
  uintptr_t in_mbuf = 0U;
  const Status status = dequeue_task->Init(queue_id, in_mbuf);
  if (status != SUCCESS) {
    return status;
  }

  cpu_task_list_.push_back(dequeue_task);
  input_mbuf_list_.push_back(in_mbuf);
  GELOGI("Set CpuKernel model dequeue task success.");
  return SUCCESS;
}

Status DavinciModel::CpuTaskModelZeroCopy(std::vector<uintptr_t> &mbuf_list,
                                          const std::map<uint32_t, ZeroCopyOffset> &outside_addrs,
                                          const std::vector<bool> &is_no_tiling_list,
                                          ZeroCpyArgs &cpy_args) {
  GELOGI("Set CpuKernel model zero_copy task enter.");
  const auto zero_copy = MakeShared<CpuTaskZeroCopy>(rt_entry_stream_);
  GE_CHECK_NOTNULL(zero_copy);

  // mdc zero_copy not support l2 fusion
  const Status status = zero_copy->Init(mbuf_list, outside_addrs, is_no_tiling_list, cpy_args);
  if (status != SUCCESS) {
    return status;
  }

  if (cpy_args.need_distribute) {
    cpu_task_list_.push_back(zero_copy);
    GELOGI("Set CpuKernel model zero_copy task success.");
  }
  return SUCCESS;
}

/// @ingroup ge
/// @brief queue schedule, bind output queue to NetOutput input address.
/// @return: 0 for success / others for failed
Status DavinciModel::BindOutputQueue() {
  // Caller checked: input_queue_ids_.size() == input_size_list_.size() != input_addr_list_.size()
  output_mbuf_list_.resize(output_queue_ids_.size());
  for (size_t i = 0U; i < output_queue_ids_.size(); ++i) {
    if (output_no_tiling_flag_[i]) {
      GELOGI("BindOutputQueue: output[%zu] support no tiling", i);
      continue;
    }

    const std::map<uint32_t, ZeroCopyOffset>::const_iterator it = output_data_info_.find(i);
    if (it == output_data_info_.cend()) {
      REPORT_INNER_ERROR("E19999", "Can't find in output_data_info, size:%zu, index:%zu, model_id:%u, check invalid",
                         output_data_info_.size(), i, model_id_);
      GELOGE(FAILED, "[Check][Param] Can't find in output_data_info, size:%zu, Index:%zu, model_id:%u",
             output_data_info_.size(), i, model_id_);
      return FAILED;
    }

    const uint32_t queue_id = output_queue_ids_[i];
    if (it->second.GetDataInfo().empty()) {
      REPORT_INNER_ERROR("E19999", "Index:%zu out_data_info in model:%u is empty, check invalid", i, model_id_);
      GELOGE(INTERNAL_ERROR, "[Check][Param] Index:%zu out_data_info in model:%u is empty, check invalid",
             i, model_id_);
      return INTERNAL_ERROR;
    }
    const uint32_t data_size = static_cast<uint32_t>(it->second.GetDataInfo().at(0U).first);
    const void *const data_ptr = it->second.GetDataInfo().at(0U).second;
    const uintptr_t data_addr = PtrToValue(data_ptr);
    GELOGI("BindOutputToQueue: graph_%u index[%zu] queue id[%u] input addr[0x%lx] input size[%u]",
           runtime_param_.graph_id, i, queue_id, data_addr, data_size);

    if (copy_only_addrs_.count(data_ptr) != 0U) {
      GELOGI("BindOutputQueue: output[%zu] doesn't support zero copy", i);
      continue;
    }

    GE_CHK_RT_RET(rtModelBindQueue(rt_model_handle_, queue_id, RT_MODEL_OUTPUT_QUEUE));

    const Status status = CpuModelPrepareOutput(i, data_addr, data_size);
    if (status != SUCCESS) {
      return status;
    }
  }

  return SUCCESS;
}

/// @ingroup ge
/// @brief definiteness queue schedule, bind output queue to task.
/// @param [in] output_idx: output index.
/// @param [in] addr: NetOutput Op input tensor address.
/// @param [in] size: NetOutput Op input tensor size.
/// @return: 0 for success / others for failed
Status DavinciModel::CpuModelPrepareOutput(const size_t output_idx, const uintptr_t addr, const uint32_t data_size) {
  GELOGI("Set CpuKernel model enqueue task enter.");
  if (input_mbuf_list_.empty()) {
    REPORT_INNER_ERROR("E19999", "input_mbuf_list_ is empty, model_id:%u, check invalid", model_id_);
    GELOGE(FAILED, "[Check][Param] input_mbuf_list_ is empty, model_id:%u", model_id_);
    return FAILED;
  }

  const auto prepare_output = MakeShared<CpuTaskProcessOutput>(rt_entry_stream_, ProcessStage::kPrepare);
  GE_CHECK_NOTNULL(prepare_output);

  uintptr_t out_mbuf = 0U;
  if (prepare_output->Init(addr, data_size, input_mbuf_list_.back(), out_mbuf) != SUCCESS) {
    return FAILED;
  }

  cpu_task_list_.push_back(prepare_output);
  output_mbuf_list_[output_idx] = out_mbuf;
  GELOGI("Set CpuKernel model enqueue task success.");
  return SUCCESS;
}

///
/// @ingroup ge
/// @brief definiteness queue schedule, active original model stream.
/// @return: 0 for success / others for failed
///
Status DavinciModel::CpuActiveStream() {
  GELOGI("Set CpuKernel active stream task enter.");
  const auto active_entry = MakeShared<CpuTaskActiveEntry>(rt_entry_stream_);
  GE_CHECK_NOTNULL(active_entry);

  const Status status = active_entry->Init(rt_head_stream_);
  if (status != SUCCESS) {
    return status;
  }

  cpu_task_list_.push_back(active_entry);
  GELOGI("Set CpuKernel active stream task success.");
  return SUCCESS;
}

/// @ingroup ge
/// @brief definiteness queue schedule, wait for end graph.
/// @return: 0 for success / others for failed
Status DavinciModel::CpuWaitEndGraph() {
  GELOGI("Set CpuKernel wait end graph task enter.");
  const auto wait_endgraph = MakeShared<CpuTaskWaitEndGraph>(rt_entry_stream_);
  GE_CHECK_NOTNULL(wait_endgraph);

  const Status status = wait_endgraph->Init(runtime_model_id_);
  if (status != SUCCESS) {
    return status;
  }

  cpu_task_list_.push_back(wait_endgraph);
  GELOGI("Set CpuKernel wait end graph task success.");
  return SUCCESS;
}

Status DavinciModel::CpuPostProcess() {
  for (size_t i = 0U; i < output_queue_ids_.size(); ++i) {
    const std::map<uint32_t, ZeroCopyOffset>::const_iterator it = output_data_info_.find(i);
    if (it == output_data_info_.cend()) {
      REPORT_INNER_ERROR("E19999", "Index:%zu can't find in output_data_info_ size:%zu in model_id:%u, check invalid",
                         i, output_data_info_.size(), model_id_);
      GELOGE(FAILED, "[Check][Param] Index:%zu can't find in output_data_info_ size:%zu in model_id:%u",
             i, output_data_info_.size(), model_id_);
      return FAILED;
    }

    if (it->second.GetDataInfo().empty()) {
      REPORT_INNER_ERROR("E19999", "Index:%zu out_data_info in model:%u is empty, check invalid", i, model_id_);
      GELOGE(INTERNAL_ERROR, "[Check][Param] Index:%zu out_data_info in model:%u is empty, check invalid",
             i, model_id_);
      return INTERNAL_ERROR;
    }
    const uint32_t data_size = static_cast<uint32_t>(it->second.GetDataInfo().at(0U).first);
    const void *const data_ptr = it->second.GetDataInfo().at(0U).second;
    const uintptr_t data_addr = PtrToValue(data_ptr);
    GELOGI("CpuPostProcess: graph_%u index[%zu] input addr[0x%lx] input size[%u]",
           runtime_param_.graph_id, i, data_addr, data_size);
    ProcessStage stage;
    if (output_no_tiling_flag_[i]) {
      GELOGI("CpuPostProcess: output[%zu] support no tiling", i);
      stage = ProcessStage::kPostDynamic;
    } else if (copy_only_addrs_.count(data_ptr) != 0U) {
      GELOGI("CpuPostProcess: output[%zu] need to non zero copy", i);
      stage = ProcessStage::kPostStatic;
    } else {
      GELOGI("CpuPostProcess: output[%zu] support zero copy, do nothing", i);
      continue;
    }
    GE_CHK_STATUS_RET_NOLOG(CpuModelPostProcess(i, data_addr, data_size, stage));
  }

  return SUCCESS;
}

Status DavinciModel::CpuModelPostProcess(const size_t ouput_idx, const uintptr_t addr,
                                         const uint32_t data_size, const ProcessStage stage) {
  GELOGI("Set CpuKernel model post process task enter.");
  if (input_mbuf_list_.empty()) {
    REPORT_INNER_ERROR("E19999", "input_mbuf_list_ is empty, model_id:%u, check invalid", model_id_);
    GELOGE(FAILED, "[Check][Param] input_mbuf_list_ is empty, model_id:%u", model_id_);
    return FAILED;
  }

  const auto post_process = MakeShared<CpuTaskProcessOutput>(rt_entry_stream_, stage);
  GE_CHECK_NOTNULL(post_process);

  uintptr_t out_mbuf = 0U;
  if (post_process->Init(addr, data_size, input_mbuf_list_.back(), out_mbuf) != SUCCESS) {
    return FAILED;
  }

  cpu_task_list_.push_back(post_process);
  output_mbuf_list_[ouput_idx] = out_mbuf;
  GELOGI("Set CpuKernel model post process task success.");
  return SUCCESS;
}

Status DavinciModel::BindEnqueue() {
  for (size_t i = 0U; i < output_queue_ids_.size(); ++i) {
    const std::map<uint32_t, ZeroCopyOffset>::const_iterator it = output_data_info_.find(i);
    if (it == output_data_info_.cend()) {
      REPORT_INNER_ERROR("E19999", "Index:%zu can't find in output_data_info_ size:%zu in model_id:%u, check invalid",
                         i, output_data_info_.size(), model_id_);
      GELOGE(FAILED, "Index:%zu can't find in output_data_info_ size:%zu in model_id:%u",
             i, output_data_info_.size(), model_id_);
      return FAILED;
    }

    if (CpuModelEnqueue(output_queue_ids_[i], output_mbuf_list_[i]) != SUCCESS) {
      return INTERNAL_ERROR;
    }
  }
  return SUCCESS;
}

Status DavinciModel::CpuModelEnqueue(const uint32_t queue_id, const uintptr_t out_mbuf) {
  GELOGI("Set CpuKernel model enqueue task enter.");
  const auto model_enqueue = MakeShared<CpuTaskModelEnqueue>(rt_entry_stream_);
  GE_CHECK_NOTNULL(model_enqueue);

  const Status status = model_enqueue->Init(queue_id, out_mbuf);
  if (status != SUCCESS) {
    return status;
  }
  cpu_task_list_.push_back(model_enqueue);
  GELOGI("Set CpuKernel model enqueue task enter.");
  return SUCCESS;
}

/// @ingroup ge
/// @brief definiteness queue schedule, repeat run model.
/// @return: 0 for success / others for failed
Status DavinciModel::CpuModelRepeat() {
  GELOGI("Set CpuKernel repeat task enter.");
  const auto model_repeat = MakeShared<CpuTaskModelRepeat>(rt_entry_stream_);
  GE_CHECK_NOTNULL(model_repeat);

  const Status status = model_repeat->Init(runtime_model_id_);
  if (status != SUCCESS) {
    return status;
  }

  cpu_task_list_.push_back(model_repeat);
  GELOGI("Set CpuKernel repeat task success.");
  return SUCCESS;
}

Status DavinciModel::GetInputOutputDescInfo(std::vector<InputOutputDescInfo> &input_desc,
                                            std::vector<InputOutputDescInfo> &output_desc) const {
  if (input_addrs_list_.empty() || (input_addrs_list_[0U].size() != 1U)) {
    GELOGI("data_op_list_ is empty or input_desc size is not 1.");
  } else {
    std::vector<uint32_t> input_formats;
    GE_CHK_STATUS_RET(GetInputDescInfo(input_desc, input_formats, false),
                      "[Get][InputDescInfo] failed, model_id:%u", model_id_);
  }

  std::vector<uint32_t> output_formats;
  GE_CHK_STATUS_RET(GetOutputDescInfo(output_desc, output_formats),
                    "[Get][OutputDescInfo] failed, model_id:%u", model_id_);
  return SUCCESS;
}

Status DavinciModel::GetInputOutputDescInfo(std::vector<InputOutputDescInfo> &input_desc,
                                            std::vector<InputOutputDescInfo> &output_desc,
                                            std::vector<uint32_t> &input_formats,
                                            std::vector<uint32_t> &output_formats, const bool by_dims) const {
  if (input_addrs_list_.empty() || (input_addrs_list_[0U].size() != 1U)) {
    GELOGI("data_op_list_ is empty or input_desc size is not 1.");
  } else {
    GE_CHK_STATUS_RET(GetInputDescInfo(input_desc, input_formats, by_dims),
                      "[Get][InputDescInfo] failed, model_id:%u", model_id_);
  }
  GE_CHK_STATUS_RET(GetOutputDescInfo(output_desc, output_formats),
                    "[Get][OutputDescInfo] failed, model_id:%u", model_id_);
  return SUCCESS;
}

///
/// @ingroup ge
/// @brief Get dynamic batch_info
/// @param [out] batch_info
/// @param [out] dynamic_type
/// @return execute result
///
Status DavinciModel::GetDynamicBatchInfo(std::vector<std::vector<int64_t>> &batch_info, int32_t &dynamic_type) const {
  dynamic_type = dynamic_type_;
  batch_info = batch_info_;

  return SUCCESS;
}

///
/// @ingroup ge
/// @brief Get combined dynamic dims info
/// @param [out] batch_info
/// @return None
///
void DavinciModel::GetCombinedDynamicDims(std::vector<std::vector<int64_t>> &batch_info) const {
  batch_info = combined_batch_info_;
}

///
/// @ingroup ge
/// @brief Get user designate shape order
/// @param [out] user_input_shape_order
/// @return None
///
void DavinciModel::GetUserDesignateShapeOrder(std::vector<std::string> &user_input_shape_order) const {
  user_input_shape_order = user_designate_shape_order_;
}

///
/// @ingroup ge
/// @brief Get AIPP input info
/// @param [in] index
/// @param [int] OpDescPtr
/// @return execute result
///
Status DavinciModel::InitAippInfo(const uint32_t index, const OpDescPtr &op_desc) {
  if (!op_desc->HasAttr(ATTR_NAME_AIPP)) {
    GELOGW("There is not AIPP related with index %u", index);
    return SUCCESS;
  }

  domi::AippOpParams aipp_params;
  NamedAttrs aipp_attr;
  GE_CHK_BOOL_RET_STATUS(AttrUtils::GetNamedAttrs(op_desc, ATTR_NAME_AIPP, aipp_attr), ACL_ERROR_GE_AIPP_NOT_EXIST,
                         "[Get][NamedAttrs] Data node:%s do not contain param aipp!", op_desc->GetName().c_str());
  GE_CHK_STATUS_RET(OpUtils::ConvertAippParams(aipp_attr, aipp_params),
                    "[Convert][AippParams] get aipp params failed, op:%s", op_desc->GetName().c_str());
  GELOGI("Node data: %s, type: %s, current index: %u, current node related input rank: %u",
         op_desc->GetName().c_str(), op_desc->GetType().c_str(), index, aipp_params.related_input_rank());

  AippConfigInfo aipp_info;
  GE_CHK_STATUS_RET(AippUtils::ConvertAippParams2AippInfo(aipp_params, aipp_info),
                    "[Call][ConvertAippParams2AippInfo] failed, op:%s", op_desc->GetName().c_str());

  aipp_info_list_[index] = aipp_info;
  return SUCCESS;
}

///
/// @ingroup ge
/// @brief Get AIPP input info
/// @param [in] index
/// @param [out] aipp_info
/// @return execute result
///
Status DavinciModel::GetAippInfo(const uint32_t index, AippConfigInfo &aipp_info) const {
  const auto it = aipp_info_list_.find(index);
  if (it == aipp_info_list_.end()) {
    GELOGW("there is not AIPP related with index %u", index);
    return ACL_ERROR_GE_AIPP_NOT_EXIST;
  }

  aipp_info = it->second;
  return SUCCESS;
}

Status DavinciModel::InitAippType(const uint32_t index, const OpDescPtr &op_desc,
                                  const std::map<uint32_t, OpDescPtr> &data_list) {
  if (!op_desc->HasAttr(ATTR_DATA_RELATED_AIPP_MODE)) {
    GELOGW("There is no aipp releated info with index %u", index);
    return SUCCESS;
  }

  // Set default value
  InputAippType aipp_type = InputAippType::DATA_WITHOUT_AIPP;
  std::string data_mode;
  (void)AttrUtils::GetStr(op_desc, ATTR_DATA_RELATED_AIPP_MODE, data_mode);
  if (data_mode == "static_aipp") {
    aipp_type = InputAippType::DATA_WITH_STATIC_AIPP;
  } else if (data_mode == "dynamic_aipp") {
    aipp_type = InputAippType::DATA_WITH_DYNAMIC_AIPP;
  } else if (data_mode == "dynamic_aipp_conf") {
    aipp_type = InputAippType::DYNAMIC_AIPP_NODE;
  } else {
    REPORT_INNER_ERROR("E19999", "Attr:%s data_mode:%s in op:%s(%s), model_id:%u, check invalid",
                       ATTR_DATA_RELATED_AIPP_MODE.c_str(), data_mode.c_str(),
                       op_desc->GetName().c_str(), op_desc->GetType().c_str(), model_id_);
    GELOGE(ACL_ERROR_GE_AIPP_MODE_INVALID, "[Get][Attr] %s data_mode:%s in op:%s(%s), model_id:%u, check invalid",
           ATTR_DATA_RELATED_AIPP_MODE.c_str(), data_mode.c_str(),
           op_desc->GetName().c_str(), op_desc->GetType().c_str(), model_id_);
    return ACL_ERROR_GE_AIPP_MODE_INVALID;
  }

  size_t aipp_index = 0xFFFFFFFFU;  // default invalid value
  if (aipp_type == InputAippType::DATA_WITH_DYNAMIC_AIPP) {
    std::string releated_name;
    (void)AttrUtils::GetStr(op_desc, ATTR_DATA_AIPP_DATA_NAME_MAP, releated_name);
    for (const auto &item : data_list) {
      if (item.second->GetName() == releated_name) {
        GELOGI("Find aipp_data [%s] index %u from index %u", releated_name.c_str(), item.first, index);
        aipp_index = item.first;
      }
    }

    if (aipp_index == 0xFFFFFFFFU) {
      GELOGW("Can not find aipp data node from index %u", index);
      return SUCCESS;
    }
  }

  aipp_type_list_[index] = { aipp_type, aipp_index };
  return SUCCESS;
}

Status DavinciModel::GetAippType(const uint32_t index, InputAippType &aipp_type, size_t &aipp_index) const {
  GE_CHK_BOOL_RET_STATUS(index < input_addrs_list_.size(), PARAM_INVALID,
                         "[Check][Param] Index %u is invalid", index);
  const auto it = aipp_type_list_.find(index);
  if (it == aipp_type_list_.end()) {
    GELOGW("There is no aipp releated info with index %u", index);
    aipp_type = InputAippType::DATA_WITHOUT_AIPP;
    aipp_index = 0xFFFFFFFFU;
    return SUCCESS;
  }

  aipp_type = it->second.first;
  aipp_index = it->second.second;
  return SUCCESS;
}

void DavinciModel::SetDynamicSize(const std::vector<uint64_t> &batch_num, const int32_t dynamic_type) {
  batch_size_.clear();
  if (batch_num.empty()) {
    GELOGD("User has not set dynammic data");
  }
  for (size_t i = 0U; i < batch_num.size(); ++i) {
    batch_size_.emplace_back(batch_num[i]);
  }

  dynamic_type_ = dynamic_type;
}

void DavinciModel::GetCurrentShape(std::vector<int64_t> &batch_info, int32_t &dynamic_type) const {
  if (batch_size_.empty()) {
    GELOGD("User does not set dynamic size");
  }
  for (size_t i = 0U; i < batch_size_.size(); ++i) {
    GELOGI("Start to get current shape");
    batch_info.emplace_back(batch_size_[i]);
  }

  dynamic_type = dynamic_type_;
}

Status DavinciModel::GetNodeAttr(const std::string &op_name, const std::string &attr_name,
                                 std::string &attr_info) const {
  const auto itr = op_name_to_attrs_.find(op_name);
  if (itr == op_name_to_attrs_.end()) {
    GELOGW("Did not save op:%s attr", op_name.c_str());
    return SUCCESS;
  }
  const auto attr_itr = itr->second.find(attr_name);
  if (attr_itr == itr->second.end()) {
    GELOGW("Did not save attr:%s of op:%s", attr_name.c_str(), op_name.c_str());
    return SUCCESS;
  }
  for (const auto &attr : attr_itr->second) {
    attr_info += "[" + std::to_string(attr.size()) + "]" + attr;
  }
  GELOGD("Get attr:%s of op:%s success, attr value:%s", attr_name.c_str(), op_name.c_str(), attr_info.c_str());
  return SUCCESS;
}

void DavinciModel::GetOutputShapeInfo(std::vector<std::string> &out_shape_info) const {
  (void)out_shape_info.insert(out_shape_info.cend(),
                              dynamic_output_shape_info_.cbegin(),
                              dynamic_output_shape_info_.cend());
}

void DavinciModel::SetInputDimsInfo(const std::vector<int64_t> &input_dims, const Format format,
                                    ShapeDescription &shape_info) const {
  const size_t n = static_cast<size_t>((format == FORMAT_NHWC) ? NHWC_DIM_N : NCHW_DIM_N);
  const size_t c = static_cast<size_t>((format == FORMAT_NHWC) ? NHWC_DIM_C : NCHW_DIM_C);
  const size_t h = static_cast<size_t>((format == FORMAT_NHWC) ? NHWC_DIM_H : NCHW_DIM_H);
  const size_t w = static_cast<size_t>((format == FORMAT_NHWC) ? NHWC_DIM_W : NCHW_DIM_W);

  if (input_dims.size() == static_cast<size_t>(NORMAL_TENSOR_SIZE)) {
    shape_info.num = input_dims[n];
    shape_info.height = input_dims[h];
    shape_info.width = input_dims[w];
    shape_info.channel = input_dims[c];
  }
  for (size_t k = 0U; k < input_dims.size(); ++k) {
    shape_info.dims.push_back(input_dims[k]);
  }
}

void DavinciModel::CreateInputDimsInfo(const OpDescPtr &op_desc, const Format format,
                                       ShapeDescription &shape_info, ShapeDescription &dims_info) const {
  // judge if this data is linked dynamic aipp first, multiply batch has been considered
  if (op_desc->HasAttr(ATTR_DYNAMIC_AIPP_INPUT_DIMS)) {
    std::vector<int64_t> dynamic_aipp_input_dims;
    (void)AttrUtils::GetListInt(op_desc, ATTR_DYNAMIC_AIPP_INPUT_DIMS, dynamic_aipp_input_dims);
    SetInputDimsInfo(dynamic_aipp_input_dims, format, shape_info);
  } else {
    // judge if this data is multiply batch
    if (!op_desc->HasAttr(ATTR_MBATCH_ORIGIN_INPUT_DIMS)) {
      const std::vector<int64_t> input_dims = op_desc->GetInputDescPtr(0U)->GetShape().GetDims();
      SetInputDimsInfo(input_dims, format, shape_info);
    } else {
      std::vector<int64_t> origin_input_dims;
      (void)AttrUtils::GetListInt(op_desc, ATTR_MBATCH_ORIGIN_INPUT_DIMS, origin_input_dims);
      SetInputDimsInfo(origin_input_dims, format, shape_info);
    }
  }

  if (op_desc->HasAttr(ATTR_NAME_INPUT_DIMS)) {
    // When static aipp is set, need to get the model input dims which processed by aipp
    std::vector<int64_t> model_input_dims;
    (void)AttrUtils::GetListInt(op_desc, ATTR_NAME_INPUT_DIMS, model_input_dims);
    SetInputDimsInfo(model_input_dims, format, dims_info);
  } else {
    dims_info = shape_info;
  }
}

Status DavinciModel::InitInputDescInfo(const OpDescPtr &op_desc) {
  GE_CHECK_NOTNULL(op_desc->GetInputDescPtr(0U));

  InputOutputDescInfo input;
  ShapeDescription dims_info;
  const Format format = op_desc->GetInputDescPtr(0U)->GetFormat();
  CreateInputDimsInfo(op_desc, format, input.shape_info, dims_info);

  input.data_type = op_desc->GetInputDescPtr(0U)->GetDataType();
  input.name = op_desc->GetName();
  int64_t input_size = 0;
  if (AttrUtils::GetInt(*op_desc->GetOutputDescPtr(0U), ATTR_NAME_SPECIAL_INPUT_SIZE, input_size) &&
      (input_size > 0)) {
    GELOGI("data[%s] output has special size [%ld]", op_desc->GetName().c_str(), input_size);
  } else {
    GE_CHK_STATUS_RET(TensorUtils::GetSize(*op_desc->GetInputDescPtr(0U), input_size),
                      "[Get][InputSize] failed in op:%s.", op_desc->GetName().c_str());
  }
  input.size = static_cast<uint64_t>(input_size);
  input_formats_.push_back(format);
  input_descs_.push_back(input);

  input.shape_info = dims_info;
  input_descs_dims_.push_back(input);
  return SUCCESS;
}

Status DavinciModel::GetInputDescInfo(std::vector<InputOutputDescInfo> &input_desc,
                                      std::vector<uint32_t> &input_format, const bool by_dims) const {
  const std::vector<InputOutputDescInfo> &input_desc_info = by_dims ? input_descs_dims_ : input_descs_;
  (void)input_desc.insert(input_desc.cend(), input_desc_info.cbegin(), input_desc_info.cend());
  (void)input_format.insert(input_format.cend(), input_formats_.cbegin(), input_formats_.cend());

  return SUCCESS;
}

void DavinciModel::CreateOutput(const size_t index, const OpDescPtr &op_desc, InputOutputDescInfo &output,
                                uint32_t &format_result) const {
  /// netoutput input tensor desc
  const auto input_desc = op_desc->GetInputDescPtr(static_cast<uint32_t>(index));
  GE_IF_BOOL_EXEC(input_desc == nullptr,
      REPORT_INNER_ERROR("E19999", "input_desc index:%zu in op:%s(%s) not exist, model_id:%u, check invalid",
                         index, op_desc->GetName().c_str(), op_desc->GetType().c_str(), model_id_);
      GELOGE(FAILED, "[Get][InputDescPtr] input_desc index:%zu in op:%s(%s) not exist, model_id:%u",
             index, op_desc->GetName().c_str(), op_desc->GetType().c_str(), model_id_);
      return);
  const auto format = input_desc->GetFormat();
  const auto shape = input_desc->GetShape();
  const auto data_type = input_desc->GetDataType();

  int64_t dims[] = {1, 1, 1, 1};
  format_result = format;
  if (format == FORMAT_ND) {  // for ND tensor
    for (size_t i = 0U; (i < shape.GetDimNum()) && (i < (sizeof(dims) / sizeof(dims[0]))); ++i) {
      dims[i] = shape.GetDim(i);
    }
  } else {                                                                    // FOR FORMAT_NHWC or FORMAT_NCHW
    dims[0] = shape.GetDim(static_cast<size_t>((format == FORMAT_NHWC) ? NHWC_DIM_N : NCHW_DIM_N));  // 0: first dim
    dims[1] = shape.GetDim(static_cast<size_t>((format == FORMAT_NHWC) ? NHWC_DIM_C : NCHW_DIM_C));  // 1: second dim
    dims[2] = shape.GetDim(static_cast<size_t>((format == FORMAT_NHWC) ? NHWC_DIM_H : NCHW_DIM_H));  // 2: third dim
    dims[3] = shape.GetDim(static_cast<size_t>((format == FORMAT_NHWC) ? NHWC_DIM_W : NCHW_DIM_W));  // 3: forth dim
  }
  output.shape_info.num = dims[0];      // 0: first dim
  output.shape_info.channel = dims[1];  // 1: second dim
  output.shape_info.height = dims[2];   // 2: third dim
  output.shape_info.width = dims[3];    // 3: forth dim

  if (input_desc->GetFormat() == FORMAT_FRACTAL_Z) {  // FraczToHWCK
    const int64_t k = shape.GetDim(0U);      // 0: first dim
    const int64_t c = shape.GetDim(1U);      // 1: second dim
    const int64_t h = shape.GetDim(2U);      // 2: third dim
    const int64_t w = shape.GetDim(3U);      // 3: forth dim
    output.shape_info.dims.push_back(h);
    output.shape_info.dims.push_back(w);
    output.shape_info.dims.push_back(c);
    output.shape_info.dims.push_back(k);
    format_result = FORMAT_HWCN;
  } else {
    for (size_t j = 0U; j < shape.GetDimNum(); ++j) {
      output.shape_info.dims.push_back(shape.GetDim(j));
    }
  }

  int64_t tensor_size = 0;
  if (AttrUtils::GetInt(input_desc, ATTR_NAME_SPECIAL_OUTPUT_SIZE, tensor_size) && (tensor_size > 0)) {
    GELOGI("netoutput[%s] [%zu]th input has special size [%ld]", op_desc->GetName().c_str(), index, tensor_size);
  } else {
    (void)TensorUtils::CalcTensorMemSize(shape, format, data_type, tensor_size);  // no need to check value
  }
  output.size = static_cast<uint64_t>(tensor_size);
  output.data_type = static_cast<uint32_t>(input_desc->GetDataType());
}

Status DavinciModel::InitOutputDescInfo(const OpDescPtr &op_desc, const std::vector<std::string> &out_node_name) {
  const size_t out_size = op_desc->GetInputsSize();
  for (size_t i = 0U; i < out_size; ++i) {
    std::string output_name;
    InputOutputDescInfo output;
    uint32_t format_result;
    CreateOutput(i, op_desc, output, format_result);

    const auto src_name = op_desc->GetSrcName();
    const auto src_index = op_desc->GetSrcIndex();
    GE_CHK_BOOL_RET_STATUS((src_name.size() > i) && (src_index.size() > i), INTERNAL_ERROR,
                           "[Check][Param] construct output failed, as index:%zu >= src name size:%zu, "
                           "or index >= src index size:%zu, op:%s.",
                           i, src_name.size(), src_index.size(), op_desc->GetName().c_str());
    // forward compatbility, if old om has no out_node_name, need to return output follow origin way
    if (out_size == out_node_name.size()) {
      // neweast plan, the index will add to name during generate model.
      const bool contains_colon = out_node_name[i].find(":") != std::string::npos;
      output_name = contains_colon ? out_node_name[i] : out_node_name[i] + ":" + std::to_string(src_index[i]);
    } else {
      output_name = std::string("output_") + std::to_string(i) + "_" + src_name[i] + "_" + std::to_string(src_index[i]);
    }
    output.name = output_name;
    output_descs_.push_back(output);
    output_formats_.push_back(format_result);
  }

  return SUCCESS;
}

Status DavinciModel::GetOutputDescInfo(std::vector<InputOutputDescInfo> &output_desc,
                                       std::vector<uint32_t> &output_format) const {
  (void)output_desc.insert(output_desc.cend(), output_descs_.cbegin(), output_descs_.cend());
  (void)output_format.insert(output_format.cend(), output_formats_.cbegin(), output_formats_.cend());
  return SUCCESS;
}

static Status CopyInputForNoTiling(const InputData &input_data, const size_t data_idx, void *&mem_addr) {
  RuntimeTensorDesc tensor_desc;
  // copy data_addr from tensor_desc addr
  GE_CHK_RT_RET(rtMemcpy(&tensor_desc, sizeof(RuntimeTensorDesc), mem_addr, sizeof(RuntimeTensorDesc),
      RT_MEMCPY_DEVICE_TO_HOST));
  if (data_idx >= input_data.shapes.size()) {
    GELOGE(PARAM_INVALID, "invalid index[%zu], input shape size[%zu]", data_idx, input_data.shapes.size());
    return PARAM_INVALID;
  }
  const auto &shape = input_data.shapes[data_idx];
  if (shape.size() > static_cast<size_t>(kMaxDimSize)) {
    GELOGE(PARAM_INVALID, "invalid InputData, input shape[%zu]'s dim size[%zu] > kMaxDimSize[%ld]",
           data_idx, shape.size(), kMaxDimSize);
    return PARAM_INVALID;
  }

  tensor_desc.shape[0] = static_cast<int64_t>(shape.size());
  for (size_t i = 0U; i < shape.size(); i++) {
    tensor_desc.shape[i + 1U] = shape[i];
  }
  // fill actual shape and copy to tensor_desc addr
  GE_CHK_RT_RET(rtMemcpy(mem_addr, sizeof(RuntimeTensorDesc), &tensor_desc, sizeof(RuntimeTensorDesc),
      RT_MEMCPY_HOST_TO_DEVICE));
  mem_addr = ValueToPtr(tensor_desc.data_addr);
  GELOGD("copy tensor desc for no tiling, data_addr:%p, dim:%ld", mem_addr, tensor_desc.shape[0]);
  return SUCCESS;
}

Status DavinciModel::CopyInputData(const InputData &input_data) {
  const std::vector<DataBuffer> &blobs = input_data.blobs;
  for (const auto &data_info : input_data_info_) {
    const size_t data_idx = data_info.first;
    if (data_idx >= blobs.size()) {
      REPORT_INNER_ERROR("E19999", "index:%u in input_data_info_ >= input_data.blobs.size:%zu, model_id:%u, "
                         "check invalid", data_info.first, blobs.size(), model_id_);
      GELOGE(FAILED, "[Check][Param] Blobs not match: blobs=%zu, tensor=%zu, index=%u, size=%ld, op_name(%s)",
             blobs.size(), input_data_info_.size(), data_info.first, data_info.second.GetDataInfo().at(0U).first,
             data_info.second.GetOpName().c_str());
      return FAILED;
    }

    const DataBuffer &data_buf = blobs.at(data_idx);
    if (data_buf.length == 0U) {
      GELOGW("No data need to memcpy!");
      continue;
    }

    const uint64_t data_size = static_cast<uint64_t>(data_info.second.GetDataSize());
    GE_CHK_BOOL_RET_STATUS(data_size >= data_buf.length, PARAM_INVALID,
                           "[Check][Param] input data size(%lu) does not match model required size(%lu), op_name(%s)",
                           data_buf.length, data_size, data_info.second.GetOpName().c_str());
    void *mem_addr = data_info.second.GetBasicAddr();
    bool is_no_tiling = false;
    if (data_idx < input_no_tiling_flag_.size()) {
      is_no_tiling = input_no_tiling_flag_[data_idx];
    } else {
      GELOGW("[Check][Param]invalid input_no_tiling_flag_ size[%zu], index[%u]",
             input_no_tiling_flag_.size(), data_info.first);
    }
    if (is_no_tiling) {
      // mem_addr will be changed to data addr here
      GE_CHK_STATUS_RET_NOLOG(CopyInputForNoTiling(input_data, data_idx, mem_addr));
    }

    GELOGI("CopyPlainData memcpy graph_%u type[F] input[%s] rank[%u] dst[%p] src[%p] mem_size[%lu] datasize[%lu]",
           runtime_param_.graph_id, data_info.second.GetOpName().c_str(), data_info.first, mem_addr, data_buf.data,
           data_size, data_buf.length);
    GE_CHK_RT_RET(rtMemcpy(mem_addr, data_size, data_buf.data, data_buf.length, RT_MEMCPY_HOST_TO_DEVICE));
  }

  return SUCCESS;
}

Status DavinciModel::HandleInputData(InputData &input_data) {
  GE_TIMESTAMP_START(Model_SyncVarData);
  if (SyncVarData() != SUCCESS) {
    return FAILED;
  }
  if (is_first_execute_) {
    GE_TIMESTAMP_EVENT_END(Model_SyncVarData, "Model Run SyncVarData");
  }

  GELOGI("Copy input data, model id:%u", model_id_);
  const bool dynamic_shape_data = is_online_infer_dynamic_ && (!is_getnext_sink_dynamic_);
  if (dynamic_shape_data) {
    cur_dynamic_dims_.clear();
    if (GetCurDynamicDims(input_data.shapes, cur_dynamic_dims_) != SUCCESS) {
      return INTERNAL_ERROR;
    }

    DataBuffer data;
    data.data = cur_dynamic_dims_.data();
    data.length = static_cast<uint32_t>(cur_dynamic_dims_.size() * sizeof(int32_t));
    input_data.blobs.push_back(data);
  }

  const auto status = CopyInputData(input_data);
  if (dynamic_shape_data) {
    input_data.blobs.pop_back();
  }

  return status;
}

Status DavinciModel::SyncVarData() {
  GELOGI("Sync var data, model id:%u", model_id_);
  if ((global_step_addr_ != 0U) && (global_step_size_ != 0U)) {
    GE_CHK_RT_RET(rtMemcpy(ValueToPtr(global_step_addr_), global_step_size_, &iterator_count_, sizeof(uint64_t),
                           RT_MEMCPY_HOST_TO_DEVICE));
  }
  return SUCCESS;
}

Status DavinciModel::InitFusionProfiling(const FusionOpInfo &fusion_op_info) {
  const auto &op_desc = GetOpByIndex(fusion_op_info.op_index);
  if (op_desc == nullptr) {
    REPORT_INNER_ERROR("E19999", "Get op by index failed, as index:%u out of range", fusion_op_info.op_index);
    GELOGE(FAILED, "[Get][Op] failed, as index:%u out of range", fusion_op_info.op_index);
    return FAILED;
  }

  ProfileInfo profile;
  profile.fusion_info = fusion_op_info;
  // save fusion op info into MsprofGeProfFusionData list
  ProfFusionOpInfo(op_desc, fusion_op_info, profile);
  profile_list_.emplace_back(profile);
  GELOGD("Add fusion task, profile info size: %zu", profile_list_.size());
  return SUCCESS;
}

Status DavinciModel::ReportProfilingData(const uint32_t graph_id) {
  auto &prof_mgr = ProfilingManager::Instance();

  // davinci model report only one time during the training
  if (prof_mgr.IsGraphProfReported(graph_id)) {
    GELOGD("[Profiling] graph id %u has been reported.", graph_id);
    return SUCCESS;
  }
  // Report profiling data
  const auto ret = ReportProfilingData();
  // graph id is UINT32_MAX on execution
  if ((ret == SUCCESS) && (graph_id != UINT32_MAX)) {
    prof_mgr.InsertReportedGraphId(graph_id);
  }
  return ret;
}

bool DavinciModel::HasZeroCopyAddr(const OpDescPtr &op_desc) const {
  const std::vector<void *> input_addrs = ModelUtils::GetInputAddrs(runtime_param_, op_desc);
  const std::vector<void *> output_addrs = ModelUtils::GetOutputAddrs(runtime_param_, op_desc);
  std::vector<void *> io_addrs;
  (void)io_addrs.insert(io_addrs.end(), input_addrs.begin(), input_addrs.end());
  (void)io_addrs.insert(io_addrs.end(), output_addrs.begin(), output_addrs.end());

  const auto zero_copy_args_index = GetZeroCopyArgsIndex(io_addrs);
  return !zero_copy_args_index.empty();
}

Status DavinciModel::ReportProfilingData() {
  auto &prof_mgr = ProfilingManager::Instance();
  MsprofGeProfModelLoadData model_load_info{};
  model_load_info.modelId = model_id_;
  const std::string model_name = om_name_.empty() ? name_ : om_name_;
  prof_mgr.SetAlternativeValue(MSPROF_MIX_DATA_STRING_LEN, model_name, model_load_info.modelName);
  model_load_info.startTime = load_begin_time_;
  model_load_info.endTime = load_end_time_;
  const std::string tag_name("model_load_info_" + std::to_string(model_id_));
  GELOGD("Reeport model_load_info, model id is %u", model_id_);
  prof_mgr.ReportData(static_cast<int32_t>(device_id_), &model_load_info, sizeof(model_load_info), tag_name);
  // report fusion op info
  GE_CHK_STATUS_RET(ReportFusionOpInfo(), "Report profiling fusion op info failed");
  // report mapping data between model id and associated graph id
  GE_CHK_STATUS_RET(ReportModelExtInfo(), "Report profiling model ext info failed");

  prof_mgr.ReportProfilingData(model_id_, task_desc_info_);
  return SUCCESS;
}

void DavinciModel::ProfFusionOpInfo(const OpDescPtr &op_desc, const FusionOpInfo &fusion_op_info,
                                    ProfileInfo &profile) const {
  const size_t op_num = profile.fusion_info.original_op_names.size();
  // init memory info
  uint64_t input_mem_size = 0UL;
  uint64_t output_mem_size = 0UL;
  uint64_t workspace_mem_size = 0UL;
  uint64_t weight_mem_size = 0UL;
  InitMemoryInfo(op_desc, input_mem_size, output_mem_size, workspace_mem_size, weight_mem_size);

  // lambda for base fusion data building
  const auto BuildProfFusionInfoBase = [op_num, input_mem_size, output_mem_size, workspace_mem_size,
                                        weight_mem_size](MsprofGeProfFusionData &prof_fusion_data) {
    prof_fusion_data.fusionOpNum = op_num;
    prof_fusion_data.inputMemSize = input_mem_size;
    prof_fusion_data.outputMemSize = output_mem_size;
    prof_fusion_data.workspaceMemSize = workspace_mem_size;
    prof_fusion_data.weightMemSize = weight_mem_size;
    prof_fusion_data.totalMemSize = weight_mem_size + input_mem_size + output_mem_size + workspace_mem_size;
  };

  auto &prof_mgr = ProfilingManager::Instance();
  // one MsprofGeProfFusionData can only contain 8 ops
  const size_t slice_index = op_num / kMsProfFusionOpNum;
  for (size_t k = 0UL; k < slice_index; ++k) {
    MsprofGeProfFusionData prof_fusion_data{};
    prof_fusion_data.modelId = model_id_;
    prof_mgr.SetAlternativeValue(MSPROF_MIX_DATA_STRING_LEN, fusion_op_info.op_name, prof_fusion_data.fusionName);

    // use lambda to build base info of fusion op
    BuildProfFusionInfoBase(prof_fusion_data);
    for (size_t j = 0UL; j < kMsProfFusionOpNum; ++j) {
      const auto origin_op_index = (k * kMsProfFusionOpNum) + j;
      FillProfFusionOp(profile, origin_op_index, j, prof_fusion_data);
    }
    profile.prof_fusion_data_lst.emplace_back(prof_fusion_data);
  }

  const size_t remain_index = op_num % kMsProfFusionOpNum;
  if (remain_index == 0UL) {
    return;
  }
  MsprofGeProfFusionData prof_fusion_data{};
  prof_fusion_data.modelId = model_id_;
  prof_mgr.SetAlternativeValue(MSPROF_MIX_DATA_STRING_LEN, fusion_op_info.op_name, prof_fusion_data.fusionName);
  BuildProfFusionInfoBase(prof_fusion_data);
  for (size_t k = 0UL; k < remain_index; ++k) {
    const auto origin_op_index = (slice_index * kMsProfFusionOpNum) + k;
    FillProfFusionOp(profile, origin_op_index, k, prof_fusion_data);
  }
  profile.prof_fusion_data_lst.emplace_back(prof_fusion_data);
}

Status DavinciModel::ReportFusionOpInfo() {
  GELOGD("Report profiling fusion_op_info, model id is %u", model_id_);
  auto &prof_mgr = ProfilingManager::Instance();
  const std::string tag_name("fusion_op_info_" + std::to_string(model_id_));
  for (auto &profile : profile_list_) {
    for (auto &prof_fusion_data : profile.prof_fusion_data_lst) {
      prof_mgr.ReportData(static_cast<int32_t>(device_id_), &prof_fusion_data, sizeof(prof_fusion_data), tag_name);
    }
  }
  return SUCCESS;
}

Status DavinciModel::ReportModelExtInfo() {
  // if it is not online model, there is no graph id
  if (!domi::GetContext().is_online_model) {
    return SUCCESS;
  }

  GELOGD("Report profiling id map info.");
  const mmTimespec tick_count = mmGetTickCount();
  const int64_t start_time = (tick_count.tv_sec * kTimeNanoRadix) + tick_count.tv_nsec;

  auto &prof_mgr = ProfilingManager::Instance();
  // the mapping between the model id and graph id is displayed in MsprofGeProfIdMapData
  MsprofGeProfIdMapData model_info_ext{};
  model_info_ext.graphId = known_node_ ? runtime_param_.root_graph_id : runtime_param_.graph_id;
  model_info_ext.sessionId = static_cast<uint32_t>(session_id_);
  model_info_ext.modelId = model_id_;
  model_info_ext.timeStamp = static_cast<uint64_t>(start_time);
  const std::string tag_name = "id_map_info";
  prof_mgr.ReportData(static_cast<int32_t>(device_id_), &model_info_ext, sizeof(model_info_ext), tag_name);
  return SUCCESS;
}

void DavinciModel::SinkTimeProfile(const uint32_t data_index, const uint64_t request_id) {
  auto &prof_mgr = ProfilingManager::Instance();
  const std::string model_name = om_name_.empty() ? name_ : om_name_;
  MsprofGeProfInferData &model_time_info = prof_infer_data_;
  model_time_info.modelId = model_id_;
  prof_mgr.SetAlternativeValue(MSPROF_MIX_DATA_STRING_LEN, model_name, model_time_info.modelName);
  model_time_info.requestId = static_cast<uint32_t>(request_id);
  model_time_info.threadId = static_cast<uint32_t>(mmGetTid());

  // report model data tag name
  std::string tag_name("model_time_info_");
  (void)tag_name.append(std::to_string(model_id_)).append("_").append(std::to_string(data_index));
  prof_mgr.ReportData(static_cast<int32_t>(device_id_), &model_time_info, sizeof(model_time_info), tag_name);
}

void DavinciModel::SetProfileTime(const ModelProcStage stage, const uint64_t end_time) {
  uint64_t prof_time = end_time;
  if (prof_time == 0U) {
    const mmTimespec tick_count = mmGetTickCount();
    const int64_t start_time = (tick_count.tv_sec * kTimeNanoRadix) + tick_count.tv_nsec;
    prof_time = static_cast<uint64_t>(start_time);
  }

  switch (stage) {
    case ModelProcStage::MODEL_LOAD_START:
      load_begin_time_ = prof_time;
      break;
    case ModelProcStage::MODEL_LOAD_END:
      load_end_time_ = prof_time;
      break;
    case ModelProcStage::MODEL_PRE_PROC_START:
      prof_infer_data_.inputDataStartTime = prof_time;
      break;
    case ModelProcStage::MODEL_PRE_PROC_END:
      prof_infer_data_.inputDataEndTime = prof_time;
      break;
    case ModelProcStage::MODEL_INFER_START:
      prof_infer_data_.inferStartTime = prof_time;
      break;
    case ModelProcStage::MODEL_INFER_END:
      prof_infer_data_.inferEndTime = prof_time;
      break;
    case ModelProcStage::MODEL_AFTER_PROC_START:
      prof_infer_data_.outputDataStartTime = prof_time;
      break;
    case ModelProcStage::MODEL_AFTER_PROC_END:
      prof_infer_data_.outputDataEndTime = prof_time;
      break;
    default:
      break;
  }
}

static Status CheckBufferSizeValid(const bool is_dynamic, const bool is_no_tiling, const uint64_t buffer_length,
                                   const uint64_t data_size) {
  if (is_dynamic || is_no_tiling) {
    GELOGI("No need to check output data size.");
    return SUCCESS;
  }
  if (buffer_length < data_size) {
    GELOGE(PARAM_INVALID, "invalid output buffer length[%zu], data size[%zu].", buffer_length, data_size);
    return FAILED;
  }
  if (buffer_length > data_size) {
    GELOGW("Tensor data size=%lu, buffer size=%lu", data_size, buffer_length);
  }
  return SUCCESS;
}

///
/// @ingroup ge
/// @brief send Output Op result to upper layer
/// @already malloced in ModelLoad, no need to malloc again
/// @param [in] data_id: the index of output_data
/// @param [in/out] output_data: real user output_data
/// @param [in] kind: the kind of rtMemcpy
/// @return Status result
/// @author
///
Status DavinciModel::CopyOutputData(const OutputData &output_data, const rtMemcpyKind_t kind) {
  if (!has_output_node_) {
    return SyncVarData();
  }

  if (output_data.blobs.size() != output_data_info_.size()) {
    REPORT_INNER_ERROR("E19999", "output_data.blobs.size:%zu != output_data_info.size:%zu, model_id:%u, check invalid",
                       output_data.blobs.size(), output_data_info_.size(), model_id_);
    GELOGE(FAILED, "[Check][Param] output_data.blobs.size:%zu != output_data_info.size:%zu, model_id:%u",
           output_data.blobs.size(), output_data_info_.size(), model_id_);
    return FAILED;
  }

  const std::vector<DataBuffer> &blobs = output_data.blobs;
  size_t idx = 0U;
  for (const auto &output : output_data_info_) {
    const size_t output_idx = output.first;
    if (output_idx >= blobs.size()) {
      REPORT_INNER_ERROR("E19999", "index:%u in output_data_info_ >= output_data.blobs.size:%zu, model_id:%u, "
                         "check invalid", output.first, blobs.size(), model_id_);
      GELOGE(FAILED, "[Check][Param] index:%u in output_data_info_ >= output_data.blobs.size:%zu, model_id:%u",
             output.first, blobs.size(), model_id_);
      return FAILED;
    }

    void *output_addr = output.second.GetBasicAddr();
    const bool feed_by_zero_copy = (kind == RT_MEMCPY_DEVICE_TO_DEVICE) && (copy_only_addrs_.count(output_addr) == 0U);
    if (feed_by_zero_copy) {
      continue;  // Skip: Feed by zero copy.
    }

    const DataBuffer &buffer = blobs.at(output_idx);
    const uint64_t mem_size = static_cast<uint64_t>(output.second.GetDataSize());
    if ((buffer.length == 0U) || (mem_size == 0U)) {
      GELOGI("Length of data is zero, No need copy. output tensor index=%u", output.first);
      continue;
    }
    const bool is_no_tiling = (output_idx < output_no_tiling_flag_.size()) ? output_no_tiling_flag_[output_idx] : false;
    GE_CHK_STATUS_RET(CheckBufferSizeValid(is_dynamic_, is_no_tiling, buffer.length, mem_size),
                      "[Check][Param] Buffer.length:%lu in output blob < data_size:%lu in output_data_info, index:%u, "
                      "model_id:%u", buffer.length, mem_size, output.first, model_id_);

    uint64_t data_size = static_cast<uint64_t>(output.second.GetDataSize());

    if (is_online_infer_dynamic_) {
      const std::map<size_t, std::map<std::vector<int32_t>, int64_t>>::const_iterator it =
          merge_nodes_gear_and_real_out_size_info_.find(idx);
      if (it != merge_nodes_gear_and_real_out_size_info_.cend()) {
        const std::map<std::vector<int32_t>, int64_t>::const_iterator size_it = it->second.find(cur_dynamic_dims_);
        data_size = (size_it != it->second.cend()) ? static_cast<uint64_t>(size_it->second) : 0U;
      }
    }
    GE_CHECK_NOTNULL(buffer.data);
    if (is_no_tiling) {
      const std::map<uint32_t, void *>::const_iterator iter = output_no_tiling_data_addr_.find(output.first);
      if (iter == output_no_tiling_data_addr_.cend()) {
        GELOGE(PARAM_INVALID, "invalid no tiling data addr, output_no_tiling_data_addr_ size[%zu], output index[%u]",
               output_no_tiling_data_addr_.size(), output.first);
        return PARAM_INVALID;
      }
      output_addr = iter->second;
      data_size = buffer.length; // use the new size calculated with real shape
      GELOGD("Output [%u] is no tiling, data addr[%p].", output.first, output_addr);
    }
    GE_CHECK_NOTNULL(output_addr);
    GELOGI("CopyPlainData memcpy %s graph_%u type[F] output[%u] dst[%p] memaddr[%p] mem_size[%lu] datasize[%lu]",
           (is_async_mode_ ? "async" : "sync"), runtime_param_.graph_id, output.first, buffer.data, output_addr,
           data_size, buffer.length);
    if (is_async_mode_) {
      GE_CHK_RT_RET(rtMemcpyAsync(buffer.data, buffer.length, output_addr, data_size, kind, rt_model_stream_));
    } else {
      GE_CHK_RT_RET(rtMemcpy(buffer.data, buffer.length, output_addr, data_size, kind));
    }

    idx++;
  }
  return SUCCESS;
}

Status DavinciModel::InitOutputTensorInfo(const OpDescPtr &op_desc) {
  size_t input_num = op_desc->GetInputsSize();
  if (is_getnext_sink_dynamic_) {
    input_num = input_num - kGetDynamicDimsCount;
  }

  for (size_t i = 0U; i < input_num; ++i) {
    int64_t size = 0;
    const auto input_desc = op_desc->GetInputDescPtr(static_cast<uint32_t>(i));
    GE_CHECK_NOTNULL(input_desc);
    const auto ret = TensorUtils::GetTensorSizeInBytes(*input_desc, size);
    if (ret != GRAPH_SUCCESS) {
      REPORT_INNER_ERROR("E19999", "Get input TensorSize in op:%s(%s) failed, input_index:%zu, model_id:%u",
                         op_desc->GetName().c_str(), op_desc->GetType().c_str(), i, model_id_);
      GELOGE(ret, "[Get][InputTensorSize] in op:%s(%s) failed, input_index:%zu, model_id:%u",
             op_desc->GetName().c_str(), op_desc->GetType().c_str(), i, model_id_);
      return ret;
    }
    const GeShape &shape = input_desc->GetShape();
    bool is_no_tiling = false;
    (void)AttrUtils::GetBool(input_desc, ATTR_NAME_TENSOR_NO_TILING_MEM_TYPE, is_no_tiling);
    GELOGI("Output size is %ld, output shape is %s, no tiling is %d.",
           size, ToString(shape.GetDims()).c_str(), static_cast<int32_t>(is_no_tiling));
    output_buffer_size_.emplace_back(size);
    output_shape_info_.emplace_back(shape);
    output_no_tiling_flag_.push_back(is_no_tiling);
    if (is_no_tiling) {
      has_no_tiling_output_ = true;
    }
  }

  return SUCCESS;
}

Status DavinciModel::BuildOutputShapeInfo(const size_t output_idx, std::vector<int64_t> &output_shape,
                                          int64_t &output_size) {
  if (output_no_tiling_flag_[output_idx]) {
    const std::map<uint32_t, ZeroCopyOffset>::const_iterator output = output_data_info_.find(output_idx);
    if (output == output_data_info_.cend()) {
      GELOGE(FAILED, "output_data_info_[%zu] is empty.", output_idx);
      return FAILED;
    }
    RuntimeTensorDesc tensor_desc;
    GE_CHK_RT_RET(rtMemcpy(&tensor_desc, sizeof(RuntimeTensorDesc),
                           output->second.GetBasicAddr(), sizeof(RuntimeTensorDesc), RT_MEMCPY_DEVICE_TO_HOST));
    const int64_t dim_num = tensor_desc.shape[0];
    for (int64_t dim_loop = 0; dim_loop < dim_num; dim_loop++) {
      output_shape.emplace_back(tensor_desc.shape[dim_loop + 1]);
    }
    GE_CHK_STATUS_RET(TensorUtils::CalcTensorMemSize(GeShape(output_shape),
                                                     static_cast<Format>(tensor_desc.format),
                                                     static_cast<DataType>(tensor_desc.dtype), output_size),
                      "tensor[%zu] CalcTensorMemSize failed.", output_idx);
    output_no_tiling_data_addr_[output_idx] = ValueToPtr(tensor_desc.data_addr);
    GELOGD("Output [%zu] is no tiling, out desc addr[%p], data addr[%#lx].",
           output_idx, output->second.GetBasicAddr(), tensor_desc.data_addr);
  } else {
    output_shape = output_shape_info_[output_idx].GetDims();
    if (is_online_infer_dynamic_) {
      const std::map<size_t, std::map<std::vector<int32_t>, int64_t>>::const_iterator it0 =
          merge_nodes_gear_and_real_out_size_info_.find(output_idx);
      if (it0 != merge_nodes_gear_and_real_out_size_info_.cend()) {
        const std::map<std::vector<int32_t>, int64_t>::const_iterator size_it = it0->second.find(cur_dynamic_dims_);
        output_size = (size_it != it0->second.cend()) ? size_it->second : 0;
        is_dynamic_ = true;
      }
      const std::map<size_t, std::map<std::vector<int32_t>, std::vector<int64_t>>>::const_iterator it1 =
          merge_nodes_gear_and_real_out_shape_info_.find(output_idx);
      if (it1 != merge_nodes_gear_and_real_out_shape_info_.cend()) {
        const auto shape_it = it1->second.find(cur_dynamic_dims_);
        output_shape = (shape_it != it1->second.end()) ? shape_it->second : std::vector<int64_t>{};
      }
    }
  }
  return SUCCESS;
}

Status DavinciModel::GenOutputTensorInfo(OutputData &output_data, std::vector<Tensor> &outputs) {
  if (!output_data.blobs.empty()) {
    GELOGI("No need to generate output tensor info, model id:%u", model_id_);
    return SUCCESS;
  }

  std::vector<int64_t> output_size_info;
  std::vector<std::vector<int64_t>> output_shape_info;
  const size_t output_num = output_buffer_size_.size();
  for (size_t i = 0U; i < output_num; ++i) {
    if (output_no_tiling_flag_[i] && (output_data_info_.find(i) == output_data_info_.end())) {
      GELOGW("output_data_info_[%zu] is empty.", i);
      continue;
    }

    std::vector<int64_t> output_shape;
    int64_t output_size = output_buffer_size_[i];
    GE_CHK_STATUS_RET_NOLOG(BuildOutputShapeInfo(i, output_shape, output_size));

    GELOGI("Output size is %ld, output shape is %s.", output_size, ToString(output_shape).c_str());
    output_size_info.push_back(output_size);
    output_shape_info.push_back(output_shape);
  }

  GELOGI("Output blobs size:%zu, model id:%u", output_size_info.size(), model_id_);
  for (size_t i = 0U; i < output_size_info.size(); ++i) {
    const auto output_buffer = MakeShared<AlignedPtr>(output_size_info[i], kMemAlignment);
    GE_CHECK_NOTNULL(output_buffer);
    const GeTensorDesc tensor_desc(GeShape(output_shape_info[i]), FORMAT_ND, DT_FLOAT);
    GeTensor ge_tensor(tensor_desc);
    ge_tensor.SetData(output_buffer, static_cast<size_t>(output_size_info[i]));
    Tensor out_tensor = TensorAdapter::AsTensor(ge_tensor);

    void *const data_ptr = output_buffer->MutableGet();
    output_data.blobs.push_back({data_ptr, static_cast<uint64_t>(output_size_info[i])});
    outputs.emplace_back(std::move(out_tensor));
    GELOGD("Output index:%zu, output dims is %s, data length:%lu.",
           i, ToString(output_shape_info[i]).c_str(), output_size_info[i]);
  }

  return SUCCESS;
}
///
/// @ingroup ge
/// @brief send Output Op result to upper layer
/// @already malloced in ModelLoad, no need to malloc again
/// @param [in] data_id: the index of output_data
/// @param [in] rslt_flg: result flag
/// @param [in] seq_end_flag: sequence end flag
/// @param [out] output_data: real user output_data
/// @return Status result
/// @author
///
void DavinciModel::ReturnResult(const uint32_t data_id, const bool rslt_flg, const bool seq_end_flag,
                                OutputData &output_data) {
  if (listener_ == nullptr) {
    REPORT_INNER_ERROR("E19999", "listener_ is nullptr, check invalid.");
    GELOGE(PARAM_INVALID, "listener_ is nullptr, check invalid, model id: %u", model_id_);
    return;
  }

  // return result is not required
  std::vector<Tensor> outputs;
  if ((!rslt_flg) && (!seq_end_flag)) {
    GELOGW("Compute failed, model id: %u", model_id_);
    const auto &exception_infos = ModelManager::GetInstance().GetExceptionInfos();
    if (!exception_infos.empty()) {
      GE_CHK_STATUS(DumpExceptionInfo(exception_infos),
                    "[Dump][Exception] Dump exception info failed, model_id:%u.", model_id_);
    }
    GE_CHK_STATUS(listener_->OnComputeDone(model_id_, data_id, INTERNAL_ERROR, outputs),
                  "[Call][OnComputeDone] failed, model_id:%u, data_id:%u.", model_id_, data_id);
    return;
  }

  if (!has_output_node_) {
    GELOGW("The tensor list of output is empty, model id: %u", model_id_);
    GE_CHK_STATUS(listener_->OnComputeDone(model_id_, data_id, INTERNAL_ERROR, outputs),
                  "[Call][OnComputeDone] failed, model_id:%u, data_id:%u.", model_id_, data_id);
    return;
  }

  output_data.index = data_id;
  output_data.model_id = model_id_;

  if (is_getnext_sink_dynamic_) {
    GELOGD("Reinit cur dynamic dims when getnext sink dynamic.");
    cur_dynamic_dims_.clear();
    cur_dynamic_dims_.resize(shape_of_cur_dynamic_dims_);
    const auto ret = rtMemcpy(cur_dynamic_dims_.data(), shape_of_cur_dynamic_dims_ * sizeof(int32_t),
                              netoutput_last_input_addr_, static_cast<uint64_t>(netoutput_last_input_size_),
                              RT_MEMCPY_DEVICE_TO_HOST);
    GE_CHK_RT_EXEC(ret, return);
  }

  GE_MAKE_GUARD(output_tensor_release, [&]() {
    if (!outputs.empty()) {
      output_data.blobs.clear();
    }
  });
  GELOGD("Cur dynamic dims is %s.", ToString(cur_dynamic_dims_).c_str());
  if (GenOutputTensorInfo(output_data, outputs) != SUCCESS) {
    return;
  }

  if (CopyOutputData(output_data, RT_MEMCPY_DEVICE_TO_HOST) != SUCCESS) {
    GE_CHK_STATUS(listener_->OnComputeDone(model_id_, data_id, INTERNAL_ERROR, outputs),
                  "[Call][OnComputeDone] failed, model_id:%u, data_id:%u.", model_id_, data_id);
    return;
  }

  if (seq_end_flag) {
    GELOGW("End of sequence, model id: %u", model_id_);
    GE_CHK_STATUS(listener_->OnComputeDone(model_id_, data_id, END_OF_SEQUENCE, outputs),
                  "[Call][OnComputeDone] failed, model_id:%u, data_id:%u.", model_id_, data_id);
    return;
  }

  GE_CHK_STATUS(listener_->OnComputeDone(model_id_, data_id, SUCCESS, outputs),
                "[Call][OnComputeDone] failed, model_id:%u, data_id:%u.", model_id_, data_id);
}
///
/// @ingroup ge
/// @brief return not output to upper layer for cloud case
/// @param [in] data_id
/// @return Status result
///
void DavinciModel::ReturnNoOutput(const uint32_t data_id) {
  GELOGI("ReturnNoOutput model id:%u.", model_id_);
  if (listener_ == nullptr) {
    REPORT_INNER_ERROR("E19999", "listener_ is nullptr, check invalid.");
    GELOGE(PARAM_INVALID, "[Check][Param]listener_ is nullptr, check invalid.");
    return;
  }

  std::vector<Tensor> outputs;
  GE_CHK_STATUS(listener_->OnComputeDone(model_id_, data_id, SUCCESS, outputs),
                "[Call][OnComputeDone] failed, model_id:%u, data_id:%u.", model_id_, data_id);
}

uint32_t DavinciModel::GetResultCode() {
  GE_CHK_BOOL_EXEC(listener_ != nullptr,
                   REPORT_INNER_ERROR("E19999", "listener_ is nullptr, check invalid.");
                   return PARAM_INVALID, "[Check][Param] listener_ is null!");
  return listener_->GetResultCode();
}

Status DavinciModel::ResetResult() {
  GE_CHK_BOOL_EXEC(listener_ != nullptr,
                   REPORT_INNER_ERROR("E19999", "listener_ is nullptr, check invalid.");
                   return PARAM_INVALID, "[Check][Param] listener_ is null!");
  return listener_->ResetResult();
}

void DavinciModel::Run() {
  const uint32_t run_dev_id = GetContext().DeviceId();
  ErrorManager::GetInstance().SetErrorContext(GetErrorContext());

  GELOGI("Model Run thread start, model_id:%u.", model_id_);
  if (ModelUtils::SetDevice(run_dev_id, GetDieId()) != SUCCESS) {
    GELOGE(FAILED, "[Run][Setdevice] failed, model_id:%u, device_id:%u die_id:%ld.", model_id_, run_dev_id, GetDieId());
    return;
  }
  // DeviceReset before thread run finished!
  GE_MAKE_GUARD(reset_device, [run_dev_id]() {
    GE_CHK_STATUS(ModelUtils::ResetDevice(run_dev_id));
  });

  auto &prof_mgr = ProfilingManager::Instance();
  ErrorManager::GetInstance().SetStage(error_message::kModelExecute, error_message::kModelExecute);
  while (run_flg_) {
    // Model hasn't truly started runing before received data
    SetRunningFlag(false);
    std::shared_ptr<InputDataWrapper> data_wrapper;
    Status ret = data_inputer_.Pop(data_wrapper);
    const bool close_terminated = (data_wrapper == nullptr) || (ret != SUCCESS) || (!run_flg_);
    if (close_terminated) {
      GELOGW("data_wrapper is null or data queue closed, exit!");
      break;
    }

    // Model run indeedly start after received data.
    GE_IF_BOOL_EXEC(prof_mgr.ProfilingModelExecuteOn(), SetProfileTime(ModelProcStage::MODEL_PRE_PROC_START));
    SetRunningFlag(true);
    InputData current_data = data_wrapper->GetInput();
    GELOGI("Model thread Run begin, model id:%u, data index:%u.", model_id_, current_data.index);
    ret = HandleInputData(current_data);
    if (ret != SUCCESS) {
      ReturnResult(current_data.index, false, false, *data_wrapper->GetOutput());
      GELOGE(FAILED, "[Call][HandleInputData] handle input data failed, model_id:%u.", model_id_);
      continue;
    }
    GE_IF_BOOL_EXEC(prof_mgr.ProfilingModelExecuteOn(), SetProfileTime(ModelProcStage::MODEL_PRE_PROC_END));

    GE_IF_BOOL_EXEC(prof_mgr.ProfilingModelExecuteOn(), SetProfileTime(ModelProcStage::MODEL_INFER_START));
    GE_TIMESTAMP_START(rtModelExecute);
    GELOGI("rtModelExecute start, model id:%u.", model_id_);
    const uint64_t index_id = iterator_count_ + 1U;
    (void)prof_mgr.ProfileStepInfo(index_id, model_id_, 0U, rt_model_stream_, run_dev_id);
    auto rt_ret = rtModelExecute(rt_model_handle_, rt_model_stream_, 0U);
    if (rt_ret != RT_ERROR_NONE) {
      ReturnResult(current_data.index, false, false, *data_wrapper->GetOutput());
      continue;
    }
    GELOGI("rtModelExecute end, model id:%u.", model_id_);
    (void)prof_mgr.ProfileStepInfo(index_id, model_id_, 1U, rt_model_stream_, run_dev_id);
    GE_IF_BOOL_EXEC(is_first_execute_, GE_TIMESTAMP_EVENT_END(rtModelExecute, "rtModelExecute"));
    iterator_count_++;

    GE_TIMESTAMP_START(rtStreamSynchronize);
    GELOGI("rtStreamSynchronize start, model id:%u.", model_id_);
    rt_ret = rtStreamSynchronize(rt_model_stream_);
    if (rt_ret == ACL_ERROR_RT_SOCKET_CLOSE) {
      GELOGI("model exec failed befause socket closed, model_id:%u", model_id_);
      ModelManager::GetInstance().SetSocketCloseStatus(true);
    }
    const bool model_abort = ((rt_ret == kSinkModelAbortNormal) || (rt_ret == kSinkModelAbortNormalNew));
    if ((!model_abort) && (rt_ret != RT_ERROR_NONE)) {
      const bool seq_end_flag = ((rt_ret == kSinkModelEndOfSequence) || (rt_ret == kSinkModelEndOfSequenceNew));
      GELOGI("seq_end_flg: %d", static_cast<int32_t>(seq_end_flag));
      ReturnResult(current_data.index, false, seq_end_flag, *data_wrapper->GetOutput());
      continue;
    }
    GELOGI("rtStreamSynchronize end, model id:%u, status:%s.", model_id_, model_abort ? "abort" : "normal");
    GE_IF_BOOL_EXEC(is_first_execute_, GE_TIMESTAMP_EVENT_END(rtStreamSynchronize, "Wait for rtStreamSynchronize"));
    GE_IF_BOOL_EXEC(prof_mgr.ProfilingModelExecuteOn(), SetProfileTime(ModelProcStage::MODEL_INFER_END));

    GE_IF_BOOL_EXEC(prof_mgr.ProfilingModelExecuteOn(), SetProfileTime(ModelProcStage::MODEL_AFTER_PROC_START));
    GE_TIMESTAMP_START(ReturnResult);
    // copy output data from device to host
    GE_IF_BOOL_EXEC(has_output_node_, ReturnResult(current_data.index, true, false, *data_wrapper->GetOutput()));
    // copy output data from device to host for variable graph
    GE_IF_BOOL_EXEC(!has_output_node_, ReturnNoOutput(current_data.index));
    GE_IF_BOOL_EXEC(is_first_execute_, GE_TIMESTAMP_EVENT_END(ReturnResult, "CopyDataFromDeviceToHost"));
    GE_IF_BOOL_EXEC(prof_mgr.ProfilingModelExecuteOn(), SetProfileTime(ModelProcStage::MODEL_AFTER_PROC_END));
    GE_IF_BOOL_EXEC(prof_mgr.ProfilingModelExecuteOn(), SinkTimeProfile(current_data.index, current_data.request_id));

    is_first_execute_ = false;
    // model run finished
    GELOGI("run iterator count is %lu, model_id:%u", iterator_count_, model_id_);
  }

  GELOGI("Model run end, model id:%u", model_id_);
}

///
/// @ingroup ge
/// @brief call API provided by data inputer to destroy thread
/// @param [in] no
/// @return Status Destroy result
/// @author
///
Status DavinciModel::DestroyThread() {
  run_flg_ = false;

  data_inputer_.Stop();
  if (thread_id_.joinable()) {
    thread_id_.join();
  }

  return SUCCESS;
}

///
/// @ingroup ge
/// @brief create model std::thread,
/// @brief start to execute Model
/// @param [in] no
/// @return Status create model thread and execute result
/// @author
///
Status DavinciModel::ModelRunStart() {
  const std::unique_lock<std::mutex> lk(mux_run_flg_);
  GE_CHK_BOOL_RET_STATUS(!run_flg_, INTERNAL_ERROR, "[Check][Param] Model already started, model id:%u.", model_id_);
  run_flg_ = true;

  // create stream instance which rt_model_handel is running on
  GE_CHK_RT_RET(rtStreamCreate(&rt_model_stream_, priority_));
  is_inner_model_stream_ = true;

  error_context_ = ErrorManager::GetInstance().GetErrorManagerContext();
  thread_id_ = std::thread(&DavinciModel::Run, this);

  GELOGI("model thread create success, model id:%u.", model_id_);
  return SUCCESS;
}

///
/// @ingroup ge
/// @brief call API provided by data inputer and destroy model Thread
/// @param [in] no
/// @return Status Destroy result
/// @author
///
Status DavinciModel::ModelRunStop() {
  const std::unique_lock<std::mutex> lk(mux_run_flg_);
  GE_CHK_STATUS_RET(DestroyThread(), "[Destoy][Thead] failed, model id:%u.", model_id_);

  return SUCCESS;
}

void DavinciModel::UnbindTaskSinkStream() {
  // unbinding hcom stream
  UnbindHcomStream();
  if (is_stream_list_bind_) {
    for (size_t i = 0U; i < stream_list_.size(); ++i) {
      // unbind rt_model_handle and streams
      GE_LOGW_IF(rtModelUnbindStream(rt_model_handle_, stream_list_[i]) != RT_ERROR_NONE,
                 "Unbind stream from model failed! Index: %zu", i);
    }
  }

  if (is_inner_model_stream_) {
    if ((!input_queue_ids_.empty()) || (!output_queue_ids_.empty())) {
      GE_LOGW_IF(rtModelUnbindStream(rt_model_handle_, rt_model_stream_) != RT_ERROR_NONE, "Unbind stream failed!");
    }
    // destroy stream that is bound with rt_model
    GE_LOGW_IF(rtStreamDestroy(rt_model_stream_) != RT_ERROR_NONE, "Destroy stream for rt_model failed.")
  }

  if (is_pure_head_stream_ && (rt_head_stream_ != nullptr)) {
    GE_LOGW_IF(rtModelUnbindStream(rt_model_handle_, rt_head_stream_) != RT_ERROR_NONE, "Unbind stream failed!");
    GE_LOGW_IF(rtStreamDestroy(rt_head_stream_) != RT_ERROR_NONE, "Destroy stream for rt_model failed.");
    rt_head_stream_ = nullptr;
  }

  if (rt_entry_stream_ != nullptr) {
    GE_LOGW_IF(rtModelUnbindStream(rt_model_handle_, rt_entry_stream_) != RT_ERROR_NONE, "Unbind stream failed!");
    GE_LOGW_IF(rtStreamDestroy(rt_entry_stream_) != RT_ERROR_NONE, "Destroy stream for rt_model failed.");
    rt_entry_stream_ = nullptr;
  }
}

void *DavinciModel::GetRunAddress(void *const addr) const {
  if (fixed_mem_base_ == mem_base_) {
    return addr;
  }

  const uintptr_t ptr = PtrToValue(addr);
  if ((fixed_mem_base_ <= ptr) && (ptr < (fixed_mem_base_ + runtime_param_.mem_size))) {
    return ValueToPtr(mem_base_ + (ptr - fixed_mem_base_));
  } else {
    return addr;
  }
}

void DavinciModel::SetTotalIOAddrs(const std::vector<void *> &io_addrs, const uint32_t mem_type) {
  auto &total_io_addrs = total_io_addrs_[GetMemoryTypeIndex(mem_type)];
  if (fixed_mem_base_ == mem_base_) {
    (void)total_io_addrs.insert(total_io_addrs.cend(), io_addrs.cbegin(), io_addrs.cend());
    return;
  }

  for (size_t i = 0U; i < io_addrs.size(); ++i) {
    total_io_addrs.emplace_back(GetRunAddress(io_addrs[i]));
  }
}

Status DavinciModel::UpdateRunAddr(std::vector<void *> &total_io_addrs, const bool update_args) {
  if ((fixed_mem_base_ != mem_base_) && update_args) {
    for (size_t i = 0U; i < total_io_addrs.size(); ++i) {
      total_io_addrs[i] = GetRunAddress(total_io_addrs[i]);
    }
  }

  GE_CHK_STATUS_RET(UpdateZeroCopyAddr(total_io_addrs, user_input_addrs_, user_output_addrs_),
                    "[Update][ZeroCopyAddr] failed, io addr size : %zu.", total_io_addrs.size());
  GELOGI("update run address success, total io addrs size: %zu", total_io_addrs.size());
  return SUCCESS;
}

const void *DavinciModel::GetLogicalAddress(void *const addr) const {
  if (fixed_mem_base_ == mem_base_) {
    return addr;
  }

  const uintptr_t ptr = PtrToValue(addr);
  if ((mem_base_ <= ptr) && (ptr < (mem_base_ + runtime_param_.mem_size))) {
    return ValueToPtr(fixed_mem_base_ + (ptr - mem_base_));
  } else {
    return addr;
  }
}

Status DavinciModel::UpdateZeroCopyAddr(std::vector<void *> &total_io_addrs, const std::vector<void *> &inputs,
                                        const std::vector<void *> &outputs) {
  for (size_t i = 0U; i < total_io_addrs.size(); ++i) {
    const auto in_idx = known_input_idx_.find(GetLogicalAddress(total_io_addrs[i]));
    if (in_idx != known_input_idx_.end() && in_idx->second < inputs.size()) {
      GELOGI("Addr index %zu, input index %u, v addr %p, p addr %p.",
             i, in_idx->second, total_io_addrs[i], inputs[in_idx->second]);
      total_io_addrs[i] = inputs[in_idx->second];
      continue;
    }

    const auto out_idx = known_output_idx_.find(GetLogicalAddress(total_io_addrs[i]));
    if (out_idx != known_output_idx_.end() && out_idx->second < outputs.size()) {
      GELOGI("Addr index %zu, output index %u, v addr %p, p addr %p.",
             i, out_idx->second, total_io_addrs[i], outputs[out_idx->second]);
      total_io_addrs[i] = outputs[out_idx->second];
      continue;
    }
  }
  return SUCCESS;
}

Status DavinciModel::UpdateKnownNodeArgs(const std::vector<void *> &inputs, const std::vector<void *> &outputs) {
  GELOGI("DavinciModel::UpdateKnownNodeArgs begin");
  user_input_addrs_ = inputs;
  user_output_addrs_ = outputs;
  total_io_addrs_ = {{}, {}};  // reset
  for (size_t task_index = 0U; task_index < task_list_.size(); ++task_index) {
    auto &task = task_list_[task_index];
    if (task != nullptr) {
      const Status ret = task->UpdateArgs();
      if (ret != SUCCESS) {
        REPORT_CALL_ERROR("E19999", "task %zu update args failed, model_id:%u", task_index, model_id_);
        GELOGE(FAILED, "[Update][Args] to task %zu failed, model_id:%u.", task_index, model_id_);
        return FAILED;
      }
    }
  }

  for (size_t i = 0U; i < known_args_sizes_.size(); ++i) {
    auto &total_io_addrs = total_io_addrs_[i];
    GE_CHK_STATUS_RET(UpdateZeroCopyAddr(total_io_addrs, inputs, outputs),
                      "[Call][UpdateKnownZeroCopyAddr] failed, model_id:%u.", model_id_);
    const auto known_args_size = known_args_sizes_[i];
    const auto known_args_base = known_args_bases_[i];
    if (known_args_size == 0U) {
      GELOGW("DavinciModel::UpdateKnownNodeArgs device args[%zu] 0x%lx, dst size %u, pass rtMemcpy.",
             i, known_args_base, known_args_size);
    } else {
      const uint64_t total_addr_size = total_io_addrs.size() * sizeof(uint64_t);
      GELOGI("DavinciModel::UpdateKnownNodeArgs device args[%zu] 0x%lx, dst size %u, src size %lu",
             i, known_args_base, known_args_size, total_addr_size);

      GE_CHK_RT_RET(rtMemcpy(ValueToPtr(known_args_base), static_cast<uint64_t>(known_args_size),
                             total_io_addrs.data(), total_addr_size, RT_MEMCPY_HOST_TO_DEVICE));
    }
  }
  GELOGI("DavinciModel::UpdateKnownNodeArgs success");
  return SUCCESS;
}

Status DavinciModel::InitTaskInfo(const domi::ModelTaskDef &model_task_def) {
  GELOGI("InitTaskInfo in, task size %d", model_task_def.task().size());
  task_list_.resize(static_cast<size_t>(model_task_def.task_size()));
  if (known_node_) {
    GE_CHK_STATUS_RET(MallocKnownArgs(model_task_def), "[Malloc][KnownArgs] failed, model_id:%u.", model_id_);
  }

  for (int32_t i = 0; i < model_task_def.task_size(); ++i) {
    // dynamic shape will create task_list_ before
    const auto &task_def = model_task_def.task(i);
    auto &task_info = task_list_[static_cast<size_t>(i)];
    if (task_info == nullptr) {
      task_info = TaskInfoFactory::Instance().Create(static_cast<rtModelTaskType_t>(task_def.type()));
    }
    GE_CHECK_NOTNULL(task_info);
    const Status ret = task_info->Init(task_def, this);
    if (ret != SUCCESS) {
      REPORT_CALL_ERROR("E19999", "Task index:%d init failed, ret:%d.", i, ret);
      GELOGE(ret, "[Init][Task] index:%d failed, ret:%d.", i, ret);
      return ret;
    }
  }
  GELOGI("InitTaskInfo out");
  return SUCCESS;
}

Status DavinciModel::MallocKnownArgs(const domi::ModelTaskDef &model_task_def) {
  GELOGI("DavinciModel::MallocKnownArgs in");
  if (model_task_def.task_size() == 0) {
    GELOGW("DavinciModel::MallocKnownArgs davincimodel has no task info.");
    return SUCCESS;
  }

  for (int32_t i = 0; i < model_task_def.task_size(); ++i) {
    const auto &task_def = model_task_def.task(i);
    auto &task_info = task_list_[static_cast<size_t>(i)];
    task_info = TaskInfoFactory::Instance().Create(static_cast<rtModelTaskType_t>(task_def.type()));
    GE_CHECK_NOTNULL(task_info);
    const auto ret = task_info->CalculateArgs(task_def, this);
    if (ret != SUCCESS) {
      REPORT_CALL_ERROR("E19999", "task index:%d CalculateArgs failed, ret:%d", i, ret);
      GELOGE(ret, "[Calculate][Args] for taskdef index:%d failed, ret:%d", i, ret);
      return ret;
    }
  }

  // malloc args memory
  void *malloc_mem_base = nullptr;
  if (known_args_sizes_[kMemTypeIndexHbm] != 0U) {
    GE_CHK_RT_RET(rtMalloc(&malloc_mem_base,
                           static_cast<uint64_t>(known_args_sizes_[kMemTypeIndexHbm]),
                           RT_MEMORY_HBM));
    known_args_bases_[kMemTypeIndexHbm] = PtrToValue(malloc_mem_base);
  }
  if (known_args_sizes_[kMemTypeIndexTs] != 0U) {
    const auto mem_type =
        rtGetTsMemType(MEM_REQUEST_FEATURE_DEFAULT, static_cast<uint64_t>(known_args_sizes_[kMemTypeIndexTs]));
    GELOGI("TS memory_type: %u", mem_type);
    GE_CHK_RT_RET(rtMalloc(&malloc_mem_base, static_cast<uint64_t>(known_args_sizes_[kMemTypeIndexTs]), mem_type));
    known_args_bases_[kMemTypeIndexTs] = PtrToValue(malloc_mem_base);
  }
  // malloc dynamic and static hybrid memory
  if (hybrid_args_size_ != 0U) {
    GE_CHK_RT_RET(rtMalloc(&malloc_mem_base, static_cast<uint64_t>(hybrid_args_size_), RT_MEMORY_HBM));
    hybrid_addrs_base_ = PtrToValue(malloc_mem_base);
  }
  // malloc fixed addr memory, eg: rts op
  if (fixed_addr_size_ > 0) {
    GELOGI("Begin to allocate fixed addr.");
    const auto mem_type = rtGetTsMemType(MEM_REQUEST_FEATURE_DEFAULT, static_cast<uint64_t>(fixed_addr_size_));
    GE_CHK_RT_RET(rtMalloc(&malloc_mem_base, static_cast<uint64_t>(fixed_addr_size_), mem_type));
    fixed_addrs_base_ = PtrToValue(malloc_mem_base);
  }

  GELOGI(
      "Malloc Task args success: total args size %u, total args size(TS) %u, hybrid args size %u, fixed addr size %ld",
      known_args_sizes_[kMemTypeIndexHbm],
      known_args_sizes_[kMemTypeIndexTs],
      hybrid_args_size_,
      fixed_addr_size_);
  return SUCCESS;
}

void DavinciModel::SaveProfilingTaskDescInfo(const OpDescPtr &op_desc, const TaskInfo &task_info,
                                             const domi::TaskDef &task_def) {
  char_t skt_enable_env[MMPA_MAX_PATH]{};
  const auto env_res = mmGetEnv("SKT_ENABLE", &skt_enable_env[0], static_cast<uint32_t>(MMPA_MAX_PATH));
  const int64_t env_flag = (env_res == EN_OK) ? std::strtol(&skt_enable_env[0], nullptr, kDecimalRadix) : 0;
  const bool flag = (env_flag != 0) ? true : GetL1FusionEnableOption();

  TaskDescInfo task_desc_info;
  task_desc_info.model_name = !om_name_.empty() ? om_name_ : name_;
  task_desc_info.op_name = op_desc->GetName();
  task_desc_info.op_type = op_desc->GetType();
  task_desc_info.block_dim = task_def.kernel().block_dim();
  task_desc_info.task_id = task_info.GetTaskID();
  task_desc_info.stream_id = task_info.GetStreamId();
  task_desc_info.shape_type = "static";
  task_desc_info.cur_iter_num = 0;
  task_desc_info.task_type = kTaskTypeInvalid;
  task_desc_info.context_id = std::numeric_limits<uint32_t>::max();

  const auto &prof_mgr = ProfilingManager::Instance();
  prof_mgr.GetOpInputOutputInfo(op_desc, task_desc_info);

  const auto model_task_type = static_cast<rtModelTaskType_t>(task_def.type());
  if (model_task_type == RT_MODEL_TASK_KERNEL) {
    const domi::KernelDef &kernel_def = task_def.kernel();
    const auto &context = kernel_def.context();
    const auto kernel_type = static_cast<ccKernelType>(context.kernel_type());
    if (kernel_type == ccKernelType::TE) {
      task_desc_info.task_type = kTaskTypeAicore;
    } else if ((kernel_type == ccKernelType::AI_CPU) || (kernel_type == ccKernelType::CUST_AI_CPU)) {
      task_desc_info.task_type = kTaskTypeAicpu;
    } else {
      GELOGD("Other kernel type: %u", context.kernel_type());
    }
  } else if (model_task_type == RT_MODEL_TASK_KERNEL_EX) {
    task_desc_info.task_type = kTaskTypeAicpu;
  } else {
    GELOGD("Skip task type: %d", static_cast<int32_t>(model_task_type));
  }

  task_desc_info_.emplace_back(task_desc_info);
  if (flag && (task_info.GetSktTaskID() != 0xFFFFFFFFU)) {
    TaskDescInfo super_task_desc_info;
    const std::string op_name = "super_kernel_" + to_string(op_desc->GetId());
    super_task_desc_info.op_name = op_name;
    super_task_desc_info.task_id = task_info.GetSktTaskID();
    task_desc_info_.emplace_back(super_task_desc_info);
  }
}

void DavinciModel::SaveFftsPlusProfilingTask(const domi::TaskDef &task_def, const TaskInfo &task_info) {
  // ffts plus task info
  // 1. report partitioncall op
  // 2. report context op
  const domi::FftsPlusTaskDef &ffts_plus_task_def = task_def.ffts_plus_task();
  const auto &sgt_node = GetOpByIndex(ffts_plus_task_def.op_index());
  if (sgt_node == nullptr) {
    GELOGW("FftsPlus node not found for index: %u", ffts_plus_task_def.op_index());
    return;
  }

  const domi::FftsPlusSqeDef &sqe_def = ffts_plus_task_def.ffts_plus_sqe();
  const domi::StarsSqeHeaderDef &sqe_header_def = sqe_def.sqe_header();
  TaskDescInfo task_desc_info;
  task_desc_info.op_name = om_name_.empty() ? name_ : om_name_;
  task_desc_info.op_type = kTaskTypeFftsPlus;
  task_desc_info.block_dim = sqe_header_def.block_dim();
  task_desc_info.task_id = task_info.GetTaskID();
  task_desc_info.stream_id = task_info.GetStreamId();
  task_desc_info.shape_type = kStaticShape;
  task_desc_info.cur_iter_num = 0;
  task_desc_info.context_id = std::numeric_limits<uint32_t>::max();

  const auto &prof_mgr = ProfilingManager::Instance();
  prof_mgr.GetOpInputOutputInfo(sgt_node, task_desc_info);
  task_desc_info_.emplace_back(task_desc_info);

  for (int32_t i = 0; i < ffts_plus_task_def.ffts_plus_ctx_size(); ++i) {
    const auto &ctx_def = ffts_plus_task_def.ffts_plus_ctx(i);
    const auto &ctx_op_desc = GetOpByIndex(ctx_def.op_index());
    if (ctx_op_desc == nullptr) {
      GELOGW("FftsPlus sub node not found for index: %u", ctx_def.op_index());
      return;
    }

    TaskDescInfo ctx_desc_info;
    ctx_desc_info.op_name = ctx_op_desc->GetName();
    ctx_desc_info.op_type = ctx_op_desc->GetType();
    ctx_desc_info.block_dim = sqe_header_def.block_dim();
    ctx_desc_info.task_id = task_info.GetTaskID();
    ctx_desc_info.stream_id = task_info.GetStreamId();
    ctx_desc_info.shape_type = "static";
    ctx_desc_info.cur_iter_num = 0;
    ctx_desc_info.task_type = kTaskTypeInvalid;
    ctx_desc_info.context_id = ctx_def.context_id();
    prof_mgr.GetOpInputOutputInfo(ctx_op_desc, ctx_desc_info);
    task_desc_info_.emplace_back(ctx_desc_info);
  }
}

Status DavinciModel::DistributeTask(const domi::ModelTaskDef &model_task_def) {
  GELOGI("DistributeTask in: model task: %d, cpu task: %zu", model_task_def.task().size(), cpu_task_list_.size());
  for (auto &task : cpu_task_list_) {
    GE_CHECK_NOTNULL(task);
    GE_CHK_STATUS_RET(task->Distribute());
  }

  task_desc_info_.clear();
  const auto &prof_mgr = ProfilingManager::Instance();
  for (int32_t task_index = 0; task_index < model_task_def.task_size(); ++task_index) {
    const auto &task_def = model_task_def.task(task_index);
    const auto &task_info = task_list_.at(static_cast<size_t>(task_index));
    GE_CHECK_NOTNULL(task_info);
    GE_CHK_STATUS_RET(task_info->Distribute(), "[Call][Distribute] for Task[%d] fail", task_index);

    if (prof_mgr.ProfilingModelLoadOn()) {
      for (auto fusion_op_info : task_info->GetAllFusionOpInfo()) {
        if (fusion_op_info.original_op_names.empty()) {
          continue;
        }
        fusion_op_info.task_id = task_info->GetTaskID();
        GELOGD("task id is %u, op num is %zu", fusion_op_info.task_id, fusion_op_info.original_op_names.size());
        GE_CHK_STATUS_RET(InitFusionProfiling(fusion_op_info), "[Init][Profiling] failed, model_id:%u.", model_id_);
      }
    }

    // for profiling and data dump
    task_info->PostProcess(task_def);
  }
  // launch dump kernel to aicpu
  GE_CHK_STATUS_RET(data_dumper_.LoadDumpInfo(), "[Load][DumpInfo] failed, model_id:%u.", model_id_);
  GELOGI("DistributeTask out");
  return SUCCESS;
}

void DavinciModel::SaveProfilingTask(const uint32_t op_idx, const domi::TaskDef &task_def, const TaskInfo &task_info) {
  const OpDescPtr &op_desc = GetOpByIndex(op_idx);
  if (op_desc == nullptr) {
    GELOGW("Node not found for profiling, index: %u", op_idx);
    return;
  }

  if (task_info.GetDumpArgs() != 0U) {
    const bool call_dump = OpNeedDump(op_desc->GetName()) && task_info.CallSaveDumpInfo();
    if (call_dump || is_op_debug_reg_) {
      SaveDumpTask(task_info.GetTaskID(), task_info.GetStreamId(), op_desc, task_info.GetDumpArgs());
    }
  }

  ExtraOpInfo extra_dump_info;
  extra_dump_info.args = task_info.GetArgs();
  extra_dump_info.input_addrs = ModelUtils::GetInputAddrs(runtime_param_, op_desc);
  extra_dump_info.output_addrs = ModelUtils::GetOutputAddrs(runtime_param_, op_desc);
  exception_dumper_.SaveDumpOpInfo(op_desc, task_info.GetTaskID(), task_info.GetStreamId(), extra_dump_info);

  // save task info for profiling
  SaveProfilingTaskDescInfo(op_desc, task_info, task_def);
}

bool DavinciModel::ModelNeedDump() {
  const auto all_dump_model = GetDumpProperties().GetAllDumpModel();
  return (all_dump_model.find(DUMP_ALL_MODEL) != all_dump_model.end()) ||
         (all_dump_model.find(dump_model_name_) != all_dump_model.end()) ||
         (all_dump_model.find(om_name_) != all_dump_model.end());
}

void DavinciModel::SetEndGraphId(const uint32_t task_id, const uint32_t stream_id) {
  if (ModelNeedDump()) {
    GELOGI("start save end_graph_info to dumper, task_id is %u, stream_id is %u", task_id, stream_id);
    data_dumper_.SaveEndGraphId(task_id, stream_id);
  }
}

///
/// @ingroup ge
/// @brief Set copy only for No task feed NetOutput address.
/// @return None.
///
void DavinciModel::SetCopyOnlyOutput() {
  for (const auto &output_outside_addrs : output_data_info_) {
    const ZeroCopyOffset &output_outside = output_outside_addrs.second;
    if (!output_outside.IsRelativeOffsetValid()) {
      return;
    }
    for (size_t out_count = 0U; out_count < output_outside.GetAddrCount(); ++out_count) {
      const auto &addrs_mapping_list = output_outside.GetOutsideAddrs();
      const std::map<uintptr_t, std::vector<uintptr_t>> &virtual_args_addrs = addrs_mapping_list[out_count];
      for (const auto &virtual_args_addr : virtual_args_addrs) {
        const auto &args_addrs = virtual_args_addr.second;
        if (args_addrs.empty()) {  // No task feed Output addr, Need copy directly.
          GELOGI("[ZCPY] just copy 0x%lx to netoutput.", virtual_args_addr.first);
          (void)copy_only_addrs_.insert(ValueToPtr(virtual_args_addr.first));
        }
      }
    }
  }
}

///
/// @ingroup ge
/// @brief Set disabled input zero copy addr.
/// @param [in] const void *addr: address of task
/// @return None.
///
void DavinciModel::DisableZeroCopy(const void *const addr) {
  if (real_virtual_addrs_.find(addr) == real_virtual_addrs_.end()) {
    return;
  }

  // Data link to RTS Op directly.
  const std::lock_guard<std::mutex> lk(outside_addrs_mutex_);
  GELOGI("[ZCPY] disable zero copy of %p.", addr);
  (void)copy_only_addrs_.insert(addr);
}

void DavinciModel::DisableZeroCopyInReuseMemoryMode(const NodePtr &node, const size_t idx, const void *const addr) {
  if (!IsInputOfNetoutputCanZeroCopy(node, static_cast<int32_t>(idx))) {
    for (const auto &item : input_data_info_) {
      if (item.second.GetBasicAddr() == addr) {
        GELOGI("Addr %p reference or reused from input and was determined by data node", addr);
        return;
      }
    }
    GELOGI("Disable zero copy for %p of %s input %zu", addr, node->GetName().c_str(), idx);
    const std::lock_guard<std::mutex> lk(outside_addrs_mutex_);
    (void)copy_only_addrs_.insert(addr);
  }
}

void DavinciModel::DisableZeroCopyNode(const OpDescPtr &op_desc) {
  const auto disable_addrs = [this](const std::vector<void *> &all_data_addrs) {
    for (const auto addr : all_data_addrs) {
      DisableZeroCopy(addr);
    }
  };

  const auto &node_type = op_desc->GetType();
  if (node_type == VARIABLE) {
    disable_addrs(ModelUtils::GetOutputAddrs(runtime_param_, op_desc));
  }
}

///
/// @ingroup ge
/// @brief Save outside address used info for ZeroCopy.
/// @param [in] const OpDescPtr &op_desc: current op desc
/// @param [in] const std::vector<void *> &outside_addrs: address of task
/// @param [in] const void *info: task args
/// @param [in] const char *args: task args
/// @param [in] size_t size: size of task args
/// @param [in] size_t offset: offset of task args
/// @return None.
///
void DavinciModel::SetZeroCopyAddr(const OpDescPtr &op_desc, const std::vector<void *> &outside_addrs,
                                   const void *const args_info, const uintptr_t args_base, const size_t args_size,
                                   const size_t offset) {
  // Internal call has ensured that op_desc is not nullptr
  GELOGD("[ZCPY] SetZeroCopyAddr for %s.", op_desc->GetName().c_str());
  const size_t nums = outside_addrs.size();
  ZeroCopyTask zero_copy_task(op_desc->GetName(), args_base, args_size);
  for (size_t i = 0U; i < nums; ++i) {
    const std::lock_guard<std::mutex> lk(outside_addrs_mutex_);

    for (auto &input_outside_addrs : input_data_info_) {
      ZeroCopyOffset &input_outside = input_outside_addrs.second;
      input_outside.SetOutsideAddrsValue(zero_copy_task, PtrToValue(outside_addrs[i]), args_base,
                                         offset + (i * kAddrSize));
    }

    for (auto &output_outside_addrs : output_data_info_) {
      ZeroCopyOffset &output_outside = output_outside_addrs.second;
      output_outside.SetOutsideAddrsValue(zero_copy_task, PtrToValue(outside_addrs[i]), args_base,
                                          offset + (i * kAddrSize));
    }
  }

  std::string batch_label;
  if ((!AttrUtils::GetStr(op_desc, ATTR_NAME_BATCH_LABEL, batch_label)) || batch_label.empty()) {
    zero_copy_task.SetBatchLabel(kDefaultBatchLabel);
  } else {
    zero_copy_task.SetBatchLabel(batch_label);
  }

  const std::lock_guard<std::mutex> lk(outside_addrs_mutex_);
  if (zero_copy_task.IsTaskArgsSet()) {
    zero_copy_task.SetOriginalArgs(args_info, offset + (nums * kAddrSize));
    zero_copy_tasks_.emplace_back(zero_copy_task);
  }
}

///
/// @ingroup ge
/// @brief Copy Check input size and model op size.
/// @param [in] const int64_t &input_size: input size.
/// @param [in] const int64_t &op_size: model op size.
/// @param [in] is_dynamic: dynamic batch input flag.
/// @return true if success
///
bool DavinciModel::CheckUserAndModelSize(const int64_t &size, const int64_t &op_size,
                                         const bool is_input, const bool is_dynamic) const {
  const std::string input_or_output = is_input ? "input" : "output";
  if (is_dynamic) {  // dynamic is max size.
    GELOGI("No need to check user %s and model size.", input_or_output.c_str());
    return true;
  }

  if (size > op_size) {
    GELOGW("User %s size [%ld] is bigger than om size [%ld], MAY cause inference ERROR, please check model input",
           input_or_output.c_str(), size, op_size);
  }

  if (is_dynamic_aipp_) {
    GELOGI("This is dynamic aipp model, no need to judge smaller user size");
    return true;
  }
  // Judge overflow first
  if (size > (std::numeric_limits<int64_t>::max() - kDataMemAlignSizeCompare)) {
    GELOGI("The user %s size [%ld] is smaller than model size [%ld] and is in the range of 64 bytes",
           input_or_output.c_str(), size, op_size);
    return true;
  }
  // The input and model input size can not be exactly equal because user input is not definite.
  if ((size + kDataMemAlignSizeCompare) < op_size) {
    REPORT_INNER_ERROR("E19999", "%s size:%ld from user add align:%ld < op_size:%ld in model:%u, check invalid",
                       input_or_output.c_str(), size, kDataMemAlignSizeCompare, op_size, model_id_);
    GELOGE(ACL_ERROR_GE_PARAM_INVALID, "[Check][Param] %s size:%ld from user add align:%ld < op_size:%ld in model:%u",
           input_or_output.c_str(), size, kDataMemAlignSizeCompare, op_size, model_id_);
    return false;
  }
  return true;
}

///
/// @ingroup ge
/// @brief Copy Inputs and Outputs addr to model for direct use.
/// @param [in] const InputData &input_data: model input data.
/// @param [in] OutputData &output_data: model output data.
/// @param [in] bool is_dynamic_input: whether is dynamic input, true: is dynamic input; false: not is dynamic input
/// @return SUCCESS handle successfully / PARAM_INVALID for failed
///
Status DavinciModel::CopyModelData(const InputData &input_data, OutputData &output_data, const bool is_dynamic) {
  if (UpdateIoTaskArgs(input_data_info_, true, input_data.blobs, is_dynamic, input_data.batch_label) != SUCCESS) {
    GELOGE(ACL_ERROR_GE_PARAM_INVALID, "[Call][UpdateIoTaskArgs] [ZCPY] Update input data to model:%u failed.",
           model_id_);
    return ACL_ERROR_GE_PARAM_INVALID;
  }

  if (UpdateIoTaskArgs(output_data_info_, false, output_data.blobs, is_dynamic, input_data.batch_label) !=
      SUCCESS) {
    GELOGE(ACL_ERROR_GE_PARAM_INVALID, "[Call][UpdateIoTaskArgs] [ZCPY] Update output data to model:%u failed.",
           model_id_);
    return ACL_ERROR_GE_PARAM_INVALID;
  }

  for (ZeroCopyTask &task : zero_copy_tasks_) {
    GE_CHK_STATUS_RET(task.DistributeParam(is_async_mode_, rt_model_stream_),
                      "[Call][DistributeParam] [ZCPY] Update args failed, model_id:%u.", model_id_);
  }

  output_data.index = input_data.index;
  output_data.model_id = model_id_;
  return SUCCESS;
}

///
/// @ingroup ge
/// @brief Build fast search hash table for zero copy tasks.
///
void DavinciModel::BuildZeroCopyTasksLookupTable() {
  const std::lock_guard<std::mutex> lk(lookup_table_build_lock_);
  if (lookup_table_built_) {
    return;
  }
  lookup_table_built_ = true;
  {
    const std::lock_guard<std::mutex> lk2(bundle_zero_copy_tasks_mutex_);
    for (const auto &task : bundle_zero_copy_tasks_) {
      zero_copy_tasks_.emplace_back(task);
    }
    bundle_zero_copy_tasks_.clear();
  }

  for (auto &task : zero_copy_tasks_) {
    const auto label = task.GetBatchLabel();
    const auto addr2offsets = task.GetTaskArgsOffset();

    (void)label2tasks_[label].insert(&task);
    auto &addr_label_tasks = (label == kDefaultBatchLabel) ? addr2default_label_tasks_ : addr2specific_label_tasks_;
    for (auto &addr2offset : addr2offsets) {
      (void)addr_label_tasks[addr2offset.first].insert(&task);
    }
  }
}

std::set<size_t> DavinciModel::GetZeroCopyArgsIndex(const std::vector<void *> &arg_logical_addrs) const {
  std::set<size_t> zero_copy_args_index;

  const auto addr_is_outline = [](const std::map<uint32_t, ZeroCopyOffset> &outline_addrs_info,
                                  const uintptr_t addr) -> bool {
    for (const auto &outline_addr_info : outline_addrs_info) {
      for (const auto &logical2outline : outline_addr_info.second.GetOutsideAddrs()) {
        if (logical2outline.find(addr) != logical2outline.end()) {
          return true;
        }
      }
    }
    return false;
  };

  for (size_t i = 0U; i < arg_logical_addrs.size(); i++) {
    if (addr_is_outline(input_data_info_, PtrToValue(arg_logical_addrs[i])) ||
        addr_is_outline(output_data_info_, PtrToValue(arg_logical_addrs[i]))) {
      (void)zero_copy_args_index.insert(i);
    }
  }

  return zero_copy_args_index;
}

void DavinciModel::SetLogicalOutsideAddrs(const std::map<uintptr_t, std::set<size_t>> &args_offset,
                                          const uintptr_t args_device_addr) {
  for (const auto &logical_offsets : args_offset) {
    for (const auto &offset : logical_offsets.second) {
      GELOGD("Set logical outside device addr for 0x%lx", logical_offsets.first);
      for (auto &info : input_data_info_) {
        info.second.SetLogicalOutsideAddrs(logical_offsets.first, args_device_addr + offset);
      }
      for (auto &info : output_data_info_) {
        info.second.SetLogicalOutsideAddrs(logical_offsets.first, args_device_addr + offset);
      }
    }
  }
}

Status DavinciModel::Mapping2BundleZeroCopy(const OpDescPtr op_desc,
                                            const std::map<uintptr_t, std::set<size_t>> &args_offset,
                                            const size_t args_size, const void *const args_host_copy,
                                            void *&args_device_addr, bool &own_memory) {
  std::string batch_label;
  if ((!AttrUtils::GetStr(op_desc, ATTR_NAME_BATCH_LABEL, batch_label)) || batch_label.empty()) {
    batch_label = kDefaultBatchLabel;
  }

  const std::lock_guard<std::mutex> lk(bundle_zero_copy_tasks_mutex_);
  GELOGD("Mapping zero copy task for %s to bundle, current bundle size %lu", op_desc->GetName().c_str(),
         bundle_zero_copy_tasks_.size());

  for (auto &task : bundle_zero_copy_tasks_) {
    if ((task.GetBatchLabel() == batch_label) && task.CanMerge(args_size)) {
      GELOGD("Mapping zero copy task for %s to existed bundle %s label %s", op_desc->GetName().c_str(),
             task.GetName().c_str(), batch_label.c_str());
      own_memory = false;
      GE_CHK_STATUS_RET_NOLOG(task.Merge(args_offset, args_size, args_host_copy, args_device_addr));
      SetLogicalOutsideAddrs(args_offset, PtrToValue(args_device_addr));
      return ge::SUCCESS;
    }
  }

  const static size_t kMaxPerBundleSize = 1024U * 8U; // 8K bytes
  // Predicted per task has 2 inputs and 1 output, then args size is (2 + 1) * 8(=sizeof(addr)) = 24
  // We assume a bundle size not over 8K bytes for better cache performance
  const size_t predicted_task_bundle_size =
      std::min((input_data_info_.size() + output_data_info_.size()) * 24U, kMaxPerBundleSize);
  // For huge task which has so many inputs and outputs, make it standalone
  const size_t dev_addr_size = std::max(predicted_task_bundle_size, args_size);

  void *dev_addr = nullptr;
  static uint64_t bundle_index = 0U;
  GE_CHK_RT_RET(rtMalloc(&dev_addr, dev_addr_size, RT_MEMORY_HBM));
  bundle_zero_copy_tasks_.push_back({"Bundle_" + std::to_string(bundle_index++), PtrToValue(dev_addr), dev_addr_size});
  auto &last_bundle_task = bundle_zero_copy_tasks_.back();
  last_bundle_task.SetBatchLabel(batch_label);
  own_memory = true;
  GELOGD("Mapping zero copy task for %s to new bundle %s label %s", op_desc->GetName().c_str(),
         last_bundle_task.GetName().c_str(), batch_label.c_str());
  GE_CHK_STATUS_RET_NOLOG(last_bundle_task.Merge(args_offset, args_size, args_host_copy, args_device_addr));
  SetLogicalOutsideAddrs(args_offset, PtrToValue(args_device_addr));
  return ge::SUCCESS;
}

///
/// @ingroup ge
/// @brief Update task parameters of zero copy tasks.
///
void DavinciModel::UpdateZeroCopyTaskParam(const std::pair<uint32_t, ZeroCopyOffset> &data,
                                           const DataBuffer &buffer, const std::string &batch_label,
                                           const bool is_input) {
  std::unordered_set<ZeroCopyTask *> same_batch_label_tasks;
  if (batch_label != kDefaultBatchLabel) {
    const auto iter = label2tasks_.find(batch_label);
    if (iter != label2tasks_.end()) {
      same_batch_label_tasks = iter->second;
    }
  }

  for (size_t i = 0U; i < data.second.GetDataCount(); ++i) {
    const auto addr = PtrToValue(data.second.GetDataInfo().at(i).second);
    const uint64_t buffer_addr = PtrToValue(buffer.data) + static_cast<uint64_t>(data.second.GetRelativeOffset().at(i));
    GELOGI("[ZCPY] Copy %s blobs_index %u, virtual_addr: 0x%lx, size: %ld, user_data_addr: 0x%lx, batch_label: %s",
           is_input ? "input" : "output", data.first, addr, data.second.GetDataInfo().at(i).first,
           buffer_addr, batch_label.c_str());
    // For input data, just copy for rts task.
    for (auto &task : addr2default_label_tasks_[addr]) {
      (void)task->UpdateTaskParam(addr, buffer_addr);
    }
    if (batch_label != kDefaultBatchLabel) {
      for (auto &task : addr2specific_label_tasks_[addr]) {
        if (same_batch_label_tasks.count(task) > 0U) {
          (void)task->UpdateTaskParam(addr, buffer_addr);
        }
      }
    }
  }
}

///
/// @ingroup ge
/// @brief Copy Data addr to model for direct use.
/// @param [in] data_info: model memory addr/size map { data_index, { tensor_size, tensor_addr } }.
/// @param [in] is_input: input data or output data
/// @param [in] blobs: user input/output data list.
/// @param [in] is_dynamic: whether is dynamic input, true: is dynamic input; false: not is dynamic input
/// @param [in] batch_label: batch label for multi-batch scenes
/// @return SUCCESS handle successfully / others handle failed
///
Status DavinciModel::UpdateIoTaskArgs(const std::map<uint32_t, ZeroCopyOffset> &data_info, const bool is_input,
                                      const std::vector<DataBuffer> &blobs, const bool is_dynamic,
                                      const std::string &batch_label) {
  if (blobs.size() != data_info.size()) {
    REPORT_INNER_ERROR("E19999", "is_input:%d blob size:%zu from user != op_size:%zu in model:%u, check invalid",
                       static_cast<int32_t>(is_input), blobs.size(), data_info.size(), model_id_);
    GELOGE(ACL_ERROR_GE_PARAM_INVALID, "[Check][Param] is_input:%d blob size:%zu from user != op_size:%zu in model:%u",
           static_cast<int32_t>(is_input), blobs.size(), data_info.size(), model_id_);
    return ACL_ERROR_GE_PARAM_INVALID;
  }

  BuildZeroCopyTasksLookupTable();

  for (const auto &data : data_info) {
    const size_t data_index = data.first;
    if (data_index >= blobs.size()) {  // check data index.
      REPORT_INNER_ERROR("E19999", "is_input:%d, data index:%u from model >= blobs.size:%zu from user, mode_id:%u"
                         "check invalid", static_cast<int32_t>(is_input), data.first, blobs.size(), model_id_);
      GELOGE(ACL_ERROR_GE_PARAM_INVALID,
             "[Check][Param] is_input:%d, data index:%u from model >= blobs.size:%zu from user, mode_id:%u",
             static_cast<int32_t>(is_input), data.first, blobs.size(), model_id_);
      return ACL_ERROR_GE_PARAM_INVALID;
    }

    const DataBuffer &buffer = blobs.at(data_index);  // index of data.
    if (buffer.data == nullptr) {
      REPORT_INNER_ERROR("E19999", "is_input:%d buffer from user is nullptr, index:%u, mode_id:%u check invalid",
                         static_cast<int32_t>(is_input), data.first, model_id_);
      GELOGE(ACL_ERROR_GE_PARAM_INVALID, "[Check][Param] data_buf.data is nullptr, index=%u, mode_id:%u",
             data.first, model_id_);
      return ACL_ERROR_GE_PARAM_INVALID;
    }

    if (!CheckUserAndModelSize(static_cast<int64_t>(buffer.length), data.second.GetDataSize(), is_input, is_dynamic)) {
      GELOGE(ACL_ERROR_GE_PARAM_INVALID, "[Call][CheckInputAndModelSize] failed, op[%s]",
             data.second.GetOpName().c_str());
      return ACL_ERROR_GE_PARAM_INVALID;
    }

    void *const basic_addr = data.second.GetBasicAddr();
    const uint64_t data_size = static_cast<uint64_t>(data.second.GetDataSize());
    if (copy_only_addrs_.count(basic_addr) > 0U) {
      if (is_input && (buffer.length > 0U)) {
        GELOGI("[IMAS] Find addr %p need direct copy from user malloc input %p", basic_addr, buffer.data);
        GE_CHK_RT_RET(rtMemcpy(basic_addr, data_size, buffer.data, buffer.length, RT_MEMCPY_DEVICE_TO_DEVICE));
      }
      GELOGI("No need to exeucte zero copy task because this addr %p need direct copy.", basic_addr);
      continue;
    }

    UpdateZeroCopyTaskParam(data, buffer, batch_label, is_input);
  }

  return SUCCESS;
}

///
/// @ingroup ge
/// @brief Get DataInputer
/// @return model ID
///
void DavinciModel::SetId(const uint32_t model_id) {
  model_id_ = model_id;
  bin_kernel_handle_.SetModelId(model_id);
}

///
/// @ingroup ge
/// @brief get unique identification for op when load two or more models
/// @param [in] const OpDescPtr: current op.
/// @param [in] std::string identification: unique identification for current op.
/// @return SUCCESS handle successfully / others handle failed
///
void DavinciModel::GetUniqueId(const OpDescPtr &op_desc, std::string &unique_identification) const {
  bin_kernel_handle_.GetUniqueId(op_desc, unique_identification);
}

///
/// @ingroup ge
/// @brief For TVM Op, avoid Addr Reuse.
/// @return void*
///
const char_t *DavinciModel::GetRegisterStub(const std::string &binfile, const std::string &session_graph_id) {
  return bin_kernel_handle_.GetRegisterStub(binfile, session_graph_id);
}

///
/// @ingroup ge
/// @brief Constant Op Init.
/// @return Status
///
Status DavinciModel::InitConstant(const OpDescPtr &op_desc) {
  const auto v_weights = ModelUtils::GetWeights(op_desc);
  const auto v_output_size = ModelUtils::GetOutputSize(op_desc);
  const auto v_output_addr = ModelUtils::GetOutputAddrs(runtime_param_, op_desc);
  if (v_weights.empty() || v_output_size.empty() || v_output_addr.empty()) {
    REPORT_INNER_ERROR("E19999", "weight.size:%zu size.size:%zu addr.size:%zu in Node:%s has empty, check invalid",
        v_weights.size(), v_output_size.size(), v_output_addr.size(), op_desc->GetName().c_str());
    GELOGE(PARAM_INVALID, "const op:%s not set output", op_desc->GetName().c_str());
    return PARAM_INVALID;
  }

  const GeTensor *const tensor = v_weights[0U].get();
  if (static_cast<size_t>(v_output_size[0U]) < tensor->GetData().size()) {
    REPORT_INNER_ERROR("E19999", "Output size:%ld < weight size:%zu in op:%s(%s) model_id:%u, check invalid",
        v_output_size[0U], tensor->GetData().size(), op_desc->GetName().c_str(), op_desc->GetType().c_str(), model_id_);
    GELOGE(PARAM_INVALID, "[Check][Param] Output size:%ld < weight size:%zu in op:%s(%s), model_id:%u",
        v_output_size[0U], tensor->GetData().size(), op_desc->GetName().c_str(), op_desc->GetType().c_str(), model_id_);
    return PARAM_INVALID;
  }

  if (tensor->GetData().size() == 0U) {
    GELOGW("const op:%s has no weight data.", op_desc->GetName().c_str());
    return SUCCESS;
  }

  const auto &desc = tensor->GetTensorDesc();
  if (desc.GetDataType() == DT_STRING) {
    const auto &cst_data = tensor->GetData();
    GE_CHECK_NOTNULL(cst_data.data());
    /// if tensor is a scaler, it's shape size if zero, according ge_tensor.cc.
    /// the logic of GetShapeSize is wrong, the scaler tensor's GetShapeSize is zero
    /// and that of unknown shape is zero too.
    /// unknown shape will not appear here, so we can use zero judge a tensor is scaler or not
    const GeShape &tensor_shape = desc.GetShape();
    int64_t shape_size = tensor_shape.GetShapeSize();
    if (shape_size < 0) {
      GELOGE(FAILED, "[%s] shape size:%ld is invalid", op_desc->GetName().c_str(), shape_size);
      return FAILED;
    }

    shape_size = ((shape_size == 0) && tensor_shape.GetDims().empty()) ? 1 : shape_size;
    if (CheckInt64Uint32MulOverflow(shape_size, kStringBytes * kStringHeadElems) != SUCCESS) {
      GELOGE(FAILED, "[Call][CheckInt64Uint32MulOverflow] Shape size:%ld is invalid", shape_size);
      return FAILED;
    }

    const size_t elem_num = static_cast<size_t>(shape_size);
    const size_t offset = (elem_num * kStringBytes) * kStringHeadElems;

    std::vector<uint64_t> vct_data((cst_data.size() / sizeof(uint64_t)) + 1U, 0U);
    const auto cpy_rslt = memcpy_s(vct_data.data(), vct_data.size() * sizeof(uint64_t),
                                   cst_data.data(), cst_data.size());
    if (cpy_rslt != EOK) {
      GELOGE(FAILED, "[Call][memcpy_s] memcpy failed, size:%lu, result: %d", cst_data.size(), cpy_rslt);
      return FAILED;
    }

    const uint64_t logic_base = vct_data[0U];
    for (size_t i = 0U; (i < elem_num) && ((i * kStringHeadElems) < vct_data.size()); ++i) {
      vct_data[i * kStringHeadElems] = (vct_data[i * kStringHeadElems] - logic_base) + offset;
    }

    // Use real size cst_data.size() when copy.
    GELOGI("[IMAS]InitConstant memcpy graph_%u type[V] name[%s] output[0] memaddr[%p] mem_size[%lu] datasize[%zu]",
           runtime_param_.graph_id, op_desc->GetName().c_str(), v_output_addr[0U], v_output_size[0U], cst_data.size());
    GE_CHK_RT_RET(rtMemcpy(v_output_addr[0U], static_cast<uint64_t>(v_output_size[0U]),
                           vct_data.data(), cst_data.size(), RT_MEMCPY_HOST_TO_DEVICE));
  } else {
    GELOGI("[IMAS]InitConstant memcpy graph_%u type[V] name[%s] output[%d] memaddr[%p] mem_size[%lu] datasize[%zu]",
           runtime_param_.graph_id, op_desc->GetName().c_str(), 0, v_output_addr[0U], v_output_size[0U],
           tensor->GetData().size());
    GE_CHK_RT_RET(rtMemcpy(v_output_addr[0U], static_cast<uint64_t>(v_output_size[0U]),
                           tensor->GetData().data(), tensor->GetData().size(), RT_MEMCPY_HOST_TO_DEVICE));
  }

  return SUCCESS;
}

///
/// @ingroup ge
/// @brief Constant Op Init.
/// @return Status
///
Status DavinciModel::InitFileConstant(const NodePtr &node) {
  const auto &op_desc = node->GetOpDesc();
  const auto &tensor_desc = op_desc->GetOutputDescPtr(0U);
  GE_CHECK_NOTNULL(tensor_desc);
  if (VarManager::Instance(session_id_)->IsVarReady(node->GetName(), *tensor_desc)) {
    GELOGI("file constant op:%s is ready", node->GetName().c_str());
    return SUCCESS;
  }
  const auto v_output_size = ModelUtils::GetOutputSize(op_desc);
  const auto v_output_addr = ModelUtils::GetOutputAddrs(runtime_param_, op_desc);
  if (v_output_size.empty() || v_output_addr.empty()) {
    REPORT_INNER_ERROR("E19999", "output_length.size:%zu output_addr.size:%zu in "
                       "op:%s(%s) has empty, model_id:%u, check invalid",
                       v_output_size.size(), v_output_addr.size(),
                       op_desc->GetName().c_str(), op_desc->GetType().c_str(), model_id_);
    GELOGE(PARAM_INVALID, "const op:%s not set output", op_desc->GetName().c_str());
    return PARAM_INVALID;
  }

  int64_t weight_size = 0;
  GE_CHK_STATUS_RET(TensorUtils::GetTensorSizeInBytes(*tensor_desc, weight_size), "Failed to get file constant size.");

  if (v_output_size[0U] < weight_size) {
    REPORT_INNER_ERROR("E19999", "Output size:%ld < weight size:%zu in op:%s(%s) model_id:%u, check invalid",
                       v_output_size[0U], weight_size,
                       op_desc->GetName().c_str(), op_desc->GetType().c_str(), model_id_);
    GELOGE(PARAM_INVALID, "[Check][Param] Output size:%ld < weight size:%zu in op:%s(%s), model_id:%u",
           v_output_size[0U], weight_size,
           op_desc->GetName().c_str(), op_desc->GetType().c_str(), model_id_);
    return PARAM_INVALID;
  }

  if (weight_size == 0) {
    GELOGW("const op:%s has no weight data.", op_desc->GetName().c_str());
    return SUCCESS;
  }

  GELOGI("[IMAS]InitConstant memcpy graph_%u type[V] name[%s] output[%d] memaddr[%p] mem_size[%ld] datasize[%ld]",
         runtime_param_.graph_id, op_desc->GetName().c_str(), 0, v_output_addr[0U], v_output_size[0U], weight_size);
  std::string file_path;
  GE_CHK_STATUS_RET(GetFilePath(op_desc, file_id_and_path_map_, file_path), "Failed to get file path.");
  size_t left_size = static_cast<size_t>(v_output_size[0U]);
  GE_CHK_STATUS_RET(CopyOneWeightFromFile(v_output_addr[0U], file_path, static_cast<size_t>(weight_size), left_size),
                    "Failed to copy data to file constant.");
  VarManager::Instance(session_id_)->SetVarIsReady(node->GetName(), *tensor_desc);
  GELOGI("Finish to copy data to device memory of file constant.");
  return SUCCESS;
}

CustAICPUKernelPtr DavinciModel::GetCustAICPUKernel(const OpDescPtr &op_desc) const {
  const CustAICPUKernelPtr aicpu_kernel = op_desc->TryGetExtAttr(OP_EXTATTR_CUSTAICPU_KERNEL, CustAICPUKernelPtr());
  if (aicpu_kernel != nullptr) {
    return aicpu_kernel;
  }

  // Called by TaskInfo::Init, ge_model_ always valid.
  return ge_model_->GetCustAICPUKernelStore().FindKernel(op_desc->GetName());
}

///
/// @ingroup ge
/// @brief TVM Op Init.
/// @return Status
///
Status DavinciModel::InitTbeHandle(const OpDescPtr &op_desc) {
  if (!IsTbeTask(op_desc)) {
    return SUCCESS;
  }
  uint32_t thread_mode = kInValidThreadMode;
  (void)AttrUtils::GetInt(op_desc, ATTR_NAME_THREAD_MODE, thread_mode);
  // ffts mode only has ATTR_NAME_THREAD_SCOPE_ID attr.
  // ffts plus auto mode has ATTR_NAME_THREAD_SCOPE_ID attr and thread mode is 1.
  // ffts plus manual mode has ATTR_NAME_THREAD_SCOPE_ID attr and thread mode is 0.
  // normal mode do not have ATTR_NAME_THREAD_SCOPE_ID and ATTR_NAME_THREAD_MODE attr.
  // only ffts mode and ffts plus auto mode enter the branch.
  if (op_desc->HasAttr(ATTR_NAME_THREAD_SCOPE_ID) && (thread_mode != kManualThreadMode)) {
    return bin_kernel_handle_.RegisterHandle(op_desc);
  }
  std::string core_type;
  (void)AttrUtils::GetStr(op_desc, ATTR_NAME_CUBE_VECTOR_CORE_TYPE, core_type);
  if ((core_type == kMixCubeCoreTypeAic) || (core_type == kMixVectorCoreTypeAiv)) {
    GE_CHK_STATUS_RET_NOLOG(
        bin_kernel_handle_.RegisterHandle(op_desc, nullptr, kMixCubeTBEKernelPrefix, ge_model_->GetTBEKernelStore()));
    GE_CHK_STATUS_RET_NOLOG(
        bin_kernel_handle_.RegisterHandle(op_desc, nullptr, kMixVectorTBEKernelPrefix, ge_model_->GetTBEKernelStore()));
  } else {
    std::string kernel_name;
    const auto status = AttrUtils::GetStr(op_desc, ATTR_NAME_TBE_KERNEL_NAME_FOR_LOAD, kernel_name);
    const auto &kernel = ge_model_->GetTBEKernelStore().FindKernel(status ? kernel_name : op_desc->GetName());
    const std::string ext_kernel_name(OP_EXTATTR_NAME_TBE_KERNEL);
    const auto &tbe_kernel = (kernel != nullptr) ? kernel : op_desc->TryGetExtAttr(ext_kernel_name, TBEKernelPtr());
    if (tbe_kernel == nullptr) {
      REPORT_INNER_ERROR("E19999", "Get tbe_kernel for op:%s(%s) fail, model_id:%u",
                         op_desc->GetName().c_str(), op_desc->GetType().c_str(), model_id_);
      GELOGE(INTERNAL_ERROR, "[Check][Param] TBE: %s can't find tvm bin file!", op_desc->GetName().c_str());
      return INTERNAL_ERROR;
    }
    GE_CHK_STATUS_RET_NOLOG(bin_kernel_handle_.RegisterHandle(op_desc, tbe_kernel, ""));
  }

  return SUCCESS;
}

Status DavinciModel::GetAddrAndPrefCnt(const std::string &kernel_name, void *&addr, uint32_t &pref_cnt) {
  return bin_kernel_handle_.GetAddrAndPrefCnt(kernel_name, addr, pref_cnt);
}

///
/// @ingroup ge
/// @brief insert active_stream_indication_
/// @return Status
///
Status DavinciModel::InitStreamActive(const OpDescPtr &op_desc) {
  if (op_desc->HasAttr(ATTR_NAME_SWITCH_BRANCH_NODE_LABEL)) {
    std::vector<uint32_t> active_stream_list;
    if (!AttrUtils::GetListInt(op_desc, ATTR_NAME_ACTIVE_STREAM_LIST, active_stream_list)) {
      REPORT_INNER_ERROR("E19999", "[Get][Attr] active_stream_list in op:%s(%s) failed, model_id:%u.",
                         op_desc->GetName().c_str(), op_desc->GetType().c_str(), model_id_);
      GELOGE(INTERNAL_ERROR, "[Get][Attr] active_stream_list in op:%s(%s) failed, model_id:%u.",
             op_desc->GetName().c_str(), op_desc->GetType().c_str(), model_id_);
      return INTERNAL_ERROR;
    }

    for (size_t j = 0U; j < active_stream_list.size(); ++j) {
      (void)active_stream_indication_.insert(active_stream_list[j]);
      GELOGI("flowctrl_op_index_map  node:%s, active_stream_id=%u.", op_desc->GetName().c_str(), active_stream_list[j]);
    }
  }

  return SUCCESS;
}

Status DavinciModel::InitStreamSwitch(const OpDescPtr &op_desc) {
  std::vector<uint32_t> active_stream_list;
  GE_LOGI_IF(!AttrUtils::GetListInt(op_desc, ATTR_NAME_ACTIVE_STREAM_LIST, active_stream_list),
             "GetInt active_stream_list failed.");
  if (active_stream_list.size() != kTrueBranchStreamCount) {
    REPORT_INNER_ERROR("E19999", "[Check][Param] Attr: active_stream_list.size:%zu in op:%s(%s) != 1, model_id:%u, "
        "check invalid", active_stream_list.size(), op_desc->GetName().c_str(), op_desc->GetType().c_str(), model_id_);
    GELOGE(INTERNAL_ERROR, "[Check][Param] Attr: active_stream_list.size:%zu in op:%s(%s) != 1, model_id:%u",
           active_stream_list.size(), op_desc->GetName().c_str(), op_desc->GetType().c_str(), model_id_);
    return INTERNAL_ERROR;
  }

  const uint32_t true_stream_id = active_stream_list.front();
  (void)active_stream_indication_.insert(true_stream_id);
  GELOGI("flowctrl_op_index_map  node:%s, true_stream_id=%u.", op_desc->GetName().c_str(), true_stream_id);

  return SUCCESS;
}

Status DavinciModel::SetDynamicBatchInfo(const OpDescPtr &op_desc, const uint32_t batch_num) {
  batch_info_.clear();
  combined_batch_info_.clear();

  (void)AttrUtils::GetInt(op_desc, ATTR_DYNAMIC_TYPE, dynamic_type_);
  (void)AttrUtils::GetListStr(op_desc, ATTR_USER_DESIGNEATE_SHAPE_ORDER, user_designate_shape_order_);
  for (uint32_t i = 0U; i < batch_num; ++i) {
    std::vector<int64_t> batch_shape;
    const std::string attr_name = ATTR_NAME_PRED_VALUE + "_" + std::to_string(i);
    if (!AttrUtils::GetListInt(op_desc, attr_name, batch_shape)) {
      REPORT_INNER_ERROR("E19999", "Get Attr:%s from op:%s(%s) fail, model_id:%u", attr_name.c_str(),
                         op_desc->GetName().c_str(), op_desc->GetType().c_str(), model_id_);
      GELOGE(FAILED, "[Get][Attr] %s from op:%s(%s) fail, model_id:%u", attr_name.c_str(),
             op_desc->GetName().c_str(), op_desc->GetType().c_str(), model_id_);
      batch_info_.clear();
      return FAILED;
    }
    batch_info_.emplace_back(batch_shape);
    batch_shape.clear();
    const std::string attr_combined_batch = ATTR_NAME_COMBINED_BATCH + "_" + std::to_string(i);
    if (AttrUtils::GetListInt(op_desc, attr_combined_batch, batch_shape)) {
      combined_batch_info_.emplace_back(batch_shape);
    }
  }

  return SUCCESS;
}

Status DavinciModel::InitCase(const OpDescPtr &op_desc) {
  uint32_t batch_num = 0U;
  if (!AttrUtils::GetInt(op_desc, ATTR_NAME_BATCH_NUM, batch_num)) {
    GELOGI("Not multi-batch Node: %s", op_desc->GetName().c_str());
    return SUCCESS;
  }

  return SetDynamicBatchInfo(op_desc, batch_num);
}

///
/// @ingroup ge
/// @brief Init model stream for NN model.
/// @param [in] stream   user input model stream.
/// @return Status
///
Status DavinciModel::InitModelStream(const rtStream_t stream) {
  const ExecuteMode curr_mode = is_async_mode_ ? ExecuteMode::ASYNCHRONIZATION : ExecuteMode::SYNCHRONIZATION;
  GE_CHK_BOOL_RET_STATUS((curr_mode == last_execute_mode_) || (last_execute_mode_ == ExecuteMode::INITIALIZATION),
                         INTERNAL_ERROR, "[Check][Param] NnExecute not support mix execute.");
  last_execute_mode_ = curr_mode;

  // asynchronize mode, use user input stream.
  if (is_async_mode_) {
    rt_model_stream_ = stream;
    is_inner_model_stream_ = false;
    return SUCCESS;
  }

  // synchronize mode, use forbidden stream.
  if (stream != nullptr) {
    if ((rt_model_stream_ != nullptr) && is_inner_model_stream_) {
      GE_LOGW_IF(rtStreamDestroy(rt_model_stream_) != RT_ERROR_NONE, "Destroy rt_stream failed!");
    }

    rt_model_stream_ = stream;
    is_inner_model_stream_ = false;
    return SUCCESS;
  }

  if (rt_model_stream_ == nullptr) {
    GE_CHK_RT_RET(rtStreamCreateWithFlags(&rt_model_stream_, priority_, RT_STREAM_FORBIDDEN_DEFAULT));
    is_inner_model_stream_ = true;
  }

  return SUCCESS;
}

///
/// @ingroup ge
/// @brief ACL case, do not start  new thread, return execute result.
/// @param [in] stream   execute model stream.
/// @param [in] async_mode  is asynchronize mode.
/// @param [in] input_data  model input data.
/// @param [out] output_data  model output data.
///
Status DavinciModel::NnExecute(const rtStream_t stream, const bool async_mode, const InputData &input_data,
                               OutputData &output_data) {
  is_async_mode_ = async_mode;
  GELOGD("Model Run begin, model id:%u, data index:%u, flag:%d.",
         model_id_, input_data.index, static_cast<int32_t>(is_async_mode_));
  GE_CHK_STATUS_RET(InitModelStream(stream), "[Init][ModelStream] failed, model_id:%u.", model_id_);
  is_dynamic_ = input_data.is_dynamic_batch;

  auto &prof_mgr = ProfilingManager::Instance();
  const bool profiling_model_execute_on = prof_mgr.ProfilingModelExecuteOn();
  GE_IF_BOOL_EXEC(profiling_model_execute_on, SetProfileTime(ModelProcStage::MODEL_PRE_PROC_START));
  Status ret = CopyModelData(input_data, output_data, is_dynamic_);
  if (ret != SUCCESS) {
    GELOGE(ret, "[Copy][ModelData] failed. model id: %u", model_id_);
    return ret;
  }

  GELOGD("current_data.index=%u", input_data.index);
  GE_IF_BOOL_EXEC(profiling_model_execute_on, SetProfileTime(ModelProcStage::MODEL_PRE_PROC_END));

  if (!task_list_.empty()) {
    const uint64_t index_id = iterator_count_ + 1U;
    // tag_id 0 means step begin, 1 meas step end.
    GE_CHK_STATUS_RET_NOLOG(prof_mgr.ProfileStepInfo(index_id, model_id_, 0U, rt_model_stream_, device_id_));

    GELOGD("rtModelExecute do, model id:%u.", model_id_);
    GE_IF_BOOL_EXEC(profiling_model_execute_on, SetProfileTime(ModelProcStage::MODEL_INFER_START));
    const rtError_t rt_ret = rtModelExecute(rt_model_handle_, rt_model_stream_, 0U);
    GE_CHK_RT_EXEC(rt_ret, return RT_ERROR_TO_GE_STATUS(rt_ret));
    GE_IF_BOOL_EXEC(profiling_model_execute_on, SetProfileTime(ModelProcStage::MODEL_INFER_END));
    GELOGD("rtModelExecute end, model id:%u.", model_id_);

    GE_CHK_STATUS_RET_NOLOG(prof_mgr.ProfileStepInfo(index_id, model_id_, 1U, rt_model_stream_, device_id_));
    iterator_count_++;
  }

  if ((prof_mgr.ProfilingModelLoadOn() || prof_mgr.ProfilingSubscribeOn()) && is_inner_model_stream_) {
    GE_CHK_RT_EXEC(rtStreamSynchronize(rt_model_stream_), return FAILED);
  }

  GE_IF_BOOL_EXEC(profiling_model_execute_on, SetProfileTime(ModelProcStage::MODEL_AFTER_PROC_START));
  output_data.index = input_data.index;
  output_data.model_id = model_id_;
  ret = CopyOutputData(output_data, RT_MEMCPY_DEVICE_TO_DEVICE);
  if (ret != SUCCESS) {
    GELOGE(ret, "[Copy][OutputData] to user failed, model_id:%u.", model_id_);
    return ACL_ERROR_GE_INTERNAL_ERROR;
  }
  GE_IF_BOOL_EXEC(profiling_model_execute_on, SetProfileTime(ModelProcStage::MODEL_AFTER_PROC_END));

  // report model time data
  GE_IF_BOOL_EXEC(profiling_model_execute_on, SinkTimeProfile(input_data.index, input_data.request_id));
  GELOGD("Model run end, model id:%u", model_id_);
  return SUCCESS;
}

// Add active entry stream for special env.
Status DavinciModel::AddHeadStream() {
  if (active_stream_list_.empty()) {
    REPORT_INNER_ERROR("E19999", "active_stream_list is empty in model:%u, check invalid", model_id_);
    GELOGE(INTERNAL_ERROR, "[Check][Param] active_stream_list is empty in model:%u, check invalid", model_id_);
    return INTERNAL_ERROR;
  }

  if (active_stream_list_.size() == 1U) {
    GELOGI("Just one active stream, take as head stream.");
    rt_head_stream_ = active_stream_list_[0U];
    is_pure_head_stream_ = false;
  } else {
    // Create stream which rt_model_handel running on, this is S0, TS stream.
    GELOGI("Multiple active stream: %zu, create head stream.", active_stream_list_.size());
    GE_CHK_RT_RET(rtStreamCreateWithFlags(&rt_head_stream_, priority_, RT_STREAM_PERSISTENT));
    GE_CHK_RT_RET(rtModelBindStream(rt_model_handle_, rt_head_stream_, static_cast<uint32_t>(RT_INVALID_FLAG)));
    is_pure_head_stream_ = true;

    for (const auto &s : active_stream_list_) {
      const auto active_entry = MakeShared<CpuTaskActiveEntry>(rt_head_stream_);
      GE_CHECK_NOTNULL(active_entry);

      const Status status = active_entry->Init(s);
      if (status != SUCCESS) {
        return status;
      }

      cpu_task_list_.emplace_back(active_entry);
    }
  }

  // Create entry stream active head stream. AICPU stream.
  GE_CHK_RT_RET(rtStreamCreateWithFlags(&rt_entry_stream_, priority_, RT_STREAM_AICPU));
  GE_CHK_RT_RET(rtModelBindStream(rt_model_handle_, rt_entry_stream_, static_cast<uint32_t>(RT_HEAD_STREAM)));
  return SUCCESS;
}

Status DavinciModel::InitEntryTask() {
  if (deploy_type_ == AICPU_DEPLOY_CROSS_THREAD) {
    GE_CHK_STATUS_RET(AddHeadStream(), "[Add][HeadStream] failed.");
    return CpuActiveStream();
  } else {
    return LoadWithQueue();
  }
}

uint8_t *DavinciModel::MallocFeatureMapMem(const size_t data_size) const {
  uint8_t *temp_mem_base = nullptr;
  auto &mem_instance = MemManager::Instance().MemInstance(RT_MEMORY_HBM);
  const std::string purpose("feature map,used for op input and output.");
  char_t static_mem_env[MMPA_MAX_PATH]{};
  size_t mem_size = data_size;
  const auto env_res = mmGetEnv(&kEnvGeuseStaticMemory[0], &static_mem_env[0], static_cast<uint32_t>(MMPA_MAX_PATH));
  if (env_res == EN_OK) {
    mem_size = static_cast<size_t>(VarManager::Instance(session_id_)->GetGraphMemoryMaxSize());
    if (mem_size < data_size) {
      return nullptr;
    }
    temp_mem_base = mem_instance.MallocMemory(purpose, kFeatureMemoryKey, mem_size, GetDeviceId());
  } else if (ExecutorUtils::IsGeUseExtendSizeStaticMemory()) {
    auto &mem_allocator = SessionMemAllocator::Instance().GetMemAllocator(session_id_);
    temp_mem_base = mem_allocator.MallocMemory(purpose, kFeatureMemoryKey, mem_size, GetDeviceId());
    const int64_t malloced_memory_size = mem_allocator.MemorySize(kFeatureMemoryKey, GetDeviceId());
    if (static_cast<size_t>(malloced_memory_size) < mem_size) {
      REPORT_CALL_ERROR("E19999", "Malloc feature map memory fail. malloced_memory_size[%ld] < mem_size[%zu],"
                        " device_id[%u]", malloced_memory_size, mem_size, GetDeviceId());
      GELOGE(ge::INTERNAL_ERROR, "Malloc feature map memory fail. malloced_memory_size[%ld] < mem_size[%zu],"
                                 " device_id[%u]", malloced_memory_size, mem_size, GetDeviceId());
      if (mem_allocator.FreeMemory(kFeatureMemoryKey, GetDeviceId()) != SUCCESS) {
        GELOGE(ge::INTERNAL_ERROR, "Free feature map memory fail. device_id[%u]", GetDeviceId());
      }
      return nullptr;
    }
  } else {
    temp_mem_base = mem_instance.MallocMemory(purpose, mem_size, GetDeviceId());
  }

  if (temp_mem_base != nullptr) {
    GE_CHK_RT(rtMemset(temp_mem_base, mem_size, 0U, data_size));
  }
  return temp_mem_base;
}

Status DavinciModel::MallocExMem() {
  char_t static_mem_env[MMPA_MAX_PATH]{};
  const auto env_res = mmGetEnv(&kEnvGeuseStaticMemory[0], &static_mem_env[0], static_cast<uint32_t>(MMPA_MAX_PATH));
  for (auto &it : runtime_param_.memory_infos) {
    const size_t mem_size = static_cast<size_t>(it.second.memory_size);
    if (mem_size == 0U) {
      continue;
    }
    const bool sessoion_scope = ((kSessionScopeMemory & it.first) == kSessionScopeMemory);
    rtMemType_t memory_type = static_cast<rtMemType_t>(it.first & kMemoryTypeMask);
    const std::string purpose("p2p memory, used for some op related to hcom or session scope memory");
    if (it.second.memory_type == RT_MEMORY_HOST) {
      memory_type = it.second.memory_type;
      it.second.memory_base = PtrToPtr<void, uint8_t>(MemManager::Instance().HostMemInstance().Malloc(mem_size));
      runtime_param_.host_mem_base = PtrToValue(it.second.memory_base);
    } else if (sessoion_scope) {
      auto &mem_instance = MemManager::Instance().SessionScopeMemInstance(memory_type);
      it.second.memory_base = mem_instance.Malloc(mem_size, runtime_param_.session_id);
    } else if (env_res == EN_OK) {
      const std::string memory_key = std::to_string(0) + it.second.memory_key;
      auto &mem_instance = MemManager::Instance().MemInstance(memory_type);
      it.second.memory_base = mem_instance.MallocMemory(purpose, memory_key, mem_size, GetDeviceId());
    } else {
      auto &mem_instance = MemManager::Instance().MemInstance(memory_type);
      it.second.memory_base = mem_instance.MallocMemory(purpose, mem_size, GetDeviceId());
    }

    if (it.second.memory_base == nullptr) {
      REPORT_CALL_ERROR("E19999", "MallocExMem fail, type:%u size:%zu, model_id:%u, check invalid",
                        memory_type, mem_size, model_id_);
      GELOGE(ACL_ERROR_GE_MEMORY_ALLOCATION, "Alloc ex memory failed, type:%u size: %zu", memory_type, mem_size);
      return ACL_ERROR_GE_MEMORY_ALLOCATION;
    }

    GELOGI("InitFeatureMap graph_%u MallocMemory type[F] mem_type[%u] mem_addr[%p] mem_size[%zu]",
           runtime_param_.graph_id, memory_type, it.second.memory_base, mem_size);
  }
  return SUCCESS;
}

uint8_t *DavinciModel::MallocWeightsMem(const size_t weights_size) const {
  auto &mem_instance = MemManager::Instance().MemInstance(RT_MEMORY_HBM);
  const std::string purpose("weights memory in inference network.");
  char_t static_mem_env[MMPA_MAX_PATH]{};
  const auto env_res = mmGetEnv(&kEnvGeuseStaticMemory[0], &static_mem_env[0], static_cast<uint32_t>(MMPA_MAX_PATH));
  if (env_res == EN_OK) {
    const std::string weight_memory_key = std::to_string(0) + "_w";
    return mem_instance.MallocMemory(purpose, weight_memory_key, weights_size, GetDeviceId());
  } else {
    return mem_instance.MallocMemory(purpose, weights_size, GetDeviceId());
  }
}

void DavinciModel::FreeFeatureMapMem() {
  if ((mem_base_ == 0U) || (!is_inner_mem_base_)) {
    return;
  }

  auto &mem_instance = MemManager::Instance().MemInstance(RT_MEMORY_HBM);
  char_t static_mem_env[MMPA_MAX_PATH]{};
  const auto env_res = mmGetEnv(&kEnvGeuseStaticMemory[0], &static_mem_env[0], static_cast<uint32_t>(MMPA_MAX_PATH));
  if (env_res == EN_OK) {
    if (mem_instance.GetMemoryAddr(kFeatureMemoryKey, GetDeviceId()) != nullptr) {
      GE_CHK_STATUS(mem_instance.FreeMemory(kFeatureMemoryKey, GetDeviceId()), "failed to free FeatureMap");
    }
  } else if (ExecutorUtils::IsGeUseExtendSizeStaticMemory()) {
    auto &mem_allocator = SessionMemAllocator::Instance().GetMemAllocator(session_id_);
    if (mem_allocator.GetMemoryAddr(kFeatureMemoryKey, GetDeviceId()) != nullptr) {
      GE_CHK_STATUS(mem_allocator.FreeMemory(kFeatureMemoryKey, GetDeviceId()), "failed to free FeatureMap");
    }
  } else {
    GE_CHK_STATUS(mem_instance.FreeMemory(ValueToPtr(mem_base_), GetDeviceId()), "failed to free FeatureMap");
  }
  mem_base_ = 0U;
}

void DavinciModel::FreeExMem() {
  char_t static_mem_env[MMPA_MAX_PATH]{};
  const auto env_res = mmGetEnv(&kEnvGeuseStaticMemory[0], &static_mem_env[0], static_cast<uint32_t>(MMPA_MAX_PATH));
  for (auto &it : runtime_param_.memory_infos) {
    // free when session destory
    if ((kSessionScopeMemory & it.first) == kSessionScopeMemory) {
      continue;
    }
    if (it.second.memory_type == RT_MEMORY_HOST) {
      if (it.second.memory_base != nullptr) {
        MemManager::Instance().HostMemInstance().Free(PtrToPtr<uint8_t, void>(it.second.memory_base));
        it.second.memory_base = nullptr;
        continue;
      }
    }
    const auto memory_type = static_cast<rtMemType_t>(it.first & kMemoryTypeMask);
    auto &mem_instance = MemManager::Instance().MemInstance(memory_type);
    if (env_res == EN_OK) {
      const std::string memory_key = std::to_string(0) + it.second.memory_key;
      if (mem_instance.GetMemoryAddr(memory_key, GetDeviceId()) != nullptr) {
        GE_CHK_STATUS(mem_instance.FreeMemory(memory_key, GetDeviceId()), "failed to free memory");
      }
      it.second.memory_base = nullptr;
    } else {
      if (it.second.memory_base != nullptr) {
        GE_CHK_STATUS(mem_instance.FreeMemory(it.second.memory_base, GetDeviceId()), "failed to free memory");
        it.second.memory_base = nullptr;
      }
    }
  }
}

void DavinciModel::FreeWeightsMem() {
  auto &mem_instance = MemManager::Instance().MemInstance(RT_MEMORY_HBM);
  char_t static_mem_env[MMPA_MAX_PATH]{};
  const auto env_res = mmGetEnv(&kEnvGeuseStaticMemory[0], &static_mem_env[0], static_cast<uint32_t>(MMPA_MAX_PATH));
  if (env_res == EN_OK) {
    const std::string memory_key = std::to_string(0) + "_w";
    if (mem_instance.GetMemoryAddr(memory_key, GetDeviceId()) != nullptr) {
      GE_CHK_STATUS(mem_instance.FreeMemory(memory_key, GetDeviceId()), "failed to free Weight");
    }
    weights_mem_base_ = 0U;
  } else {
    if ((weights_mem_base_ != 0U) && (weights_mem_base_ != mem_base_) && is_inner_weight_base_) {
      GE_CHK_STATUS(mem_instance.FreeMemory(ValueToPtr(weights_mem_base_), GetDeviceId()), "failed to free Weight");
      weights_mem_base_ = 0U;
    }
  }
}

Status DavinciModel::TransAllVarData(const ComputeGraphPtr &graph, const std::vector<NodePtr> &variable_nodes) const {
  const uint32_t device_id = GetDeviceId();
  GE_CHK_STATUS_RET(TransVarDataUtils::TransAllVarData(variable_nodes, session_id_, runtime_param_.graph_id, device_id),
                    "[Call][TransAllVarData] failed, graph:%s, session_id:%lu, graph_id:%u, device_id:%u",
                    graph->GetName().c_str(), session_id_, runtime_param_.graph_id, device_id);

  GE_CHK_STATUS_RET(TransVarDataUtils::CopyVarData(graph, variable_nodes, session_id_, device_id),
                    "[Copy][CopyVarData] failed, graph:%s, session_id:%lu, graph_id:%u, device_id:%u",
                    graph->GetName().c_str(), session_id_, runtime_param_.graph_id, device_id);
  return SUCCESS;
}

void DavinciModel::SetDataDumperArgs(const ComputeGraphPtr &graph,
                                     const std::map<std::string, OpDescPtr> &variable_by_name) {
  if (dump_model_name_.empty()) {
    dump_model_name_ = name_;
  }
  data_dumper_.SetModelName(dump_model_name_);
  data_dumper_.SetModelId(model_id_);
  data_dumper_.SetOmName(om_name_);
  data_dumper_.SetComputeGraph(graph);
  data_dumper_.SetRefInfo(saved_task_addrs_);

  int32_t tmp_device_id = -1;
  GE_CHK_RT_EXEC(rtGetDevice(&tmp_device_id), return);
  data_dumper_.SetDeviceId(static_cast<uint32_t>(tmp_device_id));

  const auto get_var_addr = [&](const std::string &var_name) -> uintptr_t {
    const auto it = variable_by_name.find(var_name);
    if (it != variable_by_name.end()) {
      const auto output_sizes = ModelUtils::GetOutputSize(it->second);
      const auto output_addrs = ModelUtils::GetOutputAddrs(runtime_param_, it->second);
      if (output_sizes.empty() || output_addrs.empty()) {
        return 0U;
      }
      return PtrToValue(output_addrs[0U]);
    }
    GELOGD("op: %s is null.", var_name.c_str());
    return 0U;
  };

  if (known_node_) {
    data_dumper_.SetLoopAddr(global_step_addr_, 0U, 0U);
  } else {
    // set loop count addr
    data_dumper_.SetLoopAddr(get_var_addr(NODE_NAME_GLOBAL_STEP),
                             get_var_addr(NODE_NAME_FLOWCTRL_LOOP_PER_ITER),
                             get_var_addr(NODE_NAME_FLOWCTRL_LOOP_COND));
  }
}

uint32_t DavinciModel::GetFlowctrlIndex(const uint32_t op_index) {
  const std::lock_guard<std::mutex> lk(flowctrl_op_index_internal_map_mutex_);
  ++flowctrl_op_index_internal_map_[op_index];
  return (flowctrl_op_index_internal_map_[op_index]) - 1U;
}

void DavinciModel::PushHcclStream(const rtStream_t hccl_stream) {
  const std::lock_guard<std::mutex> lk(all_hccl_stream_list_mutex_);
  all_hccl_stream_list_.push_back(hccl_stream);
}

void DavinciModel::SaveHcclFollowStream(const int64_t main_stream_id, rtStream_t stream) {
  const std::lock_guard<std::mutex> lk(capacity_of_stream_mutex_);
  main_follow_stream_mapping_[main_stream_id].emplace_back(stream);
}

void DavinciModel::SetTotalFixedAddrsSize(const std::string &tensor_name, const int64_t fix_addr_size) {
  if (tensor_name_to_fixed_addr_size_.find(tensor_name) == tensor_name_to_fixed_addr_size_.end()) {
    tensor_name_to_fixed_addr_size_[tensor_name] = fixed_addr_size_;
    fixed_addr_size_ += fix_addr_size;
  }
}

Status DavinciModel::InitOrigInputInfo(const uint32_t index, const OpDescPtr &op_desc) {
  if ((!op_desc->HasAttr(ATTR_NAME_AIPP_INPUTS)) || (!op_desc->HasAttr(ATTR_NAME_AIPP_OUTPUTS))) {
    GELOGI("there is not AIPP related with index %u, node: %s.", index, op_desc->GetName().c_str());
    return SUCCESS;
  }

  std::vector<std::string> inputs;
  if (AttrUtils::GetListStr(op_desc, ATTR_NAME_AIPP_INPUTS, inputs) && (!inputs.empty())) {
    const std::string &input = inputs[kAippOriginInputIndex];
    GELOGI("origin input str: %s.", input.c_str());
    const std::vector<std::string> infos = StringUtils::Split(input, ':');
    if (infos.size() != kAippInfoNum) {
      REPORT_INNER_ERROR("E19999", "Attr:%s in op:%s(%s), aipp input size:%zu != kAippInfoNum:%u, model_id:%u, "
                         "check invalid", ATTR_NAME_AIPP_INPUTS.c_str(),
                         op_desc->GetName().c_str(), op_desc->GetType().c_str(), infos.size(), kAippInfoNum,
                         model_id_);
      GELOGE(ACL_ERROR_GE_AIPP_MODE_INVALID, "[Check][Param] Attr:%s in op:%s(%s), "
             "aipp input size:%zu != kAippInfoNum:%u, model_id:%u", ATTR_NAME_AIPP_INPUTS.c_str(),
             op_desc->GetName().c_str(), op_desc->GetType().c_str(), infos.size(), kAippInfoNum, model_id_);
      return ACL_ERROR_GE_AIPP_MODE_INVALID;
    }

    OriginInputInfo input_info;
    input_info.format = TypeUtils::SerialStringToFormat(infos[kAippInfoFormat]);
    input_info.data_type = TypeUtils::SerialStringToDataType(infos[kAippInfoDataType]);
    input_info.dim_num = static_cast<uint32_t>(std::strtol(infos[kAippInfoDimNum].c_str(), nullptr, kDecimalRadix));
    orig_input_info_[index] = input_info;
  } else {
    orig_input_info_[index] = { FORMAT_RESERVED, DT_UNDEFINED, 0U };
  }

  return SUCCESS;
}

Status DavinciModel::GetOrigInputInfo(const uint32_t index, OriginInputInfo &orig_input_info) const {
  const auto it = orig_input_info_.find(index);
  if (it == orig_input_info_.end()) {
    REPORT_INNER_ERROR("E19999", "Get index:%u from orig_input_info_ fail, model_id:%u", index, model_id_);
    GELOGE(ACL_ERROR_GE_AIPP_NOT_EXIST, "[Check][Param] Get index:%u from orig_input_info_ fail, model_id:%u",
           index, model_id_);
    return ACL_ERROR_GE_AIPP_NOT_EXIST;
  }

  const OriginInputInfo &input_info = it->second;
  if ((input_info.format != FORMAT_RESERVED) || (input_info.data_type != DT_UNDEFINED)) {
    orig_input_info = input_info;
  }

  return SUCCESS;
}

void DavinciModel::ParseAIPPInfo(const std::string in_out_info, InputOutputDims &dims_info) const {
  GELOGI("ParseAIPPInfo: origin str: %s", in_out_info.c_str());
  const std::vector<std::string> infos = StringUtils::Split(in_out_info, ':');
  if (infos.size() != kAippInfoNum) {
    REPORT_INNER_ERROR("E19999", "in_out_info:%s size:%zu != kAippInfoNum:%u, model_id:%u, "
                       "check invalid", in_out_info.c_str(), infos.size(), kAippInfoNum,
                       model_id_);
    GELOGE(ACL_ERROR_GE_AIPP_MODE_INVALID, "[Check][Param] in_out_info:%s size:%zu != kAippInfoNum:%u, model_id:%u",
           in_out_info.c_str(), infos.size(), kAippInfoNum, model_id_);
    return;
  }
  dims_info.name = infos[kAippInfoTensorName];
  dims_info.size = static_cast<uint32_t>(std::strtol(infos[kAippInfoTensorSize].c_str(), nullptr, kDecimalRadix));
  dims_info.dim_num = static_cast<size_t>(std::strtol(infos[kAippInfoDimNum].c_str(), nullptr, kDecimalRadix));

  const std::vector<std::string> dims = StringUtils::Split(infos[kAippInfoShape], ',');
  for (const auto &dim : dims) {
    if (dim.empty()) {
      continue;
    }
    dims_info.dims.emplace_back(std::strtol(dim.c_str(), nullptr, kDecimalRadix));
  }
}

Status DavinciModel::InitAippInputOutputDims(const uint32_t index, const OpDescPtr &op_desc) {
  if ((!op_desc->HasAttr(ATTR_NAME_AIPP_INPUTS)) || (!op_desc->HasAttr(ATTR_NAME_AIPP_OUTPUTS))) {
    GELOGI("There is not AIPP related with index %u.", index);
    return SUCCESS;
  }

  std::vector<std::string> inputs;
  std::vector<InputOutputDims> input_dims;
  if (AttrUtils::GetListStr(op_desc, ATTR_NAME_AIPP_INPUTS, inputs) && (!inputs.empty())) {
    GELOGI("Data: %s has %zu related aippInfo.", op_desc->GetName().c_str(), inputs.size());
    for (const auto &it : inputs) {
      InputOutputDims input_info;
      ParseAIPPInfo(it, input_info);
      input_dims.emplace_back(input_info);
      GELOGD("Aipp origin input dims info: %s", it.c_str());

      const auto data_input_desc = op_desc->GetInputDescPtr(kDataIndex);
      int64_t data_input_size;
      (void)TensorUtils::GetSize(*(op_desc->GetInputDescPtr(kDataIndex)), data_input_size);
      GELOGD("Related Data[%d]: tensor_name: %s, dim_num: %zu, tensor_size: %zu, format: %s, data_type: %s, shape: %s.",
          index, op_desc->GetName().c_str(), data_input_desc->GetShape().GetDimNum(), data_input_size,
          TypeUtils::FormatToSerialString(data_input_desc->GetFormat()).c_str(),
          TypeUtils::DataTypeToSerialString(data_input_desc->GetDataType()).c_str(),
          ToString(data_input_desc->GetShape().GetDims()).c_str());
    }
  }

  std::vector<std::string> outputs;
  std::vector<InputOutputDims> output_dims;
  if (AttrUtils::GetListStr(op_desc, ATTR_NAME_AIPP_OUTPUTS, outputs) && (!outputs.empty())) {
    for (const auto &it : outputs) {
      InputOutputDims output_info;
      ParseAIPPInfo(it, output_info);
      output_dims.emplace_back(output_info);
      GELOGD("Aipp output dims info: %s", it.c_str());
    }
  }

  aipp_dims_info_[index] = { input_dims, output_dims };
  return SUCCESS;
}

Status DavinciModel::GetAllAippInputOutputDims(const uint32_t index, std::vector<InputOutputDims> &input_dims,
                                               std::vector<InputOutputDims> &output_dims) const {
  const std::map<uint32_t, std::pair<std::vector<InputOutputDims>, std::vector<InputOutputDims>>>::const_iterator it =
      aipp_dims_info_.find(index);
  if (it == aipp_dims_info_.cend()) {
    REPORT_INNER_ERROR("E19999", "Get index:%u from aipp_dims_info_ fail, model_id:%u", index, model_id_);
    GELOGE(ACL_ERROR_GE_AIPP_NOT_EXIST, "[Check][Param] Get index:%u from aipp_dims_info_ fail, model_id:%u",
           index, model_id_);
    return ACL_ERROR_GE_AIPP_NOT_EXIST;
  }

  input_dims = it->second.first;
  output_dims = it->second.second;
  return SUCCESS;
}

int64_t DavinciModel::GetFixedAddrsSize(const std::string &tensor_name) {
  const std::map<std::string, int64_t>::const_iterator it = tensor_name_to_fixed_addr_size_.find(tensor_name);
  return (it != tensor_name_to_fixed_addr_size_.cend()) ? it->second : fixed_addr_size_;
}

Status DavinciModel::InitL1DataDumperArgs() {
  if (ModelNeedDump()) {
    // malloc 2M for dump l1fusion op
    GE_CHK_RT_RET(rtMalloc(&l1_fusion_addr_, kModelL1FusionOpMByteSize, RT_MEMORY_HBM));

    // send l1fusion dump addr to rts
    if (rtDumpAddrSet(rt_model_handle_, l1_fusion_addr_, kModelL1FusionOpMByteSize, kModelFlagOfL1Fusion) !=
        RT_ERROR_NONE) {
      // l1_fusion_addr_ will be free when DavinciModel destruct
      REPORT_CALL_ERROR("E19999", "Call rtDumpAddrSet failed, model_id:%u", model_id_);
      GELOGE(FAILED, "[Call][RtDumpAddrSet] failed, model_id:%u", model_id_);
      return FAILED;
    }

    // set addr for l1 data dump
    data_dumper_.SetL1FusionAddr(PtrToValue(l1_fusion_addr_));
  }
  return SUCCESS;
}

Status DavinciModel::SetRunAsyncListenerCallback(const RunAsyncCallback &callback) {
  GE_CHECK_NOTNULL(listener_);
  listener_->SetCallback(callback);
  return SUCCESS;
}

void DavinciModel::UpdateOpIOAddrs(const uint32_t task_id, const uint32_t stream_id,
                                   const std::vector<void *> &io_addrs) {
  if (fixed_mem_base_ == mem_base_) {
    GELOGD("[Update][OpIOAddrs] No need to update op input output addr.");
    return;
  }

  OpDescInfo *const op_desc_info = exception_dumper_.MutableOpDescInfo(task_id, stream_id);
  if (op_desc_info == nullptr) {
    GELOGW("[Update][OpIOAddrs] Find op desc failed, task_id: %u, stream_id: %u.", task_id, stream_id);
    return;
  }
  const size_t input_size = op_desc_info->input_addrs.size();
  const size_t output_size = op_desc_info->output_addrs.size();
  if ((input_size + output_size) != io_addrs.size()) {
    GELOGW("[Update][OpIOAddrs] Op[%s] input size[%zu] and output size[%zu] is not equal to io addr size[%zu]",
           op_desc_info->op_name.c_str(), input_size, output_size, io_addrs.size());
    return;
  }

  std::vector<void *> input_addrs;
  std::vector<void *> output_addrs;
  for (size_t i = 0U; i < io_addrs.size(); ++i) {
    if (i < input_size) {
      input_addrs.emplace_back(GetRunAddress(io_addrs[i]));
    } else {
      output_addrs.emplace_back(GetRunAddress(io_addrs[i]));
    }
  }
  op_desc_info->input_addrs = input_addrs;
  op_desc_info->output_addrs = output_addrs;
  GELOGD("[Update][OpIOAddrs] Op [%s] update input output addr success.", op_desc_info->op_name.c_str());
}

///
/// @ingroup ge
/// @brief Get total useful size, in known subgraph, no need to allocate zero copy memory during initialization.
/// @param [in] total_useful_size: total mem size - zero copy size.
/// @return Status
///
Status DavinciModel::GetTotalMemSizeExcludeZeroCopy(int64_t &total_useful_size) {
  if (runtime_param_.mem_size < static_cast<uint64_t>(runtime_param_.zero_copy_size)) {
    REPORT_CALL_ERROR("E19999", "total mem size[%lu] is less than zero copy size[%ld] ", runtime_param_.mem_size,
                      runtime_param_.zero_copy_size);
    GELOGE(FAILED, "[Check][TotalMemSizeExcludeZeroCopy] failed, total mem size[%lu] is less than zero copy size[%ld]",
           runtime_param_.mem_size, runtime_param_.zero_copy_size);
    return FAILED;
  }
  total_useful_size = (static_cast<int64_t>(runtime_param_.mem_size) - runtime_param_.zero_copy_size);
  return SUCCESS;
}

Status DavinciModel::GetEventIdForBlockingAicpuOp(const OpDescPtr &op_desc, const rtStream_t stream,
                                                  uint32_t &event_id) {
  GELOGI("Get event id for aicpu blocking op:%s", op_desc->GetName().c_str());
  const std::map<rtStream_t, rtEvent_t>::const_iterator it = stream_2_event_.find(stream);
  if (it != stream_2_event_.cend()) {
    GE_CHK_RT_RET(rtGetEventID(it->second, &event_id));
  } else {
    rtEvent_t rt_event = nullptr;
    GE_CHK_RT_RET(rtEventCreateWithFlag(&rt_event, RT_EVENT_WITH_FLAG));
    GE_CHK_RT_RET(rtGetEventID(rt_event, &event_id));
    stream_2_event_[stream] = rt_event;
  }
  return SUCCESS;
}

Status DavinciModel::GetEventByStream(const rtStream_t stream, rtEvent_t &rt_event) {
  const std::map<rtStream_t, rtEvent_t>::const_iterator it = stream_2_event_.find(stream);
  if (it == stream_2_event_.cend()) {
    REPORT_INNER_ERROR("E19999", "Get event failed");
    GELOGE(FAILED, "[Get][Event] Get event failed");
    return FAILED;
  }
  rt_event = it->second;
  return SUCCESS;
}

Status DavinciModel::AllocateQueueResource(const Node &node, const OpDescPtr &op_desc, const NamedAttrs &resource) {
  int32_t queue_idx = -1;
  uint32_t queue_id = std::numeric_limits<uint32_t>::max();
  GE_CHK_STATUS_RET_NOLOG(aicpu_resources_.AllocateQueueResource(op_desc, resource, queue_idx, queue_id));
  const auto &src_node = NodeUtils::GetInDataNodeByIndex(node, queue_idx);
  GE_CHECK_NOTNULL(src_node);
  if (!NodeUtils::IsConst(*src_node)) {
    GELOGE(PARAM_INVALID,
           "[%s] Queue id index is not a const (actually is %s), cannot update value",
           op_desc->GetName().c_str(), src_node->GetType().c_str());
    return PARAM_INVALID;
  }
  GE_CHK_STATUS_RET_NOLOG(UpdateOpInputValue(op_desc, queue_idx, queue_id));
  GELOGD("[%s] Input [%d] updated with queue id [%u]", op_desc->GetName().c_str(), queue_idx, queue_id);
  return SUCCESS;
}

Status DavinciModel::AllocateDvppChlResource(const OpDescPtr &op_desc) {
  rtStream_t stream = nullptr;
  const size_t stream_id = static_cast<size_t>(op_desc->GetStreamId());
  GE_CHK_STATUS_RET_NOLOG(GetOpStream(op_desc, stream_id, stream));
  int32_t rt_stream_id = kInvalidStream;
  (void)rtGetStreamId(stream, &rt_stream_id);
  GE_CHK_STATUS_RET_NOLOG(aicpu_resources_.AllocateChannelResource(op_desc, rt_stream_id));
  GELOGD("[%s] Channel resource allocation with stream id [%d] is complete",
         op_desc->GetName().c_str(), rt_stream_id);
  return SUCCESS;
}

Status DavinciModel::AllocateVdecChlResource(const OpDescPtr &op_desc) {
  rtStream_t stream = nullptr;
  const size_t stream_id = static_cast<size_t>(op_desc->GetStreamId());
  GE_CHK_STATUS_RET_NOLOG(GetOpStream(op_desc, stream_id, stream));
  int32_t rt_stream_id = kInvalidStream;
  (void)rtGetStreamId(stream, &rt_stream_id);
  GE_CHK_STATUS_RET_NOLOG(aicpu_resources_.AllocateVdecChannelResource(op_desc, rt_stream_id));
  GELOGD("[%s] Vdec channel resource allocation with stream id [%d] is complete",
         op_desc->GetName().c_str(), rt_stream_id);
  return SUCCESS;
}

Status DavinciModel::AllocateResource(const Node &node) {
  const auto &op_desc = node.GetOpDesc();
  if (!op_desc->HasAttr(ATTR_NAME_RESOURCE_LIST)) {
    return SUCCESS;
  }

  std::vector<NamedAttrs> resource_list;
  if (!AttrUtils::GetListNamedAttrs(op_desc, ATTR_NAME_RESOURCE_LIST, resource_list)) {
    GELOGE(INTERNAL_ERROR, "Failed to get resource list, node name = %s", op_desc->GetName().c_str());
    return INTERNAL_ERROR;
  }

  for (const auto &resource : resource_list) {
    std::string resource_type;
    if (!AttrUtils::GetStr(resource, "resource_type", resource_type)) {
      GELOGE(PARAM_INVALID, "[%s] Failed to get resource type", op_desc->GetName().c_str());
      return PARAM_INVALID;
    }

    Status ret;
    if (resource_type == AiCpuResources::ResourceTypeQueue()) {
      ret = AllocateQueueResource(node, op_desc, resource);
    } else if (resource_type == AiCpuResources::ResourceTypeChannel()) {
      ret = AllocateDvppChlResource(op_desc);
    } else if (resource_type == AiCpuResources::ResourceTypeVdecChannel()) {
      ret = AllocateVdecChlResource(op_desc);
    } else {
      GELOGE(UNSUPPORTED, "Unsupported resource type: %s", resource_type.c_str());
      return UNSUPPORTED;
    }
    if (ret != SUCCESS) {
      GELOGE(PARAM_INVALID, "[%s] Failed to Alloce %s", op_desc->GetName().c_str(), resource_type.c_str());
      return ret;
    }
  }
  return SUCCESS;
}

Status DavinciModel::UpdateOpInputValue(const OpDescPtr &op_desc, const int32_t input_index,
                                        const uint32_t queue_id) const {
  const auto &input_addresses = ModelUtils::GetInputAddrs(runtime_param_, op_desc);
  if (static_cast<size_t>(input_index) >= input_addresses.size()) {
    GELOGE(PARAM_INVALID,
           "[%s] Invalid queue_id_idx: %d, number of inputs = %zu",
           op_desc->GetName().c_str(), input_index, input_addresses.size());
    return PARAM_INVALID;
  }
  const auto &input_desc = op_desc->MutableInputDesc(static_cast<uint32_t>(input_index));
  GE_CHECK_NOTNULL(input_desc);
  int64_t tensor_size = 0;
  (void)TensorUtils::GetSize(*input_desc, tensor_size);
  GE_CHK_RT_RET(rtMemcpy(input_addresses[static_cast<size_t>(input_index)],
                         static_cast<uint64_t>(tensor_size),
                         &queue_id,
                         sizeof(queue_id),
                         RT_MEMCPY_HOST_TO_DEVICE));
  return SUCCESS;
}

///
/// @ingroup domi_ome
/// @brief Get cur_dynamic_dims for all input.
/// @param [in] std::vector<vector<int64_t>> &tensor_input_dims: dims info of all user_inputs.
/// @param [out] std::vector<int32_t> &cur_dynamic_dims: real dims gather, where the index of -1.
/// @return 0: SUCCESS / others: INTERNAL_ERROR
///
Status DavinciModel::GetCurDynamicDims(const std::vector<std::vector<int64_t>> &tensor_input_dims,
                                       std::vector<int32_t> &cur_dynamic_dims) const {
  GELOGD("Start get cur dynamic dims.");
  // parse inputs.dims to std::vector<std::vector<uint64_t>> dynamic_dims
  std::vector<vector<int64_t>> user_real_input_dims;
  if (ParseInputsDims(tensor_input_dims, user_real_input_dims) != SUCCESS) {
    return INTERNAL_ERROR;
  }

  const auto &user_input_dims = run_context_.user_input_dims;
  if (user_real_input_dims.size() != user_input_dims.size()) {
    REPORT_INNER_ERROR("E19999", "Param user_real_input_dims.size:%zu != user_input_dims.size:%zu, check invalid",
                       user_real_input_dims.size(), user_input_dims.size());
    GELOGE(INTERNAL_ERROR, "[Check][Param] The input count of user:%zu should be equal to the data count of graph:%zu",
           user_real_input_dims.size(), user_input_dims.size());
    return INTERNAL_ERROR;
  }

  for (size_t i = 0U; i < user_input_dims.size(); ++i) {
    const auto &user_input_dim = user_input_dims.at(i).second;
    if (user_real_input_dims[i].size() != user_input_dim.size()) {
      REPORT_INNER_ERROR("E19999", "Param user_real_input_dims[%zu].size:%zu != user_input_dims[%zu].size:%zu invalid",
                         i, user_real_input_dims[i].size(), i, user_input_dim.size());
      GELOGE(INTERNAL_ERROR, "[Check][Param] The shape size:%zu of dynamic input:%s should equal to input shape:%zu",
             user_real_input_dims[i].size(), user_input_dims[i].first.c_str(), user_input_dim.size());
      return INTERNAL_ERROR;
    }
    for (size_t j = 0U; j < user_input_dim.size(); ++j) {
      if (user_input_dim.at(j) < 0) {
        cur_dynamic_dims.emplace_back(static_cast<int32_t>(user_real_input_dims[i][j]));
      }
    }
  }
  GELOGD("Cur dynamic dims is %s.", ToString(cur_dynamic_dims).c_str());
  for (const auto &dynamic_dim : run_context_.dynamic_shape_dims) {
    if (dynamic_dim == cur_dynamic_dims) {
      return SUCCESS;
    }
  }

  REPORT_INNER_ERROR("E19999", "cur dynamic dims is %s, not exist in options, check invalid",
                     ToString(cur_dynamic_dims).c_str());
  GELOGE(INTERNAL_ERROR, "[Check][Param] Cur dynamic dims is %s, not exist in options.",
         ToString(cur_dynamic_dims).c_str());
  return INTERNAL_ERROR;
}

void DavinciModel::ParseInputsDimsForData(const std::vector<std::vector<int64_t>> &tensor_input_dims,
                                          std::vector<std::vector<int64_t>> &real_input_dims) const {
  GELOGD("Start parse input dims from data.");
  for (const auto &shape_dims : tensor_input_dims) {
    GELOGD("Input tensor dims is %s.", ToString(shape_dims).c_str());
    real_input_dims.emplace_back(shape_dims);
  }
}

Status DavinciModel::ParseInputsDimsForGetNextNoSinkAndData(const std::vector<NodePtr> &dynamic_nodes,
                                                            const std::vector<std::vector<int64_t>> &tensor_input_dims,
                                                            std::vector<std::vector<int64_t>> &real_input_dims) const {
  GELOGD("Start parse inputs dims when coexist data and getnext sink.");
  for (size_t i = 0U; i < dynamic_nodes.size(); ++i) {
    const auto &op_desc = dynamic_nodes.at(i)->GetOpDesc();
    if (op_desc == nullptr) {
      continue;
    }
    int64_t index = 0;
    if (!AttrUtils::GetInt(op_desc, ATTR_NAME_INDEX, index)) {
      REPORT_CALL_ERROR("E19999", "Get Attr:%s from op:%s(%s) fail", ATTR_NAME_INDEX.c_str(),
                        op_desc->GetName().c_str(), op_desc->GetType().c_str());
      GELOGE(PARAM_INVALID, "[Get][Attr] %s from op:%s(%s) fail", ATTR_NAME_INDEX.c_str(),
             op_desc->GetName().c_str(), op_desc->GetType().c_str());
      return PARAM_INVALID;
    }
    if (static_cast<size_t>(index) >= tensor_input_dims.size()) {
      REPORT_INNER_ERROR("E19999", "Node:%s(%s) index:%zu >= param input_tensor.size:%zu, check invalid",
                         op_desc->GetName().c_str(), op_desc->GetType().c_str(), index, tensor_input_dims.size());
      GELOGE(PARAM_INVALID, "[Check][Param] Node:%s(%s) index:%zu >= param input_tensor.size:%zu",
             op_desc->GetName().c_str(), op_desc->GetType().c_str(), index, tensor_input_dims.size());
      return PARAM_INVALID;
    }

    const auto &shape_dims = tensor_input_dims.at(static_cast<size_t>(index));
    GELOGI("Shape dims of %ld data is %s.", index, ToString(shape_dims).c_str());
    real_input_dims.emplace_back(shape_dims);
  }
  return SUCCESS;
}

Status DavinciModel::ParseInputsDims(const std::vector<std::vector<int64_t>> &tensor_input_dims,
                                     std::vector<std::vector<int64_t>> &real_input_dims) const {
  GELOGI("Start parse input dims of %zu input tensor.", tensor_input_dims.size());
  if (run_context_.dynamic_node_type.empty()) {
    return SUCCESS;
  }

  const std::vector<NodePtr> &input_nodes = run_context_.data_nodes;
  const std::vector<NodePtr> &getnext_nodes = run_context_.getnext_nosink_nodes;
  GELOGD("Data nodes count is %zu, getnext nosink nodes count is %zu.", input_nodes.size(), getnext_nodes.size());
  if (run_context_.dynamic_node_type == DATA) {
    if (getnext_nodes.empty()) {
      // just data or data+getnext_sink
      ParseInputsDimsForData(tensor_input_dims, real_input_dims);
    } else {
      // data+getnext_nosink, but only need to get shape_dims of data
      if (ParseInputsDimsForGetNextNoSinkAndData(input_nodes, tensor_input_dims, real_input_dims) != SUCCESS) {
        GELOGE(PARAM_INVALID, "[Parse][Dims] from data failed, when data coexist with getnext nosink.");
        return PARAM_INVALID;
      }
    }
  } else {
    if (getnext_nodes.empty()) {
      // just getnext_sink or getnext_sink+data, need to get shape_dims from aicpu op
      GELOGI("Need to get dims from aicpu op: GETDYNAMICDIMS.");
      return SUCCESS;
    }

    if (input_nodes.empty()) {
      // just getnext_nosink
      ParseInputsDimsForData(tensor_input_dims, real_input_dims);
    } else {
      // getnext_nosink + data, but only need to get shape_dims of getnext_nosink
      if (ParseInputsDimsForGetNextNoSinkAndData(getnext_nodes, tensor_input_dims, real_input_dims) != SUCCESS) {
        GELOGE(PARAM_INVALID, "[Parse][Dims] from getnext nosink failed, when data coexist with getnext nosink");
        return PARAM_INVALID;
      }
    }
  }

  GELOGI("Parse %zu inputs dims success.", real_input_dims.size());
  return SUCCESS;
}

void DavinciModel::SuperKernelSaveTask(const OpDescPtr &op_desc, const SktTaskInfo &skt_task_info) {
  skt_info_.kernel_list.push_back(skt_task_info.kernel);
  skt_info_.arg_list.push_back(skt_task_info.args);
  skt_info_.last_stream = skt_task_info.stream;
  skt_info_.last_block_dim = skt_task_info.block_dim;
  skt_info_.last_args_size = skt_task_info.args_size;
  skt_info_.last_sm_desc = skt_task_info.sm_desc;
  skt_info_.last_dump_flag = skt_task_info.dump_flag;
  skt_info_.dump_flag_list.push_back(skt_task_info.dump_flag);
  skt_info_.op_desc_list.push_back(op_desc);
  skt_info_.dump_args_list.push_back(skt_task_info.dump_args);
  skt_info_.last_group_key = skt_task_info.group_key;
  skt_info_.last_dump_args = skt_task_info.dump_args;
  skt_info_.last_op = op_desc;

  GELOGI("Save Current task [op_name:%s, block_dim:%u, size:%zu].",
      op_desc->GetName().c_str(), skt_info_.last_block_dim, skt_info_.kernel_list.size());
}

void DavinciModel::SuperKernelUpdateTaskId(uint32_t &skt_task_id) {
  uint32_t task_id = 0U;
  uint32_t stream_id = 0U;
  GE_CHK_RT_EXEC(rtModelGetTaskId(rt_model_handle_, &task_id, &stream_id), return);

  skt_task_id = task_id;
  skt_info_.last_task_id = task_id;
  skt_info_.last_stream_id = stream_id;
  GELOGI("UpdateTaskId:UpdateSKTTaskId [%u],stream id [%u]", task_id, stream_id);
}

void DavinciModel::SuperKernelFinalize(void *const sm_desc, const int64_t group_key) {
  skt_info_.kernel_list.clear();
  skt_info_.arg_list.clear();
  skt_info_.dump_flag_list.clear();
  skt_info_.op_desc_list.clear();
  skt_info_.dump_args_list.clear();
  skt_info_.last_stream = nullptr;
  skt_info_.last_block_dim = 0U;
  skt_info_.last_sm_desc = sm_desc;
  skt_info_.last_group_key = group_key;
  skt_info_.last_dump_flag = RT_KERNEL_DEFAULT;
  skt_info_.last_dump_args = 0U;
  skt_info_.last_op = nullptr;
}

Status DavinciModel::InitQueueDataNodes(const std::vector<NodePtr> &queue_data_nodes,
                                        const uint32_t data_index,
                                        std::set<const void *> &input_outside_addrs) {
  if (queue_data_nodes.empty()) {
    GELOGD("No QueueData node");
    return SUCCESS;
  }

  if (queue_data_nodes.size() > 1U) {
    GELOGE(UNSUPPORTED, "Only supported single QueueData, actual number = %zu", queue_data_nodes.size());
    return UNSUPPORTED;
  }

  if (input_queue_ids_.empty() && output_queue_ids_.empty()) {
    GELOGE(UNSUPPORTED, "Only supported by LoadModelWithQueue");
    return UNSUPPORTED;
  }

  auto &queue_data_node = queue_data_nodes[0U];
  std::string queue_name;
  (void) AttrUtils::GetStr(queue_data_node->GetOpDesc(), "queue_name", queue_name);
  if (queue_name.empty()) {
    GELOGE(PARAM_INVALID, "Queue name not set, node = %s", queue_data_node->GetName().c_str());
    return PARAM_INVALID;
  }
  uint32_t queue_id = UINT32_MAX;
  GE_CHK_RT_RET(rtMemQueueGetQidByName(static_cast<int32_t>(device_id_), queue_name.c_str(), &queue_id));
  GELOGD("Init QueueData node: [%s], queue_name = [%s], queue_id = %u, device_id = %u",
         queue_data_node->GetName().c_str(),
         queue_name.c_str(),
         queue_id,
         device_id_);
  input_queue_ids_.emplace_back(queue_id);
  GE_CHK_STATUS_RET_NOLOG(InitInputZeroCopy(queue_data_node->GetOpDesc(), data_index, input_outside_addrs));
  return SUCCESS;
}

bool DavinciModel::CheckModelNoInputAndOutput() const {
  return (input_queue_ids_.empty() && output_queue_ids_.empty());
}

size_t DavinciModel::GetMemoryTypeIndex(const uint32_t memory_type) {
  return (memory_type & RT_MEMORY_TS) == 0U ? kMemTypeIndexHbm : kMemTypeIndexTs;
}

void DavinciModel::SetTotalArgsSize(const uint32_t args_size, const uint32_t mem_type) {
  known_args_sizes_[GetMemoryTypeIndex(mem_type)] += args_size;
}

void *DavinciModel::GetCurrentArgsAddr(const uint32_t offset, const uint32_t mem_type) const {
  return ValueToPtr(known_args_bases_[GetMemoryTypeIndex(mem_type)] + offset);
}

uint32_t DavinciModel::GetTotalArgsSize(const uint32_t mem_type) const {
  return known_args_sizes_[GetMemoryTypeIndex(mem_type)];
}
}  // namespace ge
