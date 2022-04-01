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

#include "common/plugin/op_tiling_manager.h"

#include <string>

#include "mmpa/mmpa_api.h"
#include "common/util/error_manager/error_manager.h"
#include "framework/common/debug/log.h"

namespace ge {
namespace {
const char_t *const kEnvName = "ASCEND_OPP_PATH";
const std::string kDefaultPath = "/usr/local/Ascend/opp";
const std::string kDefaultBuiltInTilingPath = "/op_impl/built-in/ai_core/tbe/op_tiling/liboptiling.so";
const std::string kDefaultCustomTilingPath = "/op_impl/custom/ai_core/tbe/op_tiling/liboptiling.so";
const uint8_t kPrefixIndex = 9U;
}  // namespace

void OpTilingManager::ClearHandles() noexcept {
  for (const auto &handle : handles_) {
    if (mmDlclose(handle.second) != 0) {
      const char_t *error = mmDlerror();
      GE_IF_BOOL_EXEC(error == nullptr, error = "");
      GELOGE(FAILED, "[Close][Handle]Failed, handle of %s: %s", handle.first.c_str(), error);
      REPORT_CALL_ERROR("E19999", "Failed to close handle of %s: %s",
                        handle.first.c_str(), error);
    }
  }
  handles_.clear();
}

OpTilingManager::~OpTilingManager() { ClearHandles(); }

std::string OpTilingManager::GetPath() const {
  char_t opp_path_env[MMPA_MAX_PATH]{};
  const INT32 res = mmGetEnv(kEnvName, &opp_path_env[0], static_cast<uint32_t>(MMPA_MAX_PATH));
  std::string opp_path = kDefaultPath;
  if (res == EN_OK) {
    char_t resolved_path[MMPA_MAX_PATH]{};
    if (mmRealPath(&opp_path_env[0], &resolved_path[0], MMPA_MAX_PATH) != EN_OK) {
      const size_t max_error_str_len = 128U;
      char_t err_buf[max_error_str_len + 1U]{};
      const auto err_msg = mmGetErrorFormatMessage(mmGetErrorCode(), &err_buf[0], max_error_str_len);
      ErrorManager::GetInstance().ATCReportErrMessage(
          "E19024", {"env", "value", "situation"}, {"ASCEND_OPP_PATH", opp_path_env, "loading the tiling lib"});
      GELOGE(PARAM_INVALID, "[Load][TilingLib]Failed, as env 'ASCEND_OPP_PATH'[%s] "
             "is invalid path. errmsg:%s", &opp_path_env[0], err_msg);
      return std::string();
    }
    opp_path = &resolved_path[0];
  }
  return opp_path;
}

void OpTilingManager::LoadSo() {
  const std::string opp_path = GetPath();
  if (opp_path.empty()) {
    GELOGW("Skip load tiling lib.");
    return;
  }
  const std::string built_in_tiling_lib = opp_path + kDefaultBuiltInTilingPath;
  const std::string custom_tiling_lib = opp_path + kDefaultCustomTilingPath;
  const std::string built_in_name = kDefaultBuiltInTilingPath.substr(kPrefixIndex);
  const std::string custom_name = kDefaultCustomTilingPath.substr(kPrefixIndex);

  void * const handle_bi = mmDlopen(built_in_tiling_lib.c_str(),
                                    static_cast<int32_t>(static_cast<uint32_t>(MMPA_RTLD_NOW) |
                                    static_cast<uint32_t>(MMPA_RTLD_GLOBAL)));
  if (handle_bi == nullptr) {
    const char_t *error = mmDlerror();
    GE_IF_BOOL_EXEC(error == nullptr, error = "");
    GELOGW("Failed to dlopen %s! errmsg:%s", built_in_tiling_lib.c_str(), error);
  } else {
    handles_[built_in_name] = handle_bi;
  }

  void * const handle_ct = mmDlopen(custom_tiling_lib.c_str(),
                                    static_cast<int32_t>(static_cast<uint32_t>(MMPA_RTLD_NOW) |
                                    static_cast<uint32_t>(MMPA_RTLD_GLOBAL)));
  if (handle_ct == nullptr) {
    const char_t *error = mmDlerror();
    GE_IF_BOOL_EXEC(error == nullptr, error = "");
    GELOGW("Failed to dlopen %s! errmsg:%s", custom_tiling_lib.c_str(), error);
  } else {
    handles_[custom_name] = handle_ct;
  }
}

OpTilingManager &OpTilingManager::GetInstance() {
  static OpTilingManager instance;
  return instance;
}
}  // namespace ge
