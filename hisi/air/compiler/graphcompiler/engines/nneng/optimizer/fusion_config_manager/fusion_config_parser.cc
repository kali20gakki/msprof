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

#include "fusion_config_parser.h"
#include <sstream>
#include <unordered_set>
#include "common/configuration.h"
#include "common/fe_error_code.h"
#include "common/string_utils.h"
#include "common/fusion_pass_util.h"
#include "common/util/error_manager/error_manager.h"
#include "common/util/json_util.h"
#include "fusion_rule_manager/fusion_rule_parser/fusion_rule_parser_utils.h"

namespace fe {

FusionConfigParser::FusionConfigParser(std::string engine_name) : engine_name_(std::move(engine_name)) {}

FusionConfigParser::~FusionConfigParser() = default;

bool FusionConfigParser::GetFusionSwitchByNameLicense(const std::string &key, const std::string &pass_type,
                                                      const std::string &license_fusion_cache) {
  std::set<std::string> license_detail_info;
  Configuration::Instance(engine_name_).GetLicenseFusionDetailInfo(license_detail_info);
  std::map<std::string, std::map<std::string, std::string>>::const_iterator iter = fusion_switch_map_.find(pass_type);
  if (iter == fusion_switch_map_.end()) {
    return true;
  }
  std::map<std::string, std::string>::const_iterator iter_inner = iter->second.find(key);
  bool user_switch_on = false;
  bool user_switch_null = false;
  FE_LOGD("Pass:[%s] GetFusionSwitchByNameLicense %s", key.c_str(), license_fusion_cache.c_str());
  if (iter_inner != iter->second.end()) {
    if (iter_inner->second == FUSION_SWITCH_OFF) {
      FE_LOGD("The value of the key : %s is off.", key.c_str());
      return false;
    } else {
      user_switch_on = true;
    }
  } else {
    user_switch_null = true;
  }
  if (user_switch_on || user_switch_null) {
    // close all fusion pass
    auto all_inner = iter->second.find("ALL");
    const bool switch_off_flag = (all_inner != iter->second.cend()) && (all_inner->second == FUSION_SWITCH_OFF);
    if (switch_off_flag) {
      if (user_switch_on) {
        FE_LOGD("ALL OFF pass:[%s] will off, because user switch is on", key.c_str());
        return true;
      }
      FE_LOGD("Although license is allowed, pass:[%s] will off, because user ALL is off", key.c_str());
      return false;
    }
    if (license_fusion_cache == "ALL") {
      FE_LOGD("The value of the key : %s is on, license_fusion_cache is all, user_switch_null: %d",
              key.c_str(), user_switch_null);
      return true;
    } else {
      if (license_detail_info.find(key) != license_detail_info.end()) {
        FE_LOGD("The value of the key : %s is on, license_detail_info exist.", key.c_str());
        return true;
      } else {
        bool is_control_map = Configuration::Instance(engine_name_).IsInLicenseControlMap(key);
        FE_LOGD("The value of the key : %s is_control_map:%d, license_detail_info is no exist.",
                key.c_str(), is_control_map);
        return !is_control_map;
      }
    }
    FE_LOGD("The value of the key : %s is on.", key.c_str());
    return true;
  }
  FE_LOGD("Pass:[%s] is on by default.", key.c_str());
  return true;
}

bool FusionConfigParser::GetFusionSwitchByName(const std::string &key, const std::string &pass_type) {
  std::map<std::string, std::map<std::string, std::string>>::const_iterator iter = fusion_switch_map_.find(pass_type);
  if (iter == fusion_switch_map_.end()) {
    FE_LOGD("Can't find this pass_type [%s] in ge.fusionSwitchFile", pass_type.c_str());
    return true;
  }

  if (ForbiddenClosedPass.find(key) != ForbiddenClosedPass.end()) {
    FE_LOGD("Pass:[%s] is forbidden closed, will be on by default.", key.c_str());
    return true;
  }
  std::string license_fusion_cache = Configuration::Instance(engine_name_).GetLicenseFusionStr();
  FE_LOGD("Pass:[%s] license_fusion_cache %s", key.c_str(), license_fusion_cache.c_str());
  return GetFusionSwitchByNameLicense(key, pass_type, license_fusion_cache);
}

Status FusionConfigParser::GetFusionPriorityByFusionType(
    const std::string &fusion_type, std::map<std::string, int32_t> &configured_fusion_priority_map_) {
  if (fusion_priority_map_.find(fusion_type) != fusion_priority_map_.end()) {
    configured_fusion_priority_map_ = fusion_priority_map_[fusion_type];
  } else {
    FE_LOGD("The fusion_priority_map is not include GraphFusion type %s.", fusion_type.c_str());
  }
  return SUCCESS;
}

Status GetKeyAndValueFromJson(const std::string &line, std::ifstream &ifs, std::string &key, std::string &value,
                              std::map<std::string, std::string> &error_key_map) {
  size_t pos_of_equal = line.find(":");
  if (pos_of_equal == std::string::npos) {
    REPORT_FE_ERROR("[GraphOpt][FusionConfig][GetKeyValFmJs] The content of [%s] delimiter must be colon",
                    line.c_str());
    error_key_map[EM_ERROR_MSG] = "this line[" + line + "] does not content separator[:]";
    LogErrorMessage(EM_READ_FILE_FAILED, error_key_map);
    return FAILED;
  }
  key = line.substr(0, pos_of_equal);
  value = line.substr(pos_of_equal + 1);
  key = StringUtils::Trim(key);
  value = StringUtils::Trim(value);
  if (value != FUSION_SWITCH_ON && value != FUSION_SWITCH_OFF) {
    REPORT_FE_ERROR("[GraphOpt][FusionConfig][GetKeyValFmJs] The pass switch value: [%s] is not on or off!",
                    value.c_str());
    std::ostringstream oss;
    oss << "the switch value of pass[" << key;
    oss << "] must be on or off, instead of [" << value << "]";
    error_key_map[EM_ERROR_MSG] = oss.str();
    LogErrorMessage(EM_READ_FILE_FAILED, error_key_map);
    return FAILED;
  }
  return SUCCESS;
}

Status FusionConfigParser::LoadOldFormatFusionSwitchFile(const string &custom_fusion_config_json_file,
                                                         std::map<std::string, std::string> &error_key_map) {
  std::ifstream ifs(custom_fusion_config_json_file);
  if (!ifs.is_open()) {
    REPORT_FE_ERROR("[GraphOpt][FusionConfig][LdOldFmtFusSwtFile] The file [%s] does not exist or has been opened.",
                    custom_fusion_config_json_file.c_str());
    error_key_map[EM_ERROR_MSG] = "The file is not existed or has been opened.";
    LogErrorMessage(EM_OPEN_FILE_FAILED, error_key_map);
    return FILE_NOT_EXIST;
  }

  std::string line;
  while (std::getline(ifs, line)) {
    if (line.empty()) {
      continue;
    }

    std::string key;
    std::string value;

    Status ret = GetKeyAndValueFromJson(line, ifs, key, value, error_key_map);
    if (ret != SUCCESS) {
      ifs.close();
      return ret;
    }

    if (!key.empty()) {
      std::map<std::string, std::string>::const_iterator iter_switch = fusion_switch_detail_map_.find(key);
      if (iter_switch != fusion_switch_detail_map_.end()) {
        REPORT_FE_ERROR("[GraphOpt][FusionConfig][LdOldFmtFusSwtFile] Pass[%s] is repetitive, please check it.",
                        key.c_str());
        error_key_map[EM_ERROR_MSG] = "pass[" + key + "] is repetitive";
        LogErrorMessage(EM_READ_FILE_FAILED, error_key_map);
        ifs.close();
        return FAILED;
      }
      fusion_switch_detail_map_.emplace(key, value);
    }
  }
  ifs.close();
  return SUCCESS;
}

Status FusionConfigParser::CheckAndReadFile(const string &custom_fusion_config_json_file,
                                            nlohmann::json &custom_fusion_config_json,
                                            std::map<std::string, std::string> &error_key_map) {
  // Check custom file(path) is legal or not.
  if (!custom_fusion_config_json_file.empty()) {
    if (!CheckFilePath(custom_fusion_config_json_file, "/")) {
      REPORT_FE_ERROR("[GraphOpt][Init][CheckAndReadFile] The file path: %s is not legal",
                      custom_fusion_config_json_file.c_str());
      error_key_map[EM_ERROR_MSG] = "File path is not valid.";
      LogErrorMessage(EM_OPEN_FILE_FAILED, error_key_map);
      return FAILED;
    }
    if (CheckFileEmpty(custom_fusion_config_json_file)) {
      REPORT_FE_ERROR("[GraphOpt][Init][CheckAndReadFile] The file [%s] is empty.",
                      custom_fusion_config_json_file.c_str());
      error_key_map[EM_ERROR_MSG] = "The file is empty.";
      LogErrorMessage(EM_OPEN_FILE_FAILED, error_key_map);
      return FAILED;
    }
    if (ReadJsonFile(custom_fusion_config_json_file, custom_fusion_config_json) != SUCCESS) {
      FE_LOGD("Read Json content from file:%s failed and start to try read this file by old format.",
              custom_fusion_config_json_file.c_str());
      if (LoadOldFormatFusionSwitchFile(custom_fusion_config_json_file, error_key_map) != SUCCESS) {
        REPORT_FE_ERROR("[GraphOpt][Init][CheckAndReadFile] Failed to read this file[%s] by old format.",
                        custom_fusion_config_json_file.c_str());
        return FAILED;
      }
      old_format_or_not_ = true;
      FE_LOGD("Read this file[%s] by old format successfully.", custom_fusion_config_json_file.c_str());
    }
  }
  return SUCCESS;
}

Status FusionConfigParser::CheckConfigFileFormat(const nlohmann::json &custom_fusion_config_json,
                                                 const nlohmann::json &build_in_fusion_config_json,
                                                 const string &built_in_fusion_config_json_file,
                                                 const string &custom_fusion_config_json_file,
                                                 std::map<std::string, std::string> &error_key_map) const {
  // Judge built-in and custom file(if have) 's top form is or not json object.
  if (custom_fusion_config_json != nullptr && !custom_fusion_config_json.is_object()) {
    REPORT_FE_ERROR(
        "[GraphOpt][Init][CheckCfgFileFormat] The top level of custom fusion config json file should be \
        object, actually is [%s].",
        GetJsonType(custom_fusion_config_json).c_str());
    error_key_map[EM_ERROR_MSG] = "The top level of custom fusion config json file should be object, actually is " +
                                  GetJsonType(custom_fusion_config_json);
    LogErrorMessage(EM_OPEN_FILE_FAILED, error_key_map);
    return ILLEGAL_JSON;
  }
  if (!build_in_fusion_config_json.is_object()) {
    REPORT_FE_ERROR(
        "[GraphOpt][Init][CheckCfgFileFormat] The top level of built-in fusion config json file should be \
        object, actually is [%s].",
        GetJsonType(build_in_fusion_config_json).c_str());
    return ILLEGAL_JSON;
  }

  // Do some judge about form, key and value.
  if (CheckFusionConfigJsonFormat(custom_fusion_config_json, CUSTOM_TYPE) != SUCCESS) {
    REPORT_FE_ERROR(
        "[GraphOpt][Init][CheckCfgFileFormat] Failed to check custom fusion config json file format, the \
        file path: %s", custom_fusion_config_json_file.c_str());
    error_key_map[EM_ERROR_MSG] =
        "Failed to check custom fusion config json file format, the file path: " + custom_fusion_config_json_file;
    LogErrorMessage(EM_READ_FILE_FAILED, error_key_map);
    return FAILED;
  }
  if (CheckFusionConfigJsonFormat(build_in_fusion_config_json, BUILDIN_TYPE) != SUCCESS) {
    REPORT_FE_ERROR(
        "[GraphOpt][Init][CheckCfgFileFormat] Failed to check built-in fusion config json file format, \
        the file path: %s",
        built_in_fusion_config_json_file.c_str());
    return FAILED;
  }
  return SUCCESS;
}

Status FusionConfigParser::ParseFusionConfigFile() {
  FE_LOGI("%s ParseFusionConfigFile start.", engine_name_.c_str());
  FE_TIMECOST_START(ParseFusionConfigFile);

  string built_in_fusion_config_json_file = Configuration::Instance(engine_name_).GetBuiltInFusionConfigFilePath();
  string custom_fusion_config_json_file = Configuration::Instance(engine_name_).GetCustomFusionConfigFilePath();
  FE_LOGD("The built-in fusion config file path is: %s", built_in_fusion_config_json_file.c_str());
  FE_LOGD("The custom fusion config file path is: %s", custom_fusion_config_json_file.c_str());
  fusion_switch_map_.clear();
  fusion_priority_map_.clear();
  fusion_switch_detail_map_.clear();
  nlohmann::json custom_fusion_config_json;
  nlohmann::json build_in_fusion_config_json;
  std::map<std::string, std::string> error_key_map;
  error_key_map[EM_FILE] = custom_fusion_config_json_file;

  if (CheckAndReadFile(custom_fusion_config_json_file, custom_fusion_config_json, error_key_map) != SUCCESS) {
    return FAILED;
  }

  // Read built-in file
  if (ReadJsonFile(built_in_fusion_config_json_file, build_in_fusion_config_json) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][Init][ParseFusConfig] Read Json content from file:%s failed.",
                    built_in_fusion_config_json_file.c_str());
    return FAILED;
  }
  try {
    Status ret = CheckConfigFileFormat(custom_fusion_config_json, build_in_fusion_config_json,
                                       built_in_fusion_config_json_file, custom_fusion_config_json_file, error_key_map);
    if (ret != SUCCESS) {
      return ret;
    }
    ret = ConstructFusionSwitchMap(custom_fusion_config_json, build_in_fusion_config_json, SWITCH_TYPE);
    if (ret != SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][Init][ParseFusionSwitch]Failed to init fusion switch with file %s or %s. ErrNo : %u",
                      built_in_fusion_config_json_file.c_str(), custom_fusion_config_json_file.c_str(), ret);
      return FAILED;
    }
    ret = ConstructFusionPriorityMap(custom_fusion_config_json, build_in_fusion_config_json, PRIORITY_TYPE);
    if (ret != SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][Init][ParseFusionPrio]Failed to init fusion priority with file %s or %s. ErrNo : %u",
                      built_in_fusion_config_json_file.c_str(), custom_fusion_config_json_file.c_str(), ret);
      return FAILED;
    }
  } catch (const std::exception &e) {
    REPORT_FE_ERROR(
        "[GraphOpt][Init][ParseFusConfig] Fail to parse fusion config file[%s] or [%s] to Json.Error message is %s",
        custom_fusion_config_json_file.c_str(), built_in_fusion_config_json_file.c_str(), e.what());
    return ILLEGAL_JSON;
  }

  FE_TIMECOST_END(ParseFusionConfigFile, "FusionConfigParser::ParseFusionConfigFile");
  FE_LOGI("%s ParseFusionConfigFile success.", engine_name_.c_str());
  return SUCCESS;
}

