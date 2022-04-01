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

#include "graph/passes/create_subgraph_with_scope_pass.h"

#include "graph/utils/node_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/tensor_utils.h"
#include "common/plugin/ge_util.h"
#include "register/op_registry.h"
#include "ir_build/option_utils.h"

namespace ge {
Status CreateSubGraphWithScopePass::Run(ComputeGraphPtr graph) {
  GELOGD("CreateSubGraphWithScopePass start.");

  GE_CHECK_NOTNULL(graph);
  if (graph->GetParentGraph() != nullptr) {
    return SUCCESS;
  }

  // add nodes to scope list by index
  GE_CHK_STATUS_RET(CollectScopeNodesByIndex(graph), "Collect scope nodes failed.");

  for (const auto &it : scopes_) {
    const int32_t scope_index = it.first;
    const std::vector<NodePtr> &scope_nodes = it.second;
    ComputeGraphPtr subgraph;

    GE_CHK_STATUS_RET(ParseMultiDimsAttr(scope_nodes),
                      "Parse dynamic dims attribute failed for scope[%d].", scope_index);
    GE_CHK_STATUS_RET(GetSubgraphFromScope(graph, it, subgraph),
                      "Get subgraph From scope[%d] failed.", scope_index);
    GE_CHK_STATUS_RET(CollectIoNodes(subgraph), "Collect Io nodes from subgraph[%s] failed.",
                      subgraph->GetName().c_str());
    GE_CHK_STATUS_RET(CreateNewPartitionedCall(graph, subgraph, scope_nodes),
                      "Create new partitioned call failed.");
    GE_CHK_STATUS_RET(MergeNodesToSubgraph(scope_nodes, subgraph), "Merge scope nodes to subgraph[%s] failed.",
                      subgraph->GetName().c_str());
    GE_CHK_STATUS_RET(SetParentIndexToData(subgraph), "Set parent index to data nodes for subgraph[%s] failed.",
                      subgraph->GetName().c_str());
    GE_CHK_STATUS_RET(SetParentIndexToNetOutput(subgraph), "Set parent index to net_output for subgraph[%s] failed.",
                      subgraph->GetName().c_str());
    GE_CHK_STATUS_RET(AddSubgraphToNewPartitionedCall(graph, subgraph),
                      "Add subgraph to new partitioned_call failed.");

    (void)AttrUtils::SetBool(subgraph, ATTR_NAME_SUBGRAPH_IS_MULTI_DIMS, true);
    GELOGI("Create multi dims original subgraph[%s] with scope[%d] success.",
           subgraph->GetName().c_str(), scope_index);
  }

  GELOGD("CreateSubGraphWithScopePass end.");
  return SUCCESS;
}

Status CreateSubGraphWithScopePass::CollectScopeNodesByIndex(const ComputeGraphPtr &graph) {
  for (const auto &node : graph->GetDirectNode()) {
    const auto &op_desc = node->GetOpDesc();
    GE_CHECK_NOTNULL(op_desc);
    int32_t node_index = -1;
    if (AttrUtils::GetInt(op_desc, ATTR_NAME_SUBGRAPH_MULTI_DIMS_INDEX, node_index) && node_index >= 0) {
      if (node->GetType() == DATA || node->GetType() == NETOUTPUT) {
        REPORT_INNER_ERROR("E19999", "Not support DATA/NETOUTPUT node[%s] in scope.", node->GetName().c_str());
        GELOGE(PARAM_INVALID, "Not support DATA/NETOUTPUT node[%s] in scope.", node->GetName().c_str());
        return PARAM_INVALID;
      }
      scopes_[node_index].push_back(node);
      GELOGD("Get node[%s] scope index[%d].", node->GetName().c_str(), node_index);
    }
  }
  return SUCCESS;
}

Status CreateSubGraphWithScopePass::ParseMultiDimsAttr(const std::vector<NodePtr> &scope_nodes) {
  for (const auto &node : scope_nodes) {
    const auto &op_desc = node->GetOpDesc();
    GE_CHECK_NOTNULL(op_desc);
    std::string input_shape;
    std::string multi_dims;
    // example: input shape: "0:1,1,-1,224; 1:-1,-1,224,224"
    //          multi dims: "112,1,1; 224,2,2; 448,4,4"
    if (!AttrUtils::GetStr(op_desc, ATTR_NAME_SUBGRAPH_MULTI_DIMS_INPUT_SHAPE, input_shape) ||
        !AttrUtils::GetStr(op_desc, ATTR_NAME_SUBGRAPH_MULTI_DIMS_INPUT_DIMS, multi_dims)) {
      continue;
    }
    GELOGI("Input node[%s] has dynamic dims attr, input shape[%s], input multi dims[%s].",
           node->GetName().c_str(), input_shape.c_str(), multi_dims.c_str());
    std::map<int64_t, std::vector<int64_t>> shape_vec;
    for (const auto &it : ge::StringUtils::Split(input_shape, ';')) {
      std::vector<std::string> tensor_shape = ge::StringUtils::Split(it, ':');
      int64_t index = std::strtol(tensor_shape[0U].c_str(), nullptr, 10); // decimal
      GE_RT_PARAM_INVALID_WITH_LOG_IF_TRUE(index >= static_cast<int64_t>(op_desc->GetInputsSize()),
                                           "Invalid input shape for node[%s], input size[%zu], index[%ld]",
                                           op_desc->GetName().c_str(), op_desc->GetInputsSize(), index);
      std::vector<int64_t> shape;
      for (const auto &dim : ge::StringUtils::Split(tensor_shape[1], ',')) {
        shape.push_back(std::strtol(dim.c_str(), nullptr, 10));
      }
      shape_vec[index] = std::move(shape);
    }
    node_to_input_shape_[node] = shape_vec;
    std::vector<std::vector<int64_t>> parsed_dims;
    for (const auto &it : ge::StringUtils::Split(multi_dims, ';')) {
      std::vector<int64_t> dims;
      for (const auto &grade : ge::StringUtils::Split(it, ',')) {
        dims.push_back(std::strtol(grade.c_str(), nullptr, 10));
      }
      parsed_dims.push_back(dims);
    }
    std::map<int64_t, std::vector<std::vector<int64_t>>> dims_map;
    for (const auto &parsed_dim : parsed_dims) {
      size_t negative = 0U;
      for (const auto &shape : shape_vec) {
        std::vector<int64_t> dims;
        for (const auto &dim : shape.second) {
          if (dim == ge::UNKNOWN_DIM) {
            dims.push_back(parsed_dim[negative]);
            negative++;
          } else {
            dims.push_back(dim);
          }
        }
        if (dims_map.find(shape.first) == dims_map.end()) {
          dims_map[shape.first] = {dims};
        } else {
          dims_map.at(shape.first).push_back(dims);
        }
      }
    }
    node_to_multi_dims_[node] = std::move(dims_map);
  }
  return SUCCESS;
}

Status CreateSubGraphWithScopePass::AddSubgraphToNewPartitionedCall(const ComputeGraphPtr &root_graph,
                                                                    const ComputeGraphPtr &subgraph) {
  const auto &parent_node = subgraph->GetParentNode();
  GE_CHECK_NOTNULL(parent_node);
  const auto &parent_desc = parent_node->GetOpDesc();
  subgraph->SetParentNode(new_partitioned_call_);
  const auto &op_desc = new_partitioned_call_->GetOpDesc();
  GE_CHECK_NOTNULL(op_desc);
  op_desc->CopyAttrsFrom(*parent_desc);

  GE_CHK_STATUS_RET(root_graph->RemoveNode(parent_node), "Remove node[%s] failed.", parent_node->GetName().c_str());
  GE_CHK_STATUS_RET(op_desc->AddSubgraphName(subgraph->GetName()), "Add subgraph[%s] to partitioned_call[%s] failed.",
                    subgraph->GetName().c_str(), op_desc->GetName().c_str());
  GE_CHK_STATUS_RET(op_desc->SetSubgraphInstanceName(0U, subgraph->GetName()),
                    "Add subgraph[%s] to partitioned_call[%s] failed.",
                    subgraph->GetName().c_str(), op_desc->GetName().c_str());
  return SUCCESS;
}

Status CreateSubGraphWithScopePass::CollectIoNodes(const ComputeGraphPtr &subgraph) {
  const auto &parent_node = subgraph->GetParentNode();
  GE_CHECK_NOTNULL(parent_node);

  anchor_to_data_nodes_.clear();
  for (const auto &node : subgraph->GetDirectNode()) {
    if (node->GetType() == DATA) {
      int32_t parent_index = -1;
      if (!AttrUtils::GetInt(node->GetOpDesc(), ATTR_NAME_PARENT_NODE_INDEX, parent_index) || parent_index < 0) {
        REPORT_INNER_ERROR("E19999", "Data node[%s] has no parent index.", node->GetName().c_str());
        GELOGE(PARAM_INVALID, "Data node[%s] has no parent index.", node->GetName().c_str());
        return PARAM_INVALID;
      }

      const auto &anchor = parent_node->GetInDataAnchor(parent_index);
      GE_CHECK_NOTNULL(anchor);
      anchor_to_data_nodes_[anchor] = node;
      GELOGD("Add anchor[%d] and node[%s] to anchor_to_data_nodes_", parent_index, node->GetName().c_str());
    }
  }
  anchor_to_output_.clear();
  const auto &output = subgraph->FindFirstNodeMatchType(NETOUTPUT);
  if (output == nullptr) {
    return SUCCESS;
  }
  const auto &op_desc = output->GetOpDesc();
  GE_CHECK_NOTNULL(op_desc);
  for (const auto &input_anchor : output->GetAllInDataAnchors()) {
    int32_t parent_index = -1;
    const auto &tensor = op_desc->GetInputDescPtr(input_anchor->GetIdx());
    if (!AttrUtils::GetInt(tensor, ATTR_NAME_PARENT_NODE_INDEX, parent_index) || parent_index < 0) {
      REPORT_INNER_ERROR("E19999", "NetOutput[%s] tensor[%d] has no parent index.",
                         output->GetName().c_str(), input_anchor->GetIdx());
      GELOGE(PARAM_INVALID, "NetOutput[%s] tensor[%d] has no parent index.",
             output->GetName().c_str(), input_anchor->GetIdx());
      return PARAM_INVALID;
    }

    const auto &anchor = parent_node->GetOutDataAnchor(parent_index);
    GE_CHECK_NOTNULL(anchor);
    anchor_to_output_[anchor] = input_anchor;
    GELOGD("Add anchor[%d] and netoutput anchor[%d] to anchor_to_output_", parent_index, input_anchor->GetIdx());
  }

  return SUCCESS;
}

Status CreateSubGraphWithScopePass::GetSubgraphFromScope(const ComputeGraphPtr &root_graph,
                                                         const std::pair<const int32_t, std::vector<NodePtr>> &scope,
                                                         ComputeGraphPtr &subgraph) const {
  const int32_t scope_index = scope.first;
  for (const auto &node : scope.second) {
    if (node->GetType() == PARTITIONEDCALL) {
      const auto &op_desc = node->GetOpDesc();
      GE_CHECK_NOTNULL(op_desc);
      subgraph = root_graph->GetSubgraph(op_desc->GetSubgraphInstanceName(0U));
      GE_CHECK_NOTNULL(subgraph);
      GELOGD("Get subgraph:%s from scope[%d].", subgraph->GetName().c_str(), scope_index);
      return SUCCESS;
    }
  }

  // no partitioned_call in scope, create new subgraph
  subgraph = MakeShared<ComputeGraph>("scope_subgraph_" + std::to_string(scope_index));
  GE_CHECK_NOTNULL(subgraph);
  const auto op_desc = MakeShared<OpDesc>("scope_partitioned_call_" + std::to_string(scope_index), PARTITIONEDCALL);
  GE_CHECK_NOTNULL(op_desc);
  const NodePtr partition_op = root_graph->AddNode(op_desc);
  (void)op_desc->AddSubgraphName(subgraph->GetName());
  (void)op_desc->SetSubgraphInstanceName(0U, subgraph->GetName());
  subgraph->SetParentNode(partition_op);
  subgraph->SetParentGraph(root_graph);
  (void)root_graph->AddSubGraph(subgraph);
  std::string session_graph_id;
  (void)AttrUtils::GetStr(root_graph, ATTR_NAME_SESSION_GRAPH_ID, session_graph_id);
  (void)AttrUtils::SetStr(subgraph, ATTR_NAME_SESSION_GRAPH_ID, session_graph_id);
  GELOGD("Scope[%d] has no subgraph, create new[%s].", scope_index, subgraph->GetName().c_str());

  return SUCCESS;
}

Status CreateSubGraphWithScopePass::MergeNodesToSubgraph(const std::vector<NodePtr> &scope_nodes,
                                                         const ComputeGraphPtr &subgraph) {
  const auto &partitioned_call = subgraph->GetParentNode();
  GE_CHECK_NOTNULL(partitioned_call);
  const auto &op_desc = partitioned_call->GetOpDesc();
  GE_CHECK_NOTNULL(op_desc);

  if ((scope_nodes.size() == 1U) && (scope_nodes[0U] == partitioned_call)) {
    GELOGI("Scope has only one node[%s] type[%s], copy attrs to data nodes.",
           partitioned_call->GetName().c_str(), partitioned_call->GetType().c_str());
    return CopyPartitionedCallAttrToData(subgraph);
  }

  for (const auto &node : scope_nodes) {
    // skip partitioned_call it self
    if (node == partitioned_call) {
      continue;
    }
    // if node has control anchor outside scope, return failed
    GE_CHK_STATUS_RET(CheckCtrlAnchorInvalid(node, scope_nodes),
                      "[CHECK]Not support control edge cross scope, error node[%s].", node->GetName().c_str());
    const auto &parent_graph = node->GetOwnerComputeGraph();
    if (GraphUtils::RemoveJustNode(parent_graph, node) != GRAPH_SUCCESS) {
      REPORT_INNER_ERROR("E19999", "Remove node[%s] from root graph[%s] failed.",
                         node->GetName().c_str(), parent_graph->GetName().c_str());
      GELOGE(FAILED, "Remove node[%s] from root graph[%s] failed.",
             node->GetName().c_str(), parent_graph->GetName().c_str());
      return FAILED;
    }
    (void)subgraph->AddNode(node);
    if (node->SetOwnerComputeGraph(subgraph) != GRAPH_SUCCESS) {
      REPORT_INNER_ERROR("E19999", "Set owner graph[%s] to node[%s] failed.",
                         subgraph->GetName().c_str(), node->GetName().c_str());
      GELOGE(FAILED, "Set owner graph[%s] to node[%s] failed.", subgraph->GetName().c_str(), node->GetName().c_str());
      return FAILED;
    }
    GE_CHK_STATUS_RET(MergeInputAnchors(subgraph, node, scope_nodes),
                      "Merge node[%s]'s input anchors to subgraph[%s] failed.",
                      node->GetName().c_str(), subgraph->GetName().c_str());
    GE_CHK_STATUS_RET(MergeOutputAnchors(subgraph, node, scope_nodes),
                      "Merge node[%s]'s output anchors to subgraph[%s] failed.",
                      node->GetName().c_str(), subgraph->GetName().c_str());
    GELOGI("Node[%s] has been moved to subgraph[%s].", node->GetName().c_str(), subgraph->GetName().c_str());
  }
  return SUCCESS;
}

Status CreateSubGraphWithScopePass::CopyPartitionedCallAttrToData(const ComputeGraphPtr &subgraph) {
  const auto &partitioned_call = subgraph->GetParentNode();
  GE_CHECK_NOTNULL(partitioned_call);

  const std::map<NodePtr, std::map<int64_t, std::vector<int64_t>>>::const_iterator &input_shape =
    node_to_input_shape_.find(partitioned_call);
  if (input_shape == node_to_input_shape_.cend()) {
    REPORT_INNER_ERROR("E19999", "No input shape for the only node[%s] in scope.",
                       partitioned_call->GetName().c_str());
    GELOGE(PARAM_INVALID, "No input shape for the only node[%s] in scope.", partitioned_call->GetName().c_str());
    return PARAM_INVALID;
  }

  const std::map<NodePtr, std::map<int64_t, std::vector<std::vector<int64_t>>>>::const_iterator &dyn_dims =
    node_to_multi_dims_.find(partitioned_call);
  if (dyn_dims == node_to_multi_dims_.cend()) {
    REPORT_INNER_ERROR("E19999", "No dynamic dims for the only node[%s] in scope.",
                       partitioned_call->GetName().c_str());
    GELOGE(PARAM_INVALID, "No dynamic dims for the only node[%s] in scope.", partitioned_call->GetName().c_str());
    return PARAM_INVALID;
  }

  for (const auto &node : subgraph->GetDirectNode()) {
    if (node->GetType() != DATA) {
      continue;
    }
    const auto &op_desc = node->GetOpDesc();
    GE_CHECK_NOTNULL(op_desc);
    int32_t parent_index = -1;
    if (!AttrUtils::GetInt(op_desc, ATTR_NAME_PARENT_NODE_INDEX, parent_index) || parent_index < 0) {
      REPORT_INNER_ERROR("E19999", "Invalid parent index for data node[%s].", node->GetName().c_str());
      GELOGE(PARAM_INVALID, "Invalid parent index for data node[%s].", node->GetName().c_str());
      return PARAM_INVALID;
    }
    const std::map<int64_t, std::vector<int64_t>>::const_iterator &shape = input_shape->second.find(parent_index);
    if (shape != input_shape->second.cend()) {
      auto tensor = op_desc->MutableInputDesc(0U);
      GE_CHECK_NOTNULL(tensor);
      tensor->SetShape(GeShape(shape->second));
      tensor->SetOriginShape(GeShape(shape->second));
    }
    const auto &dims = dyn_dims->second.find(parent_index);
    if (dims != dyn_dims->second.end()) {
      std::vector<int64_t> merged_dims;
      for (const auto &it : dims->second) {
        for (const auto &dim : it) {
          merged_dims.push_back(dim);
        }
      }
      (void)AttrUtils::SetListInt(op_desc, ATTR_NAME_OP_MULTI_DIMS_INPUT_DIMS, merged_dims);
    }
  }
  return SUCCESS;
}

Status CreateSubGraphWithScopePass::CheckCtrlAnchorInvalid(const NodePtr &node,
                                                           const std::vector<NodePtr> &scope_nodes) const {
  GE_CHECK_NOTNULL(node);
  const auto &in_ctrl_anchor = node->GetInControlAnchor();
  if (in_ctrl_anchor != nullptr) {
    for (const auto &peer_anchor : in_ctrl_anchor->GetPeerOutControlAnchors()) {
      if (peer_anchor == nullptr) {
        continue;
      }
      const auto &peer_node = peer_anchor->GetOwnerNode();
      if (peer_node == nullptr) {
        continue;
      }
      const auto &it = std::find(scope_nodes.begin(), scope_nodes.end(), peer_node);
      if (it == scope_nodes.end()) {
        REPORT_INNER_ERROR("E19999", "Exit control edge bwtween [%s] and [%s].",
                           peer_node->GetName().c_str(), node->GetName().c_str());
        GELOGE(PARAM_INVALID, "Exit control edge bwtween [%s] and [%s].",
               peer_node->GetName().c_str(), node->GetName().c_str());
        return FAILED;
      }
    }
  }
  const auto &out_ctrl_anchor = node->GetOutControlAnchor();
  if (out_ctrl_anchor != nullptr) {
    for (const auto &peer_anchor : out_ctrl_anchor->GetPeerInControlAnchors()) {
      if (peer_anchor == nullptr) {
        continue;
      }
      const auto &peer_node = peer_anchor->GetOwnerNode();
      if (peer_node == nullptr) {
        continue;
      }
      const auto &it = std::find(scope_nodes.begin(), scope_nodes.end(), peer_node);
      if (it == scope_nodes.end()) {
        REPORT_INNER_ERROR("E19999", "Exist control edge bwtween [%s] and [%s].",
                           node->GetName().c_str(), peer_node->GetName().c_str());
        GELOGE(PARAM_INVALID, "Exist control edge bwtween [%s] and [%s].",
               node->GetName().c_str(), peer_node->GetName().c_str());
        return FAILED;
      }
    }
  }
  return SUCCESS;
}

Status CreateSubGraphWithScopePass::MergeInputAnchors(const ComputeGraphPtr &subgraph, const NodePtr &node,
                                                      const std::vector<NodePtr> &scope_nodes) {
  for (const auto input_anchor : node->GetAllInDataAnchors()) {
    if (input_anchor == nullptr) {
      continue;
    }
    const auto peer_anchor = input_anchor->GetPeerOutAnchor();
    if (peer_anchor == nullptr) {
      continue;
    }
    const auto &peer_node = peer_anchor->GetOwnerNode();
    GE_CHECK_NOTNULL(peer_node);

    if (peer_node == subgraph->GetParentNode()) {
      // proc node linked with partitioned call
      return ProcPartitionOutputAnchor(peer_anchor, input_anchor);
    }

    const auto &it = std::find(scope_nodes.begin(), scope_nodes.end(), peer_node);
    if (it != scope_nodes.end()) {
      GELOGD("Peer node[%s] is also in scope, no need merge anchors.", peer_node->GetName().c_str());
      continue;
    }

    const size_t parent_index = new_partitioned_call_->GetAllInDataAnchorsSize();
    const OpDescPtr data_desc =
      MakeShared<OpDesc>(subgraph->GetName() + "_data_" + std::to_string(parent_index), DATA);
    GE_CHECK_NOTNULL(data_desc);
    const auto &peer_desc = peer_node->GetOpDesc();
    GE_CHECK_NOTNULL(peer_desc);
    GE_CHK_STATUS_RET(data_desc->AddInputDesc(peer_desc->GetOutputDesc(peer_anchor->GetIdx())),
                      "Add input desc fail");
    GE_CHK_STATUS_RET(data_desc->AddOutputDesc(peer_desc->GetOutputDesc(peer_anchor->GetIdx())),
                      "Add output desc fail");

    const auto data_node = subgraph->AddNode(data_desc);
    GE_CHECK_NOTNULL(data_node);
    GE_CHK_STATUS_RET(SetDataDynDimsInfo(node, data_node, input_anchor->GetIdx()),
                      "Set dyn dims info from node[%s] input[%d] to data[%s] failed.",
                      node->GetName().c_str(), input_anchor->GetIdx(), data_node->GetName().c_str());
    GELOGI("Peer node[%s] is out of scope, add new data node[%s] into subgraph[%s].",
           peer_node->GetName().c_str(), data_node->GetName().c_str(), subgraph->GetName().c_str());

    if (GraphUtils::RemoveEdge(peer_anchor, input_anchor) != SUCCESS) {
      REPORT_INNER_ERROR("E19999", "Remove edge from node[%s] output[%d] to node[%s] input[%d] failed.",
                         peer_node->GetName().c_str(), peer_anchor->GetIdx(),
                         node->GetName().c_str(), input_anchor->GetIdx());
      GELOGE(FAILED, "Remove edge from node[%s] output [%d] to node[%s] input[%d] failed.",
             peer_node->GetName().c_str(), peer_anchor->GetIdx(), node->GetName().c_str(), input_anchor->GetIdx());
      return FAILED;
    }
    const auto &op_desc = new_partitioned_call_->GetOpDesc();
    GE_CHECK_NOTNULL(op_desc);
    GE_CHK_STATUS_RET(op_desc->AddInputDesc(peer_desc->GetOutputDesc(peer_anchor->GetIdx())),
                      "Add input desc for [%s] failed.", op_desc->GetName().c_str());
    (void)NodeUtils::AppendInputAnchor(new_partitioned_call_, parent_index + 1);
    auto new_anchor = new_partitioned_call_->GetInDataAnchor(parent_index);
    GE_CHECK_NOTNULL(new_anchor);
    if (GraphUtils::AddEdge(peer_anchor, new_anchor) != SUCCESS) {
      REPORT_INNER_ERROR("E19999", "Add edge from node[%s] output[%d] to node[%s] input[%d] failed.",
                         peer_node->GetName().c_str(), peer_anchor->GetIdx(),
                         new_partitioned_call_->GetName().c_str(), new_anchor->GetIdx());
      GELOGE(FAILED, "Add edge from node[%s] output [%d] to node[%s] input[%d] failed.",
             peer_node->GetName().c_str(), peer_anchor->GetIdx(),
             new_partitioned_call_->GetName().c_str(), new_anchor->GetIdx());
      return FAILED;
    }
    if (GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), input_anchor) != SUCCESS) {
      REPORT_INNER_ERROR("E19999", "Add edge from node[%s] output[0] to node[%s] input[%d] failed.",
                         data_node->GetName().c_str(), node->GetName().c_str(), input_anchor->GetIdx());
      GELOGE(FAILED, "Add edge from node[%s] output[0] to node[%s] input[%d] failed.",
             data_node->GetName().c_str(), node->GetName().c_str(), input_anchor->GetIdx());
      return FAILED;
    }
    anchor_to_data_nodes_[new_anchor] = data_node;
  }

  return SUCCESS;
}

