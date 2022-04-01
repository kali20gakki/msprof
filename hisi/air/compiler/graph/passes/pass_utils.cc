/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2021. All rights reserved.
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

#include "graph/passes/pass_utils.h"

#include <memory>
#include <queue>
#include <string>
#include <vector>

#include "framework/common/op/ge_op_utils.h"
#include "framework/common/types.h"
#include "common/omg_util.h"
#include "graph/manager/graph_var_manager.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/tensor_utils.h"
#include "graph/utils/node_utils.h"

namespace ge {
bool PassUtils::IsConstant(const ConstNodePtr &node) {
  if (node == nullptr) {
    REPORT_INNER_ERROR("E19999", "Param node is nullptr, check invalid");
    GELOGE(PARAM_INVALID, "[Check][Param] node is nullptr");
    return false;
  }

  auto src_node_type = node->GetType();
  bool is_constant = (src_node_type == CONSTANT) || (src_node_type == CONSTANTOP);
  return is_constant;
}

Status PassUtils::SetOutNodeWeight(const OutDataAnchorPtr &out_data_anchor, const NodePtr &src_node) {
  GE_IF_BOOL_EXEC(src_node == nullptr,
                  REPORT_INNER_ERROR("E19999", "Param src_node is nullptr, check invalid");
                  GELOGE(PARAM_INVALID, "[Check][Param] src_node is nullptr"); return PARAM_INVALID);
  if (!IsConstant(src_node)) {
    return SUCCESS;
  }

  auto weights = OpDescUtils::MutableWeights(src_node);
  if (weights.empty()) {
    REPORT_INNER_ERROR("E19999", "Weight of node:%s(%s) is empty, check invalid",
                       src_node->GetName().c_str(), src_node->GetType().c_str());
    GELOGE(PARAM_INVALID, "[Check][Param] Weight of node:%s(%s) is empty",
           src_node->GetName().c_str(), src_node->GetType().c_str());
    return PARAM_INVALID;
  }

  auto weight = weights.at(0);
  auto src_in_ctrl = src_node->GetInControlAnchor();
  if ((src_in_ctrl == nullptr) || (out_data_anchor == nullptr)) {
    REPORT_INNER_ERROR("E19999", "Param out_data_anchor or in control anchor in Param src_node:%s(%s) is nullptr, "
                       "check invalid", src_node->GetName().c_str(), src_node->GetType().c_str());
    GELOGE(FAILED, "[Check][Param] Param out_data_anchor or in control anchor in Param src_node:%s(%s) is nullptr",
           src_node->GetName().c_str(), src_node->GetType().c_str());
    return FAILED;
  }
  auto src_out_control_anchors = src_in_ctrl->GetPeerAnchors();
  for (const auto &dst_in_data : out_data_anchor->GetPeerInDataAnchors()) {
    auto dst_node = dst_in_data->GetOwnerNode();
    auto dst_op_desc = dst_node->GetOpDesc();
    if (dst_op_desc == nullptr) {
      continue;
    }

    std::vector<bool> is_input_const = dst_op_desc->GetIsInputConst();
    auto input_index = static_cast<size_t>(dst_in_data->GetIdx());
    if (input_index < is_input_const.size()) {
      is_input_const[input_index] = true;
      dst_op_desc->SetIsInputConst(is_input_const);
    }

    GE_CHK_GRAPH_STATUS_RET(GraphUtils::RemoveEdge(out_data_anchor, dst_in_data),
                            "[Remove][Edge] between %s and %s failed",
                            out_data_anchor->GetOwnerNode()->GetName().c_str(),
                            dst_in_data->GetOwnerNode()->GetName().c_str());
    graphStatus ret = OpDescUtils::AddConstOpToAnchor(dst_in_data, weight);
    if (ret != SUCCESS) {
      return ret;
    }
    GE_CHECK_NOTNULL(dst_in_data->GetPeerOutAnchor());
    auto dynamic_const_node = dst_in_data->GetPeerOutAnchor()->GetOwnerNode();
    GE_CHECK_NOTNULL(dynamic_const_node->GetOpDesc());
    dynamic_const_node->GetOpDesc()->SetType(src_node->GetType());

    // restore control inputs to dynamically added constant ops, if any
    for (const auto &src_out_control_anchor : src_out_control_anchors) {
      GE_CHK_GRAPH_STATUS_RET(GraphUtils::AddEdge(src_out_control_anchor, dynamic_const_node->GetInControlAnchor()),
                              "[Add][ControlEdge] between %s and %s failed",
                              src_out_control_anchor->GetOwnerNode()->GetName().c_str(),
                              dynamic_const_node->GetName().c_str());
    }
  }

  /// Before:
  /// Op1 - - - > Constant ------> Switch - - - > Op2
  /// After:
  /// Op1 - - - > Op2
  for (const auto &dst_in_ctrl : out_data_anchor->GetPeerInControlAnchors()) {
    for (const auto &src_out_control_anchor : src_out_control_anchors) {
      GE_CHK_GRAPH_STATUS_RET(GraphUtils::AddEdge(src_out_control_anchor, dst_in_ctrl),
                              "[Add][ControlEdge] between %s and %s failed",
                              src_out_control_anchor->GetOwnerNode()->GetName().c_str(),
                              dst_in_ctrl->GetOwnerNode()->GetName().c_str());
    }
  }

  return SUCCESS;
}

Status PassUtils::RemoveBranch(const NodePtr &node, std::vector<NodePtr> &delete_nodes,
                               std::vector<NodePtr> &end_nodes) {
  if (node == nullptr) {
    REPORT_INNER_ERROR("E19999", "Param node is nullptr, check invalid");
    GELOGE(FAILED, "[Check][Param] parameter node is nullptr.");
    return FAILED;
  }
  GELOGI("Remove branch starting from node %s", node->GetName().c_str());
  std::queue<NodePtr> search_queue;
  search_queue.push(node);

  while (!search_queue.empty()) {
    const NodePtr src_node = search_queue.front();
    if (src_node == nullptr) {
      continue;
    }
    delete_nodes.push_back(src_node);
    search_queue.pop();

    for (const auto &src_out_anchor : src_node->GetAllOutAnchors()) {
      for (const auto &dst_in_anchor : src_out_anchor->GetPeerAnchors()) {
        if (dst_in_anchor == nullptr) {
          continue;
        }
        auto dst_node = dst_in_anchor->GetOwnerNode();
        std::string node_type;
        GE_CHK_STATUS_RET(GetOriginalType(dst_node, node_type),
                          "[Get][OriginalType] of node:%s failed", dst_node->GetName().c_str());
        if (node_type == NETOUTPUT) {
          if (dst_in_anchor->IsTypeOf<InDataAnchor>()) {
            REPORT_INNER_ERROR("E19999", "Node:%s(%s) nactive branch connected to NetOutput with data anchor, "
                               "check invalid", node->GetName().c_str(), node->GetType().c_str());
            GELOGE(INTERNAL_ERROR, "[Check][Param] [%s] Inactive branch connected to NetOutput with data anchor.",
                   node->GetName().c_str());
            return INTERNAL_ERROR;
          } else {
            // safe to unlink control edges
            GE_CHK_GRAPH_STATUS_RET(GraphUtils::RemoveEdge(src_out_anchor, dst_in_anchor),
                                    "[Remove][Edge] between %s and %s failed",
                                    src_out_anchor->GetOwnerNode()->GetName().c_str(),
                                    dst_in_anchor->GetOwnerNode()->GetName().c_str());
            end_nodes.push_back(dst_node);
          }
        } else if ((node_type == MERGE) || (node_type == REFMERGE)) {
          /// Unlink connection between the inactive branch and Merge/NetOutput.
          /// The removal of inactive nodes will be handled in PrunePass
          GE_CHK_GRAPH_STATUS_RET(GraphUtils::RemoveEdge(src_out_anchor, dst_in_anchor),
                                  "[Remove][Edge] between %s and %s failed",
                                  src_out_anchor->GetOwnerNode()->GetName().c_str(),
                                  dst_in_anchor->GetOwnerNode()->GetName().c_str());
          end_nodes.push_back(dst_node);
          GELOGD("Reach the end merge node %s, the branch removing stop", dst_node->GetName().c_str());
        } else {
          search_queue.push(dst_node);
        }
      }
    }
  }

  return SUCCESS;
}

NodePtr PassUtils::GetInDataNode(const ConstNodePtr &node, int32_t index) {
  if (node == nullptr) {
    return nullptr;
  }

  auto in_data_anchor = node->GetInDataAnchor(index);
  if (in_data_anchor == nullptr) {
    return nullptr;
  }

  auto peer_out_data_anchor = in_data_anchor->GetPeerOutAnchor();
  if (peer_out_data_anchor == nullptr) {
    return nullptr;
  }

  auto src_node = peer_out_data_anchor->GetOwnerNode();
  return src_node;
}

NodePtr PassUtils::GetInNodeCrossSubgraphByIndex(const ConstNodePtr &node, int32_t index) {
  auto src_node = GetInDataNode(node, index);

  return NodeUtils::GetInNodeCrossSubgraph(src_node);
}

bool PassUtils::IsNeedTrainIteFlowCtrl(const ComputeGraphPtr &compute_graph) {
  if (compute_graph == nullptr) {
    return false;
  }
  if (compute_graph->GetParentGraph() != nullptr) {
    GELOGI("Subgraph %s no need flow ctrl.", compute_graph->GetName().c_str());
    return false;
  }
  if (GraphUtils::IsUnknownShapeGraph(compute_graph)) {
    GELOGI("Unknown shape graph %s no need flow ctrl.", compute_graph->GetName().c_str());
    return false;
  }
  if (!ge::VarManager::Instance(compute_graph->GetSessionID())->IsVarExist(NODE_NAME_FLOWCTRL_LOOP_PER_ITER)) {
    return false;
  }
  return compute_graph->GetNeedIteration();
}

int32_t PassUtils::GetUniqueInDataAnchorIndex(const NodePtr &node_ptr) {
  const int32_t invalid_index = -1;
  if (node_ptr == nullptr) {
    REPORT_INNER_ERROR("E19999", "Param node_ptr is nullptr, check invalid");
    GELOGE(INTERNAL_ERROR, "[Check][Param] node is nullptr");
    return invalid_index;
  }
  for (const auto &in_anchor : node_ptr->GetAllInDataAnchors()) {
    if ((in_anchor->GetPeerOutAnchor() != nullptr)) {
      return (in_anchor->GetIdx());
    }
  }

  REPORT_INNER_ERROR("E19999", "Failed to find in data anchor of node:%s(%s) with a valid peer out node",
                     node_ptr->GetName().c_str(), node_ptr->GetType().c_str());
  GELOGE(INTERNAL_ERROR, "[Check][Param] Failed to find in data anchor of node:%s(%s) with a valid peer out node",
         node_ptr->GetName().c_str(), node_ptr->GetType().c_str());
  return invalid_index;
}

Status PassUtils::UnlinkNodeWithControlCopy(NodePtr &node, int32_t index) {
  if (node == nullptr) {
    REPORT_INNER_ERROR("E19999", "Param node is nullptr, check invalid");
    GELOGE(PARAM_INVALID, "[Check][Param] node is nullptr.");
    return PARAM_INVALID;
  }
  auto in_data_anchor = node->GetInDataAnchor(index);
  if (in_data_anchor == nullptr) {
    GELOGW("[%s] in_data_anchor is null with index [%d].", node->GetName().c_str(), index);
    return SUCCESS;
  }
  auto out_data_anchor = in_data_anchor->GetPeerOutAnchor();
  if (out_data_anchor == nullptr) {
    REPORT_INNER_ERROR("E19999", "Index:%d in data anchor of node:%s(%s), its peer anchor is nullptr, check invalid",
                       index, node->GetName().c_str(), node->GetType().c_str());
    GELOGE(FAILED, "[Get][PeerOutAnchor] failed, Index:%d in data anchor of node:%s(%s), its peer anchor is nullptr.",
           index, node->GetName().c_str(), node->GetType().c_str());
    return FAILED;
  }
  // Remove link between father_node and node
  in_data_anchor->UnlinkAll();

  auto father_node = out_data_anchor->GetOwnerNode();
  // link father_node's in control nodes to node
  if (GraphUtils::CopyInCtrlEdges(father_node, node) != GRAPH_SUCCESS) {
    REPORT_CALL_ERROR("E19999", "Copy in control edge from node:%s(%s) to node:%s(%s) failed",
                      father_node->GetName().c_str(), father_node->GetType().c_str(),
                      node->GetName().c_str(), node->GetType().c_str());
    GELOGE(FAILED, "[Copy][InCtrlEdges] from node:%s(%s) to node:%s(%s) failed",
           father_node->GetName().c_str(), father_node->GetType().c_str(),
           node->GetName().c_str(), node->GetType().c_str());
    return FAILED;
  }
  return SUCCESS;
}

Status PassUtils::RemoveInactiveBranchToMerge(const OutDataAnchorPtr &inactive_output_anchor,
                                              std::vector<NodePtr> &delete_nodes, std::vector<NodePtr> &end_nodes) {
  if (inactive_output_anchor == nullptr) {
    REPORT_INNER_ERROR("E19999", "Param inactive_output_anchor is nullptr, check invalid");
    GELOGE(FAILED, "[Check][Param] parameter inactive_output_anchor is nullptr.");
    return FAILED;
  }
  for (const auto &dst_anchor : inactive_output_anchor->GetPeerAnchors()) {
    auto dst_node = dst_anchor->GetOwnerNode();
    std::string dst_node_type;
    GE_CHK_STATUS_RET(GetOriginalType(dst_node, dst_node_type),
                      "[Get][OriginalType] of node:%s failed", dst_node->GetName().c_str());
    if ((dst_node_type == MERGE) || (dst_node_type == REFMERGE)) {
      GELOGD("[%s] Switch connected directly to Merge", inactive_output_anchor->GetOwnerNode()->GetName().c_str());
      GE_CHK_GRAPH_STATUS_RET(GraphUtils::RemoveEdge(inactive_output_anchor, dst_anchor),
                              "[Remove][Edge] between %s and %s failed",
                              inactive_output_anchor->GetOwnerNode()->GetName().c_str(),
                              dst_node->GetName().c_str());
      end_nodes.emplace_back(dst_node);
      continue;
    }

    Status ret = PassUtils::RemoveBranch(dst_node, delete_nodes, end_nodes);
    if (ret != SUCCESS) {
      return ret;
    }
  }
  return SUCCESS;
}
}  // namespace ge
