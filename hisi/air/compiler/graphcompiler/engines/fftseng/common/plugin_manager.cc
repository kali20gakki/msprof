/**
 * Copyright 2022-2023 Huawei Technologies Co., Ltd
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

#include "plugin_manager.h"
#include <fstream>
#include <iostream>
#include <memory>
#include "inc/ffts_utils.h"

namespace ffts {
Status PluginManager::CloseHandle() {
  FFTS_CHECK(handle == nullptr, FFTS_LOGW("Handle is nullptr."), return FAILED);
  if (dlclose(handle) != 0) {
    FFTS_LOGW("Failed to close handle of %s.", so_name.c_str());
    return FAILED;
  }

  return SUCCESS;
}

Status PluginManager::OpenPlugin(const string& path) {
  FFTS_LOGD("Start to load so file[%s].", path.c_str());

  std::string real_path = RealPath(path);
  if (real_path.empty()) {
    FFTS_LOGW("The file path [%s] is not valid", path.c_str());
    return FAILED;
  }

  // return when dlopen is failed
  handle = dlopen(real_path.c_str(), RTLD_NOW | RTLD_GLOBAL);
  FFTS_CHECK(handle == nullptr,
             REPORT_FFTS_ERROR("[FFTS Init][OpPluginSo] Fail to load so file %s, error message is %s.",
             real_path.c_str(), dlerror()), return FAILED);

  FFTS_LOGD("Finish loading so file[%s].", path.c_str());
  return SUCCESS;
}

/*
*  @ingroup ffts
*  @brief   return tbe so name
*  @return  so name
*/
const string PluginManager::GetSoName() const { return so_name; }

}  // namespace fe