Status CreateSubGraphWithScopePass::SetDataDynDimsInfo(const NodePtr &node, const NodePtr &data_node,
                                                       const int32_t input_index) {
  const std::map<NodePtr, std::map<int64_t, std::vector<int64_t>>>::const_iterator &input_shape =
    node_to_input_shape_.find(node);
  if (input_shape == node_to_input_shape_.cend()) {
    return SUCCESS;
  }
  const std::map<int64_t, std::vector<int64_t>>::const_iterator &shape = input_shape->second.find(input_index);
  if (shape == input_shape->second.cend()) {
    return SUCCESS;
  }
  const auto &data_desc = data_node->GetOpDesc();
  GE_CHECK_NOTNULL(data_desc);
  const auto &tensor_desc = data_desc->MutableInputDesc(0U);
  GE_CHECK_NOTNULL(tensor_desc);
  tensor_desc->SetShape(GeShape(shape->second));
  tensor_desc->SetOriginShape(GeShape(shape->second));

  const std::map<NodePtr, std::map<int64_t, std::vector<std::vector<int64_t>>>>::const_iterator &multi_dims =
    node_to_multi_dims_.find(node);
  if (multi_dims == node_to_multi_dims_.end()) {
    REPORT_INNER_ERROR("E19999", "Input node[%s] has input_shape but no multi_dims.", node->GetName().c_str());
    GELOGE(FAILED, "Input node[%s] has input_shape but no multi_dims.", node->GetName().c_str());
    return FAILED;
  }
  const auto &dims = multi_dims->second.find(input_index);
  if (dims == multi_dims->second.end()) {
    REPORT_INNER_ERROR("E19999", "Node[%s] input[%d] has input_shape but no multi_dims.",
                       node->GetName().c_str(), input_index);
    GELOGE(FAILED, "Node[%s] input[%d] has input_shape but no multi_dims.",
           node->GetName().c_str(), input_index);
    return FAILED;
  }
  std::vector<int64_t> merged_dims;
  for (const auto &it : dims->second) {
    for (const auto &dim : it) {
      merged_dims.push_back(dim);
    }
  }
  (void)AttrUtils::SetListInt(data_desc, ATTR_NAME_OP_MULTI_DIMS_INPUT_DIMS, merged_dims);
  GELOGI("Set multi dims to data node[%s] from [%s].", node->GetName().c_str(), data_node->GetName().c_str());
  return SUCCESS;
}

