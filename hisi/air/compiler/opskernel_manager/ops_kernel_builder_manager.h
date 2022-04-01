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

#ifndef GE_OPSKERNEL_MANAGER_OPS_KERNEL_BUILDER_MANAGER_H_
#define GE_OPSKERNEL_MANAGER_OPS_KERNEL_BUILDER_MANAGER_H_

#include "common/plugin/plugin_manager.h"
#include "common/opskernel/ops_kernel_builder.h"
#include "external/ge/ge_api_error_codes.h"
#include "register/ops_kernel_builder_registry.h"

namespace ge {
class GE_OBJECT_VISIBILITY OpsKernelBuilderManager {
 public:
  ~OpsKernelBuilderManager();

  static OpsKernelBuilderManager &Instance();

  // opsKernelManager initialize, load all opsKernelInfoStore and graph_optimizer
  Status Initialize(const std::map<std::string, std::string> &options,
                    const std::string &path_base, const bool is_train = true);

  // opsKernelManager finalize, unload all opsKernelInfoStore and graph_optimizer
  Status Finalize();

  // get opsKernelIBuilder by name
  OpsKernelBuilderPtr GetOpsKernelBuilder(const std::string &name) const;

  // get all opsKernelBuilders
  const std::map<std::string, OpsKernelBuilderPtr> &GetAllOpsKernelBuilders() const;

  Status CalcOpRunningParam(Node &node) const;

  Status GenerateTask(const Node &node, RunContext &context, std::vector<domi::TaskDef> &tasks,
                      const bool atomic_engine_flag = true) const;

 private:
  OpsKernelBuilderManager() = default;
  Status GetLibPaths(const std::map<std::string, std::string> &options,
                     const std::string &path_base, std::string &lib_paths) const;

  std::unique_ptr<PluginManager> plugin_manager_;
};
}  // namespace ge
#endif  // GE_OPSKERNEL_MANAGER_OPS_KERNEL_BUILDER_MANAGER_H_
