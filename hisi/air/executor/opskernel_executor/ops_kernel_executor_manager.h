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

#ifndef AIR_EXECUTOR_OPSKERNEL_EXECUTOR_OPSKERNEL_EXECUTOR_MANAGER_H_
#define AIR_EXECUTOR_OPSKERNEL_EXECUTOR_OPSKERNEL_EXECUTOR_MANAGER_H_

#include <map>
#include <memory>
#include <mutex>
#include "common/opskernel/ops_kernel_info_store.h"
#include "common/plugin/plugin_manager.h"

namespace ge {
using OpsKernelExecutor = OpsKernelInfoStore;
using OpsKernelExecutorPtr = std::shared_ptr<OpsKernelExecutor>;

class OpsKernelExecutorManager {
 public:
  ~OpsKernelExecutorManager() = default;
  static OpsKernelExecutorManager &GetInstance() {
    static OpsKernelExecutorManager instance;
    return instance;
  }

  Status Initialize(const std::map<std::string, std::string> &options);
  void Finalize();
  Status GetExecutor(const std::string &name, OpsKernelExecutor *&executor);

 private:
  OpsKernelExecutorManager() = default;
  static std::string GetHcclExecutorPluginLibPath();
  static std::string GetExecutorPluginLibPaths();
  static Status CheckExecutors(const std::map<std::string, OpsKernelExecutorPtr> &executors);
  Status InitializePlugin(PluginManager &plugin_manager, const std::string &plugin_paths);

  std::mutex mu_;
  std::map<std::string, std::string> options_;
  std::unique_ptr<PluginManager> hccl_executor_plugin_;  // load on-demand
  PluginManager executor_plugin_;
  std::map<std::string, OpsKernelExecutorPtr> executors_;
};
}  // namespace ge

#endif  // AIR_EXECUTOR_OPSKERNEL_EXECUTOR_OPSKERNEL_EXECUTOR_MANAGER_H_