Status CreateSubGraphWithScopePass::MergeOutputAnchors(const ComputeGraphPtr &subgraph, const NodePtr &node,
                                                       const std::vector<NodePtr> &scope_nodes) {
  auto node_output = subgraph->FindFirstNodeMatchType(NETOUTPUT);
  if (node_output == nullptr) {
    const OpDescPtr new_desc = MakeShared<OpDesc>(subgraph->GetName() + "_node_output", NETOUTPUT);
    GE_CHECK_NOTNULL(new_desc);
    node_output = subgraph->AddNode(new_desc);
    GE_CHECK_NOTNULL(node_output);
  }
  const auto &node_output_desc = node_output->GetOpDesc();
  GE_CHECK_NOTNULL(node_output_desc);
  const auto &partition_desc = new_partitioned_call_->GetOpDesc();
  GE_CHECK_NOTNULL(partition_desc);
  const auto &node_desc = node->GetOpDesc();
  GE_CHECK_NOTNULL(node_desc);

  for (auto output_anchor : node->GetAllOutDataAnchors()) {
    for (auto peer_anchor : output_anchor->GetPeerInDataAnchors()) {
      if (peer_anchor == nullptr) {
        continue;
      }
      const auto &peer_node = peer_anchor->GetOwnerNode();
      GE_CHECK_NOTNULL(peer_node);

      if (peer_node == subgraph->GetParentNode()) {
      // proc node linked with partitioned call
        return ProcPartitionInputAnchor(subgraph, output_anchor, peer_anchor);
      }

      const auto &it = std::find(scope_nodes.begin(), scope_nodes.end(), peer_node);
      if (it != scope_nodes.end()) {
        GELOGD("Peer node[%s] is also in scope, no need merge.", peer_node->GetName().c_str());
        continue;
      }

      GELOGI("Peer node[%s] is out of scope, add output edge into subgraph[%s].",
             peer_node->GetName().c_str(), subgraph->GetName().c_str());
      if (GraphUtils::RemoveEdge(output_anchor, peer_anchor) != SUCCESS) {
        REPORT_INNER_ERROR("E19999", "Remove edge from node[%s] output[%d] to node[%s] input[%d] failed.",
                           node->GetName().c_str(), output_anchor->GetIdx(),
                           peer_node->GetName().c_str(), peer_anchor->GetIdx());
        GELOGE(FAILED, "Remove edge from node[%s] output[%d] to node[%s] input[%d] failed.",
               node->GetName().c_str(), output_anchor->GetIdx(), peer_node->GetName().c_str(), peer_anchor->GetIdx());
        return FAILED;
      }

      const size_t parent_index = new_partitioned_call_->GetAllOutDataAnchorsSize();
      const auto &peer_desc = peer_node->GetOpDesc();
      GE_CHECK_NOTNULL(peer_desc);
      GeTensorDesc tensor_desc(peer_desc->GetInputDesc(peer_anchor->GetIdx()));
      (void)AttrUtils::SetInt(tensor_desc, ATTR_NAME_PARENT_NODE_INDEX, parent_index);
      GE_CHK_STATUS_RET(node_output_desc->AddInputDesc(tensor_desc), "Add input desc fail");
      const auto input_size = node_output->GetAllInDataAnchorsSize();
      (void)NodeUtils::AppendInputAnchor(node_output, input_size + 1);
      if (GraphUtils::AddEdge(output_anchor, node_output->GetInDataAnchor(input_size)) != SUCCESS) {
        REPORT_INNER_ERROR("E19999", "Add edge from node[%s] output[%d] to node[%s] input[%u] failed.",
                           node->GetName().c_str(), output_anchor->GetIdx(),
                           node_output->GetName().c_str(), input_size);
        GELOGE(FAILED, "Add edge from node[%s] output[%d] to node[%s] input[%u] failed.",
               node->GetName().c_str(), output_anchor->GetIdx(), node_output->GetName().c_str(), input_size);
        return FAILED;
      }

      (void)partition_desc->AddOutputDesc(node_desc->GetOutputDesc(output_anchor->GetIdx()));
      (void)NodeUtils::AppendOutputAnchor(new_partitioned_call_, parent_index + 1);
      auto new_anchor = new_partitioned_call_->GetOutDataAnchor(parent_index);
      GE_CHECK_NOTNULL(new_anchor);

      if (GraphUtils::AddEdge(new_anchor, peer_anchor) != SUCCESS) {
        REPORT_INNER_ERROR("E19999", "Add edge from node[%s] output[%zu] to node[%s] input[%d] failed.",
                           new_partitioned_call_->GetName().c_str(), parent_index,
                           peer_node->GetName().c_str(), peer_anchor->GetIdx());
        GELOGE(FAILED, "Add edge from node[%s] output[%zu] to node[%s] input[%d] failed.",
               new_partitioned_call_->GetName().c_str(), parent_index,
               peer_node->GetName().c_str(), peer_anchor->GetIdx());
        return FAILED;
      }
      anchor_to_output_[new_anchor] = node_output->GetInDataAnchor(input_size);
    }
  }
  return SUCCESS;
}

