/**
 * Copyright 2020 Huawei Technologies Co., Ltd
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
#include "base_engine.h"

#include "common/util/error_manager/error_manager.h"
#include "config/config_file.h"
#include "error_code/error_code.h"
#include "util/log.h"
#include "util/util.h"
#include "util/constant.h"

using namespace std;
using namespace ge;

namespace {
const string kInitConfigFileRelativePath = "config/init.conf";
}

namespace aicpu {
Status BaseEngine::LoadConfigFile(const void *instance) const {
  string real_config_file_path = GetSoPath(instance);
  real_config_file_path += kInitConfigFileRelativePath;
  AICPUE_LOGI("Configuration file path is %s.", real_config_file_path.c_str());

  aicpu::State ret = ConfigFile::GetInstance().LoadFile(real_config_file_path);
  if (ret.state != ge::SUCCESS) {
    map<string, string> err_map;
    err_map["filename"] = real_config_file_path;
    err_map["reason"] = real_config_file_path;
    string report_err_code_str = GetViewErrorCodeStr(LOAD_INIT_CONFIG_FILE_FAILED);
    ErrorManager::GetInstance().ReportErrMessage(report_err_code_str, err_map);
    AICPUE_LOGE("Configuration file load failed, %s.", ret.msg.c_str());
    return LOAD_INIT_CONFIGURE_FILE_FAILED;
  }
  return SUCCESS;
}
}  // namespace aicpu