Status FusionConfigParser::ConstructFusionSwitchMap(const nlohmann::json &pass_switch_file_json,
                                                    const nlohmann::json &BuildInPassSwitchFileJson,
                                                    const string &type_string) {
  FE_LOGD("Start to construct fusion switch map.");

  for (auto &elem_switch : BuildInPassSwitchFileJson[type_string].items()) {
    // If it is new format file, fusion_switch_detail_map_ need be clear each once.
    if (!old_format_or_not_) {
      fusion_switch_detail_map_.clear();
    }
    string fusion_type_temp = elem_switch.key();
    string fusion_type_string = StringUtils::Trim(fusion_type_temp);
    // It is not to change fusion_switch_detail_map_'s value when enter ModifyFusionSwitchMap first time,
    // because custom data may exist in fusion_switch_detail_map_.
    Status res1 = ModifyFusionSwitchMap(BuildInPassSwitchFileJson, type_string, fusion_type_temp, old_format_or_not_);
    Status res2 = SUCCESS;

    if (pass_switch_file_json != nullptr && pass_switch_file_json.find(type_string) != pass_switch_file_json.end() &&
        !old_format_or_not_) {
      res2 = ModifyFusionSwitchMap(pass_switch_file_json, type_string, fusion_type_temp, old_format_or_not_);
    }
    if (res1 != SUCCESS || res2 != SUCCESS) {
      return FAILED;
    }

    fusion_switch_map_.emplace(std::make_pair(fusion_type_string, fusion_switch_detail_map_));
  }
  return SUCCESS;
}

