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

#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_UB_FUSION_TBE_PASS_TBE_COMMON_RULES2_FUSION_PASS_H_
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_UB_FUSION_TBE_PASS_TBE_COMMON_RULES2_FUSION_PASS_H_

#include <vector>
#include "common/fe_log.h"
#include "common/fe_utils.h"
#include "graph_optimizer/buffer_fusion/buffer_fusion_pass_base.h"

namespace fe {

class TbeCommonRules2FusionPass : public BufferFusionPassBase {
 public:
  explicit TbeCommonRules2FusionPass() = default;

  ~TbeCommonRules2FusionPass() override = default;

 protected:
  /*
   * @brief:  define a common ub fusion pattern:
   * (StrideRead) -> Convolution -> (Dequant) -> Elewise*N -> Quant -> (StrideWrite)
   *
   * pattern limits:
   * 1. StrideRead, StrideWrite, Dequant are optional, Conv2D and Quant are required.
   * 2. Elewise supports LeakyRelu, Vadd, Relu, Relu6, Prelu, Add, Mul. The number of Elewise can be 0 to 5.
   * 3. There are two outputs from Dequant or Elewise, one is int8 or int4, the other is fp16.
   *
   *
   * fusion node: (StrideRead), Convolution, (AscendDequant), Elewise, AscendQuant,
   *
   * @return BufferFusionPattern: return all valid patterns.
   */
  vector<BufferFusionPattern *> DefinePatterns() override;

  /*
   * @brief: parse nodes matched in mapping and call DoFusion
   * @param [in] graph: original graph
   * @param [out] mapping: nodes matched by pattern
   * @return bool: fusion status ok or not.
   */
  Status GetFusionNodes(const BufferFusionMapping &mapping, vector<ge::NodePtr> &fusion_nodes) override;

 private:
  static int CountOtherOutput(vector<ge::NodePtr> dequant_nodes, vector<ge::NodePtr> elem_wise_nodes);

  static bool JudgeElemShapeInScopeLessThanOutScope(const vector<ge::NodePtr> &pre_elemwise_nodes,
                                                    const vector<ge::NodePtr> &elemwise_nodes);
  void CheckTransNodes(vector<ge::NodePtr> &trans1_nodes, vector<ge::NodePtr> &conv_depthwise_nodes,
                       vector<ge::NodePtr> &fusion_nodes) const;
};

}  // namespace fe

#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_UB_FUSION_TBE_PASS_TBE_COMMON_RULES2_FUSION_PASS_H_
