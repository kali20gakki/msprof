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

#ifndef GE_OPSKERNEL_MANAGER_OPS_KERNEL_MANAGER_H_
#define GE_OPSKERNEL_MANAGER_OPS_KERNEL_MANAGER_H_

#include <map>
#include <set>
#include <memory>
#include <string>
#include <vector>
#include <mutex>

#include "framework/common/debug/log.h"
#include "common/plugin/plugin_manager.h"
#include "common/plugin/op_tiling_manager.h"
#include "framework/common/ge_inner_error_codes.h"
#include "common/opskernel/ops_kernel_info_store.h"
#include "common/optimizer/graph_optimizer.h"
#include "graph/optimize/graph_optimize.h"
#include "graph/manager/util/graph_optimize_utility.h"
#include "framework/common/ge_inner_error_codes.h"
#include "external/ge/ge_api_types.h"
#include "runtime/base.h"


namespace ge {
using OpsKernelInfoStorePtr = std::shared_ptr<OpsKernelInfoStore>;

class GE_FUNC_VISIBILITY OpsKernelManager {
 public:
  friend class GELib;
  static OpsKernelManager &GetInstance();
  // get opsKernelInfo by type
  const std::vector<OpInfo> &GetOpsKernelInfo(const std::string &op_type);

  // get all opsKernelInfo
  const std::map<std::string, std::vector<OpInfo>> &GetAllOpsKernelInfo() const;

  // get opsKernelInfoStore by name
  OpsKernelInfoStorePtr GetOpsKernelInfoStore(const std::string &name) const;

  // get all opsKernelInfoStore
  const std::map<std::string, OpsKernelInfoStorePtr> &GetAllOpsKernelInfoStores() const;

  // get all graph_optimizer
  const std::map<std::string, GraphOptimizerPtr> &GetAllGraphOptimizerObjs() const;

  // get all graph_optimizer by priority
  const std::vector<std::pair<std::string, GraphOptimizerPtr>> &GetAllGraphOptimizerObjsByPriority() const {
    return atomic_first_optimizers_by_priority_;
  }

  const std::map<std::string, std::set<std::string>> &GetCompositeEngines() const {
    return composite_engines_;
  }

  const std::map<std::string, std::string> &GetCompositeEngineKernelLibNames() const {
    return composite_engine_kernel_lib_names_;
  }

  // get subgraphOptimizer by engine name
  void GetGraphOptimizerByEngine(const std::string &engine_name, std::vector<GraphOptimizerPtr> &graph_optimizer);

  // get enableFeFlag
  bool GetEnableFeFlag() const;

  // get enableAICPUFlag
  bool GetEnableAICPUFlag() const;

  // get enablePluginFlag
  bool GetEnablePluginFlag() const;

 private:
  OpsKernelManager();
  ~OpsKernelManager();

  // opsKernelManager initialize, load all opsKernelInfoStore and graph_optimizer
  Status Initialize(const std::map<std::string, std::string> &init_options);

  // opsKernelManager finalize, unload all opsKernelInfoStore and graph_optimizer
  Status Finalize();

  Status InitOpKernelInfoStores(const std::map<std::string, std::string> &options);

  Status CheckPluginPtr() const;

  void GetExternalEnginePath(std::string &extern_engine_path, const std::map<std::string, std::string>& options);

  void InitOpsKernelInfo();

  Status InitGraphOptimizers(const std::map<std::string, std::string> &options);

  Status InitPluginOptions(const std::map<std::string, std::string> &options);

  Status ParsePluginOptions(const std::map<std::string, std::string> &options, const std::string &plugin_name, bool &enable_flag);

  void ClassifyGraphOptimizers();

  void InitGraphOptimizerPriority();

  // Finalize other ops kernel resource
  Status FinalizeOpsKernel();

  PluginManager plugin_manager_;
  OpTilingManager op_tiling_manager_;
  GraphOptimizeUtility graph_optimize_utility_;
  // opsKernelInfoStore
  std::map<std::string, OpsKernelInfoStorePtr> ops_kernel_store_{};
  // graph_optimizer
  std::map<std::string, GraphOptimizerPtr> graph_optimizers_{};
  // composite_graph_optimizer
  std::map<std::string, GraphOptimizerPtr> composite_graph_optimizers_{};
  // atomic_graph_optimizer
  std::map<std::string, GraphOptimizerPtr> atomic_graph_optimizers_{};
  // ordered atomic_graph_optimizer
  std::vector<std::pair<std::string, GraphOptimizerPtr>> atomic_graph_optimizers_by_priority_{};
  // atomic_first graph_optimizer
  std::vector<std::pair<std::string, GraphOptimizerPtr>> atomic_first_optimizers_by_priority_{};
  // {composite_engine, {containing atomic_engine_names}}
  std::map<std::string, std::set<std::string>> composite_engines_{};
  // {composite_engine, composite_engine_kernel_lib_name}
  std::map<std::string, std::string> composite_engine_kernel_lib_names_{};
  // opsKernelInfo
  std::map<std::string, std::vector<OpInfo>> ops_kernel_info_{};

  std::map<std::string, std::string> initialize_{};

  std::vector<OpInfo> empty_op_info_{};

  bool init_flag_;

  bool enable_fe_flag_ = false;

  bool enable_aicpu_flag_ = false;
};
}  // namespace ge
#endif  // GE_OPSKERNEL_MANAGER_OPS_KERNEL_MANAGER_H_
