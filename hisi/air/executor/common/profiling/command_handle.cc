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

#include "command_handle.h"

#include "common/profiling/profiling_manager.h"
#include "framework/common/debug/ge_log.h"
#include "framework/common/debug/log.h"
#include "framework/omg/omg_inner_types.h"
#include "exec_runtime/execution_runtime.h"
#include "graph/load/graph_loader.h"

namespace ge {
namespace {
const size_t kDeviceListIndex = 3U;
const uint32_t kCommandNum = 6U;
const uint32_t kMaxDevNum = 64U;
const std::string kDeviceNums = "devNums";
const std::string kDeviceIdList = "devIdList";
const std::string kProfilingInit = "prof_init";
const std::string kProfilingFinalize = "prof_finalize";
const std::string kProfilingStart = "prof_start";
const std::string kProfilingStop = "prof_stop";
const std::string kProfilingModelSubscribe = "prof_model_subscribe";
const std::string kProfilingModelUnsubscribe = "prof_model_cancel_subscribe";
const std::string kProfilingModelId = "modelId";
const int32_t RT_ERROR = -1;

enum class ProfCommandHandleType {
  kProfCommandHandleInit = 0,
  kProfCommandHandleStart,
  kProfCommandHandleStop,
  kProfCommandHandleFinalize,
  kProfCommandHandleModelSubscribe,
  kProfCommandHandleModelUnsubscribe
};

const std::map<ProfCommandHandleType, std::string> kProfCommandTypeMap {
    {ProfCommandHandleType::kProfCommandHandleInit, kProfilingInit},
    {ProfCommandHandleType::kProfCommandHandleStart, kProfilingStart},
    {ProfCommandHandleType::kProfCommandHandleStop, kProfilingStop},
    {ProfCommandHandleType::kProfCommandHandleFinalize, kProfilingFinalize},
    {ProfCommandHandleType::kProfCommandHandleModelSubscribe, kProfilingModelSubscribe},
    {ProfCommandHandleType::kProfCommandHandleModelUnsubscribe, kProfilingModelUnsubscribe}
};

bool IsProfConfigValid(const uint32_t deviceid_list[], const uint32_t device_nums) {
  if ((device_nums == 0U) || (device_nums > kMaxDevNum)) {
    GELOGE(PARAM_INVALID, "[Check][DeviceNums]Invalid, device nums: %u", device_nums);
    REPORT_INNER_ERROR("E19999", "DeviceNums %u check invalid", device_nums);
    return false;
  }

  // real device num
  int32_t dev_count = 0;
  const rtError_t rt_err = rtGetDeviceCount(&dev_count);
  if (rt_err != RT_ERROR_NONE) {
    GELOGE(INTERNAL_ERROR, "[Get][DeviceCount]Failed, error_code %d", rt_err);
    REPORT_CALL_ERROR("E19999", "Get device count failed, error_code %d", rt_err);
    return false;
  }

  if (device_nums > static_cast<uint32_t>(dev_count)) {
    GELOGE(PARAM_INVALID, "[Check][Param]Device num %u is not in range [1,%d]", device_nums, dev_count);
    REPORT_INNER_ERROR("E19999", "Device num %u check invalid, it is not in range [1,%d]", device_nums, dev_count);
    return false;
  }

  std::set<uint32_t> record;
  for (uint32_t i = 0U; i < device_nums; ++i) {
    const uint32_t dev_id = deviceid_list[i];
    if (dev_id >= static_cast<uint32_t>(dev_count)) {
      GELOGE(PARAM_INVALID, "[Check][DeviceId]Device id %u is not in range [0,%d)", dev_id, dev_count);
      REPORT_CALL_ERROR("E19999", "Device id %u is not in range [0,%d)", dev_id, dev_count);
      return false;
    }
    if (!record.insert(dev_id).second) {
      GELOGE(PARAM_INVALID, "[Check][DeviceId]Device id %u is duplicatedly set", dev_id);
      REPORT_CALL_ERROR("E19999", "Device id %u is not unique, duplicatedly set", dev_id);
      return false;
    }
  }
  return true;
}

bool TransProfConfigToParam(const MsprofCommandHandle &prof_command_handle,
                            std::vector<std::string> &prof_config_params) {
  prof_config_params.clear();
  prof_config_params.emplace_back(kDeviceNums);
  prof_config_params.emplace_back(std::to_string(prof_command_handle.devNums));
  prof_config_params.emplace_back(kDeviceIdList);
  std::string dev_id;
  if (prof_command_handle.devNums == 0U) {
    GELOGE(FAILED, "[Check][Param]The device num is invalid.");
    return false;
  }
  for (uint32_t i = 0U; i < prof_command_handle.devNums; i++) {
    (void)dev_id.append(std::to_string(prof_command_handle.devIdList[i]));
    if (i != (prof_command_handle.devNums - 1U)) {
      (void)dev_id.append(",");
    }
  }

  prof_config_params.push_back(dev_id);
  return true;
}

Status NeedUnsubscribe(const ProfCommandHandleType type, const uint32_t graph_id,
                       std::vector<std::string> &prof_params) {
  if (type == ProfCommandHandleType::kProfCommandHandleModelUnsubscribe) {
    prof_params.clear();
    prof_params.emplace_back(kProfilingModelId);
    auto &prof_mgr = ProfilingManager::Instance();
    if (prof_mgr.GetSubscribeInfo().is_subscribe) {
      uint32_t model_id = 0U;
      const auto ret = prof_mgr.GetModelIdFromGraph(graph_id, model_id);
      if (ret != SUCCESS) {
        GELOGE(ret, "[Get][GraphId]graph_id:%u not not found", graph_id);
        return ret;
      }
      prof_params.emplace_back(std::to_string(model_id));
    } else {
      prof_params.emplace_back(std::to_string(graph_id));
    }
  }

  return SUCCESS;
}

Status NeedHandleStartEnd(const ProfCommandHandleType type, const MsprofCommandHandle &prof_command_handle,
                          std::vector<std::string> &prof_params) {
  if ((type == ProfCommandHandleType::kProfCommandHandleStart) ||
      (type == ProfCommandHandleType::kProfCommandHandleStop)) {
    if (!IsProfConfigValid(prof_command_handle.devIdList, prof_command_handle.devNums)) {
      return FAILED;
    }
    if (!TransProfConfigToParam(prof_command_handle, prof_params)) {
      GELOGE(PARAM_INVALID, "[Check][Param]Transfer profilerConfig to std::string vector failed");
      REPORT_CALL_ERROR("E19999", "Transfer profilerConfig to std::string vector failed");
      return PARAM_INVALID;
    }
  }
  return SUCCESS;
}

void SubscribeInfoToParam(const ProfCommandHandleType type, const MsprofCommandHandle &prof_command_handle,
                          std::vector<std::string> &prof_params) {
  if (type == ProfCommandHandleType::kProfCommandHandleModelSubscribe) {
    prof_params.clear();
    prof_params.push_back(kProfilingModelId);
    prof_params.push_back(std::to_string(prof_command_handle.modelId));
  }
}

rtError_t ExecuteCommand(const ProfCommandHandleType type,
                         const MsprofCommandHandle &prof_command_handle,
                         const std::vector<std::string> &prof_params) {
  const auto it = kProfCommandTypeMap.find(type);
  if (it == kProfCommandTypeMap.end()) {
    GELOGE(PARAM_INVALID, "[Check][Param]The prof comand type is invalid.");
    return RT_ERROR;
  }

  Command command;
  command.cmd_type = it->second;
  command.cmd_params = prof_params;
  if (type != ProfCommandHandleType::kProfCommandHandleFinalize) {
    command.module_index = prof_command_handle.profSwitch;
  }
  GELOGI("Command Type: %s, data type config: 0x%lx", it->second.c_str(), command.module_index);
  if ((type == ProfCommandHandleType::kProfCommandHandleStart) ||
      (type == ProfCommandHandleType::kProfCommandHandleStop)) {
    if (prof_params.size() > kDeviceListIndex) {
      GELOGI("Profiling device nums:%s, deviceId:%s", prof_params[0U].c_str(), prof_params[kDeviceListIndex].c_str());
    } else {
      GELOGW("Profiling input param[size=%zu] may invalid", prof_params.size());
    }
  }

  const Status ret = GraphLoader::CommandHandle(command);
  if (ret != SUCCESS) {
    GELOGE(ret, "[Handle][Command]Handle profiling command failed, command type %s, error_code %u",
           it->second.c_str(), ret);
    REPORT_CALL_ERROR("E19999", "Handle profiling command failed, command type %s, error_code %u",
                      it->second.c_str(), ret);
    return RT_ERROR;
  }

  GELOGI("Successfully execute profiling command type: %d, command 0x%lx.",
         static_cast<int32_t>(type), command.module_index);
  return RT_ERROR_NONE;
}

void HandleHeterogeneousCtrlSwitch(const MsprofCommandHandle &prof_command_handle) {
  const auto type = static_cast<ProfCommandHandleType>(prof_command_handle.type);
  if ((type == ProfCommandHandleType::kProfCommandHandleStart) ||
      (type == ProfCommandHandleType::kProfCommandHandleStop)) {
    ProfilingProperties::Instance().UpdateDeviceIdCommandParams(std::string(prof_command_handle.params.profData));
  }
}

rtError_t HandleCtrlSwitch(const MsprofCommandHandle &prof_command_handle) {
  if (prof_command_handle.type >= kCommandNum) {
    GELOGE(PARAM_INVALID, "[Check][Type]Type %u is invalid", prof_command_handle.type);
    return RT_ERROR;
  }

  GELOGD("Type is %u", prof_command_handle.type);
  const auto type = static_cast<ProfCommandHandleType>(prof_command_handle.type);
  int32_t heterogeneous_flag = 0;
  if (ge::ExecutionRuntime::IsHeterogeneousEnabled() &&
      (rtGetIsHeterogenous(&heterogeneous_flag) == RT_ERROR_NONE) &&
      (heterogeneous_flag == ge::ExecutionRuntime::kRuntimeTypeHeterogeneous)) {
    HandleHeterogeneousCtrlSwitch(prof_command_handle);
    return SUCCESS;
  }

  std::vector<std::string> prof_params;
  Status ret = NeedHandleStartEnd(type, prof_command_handle, prof_params);
  if (ret != SUCCESS) {
    GELOGE(ret, "[Handle][Command]Handle command failed, the command type is %d.", static_cast<int32_t>(type));
    return RT_ERROR;
  }
  if ((type == ProfCommandHandleType::kProfCommandHandleModelSubscribe) &&
      ProfilingProperties::Instance().IsTrainingModeProfiling()) {
    GELOGD("Subscribe in training.");
    auto &prof_mgr = ProfilingManager::Instance();
    prof_mgr.SetSubscribeInfo(prof_command_handle.profSwitch, prof_command_handle.modelId, true);
    return RT_ERROR_NONE;
  }
  SubscribeInfoToParam(type, prof_command_handle, prof_params);

  // GraphId is actually stored in prof_command_handle
  const auto graph_id = prof_command_handle.modelId;
  ret = NeedUnsubscribe(type, graph_id, prof_params);
  if (ret != SUCCESS) {
    GELOGE(ret, "[Check][Param]graph_id:%u not not found", graph_id);
    REPORT_INPUT_ERROR(
        "E10001", std::vector<std::string>({"value", "parameter", "reason"}),
        std::vector<std::string>({std::to_string(graph_id), "GraphToModelMap", "graph_id does not exist!"}));
    return RT_ERROR;
  }
  return ExecuteCommand(type, prof_command_handle, prof_params);
}
} // namespace

rtError_t ProfCtrlHandle(const uint32_t ctrl_type, void *ctrl_data, const uint32_t data_len) {
  if ((ctrl_data == nullptr) || (data_len == 0U)) {
    GELOGE(PARAM_INVALID, "[Check][Param]The prof comand is invalid.");
    return RT_ERROR;
  }

  if (ctrl_type == RT_PROF_CTRL_REPORTER) {
    auto &prof_mgr = ProfilingManager::Instance();
    prof_mgr.SetMsprofReporterCallback(reinterpret_cast<MsprofReporterCallback>(ctrl_data));
    GELOGD("return with MsprofReporterCallback");
    return RT_ERROR_NONE;
  }

  if (ctrl_type == RT_PROF_CTRL_SWITCH) {
    const MsprofCommandHandle *const prof_command_handle = PtrToPtr<void, MsprofCommandHandle>(ctrl_data);
    return HandleCtrlSwitch(*prof_command_handle);
  }
  return RT_ERROR;
}
}  // namespace ge