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

#include "common/graph/fe_graph_utils.h"

#include "graph/utils/attr_utils.h"
#include "graph/utils/graph_utils.h"
#include "common/fe_inner_attr_define.h"

namespace fe {
void FeGraphUtils::DumpSubGraphAndOnnx(const ge::ComputeGraph &graph, const std::string &suffix) {
  for (auto subgraph : graph.GetAllSubgraphs()) {
    DumpGraphAndOnnx(*subgraph, suffix);
  }
}

void FeGraphUtils::DumpGraphAndOnnx(const ge::ComputeGraph &graph, const std::string &suffix) {
  DumpGraph(graph, suffix);
  ge::GraphUtils::DumpGEGraphToOnnx(graph, suffix);
}

void FeGraphUtils::DumpGraph(const ge::ComputeGraph &graph, const std::string &suffix) {
  std::shared_ptr<ge::ComputeGraph> compute_graph_ptr = FeComGraphMakeShared<ge::ComputeGraph>(graph);
  ge::GraphUtils::DumpGEGraph(compute_graph_ptr, suffix);
}

bool FeGraphUtils::IsMainGraphData(const ge::OpDescPtr &op_desc_ptr) {
  if (op_desc_ptr == nullptr) {
    return false;
  }
  return op_desc_ptr->GetType() == DATA && !IsSubGraphData(op_desc_ptr);
}

bool FeGraphUtils::IsMainGraphNetOutput(const ge::OpDescPtr &op_desc_ptr) {
  if (op_desc_ptr == nullptr) {
    return false;
  }
  return op_desc_ptr->GetType() == NETOUTPUT && !IsSubGraphNetOutput(op_desc_ptr);
}

bool FeGraphUtils::IsSubGraphDataOrNetOutput(const ge::OpDescPtr &op_desc_ptr) {
  return IsSubGraphData(op_desc_ptr) || IsSubGraphNetOutput(op_desc_ptr);
}

bool FeGraphUtils::IsNotSubGraphDataAndNetOutput(const ge::OpDescPtr &op_desc_ptr) {
  return !IsSubGraphData(op_desc_ptr) && !IsSubGraphNetOutput(op_desc_ptr);
}

bool FeGraphUtils::IsSubGraphData(const ge::OpDescPtr &op_desc_ptr) {
  if (op_desc_ptr == nullptr || op_desc_ptr->GetType() != DATA) {
    return false;
  }
  return op_desc_ptr->HasAttr(ge::ATTR_NAME_PARENT_NODE_INDEX);
}

bool FeGraphUtils::IsSubGraphNetOutput(const ge::OpDescPtr &op_desc_ptr) {
  if (op_desc_ptr == nullptr || op_desc_ptr->GetType() != NETOUTPUT) {
    return false;
  }
  for (auto &tensor : op_desc_ptr->GetAllInputsDescPtr()) {
    if (ge::AttrUtils::HasAttr(tensor, ge::ATTR_NAME_PARENT_NODE_INDEX)) {
      return true;
    }
  }
  return false;
}

Status FeGraphUtils::GetPreOutAnchorOfSubData(const ge::NodePtr &data_node_ptr,
                                              ge::OutDataAnchorPtr &pre_out_data_anchor_ptr) {
  FE_CHECK_NOTNULL(data_node_ptr);
  ge::OpDescPtr data_op_desc_ptr = data_node_ptr->GetOpDesc();
  FE_CHECK_NOTNULL(data_op_desc_ptr);
  uint32_t parent_node_index = 0;
  if (!ge::AttrUtils::GetInt(data_op_desc_ptr, ge::ATTR_NAME_PARENT_NODE_INDEX, parent_node_index)) {
    REPORT_FE_ERROR("[GraphOptJdgInst][UpdFmtAndDtype][GetPreOutAncr] attr %s is missing for node %s",
                    ge::ATTR_NAME_PARENT_NODE_INDEX.c_str(), data_op_desc_ptr->GetName().c_str());
    return FAILED;
  }
  auto owner_graph = data_node_ptr->GetOwnerComputeGraph();
  FE_CHECK_NOTNULL(owner_graph);

  ge::NodePtr parent_node_ptr = owner_graph->GetParentNode();
  FE_CHECK_NOTNULL(parent_node_ptr);
  ge::InDataAnchorPtr in_data_anchor_ptr = parent_node_ptr->GetInDataAnchor(parent_node_index);
  FE_CHECK_NOTNULL(in_data_anchor_ptr);
  pre_out_data_anchor_ptr = in_data_anchor_ptr->GetPeerOutAnchor();
  return SUCCESS;
}

Status FeGraphUtils::GetPreSubNetoutputInAnchor(std::unordered_set<ge::RefCell, ge::RefCellHash> &reflections,
                                                std::vector<ge::InDataAnchorPtr> &vec_netoutput_in_ahchor) {
  for (const auto &cell : reflections) {
    if ((cell.in_out != ge::NODE_IN) || (cell.node->GetType() != NETOUTPUT)) {
      continue;
    }

    for (auto &in_anchor : cell.node->GetAllInDataAnchors()) {
      FE_CHECK_NOTNULL(in_anchor);
      if (in_anchor->GetIdx() == cell.in_out_idx) {
        vec_netoutput_in_ahchor.push_back(in_anchor);
        break;
      }
    }
  }

  if (vec_netoutput_in_ahchor.empty()) {
    return FAILED;
  }

  return SUCCESS;
}

Status FeGraphUtils::GetNextInAnchorsOfSubNetOutput(const ge::NodePtr &net_output_node_ptr, const int &input_index,
                                                    std::vector<ge::InDataAnchorPtr> &next_in_data_anchors) {
  FE_CHECK_NOTNULL(net_output_node_ptr);
  ge::OpDescPtr op_desc_ptr = net_output_node_ptr->GetOpDesc();
  FE_CHECK_NOTNULL(op_desc_ptr);
  auto input_desc = op_desc_ptr->GetInputDescPtr(input_index);
  uint32_t parent_index = -1;
  if (!ge::AttrUtils::GetInt(input_desc, ge::ATTR_NAME_PARENT_NODE_INDEX, parent_index)) {
    FE_LOGE("");
    return FAILED;
  }

  auto owner_graph = net_output_node_ptr->GetOwnerComputeGraph();
  FE_CHECK_NOTNULL(owner_graph);
  ge::NodePtr parent_node_ptr = owner_graph->GetParentNode();
  FE_CHECK_NOTNULL(parent_node_ptr);
  ge::OutDataAnchorPtr out_data_anchor_ptr = parent_node_ptr->GetOutDataAnchor(parent_index);
  FE_CHECK_NOTNULL(out_data_anchor_ptr);
  for (auto it : out_data_anchor_ptr->GetPeerInDataAnchors()) {
    next_in_data_anchors.push_back(it);
  }
  return SUCCESS;
}

Status FeGraphUtils::GetNextSubDatasOutAnchors(std::unordered_set<ge::RefCell, ge::RefCellHash> &reflections,
                                               std::vector<ge::OutDataAnchorPtr> &out_data_anchors) {
  for (const auto &cell : reflections) {
    if ((cell.in_out != ge::NODE_OUT) || (cell.node->GetType() != DATA)) {
      continue;
    }

    for (auto &out_anchor : cell.node->GetAllOutDataAnchors()) {
      FE_CHECK_NOTNULL(out_anchor);
      if (out_anchor->GetIdx() == cell.in_out_idx) {
        out_data_anchors.push_back(out_anchor);
        break;
      }
    }
  }

  if (out_data_anchors.empty()) {
    return FAILED;
  }
  return SUCCESS;
}

Status FeGraphUtils::UpdateFormatOfRelatedEdges(const std::unordered_set<ge::RefCell, ge::RefCellHash> &reflections,
                                                const RelationUpdateInfo &relation_update_info_a) {
  FE_LOGD("relationUpdateInfo: primary_format=[%s], sub_format=[%d], shape=[%s].",
          ge::TypeUtils::FormatToSerialString(relation_update_info_a.primary_format).c_str(),
          relation_update_info_a.sub_format, GetShapeDims(relation_update_info_a.shape).c_str());

  for (const auto &cell : reflections) {
    ge::NodePtr node_ptr = cell.node;
    FE_CHECK_NOTNULL(node_ptr);
    ge::OpDescPtr op_desc_ptr = node_ptr->GetOpDesc();
    FE_CHECK_NOTNULL(op_desc_ptr);
    auto owner_graph = node_ptr->GetOwnerComputeGraph();
    FE_CHECK_NOTNULL(owner_graph);
    string graph_name = owner_graph->GetName();

    string node_name = node_ptr->GetName();
    FE_LOGD("Graph[%s]Op[type=%s,name=%s]: cell.in_out_idx=[%d], cell.in_out=[%d].", graph_name.c_str(),
            node_ptr->GetType().c_str(), node_name.c_str(), cell.in_out_idx, cell.in_out);

    // 1. get the input or output desc
    auto index = cell.in_out_idx;
    auto desc = (cell.in_out == ge::NODE_IN ? op_desc_ptr->GetInputDesc(static_cast<uint32_t>(index))
                                            : op_desc_ptr->GetOutputDesc(static_cast<uint32_t>(index)));

    // 2. set the format
    string input_or_output = cell.in_out == ge::NODE_IN ? STR_INPUT_LOWERCASE : STR_OUTPUT_LOWERCASE;
    if (relation_update_info_a.primary_format != ge::FORMAT_RESERVED) {
      ge::Format cur_format = desc.GetFormat();
      ge::GeShape cur_shape = desc.GetShape();
      auto new_format = static_cast<ge::Format>(
          ge::GetFormatFromSub(relation_update_info_a.primary_format, relation_update_info_a.sub_format));
      desc.SetFormat(new_format);
      desc.SetShape(relation_update_info_a.shape);

      FE_LOGD(
          "Graph[%s]Op[type=%s,name=%s]: update the %s %d desc, cur_format=[%s], cur_shape=[%s], new_format=[%s], "
          "newShape=[%s].",
          graph_name.c_str(), node_ptr->GetType().c_str(), node_name.c_str(), input_or_output.c_str(), index,
          ge::TypeUtils::FormatToSerialString(cur_format).c_str(), GetShapeDims(cur_shape).c_str(),
          ge::TypeUtils::FormatToSerialString(new_format).c_str(), GetShapeDims(relation_update_info_a.shape).c_str());
    }

    // 3. set the attribute for the tensor desc of function op
    // sub graph data and netoutput will not be set INFERFORMAT
    if (!relation_update_info_a.attr_name.empty() &&
        (op_desc_ptr->GetType() != DATA && op_desc_ptr->GetType() != NETOUTPUT)) {
      (void)ge::AttrUtils::SetInt(desc, relation_update_info_a.attr_name, relation_update_info_a.attr_value);
    }

    // 4. update the tensor desc
    if (cell.in_out == ge::NODE_IN) {
      (void)op_desc_ptr->UpdateInputDesc(static_cast<uint32_t>(index), desc);
    } else {
      (void)op_desc_ptr->UpdateOutputDesc(static_cast<uint32_t>(index), desc);
    }
  }
  return SUCCESS;
}

Status CopyEdgesForAllNode(ge::ComputeGraph &origin_graph, ge::ComputeGraph &clone_graph) {
  for (const ge::NodePtr &node : origin_graph.GetDirectNode()) {
    std::string node_name = node->GetName();
    ge::NodePtr src_node = clone_graph.FindNode(node_name);
    if (src_node == nullptr) {
      continue;
    }
    // connect out data anchor
    for (const auto &out_data_anchor : node->GetAllOutDataAnchors()) {
      FE_CHECK(out_data_anchor == nullptr,
               REPORT_FE_ERROR("[GraphOptJdgInst][UpdFmtAndDtype][CpEgForAllNd] One of the out data anchor of node[%s] \
               is nullptr.", node_name.c_str()), return FAILED);
      for (const auto &peer_in_data_anchor : out_data_anchor->GetPeerAnchors()) {
        FE_CHECK(peer_in_data_anchor == nullptr,
                 REPORT_FE_ERROR("[GraphOptJdgInst][UpdFmtAndDtype][CpEgForAllNd] One peer in data anchor of node[%s] \
                 is nullptr.", node_name.c_str()), return FAILED);
        FE_CHECK_NOTNULL(peer_in_data_anchor->GetOwnerNode());
        ge::NodePtr dst_node = clone_graph.FindNode(peer_in_data_anchor->GetOwnerNode()->GetName());
        Status ret =
            FeGraphUtils::AddEdge(src_node->GetOutDataAnchor(out_data_anchor->GetIdx()), dst_node, peer_in_data_anchor);
        if (ret != SUCCESS) {
          REPORT_FE_ERROR("[GraphOptJdgInst][UpdFmtAndDtype][CpEgForAllNd] Fail to add out data edge for node[%s].",
                          node_name.c_str());
          return FAILED;
        }
      }
    }
    // connect out control anchor
    auto out_ctrl_anchor = node->GetOutControlAnchor();
    FE_CHECK(out_ctrl_anchor == nullptr,
             REPORT_FE_ERROR("[GraphOptJdgInst][UpdFmtAndDtype][CpEgForAllNd] OutCtrlAnchor of node[%s] is nullptr.",
             node_name.c_str()), return FAILED);
    for (const auto &peer_in_anchor : out_ctrl_anchor->GetPeerAnchors()) {
      FE_CHECK(peer_in_anchor == nullptr,
               REPORT_FE_ERROR("[GraphOptJdgInst][UpdFmtAndDtype][CpEgForAllNd] Peer in control anchor of node[%s] is \
               nullptr.", node->GetName().c_str()), return FAILED);
      FE_CHECK_NOTNULL(peer_in_anchor->GetOwnerNode());
      ge::NodePtr dst_node = clone_graph.FindNode(peer_in_anchor->GetOwnerNode()->GetName());
      Status ret = FeGraphUtils::AddEdge(src_node->GetOutControlAnchor(), dst_node, peer_in_anchor);
      if (ret != SUCCESS) {
        REPORT_FE_ERROR("[GraphOptJdgInst][UpdFmtAndDtype][CpEgForAllNd] Fail to add out control edge for node[%s].",
                        node_name.c_str());
        return FAILED;
      }
    }
  }
  return SUCCESS;
}

Status CopyNodes(ge::ComputeGraph &origin_graph, ge::ComputeGraph &clone_graph, const string &graph_name) {
  for (ge::NodePtr node : origin_graph.GetDirectNode()) {
    FE_CHECK(node == nullptr,
             REPORT_FE_ERROR("[GraphOptJdgInst][UpdFmtAndDtype][CpNode] One node of graph[%s] is nullptr.",
             graph_name.c_str()), return FAILED);
    ge::OpDescPtr orig_opdef = node->GetOpDesc();
    ge::OpDescPtr new_opdef = ge::AttrUtils::CopyOpDesc(orig_opdef);
    FE_CHECK(new_opdef == nullptr,
             REPORT_FE_ERROR("[GraphOptJdgInst][UpdFmtAndDtype][CpNode] Fail to copy op desc of node[%s].",
             orig_opdef->GetName().c_str()), return FAILED);

    ge::NodePtr node_new = clone_graph.AddNode(new_opdef);
    FE_CHECK(node_new == nullptr,
             REPORT_FE_ERROR("[GraphOptJdgInst][UpdFmtAndDtype][CpNode] Fail to add node[%s] to graph[%s].",
             new_opdef->GetName().c_str(), graph_name.c_str()), return FAILED);

    node_new->SetOrigNode(node->GetOrigNode());
    ge::ComputeGraphPtr node_owner_graph_ptr = node->GetOwnerComputeGraph();
    if (node_owner_graph_ptr != nullptr) {
      node_new->SetOwnerComputeGraph(node_owner_graph_ptr);
    }

    ge::kFusionDataFlowVec_t fusion_input_list;
    node->GetFusionInputFlowList(fusion_input_list);
    node_new->SetFusionInputFlowList(fusion_input_list);

    ge::kFusionDataFlowVec_t fusion_output_list;
    node->GetFusionOutputFlowList(fusion_output_list);
    node_new->SetFusionOutputFlowList(fusion_output_list);

    std::vector<uint32_t> send_event_id_list = node->GetSendEventIdList();
    if (!send_event_id_list.empty()) {
      for (uint32_t event_id : send_event_id_list) {
        node_new->AddSendEventId(event_id);
      }
    }

    std::vector<uint32_t> recv_event_id_list = node->GetRecvEventIdList();
    if (!recv_event_id_list.empty()) {
      for (uint32_t event_id : recv_event_id_list) {
        node_new->AddRecvEventId(event_id);
      }
    }
  }
  return SUCCESS;
}

Status FeGraphUtils::CloneGraph(ge::ComputeGraph &origin_graph, ge::ComputeGraph &clone_graph) {
  // 1 copy name
  std::string graph_name = origin_graph.GetName();
  FE_LOGI("Begin to clone graph[%s].", graph_name.c_str());
  clone_graph.SetName(graph_name);
  clone_graph.SetSessionID(origin_graph.GetSessionID());
  clone_graph.SetGraphID(origin_graph.GetGraphID());
  std::string session_graph_id = "";
  if (ge::AttrUtils::GetStr(origin_graph, ge::ATTR_NAME_SESSION_GRAPH_ID, session_graph_id) &&
      !session_graph_id.empty()) {
    (void)ge::AttrUtils::SetStr(clone_graph, ge::ATTR_NAME_SESSION_GRAPH_ID, session_graph_id);
  }

  // 2  copy param_share_map
  std::map<std::vector<std::string>, std::vector<std::string>> params_share_map = origin_graph.GetShareParamLayer();
  clone_graph.SetShareParamLayer(params_share_map);

  // 3  copy graph_out_nodes
  std::map<std::string, std::vector<int32_t>> out_nodes_map = origin_graph.GetGraphOutNodes();
  clone_graph.SetGraphOutNodes(out_nodes_map);

  ge::ComputeGraphPtr orig_graph_ptr = origin_graph.GetOrigGraph();
  clone_graph.SetOrigGraph(orig_graph_ptr);

  // 4. copy node of model_graph to clone_graph,
  // relate each node to the same opdef.
  Status ret = CopyNodes(origin_graph, clone_graph, graph_name);
  if (ret != SUCCESS) {
    return FAILED;
  }

  // 5. add edge
  ret = CopyEdgesForAllNode(origin_graph, clone_graph);
  if (ret != SUCCESS) {
    return FAILED;
  }

  if (clone_graph.TopologicalSorting() != ge::GRAPH_SUCCESS) {
    REPORT_FE_ERROR("[GraphOptJdgInst][UpdFmtAndDtype][CloneGph] Fail to topological sort clone graph[%s].",
                    origin_graph.GetName().c_str());
    return FAILED;
  }
  FE_LOGI("Finish the clone of graph[%s].", origin_graph.GetName().c_str());
  return SUCCESS;
}

Status FeGraphUtils::AddEdge(ge::AnchorPtr src_anchor, ge::NodePtr dst_node, ge::AnchorPtr old_dst_anchor) {
  if (src_anchor == nullptr || dst_node == nullptr || old_dst_anchor == nullptr) {
    return FAILED;
  }

  auto old_dst_data_anchor = ge::Anchor::DynamicAnchorCast<ge::InDataAnchor>(old_dst_anchor);
  auto old_dst_ctrl_anchor = ge::Anchor::DynamicAnchorCast<ge::InControlAnchor>(old_dst_anchor);
  auto src_data_anchor = ge::Anchor::DynamicAnchorCast<ge::OutDataAnchor>(src_anchor);

  if (src_data_anchor != nullptr) {
    if (old_dst_data_anchor != nullptr) {
      ge::graphStatus ret =
          ge::GraphUtils::AddEdge(src_data_anchor, dst_node->GetInDataAnchor(old_dst_data_anchor->GetIdx()));
      if (ret == ge::GRAPH_FAILED) {
        REPORT_FE_ERROR("[GraphOptJdgInst][UpdFmtAndDtype][AddEg] AddEdge failed.");
        return FAILED;
      }
    }
  }
  auto src_ctrl_anchor = ge::Anchor::DynamicAnchorCast<ge::OutControlAnchor>(src_anchor);
  if (src_ctrl_anchor && old_dst_ctrl_anchor) {
    ge::graphStatus ret = ge::GraphUtils::AddEdge(src_ctrl_anchor, dst_node->GetInControlAnchor());
    if (ret == ge::GRAPH_FAILED) {
      REPORT_FE_ERROR("[GraphOptJdgInst][UpdFmtAndDtype][AddEg] AddEdge failed");
      return FAILED;
    }
  }
  return SUCCESS;
}

bool FeGraphUtils::CheckRelatedEdgesOriginShape(const std::unordered_set<ge::RefCell, ge::RefCellHash> &reflections) {
  int init_flag = 0;
  vector<int64_t> ref_origin_shape_dims;
  for (const auto &cell : reflections) {
    ge::NodePtr node_ptr = cell.node;
    FE_CHECK_NOTNULL(node_ptr);
    ge::OpDescPtr op_desc_ptr = node_ptr->GetOpDesc();
    FE_CHECK_NOTNULL(op_desc_ptr);
    auto owner_graph = node_ptr->GetOwnerComputeGraph();
    FE_CHECK_NOTNULL(owner_graph);
    string graph_name = owner_graph->GetName();
    string node_name = node_ptr->GetName();

    string input_output = cell.in_out == ge::NODE_IN ? STR_INPUT_LOWERCASE : STR_OUTPUT_LOWERCASE;
    FE_LOGD("Relations context: the %s %d of Graph[%s]Op[%s].", input_output.c_str(),
            cell.in_out_idx, graph_name.c_str(), node_name.c_str());

    auto index = cell.in_out_idx;
    auto desc = (cell.in_out == ge::NODE_IN ? op_desc_ptr->GetInputDescPtr(static_cast<uint32_t>(index))
                                            : op_desc_ptr->GetOutputDescPtr(static_cast<uint32_t>(index)));
    if (desc == nullptr) {
      return false;
    }
    vector<int64_t> origin_shape_dims = desc->GetOriginShape().GetDims();
    if (init_flag == 0) {
      ref_origin_shape_dims = origin_shape_dims;
      init_flag = 1;
    } else {
      if (ref_origin_shape_dims != origin_shape_dims) {
        FE_LOGD("Relations: the %s %d of Graph[%s]Op[%s], shape is not equal.", input_output.c_str(), cell.in_out_idx,
                graph_name.c_str(), node_name.c_str());
        return false;
      }
    }
  }
  return true;
}

void FeGraphUtils::GetGraphIdFromAttr(const ge::ComputeGraph &graph, string &graph_id) {
  string session_graph_id = "";
  if (ge::AttrUtils::GetStr(graph, ge::ATTR_NAME_SESSION_GRAPH_ID, session_graph_id) && !session_graph_id.empty()) {
    size_t location = session_graph_id.find('_');
    if (location != string::npos && location + 1 < session_graph_id.size()) {
      graph_id = session_graph_id.substr(location + 1);
    }
  }
  FE_LOGD("Get session_graph_id=%s graph_id=%s", session_graph_id.c_str(), graph_id.c_str());
}

bool FeGraphUtils::CheckSubGraphDataForConstValue(ge::NodePtr &parent_node) {
  ge::NodePtr really_parent_node = nullptr;
  if (ge::NodeUtils::GetInNodeCrossPartionedCallNode(parent_node, 0, really_parent_node) != SUCCESS) {
    FE_LOGW(
        "[SubGraphOpt][PreCompileOp][SetTensorConstVal][Op %s, type %s]: failed to \
        getInNodeCrossPartionCallNode.", parent_node->GetName().c_str(), parent_node->GetType().c_str());
    return false;
  }
  if (really_parent_node != nullptr) {
    std::string node_type = really_parent_node->GetType();
    FE_LOGD("Parent_node:%s type:%s really_parent_node:%s type:%s", parent_node->GetName().c_str(),
            parent_node->GetType().c_str(), really_parent_node->GetName().c_str(),
            really_parent_node->GetType().c_str());
    parent_node = really_parent_node;
    return (node_type == CONSTANT || node_type == CONSTANTOP);
  } else {
    FE_LOGD("real parent node for %s is null.", parent_node->GetName().c_str());
  }
  return false;
}

bool HasPeerOutNode(const ge::Node *node, const int &anchor_index,
                    ge::NodePtr &peer_out_node) {
  auto in_anchor = node->GetInDataAnchor(anchor_index);
  FE_CHECK(in_anchor == nullptr, FE_LOGW("index:%d in_anchor is nullptr",
           anchor_index), return false);
  auto peer_out_anchor = in_anchor->GetPeerOutAnchor();
  FE_CHECK(peer_out_anchor == nullptr, FE_LOGW("index:%d peer_out_anchor is nullptr",
           anchor_index), return false);
  peer_out_node = peer_out_anchor->GetOwnerNode();
  FE_CHECK(peer_out_node == nullptr, FE_LOGW("index:%d peer_out_anchor is nullptr",
           anchor_index), return false);
  return true;
}

void FeGraphUtils::IsNodeSpecificType(const std::unordered_set<string> &types,
                                      ge::NodePtr &node, bool &matched) {
  auto type = node->GetType();
  auto name = node->GetName();
  matched = types.count(type);
  if (matched) {
    return;
  }

  // if it is placeholder, get its parent node
  if (type == OP_TYPE_PLACE_HOLDER) {
    ge::NodePtr parent_node = nullptr;
    parent_node = node->GetOpDesc()->TryGetExtAttr(ATTR_NAME_PARENT_NODE, parent_node);
    if (parent_node != nullptr) {
      node = parent_node;
      type = parent_node->GetType();
      FE_LOGD("The parent node of place holder[%s] is [%s, %s].", name.c_str(),
              parent_node->GetName().c_str(), parent_node->GetType().c_str());
      ge::NodePtr really_parent_node = parent_node;
      bool parent_node_invalid = (!types.count(type) && ge::NodeUtils::GetInNodeCrossPartionedCallNode(parent_node, 0,
                                 really_parent_node) != SUCCESS);
      if (parent_node_invalid) {
        FE_LOGW("[SubGraphOpt][IsNodeSpecType][Op %s, type %s]: failed to getInNodeCrossPartionCallNode.",
                node->GetName().c_str(), type.c_str());
        return;
      }
      if (really_parent_node != nullptr) {
        node = really_parent_node;
        type = really_parent_node->GetType();
        FE_LOGD("Parent node:%s type:%s really parent node:%s type:%s", parent_node->GetName().c_str(),
                parent_node->GetType().c_str(), really_parent_node->GetName().c_str(),
                type.c_str());
      }
      matched = types.count(type);
    }
  } else if (FeGraphUtils::IsSubGraphData(node->GetOpDesc())) {
    matched = FeGraphUtils::CheckSubGraphDataForConstValue(node);
  } else {
    FE_LOGD("Cannot match any types for node %s and type %s.", node->GetName().c_str(), type.c_str());
  }
}

bool FeGraphUtils::IsPeerOutConst(const ge::Node *node, const int &anchor_index,
                                  ge::NodePtr &peer_out_node) {
  if (node == nullptr) {
    return false;
  }
  auto op_desc = node->GetOpDesc();
  bool has_other_node = HasPeerOutNode(node, anchor_index, peer_out_node);
  if (has_other_node) {
    peer_out_node = node->GetInDataAnchor(anchor_index)->GetPeerOutAnchor()->GetOwnerNode();
    bool is_const_node = false;
    IsNodeSpecificType(kConstTypes, peer_out_node, is_const_node);
    return is_const_node;
  } else {
    return false;
  }
}

bool FeGraphUtils::IsPeerOutWeight(ge::Node *node, const int &anchor_index,
                                   ge::NodePtr &peer_out_node) {
  if (node == nullptr) {
    return false;
  }
  auto op_desc = node->GetOpDesc();
  bool has_other_node = HasPeerOutNode(node, anchor_index, peer_out_node);
  if (has_other_node) {
    FE_LOGD("[IsPeerOutWeight]peer out node is %s.", peer_out_node->GetName().c_str());
    peer_out_node = node->GetInDataAnchor(anchor_index)->GetPeerOutAnchor()->GetOwnerNode();
    bool is_const_node = false;
    IsNodeSpecificType(kWeightTypes, peer_out_node, is_const_node);
    return is_const_node;
  } else {
    return false;
  }
}
}  // namespace fe
