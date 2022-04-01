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

#include "maxpool_quant_process_fusion_pass.h"
#include "bias_optimize_quant_rollback_base.h"

namespace fe {
static const std::string PATTERN_ANTIQUANT = "pattern_antiquant";
static const std::string ANTIQUANT         = "AscendAntiQuant";
static const std::string MAXPOOL           = "MaxPool";

/*
 *  fusion pattern
 *
 *  MaxPool/ConcatV2 ---> AscendQuant ---> AscendAntiQuant ---> MaxPool--->
 *                                  |
 *                                  v
 *               MaxPool/ConcatV2 ---> AscendQuant --->
 *                      |
 *                      `------------> MaxPool--->
 *
 */
vector<FusionPattern *> MaxPoolQuantProcessFusionPass::DefinePatterns() {
  vector<FusionPattern *> patterns;
  FE_LOGD("Start to define MaxPool quant process fusion pattern.");
  FusionPattern *pattern = new (std::nothrow) FusionPattern("MaxPoolQuantProcessFusion");
  FE_CHECK(pattern == nullptr, FE_LOGW("new FusionPattern object failed!"), return patterns);
  pattern->AddOpDesc(PATTERN_QUANT, {QUANT})
      .AddOpDesc(PATTERN_ANTIQUANT, {ANTIQUANT})
      .AddOpDesc(PATTERN_CUBE, {MAXPOOL})
      .SetInputs(PATTERN_ANTIQUANT, {PATTERN_QUANT})
      .SetInputs(PATTERN_CUBE, {PATTERN_ANTIQUANT})
      .SetOutput(PATTERN_CUBE);
  patterns.push_back(pattern);

  return patterns;
}

/*
 * in maxpool quant rollback, we need to remove antiquant node and link edge
 */
Status MaxPoolQuantProcessFusionPass::Fusion(ge::ComputeGraph &graph, fe::PatternFusionBasePass::Mapping &mapping,
                                             vector<ge::NodePtr> &fusion_nodes) {
  ge::NodePtr quant_node = GetNodeFromMapping(PATTERN_QUANT, mapping);
  ge::NodePtr antiquant = GetNodeFromMapping(PATTERN_ANTIQUANT, mapping);
  ge::NodePtr maxpool = GetNodeFromMapping(PATTERN_CUBE, mapping);
  auto out_anchor = quant_node->GetInDataAnchor(0)->GetPeerOutAnchor();
  FE_CHECK_NOTNULL(out_anchor);
  ge::NodePtr pre_node = out_anchor->GetOwnerNode();
  FE_CHECK_NOTNULL(pre_node);
  // only when quant node pre_node is MaxPool or ConcatV2, do quant rollback
  if (pre_node->GetType() != MAXPOOL && pre_node->GetType() != CONCATV2) {
    FE_LOGD("MaxPool node[%s] not need to do quant rollback.", maxpool->GetName().c_str());
    return NOT_CHANGED;
  }
  auto in_anchors = antiquant->GetOutDataAnchor(0)->GetPeerInDataAnchors();
  // relink
  for (auto in_anchor : in_anchors) {
    (void)ge::GraphUtils::RemoveEdge(antiquant->GetOutDataAnchor(0), in_anchor);
    (void)ge::GraphUtils::AddEdge(out_anchor, in_anchor);
  }
  (void)ge::GraphUtils::RemoveEdge(quant_node->GetOutDataAnchor(0), antiquant->GetInDataAnchor(0));
  if (graph.RemoveNode(antiquant) != ge::GRAPH_SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][MaxPoolQntPcsFus][Fus] Remove node[%s] failed.", antiquant->GetName().c_str());
    return FAILED;
  }
  FE_LOGD("MaxPool node[%s] quant rollback success.", maxpool->GetName().c_str());
  return SUCCESS;
}

}  // namespace fe
