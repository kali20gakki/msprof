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

#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_GRAPH_FUSION_FUSION_PASS_MANAGER_BUILTIN_PASS_QUANT_PASS_TF_MERGE_WEIGHT_QUANT_FUSION_PASS_H_
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_GRAPH_FUSION_FUSION_PASS_MANAGER_BUILTIN_PASS_QUANT_PASS_TF_MERGE_WEIGHT_QUANT_FUSION_PASS_H_

#include <vector>
#include "graph_optimizer/fusion_common/pattern_fusion_base_pass.h"

namespace fe {

class TfMergeWeightQuantFusionPass : public PatternFusionBasePass {
 protected:
  vector<FusionPattern *> DefinePatterns() override;
  Status Fusion(ge::ComputeGraph &graph, Mapping &mapping, vector<ge::NodePtr> &fusion_nodes) override;

 private:
  Status MergeWeight(ge::ComputeGraph &graph, const ge::NodePtr &cube_node,
                     const ge::NodePtr &weight_quant_node) const;
  /*
  [case1]
    cube don't have third input, graph is cube + biasadd, usually in tf case

   *                       \     /
   *                     WeightQuant        bias
   *                          |              /
   *                          V             /
   *  --> AscendQuant --> CUBE_NODE --> bias_add --> AscendDequant -->
   *
   * in this case is_bias_and_cube_direct_link = false

  [case2]
    cube has third input as its bias, usually in onnx case

   *                       \     /
   *                     WeightQuant
   *                          |
   *                          V
   *  --> AscendQuant --> CUBE_NODE --> AscendDequant -->
   *                          ^
   *                          |
   *                         bias
   *
   * in this case bias_add = nullptr, is_bias_and_cube_direct_link = true
  */
  Status MergeBias(ge::ComputeGraph &graph, ge::NodePtr &cube_node,
    const ge::NodePtr &bias_add, ge::NodePtr &bias, bool is_bias_and_cube_direct_link) const;
  Status MergeBiasWithCast(const ge::NodePtr &cube_node, const ge::NodePtr &cast_node) const;
  Status MergeBiasNoCast(const ge::NodePtr &cube_node, const ge::NodePtr &bias_const, bool is_bias_and_cube_direct_link) const;
  Status remove_nodes_from_map(ge::ComputeGraph &graph, vector<ge::NodePtr> &del_nodes) const;
};

}  // namespace fe
#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_GRAPH_FUSION_FUSION_PASS_MANAGER_BUILTIN_PASS_QUANT_PASS_TF_MERGE_WEIGHT_QUANT_FUSION_PASS_H_