Status CreateSubGraphWithScopePass::ProcPartitionInputAnchor(const ComputeGraphPtr &subgraph,
                                                             const OutDataAnchorPtr &node_anchor,
                                                             const InDataAnchorPtr &partition_anchor) {
  const std::map<InDataAnchorPtr, NodePtr>::const_iterator &it = anchor_to_data_nodes_.find(partition_anchor);
  if (it == anchor_to_data_nodes_.cend()) {
    REPORT_INNER_ERROR("E19999", "Can't find data node by input anchor[%d].", partition_anchor->GetIdx());
    GELOGE(PARAM_INVALID, "Can't find data node by input anchor[%d].", partition_anchor->GetIdx());
    return PARAM_INVALID;
  }
  const auto &data_node = it->second;
  GE_CHECK_NOTNULL(data_node);
  const auto &output_anchor = data_node->GetOutDataAnchor(0);
  if (output_anchor != nullptr) {
    for (auto peer_anchor : output_anchor->GetPeerInDataAnchors()) {
      if (peer_anchor == nullptr) {
        continue;
      }
      const auto &peer_node = peer_anchor->GetOwnerNode();
      GE_CHECK_NOTNULL(peer_node);
      if (GraphUtils::RemoveEdge(output_anchor, peer_anchor) != SUCCESS) {
        REPORT_INNER_ERROR("E19999", "Remove edge from node[%s] output[%d] to node[%s] input[%d] failed.",
                           data_node->GetName().c_str(), output_anchor->GetIdx(),
                           peer_node->GetName().c_str(), peer_anchor->GetIdx());
        GELOGE(FAILED, "Remove edge from node[%s] output[%d] to node[%s] input[%d] failed.",
               data_node->GetName().c_str(), output_anchor->GetIdx(),
               peer_node->GetName().c_str(), peer_anchor->GetIdx());
        return FAILED;
      }
      if (GraphUtils::AddEdge(node_anchor, peer_anchor) != SUCCESS) {
        REPORT_INNER_ERROR("E19999", "Add edge from node[%s] output[%d] to partitioned_call input[%d] failed.",
                           node_anchor->GetOwnerNode()->GetName().c_str(), node_anchor->GetIdx(),
                           partition_anchor->GetIdx());
        GELOGE(FAILED, "Add edge from node[%s] output[%d] to partitioned_call input[%d] failed.",
               node_anchor->GetOwnerNode()->GetName().c_str(), node_anchor->GetIdx(), partition_anchor->GetIdx());
        return FAILED;
      }
    }
  }
  if (GraphUtils::RemoveEdge(node_anchor, partition_anchor) != SUCCESS) {
    REPORT_INNER_ERROR("E19999", "Remove edge from node[%s] output[%d] to partitioned_call input[%d].",
                       node_anchor->GetOwnerNode()->GetName().c_str(), node_anchor->GetIdx(),
                       partition_anchor->GetIdx());
    GELOGE(FAILED, "Remove edge from node[%s] output[%d] to partitioned_call input[%d].",
           node_anchor->GetOwnerNode()->GetName().c_str(), node_anchor->GetIdx(), partition_anchor->GetIdx());
    return FAILED;
  }
  GE_CHK_STATUS_RET(subgraph->RemoveNode(data_node), "Remove node[%s] from subgraph[%s] failed.",
                    data_node->GetName().c_str(), subgraph->GetName().c_str());
  anchor_to_data_nodes_.erase(it);
  return SUCCESS;
}

