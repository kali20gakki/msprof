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

#include "ops_kernel_executor_manager.h"
#include "common/debug/log.h"
#include "common/plugin/ge_util.h"

namespace ge {
namespace {
const char_t *const kExecutorPluginFuncInitialize = "Initialize";
const char_t *const kExecutorPluginFuncGetExecutors = "GetOpsKernelInfoStores";
const char_t *const kExecutorPluginFuncFinalize = "Finalize";
const std::set<std::string> kHcclKernelInfoStoreNames = {"ops_kernel_info_hccl",
                                                         "ops_kernel_info_hccl_gradtune",
                                                         "hvd_ops_kernel_info"};
}  // namespace

std::string OpsKernelExecutorManager::GetExecutorPluginLibPaths() {
  // other executor plugins are not supported yet
  return "";
}

Status OpsKernelExecutorManager::Initialize(const std::map<std::string, std::string> &options) {
  options_ = options;
  const std::string &lib_paths = GetExecutorPluginLibPaths();
  if (lib_paths.empty()) {
    GELOGI("No library to load");
    return SUCCESS;
  }

  GE_CHK_STATUS_RET(InitializePlugin(executor_plugin_, lib_paths), "Failed to initialize executor plugins");
  return SUCCESS;
}

Status OpsKernelExecutorManager::GetExecutor(const std::string &name, OpsKernelExecutor *&executor) {
  const std::lock_guard<std::mutex> lk(mu_);
  // ensure hccl executor plugin was initialized when needed
  if ((kHcclKernelInfoStoreNames.count(name) > 0U) && (hccl_executor_plugin_ == nullptr)) {
    auto hccl_executor_plugin = MakeUnique<PluginManager>();
    GE_CHECK_NOTNULL(hccl_executor_plugin);
    GE_CHK_STATUS_RET(InitializePlugin(*hccl_executor_plugin, GetHcclExecutorPluginLibPath()),
                      "Failed to initialize hccl executor plugins");
    hccl_executor_plugin_ = std::move(hccl_executor_plugin);
  }

  const std::map<std::string, OpsKernelExecutorPtr>::const_iterator it = executors_.find(name);
  if (it == executors_.cend()) {
    GELOGE(FAILED, "Failed to get executor, name = %s", name.c_str());
    return FAILED;
  }
  executor = it->second.get();
  return SUCCESS;
}

void OpsKernelExecutorManager::Finalize() {
  GELOGI("ge invoke ops kernel executor finalize.");
  (void) executor_plugin_.InvokeAll<Status>(kExecutorPluginFuncFinalize);
  if (hccl_executor_plugin_ != nullptr) {
    (void) hccl_executor_plugin_->InvokeAll<Status>(kExecutorPluginFuncFinalize);
  }
}

std::string OpsKernelExecutorManager::GetHcclExecutorPluginLibPath() {
  std::string path_base = GetModelPath();
  return path_base + "libhcom_executor.so";
}

Status OpsKernelExecutorManager::InitializePlugin(PluginManager &plugin_manager, const std::string &plugin_paths) {
  const std::vector<std::string> func_check_list =
      {kExecutorPluginFuncInitialize, kExecutorPluginFuncGetExecutors, kExecutorPluginFuncFinalize};
  GE_CHK_STATUS_RET(plugin_manager.LoadSo(plugin_paths, func_check_list),
                    "[Check][SoFile] not find any valid so file.");
  if (plugin_manager.InvokeAll<std::map<std::string, std::string> &, Status>(kExecutorPluginFuncInitialize, options_)
      != SUCCESS) {
    GELOGE(GE_OPS_GET_NO_VALID_SO, "[Invoke][OpsKernelInfo]PluginManager InvokeAll failed.");
    REPORT_INNER_ERROR("E19999", "PluginManager InvokeAll failed.");
    return GE_OPS_GET_NO_VALID_SO;
  }

  std::map<std::string, OpsKernelExecutorPtr> new_executors;
  const auto ret = plugin_manager.InvokeAll<std::map<std::string, std::shared_ptr<OpsKernelExecutor>> &>(
      kExecutorPluginFuncGetExecutors, new_executors);
  GE_CHK_STATUS_RET(ret, "Failed to get OpsKernelExecutors");
  GE_CHK_STATUS_RET_NOLOG(CheckExecutors(new_executors));

  executors_.insert(new_executors.cbegin(), new_executors.cend());
  return SUCCESS;
}

Status OpsKernelExecutorManager::CheckExecutors(const std::map<std::string, OpsKernelExecutorPtr> &executors) {
  for (const auto &it : executors) {
    if (it.second == nullptr) {
      GELOGE(INTERNAL_ERROR, "[Check][PluginPtr] OpsKernelExecutor key=%s is null", it.first.c_str());
      REPORT_INNER_ERROR("E19999", "CheckPluginPtr OpsKernelExecutor key=%s is null", it.first.c_str());
      return FAILED;
    } else {
      GELOGI("Executor initialized, name = %s", it.first.c_str());
    }
  }
  return SUCCESS;
}
}  // namespace ge
