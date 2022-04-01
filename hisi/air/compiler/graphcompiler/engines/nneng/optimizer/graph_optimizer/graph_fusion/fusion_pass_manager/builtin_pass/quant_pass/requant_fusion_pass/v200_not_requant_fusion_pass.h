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

#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_GRAPH_FUSION_FUSION_PASS_MANAGER_BUILTIN_PASS_QUANT_PASS_REQUANT_FUSION_PASS_V200_NOT_REQUANT_FUSION_PASS_H
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_GRAPH_FUSION_FUSION_PASS_MANAGER_BUILTIN_PASS_QUANT_PASS_REQUANT_FUSION_PASS_V200_NOT_REQUANT_FUSION_PASS_H

#include <vector>
#include "graph_optimizer/fusion_common/pattern_fusion_base_pass.h"

namespace fe {

class V200NotRequantFusionPass : public PatternFusionBasePass {
 protected:
  vector<FusionPattern *> DefinePatterns() override;
  Status Fusion(ge::ComputeGraph &graph, Mapping &mapping, vector<ge::NodePtr> &fusion_nodes) override;

 private:
  void RecordNotRequantOutputAnchorMap(vector<ge::NodePtr> &relu_nodes, vector<ge::NodePtr> &fuse_nodes);
  void RemoveHostOpFromFusionNode(vector<ge::NodePtr> &new_nodes) const;

  Status NotRequantPattern1(ge::ComputeGraph &graph, ge::NodePtr dequant_node, vector<ge::NodePtr> &new_nodes,
                            Mapping &mapping);
  Status JudgeAndExtractQuantParas(vector<ge::NodePtr> dequant_nodes, vector<float> &scale_deq,
                                   vector<int8_t> &offset_w, vector<int8_t> &N) const;
  Status DealHighPerformanceDeqaunt(ge::ComputeGraph &graph, vector<ge::NodePtr> dequant_nodes,
                                    vector<float> &scale_deq, vector<int8_t> &N,
                                    vector<ge::NodePtr> &fusion_nodes) const;
  Status CreateRequantS16BasedOnEltwise(ge::ComputeGraph &graph, const ge::OpDescPtr &eltwise_op,
                                        ge::NodePtr &requants16_node,
                                        bool dual_output, vector<ge::NodePtr> &fusion_nodes) const;
  Status UpdateRequants16Node(const int &co, const std::unique_ptr<uint64_t[]> &scale64_data,
                              const ge::NodePtr &requants16_node, const float &offset_quant,
                              const bool &relu_flag, const bool &dual_output) const;
  Status InitRequantS16Op(ge::NodePtr requants16_node, ge::NodePtr quant_node, std::vector<float> &Scale_deq,
                          std::vector<int8_t> &N, bool relu_flag, bool dual_output) const;
  Status ChangeEdgeToRequantS16(ge::NodePtr requants16_node, ge::NodePtr eltwise_node) const;
  Status RequantS16MultOutput(ge::NodePtr eltwise_node, bool &mult_output, bool &relu_flag,
                              ge::NodePtr &relu_node) const;
  Status DealHighPerformanceEltwiseQuant(ge::ComputeGraph &graph, ge::NodePtr eltwise_node, vector<ge::NodePtr> &quants,
                                         std::vector<float> &Scale_deq, std::vector<int8_t> &N,
                                         vector<ge::NodePtr> &fusion_nodes) const;
  Status ParseQuantsOfNotRequantPattern2(ge::NodePtr eltwise_node, vector<ge::NodePtr> &quants) const;
  Status ParseDequantsOfNotRequantPattern2(ge::NodePtr eltwise_node, vector<ge::NodePtr> &dequants) const;
  Status ParseNotRequantPattern2(ge::NodePtr eltwise_node, vector<ge::NodePtr> &quants,
                                 vector<ge::NodePtr> &dequants) const;
  Status NotRequantPattern0(ge::ComputeGraph &graph, ge::NodePtr eltwise_node,
                            vector<ge::NodePtr> &fusion_nodes) const;

};
}  // namespace fe
#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_GRAPH_FUSION_FUSION_PASS_MANAGER_BUILTIN_PASS_QUANT_PASS_REQUANT_FUSION_PASS_V200_NOT_REQUANT_FUSION_PASS_H
