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

#ifndef FUSION_ENGINE_OPTIMIZER_COMMON_PASS_MANAGER_H_
#define FUSION_ENGINE_OPTIMIZER_COMMON_PASS_MANAGER_H_

#include <fusion_config_manager/fusion_config_parser.h>
#include <vector>
#include "common/opskernel/ops_kernel_info_store.h"
#include "graph/graph.h"
#include "register/graph_optimizer/graph_fusion/graph_pass.h"

namespace fe {
using OpsKernelInfoStorePtr = std::shared_ptr<ge::OpsKernelInfoStore>;
using FusionConfigParserPtr = std::shared_ptr<FusionConfigParser>;
/**
 * @ingroup fe
 * @brief to manager passes
 * @author
 */
/** @brief define pass manager, which provides two interface:
* 1. add passes; 2. run passes */
class PassManager {
 public:
  explicit PassManager(FusionConfigParserPtr fusion_config_parser_ptr);

  /**
   * get passes added through AddPass() in the list
   * @author
   */
  const vector<GraphPass *> &GraphPasses();

  /**
   * add graph level pass through AddPass()
   * @param [in] pass to be added, which will be destructed with pass manager
   * @author
   */
  Status AddPass(const std::string &pass_name, const std::string &engine_name, GraphPass *pass,
                 const std::string fusion_type);

  /**
   * execute all the added passes , to optimize the graph
   * @param [inout] graph, the graph waiting for optimization
   * @return SUCCESS, successfully optimized the graph
   * @return NOT_CHANGED, the graph did not change even though traversed all
   * passes
   * @return FAILED, fail to modify graph
   * @author
   */
  Status Run(ge::ComputeGraph &graph);

  /**
   * execute the appointed passes , to optimize the graph
   * @param [inout] graph, the graph waiting for optimization
   * @return SUCCESS, successfully optimized the graph
   * @return NOT_CHANGED, the graph did not change even though traversed all
   * passes
   * @return FAILED, fail to modify graph
   * @author
   */
  static Status Run(ge::ComputeGraph &graph, vector<GraphPass *> &passes);

  /**
   * execute all the added passes , to optimize the graph
   * @param [inout] graph, the graph waiting for optimization
   * @return SUCCESS, successfully optimized the graph
   * @return NOT_CHANGED, the graph did not change even though traversed all
   * passes
   * @return FAILED, fail to modify graph
   * @author
   */
  Status Run(ge::ComputeGraph &graph, OpsKernelInfoStorePtr ops_kernel_info_store_ptr);

  /**
   * execute the appointed passes , to optimize the graph
   * @param [inout] graph, the graph waiting for optimization
   * @return SUCCESS, successfully optimized the graph
   * @return NOT_CHANGED, the graph did not change even though traversed all
   * passes
   * @return FAILED, fail to modify graph
   * @author
   */
  static Status Run(ge::ComputeGraph &graph, vector<GraphPass *> &passes,
                    OpsKernelInfoStorePtr ops_kernel_info_store_ptr);

  ~PassManager();

 private:
  vector<GraphPass *> graph_passes_;
  FusionConfigParserPtr fusion_config_parser_ptr_;
};

}  // namespace fe
#endif  // FUSION_ENGINE_OPTIMIZER_COMMON_PASS_MANAGER_H_