Status CreateSubGraphWithScopePass::ProcPartitionOutputAnchor(const OutDataAnchorPtr &partition_anchor,
                                                              const InDataAnchorPtr &node_anchor) {
  const std::map<OutDataAnchorPtr, InDataAnchorPtr>::const_iterator &it = anchor_to_output_.find(partition_anchor);
  if (it == anchor_to_output_.cend()) {
    REPORT_INNER_ERROR("E19999", "Can't find subgraph anchor by netoutput anchor[%d].", partition_anchor->GetIdx());
    GELOGE(PARAM_INVALID, "Can't find subgraph anchor by netoutput anchor[%d].", partition_anchor->GetIdx());
    return PARAM_INVALID;
  }
  const auto &output_anchor = it->second;
  GE_CHECK_NOTNULL(output_anchor);
  const auto &peer_anchor = output_anchor->GetPeerOutAnchor();
  GE_CHECK_NOTNULL(peer_anchor);
  const auto &peer_node = peer_anchor->GetOwnerNode();
  GE_CHECK_NOTNULL(peer_node);

  // unlink partitioned_call and current node
  if (GraphUtils::RemoveEdge(partition_anchor, node_anchor) != SUCCESS) {
    REPORT_INNER_ERROR("E19999", "Remove edge from partitioned_call output[%d] to node[%s] input[%d] failed.",
                       partition_anchor->GetIdx(),
                       node_anchor->GetOwnerNode()->GetName().c_str(), node_anchor->GetIdx());
    GELOGE(FAILED, "Remove edge from partitioned_call output[%d] to node[%s] input[%d] failed.",
           partition_anchor->GetIdx(), node_anchor->GetOwnerNode()->GetName().c_str(), node_anchor->GetIdx());
    return FAILED;
  }
  if (GraphUtils::AddEdge(peer_anchor, node_anchor) != SUCCESS) {
    REPORT_INNER_ERROR("E19999", "Add edge from node[%s] output[%d] to node[%s] input[%d] failed.",
                       peer_node->GetName().c_str(), peer_anchor->GetIdx(),
                       node_anchor->GetOwnerNode()->GetName().c_str(), node_anchor->GetIdx());
    GELOGE(FAILED, "Add edge from node[%s] output[%d] to node[%s] input[%d] failed.",
           peer_node->GetName().c_str(), peer_anchor->GetIdx(),
           node_anchor->GetOwnerNode()->GetName().c_str(), node_anchor->GetIdx());
    return FAILED;
  }
  if (partition_anchor->GetPeerAnchorsSize() == 0U) {
    // unlink net_output and it's input node
    if (GraphUtils::RemoveEdge(peer_anchor, output_anchor) != SUCCESS) {
      REPORT_INNER_ERROR("E19999", "Remove edge from node[%s] output[%d] to node[%s] input[%d] failed.",
                         peer_node->GetName().c_str(), peer_anchor->GetIdx(),
                         output_anchor->GetOwnerNode()->GetName().c_str(), output_anchor->GetIdx());
      GELOGE(FAILED, "Remove edge from node[%s] output[%d] to node[%s] input[%d] failed.",
             peer_node->GetName().c_str(), peer_anchor->GetIdx(),
             output_anchor->GetOwnerNode()->GetName().c_str(), output_anchor->GetIdx());
      return FAILED;
    }
    anchor_to_output_.erase(it);
  }
  return SUCCESS;
}

