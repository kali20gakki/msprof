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

#include "common/fe_utils.h"
#include <sys/time.h>
#include <atomic>
#include <sstream>
#include <thread>
#include "common/string_utils.h"
#include "common/fe_inner_error_codes.h"
#include "common/math_util.h"
#include "common/util/error_manager/error_manager.h"
#include "common/util/op_info_util.h"
#include "common/util/platform_info.h"
#include "common/configuration.h"
#include "runtime/rt.h"

namespace fe {
uint32_t GetPlatformSCubeVecSplitFlag() {
  string soc_version = Configuration::Instance(AI_CORE_NAME).GetSocVersion();
  PlatformInfo platform_info;
  OptionalInfo opti_compilation_info;
  if (PlatformInfoManager::Instance().GetPlatformInfo(soc_version, platform_info, opti_compilation_info) != SUCCESS) {
    FE_LOGW("Fail to get platform info by soc version[%s].", soc_version.c_str());
    return 0;
  }

  if (platform_info.ai_core_spec.cube_vector_split == 1) {
    return 1;
  }
  return 0;
}

CubeVecState CheckCubeVecState() {
  std::string soc_version = Configuration::Instance(AI_CORE_NAME).GetSocVersion();
  PlatFormInfos platform_infos;
  OptionalInfos optional_infos;
  if (PlatformInfoManager::Instance().GetPlatformInfos(soc_version, platform_infos, optional_infos) != SUCCESS) {
    FE_LOGW("Fail to get platform info by soc version[%s]", soc_version.c_str());
    return CubeVecState::CUBE_VEC_UNKNOWN;
  }
  std::string cube_vec_state_str;
  platform_infos.GetPlatformRes(kSocInfo, kCubVecMix, cube_vec_state_str);
  FE_LOGD("Parameter cub_vector_mix is [%s]", cube_vec_state_str.c_str());
  std::vector<std::string> cube_vec_states = StringUtils::Split(cube_vec_state_str, ',');
  if (std::find(cube_vec_states.begin(), cube_vec_states.end(), kCubeVecMix) != cube_vec_states.end()) {
    return CubeVecState::CUBE_VEC_MIX;
  }
  if (std::find(cube_vec_states.begin(), cube_vec_states.end(), kCubeVecDecouple) != cube_vec_states.end()) {
    return CubeVecState::CUBE_VEC_DECOUPLE;
  }
  return CubeVecState::CUBE_VEC_UNKNOWN;
}

// 1:ffts   2:fftsplus
AICoreMode GetPlatformAICoreMode() {
  string soc_version = Configuration::Instance(AI_CORE_NAME).GetSocVersion();
  PlatFormInfos platform_infos;
  OptionalInfos opti_compilation_infos;
  auto ret = PlatformInfoManager::Instance().GetPlatformInfos(soc_version, platform_infos, opti_compilation_infos);
  if (ret != SUCCESS) {
    FE_LOGW("Fail to get platform info by soc version[%s].", soc_version.c_str());
    return FFTS_MODE_NO_FFTS;
  }
  string ffts_mode = "";
  platform_infos.GetPlatformRes(kSocInfo, kAICoreMode, ffts_mode);
  FE_LOGD("GetPlatformAICoreMode [%s].", ffts_mode.c_str());
  if (ffts_mode == kFFTSPlusMode) {
    return FFTS_MODE_FFTS_PLUS;
  } else if (ffts_mode == kFFTSMode) {
    return FFTS_MODE_FFTS;
  }
  return FFTS_MODE_NO_FFTS;
}

std::mutex g_report_error_msg_mutex;
int64_t GetMicroSecondTime() {
  struct timeval tv = {0};
  int ret = gettimeofday(&tv, nullptr);
  if (ret != 0) {
    return 0;
  }
  if (tv.tv_sec < 0 || tv.tv_usec < 0) {
    return 0;
  }
  int64_t micro_multiples = 1000000;
  int64_t second = tv.tv_sec;
  FE_INT64_MULCHECK(second, micro_multiples);
  int64_t second_to_micro = second * micro_multiples;
  FE_INT64_ADDCHECK(second_to_micro, tv.tv_usec);
  return second_to_micro + tv.tv_usec;
}

uint64_t GetCurThreadId() {
  std::ostringstream oss;
  oss << std::this_thread::get_id();
  std::string s_tid = oss.str();
  try {
    return std::stoull(s_tid);
  } catch (...) {
    FE_LOGW("Thread Id %s invalid.", s_tid.c_str());
    uint64_t invalid_thread_id = 0;
    return invalid_thread_id;
  }
}

uint64_t GetAtomicId() {
  static std::atomic<uint64_t> global_atomic_id(1);
  return global_atomic_id.fetch_add(1, std::memory_order_relaxed);
}

std::string FormatToStr(ge::Format format) { return ge::TypeUtils::FormatToSerialString(format); }

std::string ConstFormatToStr(const ge::Format &format) {
  auto iter = GE_FORMAT_STRING_MAP.find(format);
  if (iter == GE_FORMAT_STRING_MAP.end()) {
    return "unknown-format " + std::to_string(format);
  } else {
    return iter->second;
  }
}

std::string DTypeToStr(ge::DataType d_type) { return ge::TypeUtils::DataTypeToSerialString(d_type); }

std::string GetBoolString(bool &bool_value) {
  auto iter = BOOL_STR_MAP.find(bool_value);
  if (iter == BOOL_STR_MAP.end()) {
    return kStrFalse;
  } else {
    return iter->second;
  }
}

std::string GetCheckSupportedString(te::CheckSupportedResult &check_supported) {
  auto iter = CHECKSUPPORTED_STR_MAP.find(check_supported);
  if (iter == CHECKSUPPORTED_STR_MAP.end()) {
    return STR_NOT_SUPPORTED;
  } else {
    return iter->second;
  }
}

std::string GetImplTypeString(OpImplType op_impl_type) {
  auto iter = IMPL_TYPE_STRING_MAP.find(op_impl_type);
  if (iter == IMPL_TYPE_STRING_MAP.end()) {
    return "unknown-type " + std::to_string(op_impl_type);
  } else {
    return iter->second;
  }
}

std::string GetGeImplTypeString(domi::ImplyType ge_impl_type) {
  auto iter = GE_IMPL_TYPE_STRING_MAP.find(ge_impl_type);
  if (iter == GE_IMPL_TYPE_STRING_MAP.end()) {
    return "unknown-type " + std::to_string(static_cast<int64_t>(ge_impl_type));
  } else {
    return iter->second;
  }
}

bool IsInvalidImplType(const std::string &engine_name, const OpImplType &op_impl_type) {
  if (engine_name == AI_CORE_NAME && op_impl_type == EN_IMPL_HW_DSA) {
    return true;
  }
  if (engine_name == kDsaCoreName && op_impl_type != EN_IMPL_HW_DSA) {
    return true;
  }
  return false;
}

std::string GetPassTypeString(GraphFusionPassType pass_type) {
  auto iter = PASS_TYPE_STRING_MAP.find(pass_type);
  if (iter == PASS_TYPE_STRING_MAP.end()) {
    return "unknown-pass-type " + std::to_string(pass_type);
  } else {
    return iter->second;
  }
}

std::string GetRuleTypeString(RuleType rule_type) {
  auto iter = RULE_TYPE_STRING_MAP.find(rule_type);
  if (iter == RULE_TYPE_STRING_MAP.end()) {
    return "unknown-rule-type " + std::to_string(static_cast<int>(rule_type));
  } else {
    return iter->second;
  }
}

std::string GetBufferFusionPassTypeString(BufferFusionPassType pass_type) {
  auto iter = BUFFER_FUSION_PASS_TYPE_STRING_MAP.find(pass_type);
  if (iter == BUFFER_FUSION_PASS_TYPE_STRING_MAP.end()) {
    return "unknown-buffer-fusion-pass-type " + std::to_string(pass_type);
  } else {
    return iter->second;
  }
}

int64_t GetGreatestCommonDivisor(int64_t x, int64_t y) {
  if (y == 0) {
    return x;
  }
  return GetGreatestCommonDivisor(y, x % y);
}

int64_t GetLeastCommonMultiple(int64_t x, int64_t y) {
  if (x == 0 || y == 0) {
    return 0;
  }
  FE_INT64_MULCHECK(x, y);
  return (x * y) / GetGreatestCommonDivisor(x, y);
}

bool CheckFilePath(std::string file_path, std::string delimiter) {
  if (file_path.empty()) {
    REPORT_FE_ERROR("[GraphOpt][Init][CheckFilePath] File path: %s is none, and it's error.", file_path.c_str());
    return false;
  }

  if (file_path.find(delimiter) == std::string::npos) {
    FE_LOGD("File path: %s not contains %s", file_path.c_str(), delimiter.c_str());
    file_path = "./" + file_path;
  }

  for (char ch_id : file_path) {
    if (delimiter.find(ch_id) != std::string::npos) {
      continue;
    }
    if (!((ch_id >= '0' && ch_id <= '9') || (ch_id >= 'a' && ch_id <= 'z') || (ch_id >= 'A' && ch_id <= 'Z') ||
          (ch_id == '_') || (ch_id == '-') || ch_id == '.')) {
      REPORT_FE_ERROR(
          "[GraphOpt][Init][CheckFilePath] Check parameter[%s] error, it's char should be '0'~'9' or"
          " 'a'~'z' or 'A'~'Z' or '_' or '-'.",
          file_path.c_str());
      return false;
    }
  }

  return true;
}

bool CheckFileEmpty(const std::string &file_name) {
  std::ifstream ifs(file_name);
  if (!ifs.is_open()) {
    REPORT_FE_ERROR("[GraphOpt][Init][CheckFileEmpty] The file [%s] does not exist or has been opened.",
                    file_name.c_str());
    return FILE_NOT_EXIST;
  }
  char c;
  ifs >> c;
  if (ifs.eof()) {
    ifs.close();
    return true;
  }
  ifs.close();
  return false;
}

void LogErrorMessage(std::string error_code, const std::map<std::string, std::string> &args_map) {
  int result = ErrorManager::GetInstance().ReportErrMessage(error_code, args_map);

  FE_LOGE_IF(result != 0, "Faild to call ErrorManager::GetInstance(). ReportErrMessage");
}

void SaveErrorMessage(const std::string &graph_name, const std::map<std::string, std::string> &args_map) {
  std::lock_guard<std::mutex> lock_guard(g_report_error_msg_mutex);
  int result = ErrorManager::GetInstance().ReportMstuneCompileFailedMsg(graph_name, args_map);

  FE_LOGE_IF(result != 0, "Faild to call ErrorManager::GetInstance(). SaveErrMessage");
}
}  // namespace fe
