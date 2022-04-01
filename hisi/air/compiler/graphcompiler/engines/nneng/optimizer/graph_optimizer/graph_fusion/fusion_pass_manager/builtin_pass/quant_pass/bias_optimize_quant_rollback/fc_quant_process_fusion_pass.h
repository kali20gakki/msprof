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

#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_GRAPH_FUSION_FUSION_PASS_MANAGER_BUILTIN_PASS_QUANT_PASS_BIAS_OPTIMIZE_QUANT_ROLLBACK_FC_QUANT_PROCESS_FUSION_PASS_H_
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_GRAPH_FUSION_FUSION_PASS_MANAGER_BUILTIN_PASS_QUANT_PASS_BIAS_OPTIMIZE_QUANT_ROLLBACK_FC_QUANT_PROCESS_FUSION_PASS_H_

#include "bias_optimize_quant_rollback_base.h"

namespace fe {
class FCQuantProcessFusionPass : public BiasOptimizeQuantRollbackBase {
 protected:
  Status GetCoValue(ge::NodePtr &cube_node, int64_t &co) override;

  Status BiasOptimize(ge::ComputeGraph &graph, ge::NodePtr &cube_node, ge::NodePtr &dequant_node,
                      ge::NodePtr &quant_node, vector<ge::NodePtr> &fusion_nodes) override;
  vector<FusionPattern *> DefinePatterns() override;
};
}  // namespace fe

#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_GRAPH_FUSION_FUSION_PASS_MANAGER_BUILTIN_PASS_QUANT_PASS_BIAS_OPTIMIZE_QUANT_ROLLBACK_FC_QUANT_PROCESS_FUSION_PASS_H_