Status CreateSubGraphWithScopePass::CreateNewPartitionedCall(const ComputeGraphPtr &root_graph,
                                                             const ComputeGraphPtr &subgraph,
                                                             const std::vector<NodePtr> &scope_nodes) {
  const auto &partitioned_call = subgraph->GetParentNode();
  GE_CHECK_NOTNULL(partitioned_call);

  const OpDescPtr op_desc = MakeShared<OpDesc>(partitioned_call->GetName() + "_copy", PARTITIONEDCALL);
  GE_CHECK_NOTNULL(op_desc);
  GELOGD("Copy new partitioned call from [%s].", partitioned_call->GetName().c_str());
  new_partitioned_call_ = root_graph->AddNode(op_desc);
  GE_CHECK_NOTNULL(new_partitioned_call_);

  GE_CHK_STATUS_RET(CopyPartitionedCallStaticInput(partitioned_call, scope_nodes), "Copy static input failed.");
  GE_CHK_STATUS_RET(CopyPartitionedCallStaticOutput(partitioned_call, scope_nodes), "Copy static output failed.");

  return SUCCESS;
}

Status CreateSubGraphWithScopePass::CopyPartitionedCallStaticInput(const NodePtr &src_node,
                                                                   const std::vector<NodePtr> &scope_nodes) {
  const auto &src_desc = src_node->GetOpDesc();
  GE_CHECK_NOTNULL(src_desc);
  const auto &new_desc = new_partitioned_call_->GetOpDesc();
  GE_CHECK_NOTNULL(new_desc);
  size_t copy_input_cnt = 0U;
  for (const auto &input_anchor : src_node->GetAllInDataAnchors()) {
    if (input_anchor == nullptr) {
      continue;
    }
    const auto &peer_anchor = input_anchor->GetPeerOutAnchor();
    if (peer_anchor == nullptr) {
      continue;
    }
    const auto &peer_node = peer_anchor->GetOwnerNode();
    if (peer_node == nullptr) {
      continue;
    }
    const auto &it = std::find(scope_nodes.begin(), scope_nodes.end(), peer_node);
    if (it != scope_nodes.end()) {
      GELOGD("Peer node[%s] is in scope, no need copy anchor.", peer_node->GetName().c_str());
      continue;
    }
    const auto &input_tensor = src_desc->GetInputDesc(input_anchor->GetIdx());
    GE_CHK_STATUS_RET(new_desc->AddInputDesc(input_tensor),
                      "Add input desc for [%s] fail", new_desc->GetName().c_str());
    if (GraphUtils::RemoveEdge(peer_anchor, input_anchor) != GRAPH_SUCCESS) {
      REPORT_CALL_ERROR("E19999", "Remove edge from node[%s] output[%d] to node[%s] input[%d] failed",
                        peer_node->GetName().c_str(), peer_anchor->GetIdx(),
                        src_node->GetName().c_str(), input_anchor->GetIdx());
      GELOGE(FAILED, "Remove edge from node[%s] output[%d] to node[%s] input[%d] failed",
             peer_node->GetName().c_str(), peer_anchor->GetIdx(),
             src_node->GetName().c_str(), input_anchor->GetIdx());
      return FAILED;
    }
    (void)NodeUtils::AppendInputAnchor(new_partitioned_call_, copy_input_cnt + 1);
    const auto &new_anchor = new_partitioned_call_->GetInDataAnchor(copy_input_cnt);
    if (GraphUtils::AddEdge(peer_anchor, new_anchor) != GRAPH_SUCCESS) {
      REPORT_CALL_ERROR("E19999", "Add edge from node[%s] output[%d] to node[%s] input[%zu] failed",
                        peer_node->GetName().c_str(), peer_anchor->GetIdx(),
                        new_partitioned_call_->GetName().c_str(), copy_input_cnt);
      GELOGE(FAILED, "Add edge from node[%s] output[%d] to node[%s] input[%zu] failed",
             peer_node->GetName().c_str(), peer_anchor->GetIdx(),
             new_partitioned_call_->GetName().c_str(), copy_input_cnt);
      return FAILED;
    }
    copy_input_cnt++;
    const auto &iter = anchor_to_data_nodes_.find(input_anchor);
    if (iter != anchor_to_data_nodes_.end()) {
      anchor_to_data_nodes_[new_anchor] = iter->second;
      anchor_to_data_nodes_.erase(iter);
    }
  }
  return SUCCESS;
}

