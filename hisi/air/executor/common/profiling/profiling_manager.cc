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

#include "common/profiling/profiling_manager.h"

#include <securec.h>
#include <algorithm>
#include <thread>

#include "framework/common/debug/ge_log.h"
#include "framework/common/debug/log.h"
#include "framework/common/profiling_definitions.h"
#include "graph/utils/type_utils.h"
#include "toolchain/prof_common.h"
#include "toolchain/prof_acl_api.h"
#include "runtime/dev.h"
#include "mmpa/mmpa_api.h"
#include "graph/load/model_manager/model_manager.h"

namespace {
const int32_t kMaxDeviceNum = 256;
const std::string kConfigNumsdev = "devNums";
const std::string kConfigDevIdList = "devIdList";
const std::string kProfStart = "prof_start";
const std::string kProfStop = "prof_stop";
const std::string kProfModelSubscribe = "prof_model_subscribe";
const std::string kProfModelUnsubscribe = "prof_model_cancel_subscribe";
const std::string kInput = "input";
const std::string kOutput = "output";
const std::string kDynShapeOpExecuteData = "dynamic_op_execute";
const std::string kTensorTagName = "tensor_data_info";
const std::string kSingleOpTensorTagName = "single_op_tensor_info";
const std::string kTaskTagName = "task_desc_info";
const std::string kSingleOpTaskTagName = "single_op_task_info";

std::map<std::string, MsprofGeTaskType> kTaskType2Enum {
  {"AI_CORE", MSPROF_GE_TASK_TYPE_AI_CORE},
  {"AI_CPU", MSPROF_GE_TASK_TYPE_AI_CPU},
  {"AIV", MSPROF_GE_TASK_TYPE_AIV}
};

std::map<std::string, MsprofGeTensorType> kTensorType2Enum {
  {kInput, MSPROF_GE_TENSOR_TYPE_INPUT},
  {kOutput, MSPROF_GE_TENSOR_TYPE_OUTPUT}
};

std::map<std::string, MsprofGeShapeType> kShapeType2Enum {
  {"static", MSPROF_GE_SHAPE_TYPE_STATIC},
  {"dynamic", MSPROF_GE_SHAPE_TYPE_DYNAMIC}
};

void BuildTensorBaseInfo(const ge::TaskDescInfo &task, const uint32_t model_id, const uint64_t timestamp,
                         MsprofGeProfTensorData &tensor_info) {
  tensor_info.curIterNum = static_cast<uint64_t>(task.cur_iter_num);
  tensor_info.modelId = model_id;
  tensor_info.streamId = task.stream_id;
  tensor_info.taskId = task.task_id;
  const auto ret = memcpy_s(tensor_info.reserve, sizeof(tensor_info.reserve), &timestamp, sizeof(timestamp));
  if (ret != EOK) {
    GELOGW("[Profilng] memcpy_s failed.");
  }
}

void BuildTensorData(const ge::TaskDescInfo &task, const size_t index, MsprofGeTensorData &tensor_data) {
  const auto BuildTensor = [&tensor_data](const std::string &tensor_type, const std::vector<ge::Format> &format_lst,
                                          const std::vector<ge::DataType> &data_type_lst,
                                          const std::vector<std::vector<int64_t>> &shape_lst,
                                          const size_t tensor_index) {
    tensor_data.tensorType = static_cast<uint32_t>(kTensorType2Enum[tensor_type]);

    // when enum Format is changed, profiling analyze needs to be synchronized
    const ge::Format format = format_lst[tensor_index];
    tensor_data.format = static_cast<uint32_t>(format);

    // when enum DataType is changed, profiling analyze needs to be synchronized
    const ge::DataType data_type = data_type_lst[tensor_index];
    tensor_data.dataType = (static_cast<uint32_t>(data_type) < static_cast<uint32_t>(ge::DT_MAX))
                               ? static_cast<uint32_t>(data_type)
                               : static_cast<uint32_t>(ge::DT_UNDEFINED);

    const auto shape_size = shape_lst[tensor_index].size();
    if (shape_size < static_cast<uint64_t>(MSPROF_GE_TENSOR_DATA_SHAPE_LEN)) {
      std::copy(shape_lst[tensor_index].begin(), shape_lst[tensor_index].begin() + static_cast<int64_t>(shape_size),
                tensor_data.shape);
      tensor_data.shape[shape_size] = 0U;
    } else {
      std::copy(shape_lst[tensor_index].begin(), shape_lst[tensor_index].begin() + MSPROF_GE_TENSOR_DATA_SHAPE_LEN,
                tensor_data.shape);
    }
  };

  const size_t input_size = task.input_format.size();
  if (index < input_size) {
    // when current index is smaller than input_size, build tensor by input tensor
    BuildTensor(kInput, task.input_format, task.input_data_type, task.input_shape, index);
  } else {
    // when current index is bigger than input_size, build tensor by output tensor, use index - input_size as
    // index of output tensor
    BuildTensor(kOutput, task.output_format, task.output_data_type, task.output_shape, index - input_size);
  }
}
}  // namespace


