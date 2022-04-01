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

#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/concat_quant_fusion_pass.h"
#include <algorithm>
#include <vector>
#include "common/fe_inner_attr_define.h"
#include "common/fe_utils.h"
#include "common/util/op_info_util.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/node_utils.h"
#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/node_optimize/checker/node_optimize_checker_base.h"
#include "common/math_util.h"

/**
 *       \    |     /                    \       |        /
 *          concat                      quant  quant  quant
 *            |          ======>           \     |     /
 *          quant                             concat
 *            |                                  |
 *   ============================================================
 *        |                                     |
 *        A (single out)                        A
 *      /   \                                   |
 *     |    quant1                            quant1
 *     |     |                               /      \
 *     |    ...      =================>     |        ...
 *     |     |                              |         |
 *      \    |                              |       quant1
 *        concat                             \       /
 *           |                                 concat
 *         quant1                                |
 */
namespace fe {
static const std::string PATTERN_CONCAT = "concat";
static const std::string PATTERN_QUANT = "quant";
const size_t kDimsSizeLimit = 4;

vector<FusionPattern *> ConcatQuantFusionPass::DefinePatterns() {
  vector<FusionPattern *> patterns;
  FusionPattern *pattern = new (std::nothrow) FusionPattern("ConcatQuantFusionPass");
  FE_CHECK(pattern == nullptr, REPORT_FE_ERROR("[GraphOpt][ConCatQuatFus][DfnPtn] Fail to new an object."),
           return patterns);

  pattern->AddOpDesc(PATTERN_CONCAT, {CONCATD, CONCATV2D})
      .AddOpDesc(PATTERN_QUANT, {ASCEND_QUANT})
      .SetInputs(PATTERN_QUANT, {PATTERN_CONCAT})
      .SetOutput(PATTERN_QUANT);
  patterns.push_back(pattern);

  return patterns;
}

Status ConcatQuantFusionPass::Fusion(ge::ComputeGraph &graph, Mapping &mapping, vector<ge::NodePtr> &fusion_nodes) {
  ge::NodePtr concat_node = GetNodeFromMapping(PATTERN_CONCAT, mapping);
  ge::NodePtr quant_node = GetNodeFromMapping(PATTERN_QUANT, mapping);
  FE_CHECK(concat_node == nullptr, REPORT_FE_ERROR("[GraphOpt][ConCatQuatFus][Fus] concat node is nullptr."),
           return FAILED);
  FE_CHECK(quant_node == nullptr|| quant_node->GetOpDesc() == nullptr,
    REPORT_FE_ERROR("[GraphOpt][ConCatQuatFus][Fus] quant node is nullptr."), return FAILED);
  auto quant_data_type = quant_node->GetOpDesc()->GetOutputDesc(0).GetDataType();
  if (quant_data_type == ge::DT_INT4) {
    FE_LOGD("Quant node [%s] output data type is int4, not fusion.", quant_node->GetName().c_str());
    return NOT_CHANGED;
  }
  auto concat_input_nodes = concat_node->GetInAllNodes();
  if (CheckConcatOpAligned(concat_node, quant_data_type) != SUCCESS) {
    FE_LOGD("Concat node[%s] input shape is not aligned or concat dim is not N.", concat_node->GetName().c_str());
    return NOT_CHANGED;
  }
  for (auto input_node : concat_input_nodes) {
    if (input_node == nullptr) {
      continue;
    }
    if (input_node->GetType() == STRIDEDWRITE) {
      FE_LOGI("Node[%s] has stridedwrite node before it, not do concat quant fusion.", concat_node->GetName().c_str());
      return NOT_CHANGED;
    }
  }
  return AnchorsCheck(concat_node, quant_node, graph, fusion_nodes);
}

Status ConcatQuantFusionPass::AnchorsCheck(ge::NodePtr &concat_node, ge::NodePtr &quant_node,
                                           ge::ComputeGraph &graph, vector<ge::NodePtr> &fusion_nodes) const {
  auto out_anchors = concat_node->GetAllOutDataAnchors();
  if (out_anchors.size() != 1) {
    FE_LOGD("Node[%s] out data anchors is not single, not fusion.", concat_node->GetName().c_str());
    return NOT_CHANGED;
  }
  auto out_anchor = out_anchors.at(0);
  FE_CHECK_NOTNULL(out_anchor);
  auto peer_anchors = out_anchor->GetPeerInDataAnchors();
  if (peer_anchors.size() != 1) {
    FE_LOGD("Node[%s] is not single output and single reference, not fusion.", concat_node->GetName().c_str());
    return NOT_CHANGED;
  }
  if (!HasQuantToBeMerged(concat_node, quant_node)) {
    FE_LOGD("Quant node[%s] has no quant node to merge, not fusion.", quant_node->GetName().c_str());
    return NOT_CHANGED;
  }
  // move quant node to concat input node
  if (InsertAndMergeQuantNode(graph, quant_node, concat_node, fusion_nodes) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][ConCatQuatFus][Fus] Insert and merge quant node failed, original quant node is [%s]",
                    quant_node->GetName().c_str());
    return FAILED;
  }
  FE_LOGD("Concat quant fusion pass success, concat node[%s].", concat_node->GetName().c_str());
  return SUCCESS;
}

