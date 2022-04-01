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

#include "common/graph_comm.h"
#include "common/aicore_util_attr_define.h"
#include "common/comm_log.h"
#include "common/graph_comm_impl.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/graph_utils.h"
#include "common/util/error_manager/error_manager.h"
#include "common/fe_error_code.h"

namespace fe {
GraphComm::GraphComm(const string &engine_name) : engine_name_(engine_name) {
  exist_fusion_src_list_.clear();
  exist_fusion_dst_list_.clear();
  fusion_input_dataflow_list_.clear();
  fusion_output_dataflow_list_.clear();
}

GraphComm::~GraphComm() {}

Status GraphComm::Initialize() {
  graph_comm_impl_ptr_ = std::unique_ptr<GraphCommImpl>(new (std::nothrow) GraphCommImpl);
  CM_CHECK_NOTNULL(graph_comm_impl_ptr_);
  return SUCCESS;
}
Status GraphComm::GetFusionNodeEdgeList(vector<ge::NodePtr> &fus_nodelist,
                                        std::vector<FusionDataFlow> &fus_input_edge_list,
                                        std::vector<FusionDataFlow> &fus_output_edge_list) {
  fus_input_edge_list.clear();
  fus_output_edge_list.clear();

  for (ge::NodePtr &node : fus_nodelist) {
    if (node == nullptr) {
      REPORT_CM_ERROR("[SubGraphOpt][Merge][GetNodes] one node in the fusion_node_list is nullptr.");
      return FAILED;
    }
    CM_LOGD("GetFusionNodeEdgeList name = %s", node->GetName().c_str());
    std::vector<std::pair<ge::AnchorPtr, ge::AnchorPtr>> in_edges;
    if (graph_comm_impl_ptr_->GetAllInEdgeList(node, in_edges, 0) != SUCCESS) {
      REPORT_CM_ERROR("[SubGraphOpt][Merge][GetNodes] GetAllInEdgeList failed");
      return FAILED;
    }
    for (auto &in_edge : in_edges) {
      graph_comm_impl_ptr_->PutEdgeToFusionDataFlowVec(fus_nodelist, in_edge, in_edge.second, in_edge.first,
                                                       fus_input_edge_list);
    }

    std::vector<std::pair<ge::AnchorPtr, ge::AnchorPtr>> out_edges;
    if (graph_comm_impl_ptr_->GetAllOutEdgeList(node, out_edges, 0) != SUCCESS) {
      REPORT_CM_ERROR("[SubGraphOpt][Merge][GetNodes] GetAllOutEdgeList failed");
      return FAILED;
    }
    for (auto &out_edge : out_edges) {
      graph_comm_impl_ptr_->PutEdgeToFusionDataFlowVec(fus_nodelist, out_edge, out_edge.first, out_edge.second,
                                                       fus_output_edge_list);
    }
  }
  CM_LOGD("Get InputListSize:%zu, OutputListSize:%zu", fus_input_edge_list.size(), fus_output_edge_list.size());
  return SUCCESS;
}

Status GraphComm::GetFusionNodeCtrlEdgeList(vector<ge::NodePtr> &fus_nodelist,
                                            std::vector<FusionDataFlow> &fus_input_ctrl_edge_list,
                                            std::vector<FusionDataFlow> &fus_output_ctrl_edge_list) {
  fus_input_ctrl_edge_list.clear();
  fus_output_ctrl_edge_list.clear();

  for (ge::NodePtr &node : fus_nodelist) {
    CM_CHECK(node == nullptr,
             REPORT_CM_ERROR("[SubGraphOpt][Merge][GetNdCtrlList] GetFusionNodeCtrlEdges: the node in the \
             fusion_node_list is nullptr."), return FAILED);

    std::vector<std::pair<ge::AnchorPtr, ge::AnchorPtr>> in_edges;
    if (graph_comm_impl_ptr_->GetAllInEdgeList(node, in_edges, 1) != SUCCESS) {
      REPORT_CM_ERROR("[SubGraphOpt][Merge][GetNdCtrlList] GetAllInEdgeList failed");
      return FAILED;
    }
    for (auto &in_edge : in_edges) {
      graph_comm_impl_ptr_->PutEdgeToFusionDataFlowVec(fus_nodelist, in_edge, in_edge.second, in_edge.first,
                                                       fus_input_ctrl_edge_list);
    }

    std::vector<std::pair<ge::AnchorPtr, ge::AnchorPtr>> out_edges;
    if (graph_comm_impl_ptr_->GetAllOutEdgeList(node, out_edges, 1) != SUCCESS) {
      REPORT_CM_ERROR("[SubGraphOpt][Merge][GetNdCtrlList] GetAllOutEdgeList failed");
      return FAILED;
    }
    for (auto &out_edge : out_edges) {
      graph_comm_impl_ptr_->PutEdgeToFusionDataFlowVec(fus_nodelist, out_edge, out_edge.first, out_edge.second,
                                                       fus_output_ctrl_edge_list);
    }
  }
  CM_LOGD("Get InputListSize:%zu, OutputListSize:%zu", fus_input_ctrl_edge_list.size(),
          fus_output_ctrl_edge_list.size());
  return SUCCESS;
}

Status GraphComm::GetscopeNodeMap(ge::ComputeGraph &graph, ScopeNodeMap &fusion_map) {
  int64_t scope_id = 0;

  for (const ge::NodePtr &node : graph.GetDirectNode()) {
    ge::OpDescPtr op_desc_ptr = node->GetOpDesc();
    if (op_desc_ptr->HasAttr(SCOPE_ID_ATTR) && ge::AttrUtils::GetInt(op_desc_ptr, SCOPE_ID_ATTR, scope_id) &&
        scope_id >= 0) {
      graph_comm_impl_ptr_->SaveFusionNode(scope_id, fusion_map, node);
    }
  }
  return SUCCESS;
}

void GraphComm::ClearFusionSrc() {
  exist_fusion_src_list_.clear();
  fusion_input_dataflow_list_.clear();
  fusion_output_dataflow_list_.clear();
}

void GraphComm::ClearFusionDst() { exist_fusion_dst_list_.clear(); }

// ge::AnchorPtr replace index
bool GraphComm::GetFusionSrc(const uint32_t &src_op_id, const ge::AnchorPtr &src_anchor, int32_t &fusion_src_index,
                             int32_t &fusion_dst_index) {
  for (auto &item : exist_fusion_src_list_) {
    if ((item.src_op_id == src_op_id) && (item.src_anchor == src_anchor)) {
      fusion_src_index = item.fusion_src_index;
      fusion_dst_index = item.fusion_dst_index;
      return true;
    }
  }

  return false;
}

bool GraphComm::IsFusionDstExist(const uint32_t &dst_op_id, const ge::AnchorPtr &dst_anchor) {
  for (auto &item : exist_fusion_dst_list_) {
    if ((item.dst_op_id == dst_op_id) && (item.dst_anchor == dst_anchor)) {
      return true;
    }
  }
  return false;
}

void GraphComm::AddFusionInputSrc(const uint32_t &src_op_id, const ge::AnchorPtr &src_anchor,
                                  const int32_t &fusion_dst_index,
                                  std::pair<string, ge::AnchorPtr> &node_dataindex_pair) {
  graph_comm_impl_ptr_->AddFusionSrc(src_op_id, src_anchor, 0, fusion_dst_index, exist_fusion_src_list_);
  std::multimap<string, ge::AnchorPtr> flowmap;
  flowmap.insert(node_dataindex_pair);
  fusion_input_dataflow_list_.emplace_back(flowmap);
}

void GraphComm::AddFusionOutputSrc(const uint32_t &src_op_id, const ge::AnchorPtr &src_anchor,
                                   const int32_t &fusion_src_index,
                                   std::pair<string, ge::AnchorPtr> &node_dataindex_pair) {
  graph_comm_impl_ptr_->AddFusionSrc(src_op_id, src_anchor, fusion_src_index, 0, exist_fusion_src_list_);
  std::multimap<string, ge::AnchorPtr> flowmap;
  flowmap.insert(node_dataindex_pair);
  fusion_output_dataflow_list_.emplace_back(flowmap);
}

void GraphComm::SaveFusionDst(const uint32_t &dst_op_id, ge::AnchorPtr dst_anchor) {
  FusionOpDst value;
  value.dst_op_id = dst_op_id;
  value.dst_anchor = dst_anchor;
  exist_fusion_dst_list_.emplace_back(value);
}

Status GraphComm::GetNodeDataFlowMap(
    const ge::NodePtr &fus_node, std::map<ge::NodePtr, std::map<ge::AnchorPtr, ge::AnchorPtr>> &fusion_op_anchors_map,
    ge::kFusionDataFlowVec_t &fus_dataflow_list, const int &map_type) {
  // get fusion nodes
  vector<ge::NodePtr> fus_nodelist;
  fus_nodelist = fus_node->GetOpDesc()->TryGetExtAttr("ScopeNodeSet", fus_nodelist);
  if (fus_nodelist.empty()) {
    REPORT_CM_ERROR("[SubGraphOpt][Merge][GetNdDataFlowMap] TryGetExtAttr ScopeNodeSet failed.");
    return FAILED;
  }

  ge::kFusionDataFlowVec_t fusnode_dataflow_list;
  // in type
  if (map_type == 0) {
    fus_node->GetFusionInputFlowList(fusnode_dataflow_list);
  } else {  // out type
    fus_node->GetFusionOutputFlowList(fusnode_dataflow_list);
  }

  for (ge::NodePtr &node : fus_nodelist) {
    std::map<ge::AnchorPtr, ge::AnchorPtr> tmp_anchor_map;
    fusion_op_anchors_map.insert(std::make_pair(node, tmp_anchor_map));
  }

  for (int i = 0; i < static_cast<int>(fusnode_dataflow_list.size()); i++) {
    std::multimap<std::string, ge::AnchorPtr> data_flow_map = fusnode_dataflow_list[i];
    std::multimap<std::string, ge::AnchorPtr> flow_map;
    for (auto &node_data : data_flow_map) {
      ge::NodePtr node = graph_comm_impl_ptr_->FindNodeInFusNodeList(node_data.first, fus_nodelist);
      CM_CHECK(node == nullptr,
               REPORT_CM_ERROR("[SubGraphOpt][Merge][GetNdDataFlowMap] Find node:%s in fusnodelist failed.",
               node_data.first.c_str()), return FAILED);
      int anchor_idx = std::static_pointer_cast<ge::DataAnchor>(node_data.second)->GetIdx();

      ge::AnchorPtr anchor_ptr;
      if (map_type == 0) {
        anchor_ptr = node->GetInDataAnchor(anchor_idx);
        fusion_op_anchors_map[node].insert(std::make_pair(anchor_ptr, fus_node->GetInDataAnchor(i)));
      } else {
        anchor_ptr = node->GetOutDataAnchor(anchor_idx);
        fusion_op_anchors_map[node].insert(std::make_pair(anchor_ptr, fus_node->GetOutDataAnchor(i)));
      }

      // update old node name and anchor
      flow_map.insert(std::make_pair(node_data.first, anchor_ptr));
      CM_LOGD("node:%s, idx:%d, type:%d, fusopidx:%d", node_data.first.c_str(), anchor_idx, map_type, i);
    }
    fus_dataflow_list.push_back(flow_map);
  }

  return SUCCESS;
}

Status GraphComm::CopyFusionOpNodes(const vector<FusionDataFlow> &fus_input_edge_list,
                                    const vector<FusionDataFlow> &fus_output_edge_list,
                                    const vector<ge::NodePtr> &fus_nodelist,
                                    ge::OpDescPtr fusion_op_desc, ge::ComputeGraphPtr fusion_graph) const {
  vector<ge::NodePtr> edge_nodes;
  for (const FusionDataFlow &dataflow : fus_input_edge_list) {
    std::pair<ge::AnchorPtr, ge::AnchorPtr> in_edge = dataflow.edge;
    // get peer node
    ge::NodePtr peer_node = in_edge.first->GetOwnerNode();
    edge_nodes.push_back(peer_node);
  }

  for (const FusionDataFlow &dataflow : fus_output_edge_list) {
    std::pair<ge::AnchorPtr, ge::AnchorPtr> in_edge = dataflow.edge;
    // get peer node
    ge::NodePtr peer_node = in_edge.second->GetOwnerNode();
    edge_nodes.push_back(peer_node);
  }

  vector<ge::NodePtr> new_edge_nodelist;
  for (ge::NodePtr &peer_node : edge_nodes) {
    // find exist node
    ge::NodePtr new_peer_node = fusion_graph->FindNode(peer_node->GetName());
    // create new node
    if (new_peer_node == nullptr) {
      ge::OpDescPtr new_op_desc = ge::AttrUtils::CloneOpDesc(peer_node->GetOpDesc());
      new_peer_node = fusion_graph->AddNode(new_op_desc);
      CM_CHECK(new_peer_node == nullptr,
               REPORT_CM_ERROR("[SubGraphOpt][Merge][CpFusOpNd] nodeNew is nullptr"), return FAILED);
      new_edge_nodelist.push_back(new_peer_node);
    }
  }
  fusion_op_desc->SetExtAttr("ScopeEdgeNodeSet", new_edge_nodelist);

  vector<ge::NodePtr> new_fus_nodelist;
  for (const auto &node : fus_nodelist) {
    if (node == nullptr) {
      continue;
    }
    ge::OpDescPtr new_op_desc = ge::AttrUtils::CloneOpDesc(node->GetOpDesc());
    if (new_op_desc == nullptr) {
      continue;
    }
    ge::NodePtr new_node = fusion_graph->AddNode(new_op_desc);
    if (new_node == nullptr) {
      continue;
    }
    // init offset
    size_t input_size = new_op_desc->GetInputsSize();
    size_t output_size = new_op_desc->GetOutputsSize();
    vector<int64_t> input_offset(input_size, -1);
    vector<int64_t> output_offset(output_size, -1);
    new_op_desc->SetInputOffset(input_offset);
    new_op_desc->SetOutputOffset(output_offset);
    L2FusionInfoPtr origin_l2_info = nullptr;
    origin_l2_info = node->GetOpDesc()->TryGetExtAttr(ATTR_NAME_TASK_L2_FUSION_INFO_EXTEND_PTR, origin_l2_info);
    if (origin_l2_info != nullptr) {
      new_op_desc->SetExtAttr(ATTR_NAME_TASK_L2_FUSION_INFO_EXTEND_PTR, origin_l2_info);
    } else {
      CM_LOGD("node %s does not has l2_info.", node->GetName().c_str());
    }
    new_fus_nodelist.push_back(new_node);
  }
  fusion_op_desc->SetExtAttr("ScopeNodeSet", new_fus_nodelist);

  return SUCCESS;
}

Status GraphComm::CopyOutDataEdges(const ge::NodePtr &src_node,
                                   const ge::NodePtr &node,
                                   const ge::ComputeGraphPtr &fusion_graph) {
  for (auto &out_anchor : src_node->GetAllOutDataAnchors()) {
    if (out_anchor == nullptr) {
      continue;
    }
    for (auto &peer_in_anchor : out_anchor->GetPeerAnchors()) {
      if (peer_in_anchor) {
        ge::NodePtr dst_node = fusion_graph->FindNode(peer_in_anchor->GetOwnerNode()->GetName());
        // if not find, dst_node is other node in origraph,
        // src_node is output_edge_node, break.
        if (dst_node == nullptr) {
          CM_LOGI("dstNode not find in fusion_graph. name:%s, type:%s, peer_name:%s, peer_type:%s",
              node->GetName().c_str(), node->GetType().c_str(), peer_in_anchor->GetOwnerNode()->GetName().c_str(),
              peer_in_anchor->GetOwnerNode()->GetType().c_str());
          break;
        }

        Status ret =
            graph_comm_impl_ptr_->AddEdge(node->GetOutDataAnchor(out_anchor->GetIdx()), dst_node, peer_in_anchor);
        if (ret != SUCCESS) {
          REPORT_CM_ERROR("[SubGraphOpt][Merge][CpOutDataEg] Add Data Edge failed. name:%s, type:%s, idx:%d",
                          node->GetName().c_str(), node->GetType().c_str(), out_anchor->GetIdx());
          return FAILED;
        }

        CM_LOGD("Add Data Edge Success. src_name:%s, dst_name:%s, idx:%d", src_node->GetName().c_str(),
                dst_node->GetName().c_str(), out_anchor->GetIdx());
      }
    }
  }
  return SUCCESS;
}

Status GraphComm::CopyControlEdges(const ge::NodePtr &src_node,
                                   const ge::NodePtr &node,
                                   const ge::ComputeGraphPtr &fusion_graph) {
  auto out_ctrl_anchor = src_node->GetOutControlAnchor();
  CM_CHECK_NOTNULL(out_ctrl_anchor);
  for (auto &peer_in_anchor : out_ctrl_anchor->GetPeerAnchors()) {
    if (peer_in_anchor) {
      ge::NodePtr dst_node = fusion_graph->FindNode(peer_in_anchor->GetOwnerNode()->GetName());
      if (dst_node == nullptr) {
        CM_LOGI("control dst_node not find in fusion_graph. name:%s, type:%s, peer_name:%s, peer_type:%s",
            node->GetName().c_str(), node->GetType().c_str(), peer_in_anchor->GetOwnerNode()->GetName().c_str(),
            peer_in_anchor->GetOwnerNode()->GetType().c_str());
        break;
      }

      Status ret = graph_comm_impl_ptr_->AddEdge(node->GetOutControlAnchor(), dst_node, peer_in_anchor);
      if (ret != SUCCESS) {
        REPORT_CM_ERROR("[SubGraphOpt][Merge][CpCtrlEg] Add Control Edge failed. name:%s, type:%s",
                        node->GetName().c_str(), node->GetType().c_str());
        return FAILED;
      }
      CM_LOGD("Add Control Edge Success. src_name:%s, dst_name:%s", node->GetName().c_str(),
              dst_node->GetName().c_str());
    }
  }
  return SUCCESS;
}

Status GraphComm::CopyFusionOpEdges(ge::ComputeGraph &orig_graph,
                                    ge::ComputeGraphPtr fusion_graph) {
  for (ge::NodePtr &node : fusion_graph->GetDirectNode()) {
    ge::NodePtr src_node = orig_graph.FindNode(node->GetName());
    CM_CHECK(src_node == nullptr,
             REPORT_CM_ERROR("[SubGraphOpt][Merge][CpFusOpEg] not find name:%s, type:%s",
             node->GetName().c_str(), node->GetType().c_str()), return FAILED);

    Status ret = CopyOutDataEdges(src_node, node, fusion_graph);
    if (ret != SUCCESS) {
      REPORT_CM_ERROR("[SubGraphOpt][Merge][CpFusOpEg] Failed to copy out data edges for node %s.",
                      node->GetName().c_str());
      return ret;
    }


    ret = CopyControlEdges(src_node, node, fusion_graph);
    if (ret != SUCCESS) {
      REPORT_CM_ERROR("[SubGraphOpt][Merge][CpFusOpEg] Failed to copy out control edges for node %s.",
                      node->GetName().c_str());
      return ret;
    }
  }
  return SUCCESS;
}

Status GraphComm::MergeFusionNodeEdgeList(ge::NodePtr &fus_node, std::vector<ge::NodePtr> &fus_nodelist,
                                          std::vector<FusionDataFlow> &fus_input_edge_list,
                                          std::vector<FusionDataFlow> &fus_output_edge_list) {
  if (MergeFusionNodeInputEdgeList(fus_node, fus_nodelist, fus_input_edge_list) != SUCCESS) {
    REPORT_CM_ERROR("[SubGraphOpt][Merge][MrgFusNdEgList] Failed to MergeFusionNodeInputEdgeList.");
    return FAILED;
  }

  if (MergeFusionNodeOutputEdgeList(fus_node, fus_output_edge_list) != SUCCESS) {
    REPORT_CM_ERROR("[SubGraphOpt][Merge][MrgFusNdEgList] Failed to MergeFusionNodeOutputEdgeList.");
    return FAILED;
  }

  return SUCCESS;
}

Status GraphComm::MergeFusionNodeCtrlEdgeList(ge::NodePtr &fus_node,
                                              const std::vector<ge::NodePtr> &fus_nodelist,
                                              std::vector<FusionDataFlow> &fus_input_ctrl_edge_list,
                                              std::vector<FusionDataFlow> &fus_output_ctrl_edge_list) {
  if (fus_input_ctrl_edge_list.empty() && fus_output_ctrl_edge_list.empty()) {
    CM_LOGD("No Ctrl Edge In %s, list size %zu.", fus_node->GetName().c_str(),
            fus_nodelist.size());
    return SUCCESS;
  }

  if (graph_comm_impl_ptr_->MergeFusionNodeInputCtrlEdgeList(fus_node, fus_input_ctrl_edge_list) != SUCCESS) {
    REPORT_CM_ERROR("[SubGraphOpt][Merge][MrgFusNdCtrlEgList] Failed to Merge Input Ctrl Edge.");
    return FAILED;
  }

  if (graph_comm_impl_ptr_->MergeFusionNodeOutputCtrlEdgeList(fus_node, fus_output_ctrl_edge_list) != SUCCESS) {
    REPORT_CM_ERROR("[SubGraphOpt][Merge][MrgFusNdCtrlEgList] Failed to Merge Output Ctrl Edge.");
    return FAILED;
  }

  return SUCCESS;
}

Status GraphComm::MergeFusionNodeInputEdgeList(ge::NodePtr fus_node, vector<ge::NodePtr> &fus_nodelist,
                                               vector<FusionDataFlow> &fus_input_edge_list) {
  int32_t fusion_dst_index = -1;

  ClearFusionSrc();
  CM_CHECK(fus_node == nullptr,
           REPORT_CM_ERROR("[SubGraphOpt][Merge][MrgFusNdInEg] fusNode is nullptr."), return FAILED);

  for (FusionDataFlow &dataflow : fus_input_edge_list) {
    auto inedge = dataflow.edge;
    if (inedge.first == nullptr) {
      continue;
    }

    std::pair<string, ge::AnchorPtr> &node_dstindex_pair = dataflow.node_dataindex_pair;
    auto src_node = inedge.first->GetOwnerNode();
    CM_CHECK_NOTNULL(src_node);
    int64_t op_id = src_node->GetOpDesc()->GetId();

    fusion_dst_index = fusion_dst_index + 1;
    AddFusionInputSrc(op_id, inedge.first, fusion_dst_index, node_dstindex_pair);
    auto src_out_data_anchor = std::static_pointer_cast<ge::OutDataAnchor>(inedge.first);
    auto old_dst_anchor = std::static_pointer_cast<ge::InDataAnchor>(inedge.second);
    if (src_out_data_anchor->ReplacePeer(old_dst_anchor,
                                         fus_node->GetInDataAnchor(fusion_dst_index)) != ge::GRAPH_SUCCESS) {
      REPORT_CM_ERROR("[SubGraphOpt][Merge][MrgFusNdInEg] Failed to add edge between %s's output %d and %s's input %d",
                      src_node->GetName().c_str(), src_out_data_anchor->GetIdx(),
                      fus_node->GetName().c_str(), fusion_dst_index);
      return FAILED;
    }
  }

  fus_node->SetFusionInputFlowList(fusion_input_dataflow_list_);
  graph_comm_impl_ptr_->SetFusionEltwiseInputIndex(fus_node, fus_nodelist);

  return SUCCESS;
}

void GraphComm::UnlinkOldEdges(const vector<FusionDataFlow> &fus_output_edge_list) {
  ClearFusionSrc();
  ClearFusionDst();
  for (const FusionDataFlow &dataflow : fus_output_edge_list) {
    auto outedge = dataflow.edge;
    if (outedge.first == nullptr) {
      continue;
    }
    if (!outedge.first->Unlink(outedge.second)) {
      CM_LOGD("Fail to unlink output edge.");
    }
  }
}

Status GraphComm::MergeFusionNodeOutputEdgeList(ge::NodePtr fus_node,
                                                vector<FusionDataFlow> &fus_output_edge_list) {
  UnlinkOldEdges(fus_output_edge_list);
  int32_t fusion_src_index = 0;
  CM_CHECK(fus_node == nullptr,
           REPORT_CM_ERROR("[SubGraphOpt][Merge][MrgFusNdOutEgList] fusNode is nullptr."), return FAILED);
  for (FusionDataFlow &dataflow : fus_output_edge_list) {
    auto outedge = dataflow.edge;
    std::pair<string, ge::AnchorPtr> &node_srcindex_pair = dataflow.node_dataindex_pair;
    auto peer_in_node = outedge.second->GetOwnerNode();
    CM_CHECK(peer_in_node == nullptr,
             REPORT_CM_ERROR("[SubGraphOpt][Merge][MrgFusNdOutEgList] Peer in node is nullptr."), return FAILED);

    int64_t dst_op_id = peer_in_node->GetOpDesc()->GetId();
    ge::DataAnchorPtr out_edge_dst_data_anchor_ptr = std::static_pointer_cast<ge::DataAnchor>(outedge.second);

    if (IsFusionDstExist(dst_op_id, outedge.second)) {
      CM_CHECK(out_edge_dst_data_anchor_ptr == nullptr,
               REPORT_CM_ERROR("[SubGraphOpt][Merge][MrgFusNdOutEgList] outEdgeDstDataAnchorPtr is nullptr."),
               return FAILED);
      CM_LOGI("MergeFusionNodeOutputEdgeList DstId %u, DstIndex %u.", (uint32_t)dst_op_id,
              (uint32_t)out_edge_dst_data_anchor_ptr->GetIdx());
      continue;
    }
    SaveFusionDst(dst_op_id, outedge.second);

    ge::DataAnchorPtr out_edge_src_data_anchor_ptr = std::static_pointer_cast<ge::DataAnchor>(outedge.first);
    CM_CHECK(out_edge_src_data_anchor_ptr == nullptr,
             REPORT_CM_ERROR("[SubGraphOpt][Merge][MrgFusNdOutEgList] outEdgeSrcDataAnchorPtr is nullptr."),
             return FAILED);
    auto node = out_edge_src_data_anchor_ptr->GetOwnerNode();
    CM_CHECK(node == nullptr,
             REPORT_CM_ERROR("[SubGraphOpt][Merge][MrgFusNdOutEgList] node is nullptr."), return FAILED);
    ge::OpDescPtr out_edge_src_op_desc_ptr = node->GetOpDesc();
    auto dst_in_data_anchor = std::static_pointer_cast<ge::InDataAnchor>(outedge.second);
    CM_CHECK_NOTNULL(dst_in_data_anchor);
    auto dst_node = dst_in_data_anchor->GetOwnerNode();
    CM_CHECK_NOTNULL(dst_node);
    int32_t fusion_src_exist_index;
    int32_t fusion_dst_exist_index;

    auto src_node = outedge.first->GetOwnerNode();
    CM_CHECK_NOTNULL(src_node);
    int64_t src_op_id = src_node->GetOpDesc()->GetId();
    if (!GetFusionSrc(src_op_id, outedge.first, fusion_src_exist_index, fusion_dst_exist_index)) {
      AddFusionOutputSrc(src_op_id, outedge.first, fusion_src_index, node_srcindex_pair);
      if (ge::GraphUtils::AddEdge(fus_node->GetOutDataAnchor(fusion_src_index), dst_in_data_anchor) !=
          ge::GRAPH_SUCCESS) {
        REPORT_CM_ERROR("[SubGraphOpt][Merge][LinkOutput] Failed to add edge between %s's output %d and %s's input %d",
                        fus_node->GetName().c_str(), fusion_src_index,
                        dst_node->GetName().c_str(), dst_in_data_anchor->GetIdx());
        return FAILED;
      }
      fusion_src_index++;
    } else {
      if (ge::GraphUtils::AddEdge(fus_node->GetOutDataAnchor(fusion_src_exist_index), dst_in_data_anchor) !=
          ge::GRAPH_SUCCESS) {
        REPORT_CM_ERROR("[SubGraphOpt][Merge][LinkOutput] Failed to add edge between %s's output %d and %s's input %d",
                        fus_node->GetName().c_str(), fusion_src_exist_index,
                        dst_node->GetName().c_str(), dst_in_data_anchor->GetIdx());
        return FAILED;
      }
    }
  }

  fus_node->SetFusionOutputFlowList(fusion_output_dataflow_list_);

  return SUCCESS;
}

string GraphComm::GetEngineName() { return engine_name_; }

}  // namespace fe