namespace ge {
ProfilingManager::ProfilingManager() {}

ProfilingManager::~ProfilingManager() {}

ProfilingManager &ProfilingManager::Instance() {
  static ProfilingManager profiling_manager;
  return profiling_manager;
}

void ProfilingManager::RegisterElement(int64_t &idx, const std::string &element) {
  if (ProfilingManager::Instance().ProfilingModelLoadOn() &&
      ProfilingProperties::Instance().IsDynamicShapeProfiling()) {
    uint64_t hash_id = profiling::kInvalidHashId;
    const Status stat_ret = ProfilingManager::Instance().QueryHashId(element, hash_id);
    if (stat_ret != SUCCESS) {
      GELOGW("[Get][QueryHashId]Failed, ret 0x%X", stat_ret);
    }
    idx = profiling::ProfilingContext::GetInstance().RegisterStringHash(hash_id, element);
  } else {
    idx = profiling::ProfilingContext::GetInstance().RegisterString(element);
  }
}

void ProfilingManager::ProfilingTaskDescInfo(const uint32_t model_id, const int32_t device_id,
                                             const std::vector<TaskDescInfo> &task_desc_info) {
  const thread_local int32_t thread_id = mmGetTid();
  GELOGD("[Profiling] repoort task info size is %lu", task_desc_info.size());
  for (const auto &task : task_desc_info) {
    MsprofGeProfTaskData task_info{};
    const std::map<std::string, MsprofGeTaskType>::const_iterator task_iter = kTaskType2Enum.find(task.task_type);
    if (task_iter != kTaskType2Enum.cend()) {
      task_info.taskType = static_cast<uint32_t>(task_iter->second);
    } else {
      GELOGW("[Profiling] task_type is not in kTaskType2Enum, which is %s", task.task_type.c_str());
      continue;
    }
    SetAlternativeValue(MSPROF_MIX_DATA_STRING_LEN, task.op_name, task_info.opName);
    SetAlternativeValue(MSPROF_GE_OP_TYPE_LEN, task.op_type, task_info.opType);
    const std::map<std::string, MsprofGeShapeType>::const_iterator shape_iter = kShapeType2Enum.find(task.shape_type);
    if (shape_iter != kShapeType2Enum.cend()) {
      task_info.shapeType = static_cast<uint32_t>(shape_iter->second);
    } else {
      GELOGW("[Profiling] shape_type is not in kShapeType2Enum, which is %s", task.shape_type.c_str());
      continue;
    }
    task_info.blockDims = task.block_dim;
    task_info.curIterNum = static_cast<uint64_t>(task.cur_iter_num);
    task_info.modelId = model_id;
    task_info.streamId = task.stream_id;
    task_info.taskId = task.task_id;
    const mmTimespec tick_count = mmGetTickCount();
    const int64_t prof_time = (tick_count.tv_sec * 1000000000) + tick_count.tv_nsec;
    task_info.timeStamp = static_cast<uint64_t>(prof_time);
    task_info.threadId = static_cast<uint32_t>(thread_id);
    task_info.contextId = task.context_id;
    const std::string tag_name = (model_id == kInvalidModelId) ? kSingleOpTaskTagName : kTaskTagName;
    ReportData(device_id, &task_info, sizeof(task_info), tag_name);
    if (ProfilingProperties::Instance().IsOpDetailProfiling()) {
      const std::string tag_name_ = (model_id == kInvalidModelId) ? kSingleOpTensorTagName : kTensorTagName;
      ProfilingOpInputOutInfo(task, model_id, device_id, task_info.timeStamp, tag_name_);
    }
  }
}

void ProfilingManager::ProfilingOpInputOutInfo(const TaskDescInfo &task, const uint32_t model_id,
    const int32_t device_id, const uint64_t timestamp, const std::string &tag_name) {
  const size_t total_size = task.input_format.size() + task.output_format.size();
  GELOGD("[Profiling] Report tensor info, size is %lu", total_size);

  // one tensor_info can only contain 5 tensor data.
  const size_t index = total_size / static_cast<size_t>(MSPROF_GE_TENSOR_DATA_NUM);
  for (size_t i = 0U; i < index; ++i) {
    MsprofGeProfTensorData tensor_info{};
    BuildTensorBaseInfo(task, model_id, timestamp, tensor_info);
    tensor_info.tensorNum = static_cast<uint32_t>(MSPROF_GE_TENSOR_DATA_NUM);
    for (size_t j = 0U; j < static_cast<size_t>(MSPROF_GE_TENSOR_DATA_NUM); ++j) {
      MsprofGeTensorData &tensor_data = tensor_info.tensorData[j];
      const size_t tensor_index = (i * static_cast<size_t>(MSPROF_GE_TENSOR_DATA_NUM)) + j;
      BuildTensorData(task, tensor_index, tensor_data);
    }
    ReportData(device_id, &tensor_info, sizeof(tensor_info), tag_name);
  }

  const size_t remain_index = total_size % static_cast<size_t>(MSPROF_GE_TENSOR_DATA_NUM);
  if (remain_index == 0UL) {
    return;
  }
  MsprofGeProfTensorData tensor_info{};
  BuildTensorBaseInfo(task, model_id, timestamp, tensor_info);
  tensor_info.tensorNum = remain_index;
  for (size_t i = 0U; i < remain_index; ++i) {
    MsprofGeTensorData &tensor_data = tensor_info.tensorData[i];
    const size_t tensor_index = (index * static_cast<size_t>(MSPROF_GE_TENSOR_DATA_NUM)) + i;
    BuildTensorData(task, tensor_index, tensor_data);
  }
  ReportData(device_id, &tensor_info, sizeof(tensor_info), tag_name);
}

Status ProfilingManager::ProfileStepInfo(const uint64_t index_id, const uint32_t model_id, const uint16_t tag_id,
                                         const rtStream_t stream, const uint32_t device_id) {
  if ((!ProfilingProperties::Instance().IsLoadProfiling()) && (subscribe_count_ == 0U)) {
    GELOGD("Profiling is not turned on, no need to profile step info.");
    return SUCCESS;
  }

  GELOGD("Profiling Step Info TraceTask execute async start, index_id = %lu, model_id = %u, tag_id = %u",
         index_id, model_id, static_cast<uint32_t>(tag_id));
  rtError_t rt_ret = rtProfilerTraceEx(index_id, static_cast<uint64_t>(model_id), tag_id, stream);
  if (rt_ret != RT_ERROR_NONE) {
    GELOGE(RT_FAILED, "[Call][rtProfilerTraceEx]Failed, ret 0x%X", rt_ret);
    REPORT_CALL_ERROR("E19999", "Call rtProfilerTraceEx failed, ret 0x%X", rt_ret);
    return RT_ERROR_TO_GE_STATUS(rt_ret);
  }
  GELOGD("Profiling Step Info TraceTask execute async success, index_id = %lu, model_id = %u, tag_id = %u",
         index_id, model_id, static_cast<uint32_t>(tag_id));
  uint32_t task_id = 0U;
  uint32_t stream_id = 0U;
  rt_ret = rtGetTaskIdAndStreamID(&task_id, &stream_id);
  if (rt_ret != RT_ERROR_NONE) {
    GELOGE(RT_FAILED, "[Get][RtsInfo]Task_id and stream_id failed, ret 0x%X", rt_ret);
    REPORT_CALL_ERROR("E19999", "Get task_id and stream_id failed, ret 0x%X", rt_ret);
    return RT_ERROR_TO_GE_STATUS(rt_ret);
  }
  GELOGD("Get profiling args, task_id[%u], stream_id[%u]", task_id, stream_id);
  MsprofGeProfStepData step_info{};
  step_info.modelId = model_id;
  step_info.curIterNum = index_id;
  step_info.streamId = stream_id;
  step_info.taskId = task_id;
  const mmTimespec tick_count = mmGetTickCount();
  const int64_t prof_time = (tick_count.tv_sec * 1000000000) + tick_count.tv_nsec;
  step_info.timeStamp = static_cast<uint64_t>(prof_time);
  const thread_local int32_t thread_id = mmGetTid();
  step_info.threadId = static_cast<uint32_t>(thread_id);
  step_info.tag = static_cast<uint8_t>(tag_id);
  ReportData(static_cast<int32_t>(device_id), &step_info, sizeof(step_info), "step_info");
  return SUCCESS;
}

void ProfilingManager::Record(const uint64_t element, const uint64_t event, const int64_t start, const int64_t end) {
  GELOGI("Record task profiler element hash[%lu], event hash[%lu], start[%lu], end[%lu]",
         element, event, start, end);
  const int32_t tid = mmGetTid();
  struct MsprofGeProfHostSchData host_sch_data;
  int32_t device_id = 0;
  const rtError_t rt_ret = rtGetDevice(&device_id);
  if (rt_ret != RT_ERROR_NONE) {
    GELOGW("[Get][LogicDeviceId]Failed, ret 0x%X", rt_ret);
    return;
  }
  host_sch_data.element = element;
  host_sch_data.event = event;
  host_sch_data.threadId = static_cast<uint32_t>(tid);
  host_sch_data.startTime = static_cast<uint64_t>(start);
  host_sch_data.endTime = static_cast<uint64_t>(end);
  ProfilingManager &prof_mgr = ProfilingManager::Instance();
  prof_mgr.ReportData(device_id, &host_sch_data, sizeof(host_sch_data), kDynShapeOpExecuteData);
}

void ProfilingManager::ReportData(const int32_t device_id, void *const data, const size_t data_size,
                                  const std::string &tag_name) {
  ReporterData reporter_data{};
  reporter_data.deviceId = device_id;
  reporter_data.data = PtrToPtr<void, uint8_t>(data);
  reporter_data.dataLen = data_size;
  const errno_t ret = strncpy_s(reporter_data.tag, sizeof(reporter_data.tag), tag_name.c_str(), tag_name.size());
  if (ret != EOK) {
    GELOGE(FAILED, "Report data tag [%s] memcpy error!", tag_name.c_str());
    return;
  }
  const std::lock_guard<std::mutex> lock(mutex_report_);
  const Status cb_ret = CallMsprofReport(reporter_data);
  if (cb_ret != SUCCESS) {
    GELOGE(cb_ret, "Reporter data [%s] failed, ret:%u", tag_name.c_str(), cb_ret);
    return;
  }
}

Status ProfilingManager::QueryHashId(const std::string &src_str, uint64_t &hash_id) const {
  // when some profiling data size exceeds the specified size, query its hashId instead.
  return profiling::ProfilingContext::QueryHashId(reporter_callback_, src_str, hash_id);
}

void ProfilingManager::ReportProfilingData(const uint32_t model_id, const std::vector<TaskDescInfo> &task_desc_info) {
  int32_t logic_device_id = 0;
  const rtError_t rt_ret = rtGetDevice(&logic_device_id);
  if (rt_ret != RT_ERROR_NONE) {
    GELOGE(RT_FAILED, "[Get][LogicDeviceId]Failed, ret 0x%X", rt_ret);
    REPORT_CALL_ERROR("E19999", "Get logic device id failed, ret 0x%X", rt_ret);
    return;
  }
  GELOGD("current logic_device_id:%d", logic_device_id);
  GELOGD("start ProfilingTaskDescInfo, task desc info size is %zu", task_desc_info.size());
  ProfilingTaskDescInfo(model_id, logic_device_id, task_desc_info);
  GELOGD("Report profiling data for GE end.");
}

void ProfilingManager::UpdateSubscribeDeviceModuleMap(const std::string &prof_type, const uint32_t device_id,
                                                      const uint64_t module) {
  if (prof_type == kProfModelSubscribe) {
    if (subs_dev_module_.find(device_id) != subs_dev_module_.end()) {
      subs_dev_module_[device_id].subscribe_count++;
    } else {
      DeviceSubsInfo dev_info;
      dev_info.module = module;
      dev_info.subscribe_count = 1U;
      subs_dev_module_[device_id] = dev_info;
    }
  } else if (prof_type == kProfModelUnsubscribe) {
    const auto iter = subs_dev_module_.find(device_id);
    if (iter != subs_dev_module_.end()) {
      if (iter->second.subscribe_count > 0U) {
        iter->second.subscribe_count--;
      }
      if (iter->second.subscribe_count == 0U) {
        (void)subs_dev_module_.erase(iter);
      }
    }
  } else {
    GELOGI("No need to update device_id module map.");
  }
}

Status ProfilingManager::CheckInitForSubscribe(const uint64_t module, const uint32_t device) {
  const std::lock_guard<std::mutex> lock(mutex_);
  const uint64_t model_load_mask = module & static_cast<uint64_t>(PROF_MODEL_LOAD_MASK);
  if ((subscribe_count_ == 0U) && (model_load_mask == PROF_MODEL_LOAD_MASK)) {
    // register Framework to profiling
    GE_CHK_STATUS_RET_NOLOG(PluginInit());
    GELOGI("Prof subscribe: model load profiling on.");
  }
  subscribe_count_++;
  UpdateSubscribeDeviceModuleMap(kProfModelSubscribe, device, module);
  return SUCCESS;
}

Status ProfilingManager::ProfModelUnsubscribe(const uint32_t device) {
  const std::lock_guard<std::mutex> lock(mutex_);
  if (subscribe_count_ == 0U) {
    GELOGW("The profiler has not been subscribed, you do not need to cannel the subscription.");
    return SUCCESS;
  }

  const std::map<uint32_t, DeviceSubsInfo>::const_iterator iter = subs_dev_module_.find(device);
  if (iter != subs_dev_module_.cend()) {
    UpdateSubscribeDeviceModuleMap(kProfModelUnsubscribe, device, subs_dev_module_[device].module);
  } else {
    GELOGE(FAILED, "[Cancel][DeviceId]The device_id %u has not been subscribed, do not need to cancel", device);
    REPORT_CALL_ERROR("E19999", "The device_id %u has not been subscribed, do not need to cancel", device);
    return FAILED;
  }

  subscribe_count_--;
  if (subscribe_count_ == 0U) {
    // profiling plugin uninit at last subscription
    PluginUnInit();
  }

  CleanSubscribeInfo();
  // reported_graph_id_ needs to be cleared on unsubscribe
  reported_graph_id_.clear();
  return SUCCESS;
}

Status ProfilingManager::ProfInit(const uint64_t module) {
  const std::lock_guard<std::mutex> lock(mutex_);

  // register Framework to profiling
  const Status cb_ret = PluginInit();
  if (cb_ret != SUCCESS) {
    GELOGE(cb_ret, "[Init][ProfilingPlugin]Failed, ret %u", cb_ret);
    REPORT_CALL_ERROR("E19999", "Init profiling plugin failed, ret %u", cb_ret);
    return cb_ret;
  }

  const uint64_t training_trace_mask = module & static_cast<uint64_t>(PROF_TRAINING_TRACE);
  if (training_trace_mask == static_cast<uint64_t>(PROF_TRAINING_TRACE)) {
    ProfilingProperties::Instance().SetTrainingTrace(true);
  }

  GELOGI("Prof init success.");
  return SUCCESS;
}

Status ProfilingManager::ProfFinalize() {
  const std::lock_guard<std::mutex> lock(mutex_);
  index_id_ = std::numeric_limits<int64_t>::max();

  ProfilingProperties::Instance().ClearProperties();

  // profiling plugin uninit
  PluginUnInit();

  CleanSubscribeInfo();

  unreported_model_id_.clear();
  device_id_module_map_.clear();
  device_id_.clear();
  device_id_map_.clear();
  model_id_map_.clear();
  reported_graph_id_.clear();
  GELOGI("Prof finalize success.");
  return SUCCESS;
}

Status ProfilingManager::ProfParseDeviceId(const std::map<std::string, std::string> &config_para,
                                           std::vector<int32_t> &device_list) const {
  const auto iter = config_para.find(kConfigDevIdList);
  if (iter != config_para.end()) {
    const std::string device_id_list = iter->second;
    std::string temp;
    std::vector<std::string> decvice_id;
    for (size_t i = 0U; i < device_id_list.size(); i++) {
      if (isdigit(static_cast<int32_t>(device_id_list[i])) == 1) {
        (void)temp.append(1U, device_id_list[i]);
      } else {
        if (!temp.empty()) {
          decvice_id.emplace_back(temp);
        }
        temp.clear();
      }
    }
    if (!temp.empty()) {
      decvice_id.emplace_back(temp);
    }

    for (size_t i = 0U; i < decvice_id.size(); i++) {
      int32_t dev_id = -1;
      GE_CHK_STATUS_RET_NOLOG(ConvertToInt32(decvice_id[i], dev_id));
      device_list.push_back(dev_id);
    }
  } else {
    GELOGE(FAILED, "[Parse][DeviceId]Config para not contain device id list");
    REPORT_CALL_ERROR("E19999", "Parse device id failed, config para not contain device id list");
    return FAILED;
  }
  return SUCCESS;
}

Status ProfilingManager::ProfParseParam(const std::map<std::string, std::string> &config_para, int32_t &device_num,
                                        std::vector<int32_t> &device_list) const {
  // device num
  const auto iter = config_para.find(kConfigNumsdev);
  if (iter == config_para.end()) {
    GELOGE(FAILED, "[Parse][Param]Config para not contain device num");
    REPORT_CALL_ERROR("E19999", "Parse param failed, config para not contain device num");
    return FAILED;
  }
  GE_CHK_STATUS_RET_NOLOG(ConvertToInt32(iter->second, device_num));

  // device id
  if (ProfParseDeviceId(config_para, device_list) != SUCCESS) {
    GELOGE(FAILED, "[Parse][DeviceId]Failed");
    REPORT_CALL_ERROR("E19999", "Parse device id failed");
    return FAILED;
  }

  if ((device_num == 0) || (device_num > kMaxDeviceNum) || (device_num != static_cast<int32_t>(device_list.size()))) {
    GELOGE(FAILED, "[Parse][Param]Failed, config para device num %d not equal to "
           "device list size %zu", device_num, device_list.size());
    REPORT_INNER_ERROR("E19999", "[Parse][Param]Failed, config para device num %d "
                       "not equal to device list size %zu", device_num, device_list.size());
    return FAILED;
  }
  return SUCCESS;
}

Status ProfilingManager::ProfStartProfiling(const uint64_t module,
                                            const std::map<std::string, std::string> &config_para) {
  const std::lock_guard<std::mutex> lock(mutex_);
  const uint64_t model_load_mask = module & static_cast<uint64_t>(PROF_MODEL_LOAD_MASK);

  if (model_load_mask == static_cast<uint64_t>(PROF_MODEL_LOAD_MASK)) {
    ProfilingProperties::Instance().SetLoadProfiling(true);
    GELOGI("Prof init: model load profiling on.");
    for (const auto &model_id : unreported_model_id_) {
      const auto &davinci_model = ModelManager::GetInstance().GetModel(model_id);
      if (davinci_model != nullptr) {
        davinci_model->ReportProfilingData();
      }
    }
    unreported_model_id_.clear();
  }

  const uint64_t training_trace_mask = module & static_cast<uint64_t>(PROF_TRAINING_TRACE);
  if (training_trace_mask == static_cast<uint64_t>(PROF_TRAINING_TRACE)) {
    ProfilingProperties::Instance().SetTrainingTrace(true);
  }
  // profiling op detail on
  const uint64_t op_detail_mask = module & (static_cast<uint64_t>(PROF_OP_DETAIL_MASK));
  if (op_detail_mask == static_cast<uint64_t>(PROF_OP_DETAIL_MASK)) {
    ProfilingProperties::Instance().SetOpDetailProfiling(true);
    profiling::ProfilingContext::GetInstance().UpdateElementHashId(reporter_callback_);
  }
  int32_t device_num = 0;
  std::vector<int32_t> device_list;
  if (ProfParseParam(config_para, device_num, device_list) != SUCCESS) {
    GELOGE(FAILED, "[Parse][Param]Prof start parse param failed, device num %d, "
           "device list size %zu", device_num, device_list.size());
    REPORT_CALL_ERROR("E19999", "Prof start parse param failed, device num %d, "
                      "device list size %zu", device_num, device_list.size());
    return FAILED;
  }

  if ((module & static_cast<uint64_t>(PROF_MODEL_EXECUTE_MASK)) == static_cast<uint64_t>(PROF_MODEL_EXECUTE_MASK)) {
    for (size_t i = 0U; i < static_cast<size_t>(device_num); i++) {
      if (std::find(device_id_.begin(), device_id_.end(), device_list[i]) == device_id_.end()) {
        device_id_.push_back(device_list[i]);
      }
    }
    GELOGI("Prof start: ge execute model start profiling.");
  }
  if ((module & static_cast<uint64_t>(PROF_MODEL_LOAD_MASK)) == static_cast<uint64_t>(PROF_MODEL_LOAD_MASK)) {
    GELOGW("Prof start: load model module is invalid.");
  }

  UpdateDeviceIdModuleMap(kProfStart, module, device_list);
  GELOGI("Prof start profiling success.");
  return SUCCESS;
}

Status ProfilingManager::ProfStopProfiling(const uint64_t module,
                                           const std::map<std::string, std::string> &config_para) {
  const std::lock_guard<std::mutex> lock(mutex_);
  int32_t device_num = 0;
  std::vector<int32_t> device_list;
  if (ProfParseParam(config_para, device_num, device_list) != SUCCESS) {
    GELOGE(FAILED, "[Stop][Profiling]Prof stop parse param failed, device num %d, "
           "device list size %zu", device_num, device_list.size());
    REPORT_CALL_ERROR("E19999", "Prof stop parse param failed, device num %d, device list size %zu",
                      device_num, device_list.size());
    return FAILED;
  }

  const uint64_t execute_model_mask = module & static_cast<uint64_t>(PROF_MODEL_EXECUTE_MASK);
  if (execute_model_mask == static_cast<uint64_t>(PROF_MODEL_EXECUTE_MASK)) {
    for (size_t i = 0U; i < static_cast<size_t>(device_num); i++) {
      const auto iter = std::find(device_id_.begin(), device_id_.end(), device_list[i]);
      if (iter != device_id_.end()) {
        (void)device_id_.erase(iter);
      }
    }
    GELOGI("Prof stop: ge execute model stop profiling.");
  }
  if ((module & static_cast<uint64_t>(PROF_MODEL_LOAD_MASK)) == static_cast<uint64_t>(PROF_MODEL_LOAD_MASK)) {
    GELOGW("Prof stop: load model module is invalid.");
  }

  // profiling op detail off
  const uint64_t op_detail_mask = module & (static_cast<uint64_t>(PROF_OP_DETAIL_MASK));
  if (op_detail_mask == static_cast<uint64_t>(PROF_OP_DETAIL_MASK)) {
    ProfilingProperties::Instance().SetOpDetailProfiling(false);
  }

  UpdateDeviceIdModuleMap(kProfStop, module, device_list);
  GELOGI("Prof stop profiling success.");

  if (device_id_.empty()) {
    ProfilingProperties::Instance().ClearProperties();
  }

  return SUCCESS;
}

void ProfilingManager::UpdateDeviceIdModuleMap(const std::string &prof_type, const uint64_t module,
                                               const std::vector<int32_t> &device_list) {
  if (prof_type == kProfStart) {
    for (size_t i = 0U; i < device_list.size(); i++) {
      const std::map<int32_t, uint64_t>::const_iterator iter = device_id_module_map_.find(device_list[i]);
      if (iter != device_id_module_map_.cend()) {
        const uint64_t prof_on_module = device_id_module_map_[device_list[i]];
        // save all profiling on module of device
        device_id_module_map_[device_list[i]] = prof_on_module | module;
      } else {
        device_id_module_map_[device_list[i]] = module;
      }
    }
  } else if (prof_type == kProfStop) {
    for (size_t i = 0U; i < device_list.size(); i++) {
      const std::map<int32_t, uint64_t>::const_iterator iter = device_id_module_map_.find(device_list[i]);
      if (iter != device_id_module_map_.cend()) {
        const uint64_t prof_on_module = device_id_module_map_[device_list[i]];
        const uint64_t prof_off_module = prof_on_module & module;
        const uint64_t prof_on_left_module = prof_on_module & (~prof_off_module);
        // stop profiling on module of device
        device_id_module_map_[device_list[i]] = prof_on_left_module;
      }
    }
  } else {
    GELOGI("No need to update device_id module map.");
  }
}

bool ProfilingManager::ProfilingModelExecuteOn() const {
  int32_t logic_device_id = 0;
  const rtError_t rt_ret = rtGetDevice(&logic_device_id);
  if (rt_ret != RT_ERROR_NONE) {
    GELOGE(RT_FAILED, "[Get][LogicDeviceId]Failed, ret 0x%X", rt_ret);
    REPORT_CALL_ERROR("E19999", "Get logic device id failed, ret 0x%X", rt_ret);
  }
  GELOGI("Current logic_device_id:%d", logic_device_id);

  bool execute_model_prof_on = false;
  const auto iter = std::find(device_id_.begin(), device_id_.end(), logic_device_id);
  if (iter != device_id_.end()) {
    execute_model_prof_on = true;
  }

  GELOGI("Flag is_execute_profiling: %d, execute_model_prof_on: %d, op_detail_prof_on: %d.",
         static_cast<int32_t>(ProfilingProperties::Instance().IsExecuteProfiling()),
         static_cast<int32_t>(execute_model_prof_on),
         static_cast<int32_t>(ProfilingProperties::Instance().IsOpDetailProfiling()));
  return  execute_model_prof_on;
}

Status ProfilingManager::PluginInit() {
  if (reporter_callback_ == nullptr) {
    GELOGE(PARAM_INVALID, "[Check][Param]MsprofReporterCallback callback is nullptr");
    REPORT_INNER_ERROR("E19999", "MsprofReporterCallback callback is nullptr");
    return PARAM_INVALID;
  }
  int32_t cb_ret = reporter_callback_(
      static_cast<uint32_t>(MsprofReporterModuleId::MSPROF_MODULE_FRAMEWORK),
      static_cast<uint32_t>(MsprofReporterCallbackType::MSPROF_REPORTER_INIT),
      nullptr, 0U);
  if (cb_ret != MSPROF_ERROR_NONE) {
    REPORT_CALL_ERROR("E19999", "Profiling reporter init failed, ret 0x%X", cb_ret);
    GELOGE(INTERNAL_ERROR, "[Init][ProfilingReporter]Failed, ret 0x%X", cb_ret);
    return INTERNAL_ERROR;
  }

  cb_ret = reporter_callback_(
      static_cast<uint32_t>(MsprofReporterModuleId::MSPROF_MODULE_FRAMEWORK),
      static_cast<uint32_t>(MsprofReporterCallbackType::MSPROF_REPORTER_DATA_MAX_LEN),
      &reporter_max_len_, sizeof(uint32_t));
  if (cb_ret != MSPROF_ERROR_NONE) {
    REPORT_CALL_ERROR("E19999", "Get profiling reporter data max len failed, ret 0x%X", cb_ret);
    GELOGE(INTERNAL_ERROR, "[Get][ProfilingDataMaxLen]Failed, ret 0x%X", cb_ret);
    return INTERNAL_ERROR;
  }

 return SUCCESS;
}

void ProfilingManager::PluginUnInit() const {
  if (reporter_callback_ == nullptr) {
    GELOGE(ge::PARAM_INVALID, "[Check][Param]MsprofReporterCallback callback is nullptr");
    REPORT_INNER_ERROR("E19999", "MsprofReporterCallback callback is nullptr");
    return;
  }
  const int32_t cb_ret = reporter_callback_(
      static_cast<uint32_t>(MsprofReporterModuleId::MSPROF_MODULE_FRAMEWORK),
      static_cast<uint32_t>(MsprofReporterCallbackType::MSPROF_REPORTER_UNINIT),
      nullptr, 0U);
  if (cb_ret != 0) {
    GELOGW("profiling plugin uninit failed, ret:%d", cb_ret);
  }
}

Status ProfilingManager::CallMsprofReport(ReporterData &reporter_data) const {
  if (reporter_callback_ == nullptr) {
    GELOGE(PARAM_INVALID, "[Check][Param]MsprofReporterCallback callback is nullptr");
    REPORT_INNER_ERROR("E19999", "MsprofReporterCallback callback is nullptr");
    return PARAM_INVALID;
  }
  const int32_t cb_ret = reporter_callback_(static_cast<uint32_t>(MsprofReporterModuleId::MSPROF_MODULE_FRAMEWORK),
                                            static_cast<uint32_t>(MsprofReporterCallbackType::MSPROF_REPORTER_REPORT),
                                            static_cast<void *>(&reporter_data), sizeof(ReporterData));
  if (cb_ret != MSPROF_ERROR_NONE) {
    REPORT_CALL_ERROR("E19999", "Profiling reporter report failed, ret 0x%X", cb_ret);
    GELOGE(INTERNAL_ERROR, "[Call][ProfilingReporter]Failed, ret 0x%X", cb_ret);
    return INTERNAL_ERROR;
  }

  return SUCCESS;
}

void ProfilingManager::GetOpInputInfo(const OpDescPtr &op, TaskDescInfo &task_desc_info) const {
  std::vector<Format> input_format;
  std::vector<std::vector<int64_t>> input_shape;
  std::vector<DataType> input_data_type;
  for (size_t i = 0U; i < op->GetAllInputsSize(); ++i) {
    const GeTensorDescPtr input_tensor_desc = op->MutableInputDesc(static_cast<uint32_t>(i));
    if (input_tensor_desc == nullptr) {
      continue;
    }
    input_format.emplace_back(input_tensor_desc->GetFormat());
    input_shape.emplace_back(input_tensor_desc->GetShape().GetDims());
    input_data_type.emplace_back(input_tensor_desc->GetDataType());
  }

  const std::vector<Format> format_default =  { FORMAT_NULL };
  const std::vector<std::vector<int64_t>> shape_default = { {0} };
  const std::vector<DataType> data_type_default = { DT_UNDEFINED };
  task_desc_info.input_format = input_format.empty() ? format_default : input_format;
  task_desc_info.input_shape = input_shape.empty() ? shape_default : input_shape;
  task_desc_info.input_data_type = input_data_type.empty() ? data_type_default : input_data_type;
}

void ProfilingManager::GetOpOutputInfo(const OpDescPtr &op, TaskDescInfo &task_desc_info) const {
  std::vector<Format> output_format;
  std::vector<std::vector<int64_t>> output_shape;
  std::vector<DataType> output_data_type;
  for (size_t j = 0U; j < op->GetOutputsSize(); ++j) {
    const GeTensorDescPtr output_tensor_desc = op->MutableOutputDesc(static_cast<uint32_t>(j));
    if (output_tensor_desc == nullptr) {
      continue;
    }
    output_format.emplace_back(output_tensor_desc->GetFormat());
    output_shape.emplace_back(output_tensor_desc->GetShape().GetDims());
    output_data_type.emplace_back(output_tensor_desc->GetDataType());
  }

  const std::vector<Format> format_default =  { FORMAT_NULL };
  const std::vector<std::vector<int64_t>> shape_default = { {0} };
  const std::vector<DataType> data_type_default = { DT_UNDEFINED };
  task_desc_info.output_format = output_format.empty() ? format_default : output_format;
  task_desc_info.output_shape = output_shape.empty() ? shape_default : output_shape;
  task_desc_info.output_data_type = output_data_type.empty() ? data_type_default : output_data_type;
}

void ProfilingManager::GetOpInputOutputInfo(const OpDescPtr &op, TaskDescInfo &task_desc_info) const {
  GetOpInputInfo(op, task_desc_info);
  GetOpOutputInfo(op, task_desc_info);
}

Status ProfilingManager::GetDeviceIdFromGraph(const uint32_t graph_id, uint32_t &device_id) {
  const std::map<uint32_t, uint32_t>::const_iterator iter = device_id_map_.find(graph_id);
  if (iter != device_id_map_.cend()) {
    device_id = iter->second;
    return SUCCESS;
  }
  REPORT_CALL_ERROR("E19999", "graph_id:%u does not exist!", graph_id);
  GELOGE(PARAM_INVALID, "[Check][GraphId]graph_id:%u does not exist!", graph_id);
  return FAILED;
}

void ProfilingManager::SetSubscribeInfo(const uint64_t prof_switch, const uint32_t model_id, const bool is_subscribe) {
  subscribe_info_.is_subscribe = is_subscribe;
  subscribe_info_.prof_switch = prof_switch;
  subscribe_info_.graph_id = model_id;
}

void ProfilingManager::CleanSubscribeInfo() {
  subscribe_info_.is_subscribe = false;
  subscribe_info_.prof_switch = 0U;
  subscribe_info_.graph_id = 0U;
}

Status ProfilingManager::GetModelIdFromGraph(const uint32_t graph_id, uint32_t &model_id) {
  const std::map<uint32_t, uint32_t>::const_iterator iter = model_id_map_.find(graph_id);
  if (iter != model_id_map_.cend()) {
    model_id = iter->second;
    return SUCCESS;
  }
  REPORT_CALL_ERROR("E19999", "graph_id:%u does not exist!", graph_id);
  GELOGE(PARAM_INVALID, "[Check][GraphId]graph_id:%u does not exist!", graph_id);
  return FAILED;
}

bool ProfilingManager::IsGraphProfReported(const uint32_t graph_id) {
  const std::lock_guard<std::mutex> lock(mutex_);
  const auto iter = reported_graph_id_.find(graph_id);
  return (iter != reported_graph_id_.end());
}

void ProfilingManager::InsertReportedGraphId(const uint32_t graph_id) {
  const std::lock_guard<std::mutex> lock(mutex_);
  (void)reported_graph_id_.emplace(graph_id);
}

void ProfilingManager::SetGraphIdToDeviceMap(const uint32_t graph_id, const uint32_t device_id) {
  const std::lock_guard<std::mutex> lock(mutex_);
  GELOGD("Save graph id:%u and device id:%u to device_id_map.", graph_id, device_id);
  device_id_map_[graph_id] = device_id;
}

void ProfilingManager::SetGraphIdToModelMap(const uint32_t graph_id, const uint32_t model_id) {
  const std::lock_guard<std::mutex> lock(mutex_);
  GELOGD("Save graph id:%u and model id:%u to model_id_map.", graph_id, model_id);
  model_id_map_[graph_id] = model_id;
}

bool ProfilingManager::ProfilingSubscribeOn() {
  const std::lock_guard<std::mutex> lock(mutex_);
  return (subscribe_count_ > 0U);
}
}  // namespace ge
