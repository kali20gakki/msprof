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

#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_GRAPH_FUSION_FUSION_PASS_MANAGER_BUILTIN_PASS_NODE_OPTIMIZE_SPLIT_CONV_CONCAT_FUSION_PASS_H_
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_GRAPH_FUSION_FUSION_PASS_MANAGER_BUILTIN_PASS_NODE_OPTIMIZE_SPLIT_CONV_CONCAT_FUSION_PASS_H_

#include <vector>
#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/node_optimize/checker/concat_optimize_checker.h"
#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/node_optimize/checker/split_optimize_checker.h"
#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/node_optimize/node_optimize_pass_base.h"

namespace fe {
class SplitConvConcatFusionPass : public NodeOptimizePassBase {
 public:
  Status DoFusion(ge::ComputeGraph &graph, ge::NodePtr &concat, vector<ge::NodePtr> &fusion_nodes) override;
  vector<string> GetNodeTypes() override;
  string GetPatternName() override;

 private:
  Status PatternConcatSplit(ge::NodePtr &concat, ge::NodePtr &split_node,
                            vector<ge::NodePtr> &vector_quant, vector<ge::NodePtr> &vector_dequant);
  Status CheckOutputSingleRef(ge::NodePtr &concat_node);
  ge::InDataAnchorPtr PatternPrevConv2dWithQuant(ge::OutDataAnchorPtr out_anchor, ge::NodePtr &quant,
                                                 ge::NodePtr &dequant);
  Status FusionSplit(vector<ge::NodePtr> &vector_quant, ge::ComputeGraph &graph, ge::NodePtr &split_node);
  Status FusionConcat(vector<ge::NodePtr> &vector_dequant, ge::ComputeGraph &graph, ge::NodePtr &concat);
  ConcatOptimizeChecker concat_optimize_checker;
  SplitOptimizeChecker split_optimize_checker;
};

}  // namespace fe
#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_GRAPH_FUSION_FUSION_PASS_MANAGER_BUILTIN_PASS_NODE_OPTIMIZE_SPLIT_CONV_CONCAT_FUSION_PASS_H_