Status FusionConfigParser::ConstructFusionPriorityMap(const nlohmann::json &pass_switch_file_json,
                                                      const nlohmann::json &BuildInPassSwitchFileJson,
                                                      const string &type_string) {
  // TO DO judge custom keys
  FE_LOGD("Start to construct fusion priority map.");
  for (auto &elem_pro : BuildInPassSwitchFileJson[type_string].items()) {
    string fusion_type_temp = elem_pro.key();
    string fusion_type_string = StringUtils::Trim(fusion_type_temp);
    map<string, int32_t> map_temp;
    Status res1 =
        ModifyFusionPriorityMap(BuildInPassSwitchFileJson, map_temp, type_string, fusion_type_temp);
    Status res2 = SUCCESS;
    if (pass_switch_file_json != nullptr && pass_switch_file_json.find(type_string) != pass_switch_file_json.end()) {
      res2 = ModifyFusionPriorityMap(pass_switch_file_json, map_temp, type_string, fusion_type_temp);
    }
    if (res1 != SUCCESS) {
      std::map<std::string, std::string> error_key_map;
      error_key_map[EM_FILE] = BuildInPassSwitchFileJson;
      REPORT_FE_ERROR("[GraphOpt][FusionConfig][ConstrFusPriorMap] Failed to init fusion priority with file %s.",
                      error_key_map[EM_FILE].c_str());
      return FAILED;
    }
    if (res2 != SUCCESS) {
      std::map<std::string, std::string> error_key_map;
      error_key_map[EM_FILE] = pass_switch_file_json;
      REPORT_FE_ERROR("[GraphOpt][FusionConfig][ConstrFusPriorMap]Failed to init fusion priority with file %s.",
                      error_key_map[EM_FILE].c_str());
      return FAILED;
    }
    fusion_priority_map_.emplace(std::make_pair(fusion_type_string, map_temp));
  }
  return SUCCESS;
}

