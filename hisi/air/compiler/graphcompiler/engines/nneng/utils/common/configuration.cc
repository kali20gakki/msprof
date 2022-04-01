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

#include "common/configuration.h"
#include <dlfcn.h>
#include <algorithm>
#include <sstream>
#include "common/aicore_util_constants.h"
#include "common/comm_error_codes.h"
#include "common/comm_log.h"
#include "common/common_utils.h"
#include "common/constants_define.h"
#include "common/fe_error_code.h"
#include "common/fe_type_utils.h"
#include "common/string_utils.h"
#include "ge/ge_api_types.h"
#include "graph/ge_context.h"
#include "graph/tuning_utils.h"

namespace fe {
const std::string Configuration::CONFIG_FILE_RELATIVE_PATH = "plugin/opskernel/fe_config/fe.ini";
const std::string Configuration::CONFIG_OPS_RELATIVE_PATH = "../../opp/";
const std::vector<std::string> kOpSelectImplModeVec = {"high_precision", "high_performance"};
const std::vector<std::string> kOpSelectImplModeAllVec = {"high_precision_for_all", "high_performance_for_all"};
const std::string kOpImplModeRelativePath = "op_impl/built-in/ai_core/tbe/impl_mode/";
const std::string kConfigFileSuffix = ".ini";

bool ComparePriority(const FEOpsStoreInfo &left, const FEOpsStoreInfo &right) { return left.priority < right.priority; }

Configuration::Configuration(std::string engine_name)
    : is_init_(false),
      engine_name_(engine_name),
      content_map_(),
      ops_store_info_vector_(),
      enable_small_channel_(false),
      enable_compress_weight_(false),
      allow_all_scope_id_(false),
      isa_arch_ver_(ISAArchVersion::EN_ISA_ARCH_V100),
      append_args_mode_(AppendArgsMode::NO_ARGS),
      buffer_fusion_mode_(EN_OPTIMIZE_DISABLE),
      buffer_optimize_(EN_UNKNOWN_OPTIMIZE),
      auto_tune_mode_(TUNE_MODE_NO_TUNE),
      data_visit_dist_threshold_(DATA_VISIT_DIST_THRESHOLD),
      mem_reuse_dist_threshold_(MEM_REUSE_DIST_THRESHOLD) {}

Configuration::~Configuration() {}

Configuration &Configuration::Instance(const std::string &engine_name) {
  static std::map<std::string, Configuration &> config_map;
  if (config_map.empty()) {
    static Configuration ai_core_config(AI_CORE_NAME);
    static Configuration vector_core_config(VECTOR_CORE_NAME);
    static Configuration dsa_core_config(kDsaCoreName);
    config_map.insert({AI_CORE_NAME, ai_core_config});
    config_map.insert({VECTOR_CORE_NAME, vector_core_config});
    config_map.insert({kDsaCoreName, dsa_core_config});
  }
  std::map<std::string, Configuration &>::const_iterator iter = config_map.find(engine_name);
  if (iter != config_map.cend()) {
    return iter->second;
  }
  CM_LOGD("Engine name %s is not found, using the default instance.", engine_name.c_str());
  /* If engine_name is invalid, we just return the first element of map
   * config_map */
  return config_map.begin()->second;
}

Status Configuration::Initialize(const std::map<std::string, std::string> &options,
                                 const std::string &soc_version) {
  if (is_init_) {
    return SUCCESS;
  }

  CM_LOGD("Configuration begin to initialize.");

  if (soc_version.empty()) {
    REPORT_CM_ERROR("[GraphOpt][Init] The value of soc version is empty.");
    return FAILED;
  }
  soc_version_ = soc_version;

  // initialize parameters based on the options from ge
  Status status = InitOptions(options);
  if (status != SUCCESS) {
    REPORT_CM_ERROR("[GraphOpt][Init] Fail to initialize parameters which is based on options from ge.");
    return FAILED;
  }

  status = InitLibPath();
  if (status != SUCCESS) {
    REPORT_CM_ERROR("[GraphOpt][Init] Fail to initialize the real path of libfe.");
    return FAILED;
  }

  status = InitAscendOpsPath();
  if (status != SUCCESS) {
    REPORT_CM_ERROR("[GraphOpt][Init] Failed to load ascend ops path.");
    return status;
  }

  status = LoadConfigFile();
  if (status != SUCCESS) {
    REPORT_CM_ERROR("[GraphOpt][Init] Failed to load the data from configuration file.");
    return status;
  }

  status = ValidateConfigItems();
  if (status != SUCCESS) {
    REPORT_CM_ERROR("[GraphOpt][Init] Failed to validate the mandatory config items for config file.");
    return status;
  }

  status = AssembleOpsStoreInfoVector();
  if (status != SUCCESS) {
    REPORT_CM_ERROR("[GraphOpt][Init] Failed to build OpsStoreInfo Vector.");
    return status;
  }

  InitEnableNetworkAnalysis();

  FillupDefaultConfigValues();

  // init other parameters based on soc version and items of ini file
  InitParametersOfConfigFile();

  Status precision_mode_status = InitPrecisionMode();
  if (precision_mode_status != SUCCESS) {
    REPORT_CM_ERROR("[GraphOpt][Init] Fail to initialize precision mode.");
    return precision_mode_status;
  }

  Status op_precision_mode_status = InitOpPrecisionMode(options);
  if (op_precision_mode_status != SUCCESS) {
    REPORT_CM_ERROR("[GraphOpt][Init] Fail to init op precision mode.");
    return op_precision_mode_status;
  }

  is_init_ = true;
  CM_LOGI("Initialization of Configuration end.");
  return SUCCESS;
}

template <typename T>
Status Configuration::InitFromGeContext(const std::map<std::string, T> str_map,
                                        const std::string &key, T &value) const {
  std::string value_in_context;
  if (GetGeContextStringValue(key, value_in_context) == SUCCESS && !value_in_context.empty()) {
    CM_LOGD("The value of parameter [%s] is %s.", key.c_str(), value_in_context.c_str());
    auto iter_map = str_map.find(value_in_context);

    if (iter_map == str_map.end()) {
      std::map<std::string, std::string> error_key_map;
      error_key_map[EM_VALUE] = value_in_context;
      error_key_map[EM_OPTION] = key;
      ReportErrorMessage(EM_INPUT_OPTION_INVALID, error_key_map);
      REPORT_CM_ERROR("[GraphOpt][Init] The value[%s] of [%s] in GE context is invalid.",
                      value_in_context.c_str(), key.c_str());
      return FAILED;
    }
    value = iter_map->second;
  } else {
    CM_LOGI("GeContext does not contain parameter[%s] or its value is empty.", key.c_str());
  }
  return SUCCESS;
}

template <typename T>
Status Configuration::InitSingleParamFromOptions(const std::map<std::string, std::string> &options,
                                                 const std::map<std::string, T> str_map, const std::string &key,
                                                 T &value) const {
  auto iter = options.find(key);
  if (iter != options.end() && !iter->second.empty()) {
    CM_LOGD("The value of parameter [%s] is %s.", iter->first.c_str(), iter->second.c_str());
    auto iter_map = str_map.find(iter->second);

    if (iter_map == str_map.end()) {
      std::map<std::string, std::string> error_key_map;
      error_key_map[EM_VALUE] = iter->second;
      error_key_map[EM_OPTION] = key;
      ReportErrorMessage(EM_INPUT_OPTION_INVALID, error_key_map);
      REPORT_CM_ERROR("[GraphOpt][Init][InitSingParm] The value[%s] of [%s] is invalid.",
                      iter->first.c_str(), iter->second.c_str());
      return FAILED;
    }

    value = iter_map->second;
  } else {
    CM_LOGD("Option does not contain parameter [%s] or its value is empty.", key.c_str());
  }
  return SUCCESS;
}

Status Configuration::InitOpPrecisionMode(const std::map<std::string, std::string> &options) {
  // 1. Check op precision mode
  auto iter = options.find(ge::OP_PRECISION_MODE);
  if (iter != options.end() && !iter->second.empty()) {
    std::string op_precision_mode = iter->second;
    FE_LOGD("The value of parameter [op_precision_mode] is [%s].", op_precision_mode.c_str());
    return InitOpPrecisionModeByPrecisionMode(op_precision_mode);
  }
  FE_LOGD("The parameter [op_precision_mode] is not found or its value is empty.");

  iter = options.find(ge::OP_SELECT_IMPL_MODE);
  if (iter == options.end() || iter->second.empty()) {
    FE_LOGD("The parameter[op_select_impl_mode] is not found or its value is empty.");
    return SUCCESS;
  }
  std::string op_select_impl_mode = iter->second;
  FE_LOGD("The value of parameter[op_select_impl_mode] is [%s].", op_select_impl_mode.c_str());
  if (std::find(kOpSelectImplModeAllVec.begin(), kOpSelectImplModeAllVec.end(), op_select_impl_mode) !=
          kOpSelectImplModeAllVec.end()) {
    return InitOpPrecisionModeByImplModeAll(op_select_impl_mode);
  }
  if (std::find(kOpSelectImplModeVec.begin(), kOpSelectImplModeVec.end(), op_select_impl_mode) !=
          kOpSelectImplModeVec.end()) {
    std::string op_type_list_str;
    iter = options.find(ge::OPTYPELIST_FOR_IMPLMODE);
    if (iter != options.end()) {
      op_type_list_str = iter->second;
      FE_LOGD("The value of parameter [optypelist_for_implmode] is [%s].", op_type_list_str.c_str());
      op_type_list_str = StringUtils::Trim(op_type_list_str);
    }
    return InitOpPrecisionModeByImplMode(op_select_impl_mode, op_type_list_str);
  }

  REPORT_CM_ERROR("[GraphOpt][Init][InitOpPrecisionMode] Para:op_select_impl_mode[%s] is invalid.",
                  op_select_impl_mode.c_str());
  std::map<std::string, std::string> error_key_map;
  error_key_map[EM_VALUE] = op_select_impl_mode;
  error_key_map[EM_OPTION] = ge::OP_SELECT_IMPL_MODE;
  ReportErrorMessage(EM_INPUT_OPTION_INVALID, error_key_map);
  return FAILED;
}

Status Configuration::InitOpPrecisionModeByPrecisionMode(const std::string &op_precision_mode) {
  if (op_precision_mode.empty()) {
    return SUCCESS;
  }
  // check whether file is existed
  std::string file_path = GetRealPath(op_precision_mode);
  if (file_path.empty()) {
    REPORT_CM_ERROR("[GraphOpt][Init][InitOpPrecisionMode] op precision mode config file [%s] is not existed.",
                    op_precision_mode.c_str());
    return FAILED;
  }
  Status ret = GetOpPrecisonModeStrFromConfigFile(file_path);
  if (ret != SUCCESS) {
    REPORT_CM_ERROR("[GraphOpt][Init][InitOpPrecisionMode] Fail to get op_precision_mode string from [%s]file.",
                    op_precision_mode.c_str());
    return ret;
  }
  return SUCCESS;
}

Status Configuration::InitOpPrecisionModeByImplModeAll(const std::string &op_select_impl_mode_all) {
  string file_path = ascend_ops_path_ + kOpImplModeRelativePath + op_select_impl_mode_all + kConfigFileSuffix;
  std::string real_file_path = GetRealPath(file_path);
  if (real_file_path.empty()) {
    CM_LOGW("[%s] file [%s] is not existed.", op_select_impl_mode_all.c_str(), file_path.c_str());
    return SUCCESS;
  }
  Status ret = GetOpPrecisonModeStrFromConfigFile(real_file_path);
  if (ret != SUCCESS) {
    REPORT_CM_ERROR("[GraphOpt][Init][InitOpPrecisionMode] Fail to get op_precision_mode string from [%s]file.",
                    op_select_impl_mode_all.c_str());
    return ret;
  }
  return SUCCESS;
}

Status Configuration::InitOpPrecisionModeByImplMode(const std::string &op_select_impl_mode,
                                                    const std::string &op_type_list_str) {
  if (!op_type_list_str.empty()) {
    std::vector<std::string> op_type_list = StringUtils::Split(op_type_list_str, ',');
    for (std::string &op_type : op_type_list) {
      op_type = StringUtils::Trim(op_type);
      op_select_impl_mode_map_.emplace(op_type, op_select_impl_mode);
    }
  } else {
    string file_path = ascend_ops_path_ + kOpImplModeRelativePath + op_select_impl_mode + kConfigFileSuffix;
    std::string real_file_path = GetRealPath(file_path);
    if (real_file_path.empty()) {
      CM_LOGW("[%s] file [%s] is not existed.", op_select_impl_mode.c_str(), file_path.c_str());
      return SUCCESS;
    }
    Status status = GetOpPrecisonModeStrFromConfigFile(real_file_path);
    if (status != SUCCESS) {
      REPORT_CM_ERROR("[GraphOpt][Init][InitOpPrecisionMode] Fail to get op_precision_mode string from [%s]file.",
                      op_select_impl_mode.c_str());
      return status;
    }
  }
  return SUCCESS;
}

Status Configuration::GetOpPrecisonModeStrFromConfigFile(const std::string &file_path) {
  FE_LOGD("Begin to load op select impl mode file[%s]", file_path.c_str());
  std::ifstream ifs(file_path);
  if (!ifs.is_open()) {
    REPORT_CM_ERROR("[GraphOpt][InitOpPrecisonMode] Fail to open config file[%s].", file_path.c_str());
    return INVALID_FILE_PATH;
  }

  std::string line;
  while (std::getline(ifs, line)) {
    if (line.empty() || line.find('#') == 0) {
      continue;
    }
    size_t pos_of_equal = line.find('=');
    if (pos_of_equal == std::string::npos) {
      continue;
    }
    std::string op_type = line.substr(0, pos_of_equal);
    std::string impl_mode = line.substr(pos_of_equal + 1);
    op_type = StringUtils::Trim(op_type);
    impl_mode = StringUtils::Trim(impl_mode);
    if (!op_type.empty() && !impl_mode.empty()) {
      op_select_impl_mode_map_.emplace(op_type, impl_mode);
    }
  }
  ifs.close();
  FE_LOGD("Finish parsing op select impl mode file [%s]", file_path.c_str());
  return SUCCESS;
}

void Configuration::GetOpSelectImplModeStr(std::string &op_select_impl_mode_str) const {
  std::ostringstream oss;
  for (auto iter = op_select_impl_mode_map_.begin(); iter != op_select_impl_mode_map_.end(); iter++) {
    if (iter != op_select_impl_mode_map_.begin()) {
      oss << ",";
    }
    oss << iter->first << ":" << iter->second;
  }
  op_select_impl_mode_str = oss.str();
}

Status Configuration::InitOptions(const std::map<std::string, std::string> &options) {
  // ENABLE_SMALL_CHANNEL
  Status status = InitSingleParamFromOptions(options, GE_SWITCH_MAP, ge::ENABLE_SMALL_CHANNEL, enable_small_channel_);
  if (status != SUCCESS) {
    return status;
  }
  CM_LOGD("The value of enable_small_channel is %d.", enable_small_channel_);

  // ENABLE_COMPRESS_WEIGHT
  status = InitSingleParamFromOptions(options, GE_SWITCH_MAP, ge::ENABLE_COMPRESS_WEIGHT, enable_compress_weight_);
  if (status != SUCCESS) {
    return status;
  }
  CM_LOGD("The value of enable_compress_weight is %d.", enable_compress_weight_);

  // BUFFER_OPTIMIZE
  status = InitSingleParamFromOptions(options, BUFFER_OPTIMIZE_MAP, ge::BUFFER_OPTIMIZE, buffer_optimize_);
  if (status != SUCCESS) {
    return status;
  }

  status = InitSingleParamFromOptions(options, GE_DISABLE_REUSE_MEMORY_MAP, ge::OPTION_EXEC_DISABLE_REUSED_MEMORY,
                                      is_enable_reuse_mem_);
  if (status != SUCCESS) {
    return status;
  }
  CM_LOGD("The value of buffer_optimize is %s.", GetBufferOptimizeString(buffer_optimize_).c_str());

  status = InitSingleParamFromOptions(options, AUTO_TUNE_MODE_MAP, ge::AUTO_TUNE_MODE, auto_tune_mode_);
  if (status != SUCCESS) {
    return status;
  }

  auto iter_fusion = options.find(ge::FUSION_SWITCH_FILE);
  if (iter_fusion != options.end() && !iter_fusion->second.empty()) {
    CM_LOGD("The value of parameter[ge.fusion_switch_file] is [%s].", iter_fusion->second.c_str());
    custom_fusion_config_file_ = iter_fusion->second;
  } else {
    CM_LOGD("the ge.fusion_switch_file is not find in ge.option.");
  }

  auto iter = options.find(ge::MODIFY_MIXLIST);
  if (iter != options.end() && !iter->second.empty()) {
    CM_LOGD("The value of para [ge.exec.modify_mixlist] is [%s].", iter->second.c_str());
    modify_mixlist_path_ = iter->second;
  } else {
    CM_LOGD("The value of para [ge.exec.modify_mixlist] is not config in ge.option.");
    modify_mixlist_path_ = "";
  }

  return SUCCESS;
}

void Configuration::InitParametersOfConfigFile() {
  InitISAArchVersion();

  InitBufferFusionMode();

  InitAppendArgsMode();

  InitScopeId();

  InitUseCmo();

  InitMemReuseDistThreshold();

  InitCompressRatio();
}

Status Configuration::Finalize() {
  CM_LOGD("Finalizing the Configuration...");
  if (!is_init_) {
    CM_LOGD("Configuration has not been initialized, Finalize is not allowed.");
    return SUCCESS;
  }

  content_map_.clear();
  ops_store_info_vector_.clear();
  is_init_ = false;
  CM_LOGI("Configuration finalize successfully.");
  return SUCCESS;
}

Status Configuration::InitLibPath() {
  Dl_info dl_info;
  Configuration &(*instance_ptr)(const std::string &) = &Configuration::Instance;
  if (dladdr(reinterpret_cast<void *>(instance_ptr), &dl_info) == 0) {
    REPORT_CM_ERROR("[GraphOpt][Init][InitLibPath] Failed to get so file path.");
    return FAILED;
  } else {
    std::string so_path = dl_info.dli_fname;
    CM_LOGD("Libfe so file path is %s.", so_path.c_str());

    if (so_path.empty()) {
      REPORT_CM_ERROR("[GraphOpt][Init][InitLibPath] So file path is empty.");
      return FAILED;
    }

    lib_path_ = GetRealPath(so_path);
    int32_t pos = lib_path_.rfind('/');
    if (pos < 0) {
      REPORT_CM_ERROR("[GraphOpt][Init][InitLibPath] The path of current so file dose not contain /.");
      return FAILED;
    }

    lib_path_ = lib_path_.substr(0, pos + 1);
  }
  CM_LOGD("The real path of libfe is: %s", lib_path_.c_str());
  return SUCCESS;
}

Status Configuration::InitAscendOpsPath() {
  const char *ascend_ops_path_ptr = std::getenv(ASCEND_OPP_PATH);
  ascend_ops_path_ = "";
  if (ascend_ops_path_ptr != nullptr) {
    std::string ascend_ops_path = string(ascend_ops_path_ptr);
    ascend_ops_path_ = GetRealPath(ascend_ops_path);
  } else {
    ascend_ops_path_ = GetRealPath(lib_path_ + CONFIG_OPS_RELATIVE_PATH);
  }
  CM_LOGD("The path of Ascend opp is: %s", ascend_ops_path_.c_str());
  if (ascend_ops_path_.empty()) {
    REPORT_CM_ERROR("[GraphOpt][Init][InitOpsPath] Ascend opp path is empty.");
    return INVALID_FILE_PATH;
  }

  ascend_ops_path_ = ascend_ops_path_ + "/";
  CM_LOGD("The real file path of Ascend ops is %s.", ascend_ops_path_.c_str());
  return SUCCESS;
}

void Configuration::InitEnableNetworkAnalysis() {
  CM_LOGD("InitEnableNetworkAnalysis begin.");
  const char *enable_network_analysis_ptr = std::getenv("ENABLE_NETWORK_ANALYSIS_DEBUG");
  if (enable_network_analysis_ptr == nullptr) {
    CM_LOGD("[GraphOpt][Init][InitEnableNetworkAnalysis]ENABLE_NETWORK_ANALYSIS_DEBUG is null.");
    return;
  }
  std::string enable_network_analysis_str(enable_network_analysis_ptr);
  enable_network_analysis_ = atoi(enable_network_analysis_str.c_str());
  CM_LOGD("[GraphOpt][Init][InitEnableNetworkAnalysis]The enable_network_analysis is: %d.",
          enable_network_analysis_);
  CM_LOGD("InitEnableNetworkAnalysis end.");
}

Status Configuration::LoadConfigFile() {
  CM_LOGD("Begin to LoadConfigFile.");
  std::string config_file_real_path = GetRealPath(lib_path_ + CONFIG_FILE_RELATIVE_PATH);
  CM_LOGI("The real path of fe.ini is %s.", config_file_real_path.c_str());

  if (config_file_real_path.empty()) {
    REPORT_CM_ERROR("[GraphOpt][InitOpsPath] Fe configuration file path is invalid.");
    return INVALID_FILE_PATH;
  }

  std::ifstream ifs(config_file_real_path);
  if (!ifs.is_open()) {
    REPORT_CM_ERROR("[GraphOpt][InitOpsPath] Open configuration file failed. The file does not exist or has been \
                    opened.");
    return INVALID_FILE_PATH;
  }

  content_map_.clear();
  std::string line;
  while (std::getline(ifs, line)) {
    if (line.empty()) {
      continue;
    }
    line = StringUtils::Trim(line);
    if (line.empty() || line.find('#') == 0) {
      continue;
    }

    size_t pos_of_equal = line.find('=');
    if (pos_of_equal == std::string::npos) {
      continue;
    }
    std::string key = line.substr(0, pos_of_equal);
    std::string value = line.substr(pos_of_equal + 1);
    key = StringUtils::Trim(key);
    value = StringUtils::Trim(value);
    if (!key.empty()) {
      content_map_.emplace(key, value);
    }
  }
  ifs.close();
  CM_LOGD("End to loadConfigFile.");
  return SUCCESS;
}

Status Configuration::ValidateConfigItems() const {
  CM_LOGD("ValidateConfigItems begin.");
  for (const std::string &config_key : MANDATORY_CONFIG_ITEMS) {
    if (!ContainKey(config_key)) {
      REPORT_CM_ERROR("[GraphOpt][Init][ValidConfigItem] The mandatory config item %s is not configured.",
                      config_key.c_str());
      return LACK_MANDATORY_CONFIG_KEY;
    }
  }
  CM_LOGD("End to validateConfigItems.");
  return SUCCESS;
}

void Configuration::FillupDefaultConfigValues() {
  CM_LOGD("FillupDefaultConfigValues begin.");
  for (const auto &iter : DEFAULT_CONFIG_ITEM_VALUE) {
    if (!ContainKey(iter.first)) {
      content_map_.emplace(iter.first, iter.second);
    }
  }
  CM_LOGD("FillupDefaultConfigValues end.");
}

bool Configuration::IsIgnoreOpStore(const FEOpsStoreInfo &ops_store_info) const {
  bool is_vevtor_core_type = ops_store_info.op_impl_type == EN_IMPL_VECTOR_CORE_HW_TBE ||
                             ops_store_info.op_impl_type == EN_IMPL_VECTOR_CORE_CUSTOM_TBE;
  bool is_dsa_core_type = ops_store_info.op_impl_type == EN_IMPL_HW_DSA;
  return (engine_name_ == AI_CORE_NAME && (is_vevtor_core_type || is_dsa_core_type)) ||
         (engine_name_ == VECTOR_CORE_NAME && !is_vevtor_core_type) ||
         (engine_name_ == kDsaCoreName && !is_dsa_core_type);
}

Status Configuration::AssembleOpsStoreInfoVector() {
  CM_LOGD("AssembleOpsStoreInfoVector of engine %s begin.", engine_name_.c_str());

  if (content_map_.empty()) {
    REPORT_CM_ERROR("[GraphOpt][Init][AssmOpsStoreInfoVec] There is no OP store item in fe.ini.");
    return OPSTORE_EMPTY;
  }
  ops_store_info_vector_.clear();

  std::map<std::string, std::string>::iterator iter;
  for (iter = content_map_.begin(); iter != content_map_.end(); ++iter) {
    if (iter->first.find(OP_STOREINFO_PREFIX) == std::string::npos) {
      continue;
    }

    // Format : op.storeinfo.name=priority|op_impl_type|cfg_file_path|op_impl_file_path
    std::string op_store_name = iter->first.substr(OP_STOREINFO_PREFIX.length());
    if (op_store_name.empty()) {
      REPORT_CM_ERROR("[GraphOpt][Init][AssmOpsStoreInfoVec] The name of op store info config item is empty.");
      return OPSTORE_NAME_EMPTY;
    }
    if (iter->second.empty()) {
      REPORT_CM_ERROR("[GraphOpt][Init][AssmOpsStoreInfoVec] The value of [%s] in fe.ini file is empty.",
                      op_store_name.c_str());
      return OPSTORE_VALUE_EMPTY;
    }

    std::vector<std::string> op_store_vector = StringUtils::Split(iter->second, '|');
    Status verify_status = VerifyOpStoreVector(op_store_vector, op_store_name);
    if (verify_status != SUCCESS) {
      REPORT_CM_ERROR("[GraphOpt][Init][AssmOpsStoreInfoVec] Fail to verify op store [%s].", op_store_name.c_str());
      return verify_status;
    }

    FEOpsStoreInfo ops_store_info;
    Status status = AssembleEachOpsStoreInfo(op_store_name, op_store_vector, ops_store_info);
    if (status != SUCCESS) {
      return status;
    }
    if (!IsIgnoreOpStore(ops_store_info)) {
      ops_store_info_vector_.push_back(ops_store_info);
      CM_LOGI("OP store [%s] has been added to ops_store_info_vector_ of %s.", ops_store_info.fe_ops_store_name.c_str(),
              engine_name_.c_str());
    }
  }

  if (ops_store_info_vector_.empty()) {
    REPORT_CM_ERROR("[GraphOpt][Init][AssmOpsStoreInfoVec] There is no OP store item in fe.ini of engine %s.",
                    engine_name_.c_str());
    return OPSTORE_EMPTY;
  }
  std::sort(ops_store_info_vector_.begin(), ops_store_info_vector_.end(), ComparePriority);
  CM_LOGD("Finish assemble OpsStoreInfoVector whose size is %zu of engine %s", ops_store_info_vector_.size(),
          engine_name_.c_str());
  return SUCCESS;
}

Status Configuration::AssembleEachOpsStoreInfo(std::string &op_store_name, std::vector<std::string> &op_store_vector,
                                               FEOpsStoreInfo &ops_store_info) {
  if (op_store_vector.size() > IDX_BOTTOM) {
    REPORT_CM_ERROR("[GraphOpt][Init] The opimpltype of opstore[%s] should be non-negative integer.",
                    op_store_name.c_str());
    return OPSTORE_CONFIG_NOT_INTEGRAL;
  }
  ops_store_info.fe_ops_store_name = op_store_name;
  std::istringstream(op_store_vector.at(IDX_PRIORITY)) >> ops_store_info.priority;

  std::string str_op_impl_type = op_store_vector.at(IDX_OPIMPL_TYPE);
  if (!StringUtils::IsInteger(str_op_impl_type)) {
    REPORT_CM_ERROR("[GraphOpt][Init][AssemEachOpsStoreInfo] The opimpltype of opstore[%s] should be non-negative \
                    integer.", op_store_name.c_str());
    return OPSTORE_OPIMPLTYPE_INVALID;
  }
  int16_t impl_type;
  std::istringstream(str_op_impl_type) >> impl_type;
  ops_store_info.op_impl_type = static_cast<OpImplType>(impl_type);

  Status status = CheckOpStoreInfo(ops_store_info);
  if (status != SUCCESS) {
    return status;
  }

  if (ops_store_info.op_impl_type != EN_IMPL_PLUGIN_TBE &&
      (op_store_vector.at(IDX_CFG_FILEPATH).empty() || op_store_vector.at(IDX_OPIMPL_FILEPATH).empty())) {
    REPORT_CM_ERROR("[GraphOpt][Init][AssemEachOpsStoreInfo] At least one of columns in [%s] value in fe.ini is empty.",
                    op_store_name.c_str());
    return OPSTORE_VALUE_ITEM_EMPTY;
  }

  std::string str_cfg_file_path = ascend_ops_path_ + op_store_vector.at(IDX_CFG_FILEPATH);
  std::string str_op_imply_file_path = ascend_ops_path_ + op_store_vector.at(IDX_OPIMPL_FILEPATH);

  if (ops_store_info.op_impl_type == EN_IMPL_HW_TBE || ops_store_info.op_impl_type == EN_IMPL_CUSTOM_TBE ||
      ops_store_info.op_impl_type == EN_IMPL_HW_DSA) {
    // default value of sub path is the lower case of soc version
    std::string sub_path = soc_version_;
    std::transform(sub_path.begin(), sub_path.end(), sub_path.begin(), ::tolower);
    auto iter = SOC_OPS_SUBPATH_MAP.find(soc_version_);
    if (iter != SOC_OPS_SUBPATH_MAP.end()) {
      sub_path = iter->second;
    }

    str_cfg_file_path = str_cfg_file_path + "/" + sub_path;
  }
  CM_LOGD("The configuration file path of op store[%s] is %s.", op_store_name.c_str(), str_cfg_file_path.c_str());
  CM_LOGD("The implementation path of op store[%s] is %s.", op_store_name.c_str(), str_cfg_file_path.c_str());
  ops_store_info.cfg_file_path = str_cfg_file_path;
  ops_store_info.op_impl_file_path = str_op_imply_file_path;

  std::string str_need_pre_compile = op_store_vector.at(IDX_NEED_PRECOMPILE);
  std::string str_need_compile = op_store_vector.at(IDX_NEED_COMPILE);
  std::transform(str_need_pre_compile.begin(), str_need_pre_compile.end(), str_need_pre_compile.begin(), ::tolower);
  std::transform(str_need_compile.begin(), str_need_compile.end(), str_need_compile.begin(), ::tolower);
  ops_store_info.need_pre_compile = str_need_pre_compile == NEED_PRECOMPILE_TRUE;
  ops_store_info.need_compile = str_need_compile == NEED_COMPILE_TRUE;

  return SUCCESS;
}

Status Configuration::VerifyOpStoreVector(std::vector<std::string> &op_store_vector,
                                          const std::string &op_store_name) const {
  if (op_store_vector.size() != OP_STORE_FORMAT_SIZE) {
    REPORT_CM_ERROR("[GraphOpt][Init][VerifyOpStoreVec] The columns size of [%s] value in fe.ini is not %d.",
                    op_store_name.c_str(), OP_STORE_FORMAT_SIZE);
    return OPSTORE_VALUE_ITEM_SIZE_INCORRECT;
  }

  if (op_store_vector.at(IDX_PRIORITY).empty() || op_store_vector.at(IDX_OPIMPL_TYPE).empty() ||
      op_store_vector.at(IDX_NEED_PRECOMPILE).empty() || op_store_vector.at(IDX_NEED_COMPILE).empty()) {
    REPORT_CM_ERROR("[GraphOpt][Init][VerifyOpStoreVec] At least one of columns in [%s] value in fe.ini is empty.",
                    op_store_name.c_str());
    return OPSTORE_VALUE_ITEM_EMPTY;
  }

  if (!StringUtils::IsInteger(op_store_vector.at(IDX_PRIORITY))) {
    REPORT_CM_ERROR("[GraphOpt][Init][VerifyOpStoreVec] The priority of opstore[%s] should be non-negative integer",
                    op_store_name.c_str());
    return OPSTORE_PRIORITY_INVALID;
  }
  return SUCCESS;
}

const std::string &Configuration::GetSocVersion() const { return soc_version_; }

ISAArchVersion Configuration::GetIsaArchVer() const { return isa_arch_ver_; }

void Configuration::SetOpsStoreInfo(const FEOpsStoreInfo &fe_ops_store_info) {
  std::lock_guard<std::mutex> lock_guard(ops_store_info_vector_mutex_);
  ops_store_info_vector_.push_back(fe_ops_store_info);
  std::sort(ops_store_info_vector_.begin(), ops_store_info_vector_.end(), ComparePriority);
}

const std::vector<FEOpsStoreInfo> &Configuration::GetOpsStoreInfo() const {
  std::lock_guard<std::mutex> lock_guard(ops_store_info_vector_mutex_);
  return ops_store_info_vector_;
}

Status Configuration::GetOpStoreInfoByImplType(OpImplType op_impl_type, FEOpsStoreInfo &op_store_info) const {
  CM_LOGD("Begin to get op store info by impl_type. op_impl_type=%d", op_impl_type);
  Status return_status = FAILED;
  std::vector<FEOpsStoreInfo> ops_store_info_vec = GetOpsStoreInfo();
  for (FEOpsStoreInfo &fe_op_store_info : ops_store_info_vec) {
    if (op_impl_type == fe_op_store_info.op_impl_type) {
      op_store_info = fe_op_store_info;
      return_status = SUCCESS;
      break;
    }
  }
  return return_status;
}

Status Configuration::CheckOpStoreInfo(const FEOpsStoreInfo &op_store_info) const {
  if (op_store_info.priority < PRIORITY_START_NUM) {
    REPORT_CM_ERROR("[GraphOpt][Init][ChkOpStoreInfo] The priority[%d] is out of range.", op_store_info.priority);
    return OPSTORE_PRIORITY_INVALID;
  }

  if (op_store_info.op_impl_type >= EN_RESERVED || op_store_info.op_impl_type < EN_IMPL_CUSTOM_CONSTANT_CCE) {
    REPORT_CM_ERROR("[GraphOpt][Init][ChkOpStoreInfo] The op impl type of op store [%s] is invalid.",
                    op_store_info.fe_ops_store_name.c_str());
    return OPSTORE_OPIMPLTYPE_INVALID;
  }

  for (const FEOpsStoreInfo &op_store : ops_store_info_vector_) {
    if (op_store.priority == op_store_info.priority) {
      REPORT_CM_ERROR("[GraphOpt][Init][ChkOpStoreInfo] The priority [%d] already exists in %s.",
                      op_store_info.priority, op_store.fe_ops_store_name.c_str());
      return OPSTORE_PRIORITY_INVALID;
    }
    if (op_store.op_impl_type == op_store_info.op_impl_type) {
      REPORT_CM_ERROR("[GraphOpt][Init][ChkOpStoreInfo] The op impl type of op stores can not be repeated.");
      return OPSTORE_OPIMPLTYPE_REPEAT;
    }
  }
  return SUCCESS;
}

bool Configuration::ContainKey(const std::string &key) const {
  auto iter = content_map_.find(key);
  return iter != content_map_.end();
}

Status Configuration::GetStringValue(const std::string &key, std::string &return_value) {
  auto iter = content_map_.find(key);
  if (iter == content_map_.end()) {
    CM_LOGW("Can not find the value of key %s", key.c_str());
    return FAILED;
  }

  return_value = iter->second;
  return SUCCESS;
}

bool Configuration::GetBoolValue(const std::string &key, bool default_value) const {
  auto iter = content_map_.find(key);
  if (iter == content_map_.end()) {
    return default_value;
  }

  bool value = false;
  std::istringstream(iter->second) >> std::boolalpha >> value;
  return value;
}

bool Configuration::GetGeContextBoolValue(const std::string &key, bool default_value) const {
  std::string option_value;
  ge::graphStatus status = ge::GetContext().GetOption(key, option_value);
  if (status != ge::GRAPH_SUCCESS) {
    CM_LOGD("Canot get ge option value[%s], return default value[%u].", key.c_str(), default_value);
    return default_value;
  }

  if (option_value.empty()) {
    CM_LOGD("Ge option value[%s] is empty, return default value[%u].", key.c_str(), default_value);
    return default_value;
  }
  CM_LOGI("The option value[%s] in ge context is %s.", key.c_str(), option_value.c_str());

  bool value = false;
  std::istringstream(option_value) >> value;
  return value;
}

Status Configuration::GetGeContextStringValue(const std::string &key, std::string &option_value) const {
  ge::graphStatus status = ge::GetContext().GetOption(key, option_value);
  if (status != ge::GRAPH_SUCCESS) {
    CM_LOGD("Cannot get ge option value [%s].", key.c_str());
    return FAILED;
  }

  if (option_value.empty()) {
    CM_LOGD("The value of ge option[%s] is empty.", key.c_str());
    return SUCCESS;
  }
  CM_LOGI("The option value[%s] in ge context is %s.", key.c_str(), option_value.c_str());

  return SUCCESS;
}

bool Configuration::IsDCorMDCSoc() const {
  return std::find(SOC_VERSION_MDCOrDC_LIST.begin(), SOC_VERSION_MDCOrDC_LIST.end(), soc_version_) !=
         SOC_VERSION_MDCOrDC_LIST.end();
}

bool Configuration::IsDCSoc() const {
  return std::find(SOC_VERSION_DC_LIST.begin(), SOC_VERSION_DC_LIST.end(), soc_version_) != SOC_VERSION_DC_LIST.end();
}

bool Configuration::IsMDCSoc() const {
  return std::find(SOC_VERSION_MDC_LIST.begin(), SOC_VERSION_MDC_LIST.end(), soc_version_) !=
         SOC_VERSION_MDC_LIST.end();
}

bool Configuration::IsCloudSoc() const {
  return std::find(SOC_VERSION_CLOUD_LIST.begin(), SOC_VERSION_CLOUD_LIST.end(), soc_version_) !=
         SOC_VERSION_CLOUD_LIST.end();
}

bool Configuration::IsLhisiSoc() const {
  return std::find(SOC_VERSION_LHISI_LIST.begin(), SOC_VERSION_LHISI_LIST.end(), soc_version_) !=
         SOC_VERSION_LHISI_LIST.end();
}

Status Configuration::InitBufferOptimize() {
  auto ret = InitFromGeContext(BUFFER_OPTIMIZE_MAP, ge::BUFFER_OPTIMIZE, buffer_optimize_);
  buffer_fusion_mode_ = EN_OPTIMIZE_DISABLE;
  InitBufferFusionMode();
  InitAppendArgsMode();
  return ret;
}

void Configuration::InitSmallChannel(const std::string &enable_small_channel) {
  if (enable_small_channel == "1") {
    enable_small_channel_ = true;
  } else if (enable_small_channel == "0") {
    enable_small_channel_ = false;
  }
  return;
}

void Configuration::InitLicenseFusion(std::string &license_fusion_value) {
  std::vector<std::string> temp_str;
  license_fusion_value_ = license_fusion_value;
  temp_str = StringUtils::Split(StringUtils::Trim(license_fusion_value_), ':');
  license_fusion_detail_value_.clear();

  for (size_t i = 0; i < temp_str.size(); ++i) {
    auto iter = LICENSE_PASSNAME_MAP.find(temp_str[i].c_str());
    if (iter != LICENSE_PASSNAME_MAP.end() && !iter->second.empty()) {
      CM_LOGD("InitLicenseFusion %s-%s", iter->first.c_str(), iter->second.c_str());
      license_fusion_detail_value_.insert(iter->second.c_str());
    }
  }
  return;
}

bool Configuration::IsInLicenseControlMap(const std::string &key) const {
  for (auto iter = LICENSE_PASSNAME_MAP.begin();
       iter != LICENSE_PASSNAME_MAP.end(); ++iter) {
    if (iter->second == key) {
      return true;
    }
  }
  return false;
}

Status Configuration::InitPrecisionMode() {
  /* Initialize attribute is mini board first because the deafult precision mode
   * depends on it. */
  std::string precision_mode;
  if (GetGeContextStringValue(ge::PRECISION_MODE, precision_mode) == SUCCESS && !precision_mode.empty()) {
    auto iter = find(PRECISION_MODE_LIST.begin(), PRECISION_MODE_LIST.end(), precision_mode);
    if (iter == PRECISION_MODE_LIST.end()) {
      CM_LOGE(
          "Ge option value[%s] is not correct, it must be one of [allow_mix_precision, allow_mix_precision_fp16,"
          " allow_mix_precision_bf16, force_fp16, force_bf16, force_lowerprecision, allow_fp32_to_fp16,"
          " allow_fp32_to_bf16, allow_fp32_to_lowprecison, must_keep_origin_dtype], please check it.",
          precision_mode.c_str());
      std::map<std::string, std::string> error_key_map;
      error_key_map[EM_VALUE] = precision_mode;
      error_key_map[EM_OPTION] = ge::PRECISION_MODE;
      ReportErrorMessage(EM_INPUT_OPTION_INVALID, error_key_map);
      return FAILED;
    } else {
      precision_mode_ = precision_mode;
      CM_LOGI("Precision mode is configurated as %s", precision_mode_.c_str());
    }
  } else {
    std::map<std::string, std::string> error_key_map;
    error_key_map[EM_VALUE] = "precision mode is empty";
    error_key_map[EM_OPTION] = ge::PRECISION_MODE;
    ReportErrorMessage(EM_INPUT_OPTION_INVALID, error_key_map);
    REPORT_CM_ERROR("[GraphOpt][Init] Precision mode is empty!");
    return FAILED;
  }
  return SUCCESS;
}

void Configuration::InitScopeId() {
  std::vector<string> scope_id = ParseConfig("scope.id", ',');
  if (!scope_id.empty()) {
    if (scope_id[0] == "All") {
      allow_all_scope_id_ = true;
    } else {
      allow_all_scope_id_ = false;
      for (auto i : scope_id) {
        qualified_scope_id_.push_back(atoi(i.c_str()));
      }
    }
  }
}

Status Configuration::GetGraphFilePath(std::string &graph_file_path) {
  Status ret;
  if (engine_name_ == VECTOR_CORE_NAME) {
    ret = GetStringValue(VECTOR_CORE_CONFIG_KEY_GRAPH_FILE, graph_file_path);
  } else if (engine_name_ == kDsaCoreName) {
    ret = GetStringValue(DSA_CORE_CONFIG_KEY_GRAPH_FILE, graph_file_path);
  } else {
    ret = GetStringValue(CONFIG_KEY_GRAPH_FILE, graph_file_path);
  }
  if (ret == SUCCESS) {
    if (graph_file_path.empty()) {
      CM_LOGW("The path of input built-in graph fusion rule json file is empty.");
      return SUCCESS;
    }

    graph_file_path = ascend_ops_path_ + graph_file_path;
  }
  return ret;
}

Status Configuration::GetCustomFilePath(std::string &custom_file_path) {
  Status ret;
  if (engine_name_ == VECTOR_CORE_NAME) {
    ret = GetStringValue(VECTOR_CORE_CONFIG_KEY_CUSTOM_FILE, custom_file_path);
  } else if (engine_name_ == kDsaCoreName) {
    ret = GetStringValue(DSA_CORE_CONFIG_KEY_CUSTOM_FILE, custom_file_path);
  } else {
    ret = GetStringValue(CONFIG_KEY_CUSTOM_FILE, custom_file_path);
  }
  if (ret == SUCCESS) {
    if (custom_file_path.empty()) {
      CM_LOGW("The path of input custom graph fusion rule json file is empty.");
      return SUCCESS;
    }
    custom_file_path = ascend_ops_path_ + custom_file_path;
  }
  return ret;
}

Status Configuration::GetCustomPassFilePath(std::string &custom_pass_file_path) {
  Status ret;
  if (engine_name_ == VECTOR_CORE_NAME) {
    ret = GetStringValue(VECTOR_CORE_CONFIG_KEY_CUSTOM_PASS_FILE, custom_pass_file_path);
  } else if (engine_name_ == kDsaCoreName) {
    ret = GetStringValue(DSA_CORE_CONFIG_KEY_CUSTOM_PASS_FILE, custom_pass_file_path);
  } else {
    ret = GetStringValue(CONFIG_KEY_CUSTOM_PASS_FILE, custom_pass_file_path);
  }
  if (ret == SUCCESS) {
    if (custom_pass_file_path.empty()) {
      CM_LOGW("Input custom graph fusion pass path is empty.");
      return SUCCESS;
    }
    custom_pass_file_path = ascend_ops_path_ + custom_pass_file_path;
  }
  return ret;
}

Status Configuration::GetBuiltinPassFilePath(std::string &builtin_pass_file_path) {
  Status ret;
  if (engine_name_ == VECTOR_CORE_NAME) {
    ret = GetStringValue(VECTOR_CORE_CONFIG_KEY_BUILTIN_PASS_FILE, builtin_pass_file_path);
  } else if (engine_name_ == kDsaCoreName) {
    ret = GetStringValue(DSA_CORE_CONFIG_KEY_BUILTIN_PASS_FILE, builtin_pass_file_path);
  } else {
    ret = GetStringValue(CONFIG_KEY_BUILTIN_PASS_FILE, builtin_pass_file_path);
  }
  if (ret == SUCCESS) {
    if (builtin_pass_file_path.empty()) {
      CM_LOGW("Input built-in graph fusion pass path is empty.");
      return SUCCESS;
    }
    builtin_pass_file_path = ascend_ops_path_ + builtin_pass_file_path;
  }
  return ret;
}

std::string Configuration::GetRootPath() {
  std::string root_path;
  Status ret = GetStringValue(CONFIG_KEY_ROOT, root_path);
  if (ret != SUCCESS) {
    root_path = lib_path_;
  }
  return root_path;
}

bool Configuration::GetAutoMixPrecisionSwitch() const {
  if (!precision_mode_.empty() &&
     (precision_mode_ == ALLOW_MIX_PRECISION || precision_mode_ == ALLOW_MIX_PRECISION_FP16)) {
    return true;
  } else {
    return false;
  }
}

bool Configuration::GetAutoMixPrecisionBF16Switch() const {
  if (!precision_mode_.empty() && precision_mode_ == ALLOW_MIX_PRECISION_BF16) {
    return true;
  } else {
    return false;
  }
}

const std::string &Configuration::GetPrecisionModeStr() { return precision_mode_; }

const std::string &Configuration::GetLicenseFusionStr() { return license_fusion_value_; }

AppendArgsMode Configuration::GetAppendArgsMode() const { return append_args_mode_; }

AutoTuneMode Configuration::GetAutoTuneMode() {
  std::string auto_tune_mode;
  if (auto_tune_mode_ == AutoTuneMode::TUNE_MODE_NO_TUNE) {
    if (GetGeContextStringValue(ge::AUTO_TUNE_MODE, auto_tune_mode) == SUCCESS && !auto_tune_mode.empty()) {
      CM_LOGD("The value of auto tune mode [%s] is %s.", ge::AUTO_TUNE_MODE.c_str(), auto_tune_mode.c_str());
      auto iter_map = AUTO_TUNE_MODE_MAP.find(auto_tune_mode);
      if (iter_map != AUTO_TUNE_MODE_MAP.end()) {
        auto_tune_mode_ = iter_map->second;
      }
    }
  }
  
  return auto_tune_mode_;
}

void Configuration::SetAppendArgsMode(AppendArgsMode args_mode) { append_args_mode_ = args_mode; }

bool Configuration::GetEnableSmallChannel() const { return enable_small_channel_; }

bool Configuration::IsEnableReuseMem() const { return is_enable_reuse_mem_; }

bool Configuration::GetEnableCompressWeight() const { return enable_compress_weight_; }

std::map<int32_t, float> Configuration::GetCompressRatio() const { return compress_ratio_; }

void Configuration::InitISAArchVersion() {
  // init isa_arch_ver
  auto isa_arch_ver_iter = SOC_ISA_ARCH_VERSION_MAP.find(soc_version_);
  if (isa_arch_ver_iter != SOC_ISA_ARCH_VERSION_MAP.end()) {
    isa_arch_ver_ = isa_arch_ver_iter->second;
  }
  CM_LOGD("The value of ISA arch version is %d.", static_cast<int32_t>(isa_arch_ver_));
}

void Configuration::InitAppendArgsMode() {
  // 1910
  if (soc_version_ == SOC_VERSION_ASCEND310) {
    append_args_mode_ = buffer_fusion_mode_ != EN_OPTIMIZE_DISABLE ? AppendArgsMode::L2_BUFFER_ARGS :
                                                                   AppendArgsMode::NO_ARGS;
  }

  CM_LOGD("soc_version=[%s], buffer_fusion_mode=[%s], append_args_mode=[%s].", soc_version_.c_str(),
          GetBufferFusionModeString(buffer_fusion_mode_).c_str(), GetAppendArgsModeString(append_args_mode_).c_str());
}

int32_t Configuration::GetDataVisitDistThreshold() const { return data_visit_dist_threshold_; }

int32_t Configuration::GetMemReuseDistThreshold() const { return mem_reuse_dist_threshold_; }

bool Configuration::CheckSupportCMO() const {
  return (soc_version_== SOC_VERSION_ASCEND320 && use_cmo_ == kStrTrue);
}

void Configuration::InitUseCmo() {
  use_cmo_ = kStrFalse;
  Status status = GetStringValue("cmo.use", use_cmo_);
  if (status != SUCCESS || use_cmo_.empty()) {
    use_cmo_ = kStrFalse;
  }
  CM_LOGD("get cmo use mode:%s.", use_cmo_.c_str());
  return;
}

int32_t Configuration::ParseDataVisitDistThreshold() {
  string threshold;
  Status status = GetStringValue("l2cache.datavisitdist.threshold", threshold);
  if (status != SUCCESS || threshold.empty()) {
    return DATA_VISIT_DIST_THRESHOLD;
  }
  try {
    return stoi(threshold);
  } catch (...) {
    FE_LOGE("Convert %s to int value failed.", threshold.c_str());
    return DATA_VISIT_DIST_THRESHOLD;
  }
}

void Configuration::InitMemReuseDistThreshold() {
  string threshold;
  Status status = GetStringValue("mem.reusedist.threshold", threshold);
  if (status != SUCCESS || threshold.empty()) {
    return;
  }
  try {
    mem_reuse_dist_threshold_ = stoi(threshold);
  } catch (...) {
    FE_LOGW("Convert %s to int value failed.", threshold.c_str());
  }
  CM_LOGD("get mem reuse distance:%d.", mem_reuse_dist_threshold_);
  return;
}

void Configuration::InitCompressRatio() {
  string ratio_str;
  Status status = GetStringValue("multi_core.compress_ratio", ratio_str);
  if (status != SUCCESS || ratio_str.empty()) {
    return;
  }
  std::vector<std::string> multi_core_ratios = StringUtils::Split(ratio_str, '|');
  for (const auto &multi_core_ratio : multi_core_ratios) {
    std::vector<std::string> core_ratio = StringUtils::Split(multi_core_ratio, ':');
    if (core_ratio.size() != 2) { // 2 means no ':' or configure illegal
      continue;
    }
    int32_t core_num;
    try {
      core_num = std::stoi(core_ratio[0]);
    } catch (...) {
      FE_LOGW("Convert %s to int value failed.", core_ratio[0].c_str());
      continue;
    }
    float ratio;
    try {
      ratio = std::stof(core_ratio[1]);
    } catch (...) {
      FE_LOGW("Convert %s to float value failed.", core_ratio[1].c_str());
      continue;
    }
    compress_ratio_[core_num] = ratio;
  }
  return;
}

std::vector<std::string> Configuration::ParseConfig(const std::string &key, char pattern) {
  std::string value_str;
  Status get_status = GetStringValue(key, value_str);
  CM_LOGD("ParseConfig: key=[%s], value=[%s].", key.c_str(), value_str.c_str());
  std::vector<std::string> result;
  if (get_status == SUCCESS) {
    return StringUtils::Split(StringUtils::Trim(value_str), pattern);
  }
  return result;
}

std::string Configuration::GetBuiltInFusionConfigFilePath() const {
  std::string switch_priority_file_path = "plugin/opskernel/fe_config/fusion_config.json";
  auto iter = content_map_.find(FUSION_CONFIG_BUILT_IN_FILE);
  if (iter != content_map_.end()) {
    switch_priority_file_path = iter->second;
  }
  std::string real_file_path = lib_path_ + switch_priority_file_path;
  CM_LOGD("The real path of built-in fusion switch file is %s", real_file_path.c_str());
  return real_file_path;
}

std::string Configuration::GetCustomFusionConfigFilePath() {
  std::string custom_fusion_config_file = custom_fusion_config_file_;
  return custom_fusion_config_file;
}

bool Configuration::EnableL1Fusion() const {
  return buffer_optimize_ == EN_L1_OPTIMIZE;
}

bool Configuration::EnableL2Fusion() {
  return GetBufferFusionMode() == EN_L2_FUSION;
}

void Configuration::InitBufferFusionMode() {
  if (buffer_optimize_ == EN_OFF_OPTIMIZE) {
    return;
  } else if (soc_version_ != SOC_VERSION_ASCEND310 && soc_version_ != SOC_VERSION_ASCEND710 &&
             soc_version_ != SOC_VERSION_ASCEND710P &&
             soc_version_ != SOC_VERSION_ASCEND610 &&
             soc_version_ != SOC_VERSION_ASCEND910 &&
             soc_version_ != SOC_VERSION_ASCEND910A &&
             soc_version_ != SOC_VERSION_ASCEND910B &&
             soc_version_ != SOC_VERSION_ASCEND910PREMIUMA) {
    buffer_fusion_mode_ = EN_L2_BUFFER;
  } else {
    buffer_fusion_mode_ = EN_L2_FUSION;
  }
  CM_LOGD("The value of buffer fusion mode is [%s].", GetBufferFusionModeString(buffer_fusion_mode_).c_str());
}

BufferFusionMode Configuration::GetBufferFusionMode() {
  // In mstune mode, when build_mode = baseline, we need to go l2buffer
  std::string build_mode_value = GetGeContextOptionValue(ge::BUILD_MODE);
  if (build_mode_value == ge::BUILD_MODE_BASELINE) {
    CM_LOGD("The value of buffer fusion mode is [%s].", GetBufferFusionModeString(EN_L2_BUFFER).c_str());
    return EN_L2_FUSION;
  } else if (build_mode_value == ge::BUILD_MODE_TUNING) {
    CM_LOGD("The value of buffer fusion mode is [%s].", GetBufferFusionModeString(EN_L2_FUSION).c_str());
    return EN_L2_FUSION;
  }
  CM_LOGD("The value of buffer fusion mode is [%s].", GetBufferFusionModeString(buffer_fusion_mode_).c_str());
  return buffer_fusion_mode_;
}

bool Configuration::GetDumpOriginalNodesEnable() { return GetBoolValue(CONFIG_KEY_ORIGINAL_NODES_ENABLE, false); }

bool Configuration::GetMixL2Enable() const {
  bool ret = false;
  ret = GetBoolValue(CONFIG_KEY_MIX_L2_ENABLE, false);
  CM_LOGD("Get MiXl2 switch[%d].", ret);
  return ret;
}

bool Configuration::GetDuplicationSwitch() { return GetBoolValue(CONFIG_KEY_MAY_DUPLICATE_IN_AUTO_FUSION, false); }

bool Configuration::IsEnableNetworkAnalysis() const { return enable_network_analysis_; }

std::vector<int64_t> Configuration::GetQualifiedScopeId() { return qualified_scope_id_; }

std::string Configuration::GetGeContextOptionValue(const std::string &key) const {
  std::string option_value;
  ge::graphStatus status = ge::GetContext().GetOption(key, option_value);
  if (status != ge::GRAPH_SUCCESS) {
    CM_LOGD("Cannot get option value [%s].", key.c_str());
  } else {
    CM_LOGD("The option value[%s] in ge context is %s.", key.c_str(), option_value.c_str());
  }
  return option_value;
}

std::string Configuration::GetBuildStep() { return GetGeContextOptionValue(ge::BUILD_STEP); }

std::string Configuration::GetBuildMode() { return GetGeContextOptionValue(ge::BUILD_MODE); }

std::string Configuration::GetFeLibPath() const { return lib_path_ + "plugin/opskernel/"; }

void Configuration::GetModifyMixlist(std::string &modify_mixlist_path) const {
  modify_mixlist_path = modify_mixlist_path_;
}

void Configuration::GetLicenseFusionDetailInfo(std::set<std::string> &license_detail_info) const {
  license_detail_info = license_fusion_detail_value_;
}
}  // namespace fe
