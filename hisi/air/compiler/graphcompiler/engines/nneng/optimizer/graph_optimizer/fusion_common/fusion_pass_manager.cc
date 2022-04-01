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

#include "graph_optimizer/fusion_common/fusion_pass_manager.h"
#include <dirent.h>
#include <memory>
#include <regex>
#include <string>
#include "common/configuration.h"
#include "common/fe_log.h"
#include "common/fe_utils.h"
#include "common/pass_manager.h"
#include "common/util/constants.h"
#include "common/util/json_util.h"

namespace fe {
FusionPassManager::FusionPassManager() : init_flag_(false) {}

FusionPassManager::~FusionPassManager() {}

Status FusionPassManager::Initialize(std::string engine_name) {
  if (init_flag_) {
    FE_LOGW("FusionPassManager has been initialized.");
    return SUCCESS;
  }
  this->engine_name_ = engine_name;
  Status ret = InitFusionPassPlugin(engine_name);
  if (ret != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][Init] Init passes failed.");
    CloseFusionPassPlugin();
    return ret;
  }

  ret = InitLxFusionPlugin(engine_name);
  if (ret != SUCCESS) {
    CloseLxFusionPlugin();
  }
  FE_LOGI("Fusion pass manager initialize success.");
  init_flag_ = true;
  return SUCCESS;
}

Status FusionPassManager::Finalize() {
  if (!init_flag_) {
    FE_LOGW("FusionPassManager has not been initialized.");
    return SUCCESS;
  }

  CloseFusionPassPlugin();
  CloseLxFusionPlugin();
  init_flag_ = false;
  FE_LOGD("FusionPassManager finalize success.");
  return SUCCESS;
}

void FusionPassManager::CloseFusionPassPlugin() {
  if (fusion_pass_plugin_manager_vec_.empty()) {
    return;
  }
  for (PluginManagerPtr &plugin_manager_ptr : fusion_pass_plugin_manager_vec_) {
    if (plugin_manager_ptr == nullptr) {
      continue;
    }
    if (plugin_manager_ptr->CloseHandle() != SUCCESS) {
      FE_LOGW("Failed to close so handle[%s].", plugin_manager_ptr->GetSoName().c_str());
    }
  }
  fusion_pass_plugin_manager_vec_.clear();
}

void FusionPassManager::CloseLxFusionPlugin() {
  if (lx_fusion_plugin_manager_ == nullptr) {
    return;
  }
  if (lx_fusion_plugin_manager_->CloseHandle() != SUCCESS) {
    FE_LOGW("Failed to close the handle of the plugin [%s].", lx_fusion_plugin_manager_->GetSoName().c_str());
  }
}

Status FusionPassManager::InitFusionPassPlugin(const std::string &engine_name) {
  std::string custom_pass_file_path;
  std::string builtin_pass_file_path;
  std::vector<string> custom_pass_file;
  std::vector<string> builtin_pass_file;
  // Get pass directory from configuration
  if (Configuration::Instance(engine_name).GetCustomPassFilePath(custom_pass_file_path) == SUCCESS) {
    if (custom_pass_file_path.empty()) {
      FE_LOGI("Custom pass file path is null.");
    } else {
      Status ret = OpenPassPath(custom_pass_file_path, custom_pass_file);
      if (ret != SUCCESS) {
        FE_LOGI("Load custom pass file path %s not success.", custom_pass_file_path.c_str());
      }
    }
  }
  if (Configuration::Instance(engine_name).GetBuiltinPassFilePath(builtin_pass_file_path) == SUCCESS) {
    if (builtin_pass_file_path.empty()) {
      FE_LOGI("Built-in pass file path is null.");
    } else {
      Status ret = OpenPassPath(builtin_pass_file_path, builtin_pass_file);
      if (ret != SUCCESS) {
        FE_LOGW("Load built-in pass file path %s not success.", builtin_pass_file_path.c_str());
        return SUCCESS;
      }
    }
  }
  return SUCCESS;
}