Status FusionConfigParser::ModifyFusionSwitchMap(const nlohmann::json &common_pass_switch_file_json,
                                                 const string &type_string, string fusion_type_temp,
                                                 bool update_map_value_or_not) {
  // judge common_pass_switch's key have or not have fusion_type_string
  if (common_pass_switch_file_json[type_string].find(fusion_type_temp) ==
      common_pass_switch_file_json[type_string].end()) {
    return SUCCESS;
  }
  if (!common_pass_switch_file_json[type_string][fusion_type_temp].is_object()) {
    REPORT_FE_ERROR(
        "[GraphOpt][FusionConfig][MdfFusSwtMap] The third level of json file should be object, actually is %s.",
        GetJsonType(common_pass_switch_file_json[type_string][fusion_type_temp]).c_str());
    return ILLEGAL_JSON;
  }

  for (auto &elem_inner : common_pass_switch_file_json[type_string][fusion_type_temp].items()) {
    string key_inner = elem_inner.key();
    string value_inner = elem_inner.value();

    if (!key_inner.empty()) {
      if (fusion_switch_detail_map_.find(StringUtils::Trim(key_inner)) != fusion_switch_detail_map_.end()) {
        if (update_map_value_or_not) {
          continue;
        }
        FE_LOGD("Modify [%s] fusion switch value, build-in - %s, custom - %s", StringUtils::Trim(key_inner).c_str(),
                fusion_switch_detail_map_[StringUtils::Trim(key_inner)].c_str(),
                StringUtils::Trim(value_inner).c_str());
        fusion_switch_detail_map_[StringUtils::Trim(key_inner)] = StringUtils::Trim(value_inner);
      } else {
        fusion_switch_detail_map_.emplace(std::make_pair(StringUtils::Trim(key_inner), StringUtils::Trim(value_inner)));
      }
    }
  }
  return SUCCESS;
}

