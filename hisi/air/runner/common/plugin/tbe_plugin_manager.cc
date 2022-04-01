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

#include "common/plugin/tbe_plugin_manager.h"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <string>

#include "common/plugin/ge_util.h"
#include "framework/common/debug/log.h"
#include "framework/common/debug/ge_log.h"
#include "framework/common/util.h"
#include "framework/common/ge_inner_error_codes.h"
#include "framework/engine/dnnengine.h"
#include "framework/omg/omg_inner_types.h"
#include "external/ge/ge_api_types.h"
#include "register/op_registry.h"
#include "graph/opsproto_manager.h"
#include "graph/utils/type_utils.h"
#include "mmpa/mmpa_api.h"

namespace {
const size_t kMaxErrBufStrLen = 128U;
const uint32_t kMaxPointStrLen = 3U;
}  //  namespace

namespace ge {
const int32_t kBaseIntValue = 10;

std::map<std::string, std::string> TBEPluginManager::options_ = {};

// Get Singleton Instance
TBEPluginManager &TBEPluginManager::Instance() {
  static TBEPluginManager instance_ptr_;
  return instance_ptr_;
}

Status TBEPluginManager::ClearHandles_() {
  Status ret = SUCCESS;
  for (const auto &handle : handles_vec_) {
    if (mmDlclose(handle) != 0) {
      ret = FAILED;
      const char_t *error = mmDlerror();
      GE_IF_BOOL_EXEC(error == nullptr, error = "");
      GELOGW("Failed to close handle: %s", error);
    }
  }
  handles_vec_.clear();
  return ret;
}

Status TBEPluginManager::Finalize() {
  return ClearHandles_();
}

std::string TBEPluginManager::GetPath() {
  mmDlInfo dl_info;
  if (mmDladdr(reinterpret_cast<void *>(&TBEPluginManager::GetPath), &dl_info) != EN_OK) {
    const char_t *error = mmDlerror();
    GE_IF_BOOL_EXEC(error == nullptr, error = "");
    GELOGW("Failed to read so path! errmsg:%s", error);
    return std::string();
  } else {
    std::string so_path = dl_info.dli_fname;
    char_t path[MMPA_MAX_PATH] = {};
    if (so_path.length() >= static_cast<size_t>(MMPA_MAX_PATH)) {
      GELOGW("File path is too long!");
      return std::string();
    }
    if (mmRealPath(so_path.c_str(), &path[0], MMPA_MAX_PATH) != EN_OK) {
      char_t err_buf[kMaxErrBufStrLen + 1U] = {};
      const auto err_msg = mmGetErrorFormatMessage(mmGetErrorCode(), &err_buf[0], kMaxErrBufStrLen);
      GELOGW("Failed to get realpath of %s, errmsg:%s", so_path.c_str(), err_msg);
      return std::string();
    }

    so_path = &path[0];
    so_path = so_path.substr(0U, so_path.rfind('/') + 1U);
    return so_path;
  }
}

void TBEPluginManager::ProcessSoFullName(std::vector<std::string> &file_list,
                                         std::string &caffe_parser_path,
                                         const std::string &full_name,
                                         const std::string &caffe_parser_so_suff) {
  if ((full_name.size() >= caffe_parser_so_suff.size()) &&
      (full_name.compare(full_name.size() - caffe_parser_so_suff.size(), caffe_parser_so_suff.size(),
                         caffe_parser_so_suff) == 0)) {
    caffe_parser_path = full_name;
  } else {
    // Save parser so path into file_list vector
    file_list.push_back(full_name);
  }
}

void TBEPluginManager::FindParserUsedSo(const std::string &path, std::vector<std::string> &file_list,
                                        std::string &caffe_parser_path, const uint32_t recursive_depth) {
  static const uint32_t max_recursive_depth = 20U; // For recursive depth protection

  if (recursive_depth >= max_recursive_depth) {
    GELOGW("Recursive depth is become %u, Please check input!", recursive_depth);
    return;
  }

  // Path, change to absolute path
  const std::string real_path = RealPath(path.c_str());
  // Plugin path does not exist
  if (real_path.empty()) {
    GELOGW("RealPath is empty.");
    return;
  }
  const INT32 is_dir = mmIsDir(real_path.c_str());
  // Lib plugin path not exist
  if (is_dir != EN_OK) {
      char_t err_buf[kMaxErrBufStrLen + 1U] = {};
      const auto err_msg = mmGetErrorFormatMessage(mmGetErrorCode(), &err_buf[0], kMaxErrBufStrLen);
      GELOGW("%s is not a dir. errmsg:%s", real_path.c_str(), err_msg);
      return;
  }

  mmDirent **entries = nullptr;
  const auto ret = mmScandir(real_path.c_str(), &entries, nullptr, nullptr);
  if (ret < EN_OK) {
      char_t err_buf[kMaxErrBufStrLen + 1U] = {};
      const auto err_msg = mmGetErrorFormatMessage(mmGetErrorCode(), &err_buf[0], kMaxErrBufStrLen);
      GELOGW("scan dir failed. path = %s, ret = %d, errmsg = %s", real_path.c_str(), ret, err_msg);
      return;
  }
  for (int32_t i = 0; i < ret; ++i) {
      const mmDirent * const dent = entries[i];
      if ((strncmp(dent->d_name, ".", kMaxPointStrLen) == 0) ||
          (strncmp(dent->d_name, "..", kMaxPointStrLen) == 0)) { continue; }
      const std::string name = dent->d_name;
      const std::string full_name = real_path + "/" + name;
      const std::string so_suff = ".so";
      const std::string caffe_parser_so_suff = "lib_caffe_parser.so";
      if ((name.size() >= so_suff.size()) &&
          (name.compare(name.size() - so_suff.size(), so_suff.size(), so_suff) == 0)) {
          ProcessSoFullName(file_list, caffe_parser_path, full_name, caffe_parser_so_suff);
      } else {
          FindParserUsedSo(full_name, file_list, caffe_parser_path, recursive_depth + 1U);
      }
  }
  mmScandirFree(entries, ret);
}

void TBEPluginManager::GetOpPluginSoFileList(const std::string &path,
                                             std::vector<std::string> &file_list,
                                             std::string &caffe_parser_path) {
  // Support to split multiple so directories by ":"
  const std::vector<std::string> v_path = StringUtils::Split(path, ':');
  for (size_t i = 0U; i < v_path.size(); ++i) {
    FindParserUsedSo(v_path[i], file_list, caffe_parser_path);
    GELOGI("CustomOpLib full name = %s", v_path[i].c_str());
  }
}

void TBEPluginManager::GetCustomOpPath(std::string &customop_path) {
  GELOGI("Enter get custom op path schedule");
  std::string fmk_type;
  domi::FrameworkType type = domi::TENSORFLOW;
  const auto it = options_.find(FRAMEWORK_TYPE);
  if (it != options_.end()) {
    type = static_cast<domi::FrameworkType>(std::strtol(it->second.c_str(), nullptr, kBaseIntValue /* base int */));
  }
  fmk_type = ge::TypeUtils::FmkTypeToSerialString(type);
  GELOGI("Framework type is %s.", fmk_type.c_str());

  char_t path_env[MMPA_MAX_PATH] = {};
  const INT32 res = mmGetEnv("ASCEND_OPP_PATH", &path_env[0], static_cast<uint32_t>(MMPA_MAX_PATH));
  if (res == EN_OK) {
    const std::string path = path_env;
    customop_path = (path + "/framework/custom" + "/:") + (path + "/framework/built-in/" + fmk_type);
    GELOGI("Get custom so path from env : %s", &path_env[0]);
    return;
  }
  std::string path_base = GetPath();
  GELOGI("path_base is %s", path_base.c_str());
  path_base = path_base.substr(0U, path_base.rfind('/'));
  path_base = path_base.substr(0U, path_base.rfind('/') + 1U);
  customop_path = (path_base + "ops/framework/custom" + "/:") + (path_base + "ops/framework/built-in/" + fmk_type);
  return;
}

void TBEPluginManager::LoadCustomOpLib() {
  LoadPluginSo(options_);

  std::string fmk_type = std::to_string(domi::TENSORFLOW);
  const auto it = options_.find(ge::FRAMEWORK_TYPE);
  if (it != options_.end()) {
   fmk_type = it->second;
  }
  const std::vector<OpRegistrationData> registration_datas = domi::OpRegistry::Instance()->registrationDatas;
  GELOGI("The size of registration_datas is: %zu", registration_datas.size());
  for (const OpRegistrationData &reg_data : registration_datas) {
    if (std::to_string(reg_data.GetFrameworkType()) == fmk_type) {
      ge::AscendString om_op_type;
      (void) reg_data.GetOmOptype(om_op_type);
      GELOGD("Begin to register optype: %s, imply_type: %s", om_op_type.GetString(),
             TypeUtils::ImplyTypeToSerialString(reg_data.GetImplyType()).c_str());
      (void)domi::OpRegistry::Instance()->Register(reg_data);
    }
  }
}

void TBEPluginManager::LoadPluginSo(const std::map<std::string, std::string> &options) {
  std::vector<std::string> file_list;
  std::string caffe_parser_path;
  std::string plugin_path;

  options_ = options;
  GetCustomOpPath(plugin_path);

  // Whether there are files in the plugin so path
  GetOpPluginSoFileList(plugin_path, file_list, caffe_parser_path);

  //  No file
  if (file_list.empty()) {
    // Print log
    GELOGW("Can not find any plugin file in plugin_path: %s", plugin_path.c_str());
  }

  GELOGW("The shared library will not be checked. Please ensure that the source of the shared library is trusted.");

  // Load other so files except lib_caffe_parser.so in the plugin so path
  for (auto elem : file_list) {
    (void) StringUtils::Trim(elem);

    void * const handle = mmDlopen(elem.c_str(), static_cast<int32_t>(static_cast<uint32_t>(MMPA_RTLD_NOW) |
                                                                      static_cast<uint32_t>(MMPA_RTLD_GLOBAL) |
                                                                      static_cast<uint32_t>(MMPA_RTLD_NODELETE)));
    if (handle == nullptr) {
      const char_t *error = mmDlerror();
      GE_IF_BOOL_EXEC(error == nullptr, error = "");
      GELOGW("dlopen failed, plugin name:%s. Message(%s).", elem.c_str(), error);
    } else if (find(handles_vec_.begin(), handles_vec_.end(), handle) == handles_vec_.end()) {
      // Close dl when the program exist, not close here
      GELOGI("Plugin load %s success.", elem.c_str());
      handles_vec_.push_back(handle);
    } else {
      GELOGI("Plugin so has already been loaded, no need to load again.");
    }
  }
}

void TBEPluginManager::InitPreparation(const std::map<std::string, std::string> &options) {
  options_.insert(options.begin(), options.end());
  // Load TBE plugin
  TBEPluginManager::Instance().LoadCustomOpLib();
}
}  // namespace ge