Status FusionPassManager::OpenPassPath(const string &file_path, vector<string> &get_pass_files) {
  FE_LOGD("Start to load fusion pass files in path %s.", file_path.c_str());

  std::string real_path = RealPath(file_path);
  if (real_path.empty()) {
    FE_LOGI("File path[%s] is not valid.", file_path.c_str());
    return FAILED;
  }
  DIR *dir;
  struct dirent *dirp = nullptr;
  char *file_suffix = const_cast<char *>(".so");

  dir = opendir(real_path.c_str());
  FE_CHECK(dir == nullptr,
           REPORT_FE_ERROR("[GraphOpt][Init][OpenPassPath] Fail to open directory %s.", real_path.c_str()),
           return FAILED);
  while ((dirp = readdir(dir)) != nullptr) {
    if (dirp->d_name[0] == '.') {
      continue;
    }
    if (strlen(dirp->d_name) <= strlen(file_suffix)) {
      continue;
    }
    if (strcmp(&(dirp->d_name)[strlen(dirp->d_name) - strlen(file_suffix)], file_suffix) == 0) {
      get_pass_files.push_back(real_path + "/" + dirp->d_name);
    }
  }
  closedir(dir);

  if (get_pass_files.empty()) {
    FE_LOGI("No fusion pass so files get read in path %s", real_path.c_str());
    return SUCCESS;
  }

  for (const auto &pass_file : get_pass_files) {
    FE_LOGD("Begin to open pass so file[%s].", pass_file.c_str());
    PluginManagerPtr plugin_manager_ptr = nullptr;
    FE_MAKE_SHARED(plugin_manager_ptr = std::make_shared<PluginManager>(pass_file), return FAILED);
    FE_CHECK(plugin_manager_ptr == nullptr,
             REPORT_FE_ERROR("[GraphOpt][Init][OpenPassPath] Fail to create PluginManagerPtr."), return FAILED);
    if (plugin_manager_ptr->OpenPlugin(pass_file) != SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][Init][OpenPassPath] Failed to open the fusion pass so [%s].", pass_file.c_str());
      return FAILED;
    }
    fusion_pass_plugin_manager_vec_.push_back(plugin_manager_ptr);
    FE_LOGI("Load the fusion pass so [%s] successfully.", pass_file.c_str());
  }

  return SUCCESS;
}

Status FusionPassManager::InitLxFusionPlugin(const std::string &engine_name) {
  // 1. open the lx fusion plugin
  string plugin_path = Configuration::Instance(engine_name).GetFeLibPath() + LX_FUSION_PLUGIN;
  FE_MAKE_SHARED(lx_fusion_plugin_manager_ = std::make_shared<PluginManager>(plugin_path), return FAILED);
  FE_CHECK(lx_fusion_plugin_manager_ == nullptr, REPORT_FE_ERROR("[GraphOpt][Init][InitLxFusPlg] Create lx fusion \
           plugin manager ptr failed."),
           return FAILED);
  if (lx_fusion_plugin_manager_->OpenPlugin(plugin_path) != SUCCESS) {
    FE_LOGW("Failed to open %s.", plugin_path.c_str());
    return FAILED;
  }

  // 2. get the functions of l1 fusion
  Status ret = lx_fusion_plugin_manager_
                   ->GetFunction<tune::Status, ge::ComputeGraph &, GraphCommPtr,
                   ScopeAllocatorPtr, const string &, AOEOption>(
      L1_FUSION_OPTIMIZER_FUNC_NAME, L1FusionOptimizer);
  if (ret != SUCCESS) {
    FE_LOGW("Failed to get the function %s in the plugin %s.", L1_FUSION_OPTIMIZER_FUNC_NAME.c_str(),
            plugin_path.c_str());
    return FAILED;
  }

  ret = lx_fusion_plugin_manager_->GetFunction<tune::Status, ge::ComputeGraph &, const std::vector<ge::NodePtr> &,
                                               std::vector<ge::NodePtr> *, std::vector<ge::NodePtr> *>(
      L1_RECOVERY_FUNC_NAME, L1Recovery);
  if (ret != SUCCESS) {
    FE_LOGW("Failed to get the function %s in the plugin %s.", L1_RECOVERY_FUNC_NAME.c_str(), plugin_path.c_str());
    return FAILED;
  }

  // 3. get the functions of l2 fusion
  ret = lx_fusion_plugin_manager_
            ->GetFunction<tune::Status, ge::ComputeGraph &, GraphCommPtr, ScopeAllocatorPtr, const string &, AOEOption>(
      L2_FUSION_OPTIMIZER_FUNC_NAME, L2FusionOptimizer);
  if (ret != SUCCESS) {
    FE_LOGW("Failed to get the function %s in the plugin %s.", L2_FUSION_OPTIMIZER_FUNC_NAME.c_str(),
            plugin_path.c_str());
    return FAILED;
  }

  ret = lx_fusion_plugin_manager_->GetFunction<tune::Status, ge::ComputeGraph &, const std::vector<ge::NodePtr> &,
                                               std::vector<ge::NodePtr> *, std::vector<ge::NodePtr> *>(
      L2_RECOVERY_FUNC_NAME, L2Recovery);
  if (ret != SUCCESS) {
    FE_LOGW("Failed to get the function %s in the plugin %s.", L2_FUSION_OPTIMIZER_FUNC_NAME.c_str(),
            plugin_path.c_str());
    return FAILED;
  }

  ret = lx_fusion_plugin_manager_->GetFunction<tune::Status, const ge::ComputeGraph &>(
      lx_fusion_finalize_func_name_, lx_fusion_finalize_);
  if (ret != SUCCESS) {
    FE_LOGW("Failed to get the function %s in the plugin %s.", lx_fusion_finalize_func_name_.c_str(),
            plugin_path.c_str());
    return FAILED;
  }

  return SUCCESS;
}
}  // namespace fe