Status FusionConfigParser::ModifyFusionPriorityMap(const nlohmann::json &common_pass_switch_file_json,
                                                   map<string, int32_t> &map_temp, const string &type_string,
                                                   string fusion_type_temp) const {
  // judge common_pass_switch's key have or not have fusion_type_string
  if (common_pass_switch_file_json[type_string].find(fusion_type_temp) ==
      common_pass_switch_file_json[type_string].end()) {
    return SUCCESS;
  }

  for (auto &elem_pro_level : common_pass_switch_file_json[type_string][fusion_type_temp].items()) {
    string fusion_level_temp = elem_pro_level.key();

    for (auto &elem_inner : common_pass_switch_file_json[type_string][fusion_type_temp][fusion_level_temp].items()) {
      string key_inner = elem_inner.key();
      string value_inner = elem_inner.value();
      string key_inner_string = StringUtils::Trim(key_inner);
      string value_inner_string = StringUtils::Trim(value_inner);
      // This is safety, because CheckFusionConfigJsonLastLayerFormat has been checked the value.
      int32_t priority = 0;
      try {
        priority = stoi(value_inner_string);
      } catch (...) {

        REPORT_FE_ERROR("[GraphOpt][FusionConfig][MdfFusPrioMap] Value %s in priority json is not integer.",
                        value_inner_string.c_str());
        return FAILED;
      }
      if (!key_inner.empty()) {
        if (map_temp.find(key_inner_string) != map_temp.end()) {
          FE_LOGD("Modify [%s] fusion priority value, build-in - %d, custom - %d", key_inner_string.c_str(),
                  map_temp[key_inner_string], priority);
          map_temp[key_inner_string] = priority;
        } else {
          map_temp.emplace(std::make_pair(key_inner_string, priority));
        }
      }
    }
  }
  return SUCCESS;
}

