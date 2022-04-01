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

#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_FUSION_COMMON_FUSION_PASS_MANAGER_H_
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_FUSION_COMMON_FUSION_PASS_MANAGER_H_

#include <map>
#include <memory>
#include <string>
#include <vector>
#include "common/fe_inner_error_codes.h"
#include "common/fe_log.h"
#include "common/graph_comm.h"
#include "common/scope_allocator.h"
#include "common/plugin_manager.h"
#include "graph/compute_graph.h"
#include "graph_tuner/graph_tuner_errorcode.h"

namespace fe {
using PluginManagerPtr = std::shared_ptr<fe::PluginManager>;
using GraphCommPtr = std::shared_ptr<GraphComm>;
using ScopeAllocatorPtr = std::shared_ptr<ScopeAllocator>;
using LxFusionOptimizerFunc = std::function<tune::Status(ge::ComputeGraph &, GraphCommPtr,
                                                         ScopeAllocatorPtr, const string &, AOEOption)>;
using LxFusionRecoveryFunc = std::function<tune::Status(ge::ComputeGraph &, const std::vector<ge::NodePtr> &,
                                                        std::vector<ge::NodePtr> *, std::vector<ge::NodePtr> *)>;
using LxFusionFinalizeFunc = std::function<tune::Status(const ge::ComputeGraph &)>;

class FusionPassManager {
 public:
  FusionPassManager();
  ~FusionPassManager();

  Status Initialize(std::string engine_name);
  Status Finalize();

  LxFusionOptimizerFunc GetL1FusionOptimizerFunc() {return L1FusionOptimizer;};
  LxFusionRecoveryFunc GetL1FusionRecoveryFunc()  {return L1Recovery;};
  LxFusionOptimizerFunc GetL2FusionOptimizerFunc()  {return L2FusionOptimizer;};
  LxFusionRecoveryFunc GetL2FusionRecoveryFunc()  {return L2Recovery;};
  LxFusionFinalizeFunc GetLxFusionFinalizeFunc()  {return lx_fusion_finalize_;}


 private:
  bool init_flag_;
  std::string engine_name_;
  vector<PluginManagerPtr> fusion_pass_plugin_manager_vec_;
  PluginManagerPtr lx_fusion_plugin_manager_;

  LxFusionOptimizerFunc L1FusionOptimizer{nullptr};
  LxFusionRecoveryFunc L1Recovery{nullptr};
  LxFusionOptimizerFunc L2FusionOptimizer{nullptr};
  LxFusionRecoveryFunc L2Recovery{nullptr};
  LxFusionFinalizeFunc lx_fusion_finalize_{nullptr};
  const string L1_FUSION_OPTIMIZER_FUNC_NAME = "L1FusionOptimize";
  const string L1_RECOVERY_FUNC_NAME = "L1Recovery";
  const string L2_FUSION_OPTIMIZER_FUNC_NAME = "L2FusionOptimize";
  const string L2_RECOVERY_FUNC_NAME = "L2Recovery";
  const std::string LX_FUSION_PLUGIN = "libgraph_tuner.so";
  const string lx_fusion_finalize_func_name_ = "LxFusionFinalize";

  Status InitFusionPassPlugin(const std::string &engine_name);
  Status OpenPassPath(const std::string &file_path, std::vector<string> &get_pass_files);
  void CloseFusionPassPlugin();

  Status InitLxFusionPlugin(const std::string &engine_name);
  void CloseLxFusionPlugin();
};
}  // namespace fe
#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_FUSION_COMMON_FUSION_PASS_MANAGER_H_
