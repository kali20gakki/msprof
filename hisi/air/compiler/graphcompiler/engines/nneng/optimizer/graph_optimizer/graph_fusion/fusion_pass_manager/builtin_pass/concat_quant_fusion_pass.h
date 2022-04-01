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

#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_GRAPH_FUSION_FUSION_PASS_MANAGER_BUILTIN_PASS_CONCAT_QUANT_FUSION_PASS_H_
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_GRAPH_FUSION_FUSION_PASS_MANAGER_BUILTIN_PASS_CONCAT_QUANT_FUSION_PASS_H_

#include <vector>
#include "graph_optimizer/fusion_common/pattern_fusion_base_pass.h"

namespace fe {
class ConcatQuantFusionPass : public PatternFusionBasePass {
 protected:
  vector<FusionPattern *> DefinePatterns() override;
  Status Fusion(ge::ComputeGraph &graph, Mapping &mapping, vector<ge::NodePtr> &fusion_nodes) override;

 private:
  bool HasQuantToBeMerged(const ge::NodePtr &concat_node, const ge::NodePtr &quant_node) const;
  Status CheckConcatOpAligned(const ge::NodePtr &concat_node, const ge::DataType &quant_data_type) const;
  bool IsSameQuant(ge::NodePtr original_node, ge::NodePtr new_node) const;
  Status InsertAndMergeQuantNode(ge::ComputeGraph &graph, ge::NodePtr &quant_node, ge::NodePtr &concat_node,
                                 vector<ge::NodePtr> &fusion_nodes) const;
  Status MergeQuantNode(ge::ComputeGraph &graph, vector<ge::NodePtr> &quant_nodes) const;
  Status AnchorsCheck(ge::NodePtr &concat_node, ge::NodePtr &quant_node,
                      ge::ComputeGraph &graph, vector<ge::NodePtr> &fusion_nodes) const;
};
}  // namespace fe
#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_GRAPH_FUSION_FUSION_PASS_MANAGER_BUILTIN_PASS_CONCAT_QUANT_FUSION_PASS_H_
