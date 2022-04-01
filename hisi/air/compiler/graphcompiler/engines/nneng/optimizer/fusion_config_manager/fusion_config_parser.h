/**
 * Copyright 2019-2020 Huawei Technologies Co., Ltd
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

#ifndef FUSION_ENGINE_OPTIMIZER_FUSION_CONFIG_MANAGER_FUSION_CONFIG_PARSER_H_
#define FUSION_ENGINE_OPTIMIZER_FUSION_CONFIG_MANAGER_FUSION_CONFIG_PARSER_H_

#include <string>
#include <map>
#include "common/util/json_util.h"
#include "common/string_utils.h"

namespace fe {
using std::map;

const std::string SWITCH_TYPE = "Switch";
const std::string PRIORITY_TYPE = "Priority";
const std::string BUILDIN_TYPE = "build-in";
const std::string CUSTOM_TYPE = "custom";
const std::string PRIORITY_TOP = "Top";
const std::string PRIORITY_MAIN = "Main";
const std::string PRIORITY_DOWN = "Down";
const std::string SWITCH_ON = "on";
const std::string SWITCH_OFF = "off";

const int32_t CUSTOM_CFG_TOP_PRIORITY_MIN = 0;
const int32_t CUSTOM_CFG_MAIN_PRIORITY_MIN = 1000;
const int32_t CUSTOM_CFG_DOWN_PRIORITY_MIN = 2000;
const int32_t BUILT_IN_CFG_TOP_PRIORITY_MIN = 3000;
const int32_t BUILT_IN_CFG_MAIN_PRIORITY_MIN = 4000;
const int32_t BUILT_IN_CFG_DOWN_PRIORITY_MIN = 5000;
const int32_t CUSTOM_PASS_PRIORITY_MIN = 6000;
const int32_t CUSTOM_RULE_PRIORITY_MIN = 7000;
const int32_t BUILT_IN_PASS_PRIORITY_MIN = 8000;
const int32_t BUILT_IN_RULE_PRIORITY_MIN = 9000;
const int32_t RESERVED_FOR_DOWN_PRIORITY = 10000;

class FusionConfigParser {
 public:
  explicit FusionConfigParser(std::string engine_name);
  virtual ~FusionConfigParser();
  FusionConfigParser(const FusionConfigParser &) = delete;
  FusionConfigParser &operator=(const FusionConfigParser &) = delete;

  Status ParseFusionConfigFile();

  bool GetFusionSwitchByName(const std::string &key, const std::string &pass_type);

  bool GetFusionSwitchByNameLicense(const std::string &key, const std::string &pass_type, const std::string &license_fusion_cache);

  Status GetFusionPriorityByFusionType(const std::string &fusion_type,
                                       std::map<std::string, int32_t> &configured_fusion_priority_map_);

 private:
  Status ConstructFusionSwitchMap(const nlohmann::json &pass_switch_file_json,
                                  const nlohmann::json &BuildInPassSwitchFileJson, const string &type_string);

  Status ConstructFusionPriorityMap(const nlohmann::json &pass_switch_file_json,
                                    const nlohmann::json &BuildInPassSwitchFileJson, const string &type_string);

  Status ModifyFusionSwitchMap(const nlohmann::json &common_pass_switch_file_json, const string &type_string,
                               string fusion_type_temp, bool update_map_value_or_not);

  Status ModifyFusionPriorityMap(const nlohmann::json &common_pass_switch_file_json, map<string, int32_t> &map_temp,
                                 const string &type_string, string fusion_type_temp) const;

  Status CheckFusionConfigJsonFormat(nlohmann::json pass_switch_file_json, const string &owner_type) const;

  Status CheckLastLayerFormatForPriorityPart(string type_elem, string fusion_elem,
      nlohmann::json pass_switch_file_json, const string &owner_type) const;

  Status CheckConfigFileFormat(const nlohmann::json &custom_fusion_config_json,
                               const nlohmann::json &build_in_fusion_config_json,
                               const string &built_in_fusion_config_json_file,
                               const string &custom_fusion_config_json_file,
                               std::map<std::string, std::string> &error_key_map) const;

  Status CheckFusionConfigJsonLastLayerFormat(string type_elem, string fusion_elem,
                                              nlohmann::json pass_switch_file_json, const string &owner_type) const;

  Status JudgePriority(string level_elem, int32_t priority_value, const string &owner_type) const;

  Status CheckAndReadFile(const string& custom_fusion_config_json_file,
                          nlohmann::json &custom_fusion_config_json,
                          std::map<std::string, std::string> &error_key_map);

  std::map<std::string, map<std::string, std::string>> fusion_switch_map_;
  std::map<std::string, map<std::string, int32_t>> fusion_priority_map_;
  std::map<string, string> fusion_switch_detail_map_;
  std::string engine_name_;
  bool old_format_or_not_ = false;

  Status LoadOldFormatFusionSwitchFile(const string &custom_fusion_config_json_file,
                                       std::map<std::string, std::string> &error_key_map);
};
}  // namespace fe
#endif  // FUSION_ENGINE_OPTIMIZER_FUSION_CONFIG_MANAGER_FUSION_CONFIG_PARSER_H_
