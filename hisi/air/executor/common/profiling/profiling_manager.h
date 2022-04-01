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

#ifndef GE_COMMON_PROFILING_PROFILING_MANAGER_H_
#define GE_COMMON_PROFILING_PROFILING_MANAGER_H_

#include <mutex>
#include <map>
#include <string>
#include <vector>

#include "securec.h"
#include "nlohmann/json.hpp"
#include "graph/op_desc.h"
#include "framework/common/ge_inner_error_codes.h"
#include "framework/common/ge_types.h"
#include "external/register/register_types.h"
#include "runtime/stream.h"
#include "toolchain/prof_callback.h"
#include "toolchain/prof_common.h"
#include "common/profiling/profiling_properties.h"
#include "framework/common/debug/ge_log.h"

#define RECORD_PROFILING_START(event) \
  mmTimespec event##ts; \
  int64_t event##start_time = 0; \
  if (ProfilingProperties::Instance().IsDynamicShapeProfiling() && \
      ProfilingManager::Instance().ProfilingModelLoadOn()) { \
    event##ts = mmGetTickCount(); \
    event##start_time = (event##ts.tv_sec * 1000 * 1000 * 1000) + event##ts.tv_nsec; \
  }

#define RECORD_PROFILING_END(event, event_name) \
  do {                                              \
    if (ProfilingProperties::Instance().IsDynamicShapeProfiling() && \
        ProfilingManager::Instance().ProfilingModelLoadOn()) { \
      event_name##ts = mmGetTickCount(); \
      const int64_t event_name##end_time = (event_name##ts.tv_sec * 1000 * 1000 * 1000) + event_name##ts.tv_nsec; \
      ProfilingManager::Instance().Record(0U,                                                      \
                                          profiling::ProfilingContext::GetInstance().GetProfiler()->  \
                                          GetStringHashes()[(event)].hash,                 \
                                          event_name##start_time, event_name##end_time);   \
    }  \
  } while (false)

#define RECORD_SHELL_PROFILING_START(event)  RECORD_PROFILING_START(event)

#define RECORD_SHELL_PROFILING_END(event, element, event_name) \
  do {                                                         \
    if (ProfilingProperties::Instance().IsDynamicShapeProfiling() && \
        ProfilingManager::Instance().ProfilingModelLoadOn()) { \
      event_name##ts = mmGetTickCount(); \
      if (profiling::ProfilingContext::GetInstance().GetProfiler() != nullptr) {                        \
        const int64_t event_name##end_time = (event_name##ts.tv_sec * 1000 * 1000 * 1000) + event_name##ts.tv_nsec; \
        ProfilingManager::Instance().Record(profiling::ProfilingContext::GetInstance().GetProfiler()->  \
                                            GetStringHashes()[(element)].hash,                          \
                                            profiling::ProfilingContext::GetInstance().GetProfiler()->  \
                                            GetStringHashes()[(event)].hash,               \
                                            event_name##start_time, event_name##end_time); \
      } \
    }  \
  } while (false)

#define REPORT_TASK_PROFILING_DETAIL(task_ctx, graph_ctx) \
  do {                                 \
  if ((ProfilingManager::Instance().ProfilingModelLoadOn() || ProfilingManager::Instance().ProfilingSubscribeOn()) && \
      (!(task_ctx).NeedCallback())) { \
      (void)NodeDoneCallback::ReportTaskDetailInfo((task_ctx), (graph_ctx));   \
    }                                  \
  } while (false);

namespace ge {
struct DeviceSubsInfo {
  uint64_t module;
  uint32_t subscribe_count;
};

struct ProfSubscribeInfo {
  bool is_subscribe;
  uint64_t prof_switch;
  uint32_t graph_id;
};

class ProfilingManager {
 public:
  ProfilingManager();
  virtual ~ProfilingManager();
  static ProfilingManager &Instance();
  static void RegisterElement(int64_t &idx, const std::string &element);

  // Ctrl from ModelManager.
  Status ProfInit(const uint64_t module);
  Status ProfFinalize();
  static void Record(const uint64_t element, const uint64_t event, const int64_t start, const int64_t end);
  Status ProfStartProfiling(const uint64_t module, const std::map<std::string, std::string> &config_para);
  Status ProfStopProfiling(const uint64_t module, const std::map<std::string, std::string> &config_para);
  Status CheckInitForSubscribe(const uint64_t module, const uint32_t device);
  Status ProfModelUnsubscribe(const uint32_t device);

  // report model load profiling data flag, data contain task desc info, step info, model load fusion op info
  bool ProfilingModelLoadOn() const { return ProfilingProperties::Instance().IsLoadProfiling(); }
  // report model execute profiling data flag, data contain model execute time info
  bool ProfilingModelExecuteOn() const;
  // profiling subscribe is set
  bool ProfilingSubscribeOn();
  // is_execute_profiling_ only used by ge option and env
  void ReportProfilingData(const uint32_t model_id, const std::vector<TaskDescInfo> &task_desc_info);

  Status PluginInit();
  void PluginUnInit() const;

  void SetMsprofReporterCallback(const MsprofReporterCallback func) { reporter_callback_ = func; }
  void GetOpInputOutputInfo(const OpDescPtr &op, TaskDescInfo &task_desc_info) const;
  void ReportData(const int32_t device_id, void *const data, const size_t data_size, const std::string &tag_name);
  Status ProfileStepInfo(const uint64_t index_id, const uint32_t model_id, const uint16_t tag_id,
                         const rtStream_t stream, const uint32_t device_id);
  void SetStepInfoIndex(const int64_t index_id) { index_id_ = index_id; }
  int64_t GetStepInfoIndex() const { return index_id_; }
  void RecordUnReportedModelId(const uint32_t model_id_) {
    const std::lock_guard<std::mutex> lock(mutex_);
    unreported_model_id_.push_back(model_id_);
  }

  void SetGraphIdToDeviceMap(const uint32_t graph_id, const uint32_t device_id);
  Status GetDeviceIdFromGraph(const uint32_t graph_id, uint32_t &device_id);
  void SetSubscribeInfo(const uint64_t prof_switch, const uint32_t model_id, const bool is_subscribe);
  const ProfSubscribeInfo &GetSubscribeInfo() const { return subscribe_info_; }
  void CleanSubscribeInfo();
  void SetGraphIdToModelMap(const uint32_t graph_id, const uint32_t model_id);
  Status GetModelIdFromGraph(const uint32_t graph_id, uint32_t &model_id);
  Status QueryHashId(const std::string &src_str, uint64_t &hash_id) const;
  bool IsGraphProfReported(const uint32_t graph_id);
  void InsertReportedGraphId(const uint32_t graph_id);


  template <typename T>
  void SetAlternativeValue(const int32_t property_len, const std::string &value, T &property) const {
    if (value.size() < static_cast<size_t>(property_len)) {
      property.type = static_cast<uint8_t>(MSPROF_MIX_DATA_STRING);
      const auto ret = strncpy_s(property.data.dataStr, static_cast<size_t>(property_len), value.c_str(), value.size());
      if (ret != 0) {
        GELOGE(FAILED, "[Profiling] Strcpy value [%s] error!", value.c_str());
        return;
      }
    } else {
      property.type = static_cast<uint8_t>(MSPROF_MIX_DATA_HASH_ID);
      uint64_t hash_id = 0UL;
      const auto ret = QueryHashId(value, hash_id);
      if (ret != SUCCESS) {
        GELOGE(FAILED, "[Profiling] Query hash id of [%s] error!", value.c_str());
        return;
      }
      property.data.hashId = hash_id;
    }
  }
 private:
  Status ProfParseParam(const std::map<std::string, std::string> &config_para, int32_t &device_num,
                        std::vector<int32_t> &device_list) const;
  Status ProfParseDeviceId(const std::map<std::string, std::string> &config_para,
                           std::vector<int32_t> &device_list) const;
  void UpdateDeviceIdModuleMap(const std::string &prof_type, const uint64_t module,
                               const std::vector<int32_t> &device_list);
  void UpdateSubscribeDeviceModuleMap(const std::string &prof_type, const uint32_t device_id, const uint64_t module);
  void GetOpInputInfo(const OpDescPtr &op, TaskDescInfo &task_desc_info) const;
  void GetOpOutputInfo(const OpDescPtr &op, TaskDescInfo &task_desc_info) const;
  void ProfilingTaskDescInfo(const uint32_t model_id, const int32_t device_id,
                             const std::vector<TaskDescInfo> &task_desc_info);
  void ProfilingOpInputOutInfo(const TaskDescInfo &task, const uint32_t model_id, const int32_t device_id,
                               const uint64_t timestamp, const std::string &tag_name);

  Status CallMsprofReport(ReporterData &reporter_data) const;

  std::vector<int32_t> device_id_;
  std::map<int32_t, uint64_t> device_id_module_map_; // key: device_id, value: profiling on module
  std::map<uint32_t, DeviceSubsInfo> subs_dev_module_; // key: device_id, value: profiling on module
  uint32_t subscribe_count_{0U};
  std::mutex mutex_;
  std::mutex mutex_report_;
  std::mutex mutex_hash_;
  uint32_t reporter_max_len_{1U};
  int64_t index_id_{std::numeric_limits<int64_t>::max()};
  std::map<uint32_t, uint32_t> device_id_map_; // key: graph_id, value: device_id
  std::map<uint32_t, uint32_t> model_id_map_; // key: graph_id, value: model_id
  ProfSubscribeInfo subscribe_info_{false, 0U, 0U};
  MsprofReporterCallback reporter_callback_{nullptr};
  std::unordered_set<uint32_t> reported_graph_id_;
  std::vector<uint32_t> unreported_model_id_;
};
}  // namespace ge
#endif  // GE_COMMON_PROFILING_PROFILING_MANAGER_H_