bool ConcatQuantFusionPass::HasQuantToBeMerged(const ge::NodePtr &concat_node,
                                               const ge::NodePtr &quant_node) const {
  if (concat_node == nullptr) {
    return false;
  }
  for (auto concat_in_anchor : concat_node->GetAllInDataAnchors()) {
    if (concat_in_anchor == nullptr) {
      continue;
    }
    auto peer_out_anchor = concat_in_anchor->GetPeerOutAnchor();
    if (peer_out_anchor == nullptr) {
      continue;
    }
    for (auto in_anchor : peer_out_anchor->GetPeerInDataAnchors()) {
      if (in_anchor == nullptr || in_anchor->GetOwnerNode() == nullptr) {
        continue;
      }
      if (IsSameQuant(quant_node, in_anchor->GetOwnerNode())) {
        return true;
      }
    }
  }
  return false;
}

Status ConcatQuantFusionPass::CheckConcatOpAligned(const ge::NodePtr &concat_node,
                                                   const ge::DataType& quant_data_type) const {
  std::shared_ptr<NodeOptimizeCheckerBase> node_checker_ptr = nullptr;
  FE_MAKE_SHARED(node_checker_ptr = std::make_shared<NodeOptimizeCheckerBase>(), return FAILED);
  // concat dim is not dim c, not need to check
  for (auto input_desc : concat_node->GetOpDesc()->GetAllInputsDescPtr()) {
    if (input_desc->GetOriginShape().GetDims().size() != kDimsSizeLimit) {
      FE_LOGD("Concat node:[%s] has input shape is not 4D.", concat_node->GetName().c_str());
      return FAILED;
    }
  }
  if (!node_checker_ptr->IsDimC(concat_node, CONCAT_DIM, true)) {
    FE_LOGD("concat node %s concat dim is not dim_c.", concat_node->GetName().c_str());
    return SUCCESS;
  }
  int dim_c = 0;
  for (auto &input_desc : concat_node->GetOpDesc()->GetAllInputsDescPtr()) {
    // check all input_desc
    if (node_checker_ptr->GetDimC(*(input_desc.get()), dim_c) != SUCCESS) {
      FE_LOGW("Get dim_c value failed, concat node:[%s]", concat_node->GetName().c_str());
      return FAILED;
    }
    if (!node_checker_ptr->IsDimCOfInputAligned(*(input_desc.get()), dim_c, quant_data_type)) {
      FE_LOGD("Concat node [%s] dim_c is not aligned, not fusion.", concat_node->GetName().c_str());
      return FAILED;
    }
  }
  return SUCCESS;
}

bool ConcatQuantFusionPass::IsSameQuant(ge::NodePtr original_node, ge::NodePtr new_node) const {
  if (original_node->GetType() != ASCEND_QUANT || new_node->GetType() != ASCEND_QUANT) {
    return false;
  }
  ge::DataType ori_data_type = original_node->GetOpDesc()->GetOutputDesc(0).GetDataType();
  ge::DataType data_type = new_node->GetOpDesc()->GetOutputDesc(0).GetDataType();
  float ori_scale = 1;
  float scale = 1;
  float ori_offset = 0;
  float offset = 0;
  bool ori_sqrt_mode = false;
  bool sqrt_mode = false;
  (void)ge::AttrUtils::GetFloat(original_node->GetOpDesc(), "scale", ori_scale);
  (void)ge::AttrUtils::GetFloat(original_node->GetOpDesc(), "offset", ori_offset);
  (void)ge::AttrUtils::GetBool(original_node->GetOpDesc(), "sqrt_mode", ori_sqrt_mode);
  (void)ge::AttrUtils::GetFloat(new_node->GetOpDesc(), "scale", scale);
  (void)ge::AttrUtils::GetFloat(new_node->GetOpDesc(), "offset", offset);
  (void)ge::AttrUtils::GetBool(new_node->GetOpDesc(), "sqrt_mode", sqrt_mode);
  if (!FloatEqual(ori_scale, scale) ||
      !FloatEqual(ori_offset, offset) ||
      (ori_sqrt_mode != sqrt_mode) ||
      (ori_data_type !=  data_type)) {
    return false;
  }
  return true;
}

