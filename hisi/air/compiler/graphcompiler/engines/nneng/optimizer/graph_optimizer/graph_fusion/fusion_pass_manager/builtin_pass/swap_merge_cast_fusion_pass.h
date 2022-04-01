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

#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_GRAPH_FUSION_FUSION_PASS_MANAGER_BUILTIN_PASS_SWAP_MERGE_CAST_FUSION_PASS_H_
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_GRAPH_FUSION_FUSION_PASS_MANAGER_BUILTIN_PASS_SWAP_MERGE_CAST_FUSION_PASS_H_

#include "graph_optimizer/fusion_common/pattern_fusion_base_pass.h"

namespace fe {
/** @brief swap cast node and merge node
*        while merge node, cast node and netoutput node are in a line */
class SwapMergeCastFusionPass : public PatternFusionBasePass {
 protected:
  vector<FusionPattern *> DefinePatterns() override;
  Status Fusion(ge::ComputeGraph &graph, Mapping &mapping, vector<ge::NodePtr> &new_nodes) override;

 private:
  Status VerifyNodes(const ge::NodePtr &merge_node, ge::NodePtr &cast_node, ge::NodePtr &netout_node) const;

  Status RelinkMergeNode(const ge::NodePtr &merge_node, const ge::NodePtr &cast_node, const ge::NodePtr &netout_node);

  Status AddCastNodeBeforeMergeNode(const ge::NodePtr &merge_node, ge::OpDescPtr &cast_op_desc,
                                    ge::ComputeGraph &graph);
};
}  // namespace fe

#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_GRAPH_FUSION_FUSION_PASS_MANAGER_BUILTIN_PASS_SWAP_MERGE_CAST_FUSION_PASS_H_
