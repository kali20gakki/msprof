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

#include "profiling_init.h"

#include "common/properties_manager.h"
#include "framework/common/debug/ge_log.h"
#include "framework/common/debug/log.h"
#include "common/profiling/profiling_properties.h"
#include "runtime/base.h"
#include "common/profiling/command_handle.h"
#include "common/profiling/profiling_manager.h"
#include "toolchain/prof_acl_api.h"
#include "mmpa/mmpa_api.h"

namespace {
const char *const kTrainingTrace = "training_trace";
const char *const kFpPoint = "fp_point";
const char *const kBpPoint = "bp_point";
}

namespace ge {
ProfilingInit &ProfilingInit::Instance() {
  static ProfilingInit profiling_init;
  return profiling_init;
}

ge::Status ProfilingInit::Init(const Options &options) {
  GELOGI("ProfilingManager::Init job_id:%s", options.job_id.c_str());

  struct MsprofGeOptions prof_conf = {};
  bool is_execute_profiling = false;
  Status ret = InitFromOptions(options, prof_conf, is_execute_profiling);
  if (ret != SUCCESS) {
    GELOGE(ret, "[Init][Profiling]Failed, error_code %u", ret);
    REPORT_CALL_ERROR("E19999", "Init profiling failed, error_code %u", ret);
    return ret;
  }
  ProfRegisterCtrlCallback();
  if (is_execute_profiling) {
     int32_t cb_ret = MsprofInit(static_cast<uint32_t>(MsprofCtrlCallbackType::MSPROF_CTRL_INIT_GE_OPTIONS),
                                 &prof_conf, sizeof(MsprofGeOptions));
     if (cb_ret != 0) {
       GELOGE(FAILED, "[Call][msprofCtrlCallback]Failed, type %u, return %d",
              static_cast<uint32_t>(MsprofCtrlCallbackType::MSPROF_CTRL_INIT_GE_OPTIONS), cb_ret);
       REPORT_CALL_ERROR("E19999", "Call msprofCtrlCallback failed, type %u, return %d",
                         static_cast<uint32_t>(MsprofCtrlCallbackType::MSPROF_CTRL_INIT_GE_OPTIONS), cb_ret);
       return FAILED;
     }
    GELOGI("Profiling init success");
  } else {
    auto df_ret = MsprofInit(0xFF, nullptr, 0);
    GELOGD("Default profiling init, return %d", df_ret);
  }
  return SUCCESS;
}

ge::Status ProfilingInit::ProfRegisterCtrlCallback() {;
  rtProfCtrlHandle callback = ProfCtrlHandle;
  rtError_t rt_ret = rtProfRegisterCtrlCallback(GE, callback);
  if (rt_ret != RT_ERROR_NONE) {
    GELOGE(FAILED, "Register CtrlCallBack failed");
    return FAILED;
  }
  return SUCCESS;
}

ge::Status ProfilingInit::InitFromOptions(const Options &options, MsprofGeOptions &prof_conf,
                                          bool &is_execute_profiling) {
  if (options.profiling_mode == "1" && !options.profiling_options.empty()) {
    // enable profiling by ge option
    if (strncpy_s(prof_conf.options, MSPROF_OPTIONS_DEF_LEN_MAX, options.profiling_options.c_str(),
                  MSPROF_OPTIONS_DEF_LEN_MAX - 1) != EOK) {
      GELOGE(INTERNAL_ERROR, "[copy][ProfilingOptions]Failed, options %s", options.profiling_options.c_str());
      REPORT_CALL_ERROR("E19999", "Copy profiling_options %s failed", options.profiling_options.c_str());
      return INTERNAL_ERROR;
    }
    GELOGI("The profiling in options is %s, %s. origin option: %s", options.profiling_mode.c_str(), prof_conf.options,
           options.profiling_options.c_str());
  } else {
    // enable profiling by env
    char env_profiling_mode[MMPA_MAX_PATH] = {0x00};
    (void)mmGetEnv("PROFILING_MODE", env_profiling_mode, MMPA_MAX_PATH);
    (void)mmGetEnv("PROFILING_OPTIONS", prof_conf.options, MSPROF_OPTIONS_DEF_LEN_MAX);
    // The env is invalid
    if (strcmp("true", env_profiling_mode) != 0) {
      return SUCCESS;
    }
     // set default value
    if (strcmp(prof_conf.options, "\0") == 0) {
      const char* default_options = "{\"output\":\"./\",\"training_trace\":\"on\",\"task_trace\":\"on\","\
        "\"hccl\":\"on\",\"aicpu\":\"on\",\"aic_metrics\":\"PipeUtilization\",\"msproftx\":\"off\"}";
      (void)strncpy_s(prof_conf.options, MSPROF_OPTIONS_DEF_LEN_MAX, default_options, strlen(default_options));
    }
    // enable profiling by env
    GELOGI("The profiling in env is %s, %s", env_profiling_mode, prof_conf.options);
  }

  is_execute_profiling = true;
  ProfilingProperties::Instance().SetExecuteProfiling(is_execute_profiling);

  // Parse json str for bp fp
  Status ret = ParseOptions(prof_conf.options);
  if (ret != ge::SUCCESS) {
    GELOGE(ge::PARAM_INVALID, "[Parse][Options]Parse training trace param %s failed, error_code %u", prof_conf.options,
           ret);
    REPORT_CALL_ERROR("E19999", "Parse training trace param %s failed, error_code %u", prof_conf.options, ret);
    return ge::PARAM_INVALID;
  }

  if (strncpy_s(prof_conf.jobId, MSPROF_OPTIONS_DEF_LEN_MAX, options.job_id.c_str(), MSPROF_OPTIONS_DEF_LEN_MAX - 1) !=
      EOK) {
    GELOGE(INTERNAL_ERROR, "[Copy][JobId]Failed, original job_id %s", options.job_id.c_str());
    REPORT_CALL_ERROR("E19999", "Copy job_id %s failed", options.job_id.c_str());
    return INTERNAL_ERROR;
  }
  GELOGI("Job id: %s, original job id: %s.", prof_conf.jobId, options.job_id.c_str());
  return ge::SUCCESS;
}

ge::Status ProfilingInit::ParseOptions(const std::string &options) {
  if (options.empty()) {
    GELOGE(ge::PARAM_INVALID, "[Check][Param]Profiling options is empty");
    REPORT_INNER_ERROR("E19999", "Profiling options is empty");
    return ge::PARAM_INVALID;
  }
  try {
    Json prof_options = Json::parse(options);
    if (options.find(kTrainingTrace) == std::string::npos) {
      return ge::SUCCESS;
    }
    std::string training_trace;
    if (prof_options.contains(kTrainingTrace)) {
      training_trace = prof_options[kTrainingTrace];
    }
    if (training_trace.empty()) {
      GELOGI("Training trace will not take effect.");
      return ge::SUCCESS;
    }
    GELOGI("GE profiling training trace:%s", training_trace.c_str());
    if (training_trace != "on") {
      GELOGE(ge::PARAM_INVALID, "[Check][Param]Training trace param:%s is invalid.", training_trace.c_str());
      REPORT_INNER_ERROR("E19999", "Training trace param:%s is invalid.", training_trace.c_str());
      return ge::PARAM_INVALID;
    }
    std::string fp_point;
    std::string bp_point;
    if (prof_options.contains(kFpPoint)) {
      fp_point = prof_options[kFpPoint];
    }
    if (prof_options.contains(kBpPoint)) {
      bp_point = prof_options[kBpPoint];
    }
    ProfilingProperties::Instance().SetTrainingTrace(true);
    ProfilingProperties::Instance().SetFpBpPoint(fp_point, bp_point);
  } catch (Json::exception &e) {
    GELOGE(FAILED, "[Check][Param]Json prof_conf options is invalid, catch exception:%s", e.what());
    REPORT_INNER_ERROR("E19999", "Json prof_conf options is invalid, catch exception:%s", e.what());
    return ge::PARAM_INVALID;
  }
  return ge::SUCCESS;
}

void ProfilingInit::StopProfiling() {
  // stop profiling
  int32_t cb_ret = MsprofFinalize();
  if (cb_ret != 0) {
     GELOGE(ge::FAILED, "call msprofCtrlCallback failed, type:%u, return:%d",
            static_cast<uint32_t>(MsprofCtrlCallbackType::MSPROF_CTRL_FINALIZE), cb_ret);
     return;
  }
  GELOGI("Stop Profiling success.");
}

void ProfilingInit::ShutDownProfiling() {
  if (ge::ProfilingProperties::Instance().ProfilingOn()) {
    ProfilingManager::Instance().PluginUnInit();
    ProfilingProperties::Instance().ClearProperties();
  }
  StopProfiling();
}

uint64_t ProfilingInit::GetProfilingModule() {
  uint64_t module = PROF_MODEL_EXECUTE_MASK |
      PROF_RUNTIME_API_MASK |
      PROF_RUNTIME_TRACE_MASK |
      PROF_SCHEDULE_TIMELINE_MASK |
      PROF_SCHEDULE_TRACE_MASK |
      PROF_TASK_TIME_MASK |
      PROF_SUBTASK_TIME_MASK |
      PROF_AICPU_TRACE_MASK |
      PROF_AICORE_METRICS_MASK |
      PROF_AIVECTORCORE_METRICS_MASK |
      PROF_MODEL_LOAD_MASK;
  return module;
}

Status ProfilingInit::SetDeviceIdByModelId(uint32_t model_id, uint32_t device_id) {
  GELOGD("Set model id:%u and device id:%u to runtime.", model_id, device_id);
  auto rt_ret = rtSetDeviceIdByGeModelIdx(model_id, device_id);
  if (rt_ret != RT_ERROR_NONE) {
    GELOGE(ge::FAILED, "[Set][Device]Set Device id failed");
    return ge::FAILED;
  }
  return ge::SUCCESS;
}

Status ProfilingInit::UnsetDeviceIdByModelId(uint32_t model_id, uint32_t device_id) {
  GELOGD("Unset model id:%u and device id:%u to runtime.", model_id, device_id);
  auto rt_ret = rtUnsetDeviceIdByGeModelIdx(model_id, device_id);
  if (rt_ret != RT_ERROR_NONE) {
    GELOGE(ge::FAILED, "[Set][Device]Set Device id failed");
    return ge::FAILED;
  }
  return ge::SUCCESS;
}
}  // namespace ge