Status CreateSubGraphWithScopePass::CopyPartitionedCallStaticOutput(const NodePtr &src_node,
                                                                    const std::vector<NodePtr> &scope_nodes) {
  const auto &src_desc = src_node->GetOpDesc();
  GE_CHECK_NOTNULL(src_desc);
  const auto &new_desc = new_partitioned_call_->GetOpDesc();
  GE_CHECK_NOTNULL(new_desc);

  size_t copy_output_cnt = 0U;
  for (const auto &output_anchor : src_node->GetAllOutDataAnchors()) {
    std::vector<InDataAnchorPtr> in_scope;
    std::vector<InDataAnchorPtr> out_scope;
    for (const auto &peer_anchor : output_anchor->GetPeerInDataAnchors()) {
      if (peer_anchor == nullptr) {
        continue;
      }
      const auto &peer_node = peer_anchor->GetOwnerNode();
      if (peer_node == nullptr) {
        continue;
      }
      const auto &it = std::find(scope_nodes.begin(), scope_nodes.end(), peer_node);
      if (it != scope_nodes.end()) {
        in_scope.push_back(peer_anchor);
        GELOGD("Peer node[%s] is in scope, no need copy anchor.", peer_node->GetName().c_str());
      } else {
        out_scope.push_back(peer_anchor);
      }
    }
    if (!out_scope.empty()) {
      const auto &output_tensor = src_desc->GetOutputDesc(output_anchor->GetIdx());
      GE_CHK_STATUS_RET(new_desc->AddOutputDesc(output_tensor), "Add output desc for [%s] fail",
                        new_desc->GetName().c_str());
      (void)NodeUtils::AppendOutputAnchor(new_partitioned_call_, copy_output_cnt + 1);
      const auto &new_anchor = new_partitioned_call_->GetOutDataAnchor(copy_output_cnt);
      for (auto it : out_scope) {
        if (GraphUtils::RemoveEdge(output_anchor, it) != GRAPH_SUCCESS) {
          REPORT_CALL_ERROR("E19999", "Remove edge from node[%s] output[%d] to node[%s] input[%d] failed",
                            src_node->GetName().c_str(), output_anchor->GetIdx(),
                            it->GetOwnerNode()->GetName().c_str(), it->GetIdx());
          GELOGE(FAILED, "Remove edge from node[%s] output[%d] to node[%s] input[%d] failed",
                 src_node->GetName().c_str(), output_anchor->GetIdx(),
                 it->GetOwnerNode()->GetName().c_str(), it->GetIdx());
          return FAILED;
        }
        if (GraphUtils::AddEdge(new_anchor, it) != GRAPH_SUCCESS) {
          REPORT_CALL_ERROR("E19999", "Add edge from node[%s] output[%zu] to node[%s] input[%d] failed",
                            new_partitioned_call_->GetName().c_str(), copy_output_cnt,
                            it->GetOwnerNode()->GetName().c_str(), it->GetIdx());
          GELOGE(FAILED, "Add edge from node[%s] output[%zu] to node[%s] input[%d] failed",
                 new_partitioned_call_->GetName().c_str(), copy_output_cnt,
                 it->GetOwnerNode()->GetName().c_str(), it->GetIdx());
          return FAILED;
        }
      }
      copy_output_cnt++;
      if (in_scope.empty()) {
        const auto &iter = anchor_to_output_.find(output_anchor);
        if (iter != anchor_to_output_.end()) {
          anchor_to_output_[new_anchor] = iter->second;
          anchor_to_output_.erase(iter);
        }
      }
    }
  }
  return SUCCESS;
}

