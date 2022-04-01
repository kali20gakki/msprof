/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2022. All rights reserved.
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

#ifndef AIR_DEVICE_DEBUG_CONFIG_H
#define AIR_DEVICE_DEBUG_CONFIG_H

#include <cstdint>
#include <string>
#include <map>
#include <vector>
#include "ge/ge_api_types.h"
#include "nlohmann/json.hpp"
#include "common/dump/dump_manager.h"


namespace ge {
class DeviceDebugConfig {
 public:
  enum ConfigType {
    kLogConfigType = 0,
    kDumpConfigType = 1,
    kProfilingConfigType = 2,
    kConfigTypeEnd = 0xFF
  };
  DeviceDebugConfig() = default;
  virtual ~DeviceDebugConfig() = default;
  static nlohmann::json global_configs_; // log is global conf
 protected:
  mutable std::map<ConfigType, nlohmann::json> configs_{}; // dump and profiling is device conf
};

class DeviceMaintenanceMasterCfg : public DeviceDebugConfig {
 public:
  DeviceMaintenanceMasterCfg() = default;
  ~DeviceMaintenanceMasterCfg() override = default;
  void InitDevMaintenanceConfigs();
  static Status InitGlobalMaintenanceConfigs();
  Status GetJsonDataByType(ConfigType config_type, std::string &data) const;

 private:
  void InitProfilingConfig();
  static Status InitLogConfig();
  void InitDumpConfig();
};

class DeviceMaintenanceCfgManager {
 public:
  Status CreateDevMaintenanceConfig(int32_t device_id);
  void CloseDevMaintenanceConfig(int32_t device_id);
  const DeviceMaintenanceMasterCfg *GetDevMaintenanceConfig(int32_t dev_id);
  static DeviceMaintenanceCfgManager &GetInstance() {
    static DeviceMaintenanceCfgManager instance;
    return instance;
  }
 private:
  std::mutex mu_;
  std::map<int32_t, std::unique_ptr<DeviceMaintenanceMasterCfg>> device_configs_;
};

class DeviceMaintenanceClientCfg : public DeviceDebugConfig {
 public:
  DeviceMaintenanceClientCfg() : DeviceDebugConfig() {};
  ~DeviceMaintenanceClientCfg() override = default;
  Status DecodeConfig(std::map<std::string, std::string> &env_option,
                      std::map<std::string, std::string> &args_option);
  Status LoadJsonData(const std::string &data);
  static Status DecodeLogConfig(std::map<std::string, std::string> &env_option,
                                std::map<std::string, std::string> &args_option);
 private:
  Status DecodeDumpConfig(std::map<std::string, std::string> &env_option,
                          std::map<std::string, std::string> &args_option);
  Status DecodeProfilingConfig(std::map<std::string, std::string> &env_option,
                               std::map<std::string, std::string> &args_option);
};

class DevMaintenanceCfgUtils {
 public:
  DevMaintenanceCfgUtils() = default;
  ~DevMaintenanceCfgUtils() = default;
  static std::vector<std::string> TransArgsOption2Array(std::map<std::string, std::string> &args_option);
  static void SetEnvByOption(const std::map<std::string, std::string> &env_option);
  static Status DecodeJsonConfig(const nlohmann::json &param_list_jsons,
                                 std::map<std::string, std::string> &env_option,
                                 std::map<std::string, std::string> &args_option);
};
} // namespace ge

#endif  // AIR_DEVICE_DEBUG_CONFIG_H
