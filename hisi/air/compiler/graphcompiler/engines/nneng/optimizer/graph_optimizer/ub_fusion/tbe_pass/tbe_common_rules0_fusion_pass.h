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

#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_UB_FUSION_TBE_PASS_TBE_COMMON_RULES0_FUSION_PASS_H_
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_UB_FUSION_TBE_PASS_TBE_COMMON_RULES0_FUSION_PASS_H_

#include <vector>
#include "common/fe_log.h"
#include "common/fe_utils.h"
#include "graph_optimizer/buffer_fusion/buffer_fusion_pass_base.h"

namespace fe {
class TbeCommonRules0FusionPass : public BufferFusionPassBase {
 public:
  explicit TbeCommonRules0FusionPass() {}

  ~TbeCommonRules0FusionPass() override {}

 protected:
  /*
  * @brief:  define common rules0 ops fusion pattern
  *
  *   (StrideRead) + conv2_d + (dequant) + ele-wise*N + (quant) + (StrideWrite)
  *   restriction: 1.each node must be single output and single reference
  *                2.the range of N is 0 to 5
  *                3.allow multiple input, but only one input can be fusion
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
  static bool DealWithSameInAndOutScopeDimSize(const vector<int64_t> &in_scope_dims,
                                               const vector<int64_t> &out_scope_dims,
                                               const vector<ge::NodePtr> &elemwise_nodes,
                                               const ge::NodePtr &cur_node, const size_t &i,
                                               vector<ge::NodePtr> &fusion_nodes);

  static bool JudgeElemShapeInScopeLessThanOutScope(const vector<ge::NodePtr> &pre_elemwise_nodes,
                                                    const vector<ge::NodePtr> &elemwise_nodes,
                                                    vector<ge::NodePtr> &fusion_nodes);
  static bool IsInBlackListOfOpPatternElemwise(vector<ge::NodePtr> &elemwise_nodes, ge::NodePtr &node_ptr);

  void CheckTransNodes(vector<ge::NodePtr> &trans1_nodes, vector<ge::NodePtr> &conv_depthwise_nodes,
                       vector<ge::NodePtr> &fusion_nodes) const;
};
}  // namespace fe

#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_UB_FUSION_TBE_PASS_TBE_COMMON_RULES0_FUSION_PASS_H_
