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

#include "swap_merge_cast_fusion_pass.h"
#include <map>
#include <string>
#include <vector>

#include "common/fe_log.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/graph_utils.h"
#include "graph_optimizer/graph_fusion/fusion_pass_manager/fusion_pass_registry.h"

using namespace ge;

/**
 *      Input                            Input
 *        |                                |
 *      switch                           Switch
 *     /     \                          /     \
 *    A      B                         A      B
 *     \    /           ->             |      |
 *      Merge                        Cast    Cast
 *        |                            \     /
 *       Cast                           Merge
 *        |                               |
 *     NetOutput                       NetOutPut
 */
namespace fe {
static const string SWAPMERGECAST_PASS_NAME = "SwapMergeCastFusionPass";
static const string PATTERN_MERGE = "Pattern_Merge";
static const string PATTERN_CAST = "Pattern_Cast";
static const string OP_TYPE_MERGE = "Merge";
static const string OP_TYPE_CAST = "Cast";
static const string OP_TYPE_NETOUTPUT = "NetOutput";

vector<FusionPattern *> SwapMergeCastFusionPass::DefinePatterns() {
  vector<FusionPattern *> patterns;

  FusionPattern *pattern = new (std::nothrow) FusionPattern("SwapMergeCastFusionPattern");
   FE_CHECK(pattern == nullptr,
            REPORT_FE_ERROR("[GraphOpt][SwapMrgCastFus][DefPtn] Fail to create a new pattern object."),
            return patterns);

  pattern->AddOpDesc(PATTERN_MERGE, {OP_TYPE_MERGE})
      .AddOpDesc(PATTERN_CAST, {OP_TYPE_CAST})
      .SetInputs(PATTERN_CAST, {PATTERN_MERGE})
      .SetOutput(PATTERN_CAST);

  patterns.push_back(pattern);

  return patterns;
}

Status SwapMergeCastFusionPass::Fusion(ge::ComputeGraph &graph, Mapping &mapping, vector<ge::NodePtr> &new_nodes) {
  ge::NodePtr merge_node = GetNodeFromMapping(PATTERN_MERGE, mapping);
  ge::NodePtr cast_node = GetNodeFromMapping(PATTERN_CAST, mapping);

  ge::NodePtr netout_node = nullptr;
  Status verify_status = VerifyNodes(merge_node, cast_node, netout_node);
  if (verify_status != SUCCESS) {
    return verify_status;
  }

  // unlink cast node and link merge node to netoutput node
  Status status = RelinkMergeNode(merge_node, cast_node, netout_node);
  if (status != SUCCESS) {
    return status;
  }

  // add cast node for each input data anchor of merge node
  OpDescPtr cast_op_desc = cast_node->GetOpDesc();
  status = AddCastNodeBeforeMergeNode(merge_node, cast_op_desc, graph);
  if (status != SUCCESS) {
    return status;
  }

  if (graph.RemoveNode(cast_node) != GRAPH_SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][SwapMrgCastFus][Fus] Fail to remove cast node[%s] from graph.",
                    cast_node->GetName().c_str());
    return FAILED;
  }

  return SUCCESS;
}

Status SwapMergeCastFusionPass::AddCastNodeBeforeMergeNode(const ge::NodePtr &merge_node, ge::OpDescPtr &cast_op_desc,
                                                           ge::ComputeGraph &graph) {
  OpDescPtr merge_op_desc = merge_node->GetOpDesc();
  DataType cast_out_d_type = cast_op_desc->MutableOutputDesc(0)->GetDataType();
  merge_op_desc->MutableOutputDesc(0)->SetDataType(cast_out_d_type);

  size_t input_size = merge_op_desc->GetAllInputsSize();
  for (size_t i = 0; i < input_size; i++) {
    InDataAnchorPtr in_data_anchor = merge_node->GetInDataAnchor(i);
    if (in_data_anchor == nullptr || in_data_anchor->GetPeerOutAnchor() == nullptr) {
      FE_LOGD("InData Anchor[%zu] of merge node[%s] is not linked.", i, merge_node->GetName().c_str());
      continue;
    }

    // update data Type of each input tensor desc of merge node
    GeTensorDescPtr in_data_desc = merge_op_desc->MutableInputDesc(i);
    if (in_data_desc == nullptr) {
      FE_LOGD("In data desc[%zu] is null.", i);
      continue;
    }
    in_data_desc->SetDataType(cast_out_d_type);

    // copy cast op desc and update the shape of input and output
    OpDescPtr new_cast_op_desc = AttrUtils::CopyOpDesc(cast_op_desc);
    FE_CHECK(new_cast_op_desc == nullptr,
             REPORT_FE_ERROR("[GraphOpt][SwapMrgCastFus][AddCastNd] Fail to copy op desc for cast node[%s].",
                             cast_op_desc->GetName().c_str()),
             return FAILED);

    new_cast_op_desc->SetName(cast_op_desc->GetName() + std::to_string(i));
    new_cast_op_desc->MutableInputDesc(0)->SetShape(in_data_desc->GetShape());
    new_cast_op_desc->MutableOutputDesc(0)->SetShape(in_data_desc->GetShape());

    NodePtr new_cast_node = graph.AddNode(new_cast_op_desc);
    FE_CHECK(new_cast_node == nullptr,
             REPORT_FE_ERROR("[GraphOpt][SwapMrgCastFus][AddCastNd] Fail to add cast node[%s] to graph.",
                             new_cast_op_desc->GetName().c_str()),
             return FAILED);

    OutDataAnchorPtr out_data_anchor = in_data_anchor->GetPeerOutAnchor();
    FE_CHECK_NOTNULL(out_data_anchor);
    // unlink the indata anchor of merge node
    in_data_anchor->UnlinkAll();
    if (GraphUtils::AddEdge(out_data_anchor, new_cast_node->GetInDataAnchor(0)) != GRAPH_SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][SwapMrgCastFus][AddCastNd] Fail to link in_data_anchor of cast node[%s].",
                      new_cast_node->GetName().c_str());
      return FAILED;
    }
    if (GraphUtils::AddEdge(new_cast_node->GetOutDataAnchor(0), in_data_anchor) != GRAPH_SUCCESS) {
      REPORT_FE_ERROR(
          "[GraphOpt][SwapMrgCastFus][AddCastNd] Fail to link in_data_anchor[%zu] of merge node[%s]"
          " with cast node.",
          i, merge_node->GetName().c_str());
      return FAILED;
    }
  }

  return SUCCESS;
}