Status ConcatQuantFusionPass::InsertAndMergeQuantNode(ge::ComputeGraph &graph, ge::NodePtr &quant_node,
                                                      ge::NodePtr &concat_node,
                                                      vector<ge::NodePtr> &fusion_nodes) const {
  ge::OpDescPtr quant_desc = quant_node->GetOpDesc();
  std::string quant_name = quant_desc->GetName();
  ge::OpDescPtr concat_desc = concat_node->GetOpDesc();
  ge::DataType dtype = quant_desc->GetOutputDesc(0).GetDataType();
  vector<ge::NodePtr> quant_nodes;
  // insert quant node before concat node
  for (auto &anchor : concat_node->GetAllInDataAnchors()) {
    auto peer_out_anchor = anchor->GetPeerOutAnchor();
    if (peer_out_anchor == nullptr) {
      continue;
    }
    auto idx = anchor->GetIdx();
    auto tmp_desc = ge::AttrUtils::CopyOpDesc(quant_desc);
    tmp_desc->SetName(quant_name + concat_node->GetName() + std::to_string(static_cast<uint32_t>(idx)));
    auto input_desc = concat_desc->GetInputDesc(static_cast<uint32_t>(idx));
    tmp_desc->UpdateInputDesc(0, input_desc);
    tmp_desc->UpdateOutputDesc(0, input_desc);
    tmp_desc->MutableOutputDesc(0)->SetDataType(dtype);
    tmp_desc->MutableOutputDesc(0)->SetOriginDataType(dtype);
    auto input_desc_ptr = concat_desc->MutableInputDesc(static_cast<uint32_t>(idx));
    if (input_desc_ptr == nullptr) {
      continue;
    }
    input_desc_ptr->SetDataType(dtype);
    input_desc_ptr->SetOriginDataType(dtype);
    ge::NodePtr new_quant = graph.AddNode(tmp_desc);
    if (new_quant == nullptr) {
      continue;
    }
    fusion_nodes.push_back(new_quant);
    quant_nodes.push_back(new_quant);
    (void)ge::GraphUtils::RemoveEdge(peer_out_anchor, anchor);
    (void)ge::GraphUtils::AddEdge(peer_out_anchor, new_quant->GetInDataAnchor(0));
    (void)ge::GraphUtils::AddEdge(new_quant->GetOutDataAnchor(0), anchor);
  }

  (void)MergeQuantNode(graph, quant_nodes);
  // remove ori quant node
  auto concat_out_anchor = concat_node->GetOutDataAnchor(0);
  (void)concat_node->GetOpDesc()->MutableOutputDesc(0)->SetDataType(dtype);
  (void)ge::GraphUtils::RemoveEdge(concat_node->GetOutDataAnchor(0), quant_node->GetInDataAnchor(0));
  for (auto quant_peer_in_anchor : quant_node->GetOutDataAnchor(0)->GetPeerInDataAnchors()) {
    if (quant_peer_in_anchor == nullptr) {
      continue;
    }
    (void)ge::GraphUtils::RemoveEdge(quant_node->GetOutDataAnchor(0), quant_peer_in_anchor);
    (void)ge::GraphUtils::AddEdge(concat_out_anchor, quant_peer_in_anchor);
  }
  (void)graph.RemoveNode(quant_node);
  return SUCCESS;
}

Status ConcatQuantFusionPass::MergeQuantNode(ge::ComputeGraph &graph, vector<ge::NodePtr> &quant_nodes) const {
  // merge same quant node from one out anchor
  for (auto quant : quant_nodes) {
    if (quant == nullptr || quant->GetAllInDataAnchorsSize() == 0) {
      continue;
    }
    auto quant_in_anchor = quant->GetInDataAnchor(0);
    if (quant_in_anchor == nullptr || quant_in_anchor->GetPeerOutAnchor() == nullptr) {
      continue;
    }
    auto peer_out_anchor = quant_in_anchor->GetPeerOutAnchor();
    for (auto peer_in_anchor : peer_out_anchor->GetPeerInDataAnchors()) {
      if (peer_in_anchor == quant_in_anchor) {
        continue;
      }
      bool skip_flag = peer_in_anchor == nullptr || peer_in_anchor->GetOwnerNode() == nullptr ||
                       peer_in_anchor->GetOwnerNode()->GetOpDesc() == nullptr ||
                       !IsSameQuant(quant, peer_in_anchor->GetOwnerNode());
      if (skip_flag) {
        continue;
      }
      // relink same quant node
      auto next_quant_node = peer_in_anchor->GetOwnerNode();
      if (next_quant_node->GetAllOutDataAnchors().empty() || next_quant_node->GetOutDataAnchor(0) == nullptr ||
          next_quant_node->GetOutDataAnchor(0)->GetPeerInDataAnchors().empty()) {
        continue;
      }
      for (auto next_anchor : next_quant_node->GetOutDataAnchor(0)->GetPeerInDataAnchors()) {
        (void)ge::GraphUtils::RemoveEdge(next_quant_node->GetOutDataAnchor(0), next_anchor);
        (void)ge::GraphUtils::AddEdge(quant->GetOutDataAnchor(0), next_anchor);
      }
      (void)ge::GraphUtils::RemoveEdge(peer_out_anchor, peer_in_anchor);
      (void)graph.RemoveNode(next_quant_node);
    }
  }
  return SUCCESS;
}
}  // namespace fe