Status FusionConfigParser::JudgePriority(string level_elem, int32_t priority_value, const string &owner_type) const {
  const int32_t first_condition =
      (owner_type == CUSTOM_TYPE) ? CUSTOM_CFG_TOP_PRIORITY_MIN : BUILT_IN_CFG_TOP_PRIORITY_MIN;
  const int32_t second_condition =
      (owner_type == CUSTOM_TYPE) ? CUSTOM_CFG_MAIN_PRIORITY_MIN : BUILT_IN_CFG_MAIN_PRIORITY_MIN;
  const int32_t third_condition =
      (owner_type == CUSTOM_TYPE) ? CUSTOM_CFG_DOWN_PRIORITY_MIN : BUILT_IN_CFG_DOWN_PRIORITY_MIN;
  const int32_t fourth_condition =
      (owner_type == CUSTOM_TYPE) ? BUILT_IN_CFG_TOP_PRIORITY_MIN : CUSTOM_PASS_PRIORITY_MIN;

  string level_elem_string = StringUtils::Trim(level_elem);

  if (level_elem_string == PRIORITY_TOP && (priority_value < first_condition || priority_value >= second_condition)) {
    REPORT_FE_ERROR(
        "[GraphOpt][FusionConfig][JdgPriority] The priority value [%d] of top level in fusion config \
        file[%s] is illegal, out of range [%d, %d).",
        priority_value, owner_type.c_str(), first_condition, second_condition);
    return FAILED;
  }
  if (level_elem_string == PRIORITY_MAIN && (priority_value < second_condition || priority_value >= third_condition)) {
    REPORT_FE_ERROR(
        "[GraphOpt][FusionConfig][JdgPriority] The priority value [%d] of main level in fusion config \
        file[%s] is illegal, out of range [%d, %d).",
        priority_value, owner_type.c_str(), second_condition, third_condition);
    return FAILED;
  }
  if (level_elem_string == PRIORITY_DOWN && (priority_value < third_condition || priority_value >= fourth_condition)) {
    REPORT_FE_ERROR(
        "[GraphOpt][FusionConfig][JdgPriority] The priority value [%d] of down level in fusion config \
        file[%s] is illegal, out of range [%d, %d).",
        priority_value, owner_type.c_str(), third_condition, fourth_condition);
    return FAILED;
  }

  return SUCCESS;
}