Status SwapMergeCastFusionPass::RelinkMergeNode(const ge::NodePtr &merge_node, const ge::NodePtr &cast_node,
                                                const ge::NodePtr &netout_node) {
  InDataAnchorPtr netout_in_data_anchor = cast_node->GetOutDataAnchor(0)->GetPeerInDataAnchors().at(0);
  cast_node->GetInDataAnchor(0)->UnlinkAll();
  cast_node->GetOutDataAnchor(0)->UnlinkAll();

  // if cast node has in control anchors, then link them to netoutputnode
  if (cast_node->GetInControlAnchor() != nullptr) {
    if (cast_node->GetInControlAnchor()->GetPeerOutControlAnchors().size() > 0 &&
        netout_node->GetInControlAnchor() != nullptr) {
      for (OutControlAnchorPtr out_control_anchor : cast_node->GetInControlAnchor()->GetPeerOutControlAnchors()) {
        if (GraphUtils::AddEdge(out_control_anchor, netout_node->GetInControlAnchor()) != GRAPH_SUCCESS) {
          REPORT_FE_ERROR(
              "[GraphOpt][SwapMrgCastFus][RelkMrgNd] Fail to link control edge between netoutput node[%s]"
              " and peer out control anchor of cast node[%s].",
              netout_node->GetName().c_str(), cast_node->GetName().c_str());
          return FAILED;
        }
      }
    }
    cast_node->GetInControlAnchor()->UnlinkAll();
  }

  // usually cast node do not have any output control anchor
  // if cast node has output control anchors, unlink them
  if (cast_node->GetOutControlAnchor() != nullptr) {
    cast_node->GetOutControlAnchor()->UnlinkAll();
  }

  if (GraphUtils::AddEdge(merge_node->GetOutDataAnchor(0), netout_in_data_anchor) != GRAPH_SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][SwapMrgCastFus][RelkMrgNd] Fail to link the output data anchor of merge node[%s].",
                    merge_node->GetName().c_str());
    return FAILED;
  }

  return SUCCESS;
}

Status SwapMergeCastFusionPass::VerifyNodes(const ge::NodePtr &merge_node,
                                            ge::NodePtr &cast_node, ge::NodePtr &netout_node) const {
  FE_CHECK(merge_node == nullptr, REPORT_FE_ERROR("[GraphOpt][SwapMrgCastFus][VerifyNd] Merge node is nullptr."),
           return PARAM_INVALID);

  FE_CHECK(cast_node == nullptr, REPORT_FE_ERROR("[GraphOpt][SwapMrgCastFus][VerifyNd] Cast node is nullptr."),
           return PARAM_INVALID);

  // merge node has two outputs, first output must has only one peer in anchor
  if (merge_node->GetOutDataAnchor(0)->GetPeerInDataAnchors().size() > 1) {
    FE_LOGD(
        "The first output anchor of Merge node[%s]"
        " has more than one peer in anchor.",
        merge_node->GetName().c_str());
    return NOT_CHANGED;
  }

  // cast node must have only one output node
  if (cast_node->GetOutDataNodesSize() != 1) {
    FE_LOGD("Cast node[%s] has more than one out data nodes.", cast_node->GetName().c_str());
    return NOT_CHANGED;
  }

  netout_node = cast_node->GetOutDataNodes().at(0);
  FE_CHECK_NOTNULL(netout_node);
  if (netout_node->GetType() != OP_TYPE_NETOUTPUT) {
    FE_LOGD("The next node of cast node[%s] is not NetOutput.", cast_node->GetName().c_str());
    return NOT_CHANGED;
  }

  return SUCCESS;
}

REGISTER_PASS(SWAPMERGECAST_PASS_NAME, SECOND_ROUND_BUILT_IN_GRAPH_PASS, SwapMergeCastFusionPass);
}  // namespace fe
