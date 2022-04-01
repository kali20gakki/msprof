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

#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_GRAPH_FUSION_FUSION_PASS_MANAGER_BUILTIN_PASS_NODE_OPTIMIZE_CONV_CONCAT_FUSION_PASS_H_
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_GRAPH_FUSION_FUSION_PASS_MANAGER_BUILTIN_PASS_NODE_OPTIMIZE_CONV_CONCAT_FUSION_PASS_H_
#include <vector>
#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/node_optimize/checker/concat_optimize_checker.h"
#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/node_optimize/node_optimize_pass_base.h"

namespace fe {
enum ConvConcatFusionPattern {
  UN_SUPPORTED = 0,
  PATTERN_CONV2D_CONCAT = 1,
  PATTERN_CONV2D_RELU_CONCAT = 2,
  PATTERN_CONV2D_LEAKYRELU_CONCAT = 3,
  PATTERN_CONV2D_DEQUANT_CONCAT = 4,
  PATTERN_MAXPOOL_CONCAT = 5,
  PATTERN_CONV2D_REQUANT_CONCAT = 6,
  PATTERN_CONV2D_DEQUANT_LEAKYRELU_CONCAT = 7,
  PATTERN_CONV2D_MISH_CONCAT = 8
};

class ConvConcatFusionPass : public NodeOptimizePassBase {
 public:
  Status DoFusion(ge::ComputeGraph &graph, ge::NodePtr &node_ptr, vector<ge::NodePtr> &fusion_nodes) override;
  vector<string> GetNodeTypes() override;
  string GetPatternName() override;
  Status DoQuantFusion(ge::ComputeGraph &graph, ge::NodePtr concat_node, ge::NodePtr quant_node);

 private:
  Status MatchPattern(const ge::NodePtr &node_ptr);
  Status MatchForNoDequuant(const ge::NodePtr &node_ptr);
  Status MatchForDequant(const ge::NodePtr &node_ptr) const;
  ConvConcatFusionPattern GetMatchPattern(const ge::NodePtr &pre_node_ptr) const;
  Status InsertStrideWrite(ge::ComputeGraph &graph, const ge::NodePtr &node_ptr, vector <ge::OpDescPtr> &stride_write_op_desc_ptr_vec);
  Status IsQuantNodeSame(const ge::NodePtr quant_node, const ge::OutDataAnchor::Vistor<ge::InDataAnchorPtr> &in_anchors);
  Status IsConv(const ge::NodePtr &pre_node_ptr) const;
  Status IsMaxPool(const ge::NodePtr &pre_node_ptr) const;
  Status IsConvAndExpcetOp(const ge::NodePtr &pre_node_ptr, const string &expect_op_type) const;
  Status IsLeakyRelu(const ge::NodePtr &pre_node_ptr) const;
  Status IsDequantElemwise(const ge::NodePtr &pre_node_ptr) const;
  ConcatOptimizeChecker concat_optimize_checker;
  const size_t DIM_SIZE = 2;
};
}  // namespace fe
#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_GRAPH_FUSION_FUSION_PASS_MANAGER_BUILTIN_PASS_NODE_OPTIMIZE_CONV_CONCAT_FUSION_PASS_H_