Status FusionConfigParser::CheckLastLayerFormatForPriorityPart(string type_elem, string fusion_elem,
                                                               nlohmann::json pass_switch_file_json,
                                                               const string &owner_type) const {
  unordered_set<string> pass_name_set;
  for (auto &elem3 : pass_switch_file_json[type_elem][fusion_elem].items()) {
    string level_elem = elem3.key();
    string level_elem_string = StringUtils::Trim(level_elem);

    if (level_elem_string != PRIORITY_TOP && level_elem_string != PRIORITY_MAIN && level_elem_string != PRIORITY_DOWN) {
      REPORT_FE_ERROR(
          "[GraphOpt][FusionConfig][ChkLstLyrFmt] The third level key [%s] of [%s] json file is error, \
          should be top/main/down.",
          level_elem.c_str(), owner_type.c_str());
      return FAILED;
    }
    if (!pass_switch_file_json[type_elem][fusion_elem][level_elem].is_object()) {
      REPORT_FE_ERROR(
          "[GraphOpt][FusionConfig][ChkLstLyrFmt] The fourth level of [%s] json file should be json form, \
          actually is %s.",
          owner_type.c_str(), GetJsonType(pass_switch_file_json[type_elem][fusion_elem][level_elem]).c_str());
      return ILLEGAL_JSON;
    }

    // This layer is pass_name:priority
    for (auto &elem4 : pass_switch_file_json[type_elem][fusion_elem][level_elem].items()) {
      string key_inner = elem4.key();
      string value_inner = elem4.value();
      string key_inner_string = StringUtils::Trim(key_inner);
      string value_inner_string = StringUtils::Trim(value_inner);
      // check the same pass
      FE_CHECK(pass_name_set.find(key_inner_string) != pass_name_set.end(),
               REPORT_FE_ERROR("[GraphOpt][FusionConfig][ChkLstLyrFmt] Priority pass[%s] is repetitive, please check \
               it.", key_inner_string.c_str()),
               return FAILED);
      pass_name_set.insert(key_inner_string);
      if (!StringUtils::IsInteger(value_inner_string)) {
        REPORT_FE_ERROR(
            "[GraphOpt][FusionConfig][ChkLstLyrFmt] The pass[%s] of priority[%s] value is illegal, please check it.",
            key_inner_string.c_str(), value_inner_string.c_str());
        return FAILED;
      }
      int32_t priority_value = 0;
      try {
        priority_value = stoi(value_inner_string);
      } catch (...) {
        REPORT_FE_ERROR("[GraphOpt][FusionConfig][ChkLstLyrFmt] Fail to convert [%s] to integer.",
                        value_inner_string.c_str());
        return FAILED;
      }
      FE_CHECK(JudgePriority(level_elem, priority_value, owner_type) != SUCCESS,
               REPORT_FE_ERROR("[GraphOpt][FusionConfig][ChkLstLyrFmt] Fail to check the pass[%s] priority of owner_\
               type[%s] and level[%s]", key_inner_string.c_str(), owner_type.c_str(), level_elem.c_str()),
               return FAILED);
    }
  }
  return SUCCESS;
}

