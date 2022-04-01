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

#ifndef FUSION_ENGINE_OPTIMIZER_FUSION_MANAGER_FUSION_MANAGER_H_
#define FUSION_ENGINE_OPTIMIZER_FUSION_MANAGER_FUSION_MANAGER_H_

#include <map>
#include <memory>
#include <string>
#include "adapter/common/op_store_adapter_manager.h"
#include "graph_optimizer/fe_graph_optimizer.h"
#include "graph_optimizer/dsa_optimizer/dsa_graph_optimizer.h"
#include "ops_kernel_store/fe_ops_kernel_info_store.h"
#include "platform_info.h"

namespace fe {
using GraphOptimizerPtr = std::shared_ptr<ge::GraphOptimizer>;
using FEGraphOptimizerPtr = std::shared_ptr<fe::FEGraphOptimizer>;
using DSAGraphOptimizerPtr = std::shared_ptr<fe::DSAGraphOptimizer>;
using OpStoreAdapterManagerPtr = std::shared_ptr<fe::OpStoreAdapterManager>;

class FusionManager {
 public:
  static FusionManager &Instance(const std::string &engine_name);

  /*
   * to initialize the subparts of fusion manager
   * param[in] the options of init
   * param[in] engine_name
   * param[in] soc_version soc version from ge
   * return Status(SUCCESS/FAILED)
   */
  Status Initialize(const std::map<string, string> &options, const std::string &engine_name, const std::string &soc_version);

  /*
   * to release the source of fusion manager
   * return Status(SUCCESS/FAILED)
   */
  Status Finalize();

  /*
   * to get the information of OpsKernel InfoStores
   * param[out] the map of OpsKernel InfoStores
   */
  void GetOpsKernelInfoStores(map<string, OpsKernelInfoStorePtr> &op_kern_infos, const std::string &engine_name);

  /*
   * to get the information of Graph Optimizer
   * param[out] the map of Graph Optimizer
   */
  void GetGraphOptimizerObjs(map<string, GraphOptimizerPtr> &graph_optimizers, const std::string &engine_name);

  Status CheckOptiCompilationOfAiCoreNum(const map<string, string> &options, const PlatformInfo &platform_info,
                                         OptionalInfo &opti_compilation_info) const;

  Status CheckOptiCompilationInfo(const map<string, string> &options, const PlatformInfo &platform_info,
                                  OptionalInfo &opti_compilation_info) const;

  Status CheckOptiCompilationOfAiCoreNum(const map<string, string> &options, PlatFormInfos &platform_info,
                                         OptionalInfos &opti_compilation_info) const;

  Status CheckOptiCompilationInfo(const map<string, string> &options, PlatFormInfos &platform_info,
                                  OptionalInfos &opti_compilation_info) const;

  Status InitPlatformConfig(const std::string &soc_version, const map<string, string> &options) const;

 private:
  FusionManager();
  ~FusionManager();
  FEOpsKernelInfoStorePtr ops_kernel_info_store_;
  FEGraphOptimizerPtr graph_opt_;
  DSAGraphOptimizerPtr dsa_graph_opt_;
  OpStoreAdapterManagerPtr op_store_adapter_manager_;
  bool inited_;
};
}  // namespace fe

#endif  // FUSION_ENGINE_OPTIMIZER_FUSION_MANAGER_FUSION_MANAGER_H_