Status CreateSubGraphWithScopePass::SetParentIndexToData(const ComputeGraphPtr &subgraph) {
  for (const auto &it : anchor_to_data_nodes_) {
    if (it.first == nullptr || it.first->GetPeerOutAnchor() == nullptr) {
      subgraph->RemoveNode(it.second);
    }
    const int32_t parent_index = it.first->GetIdx();
    const auto &data_desc = it.second->GetOpDesc();
    GE_CHECK_NOTNULL(data_desc);
    (void)AttrUtils::SetInt(data_desc, ATTR_NAME_PARENT_NODE_INDEX, parent_index);
    GELOGD("Set data node[%s] parent index[%d].", data_desc->GetName().c_str(), parent_index);
  }
  return SUCCESS;
}

Status CreateSubGraphWithScopePass::SetParentIndexToNetOutput(const ComputeGraphPtr &subgraph) {
  const auto &net_output = subgraph->FindFirstNodeMatchType(NETOUTPUT);
  GE_CHECK_NOTNULL(net_output);
  const auto &old_desc = net_output->GetOpDesc();
  GE_CHECK_NOTNULL(old_desc);

  const OpDescPtr op_desc = MakeShared<OpDesc>(net_output->GetName() + "_copy", NETOUTPUT);
  GE_CHECK_NOTNULL(op_desc);
  GELOGD("Copy new node_output from [%s].", net_output->GetName().c_str());
  const auto new_net_output = subgraph->AddNode(op_desc);
  GE_CHECK_NOTNULL(new_net_output);

  size_t all_input_size = 0U;
  for (const auto &it : anchor_to_output_) {
    if (it.first == nullptr || it.first->GetPeerAnchorsSize() == 0U ||
        it.second == nullptr || it.second->GetPeerAnchorsSize() == 0U) {
      continue;
    }
    GE_CHK_STATUS_RET(op_desc->AddInputDesc(old_desc->GetInputDesc(it.second->GetIdx())),
                      "Add input desc for [%s] failed.", op_desc->GetName().c_str());
    (void)NodeUtils::AppendInputAnchor(new_net_output, all_input_size + 1);
    const auto &new_anchor = new_net_output->GetInDataAnchor(all_input_size);
    GE_CHECK_NOTNULL(new_anchor);

    const auto &peer_anchor = it.second->GetPeerOutAnchor();
    GE_CHECK_NOTNULL(peer_anchor);
    if (GraphUtils::RemoveEdge(peer_anchor, it.second) != SUCCESS) {
      REPORT_INNER_ERROR("E19999", "Remove edge from node[%s] output[%d] to node[%s] input[%d] failed.",
                         peer_anchor->GetOwnerNode()->GetName().c_str(), peer_anchor->GetIdx(),
                         net_output->GetName().c_str(), it.second->GetIdx());
      GELOGE(FAILED, "Remove edge from node[%s] output[%d] to node[%s] input[%d] failed.",
             peer_anchor->GetOwnerNode()->GetName().c_str(), peer_anchor->GetIdx(),
             net_output->GetName().c_str(), it.second->GetIdx());
      return FAILED;
    }
    if (GraphUtils::AddEdge(peer_anchor, new_anchor) != SUCCESS) {
      REPORT_INNER_ERROR("E19999", "Add edge from node[%s] output[%d] to node[%s] input[%d] failed.",
                         peer_anchor->GetOwnerNode()->GetName().c_str(), peer_anchor->GetIdx(),
                         new_net_output->GetName().c_str(), new_anchor->GetIdx());
      GELOGE(FAILED, "Add edge from node[%s] output [%d] to node[%s] input[%d] failed.",
             peer_anchor->GetOwnerNode()->GetName().c_str(), peer_anchor->GetIdx(),
             new_net_output->GetName().c_str(), new_anchor->GetIdx());
      return FAILED;
    }

    const auto &new_tensor = op_desc->MutableInputDesc(all_input_size);
    const int32_t parent_index = it.first->GetIdx();
    (void)AttrUtils::SetInt(new_tensor, ATTR_NAME_PARENT_NODE_INDEX, parent_index);
    GELOGD("Set parent index[%d] to new net_output[%s] input[%zu].",
           parent_index, new_net_output->GetName().c_str(), all_input_size);
    all_input_size++;
  }
  (void)subgraph->RemoveNode(net_output);
  return SUCCESS;
}
}  // namespace ge