Status FusionConfigParser::CheckFusionConfigJsonLastLayerFormat(string type_elem, string fusion_elem,
                                                                nlohmann::json pass_switch_file_json,
                                                                const string &owner_type) const {
  if (StringUtils::Trim(type_elem) == SWITCH_TYPE) {
    // This layer is pass_name:on/off
    for (auto &elem3 : pass_switch_file_json[type_elem][fusion_elem].items()) {
      string key_inner = elem3.key();
      string value_inner = elem3.value();
      string key = StringUtils::Trim(key_inner);
      string value = StringUtils::Trim(value_inner);

      if (value != SWITCH_ON && value != SWITCH_OFF) {
        REPORT_FE_ERROR(
            "[GraphOpt][FusionConfig][ChkFusCfgJsLyrFmt] The pass's switch of json file should be on/off, \
            actually is %s.", value.c_str());
        return FAILED;
      }
    }
  } else {
    // This layer is top/main/down
    Status ret = CheckLastLayerFormatForPriorityPart(type_elem, fusion_elem, pass_switch_file_json, owner_type);
    if (ret != SUCCESS) {
      return ret;
    }
  }
  return SUCCESS;
}

Status FusionConfigParser::CheckFusionConfigJsonFormat(nlohmann::json pass_switch_file_json,
                                                       const string &owner_type) const {
  FE_LOGD("Start to check %s fusion config file.", owner_type.c_str());

  if (pass_switch_file_json == nullptr) {
    FE_LOGD("The %s fusion config file is not exist, don't need to check.", owner_type.c_str());
    return SUCCESS;
  }

  // This layer is Switch/Priority.
  for (auto &elem : pass_switch_file_json.items()) {
    string type_elem = elem.key();
    string type_elem_string = StringUtils::Trim(type_elem);
    if (type_elem_string != SWITCH_TYPE && type_elem_string != PRIORITY_TYPE) {
      REPORT_FE_ERROR(
          "[GraphOpt][FusionConfig][ChkFusCfgJsFmt] The first level key [%s] of [%s] json file is error, \
          should be Switch or Priority.", type_elem.c_str(), owner_type.c_str());
      return FAILED;
    }
    if (!pass_switch_file_json[type_elem].is_object()) {
      REPORT_FE_ERROR(
          "[GraphOpt][FusionConfig][ChkFusCfgJsLyrFmt] The second level of [%s] json file should be json \
          form, actually [%s].",
          owner_type.c_str(), GetJsonType(pass_switch_file_json[type_elem]).c_str());
      return ILLEGAL_JSON;
    }

    // This layer is GraphFusion/UBFusion.
    for (auto &elem2 : pass_switch_file_json[type_elem].items()) {
      string fusion_elem = elem2.key();
      string fusion_elem_string = StringUtils::Trim(fusion_elem);
      if (fusion_elem_string != GRAPH_FUSION && fusion_elem_string != UB_FUSION) {
        REPORT_FE_ERROR(
            "[GraphOpt][FusionConfig][ChkFusCfgJsLyrFmt] The second level key [%s] of [%s] json file is \
            error, should be GraphFusion or UBFusion.",
            fusion_elem.c_str(), owner_type.c_str());
        return FAILED;
      }
      if (!pass_switch_file_json[type_elem][fusion_elem].is_object()) {
        REPORT_FE_ERROR(
            "[GraphOpt][FusionConfig][ChkFusCfgJsLyrFmt] The third level of [%s] json file should be json \
            form, actually is %s.",
            owner_type.c_str(), GetJsonType(pass_switch_file_json[type_elem][fusion_elem]).c_str());
        return ILLEGAL_JSON;
      }

      // This layer is on/off or top/main/down
      if (CheckFusionConfigJsonLastLayerFormat(type_elem, fusion_elem, pass_switch_file_json, owner_type) != SUCCESS) {
        return FAILED;
      }
    }
  }
  return SUCCESS;
}
}  // namespace fe
