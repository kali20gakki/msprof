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

#include "graph/load/model_manager/model_manager.h"

#include "cce/aicpu_engine_struct.h"
#include "aicpu/aicpu_schedule/aicpu_op_type_list.h"
#include "common/model_parser/model_parser.h"
#include "common/dump/dump_manager.h"
#include "exec_runtime/execution_runtime.h"
#include "common/profiling/profiling_manager.h"
#include "common/ge_call_wrapper.h"
#include "graph/ge_context.h"
#include "graph/load/model_manager/model_utils.h"
#include "graph/load/model_manager/davinci_model.h"
#include "graph/passes/mds_kernels/mds_utils.h"
#include "common/profiling_definitions.h"

namespace ge {
namespace {
constexpr size_t kCmdParSize = 2U;
constexpr size_t kDumpCmdPairSize = 2U;
constexpr size_t kProfCmdParaMaxSize = 1000U;
constexpr size_t kProfStartCmdParaSize = 2U;
constexpr int32_t kTimeSpecNano = 1000000000;  // 1000 ^ 3 converts second to nanosecond
constexpr int32_t kTimeSpecMiro = 1000000;
constexpr size_t kOpNameMaxSize = 100U;
constexpr uint64_t kOfflineSessionId = 0U;
constexpr uint64_t kMaxEventNum = 1024U;
constexpr uint32_t kTriggerScanIntervalMs = 5U; // millisconds
constexpr uint32_t kRecordIntervalMs = 60000U; // millisconds
constexpr uint32_t kRecordTimes = 4U;

const std::string kCmdTypeDump = "dump";
const std::string kCmdTypeProfInit = "prof_init";
const std::string kCmdTypeProfFinalize = "prof_finalize";
const std::string kCmdTypeProfStart = "prof_start";
const std::string kCmdTypeProfStop = "prof_stop";
const std::string kCmdTypeProfModelSubscribe = "prof_model_subscribe";
const std::string kCmdTypeProfModelUnsubscribe = "prof_model_cancel_subscribe";
const std::string kStreamResource = "stream";
const std::string kEventResource = "event";
const std::string kIsCopyOutputAddr = "1";
const std::string kBatchLoadBuf = "batchLoadsoFrombuf";
const std::string kDeleteCustOp = "deleteCustOp";
const std::string kTriggerFile = "exec_record_trigger";
const char_t *const kEnvNpuCollectPath = "NPU_COLLECT_PATH";
const std::string kRecordFilePrefix = "exec_record_";
const std::string kPathSeparator = "/";

#pragma pack(push, 1)
struct CustAicpuSoBuf {
  uint64_t kernelSoBuf;
  uint32_t kernelSoBufLen;
  uint64_t kernelSoName;
  uint32_t kernelSoNameLen;
};

struct BatchLoadOpFromBufArgs {
  uint32_t soNum;
  uint64_t args;
};
#pragma pack(pop)
}  // namespace

std::string ModelManager::record_file_name_;

ModelManager &ModelManager::GetInstance() {
  static const std::shared_ptr<ModelManager> instance_ptr =
      std::shared_ptr<ModelManager>(new (std::nothrow) ModelManager, [](ModelManager *){});
  return *instance_ptr;
}

ModelManager::ModelManager() {
  CreateMonitorThread();
}

Status ModelManager::KernelLaunchEx(const aicpu::FWKAdapter::FWKOperateType op_type, const uint64_t session_id,
                                    const uint32_t model_id, const uint32_t sub_model_id) {
  rtStream_t stream = nullptr;
  std::vector<void *> allocated_mem;
  GE_MAKE_GUARD(kernel_launch_release, [&]() {
    for (auto &mem : allocated_mem) {
      GE_CHK_RT(rtFree(mem));
    }
    if (stream != nullptr) {
      GE_CHK_RT(rtStreamDestroy(stream));
    }
  });

  STR_FWK_OP_KERNEL param_base{};
  const uint32_t kKernelType = 0U;
  param_base.fwkKernelType = kKernelType;
  param_base.fwkKernelBase.fwk_kernel.opType = op_type;
  param_base.fwkKernelBase.fwk_kernel.sessionID = session_id;
  if (op_type == aicpu::FWKAdapter::FWKOperateType::FWK_ADPT_KERNEL_DESTROY) {
    const std::string model_key = std::to_string(session_id) + "_" + std::to_string(model_id) + "_" +
                                  std::to_string(sub_model_id);
    const std::lock_guard<std::recursive_mutex> lk(map_mutex_);
    const std::map<std::string, std::vector<uint64_t>>::const_iterator iter = model_aicpu_kernel_.find(model_key);
    if (iter != model_aicpu_kernel_.cend()) {
      GELOGD("kernel destroy session_id %lu, model_id %u, sub_model_id %u..", session_id, model_id, sub_model_id);
      std::vector<uint64_t> v_aicpu_kernel = model_aicpu_kernel_.at(model_key);
      // Insert size of aicpu kernel vector in the first element
      (void)v_aicpu_kernel.insert(v_aicpu_kernel.cbegin(), v_aicpu_kernel.size());

      const auto kernel_size = sizeof(uint64_t) * (v_aicpu_kernel.size());
      void *aicpu_kernel_addr = nullptr;
      GE_CHK_RT_RET(rtMalloc(&aicpu_kernel_addr, kernel_size, RT_MEMORY_HBM));
      allocated_mem.emplace_back(aicpu_kernel_addr);

      GE_CHK_RT_RET(rtMemcpy(aicpu_kernel_addr, kernel_size, v_aicpu_kernel.data(), kernel_size,
                             RT_MEMCPY_HOST_TO_DEVICE));
      param_base.fwkKernelBase.fwk_kernel.kernelID = PtrToValue(aicpu_kernel_addr);
      // In the scene of loading once and running many times, the kernel needs to be destroyed many times,
      // and connot be removed from kernel map.
    }
  }

  void *device_base = nullptr;
  const size_t op_kernel_size = sizeof(STR_FWK_OP_KERNEL);
  GE_CHK_RT_RET(rtMalloc(&device_base, op_kernel_size, RT_MEMORY_HBM));
  allocated_mem.emplace_back(device_base);
  GE_CHK_RT_RET(rtMemcpy(device_base, op_kernel_size, &param_base, op_kernel_size, RT_MEMCPY_HOST_TO_DEVICE));

  GE_CHK_RT_RET(rtStreamCreate(&stream, 0));
  GE_CHK_RT_RET(rtKernelLaunchEx(device_base, op_kernel_size, 0U, stream));
  GE_CHK_RT_RET(rtStreamSynchronize(stream));

  return SUCCESS;
}

void ModelManager::DestroyAicpuSession(const uint64_t session_id) {
  // when model execute failed because socket close, destroy aicpu session will block all process, must skip
  if (IsSocketClose()) {
    GELOGI("socket is closed, skip destroy aicpu session, session_id:%lu", session_id);
    return;
  }
  const std::lock_guard<std::recursive_mutex> lk(map_mutex_);
  const std::set<uint64_t>::const_iterator it = sess_ids_.find(session_id);
  if (it == sess_ids_.cend()) {
    GELOGI("The session: %lu not created.", session_id);
    return;
  } else {
    rtContext_t ctx = nullptr;
    const bool has_ctx = (rtCtxGetCurrent(&ctx) == RT_ERROR_NONE);
    if (!has_ctx) {
      GELOGI("Set device %u.", GetContext().DeviceId());
      if (MdsUtils::SetDevice(static_cast<int32_t>(GetContext().DeviceId())) != SUCCESS) {
        GELOGE(FAILED, "Set device failed.");
      }
    }

    const auto ret = KernelLaunchEx(aicpu::FWKAdapter::FWKOperateType::FWK_ADPT_SESSION_DESTROY, session_id, 0U, 0U);
    if (ret != SUCCESS) {
      GELOGW("The session: %lu destroy failed.", session_id);
    } else {
      (void)sess_ids_.erase(it);
      GELOGI("The session: %lu destroyed.", session_id);
    }

    if (!has_ctx) {
      GELOGI("Reset device %u.", GetContext().DeviceId());
      GE_CHK_RT(rtDeviceReset(static_cast<int32_t>(GetContext().DeviceId())));
    }
  }
}

Status ModelManager::DestroyAicpuSessionForInfer(const uint32_t model_id) {
  const std::lock_guard<std::recursive_mutex> lk(map_mutex_);
  const std::map<uint32_t, std::shared_ptr<hybrid::HybridDavinciModel>>::const_iterator hybrid_it =
      hybrid_model_map_.find(model_id);
  if (hybrid_it != hybrid_model_map_.cend()) {
    DestroyAicpuSession(hybrid_it->second->GetSessionId());
    return SUCCESS;
  }

  const std::map<uint32_t, std::shared_ptr<DavinciModel>>::const_iterator it = model_map_.find(model_id);
  if (it == model_map_.cend()) {
    REPORT_INNER_ERROR("E19999", "Param model_id:%u can't find in model_map, check invalid", model_id);
    GELOGE(ACL_ERROR_GE_EXEC_MODEL_ID_INVALID, "[Check][Param] model id %u does not exists.", model_id);
    return ACL_ERROR_GE_EXEC_MODEL_ID_INVALID;
  }

  DestroyAicpuSession(it->second->GetSessionId());
  return SUCCESS;
}

Status ModelManager::DestroyAicpuKernel(const uint64_t session_id, const uint32_t model_id,
                                        const uint32_t sub_model_id) {
  GELOGD("destroy aicpu kernel in session_id %lu, model_id %u.", session_id, model_id);
  const std::lock_guard<std::recursive_mutex> lk(map_mutex_);
  const std::string model_key = std::to_string(session_id) + "_" + std::to_string(model_id) + "_" +
                                std::to_string(sub_model_id);
  if (model_aicpu_kernel_.find(model_key) != model_aicpu_kernel_.end()) {
    const Status ret = KernelLaunchEx(aicpu::FWKAdapter::FWKOperateType::FWK_ADPT_KERNEL_DESTROY, session_id, model_id,
                                      sub_model_id);
    if (ret != SUCCESS) {
      REPORT_CALL_ERROR("E19999", "Call KernelLaunchEx fail, model_id:%u, sub_model_id:%u, session_id:%lu",
                        model_id, sub_model_id, session_id);
      GELOGE(FAILED, "[Call][KernelLaunchEx] fail, model_id:%u, sub_model_id:%u, session_id:%lu",
             model_id, sub_model_id, session_id);
      return FAILED;
    }
  }
  return SUCCESS;
}

void ModelManager::CreateAicpuKernel(const uint64_t session_id, const uint32_t model_id, const uint32_t sub_model_id,
                                     const uint64_t kernel_id) {
  const std::lock_guard<std::recursive_mutex> lk(map_mutex_);
  const std::string model_key = std::to_string(session_id) + "_" + std::to_string(model_id) + "_" +
                                std::to_string(sub_model_id);
  const auto it = model_aicpu_kernel_.find(model_key);
  if (it != model_aicpu_kernel_.cend()) {
    it->second.push_back(kernel_id);
  } else {
    model_aicpu_kernel_[model_key] = { kernel_id };
  }
}

ModelManager::~ModelManager() {
  const std::lock_guard<std::recursive_mutex> lk(map_mutex_);
  model_map_.clear();
  hybrid_model_map_.clear();
  heterogeneous_model_map_.clear();
  model_aicpu_kernel_.clear();
  sess_ids_.clear();
  exception_infos_.clear();
  cust_aicpu_so_.clear();
  dump_exception_flag_ = false;
  ClearMonitorThread();
}

Status ModelManager::SetDynamicSize(const uint32_t model_id, const std::vector<uint64_t> &batch_num,
                                    const int32_t dynamic_type) {
  const auto &davinci_model = GetModel(model_id);
  GE_CHECK_NOTNULL(davinci_model);
  davinci_model->SetDynamicSize(batch_num, dynamic_type);
  return SUCCESS;
}

Status ModelManager::DoLoadHybridModelOnline(const uint32_t model_id, const std::string &om_name,
                                             const GeRootModelPtr &ge_root_model,
                                             const std::shared_ptr<ModelListener> &listener) {
  auto hybrid_model = hybrid::HybridDavinciModel::Create(ge_root_model);
  GE_CHECK_NOTNULL(hybrid_model);
  hybrid_model->SetListener(listener);
  hybrid_model->SetModelId(model_id);
  hybrid_model->SetDeviceId(GetContext().DeviceId());
  hybrid_model->SetOmName(om_name);
  GE_CHK_STATUS_RET(hybrid_model->Init(), "[Init][HybridModel] failed. model_id = %u", model_id);
  const auto shared_model = std::shared_ptr<hybrid::HybridDavinciModel>(hybrid_model.release());
  InsertModel(model_id, shared_model);
  return SUCCESS;
}

bool ModelManager::IsNeedHybridLoad(const GeRootModel &ge_root_model) const {
  const auto root_graph = ge_root_model.GetRootGraph();
  GE_RT_FALSE_CHECK_NOTNULL(root_graph);

  bool is_dsp_partitioned_graph = false;
  (void)AttrUtils::GetBool(root_graph, ATTR_NAME_DYNAMIC_SHAPE_PARTITIONED, is_dsp_partitioned_graph);
  return root_graph->GetGraphUnknownFlag() || is_dsp_partitioned_graph || GetContext().GetHostExecFlag();
}

///
/// @ingroup domi_ome
/// @brief load model online
/// @return Status run result
///
Status ModelManager::LoadModelOnline(uint32_t &model_id, const GeRootModelPtr &ge_root_model,
                                     const GraphNodePtr &graph_node, const uint32_t device_id, const int64_t die_id) {
  std::shared_ptr<ModelListener> listener;
  if (graph_node->IsAsync()) {
    listener = MakeShared<RunAsyncListener>();
  } else {
    listener = MakeShared<GraphModelListener>();
  }
  GE_CHK_BOOL_RET_STATUS(listener.get() != nullptr, PARAM_INVALID, "[Check][Param] Param incorrect, listener is null");
  if (model_id == INVALID_MODEL_ID) {
    GenModelId(model_id);
    GELOGD("Generate new model_id:%u", model_id);
  }

  ProfilingManager::Instance().SetGraphIdToModelMap(graph_node->GetGraphId(), model_id);
  GELOGI("Set graph id to model map, graph_id: %u, model_id: %u.", graph_node->GetGraphId(), model_id);

  domi::GetContext().is_online_model = true;

  if (IsNeedHybridLoad(*ge_root_model)) {
    const std::string om_name;
    return DoLoadHybridModelOnline(model_id, om_name, ge_root_model, listener);
  }

  const mmTimespec tick_count = mmGetTickCount();
  const int64_t start_time = (tick_count.tv_sec * kTimeSpecNano) + tick_count.tv_nsec;
  const auto root_graph = ge_root_model->GetRootGraph();
  GE_CHECK_NOTNULL(root_graph);

  const auto &name_to_model = ge_root_model->GetSubgraphInstanceNameToModel();
  const auto it = name_to_model.find(root_graph->GetName());
  const GeModelPtr ge_model = (it != name_to_model.end()) ? it->second : nullptr;
  GE_CHECK_NOTNULL(ge_model);

  const auto davinci_model = MakeShared<DavinciModel>(0, listener);
  GE_CHECK_NOTNULL(davinci_model);
  davinci_model->SetProfileTime(ModelProcStage::MODEL_LOAD_START, static_cast<uint64_t>(start_time));
  davinci_model->SetId(model_id);
  davinci_model->SetDeviceId(device_id);
  davinci_model->SetDieId(die_id);
  davinci_model->SetRunContext(graph_node->GetOmeContext());

  GE_TIMESTAMP_START(Assign);
  davinci_model->Assign(ge_model);
  GE_TIMESTAMP_END(Assign, "GraphLoader::ModelAssign");
  const uint64_t session_id = GetContext().SessionId();

  const DumpProperties &dump_properties = DumpManager::GetInstance().GetDumpProperties(session_id);
  davinci_model->SetDumpProperties(dump_properties);
  {
    const std::lock_guard<std::mutex> lk(dump_properties_mutex_);
    dump_properties_ = dump_properties;
  }
  std::string execute_mode;
  auto result = GetContext().GetOption(OPTION_EXEC_DYNAMIC_EXECUTE_MODE, execute_mode);
  if (result != SUCCESS) {
    GELOGW("Can not get dynamic execute mode attr");
  }
  GELOGD("The dynamic execute mode is %s", execute_mode.c_str());

  std::string is_copy_output_addr;
  result = GetContext().GetOption(OPTION_EXEC_ENABLE_COPY_OUTPUT_ADDR, is_copy_output_addr);
  if (result != SUCCESS) {
    GELOGW("Can not get option exec enable copy output addr attr");
  }
  GELOGD("The enable_copy_output_addrs is %s", is_copy_output_addr.c_str());

  if ((execute_mode == kLazyRecompile) && (is_copy_output_addr == kIsCopyOutputAddr)) {
    result = CheckAndReleaseStreamEventResource(ge_model, model_id);
    GE_IF_BOOL_EXEC(result != SUCCESS, GELOGW("[Release][Resource] failed, model id:%u", model_id));
    GELOGI("Release stream and event resources for model id:%u success", model_id);
  }

  GE_TIMESTAMP_START(Init);
  result = davinci_model->Init();
  if (result != SUCCESS) {
    GELOGE(result, "DavinciModel Init failed.");
    return result;
  }
  GE_TIMESTAMP_END(Init, "GraphLoader::ModelInit");

  InsertModel(model_id, davinci_model);
  GELOGI("Parse model %u success.", model_id);

  return SUCCESS;
}

Status ModelManager::LoadFlowModelOnline(uint32_t &model_id, const FlowModelPtr &flow_model, const GraphNodePtr &graph_node) {
  std::shared_ptr<ModelListener> listener;
  if (graph_node->IsAsync()) {
    listener = MakeShared<RunAsyncListener>();
  } else {
    listener = MakeShared<GraphModelListener>();
  }
  GE_CHK_BOOL_RET_STATUS(listener.get() != nullptr, PARAM_INVALID, "[Check][Param] Param incorrect, listener is null");
  GenModelId(model_id);
  GELOGD("Generate new model_id:%u", model_id);

  ProfilingManager::Instance().SetGraphIdToModelMap(graph_node->GetGraphId(), model_id);
  GELOGI("Set graph id to model map, graph_id: %u, model_id: %u.", graph_node->GetGraphId(), model_id);

  domi::GetContext().is_online_model = true;
  {
    const std::lock_guard<std::mutex> lk(dump_properties_mutex_);
    dump_properties_ = DumpManager::GetInstance().GetDumpProperties(GetContext().SessionId());
  }
  return LoadAsHeterogeneousModel(model_id, flow_model, listener);
}

Status ModelManager::CheckAndReleaseStreamEventResource(const GeModelPtr &ge_model, const uint32_t model_id) {
  GE_CHECK_NOTNULL(ge_model);
  uint64_t need_stream_num = 0U;
  uint64_t need_event_num = 0U;
  (void)AttrUtils::GetInt(ge_model, ATTR_MODEL_STREAM_NUM, need_stream_num);
  (void)AttrUtils::GetInt(ge_model, ATTR_MODEL_EVENT_NUM, need_event_num);
  GELOGD("The main stream number is %lu, event number is %lu", need_stream_num, need_event_num);

  uint64_t hccl_follow_stream = 0U;
  Status status = ModelUtils::CalculateFollowStream(ge_model, hccl_follow_stream);
  if (status != SUCCESS) {
    GELOGE(FAILED, "[Calculate][stream] Calculate follow stream num failed");
    return FAILED;
  }
  need_stream_num = need_stream_num + hccl_follow_stream;
  GELOGD("The model is %u, need stream number is %ld, hccl follow stream number is %ld",
         model_id, need_stream_num, hccl_follow_stream);

  uint64_t free_stream_num = 0U;
  status = GetFreeStream(free_stream_num);
  if (status != SUCCESS) {
    GELOGE(FAILED, "Get free stream num failed");
    return FAILED;
  }
  if (need_stream_num > free_stream_num) {
    status = ReleaseResource(need_stream_num, free_stream_num, kStreamResource);
    if (status != SUCCESS) {
      GELOGE(FAILED, "Release stream resoure failed");
      return FAILED;
    }
  }

  uint64_t free_event_num = 0U;
  GetFreeEvent(free_event_num);
  if (need_event_num > free_event_num) {
    status = ReleaseResource(need_event_num, free_event_num, kEventResource);
    if (status != SUCCESS) {
      GELOGE(FAILED, "Release event resource failed");
      return FAILED;
    }
  }
  return SUCCESS;
}

Status ModelManager::ReleaseResource(const uint64_t need_resource, uint64_t free_resource,
                                     const std::string &resource_kind) {
  Status ret = SUCCESS;
  while (need_resource > free_resource) {
    GELOGD("Resource Kind: %s, need: %lu, free: %lu", resource_kind.c_str(), need_resource, free_resource);
    uint32_t max_stream_model_id = 0U;
    uint32_t max_event_model_id = 0U;
    GetMaxStreamAndEventModel(max_stream_model_id, max_event_model_id);
    GELOGD("The max stream num model is: %u, the max event num model is :%u", max_stream_model_id, max_event_model_id);
    const std::lock_guard<std::recursive_mutex> lk(map_mutex_);
    if (resource_kind == kStreamResource) {
      const std::map<uint32_t, std::shared_ptr<DavinciModel>>::const_iterator it = model_map_.find(max_stream_model_id);
      if (it == model_map_.cend()) {
        REPORT_INNER_ERROR("E19999", "model_id:%u not exist in model_map, check invalid", max_stream_model_id);
        GELOGE(ACL_ERROR_GE_EXEC_MODEL_ID_INVALID, "max stream model id %u does not exists.", max_stream_model_id);
        return ACL_ERROR_GE_EXEC_MODEL_ID_INVALID;
      }
      const uint64_t max_stream_num = it->second->GetAllStreamNum();
      ret = Unload(max_stream_model_id);
      if (ret != SUCCESS) {
        GELOGE(FAILED, "Unload max stream model failed, model id : %u", max_stream_model_id);
        return FAILED;
      }
      free_resource += max_stream_num;
      GELOGD("Unload model for stream, model id : %u, stream num : %lu", max_stream_model_id, max_stream_num);
    }
    if (resource_kind == kEventResource) {
      const std::map<uint32_t, std::shared_ptr<DavinciModel>>::const_iterator it = model_map_.find(max_event_model_id);
      if (it == model_map_.cend()) {
        REPORT_INNER_ERROR("E19999", "model_id:%u not exist in model_map, check invalid", max_event_model_id);
        GELOGE(ACL_ERROR_GE_EXEC_MODEL_ID_INVALID, "max event model id %u does not exists.", max_event_model_id);
        return ACL_ERROR_GE_EXEC_MODEL_ID_INVALID;
      }
      const uint64_t max_event_num = it->second->GetEventList().size();
      ret = Unload(max_event_model_id);
      if (ret != SUCCESS) {
        GELOGE(FAILED, "Unload max event model failed, model id : %u", max_event_model_id);
        return FAILED;
      }
      free_resource += max_event_num;
      GELOGD("Unload model for event, model id : %u, event num : %zu", max_event_model_id, max_event_num);
    }
  }
  return SUCCESS;
}

Status ModelManager::GetFreeStream(uint64_t &free_stream) {
  uint32_t max_stream_count = 0U;
  uint32_t max_task_count = 0U;
  GE_CHK_RT_RET(rtGetMaxStreamAndTask(RT_NORMAL_STREAM, &max_stream_count, &max_task_count));

  GELOGD("Allowed max stream cout :%u, max task cout per stream:%u", max_stream_count, max_task_count);
  const std::lock_guard<std::recursive_mutex> lk(map_mutex_);
  uint64_t stream_sum = 0U;
  const auto accum_stream_num = [&stream_sum](const std::pair<uint32_t, std::shared_ptr<DavinciModel>> &item) {
    stream_sum += item.second->GetAllStreamNum();
    return true;
  };
  (void)std::all_of(model_map_.cbegin(), model_map_.cend(), accum_stream_num);

  free_stream = static_cast<uint64_t>(max_stream_count) - stream_sum;
  return SUCCESS;
}

void ModelManager::GetFreeEvent(uint64_t &free_event) {
  const std::lock_guard<std::recursive_mutex> lk(map_mutex_);
  uint64_t event_sum = 0U;
  const auto accum_event_num = [&event_sum](const std::pair<uint32_t, std::shared_ptr<DavinciModel>> &item) {
    event_sum += item.second->GetEventList().size();
    return true;
  };
  (void)std::all_of(model_map_.cbegin(), model_map_.cend(), accum_event_num);

  free_event = kMaxEventNum - event_sum;
}

void ModelManager::GetMaxStreamAndEventModel(uint32_t &max_stream_model, uint32_t &max_event_model) {
  const std::lock_guard<std::recursive_mutex> lk(map_mutex_);
  uint64_t max_stream_num = 0U;
  uint64_t max_event_num = 0U;
  for (const auto &it : model_map_) {
    if (it.second->GetAllStreamNum() > max_stream_num) {
      max_stream_num = it.second->GetAllStreamNum();
      max_stream_model = it.first;
    }
    if (it.second->GetEventList().size() > max_event_num) {
      max_event_num = it.second->GetEventList().size();
      max_event_model = it.first;
    }
  }
}

void ModelManager::InsertModel(const uint32_t model_id, const std::shared_ptr<DavinciModel> &davinci_model) {
  GE_CHK_BOOL_EXEC(davinci_model != nullptr, return, "[Check][Param] davinci_model ptr is null, id:%u", model_id);
  const std::lock_guard<std::recursive_mutex> lk(map_mutex_);
  model_map_[model_id] = davinci_model;
}

void ModelManager::InsertModel(const uint32_t model_id, const shared_ptr<hybrid::HybridDavinciModel> &hybrid_model) {
  GE_CHK_BOOL_EXEC(hybrid_model != nullptr, return, "[Check][Param] hybrid_model ptr is null, id:%u", model_id);
  const std::lock_guard<std::recursive_mutex> lk(map_mutex_);
  hybrid_model_map_[model_id] = hybrid_model;
}

Status ModelManager::DeleteModel(const uint32_t id) {
  if (UnloadHeterogeneousModel(id)) {
    return SUCCESS;
  }

  // These two pointers are used to unbind erase() and model destruction process.
  {
    const std::lock_guard<std::recursive_mutex> lk(map_mutex_);
    const std::map<uint32_t, std::shared_ptr<DavinciModel>>::const_iterator it = model_map_.find(id);
    if (it != model_map_.cend()) {
      const uint64_t session_id = it->second->GetSessionId();
      const std::string model_key = std::to_string(session_id) + "_" + std::to_string(id)  + "_" +
                                    std::to_string(it->second->SubModelId());
      (void)model_aicpu_kernel_.erase(model_key);
      (void)model_map_.erase(it);
      return SUCCESS;
    }

    const std::map<uint32_t, std::shared_ptr<hybrid::HybridDavinciModel>>::const_iterator hybrid_model_it =
        hybrid_model_map_.find(id);
    if (hybrid_model_it != hybrid_model_map_.cend()) {
      (void)hybrid_model_map_.erase(hybrid_model_it);
    } else {
      REPORT_INNER_ERROR("E19999", "model_id:%u not exist in model_map, check invalid", id);
      GELOGE(ACL_ERROR_GE_EXEC_MODEL_ID_INVALID, "model id %u does not exists.", id);
      return ACL_ERROR_GE_EXEC_MODEL_ID_INVALID;
    }
  }

  return SUCCESS;
}

std::shared_ptr<DavinciModel> ModelManager::GetModel(const uint32_t id) {
  const std::lock_guard<std::recursive_mutex> lk(map_mutex_);

  std::map<uint32_t, std::shared_ptr<DavinciModel>>::const_iterator it = model_map_.find(id);
  return (it == model_map_.cend()) ? nullptr : it->second;
}

std::shared_ptr<hybrid::HybridDavinciModel> ModelManager::GetHybridModel(const uint32_t id) {
  const std::lock_guard<std::recursive_mutex> lk(map_mutex_);

  const std::map<uint32_t, std::shared_ptr<hybrid::HybridDavinciModel>>::const_iterator it = hybrid_model_map_.find(id);
  return (it == hybrid_model_map_.cend()) ? nullptr : it->second;
}

Status ModelManager::Unload(const uint32_t model_id) {
  GE_CHK_STATUS_RET(DeleteModel(model_id), "[Delete][Model] failed, model id:%u", model_id);
  GELOGI("Unload model %u success.", model_id);

  if (!domi::GetContext().is_online_model) {
    (void)rtUnsetDeviceIdByGeModelIdx(model_id, 0U);
  }
  {
    const std::lock_guard<std::mutex> lk(exception_infos_mutex_);
    exception_infos_.clear();
  }
  return SUCCESS;
}

Status ModelManager::SyncExecuteModel(const InputData &input_data, OutputData &output_data) {
  GELOGI("calling the DataInput");
  const auto &model = GetModel(input_data.model_id);
  GE_CHECK_NOTNULL(model);

  const auto data_wrap = MakeShared<InputDataWrapper>(input_data, output_data);
  GE_CHECK_NOTNULL(data_wrap);

  const uint32_t model_id = input_data.model_id;
  output_data.model_id = input_data.model_id;

  if (model->ResetResult() != SUCCESS) {
    REPORT_CALL_ERROR("E19999", "Call graph_run_listener_.ResetResult fail, model_id:%u", model_id);
    GELOGE(GE_GRAPH_EXECUTE_FAILED, "[Reset][Result] failed, model_id:%u", model_id);
    return GE_GRAPH_EXECUTE_FAILED;
  }

  GE_IF_BOOL_EXEC(model->GetDataInputTid() == 0, model->SetDataInputTid(mmGetTid()));

  if (model->Push(data_wrap) != SUCCESS) {
    REPORT_CALL_ERROR("E19999", "Data queue is full, call again later, model_id %u", model_id);
    GELOGE(GE_GRAPH_DATA_INPUT_FAILED, "[Call][Push] Data queue is full, call again later, model_id %u", model_id);
    return GE_GRAPH_DATA_INPUT_FAILED;
  }
  GELOGI("[GraphExecutor] input data push to wrapper finish, waiting for result...");

  // Run graph return, pending until async execute graph complete
  const uint32_t result_code = model->GetResultCode();
  if ((result_code != SUCCESS) && (result_code != END_OF_SEQUENCE)) {
    REPORT_CALL_ERROR("E19999", "Execute model fail, result:%u, model_id:%u", result_code, model_id);
    GELOGE(GE_GRAPH_SYNC_MODEL_FAILED, "[Execute][Model] failed, ret=%u, modelId=%u.", result_code, model_id);
    return GE_GRAPH_SYNC_MODEL_FAILED;
  }
  return SUCCESS;
}

///
/// @ingroup domi_ome
/// @brief load Input and output TensorInfo for Model
/// @return Status run result
///
Status ModelManager::DataInputTensor(const uint32_t model_id, const std::vector<Tensor> &inputs) {
  const auto &model = GetModel(model_id);
  const auto &hybrid_model = GetHybridModel(model_id);
  if (hybrid_model == nullptr) {
    GE_CHECK_NOTNULL(model);
  }

  InputData input_data;
  input_data.model_id = model_id;
  input_data.timeout = 0U;
  input_data.timestamp = 0U;
  input_data.index = 0U;
  for (size_t i = 0U; i < inputs.size(); ++i) {
    const TensorDesc &tensor_desc = inputs[i].GetTensorDesc();
    input_data.shapes.emplace_back(tensor_desc.GetShape().GetDims());
    DataBuffer data_blob;
    data_blob.data = ValueToPtr(PtrToValue(inputs[i].GetData()));
    data_blob.length = inputs[i].GetSize();
    data_blob.placement = static_cast<uint32_t>(tensor_desc.GetPlacement());
    input_data.blobs.push_back(data_blob);
  }

  OutputData output_data;
  output_data.model_id = model_id;
  output_data.index = 0U;

  const auto data_wrap = MakeShared<InputDataWrapper>(input_data, output_data);
  GE_CHECK_NOTNULL(data_wrap);

  if (hybrid_model != nullptr) {
    GE_CHK_STATUS_RET(hybrid_model->EnqueueData(data_wrap),
                      "[Enqueue][Data] Data queue is full, please call again later, model_id:%u", model_id);
    return SUCCESS;
  }

  GE_CHK_BOOL_RET_STATUS(model != nullptr, PARAM_INVALID,
                         "[Check][Param] Invalid model id %u in InputData!", model_id);

  GE_CHK_STATUS_EXEC(model->Push(data_wrap), return domi::DATA_QUEUE_ISFULL,
                     "[Call][Push] Data queue is full, please call again later, model_id %u ", model_id);

  GELOGD("Data input success, model id:%u", model_id);

  return SUCCESS;
}
///
/// @ingroup domi_ome
/// @brief create model thread, start to execute model
/// @param [in] model_id Model ID to be started
/// @return Status model run result
/// @author
///
Status ModelManager::Start(const uint32_t model_id) {
  const auto &heterogeneous_model = GetHeterogeneousModelExecutor(model_id);
  if (heterogeneous_model != nullptr) {
    GE_CHK_STATUS_RET_NOLOG(heterogeneous_model->ModelRunStart());
    GELOGI("Start heterogeneous model %u success.", model_id);
    return SUCCESS;
  }

  const auto &hybrid_model = GetHybridModel(model_id);
  if (hybrid_model != nullptr) {
    GE_CHK_STATUS_RET_NOLOG(hybrid_model->ModelRunStart());
    GELOGI("Start hybrid model %u success.", model_id);
    return SUCCESS;
  }

  const auto &davinci_model = GetModel(model_id);
  GE_CHK_BOOL_RET_STATUS(davinci_model != nullptr, PARAM_INVALID,
                         "[Get][Model] failed, Invalid model id %u to start! ", model_id);

  const auto status = davinci_model->ModelRunStart();
  if (status == SUCCESS) {
    GELOGI("Start model %u success.", model_id);
  }

  return status;
}

///
/// @ingroup domi_ome
/// @brief Model ID stop
/// @only when unloaded
/// @param [in] model_id Model ID to be stopped
/// @return Status model stop result
/// @author
///
Status ModelManager::Stop(const uint32_t model_id) {
  const auto &heterogeneous_model = GetHeterogeneousModelExecutor(model_id);
  if (heterogeneous_model != nullptr) {
    GE_CHK_STATUS_RET_NOLOG(heterogeneous_model->ModelRunStop());
    GELOGI("Start heterogeneous model %u success.", model_id);
    return SUCCESS;
  }

  const auto &hybrid_model = GetHybridModel(model_id);
  if (hybrid_model != nullptr) {
    GE_CHK_STATUS_RET_NOLOG(hybrid_model->ModelRunStop());
    GELOGI("Stop hybrid model %u success.", model_id);
    return SUCCESS;
  }

  const auto &davinci_model = GetModel(model_id);
  GE_CHK_BOOL_RET_STATUS(davinci_model != nullptr, PARAM_INVALID,
                         "[Get][Model] failed, Invalid model id %u to stop!", model_id);

  const auto status = davinci_model->ModelRunStop();
  if (status == SUCCESS) {
    GELOGI("Stop model %u success.", model_id);
  }

  return status;
}

///
/// @ingroup domi_ome
/// @brief Command handle
/// @iterator 1 only Ieference, Debug 2 modes
/// @param [in] command command to handle
/// @return Status command handle result
/// @author
///
Status ModelManager::HandleCommand(const Command &cmd_info) {
  static const std::map<std::string, std::function<uint32_t(const Command &)>> cmds = {
      {kCmdTypeDump, &HandleDumpCommand}, {kCmdTypeProfInit, &HandleProfInitCommand},
      {kCmdTypeProfFinalize, &HandleProfFinalizeCommand}, {kCmdTypeProfStart, &HandleProfStartCommand},
      {kCmdTypeProfStop, &HandleProfStopCommand},
      {kCmdTypeProfModelSubscribe, &HandleProfModelSubscribeCommand},
      {kCmdTypeProfModelUnsubscribe, &HandleProfModelUnsubscribeCommand}};

  const auto iter = cmds.find(cmd_info.cmd_type);
  if (iter == cmds.end()) {
    REPORT_INNER_ERROR("E19999", "Unsupported command:%s check", cmd_info.cmd_type.c_str());
    GELOGE(PARAM_INVALID, "[Check][Param] Unsupported command:%s", cmd_info.cmd_type.c_str());
    return PARAM_INVALID;
  } else {
    return iter->second(cmd_info);
  }
}

Status ModelManager::GetModelIdByCmd(const Command &cmd_info, uint32_t &model_id) {
  if (cmd_info.cmd_params.size() < kCmdParSize) {
    REPORT_INNER_ERROR("E19999", "Command::cmd_params.size:%zu < kCmdParSize:%zu, Command::cmd_type:%s, check invalid",
                       cmd_info.cmd_params.size(), kCmdParSize, cmd_info.cmd_type.c_str());
    GELOGE(PARAM_INVALID, "[Check][Param] When the cmd_type is '%s', the size of cmd_params must larger than 2.",
           cmd_info.cmd_type.c_str());
    return PARAM_INVALID;
  }

  if (cmd_info.cmd_params[0U] != PROFILE_MODEL_ID) {
    REPORT_INNER_ERROR("E19999", "Fisrt cmd_param not %s, check invalid", PROFILE_MODEL_ID.c_str());
    GELOGE(FAILED, "[Check][Param] The model_id parameter is not found in the command.");
    return FAILED;
  }

  int32_t tmp_value = 0;
  GE_CHK_STATUS_RET_NOLOG(ConvertToInt32(cmd_info.cmd_params[1U], tmp_value));
  model_id = static_cast<uint32_t>(tmp_value);
  return SUCCESS;
}

Status ModelManager::HandleProfModelSubscribeCommand(const Command &cmd_info) {
  uint32_t model_id = std::numeric_limits<uint32_t>::max();
  const Status ret = GetModelIdByCmd(cmd_info, model_id);
  if (ret != SUCCESS) {
    return ret;
  }

  // set graph id to UINT32_MAX to determine whether execution
  return ModelManager::GetInstance().ProfModelSubscribe(cmd_info.module_index, model_id, UINT32_MAX);
}

Status ModelManager::HandleProfModelUnsubscribeCommand(const Command &cmd_info) {
  uint32_t model_id = std::numeric_limits<uint32_t>::max();
  const Status ret = GetModelIdByCmd(cmd_info, model_id);
  if (ret != SUCCESS) {
    return ret;
  }

  const auto hybrid_davinci_model = ModelManager::GetInstance().GetHybridModel(model_id);
  if (hybrid_davinci_model != nullptr) {
    return ProfilingManager::Instance().ProfModelUnsubscribe(hybrid_davinci_model->GetDeviceId());
  }

  const auto davinci_model = ModelManager::GetInstance().GetModel(model_id);
  if (davinci_model != nullptr) {
    return ProfilingManager::Instance().ProfModelUnsubscribe(davinci_model->GetDeviceId());
  }
  return FAILED;
}

Status ModelManager::HandleProfInitCommand(const Command &cmd_info) {
  if (profiling::ProfilingContext::IsDumpToStdEnabled()) {
    return SUCCESS;
  }

  const uint64_t module_index = cmd_info.module_index;
  if (ProfilingManager::Instance().ProfInit(module_index) != SUCCESS) {
    GELOGE(FAILED, "[Handle][ProfInit] failed, module_index:%lu.", module_index);
    return FAILED;
  }
  return SUCCESS;
}

Status ModelManager::HandleProfFinalizeCommand(const Command &cmd_info) {
  if (ProfilingManager::Instance().ProfFinalize() != SUCCESS) {
    GELOGE(FAILED, "[Handle][ProfFinalize] failed, moduld index: %lu.", cmd_info.module_index);
    return FAILED;
  }
  return SUCCESS;
}
/*
 * cmd para when prof start
 * "devNums:2"
 * "devIdList:1,2"
 * "profilingOption:PROF_OP_TRACE"
 * "aicoreMetrics:AICORE_ARITHMATIC_THROUGHPUT"
 */
Status ModelManager::HandleProfStartCommand(const Command &cmd_info) {
  if (profiling::ProfilingContext::IsDumpToStdEnabled()) {
    profiling::ProfilingContext::GetInstance().SetEnable();
    return SUCCESS;
  }

  if (cmd_info.cmd_params.size() < kProfStartCmdParaSize) {
    REPORT_INNER_ERROR("E19999", "Command::cmd_params.size:%zu < %zu, check invalid",
                       cmd_info.cmd_params.size(), kProfStartCmdParaSize);
    GELOGE(PARAM_INVALID, "[Check][Param] When the cmd_type is 'profile start', "
           "the size:%zu of cmd_params must larger than 2.", cmd_info.cmd_params.size());
    return PARAM_INVALID;
  }
  if (cmd_info.cmd_params.size() > kProfCmdParaMaxSize) {
    REPORT_INNER_ERROR("E19999", "Command::cmd_params.size:%zu > %zu, check invalid",
                       cmd_info.cmd_params.size(), kProfCmdParaMaxSize);
    GELOGE(PARAM_INVALID, "[Check][Param] Command param size[%zu] larger than max[1000].", cmd_info.cmd_params.size());
    return PARAM_INVALID;
  }

  std::map<std::string, std::string> cmd_params_map;
  const uint32_t step = 2U;
  for (size_t i = 0U; i < cmd_info.cmd_params.size(); i += step) {
    if ((i + 1U) >= cmd_info.cmd_params.size()) {
      continue;
    }
    cmd_params_map[cmd_info.cmd_params[i]] = cmd_info.cmd_params[i + 1U];
  }
  const uint64_t module_index = cmd_info.module_index;
  if (ProfilingManager::Instance().ProfStartProfiling(module_index, cmd_params_map) != SUCCESS) {
    GELOGE(FAILED, "[Handle][ProfStartProfiling] failed, module_index:%lu.", module_index);
    return FAILED;
  }
  return SUCCESS;
}

Status ModelManager::HandleProfStopCommand(const Command &cmd_info) {
  if (profiling::ProfilingContext::IsDumpToStdEnabled()) {
    profiling::ProfilingContext::GetInstance().DumpToStdOut();
    profiling::ProfilingContext::GetInstance().Reset();
    profiling::ProfilingContext::GetInstance().SetDisable();
    return SUCCESS;
  }

  if (cmd_info.cmd_params.size() < kProfStartCmdParaSize) {
    REPORT_INNER_ERROR("E19999", "Command::cmd_params.size:%zu < %zu, check invalid",
                       cmd_info.cmd_params.size(), kProfStartCmdParaSize);
    GELOGE(PARAM_INVALID, "[Check][Param] When the cmd_type is 'profile stop', "
           "the size:%zu of cmd_params must larger than 2.", cmd_info.cmd_params.size());
    return PARAM_INVALID;
  }
  if (cmd_info.cmd_params.size() > kProfCmdParaMaxSize) {
    REPORT_INNER_ERROR("E19999", "Command::cmd_params.size:%zu > %zu, check invalid",
                       cmd_info.cmd_params.size(), kProfCmdParaMaxSize);
    GELOGE(PARAM_INVALID, "[Check][Param] Command param size[%zu] larger than max[1000].", cmd_info.cmd_params.size());
    return PARAM_INVALID;
  }

  std::map<std::string, std::string> cmd_params_map;
  const uint32_t step = 2U; // cmd params saved as: { key1, val1, key2, val2, key3, val3 }
  for (size_t i = 0U; i < cmd_info.cmd_params.size(); i += step) {
    if ((i + 1U) >= cmd_info.cmd_params.size()) {  // +1 for value.
      break;
    }
    cmd_params_map[cmd_info.cmd_params[i]] = cmd_info.cmd_params[i + 1U];
  }
  const uint64_t module_index = cmd_info.module_index;
  if (ProfilingManager::Instance().ProfStopProfiling(module_index, cmd_params_map) != SUCCESS) {
    GELOGE(FAILED, "[Handle][ProfStopProfiling] failed, module_index:%lu.", module_index);
    return FAILED;
  }
  return SUCCESS;
}

static Status ParserPara(const Command &cmd_info, const std::string &dump_key, std::string &dump_value) {
  auto iter = std::find(cmd_info.cmd_params.begin(), cmd_info.cmd_params.end(), dump_key);
  if (iter != cmd_info.cmd_params.end()) {
    ++iter; // cmd params saved as: { key1, val1, key2, val2, key3, val3 }
    if (iter == cmd_info.cmd_params.end()) {
      REPORT_INNER_ERROR("E19999", "dump_key:%s can't find in Command::cmd_param, check invalid", dump_key.c_str());
      GELOGE(PARAM_INVALID, "[Check][Param] dump_key:%s can't find in cmd_param, check invalid", dump_key.c_str());
      return PARAM_INVALID;
    }
    dump_value = *iter;
  }
  return SUCCESS;
}

Status ModelManager::HandleDumpCommand(const Command &cmd_info) {
  if ((cmd_info.cmd_params.size() % kDumpCmdPairSize) != 0U) {
    REPORT_INNER_ERROR("E19999", "Command::cmd_params.size:%zu MOD 2 != 0, check invalid", cmd_info.cmd_params.size());
    GELOGE(PARAM_INVALID, "[Check][Param] When the cmd_type is 'dump', "
           "the size:%zu of cmd_params must be a even number.", cmd_info.cmd_params.size());
    return PARAM_INVALID;
  }

  std::string dump_status("off");
  std::string dump_model(DUMP_ALL_MODEL);
  std::string dump_path("/");
  std::string dump_mode("output");
  std::set<std::string> dump_layers;

  auto ret = ParserPara(cmd_info, DUMP_STATUS, dump_status);
  if (ret != SUCCESS) {
    GELOGE(PARAM_INVALID, "[Parser][DumpStatus] failed, ret:%d", ret);
    return FAILED;
  }
  GELOGI("dump status = %s.", dump_status.c_str());

  ret = ParserPara(cmd_info, DUMP_MODEL, dump_model);
  if (ret != SUCCESS) {
    GELOGE(PARAM_INVALID, "[Parser][DumpModel] failed, ret:%d", ret);
    return FAILED;
  }
  GELOGI("dump model = %s.", dump_model.c_str());

  auto &ref_dump_properties = ModelManager::GetInstance().dump_properties_;
  if ((dump_status == "off") || (dump_status == "OFF")) {
    ref_dump_properties.DeletePropertyValue(dump_model);
    return SUCCESS;
  }

  for (size_t i = 0U; i < (cmd_info.cmd_params.size() / kDumpCmdPairSize); ++i) {
    if (cmd_info.cmd_params.at(i * kDumpCmdPairSize).find(DUMP_LAYER) != std::string::npos) {
      GELOGI("dump layer: %s.", cmd_info.cmd_params.at((i * kDumpCmdPairSize) + 1U).c_str());
      (void)dump_layers.insert(cmd_info.cmd_params.at((i * kDumpCmdPairSize) + 1U));
    }
  }

  ret = ParserPara(cmd_info, DUMP_FILE_PATH, dump_path);
  if (ret != SUCCESS) {
    GELOGE(PARAM_INVALID, "[Parser][DumpPath] failed, ret:%d", ret);
    return FAILED;
  }
  if ((!dump_path.empty()) && (dump_path[dump_path.size() - 1U] != '/')) {
    dump_path = dump_path + "/";
  }
  dump_path = (dump_path + CurrentTimeInStr()) + "/";
  GELOGI("dump path = %s.", dump_path.c_str());

  ret = ParserPara(cmd_info, DUMP_MODE, dump_mode);
  if (ret != SUCCESS) {
    GELOGE(PARAM_INVALID, "[Parser][DumpMode] failed, ret:%d", ret);
    return FAILED;
  }
  GELOGI("dump mode = %s", dump_mode.c_str());

  ref_dump_properties.AddPropertyValue(dump_model, dump_layers);
  ref_dump_properties.SetDumpPath(dump_path);
  ref_dump_properties.SetDumpMode(dump_mode);
  return SUCCESS;
}

Status ModelManager::GetMaxUsedMemory(const uint32_t model_id, uint64_t &max_size) {
  const auto &hybrid_model = GetHybridModel(model_id);
  if (hybrid_model != nullptr) {
    max_size = 0U;
    return SUCCESS;
  }

  const auto &davinci_model = GetModel(model_id);
  GE_CHK_BOOL_RET_STATUS(davinci_model != nullptr, PARAM_INVALID,
                         "[Get][Model] failed, Invalid model id:%u!", model_id);

  max_size = davinci_model->TotalMemSize();
  return SUCCESS;
}

Status ModelManager::GetInputOutputDescInfo(const uint32_t model_id, std::vector<InputOutputDescInfo> &input_desc,
                                            std::vector<InputOutputDescInfo> &output_desc) {
  const auto &davinci_model = GetModel(model_id);
  GE_CHK_BOOL_RET_STATUS(davinci_model != nullptr, PARAM_INVALID,
                         "[Get][Model] failed, Invalid model id %u!", model_id);

  return davinci_model->GetInputOutputDescInfo(input_desc, output_desc);
}

Status ModelManager::GetInputOutputDescInfo(const uint32_t model_id, std::vector<InputOutputDescInfo> &input_desc,
                                            std::vector<InputOutputDescInfo> &output_desc,
                                            std::vector<uint32_t> &inputFormats, std::vector<uint32_t> &outputFormats,
                                            const bool new_model_desc) {
  const auto &hybrid_davinci_model = GetHybridModel(model_id);
  if (hybrid_davinci_model != nullptr) {
    hybrid_davinci_model->SetModelDescVersion(new_model_desc);
    return hybrid_davinci_model->GetInputOutputDescInfo(input_desc, output_desc, inputFormats, outputFormats);
  }

  const auto &davinci_model = GetModel(model_id);
  GE_CHK_BOOL_RET_STATUS(davinci_model != nullptr, ACL_ERROR_GE_EXEC_MODEL_ID_INVALID,
                         "[Get][Model] Failed, Invalid model id %u!", model_id);

  return davinci_model->GetInputOutputDescInfo(input_desc, output_desc, inputFormats, outputFormats, new_model_desc);
}

///
/// @ingroup ge
/// @brief Get dynamic batch_info
/// @param [in] model_id
/// @param [out] batch_info
/// @return execute result
///
Status ModelManager::GetDynamicBatchInfo(const uint32_t model_id, std::vector<std::vector<int64_t>> &batch_info,
                                         int32_t &dynamic_type) {
  const auto &hybrid_davinci_model = GetHybridModel(model_id);
  if (hybrid_davinci_model != nullptr) {
    return hybrid_davinci_model->GetDynamicBatchInfo(batch_info, dynamic_type);
  }

  const auto &davinci_model = GetModel(model_id);
  GE_CHK_BOOL_RET_STATUS(davinci_model != nullptr, ACL_ERROR_GE_EXEC_MODEL_ID_INVALID,
                         "[Get][Model] failed, Invalid model id %u!", model_id);

  return davinci_model->GetDynamicBatchInfo(batch_info, dynamic_type);
}

///
/// @ingroup ge
/// @brief Get combined dynamic dims info
/// @param [in] model_id
/// @param [out] batch_info
/// @return execute result
///
Status ModelManager::GetCombinedDynamicDims(const uint32_t model_id, std::vector<std::vector<int64_t>> &batch_info) {
  const auto &davinci_model = GetModel(model_id);
  GE_CHK_BOOL_RET_STATUS(davinci_model != nullptr, ACL_ERROR_GE_EXEC_MODEL_ID_INVALID,
                         "[Get][Model] Failed, Invalid Model ID %u!", model_id);

  davinci_model->GetCombinedDynamicDims(batch_info);
  return SUCCESS;
}

///
/// @ingroup ge
/// @brief Get user designate shape order
/// @param [in] model_id
/// @param [out] user_input_shape_order
/// @return execute result
///
Status ModelManager::GetUserDesignateShapeOrder(const uint32_t model_id,
                                                std::vector<std::string> &user_input_shape_order) {
  const auto &hybrid_davinci_model = GetHybridModel(model_id);
  if (hybrid_davinci_model != nullptr) {
    hybrid_davinci_model->GetUserDesignateShapeOrder(user_input_shape_order);
    return SUCCESS;
  }

  const auto &davinci_model = GetModel(model_id);
  GE_CHK_BOOL_RET_STATUS(davinci_model != nullptr, ACL_ERROR_GE_EXEC_MODEL_ID_INVALID,
                         "[Get][Model] Failed, Invalid Model ID %u!", model_id);
  davinci_model->GetUserDesignateShapeOrder(user_input_shape_order);
  return SUCCESS;
}

Status ModelManager::GetCurrentShape(const uint32_t model_id, std::vector<int64_t> &batch_info,
                                     int32_t &dynamic_type) {
  const auto &davinci_model = GetModel(model_id);
  GE_CHK_BOOL_RET_STATUS(davinci_model != nullptr, ACL_ERROR_GE_EXEC_MODEL_ID_INVALID,
                         "[Get][Model] Failed, Invalid Model ID %u!", model_id);
  davinci_model->GetCurrentShape(batch_info, dynamic_type);
  return SUCCESS;
}

Status ModelManager::GetNodeAttr(const uint32_t model_id, const std::string &op_name, const std::string &attr_name,
                                 std::string &attr_info) {
  const auto &davinci_model = GetModel(model_id);
  if (davinci_model != nullptr) {
    return davinci_model->GetNodeAttr(op_name, attr_name, attr_info);
  }
  const auto &hybrid_davinci_model = GetHybridModel(model_id);
  if (hybrid_davinci_model != nullptr) {
    return hybrid_davinci_model->GetOpAttr(op_name, attr_name, attr_info);
  }
  GELOGE(ACL_ERROR_GE_EXEC_MODEL_ID_INVALID, "[Get][Model]Get model failed, invalid model id:%u.", model_id);
  REPORT_INNER_ERROR("E19999", "Get model failed, invalid model id:%u.", model_id);
  return ACL_ERROR_GE_EXEC_MODEL_ID_INVALID;
}

Status ModelManager::GetOutputShapeInfo(const uint32_t model_id, std::vector<std::string> &dynamic_output_shape_info) {
  const auto &hybrid_davinci_model = GetHybridModel(model_id);
  if (hybrid_davinci_model != nullptr) {
    hybrid_davinci_model->GetOutputShapeInfo(dynamic_output_shape_info);
    return SUCCESS;
  }

  const auto &davinci_model = GetModel(model_id);
  GE_CHK_BOOL_RET_STATUS(davinci_model != nullptr, ACL_ERROR_GE_EXEC_MODEL_ID_INVALID,
                         "[Get][Model] Failed, Invalid Model ID %u!", model_id);
  davinci_model->GetOutputShapeInfo(dynamic_output_shape_info);
  return SUCCESS;
}

///
/// @ingroup ge
/// @brief Get AIPP info
/// @param [in] model_id
/// @param [in] index
/// @param [out] aipp_info
/// @return execute result
///
Status ModelManager::GetAippInfo(const uint32_t model_id, const uint32_t index, AippConfigInfo &aipp_info) {
  const auto &hybrid_davinci_model = GetHybridModel(model_id);
  if (hybrid_davinci_model != nullptr) {
    return hybrid_davinci_model->GetAippInfo(index, aipp_info);
  }
  const auto &davinci_model = GetModel(model_id);
  GE_CHK_BOOL_RET_STATUS(davinci_model != nullptr, ACL_ERROR_GE_EXEC_MODEL_ID_INVALID,
      "[Get][Model] failed, invalid model_id is %u.", model_id);
  return davinci_model->GetAippInfo(index, aipp_info);
}

Status ModelManager::GetAippType(const uint32_t model_id, const uint32_t index,
                                 InputAippType &type, size_t &aipp_index) {
  const auto &hybrid_davinci_model = GetHybridModel(model_id);
  if (hybrid_davinci_model != nullptr) {
    return hybrid_davinci_model->GetAippType(index, type, aipp_index);
  }
  const auto &davinci_model = GetModel(model_id);
  GE_CHK_BOOL_RET_STATUS(davinci_model != nullptr, ACL_ERROR_GE_EXEC_MODEL_ID_INVALID,
      "[Get][Model] failed, invalid model_id is %u.", model_id);
  return davinci_model->GetAippType(index, type, aipp_index);
}

void ModelManager::GenSessionId(uint64_t &session_id) {
  const uint64_t kSessionTimeMask = 0xffffffffffff0000UL;
  const uint64_t kSessionPidMask  = 0x000000000000ff00UL;
  const uint64_t kSessionBiasMask = 0x00000000000000ffUL;

  const uint64_t kMaskPerOffset = 8UL;

  const std::lock_guard<std::mutex> lk(session_id_create_mutex_);

  mmTimeval tv;
  (void)mmGetTimeOfDay(&tv, nullptr);
  const int64_t timestamp = (tv.tv_sec * kTimeSpecMiro) + tv.tv_usec;  // 1000000us

  static const uint64_t pid = static_cast<uint64_t>(mmGetPid());
  session_id_bias_++;

  session_id = (((static_cast<uint64_t>(timestamp) << kMaskPerOffset) << kMaskPerOffset) & kSessionTimeMask) +
               ((pid << kMaskPerOffset) & kSessionPidMask) + (session_id_bias_ & kSessionBiasMask);

  GELOGD("Generate new session id: %lu.", session_id);
}

Status ModelManager::LoadModelOffline(uint32_t &model_id, const ModelData &model, shared_ptr<ModelListener> listener,
                                      const uintptr_t mem_ptr, const size_t mem_size,
                                      const uintptr_t weight_ptr, const size_t weight_size) {
  domi::GetContext().is_online_model = false;
  ProfilingProperties::Instance().SetProfilingLoadOffineFlag(true);
  GenModelId(model_id);

  const mmTimespec tick_count = mmGetTickCount();
  const int64_t start_time = (tick_count.tv_sec * kTimeSpecNano) + tick_count.tv_nsec;

  ModelHelper model_helper;
  Status ret = model_helper.LoadRootModel(model);
  if (ret != SUCCESS) {
    GELOGE(ret, "[Load][RootModel] failed, ret:%d, model_id:%u.", ret, model_id);
    return ret;
  }
  int32_t device_id = -1;
  GE_CHK_RT_RET(rtGetDevice(&device_id));
  (void)rtSetDeviceIdByGeModelIdx(model_id, static_cast<uint32_t>(device_id));
  /// In multi-threaded inference,  using the same session_id among multiple threads may cause some threads to fail.
  /// These session_ids come from the same model, so the values of session_id are the same.
  /// Update session_id for infer in load model to avoid the same session_id.
  uint64_t new_session_id;
  GenSessionId(new_session_id);

  ProfilingManager::Instance().SetGraphIdToModelMap(model_id, model_id);
  GELOGI("Set graph id to model map, model_id: %u.", model_id);

  if (model_helper.GetModelType()) {
    bool is_shape_unknown = false;
    GE_CHK_STATUS_RET(model_helper.GetGeRootModel()->CheckIsUnknownShape(is_shape_unknown),
                      "[Check][IsUnknownShape] failed, model id:%u", model_id);
    if (is_shape_unknown || GetContext().GetHostExecFlag()) {
      const auto &ge_models = model_helper.GetGeRootModel()->GetSubgraphInstanceNameToModel();
      if (ge_models.empty()) {
        GELOGE(INTERNAL_ERROR, "[Get][SubModel]Root model has no sub model.");
        REPORT_INNER_ERROR("E19999", "Root model has no sub model.");
        return INTERNAL_ERROR;
      }
      for (auto iter = ge_models.begin(); iter != ge_models.end(); ++iter) {
        (void)AttrUtils::SetInt(iter->second, MODEL_ATTR_SESSION_ID, static_cast<int64_t>(new_session_id));
      }

      return DoLoadHybridModelOnline(model_id, model.om_name, model_helper.GetGeRootModel(), listener);
    }
  }

  const GeModelPtr &ge_model = model_helper.GetGeModel();
  GE_CHECK_NOTNULL(ge_model);
  const auto davinci_model = MakeShared<DavinciModel>(model.priority, listener);
  GE_CHECK_NOTNULL(davinci_model);

  davinci_model->SetProfileTime(ModelProcStage::MODEL_LOAD_START, static_cast<uint64_t>(start_time));
  davinci_model->Assign(ge_model);
  davinci_model->SetId(model_id);

  davinci_model->SetDeviceId(static_cast<uint32_t>(device_id));
  davinci_model->SetOmName(model.om_name);
  SetDumpProperties(davinci_model);

  ret = davinci_model->UpdateSessionId(new_session_id);
  if (ret != SUCCESS) {
    GELOGE(FAILED, "[Update][SessionId] for inference failed, session id:%lu.", new_session_id);
    return ret;
  }

  ret = davinci_model->Init(mem_ptr, mem_size, weight_ptr, weight_size);
  if (ret != SUCCESS) {
    GELOGE(FAILED, "[Init][DavinciModel] failed, ret:%d.", ret);
    return ret;
  }

  InsertModel(model_id, davinci_model);
  GELOGI("Parse model %u success.", model_id);

  return SUCCESS;
}

///
/// @ingroup ge
/// @brief ACL case, Load task list with queue.
/// @param [out] model_id: model id for manager.
/// @param [in] model_data: Model data load from offline model file.
/// @param [in] input_que_ids: input queue ids from user, num equals Data Op.
/// @param [in] output_que_ids: input queue ids from user, num equals NetOutput Op.
/// @return: 0 for success / others for fail
///
Status ModelManager::LoadModelWithQ(uint32_t &model_id,
                                    const ModelData &model_data,
                                    const std::vector<uint32_t> &input_queue_ids,
                                    const std::vector<uint32_t> &output_queue_ids) {
  ModelHelper model_helper;
  const Status ret = model_helper.LoadModel(model_data);
  if (ret != SUCCESS) {
    GELOGE(ret, "[Load][Model] failed.");
    return ret;
  }
  return LoadModelWithQ(model_id, model_helper.GetGeRootModel(),
                        input_queue_ids, output_queue_ids, model_data.priority);
}

Status ModelManager::LoadModelWithQ(uint32_t &model_id,
                                    const GeRootModelPtr &root_model,
                                    const std::vector<uint32_t> &input_queue_ids,
                                    const std::vector<uint32_t> &output_queue_ids,
                                    const int32_t priority,
                                    const bool need_update_session_id) {
  if (IsNeedHybridLoad(*root_model)) {
    REPORT_CALL_ERROR("E19999", "Dynamic shaped graphs are not currently supported by LoadModelWithQ");
    GELOGE(UNSUPPORTED, "Dynamic shaped graphs are not currently supported by LoadModelWithQ, model_id: %u", model_id);
    return UNSUPPORTED;
  }
  const auto &root_graph = root_model->GetRootGraph();
  const auto &submodels = root_model->GetSubgraphInstanceNameToModel();
  const auto it = submodels.find(root_graph->GetName());
  if (it == submodels.end()) {
    REPORT_CALL_ERROR("E19999", "Failed to get GeModel");
    GELOGE(INTERNAL_ERROR, "Failed to get GeModel, name = %s", root_graph->GetName().c_str());
    return INTERNAL_ERROR;
  }
  const auto &ge_model = it->second;
  GE_CHECK_NOTNULL(ge_model);
  const auto davinci_model = MakeShared<DavinciModel>(priority, nullptr);
  GE_CHECK_NOTNULL(davinci_model);
  davinci_model->Assign(ge_model);

  Status ret = SUCCESS;
  if (need_update_session_id) {
    /// In multi-threaded inference,  using the same session_id among multiple threads may cause some threads to fail.
    /// These session_ids come from the same model, so the values of session_id are the same.
    /// Update session_id for infer in load model to avoid the same session_id.
    uint64_t new_session_id;
    GenSessionId(new_session_id);
    ret = davinci_model->UpdateSessionId(new_session_id);
    GE_CHK_BOOL_TRUE_EXEC_WITH_LOG(ret != SUCCESS, return ret,
                                   "[Update][SessionId] for infer failed, SessionId:%lu.", new_session_id);
  }

  GenModelId(model_id);
  davinci_model->SetId(model_id);
  int32_t device_id = -1;
  GE_CHK_RT_RET(rtGetDevice(&device_id));
  GELOGD("Get device_id %d success", device_id);
  davinci_model->SetDeviceId(static_cast<uint32_t>(device_id));
  ret = davinci_model->SetQueIds(input_queue_ids, output_queue_ids);
  if (ret != SUCCESS) {
    GELOGE(ret, "[Set][Ids] for model queue failed, ret:%d, model_id:%u.", ret, model_id);
    return ret;
  }
  SetDumpProperties(davinci_model);

  ret = davinci_model->Init();
  if (ret != SUCCESS) {
    GELOGE(ret, "[Init][Model] failed, ret:%d, model_id:%u.", ret, model_id);
    return ret;
  }

  InsertModel(model_id, davinci_model);
  GELOGI("Parse model %u success.", model_id);

  return SUCCESS;
}

void ModelManager::SetDumpProperties(std::shared_ptr<DavinciModel> davinci_model) {
  if (DumpManager::GetInstance().GetDumpProperties(kOfflineSessionId).IsDumpOpen()) {
    davinci_model->SetDumpProperties(DumpManager::GetInstance().GetDumpProperties(kOfflineSessionId));
  } else {
    davinci_model->SetDumpProperties(dump_properties_);
  }
}

Status ModelManager::LoadModelWithoutQ(uint32_t &model_id, const GeRootModelPtr &root_model, const int32_t priority) {
  GE_CHECK_NOTNULL(root_model);
  if (IsNeedHybridLoad(*root_model)) {
    REPORT_CALL_ERROR("E19999", "Dynamic shaped graphs are not currently supported by LoadModelWithQ");
    GELOGE(UNSUPPORTED, "Dynamic shaped graphs are not currently supported by LoadModelWithQ, model_id: %u", model_id);
    return UNSUPPORTED;
  }
  const auto &root_graph = root_model->GetRootGraph();
  GE_CHECK_NOTNULL(root_graph);
  const auto &submodels = root_model->GetSubgraphInstanceNameToModel();
  const auto it = submodels.find(root_graph->GetName());
  if (it == submodels.end()) {
    REPORT_CALL_ERROR("E19999", "Failed to get GeModel");
    GELOGE(INTERNAL_ERROR, "Failed to get GeModel, name = %s, model_id: %u", root_graph->GetName().c_str(), model_id);
    return INTERNAL_ERROR;
  }
  const auto &ge_model = it->second;
  GE_CHECK_NOTNULL(ge_model);
  const auto davinci_model = MakeShared<DavinciModel>(priority, nullptr);
  GE_CHECK_NOTNULL(davinci_model);
  davinci_model->Assign(ge_model);

  GenModelId(model_id);
  davinci_model->SetId(model_id);
  davinci_model->SetDumpProperties(dump_properties_);
  int32_t device_id = -1;
  GE_CHK_RT_RET(rtGetDevice(&device_id));
  GELOGD("Get device_id %d success", device_id);
  davinci_model->SetDeviceId(static_cast<uint32_t>(device_id));
  GELOGD("Begin to init model %u.", model_id);
  GE_CHK_STATUS_RET(davinci_model->Init(), "[Init][Model] failed, model_id:%u.", model_id);

  InsertModel(model_id, davinci_model);
  GELOGI("Parse model %u success.", model_id);

  if (davinci_model->CheckModelNoInputAndOutput()) {
    GELOGI("need model %u execute.", model_id);
    const std::vector<GeTensor> input_tensor;
    const std::vector<GeTensor> output_tensor;
    GE_CHK_STATUS_RET(ExecuteModel(model_id, nullptr, false, input_tensor, output_tensor),
                      "[Execute][Model] failed, model_id:%u.", model_id);
    GELOGI("Finish model %u execute.", model_id);
  }

  return SUCCESS;
}
///
/// @ingroup domi_ome
/// @brief  ACL case, not start new thread, return result
/// @param [in] model_id  mode id
/// @param [in] stream   model stream
/// @param [in] async_mode  is asynchronize mode.
/// @param [in] input_data  input data
/// @param [in] input_desc  description of input data
/// @param [out] output_data  output data
/// @param [out] output_desc  description of output data
///
Status ModelManager::ExecuteModel(const uint32_t model_id, const rtStream_t stream, const bool async_mode,
                                  const InputData &input_data, const std::vector<GeTensorDesc> &input_desc,
                                  OutputData &output_data, std::vector<GeTensorDesc> &output_desc) {
  Status status = SUCCESS;
  const auto &hybrid_davinci_model = GetHybridModel(model_id);
  if (hybrid_davinci_model != nullptr) {
    status = hybrid_davinci_model->Execute(input_data.blobs, input_desc, output_data.blobs, output_desc, stream);
    if (status == SUCCESS) {
      GELOGI("Execute model %u success.", model_id);
    }
    return status;
  }

  const auto &davinci_model = GetModel(model_id);
  GE_CHK_BOOL_RET_STATUS(davinci_model != nullptr, ACL_ERROR_GE_EXEC_MODEL_ID_INVALID,
                         "[Get][Model] Invalid model id %u, check whether model has been loaded or not.", model_id);

  if (davinci_model->NeedDestroyAicpuKernel()) {
    GELOGI("Start to destroy specified aicpu kernel.");
    // Zero copy is enabled by default, no need to judge.
    const uint64_t session_id_davinci = davinci_model->GetSessionId();
    const uint32_t model_id_davinci = davinci_model->GetModelId();
    const uint32_t sub_model_id = davinci_model->SubModelId();
    status = DestroyAicpuKernel(session_id_davinci, model_id_davinci, sub_model_id);
    if (status != SUCCESS) {
      GELOGW("Destroy specified aicpu kernel failed, session id is %lu, model id is %u.", session_id_davinci,
             model_id_davinci);
    }
  }

  status = davinci_model->NnExecute(stream, async_mode, input_data, output_data);
  if (status == SUCCESS) {
    GELOGD("Execute model %u success.", model_id);
  }
  return status;
}

namespace {
void GetGeTensorBlobs(const std::vector<GeTensor> &tensors, std::vector<DataBuffer> &blobs) {
  blobs.resize(tensors.size());
  for (size_t i = 0U; i < tensors.size(); i++) {
    blobs[i].data = ValueToPtr(PtrToValue(tensors[i].GetData().data()));
    blobs[i].length = tensors[i].GetData().size();
    blobs[i].isDataSupportMemShare = false;
  }
}

void GetGeTensorDescs(const std::vector<GeTensor> &tensors, std::vector<GeTensorDesc> &descs) {
  descs.reserve(tensors.size());
  const auto accum_tensors = [&descs](const GeTensor &tensor) {
    descs.emplace_back(tensor.GetTensorDesc());
    return true;
  };
  (void)std::all_of(tensors.cbegin(), tensors.cend(), accum_tensors);
}
}

Status ModelManager::ExecuteModel(const uint32_t model_id, const rtStream_t stream, const bool async_mode,
                                  const std::vector<GeTensor> &input_tensor,
                                  const std::vector<GeTensor> &output_tensor) {
  InputData input_buffer;
  input_buffer.index = 0U;
  input_buffer.model_id = model_id;
  GetGeTensorBlobs(input_tensor, input_buffer.blobs);

  OutputData output_buffer;
  output_buffer.index = 0U;
  output_buffer.model_id = model_id;
  GetGeTensorBlobs(output_tensor, output_buffer.blobs);

  std::vector<GeTensorDesc> input_desc;
  std::vector<GeTensorDesc> output_desc;
  if (GetHybridModel(model_id) != nullptr) {
    GetGeTensorDescs(input_tensor, input_desc);
    GetGeTensorDescs(output_tensor, output_desc);
  }

  return ExecuteModel(model_id, stream, async_mode, input_buffer, input_desc,  output_buffer, output_desc);
}

Status ModelManager::CreateAicpuSession(const uint64_t session_id) {
  const std::lock_guard<std::recursive_mutex> lk(map_mutex_);
  const std::set<uint64_t>::const_iterator it = sess_ids_.find(session_id);
  // never been created by any model
  if (it == sess_ids_.cend()) {
    const Status ret = KernelLaunchEx(aicpu::FWKAdapter::FWKOperateType::FWK_ADPT_SESSION_CREATE, session_id, 0U, 0U);
    if (ret == SUCCESS) {
      (void)sess_ids_.insert(session_id);
      GELOGI("The session: %lu create success.", session_id);
    }
    return ret;
  }
  return SUCCESS;
}

Status ModelManager::LoadCustAicpuSo(const OpDescPtr &op_desc, const std::string &so_name, bool &loaded) {
  GELOGD("LoadCustAicpuSo in, op name %s, so name %s", op_desc->GetName().c_str(), so_name.c_str());
  const CustAICPUKernelPtr aicpu_kernel = op_desc->TryGetExtAttr(OP_EXTATTR_CUSTAICPU_KERNEL, CustAICPUKernelPtr());
  return LoadCustAicpuSo(aicpu_kernel, so_name, loaded);
}

Status ModelManager::LoadCustAicpuSo(const CustAICPUKernelPtr &aicpu_kernel, const std::string &so_name, bool &loaded) {
  if (aicpu_kernel == nullptr) {
    GELOGI("cust aicpu has no corresponding kernel: %s!", so_name.c_str());
    return SUCCESS;
  }

  // get current context
  rtContext_t rt_cur_ctx = nullptr;
  GE_CHK_RT_RET(rtCtxGetCurrent(&rt_cur_ctx));

  // use current context as resource key
  const std::lock_guard<std::mutex> lk(cust_aicpu_mutex_);
  const uintptr_t resource_id = PtrToValue(rt_cur_ctx);
  const auto it = cust_aicpu_so_.find(resource_id);
  if (it == cust_aicpu_so_.end()) {
    std::map<std::string, CustAICPUKernelPtr> new_so_name;
    (void)new_so_name.insert({so_name, aicpu_kernel});
    cust_aicpu_so_[resource_id] = new_so_name;
    loaded = false;
    GELOGD("LoadCustAicpuSo new aicpu so name %s, resource id %lu", so_name.c_str(), resource_id);
    return SUCCESS;
  }
  const auto it_so_name = it->second.find(so_name);
  if (it_so_name == it->second.end()) {
    (void)it->second.insert({so_name, aicpu_kernel});
    loaded = false;
    GELOGD("LoadCustAicpuSo add aicpu so name %s, resource id %lu", so_name.c_str(), resource_id);
    return SUCCESS;
  }
  loaded = true;
  GELOGD("LoadCustAicpuSo so name %s has been loaded.", so_name.c_str());
  return SUCCESS;
}

Status ModelManager::LaunchKernelCustAicpuSo(const std::string &kernel_name) {
  GELOGD("Aicpu kernel launch task in, kernel name %s.", kernel_name.c_str());
  const std::lock_guard<std::mutex> lk(cust_aicpu_mutex_);
  if (cust_aicpu_so_.empty()) {
    return SUCCESS;
  }
  // get current context
  rtContext_t rt_cur_ctx = nullptr;
  GE_CHK_RT_RET(rtCtxGetCurrent(&rt_cur_ctx));

  const uintptr_t resource_id = PtrToValue(rt_cur_ctx);
  const auto it = cust_aicpu_so_.find(resource_id);
  if (it == cust_aicpu_so_.end()) {
    GELOGI("Cust aicpu so map is empty, context id %lu", resource_id);
    return SUCCESS;
  }

  rtStream_t stream = nullptr;
  std::vector<void *> allocated_mem;
  const std::function<void()> callback = [&stream, &allocated_mem]() {
    for (auto &mem : allocated_mem) {
      GE_CHK_RT(rtFree(mem));
    }
    if (stream != nullptr) {
      GE_CHK_RT(rtStreamDestroy(stream));
    }
  };
  GE_MAKE_GUARD(release, callback);

  std::vector<CustAicpuSoBuf> v_cust_so;

  for (const auto &it_so : it->second) {
    const void *const aicpu_data = it_so.second->GetBinData();
    const size_t aicpu_data_length = it_so.second->GetBinDataSize();
    const std::string so_name = it_so.first;
    void *d_aicpu_data = nullptr;
    void *d_so_name = nullptr;

    GE_CHK_RT_RET(rtMalloc(&d_aicpu_data, aicpu_data_length, RT_MEMORY_HBM));
    allocated_mem.push_back(d_aicpu_data);
    GE_CHK_RT_RET(rtMalloc(&d_so_name, so_name.size(), RT_MEMORY_HBM));
    allocated_mem.push_back(d_so_name);
    GE_CHK_RT(rtMemcpy(d_aicpu_data, aicpu_data_length, aicpu_data, aicpu_data_length, RT_MEMCPY_HOST_TO_DEVICE));
    GE_CHK_RT(rtMemcpy(d_so_name, so_name.size(), so_name.c_str(), so_name.size(), RT_MEMCPY_HOST_TO_DEVICE));

    CustAicpuSoBuf cust_aicpu_so_buf;
    cust_aicpu_so_buf.kernelSoBuf = PtrToValue(d_aicpu_data);
    cust_aicpu_so_buf.kernelSoBufLen = static_cast<uint32_t>(aicpu_data_length);
    cust_aicpu_so_buf.kernelSoName = PtrToValue(d_so_name);
    cust_aicpu_so_buf.kernelSoNameLen = so_name.size();
    v_cust_so.push_back(cust_aicpu_so_buf);
  }
  if (kernel_name == kDeleteCustOp) {
    (void)cust_aicpu_so_.erase(it);
  }

  void *args = nullptr;
  const size_t args_size = sizeof(CustAicpuSoBuf) * v_cust_so.size();
  GE_CHK_RT_RET(rtMalloc(&args, args_size, RT_MEMORY_HBM));
  allocated_mem.push_back(args);
  GE_CHK_RT(rtMemcpy(args, args_size, v_cust_so.data(), args_size, RT_MEMCPY_HOST_TO_DEVICE));

  BatchLoadOpFromBufArgs batch_cust_so;
  batch_cust_so.soNum = v_cust_so.size();
  batch_cust_so.args = PtrToValue(args);

  void *batch_args = nullptr;
  const uint32_t batch_args_size = sizeof(BatchLoadOpFromBufArgs);
  GE_CHK_RT_RET(rtMalloc(&batch_args, batch_args_size, RT_MEMORY_HBM));
  allocated_mem.push_back(batch_args);
  GE_CHK_RT(rtMemcpy(batch_args, batch_args_size, static_cast<void *>(&batch_cust_so),
                     batch_args_size, RT_MEMCPY_HOST_TO_DEVICE));

  GE_CHK_RT(rtStreamCreate(&stream, 0));
  GE_CHK_RT(rtCpuKernelLaunch(nullptr, kernel_name.c_str(), 1U, batch_args, batch_args_size, nullptr, stream));

  GE_CHK_RT_RET(rtStreamSynchronize(stream));
  GELOGI("Cpu kernel launch task success.");
  return SUCCESS;
}

Status ModelManager::ClearAicpuSo() {
  GE_CHK_STATUS_RET(LaunchKernelCustAicpuSo(kDeleteCustOp),
                    "[Call][LaunchKernelCustAicpuSo] delete cust op so failed.");
  return SUCCESS;
}

Status ModelManager::LaunchCustAicpuSo() {
  GE_CHK_STATUS_RET(LaunchKernelCustAicpuSo(kBatchLoadBuf),
                    "[Call][LaunchKernelCustAicpuSo] launch cust op so failed.");
  return SUCCESS;
}

///
/// @ingroup ge
/// @brief get model memory size and weight
/// @param [in] const ModelData model: model type
/// @param [out] size_t memSize: model memory usage
///           size_t weightSize: model weight and memory size
/// @return SUCCESS success / others failure
///
Status ModelManager::GetModelMemAndWeightSize(const ModelData &model, size_t &mem_size, size_t &weight_size) {
  uint8_t *model_data = nullptr;
  uint32_t model_len = 0U;
  Status ret = ModelParserBase::ParseModelContent(model, model_data, model_len);
  GE_CHK_BOOL_TRUE_EXEC_WITH_LOG(ret != SUCCESS, return ACL_ERROR_GE_PARAM_INVALID, "[Parse][ModelContent] failed!");

  OmFileLoadHelper om_file_helper;
  ret = om_file_helper.Init(model_data, model_len);
  GE_CHK_BOOL_TRUE_EXEC_WITH_LOG(ret != SUCCESS, return ret, "[Init][OmFileHelper] failed, ret:%d", ret);

  const auto &partition_table = *(static_cast<ModelPartitionTable *>(static_cast<void *>(model_data)));
  if (partition_table.num == 1U) {
    REPORT_INNER_ERROR("E19999", "partition_table num in model_data is 1, check invalid");
    GELOGE(ACL_ERROR_GE_PARAM_INVALID, "[Check][Param] om model is error, please use executable om model");
    return ACL_ERROR_GE_PARAM_INVALID;
  }
  ModelPartition task_partition;
  if (om_file_helper.GetModelPartition(ModelPartitionType::TASK_INFO, task_partition) != SUCCESS) {
    GELOGE(ACL_ERROR_GE_EXEC_LOAD_TASK_PARTITION_FAILED, "[Get][ModelPartition] failed.");
    return ACL_ERROR_GE_EXEC_LOAD_TASK_PARTITION_FAILED;
  }

  domi::ModelTaskDef model_task_def;
  if (task_partition.size != 0U) {
    if (!ReadProtoFromArray(task_partition.data, static_cast<int32_t>(task_partition.size), &model_task_def)) {
      GELOGE(ACL_ERROR_GE_EXEC_LOAD_TASK_PARTITION_FAILED, "[Read][Proto] From Array failed.");
      return ACL_ERROR_GE_EXEC_LOAD_TASK_PARTITION_FAILED;
    }
  }

  ModelPartition partition_weight;
  ret = om_file_helper.GetModelPartition(ModelPartitionType::WEIGHTS_DATA, partition_weight);
  GE_CHK_BOOL_TRUE_EXEC_WITH_LOG(ret != SUCCESS, return ACL_ERROR_GE_EXEC_LOAD_WEIGHT_PARTITION_FAILED,
                                 "[Get][ModelPartition] failed. ret = %u", ret);

  mem_size = model_task_def.memory_size();
  weight_size = partition_weight.size;
  return SUCCESS;
}

void ModelManager::GenModelId(uint32_t &id) {
  const std::lock_guard<std::recursive_mutex> lk(map_mutex_);
  ++max_model_id_;
  id = max_model_id_;
}

Status ModelManager::GetOrigInputInfo(const uint32_t model_id, const uint32_t index,
                                      OriginInputInfo &orig_input_info) {
  const auto &davinci_model = GetModel(model_id);
  GE_CHK_BOOL_RET_STATUS(davinci_model != nullptr, ACL_ERROR_GE_EXEC_MODEL_ID_INVALID,
                         "[Get][Model] failed, invalid model_id is %u.", model_id);

  return davinci_model->GetOrigInputInfo(index, orig_input_info);
}

Status ModelManager::GetAllAippInputOutputDims(const uint32_t model_id, const uint32_t index,
                                               std::vector<InputOutputDims> &input_dims,
                                               std::vector<InputOutputDims> &output_dims) {
  const auto &davinci_model = GetModel(model_id);
  GE_CHK_BOOL_RET_STATUS(davinci_model != nullptr, ACL_ERROR_GE_EXEC_MODEL_ID_INVALID,
                         "[Get][Model] failed, invalid model_id is %u.", model_id);

  return davinci_model->GetAllAippInputOutputDims(index, input_dims, output_dims);
}

bool ModelManager::IsDynamicShape(const uint32_t model_id) {
  const auto &model = GetHybridModel(model_id);
  return model != nullptr;
}

Status ModelManager::SyncExecuteModel(const uint32_t model_id, const std::vector<GeTensor> &inputs,
                                      std::vector<GeTensor> &outputs) {
  const auto &model = GetHybridModel(model_id);
  if (model == nullptr) {
    REPORT_INNER_ERROR("E19999", "partition_table num in model_data is 1, check invalid");
    GELOGE(FAILED, "[Check][Param] Hybrid model not found. model id = %u.", model_id);
    return FAILED;
  }

  return model->Execute(inputs, outputs);
}

Status ModelManager::GetOpDescInfo(const uint32_t device_id, const uint32_t stream_id, const uint32_t task_id,
                                   OpDescInfo &desc_info) const {
  for (const auto &model : model_map_) {
    const auto davinci_model = model.second;
    if (davinci_model->GetDeviceId() == device_id) {
      GELOGI("[Get][OpDescInfo] Start to GetOpDescInfo of device_id: %u in davinci model.", device_id);
      if (davinci_model->GetOpDescInfo(stream_id, task_id, desc_info)) {
        GELOGI("[Get][OpDescInfo] Find specific node of stream_id: %u, task_id: %u in davinci model.",
               stream_id, task_id);
        return SUCCESS;
      }
    }
  }
  for (const auto &model : hybrid_model_map_) {
    const auto hybrid_model = model.second;
    if (hybrid_model->GetDeviceId() == device_id) {
      GELOGI("[Get][OpDescInfo] Start to GetOpDescInfo of device_id: %u in hybrid model.", device_id);
      if (hybrid_model->GetOpDescInfo(stream_id, task_id, desc_info)) {
        GELOGI("[Get][OpDescInfo] Find specific node of stream_id: %u, task_id: %u in hybrid model.",
               stream_id, task_id);
        return SUCCESS;
      }
    }
  }
  return FAILED;
}

Status ModelManager::EnableExceptionDump(const std::map<std::string, std::string> &run_options) {
  char_t record_path[MMPA_MAX_PATH]{};
  if ((mmGetEnv(kEnvNpuCollectPath, &record_path[0U], static_cast<uint32_t>(MMPA_MAX_PATH)) == EN_OK) &&
      (record_path[0U] != '\0')) {
    dump_exception_flag_ = true;
    GE_CHK_RT_RET(rtSetTaskFailCallback(&ExceptionCallback));
    return SUCCESS;
  }
  const auto iter = run_options.find(OPTION_EXEC_ENABLE_EXCEPTION_DUMP);
  if (iter != run_options.end()) {
    GELOGI("Find option enable_exeception_dump is %s", iter->second.c_str());
    if (iter->second == "1") {
      dump_exception_flag_ = true;
      GE_CHK_RT_RET(rtSetTaskFailCallback(&ExceptionCallback));
    } else {
      GELOGI("Option enable exception dump is %s", iter->second.c_str());
    }
  } else {
    GELOGI("Not find option enable exception dump");
  }
  return SUCCESS;
}

Status ModelManager::LaunchKernelCheckAicpuOp(const std::vector<std::string> &aicpu_optype_list,
                                              const std::vector<std::string> &aicpu_tf_optype_list) {
  const std::string kernel_name = "checkOpType";
  GELOGI("LaunchKernelCheckAicpuOpType in, kernel name %s", kernel_name.c_str());
  const std::lock_guard<std::mutex> lk(cust_aicpu_mutex_);
  std::vector<SysOpInfo> req_aicpu_op_info_list;

  if (aicpu_optype_list.empty() && aicpu_tf_optype_list.empty()) {
    GELOGI("No need to check aicpu op type.");
    return SUCCESS;
  }

  std::vector<void *> allocated_mem;

  const size_t aicpu_op_nums = aicpu_optype_list.size();
  const size_t tf_op_nums = aicpu_tf_optype_list.size();
  const size_t op_nums = aicpu_op_nums + tf_op_nums;
  const std::function<void()> callback = [&allocated_mem]() {
    for (auto &mem : allocated_mem) {
      GE_CHK_RT(rtFree(mem));
    }
  };
  GE_MAKE_GUARD(release, callback);

  // malloc sysOpInfoList in SysOpCheckInfo
  void *d_req_op_list = nullptr;
  GE_CHK_RT_RET(rtMalloc(&d_req_op_list, op_nums * sizeof(SysOpInfo), RT_MEMORY_HBM));
  allocated_mem.push_back(d_req_op_list);

  // malloc sysOpInfoList in SysOpCheckResp
  void *d_res_op_list = nullptr;
  GE_CHK_RT_RET(rtMalloc(&d_res_op_list, op_nums * sizeof(SysOpInfo), RT_MEMORY_HBM));
  allocated_mem.push_back(d_res_op_list);

  // malloc returnCodeList in SysOpCheckResp
  void *d_ret_code_list = nullptr;
  GE_CHK_RT_RET(rtMalloc(&d_ret_code_list, op_nums * sizeof(ReturnCode), RT_MEMORY_HBM));
  allocated_mem.push_back(d_ret_code_list);

  for (const auto &op_type : aicpu_optype_list) {
    SysOpInfo op_info;
    // malloc op_type name in SysOpInfo
    void *d_op_type_name = nullptr;
    GE_CHK_RT_RET(rtMalloc(&d_op_type_name, op_type.length(), RT_MEMORY_HBM));

    allocated_mem.push_back(d_op_type_name);
    GE_CHK_RT(rtMemcpy(d_op_type_name, op_type.length(), op_type.c_str(), op_type.length(), RT_MEMCPY_HOST_TO_DEVICE));
    op_info.opType = PtrToValue(d_op_type_name);
    op_info.opLen = op_type.length();
    op_info.kernelsType = CPU_KERNEL;
    req_aicpu_op_info_list.emplace_back(op_info);
  }

  for (const auto &op_type : aicpu_tf_optype_list) {
    SysOpInfo op_info;
    // malloc op_type name in SysOpInfo
    void *d_op_type_name = nullptr;
    GE_CHK_RT_RET(rtMalloc(&d_op_type_name, op_type.length(), RT_MEMORY_HBM));

    allocated_mem.push_back(d_op_type_name);
    GE_CHK_RT(rtMemcpy(d_op_type_name, op_type.size(), op_type.c_str(), op_type.size(), RT_MEMCPY_HOST_TO_DEVICE));
    op_info.opType = PtrToValue(d_op_type_name);
    op_info.opLen = op_type.size();
    op_info.kernelsType = TF_KERNEL;
    req_aicpu_op_info_list.emplace_back(op_info);
  }
  GELOGI("Check aicpu op all attr size: %zu, real attr size: %zu.", op_nums, req_aicpu_op_info_list.size());
  GE_CHK_RT(rtMemcpy(d_req_op_list, sizeof(SysOpInfo) * req_aicpu_op_info_list.size(), req_aicpu_op_info_list.data(),
                     sizeof(SysOpInfo) * req_aicpu_op_info_list.size(), RT_MEMCPY_HOST_TO_DEVICE));

  SysOpCheckInfo op_check_info_req{};
  SysOpCheckResp op_check_info_res{};
  op_check_info_req.opListNum = op_nums;
  op_check_info_req.offSetLen = sizeof(SysOpCheckInfo);
  op_check_info_req.sysOpInfoList = PtrToValue(d_req_op_list);

  op_check_info_res.opListNum = 0U;
  op_check_info_res.isWithoutJson = false;
  op_check_info_res.returnCodeList = PtrToValue(d_ret_code_list);
  op_check_info_res.sysOpInfoList = PtrToValue(d_res_op_list);

  void *args = nullptr;
  const uint32_t args_size = sizeof(SysOpCheckInfo) + sizeof(SysOpCheckResp);
  GE_CHK_RT_RET(rtMalloc(&args, args_size, RT_MEMORY_HBM));

  allocated_mem.push_back(args);
  GE_CHK_RT(rtMemcpy(args, sizeof(SysOpCheckInfo), static_cast<void *>(&op_check_info_req), sizeof(SysOpCheckInfo),
                     RT_MEMCPY_HOST_TO_DEVICE));
  GE_CHK_RT(rtMemcpy(ValueToPtr(PtrToValue(args) + op_check_info_req.offSetLen),
                     sizeof(SysOpCheckResp), static_cast<void *>(&op_check_info_res), sizeof(SysOpCheckResp),
                     RT_MEMCPY_HOST_TO_DEVICE));

  rtStream_t stream = nullptr;
  GE_CHK_RT(rtStreamCreate(&stream, 0));
  GE_CHK_RT(rtCpuKernelLaunch(nullptr, kernel_name.c_str(), 1U, args, args_size, nullptr, stream));

  GE_CHK_RT_RET(rtStreamSynchronize(stream));

  // Check the response
  const void *const d_op_check_info_res = ValueToPtr(PtrToValue(args) + op_check_info_req.offSetLen);
  GE_CHK_RT(rtMemcpy(&op_check_info_res, sizeof(SysOpCheckResp), d_op_check_info_res, sizeof(SysOpCheckResp),
                     RT_MEMCPY_DEVICE_TO_HOST));

  if (op_check_info_res.isWithoutJson) {
    GELOGI("No need to check aicpu in this scenoria.");
    GE_CHK_RT(rtStreamDestroy(stream));
    return SUCCESS;
  }
  const uint64_t res_op_nums = op_check_info_res.opListNum;
  GELOGI("Check aicpu type, is without json: %d, res op num: %lu.",
         static_cast<int32_t>(op_check_info_res.isWithoutJson), res_op_nums);
  if (res_op_nums != 0U) {
    std::vector<ReturnCode> res_ret_code_list(res_op_nums);
    std::vector<SysOpInfo> res_aicpu_op_info_list(res_op_nums);
    GE_CHK_RT(rtMemcpy(res_ret_code_list.data(), sizeof(ReturnCode) * res_op_nums,
                       ValueToPtr(op_check_info_res.returnCodeList),
                       sizeof(ReturnCode) * res_op_nums, RT_MEMCPY_DEVICE_TO_HOST));
    GE_CHK_RT(rtMemcpy(res_aicpu_op_info_list.data(), sizeof(SysOpInfo) * res_op_nums,
                       ValueToPtr(op_check_info_res.sysOpInfoList),
                       sizeof(SysOpInfo) * res_op_nums, RT_MEMCPY_DEVICE_TO_HOST));
    if ((res_ret_code_list.size() != res_aicpu_op_info_list.size()) || (res_ret_code_list.size() != res_op_nums)) {
      REPORT_INNER_ERROR("E19999", "res_ret_code.size:%zu res_aicpu_op_info.size:%zu res_op_nums:%lu not equal, "
                         "check invalid", res_ret_code_list.size(), res_aicpu_op_info_list.size(), res_op_nums);
      GELOGE(FAILED, "[Check][Param] Number:%zu of retcode is not equal to number:%zu of op type or not equal %lu.",
             res_ret_code_list.size(), res_aicpu_op_info_list.size(), res_op_nums);
      GE_CHK_RT(rtStreamDestroy(stream));
      return FAILED;
    }
    std::string fail_reason;
    for (uint64_t i = 0U; i < res_op_nums; i++) {
      const SysOpInfo &aicpu_info = res_aicpu_op_info_list.at(i);
      GELOGI("Not support aicpu op type: %lu, kernel_type:%d, opLen:%lu, ret_code:%d", aicpu_info.opType,
             aicpu_info.kernelsType, aicpu_info.opLen, res_ret_code_list.at(i));
      std::vector<char> op_name(kOpNameMaxSize);
      GE_CHK_RT(rtMemcpy(op_name.data(), aicpu_info.opLen, ValueToPtr(aicpu_info.opType), aicpu_info.opLen,
                         RT_MEMCPY_DEVICE_TO_HOST));
      const std::string kernel_type = (aicpu_info.kernelsType == TF_KERNEL) ? "TF_KERNEL" : "CPU_KERNEL";
      fail_reason += "op_type: " + std::string(op_name.data()) + " kernel_type: " + kernel_type +
                     " ret code:" + std::to_string(res_ret_code_list.at(i)) + "<0: op_type, 1: format, 2: datatype>\n";
    }
    fail_reason += "not support.";
    REPORT_INNER_ERROR("E19999", "Check aicpu op_type failed, details:%s", fail_reason.c_str());
    GELOGE(FAILED, "[Check][Param] Check aicpu op_type failed. details:%s", fail_reason.c_str());
    GE_CHK_RT(rtStreamDestroy(stream));
    return FAILED;
  }

  GE_CHK_RT(rtStreamDestroy(stream));
  GELOGI("Cpu kernel launch check optype task success.");
  return SUCCESS;
}

Status ModelManager::CheckAicpuOpList(const GeModelPtr ge_model) {
  std::vector<std::string> aicpu_optype_list;
  std::vector<std::string> aicpu_tf_optype_list;
  const bool aicpu_need_check = AttrUtils::GetListStr(ge_model, "needCheckCpu", aicpu_optype_list);
  const bool tf_need_check = AttrUtils::GetListStr(ge_model, "needCheckTf", aicpu_tf_optype_list);
  if ((!aicpu_need_check) && (!tf_need_check)) {
    GELOGI("Model:%s No need to check aicpu optype.", ge_model->GetName().c_str());
    return SUCCESS;
  }
  GE_CHK_STATUS_RET(LaunchKernelCheckAicpuOp(aicpu_optype_list, aicpu_tf_optype_list),
                    "[Call][LaunchKernelCheckAicpuOp] failed.");
  return SUCCESS;
}

Status ModelManager::LoadAsHeterogeneousModel(const uint32_t model_id,
                                              const FlowModelPtr &flow_model,
                                              const std::shared_ptr<ModelListener> &listener) {
  auto *const execution_runtime = ExecutionRuntime::GetInstance();
  GE_CHECK_NOTNULL(execution_runtime);
  auto &model_deployer = execution_runtime->GetModelDeployer();
  DeployResult deploy_result;
  GE_CHK_STATUS_RET_NOLOG(model_deployer.DeployModel(flow_model, {}, {}, deploy_result));
  auto executor = MakeShared<HeterogeneousModelExecutor>(flow_model, deploy_result);
  executor->SetModelId(model_id);
  executor->SetListener(listener);
  executor->SetDeviceId(GetContext().DeviceId());
  GE_CHK_STATUS_RET_NOLOG(executor->Initialize());

  const std::lock_guard<std::recursive_mutex> lk(map_mutex_);
  (void)heterogeneous_model_map_.emplace(model_id, std::move(executor));
  return SUCCESS;
}

bool ModelManager::UnloadHeterogeneousModel(const uint32_t model_id) {
  const auto executor = GetHeterogeneousModelExecutor(model_id);
  if (executor == nullptr) {
    return false;
  }
  auto *const execution_runtime = ExecutionRuntime::GetInstance();
  GE_RT_FALSE_CHECK_NOTNULL(execution_runtime);

  const auto deployed_model_id = executor->GetDeployedModelId();
  {
    // minimize lock time
    const std::lock_guard<std::recursive_mutex> lk(map_mutex_);
    (void)heterogeneous_model_map_.erase(model_id);
  }

  (void)execution_runtime->GetModelDeployer().Undeploy(deployed_model_id);
  return true;
}

bool ModelManager::IsHeterogeneous(const uint32_t model_id) const {
  return heterogeneous_model_map_.find(model_id) != heterogeneous_model_map_.end();
}

std::shared_ptr<HeterogeneousModelExecutor> ModelManager::GetHeterogeneousModelExecutor(const uint32_t model_id) {
  const std::lock_guard<std::recursive_mutex> lk(map_mutex_);
  const std::map<uint32_t, std::shared_ptr<HeterogeneousModelExecutor>>::const_iterator &it =
      heterogeneous_model_map_.find(model_id);
  if (it == heterogeneous_model_map_.cend()) {
    return nullptr;
  }
  return it->second;
}

Status ModelManager::ExecuteHeterogeneousModel(const uint32_t model_id,
                                               const std::vector<GeTensor> &inputs,
                                               std::vector<GeTensor> &outputs) {
  const auto executor = GetHeterogeneousModelExecutor(model_id);
  GE_CHECK_NOTNULL(executor);
  return executor->Execute(inputs, outputs);
}

uint32_t ModelManager::GetRunningFlag(const uint32_t model_id) {
  const auto hybrid_model = GetHybridModel(model_id);
  if (hybrid_model != nullptr) {
    return static_cast<uint32_t>(hybrid_model->GetRunningFlag());
  }

  const auto davinci_model = GetModel(model_id);
  return (davinci_model != nullptr) ? static_cast<uint32_t>(davinci_model->GetRunningFlag()) : 0U;
}

uint32_t ModelManager::GetDataInputerSize(const uint32_t model_id) {
  const auto hybrid_model = GetHybridModel(model_id);
  if (hybrid_model != nullptr) {
    return hybrid_model->GetDataInputerSize();
  }

  const auto davinci_model = GetModel(model_id);
  return (davinci_model != nullptr) ? davinci_model->GetDataInputerSize() : 0U;
}

Status ModelManager::SetCallback(const uint32_t model_id, const GeRootModelPtr &ge_root_model,
                                 const RunAsyncCallback &callback) {
  if (IsNeedHybridLoad(*ge_root_model)) {
    const auto model = GetHybridModel(model_id);
    GE_CHECK_NOTNULL(model);
    return model->SetRunAsyncListenerCallback(callback);
  } else {
    const auto model = GetModel(model_id);
    GE_CHECK_NOTNULL(model);
    return model->SetRunAsyncListenerCallback(callback);
  }
}

Status ModelManager::ModelSubscribe(const uint32_t graph_id) {
  auto &prof_mgr = ProfilingManager::Instance();
  const auto &subcribe_info = prof_mgr.GetSubscribeInfo();
  if (!subcribe_info.is_subscribe) {
    return SUCCESS;
  }

  uint32_t model_id = 0U;
  if (prof_mgr.GetModelIdFromGraph(graph_id, model_id) != SUCCESS) {
    return FAILED;
  }

  return ProfModelSubscribe(subcribe_info.prof_switch, model_id, graph_id);
}

Status ModelManager::ProfModelSubscribe(const uint64_t module, const uint32_t model_id, const uint32_t graph_id) {
  auto &prof_mgr = ProfilingManager::Instance();
  const auto hybrid_davinci_model = GetHybridModel(model_id);
  if (hybrid_davinci_model != nullptr) {
    GE_CHK_STATUS_RET_NOLOG(prof_mgr.CheckInitForSubscribe(module, hybrid_davinci_model->GetDeviceId()));
    return hybrid_davinci_model->ReportProfilingData();
  }

  const auto davinci_model = GetModel(model_id);
  if (davinci_model != nullptr) {
    GE_CHK_STATUS_RET_NOLOG(prof_mgr.CheckInitForSubscribe(module, davinci_model->GetDeviceId()));
    return davinci_model->ReportProfilingData(graph_id);
  }

  GELOGE(FAILED, "[Call][GetModel] failed, model_id:%u", model_id);
  return FAILED;
}

void ModelManager::RecordTsSnapshot() {
  while (!stop_monitor_) {
    const INT32 trigger_fd = mmOpen(trigger_file_name_.c_str(), M_RDONLY);
    if (trigger_fd >= 0) {
      (void)mmClose(trigger_fd);
      for (uint32_t i = 0U; (i <= kRecordTimes) && (!stop_monitor_); ++i) {
        GE_CHK_RT(rtGetDevMsg(RT_GET_DEV_RUNNING_STREAM_SNAPSHOT_MSG, &getDevMsgCallback));

        const auto wait_interval = (i < kRecordTimes) ? kRecordIntervalMs : kTriggerScanIntervalMs;
        const std::chrono::milliseconds wait_for_ms(static_cast<uint32_t>(wait_interval));
        std::unique_lock<std::mutex> lk(monitor_mtx_);
        (void)monitor_cv_.wait_for(lk, wait_for_ms, [&]{ return stop_monitor_; });
      }
      (void)mmUnlink(trigger_file_name_.c_str());
    } else {
      std::unique_lock<std::mutex> lk(monitor_mtx_);
      (void)monitor_cv_.wait_for(lk, std::chrono::milliseconds(kTriggerScanIntervalMs), [&]{ return stop_monitor_; });
    }
  }

  GELOGI("Finish recording snapshot because of stopping");
}

void ModelManager::CreateMonitorThread() {
  char_t record_path[MMPA_MAX_PATH]{};
  const auto ret = mmGetEnv(kEnvNpuCollectPath, &record_path[0U], static_cast<uint32_t>(MMPA_MAX_PATH));
  if ((ret == EN_OK) && (!(std::string(&record_path[0U]).empty()))) {
    GELOGI("Create thread to monitor snapshot");
    stop_monitor_ = false;
    trigger_file_name_ = std::string(&record_path[0U]) + kPathSeparator + kTriggerFile;

    (void)CreateDirectory(record_path);
    record_file_name_ = std::string(&record_path[0U]) + kPathSeparator + kRecordFilePrefix + std::to_string(mmGetPid());
    monitor_thread_ = std::thread(&ModelManager::RecordTsSnapshot, this);
  } else {
    GELOGI("Abandon creating thread to monitor snapshot for there's no valid env: %s", kEnvNpuCollectPath);
  }
}

void ModelManager::ClearMonitorThread() {
  GELOGI("Start to clear monitor thread");
  {
    const std::unique_lock<std::mutex> lk(monitor_mtx_);
    stop_monitor_ = true;
    monitor_cv_.notify_all();
  }

  if (monitor_thread_.joinable()) {
    monitor_thread_.join();
  }
  GELOGI("Finish to clear monitor thread");
}

void ModelManager::getDevMsgCallback(const char_t *const msg, const uint32_t len) {
  const int32_t open_flags = static_cast<int32_t>(static_cast<uint32_t>(M_CREAT) |
                                                  static_cast<uint32_t>(M_WRONLY) |
                                                  static_cast<uint32_t>(M_APPEND));
  const INT32 record_fd = mmOpen(record_file_name_.c_str(), open_flags);
  if ((record_fd == EN_INVALID_PARAM) || (record_fd == EN_ERROR)) {
    GELOGE(FAILED, "Fail to open file[%s] to record snapshot", record_file_name_.c_str());
    return;
  }

  const mmSsize_t write_count = mmWrite(record_fd, ValueToPtr(PtrToValue(msg)), len);
  if ((write_count != EN_INVALID_PARAM) && (write_count != EN_ERROR)) {
    static char_t kRecordSeperate[] = "@@@\n";
    (void)mmWrite(record_fd, static_cast<void *>(&kRecordSeperate[0]), static_cast<uint32_t>(sizeof(kRecordSeperate)));
  }
  (void)mmClose(record_fd);
}
}  // namespace ge
