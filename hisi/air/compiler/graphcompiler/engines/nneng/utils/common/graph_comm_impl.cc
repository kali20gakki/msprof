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

#include "common/graph_comm_impl.h"

#include "common/comm_log.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/type_utils.h"

namespace fe {
GraphCommImpl::GraphCommImpl() {}
GraphCommImpl::~GraphCommImpl() {}

Status GraphCommImpl::GetAllInEdgeList(const ge::NodePtr &node,
                                       std::vector<std::pair<ge::AnchorPtr, ge::AnchorPtr>> &in_edge_pair,
                                       const int32_t &edge_type) const {
  // data anchor
  if (edge_type == 0) {
    for (size_t i = 0; i < node->GetAllInDataAnchors().size(); i++) {
      auto in_data_anchor = node->GetInDataAnchor(i);
      CM_CHECK(in_data_anchor == nullptr,
               REPORT_CM_ERROR("[SubGraphOpt][Merge][GetAllInEgList]] inDataAnchor is nullptr."), return FAILED);
      auto pre_out_data_anchor = in_data_anchor->GetPeerOutAnchor();
      if (pre_out_data_anchor != nullptr) {
        in_edge_pair.push_back(make_pair(pre_out_data_anchor, in_data_anchor));
      }
    }
  } else {
    ge::InControlAnchorPtr in_ctrl_anchor = node->GetInControlAnchor();
    CM_CHECK(in_ctrl_anchor == nullptr,
             REPORT_CM_ERROR("[SubGraphOpt][Merge][GetAllInEgList]] inCtrlAnchor is nullptr."), return FAILED);
    for (auto &pre_out_data_anchor : in_ctrl_anchor->GetPeerOutControlAnchors()) {
      in_edge_pair.push_back(make_pair(pre_out_data_anchor, in_ctrl_anchor));
    }
  }

  return SUCCESS;
}

Status GraphCommImpl::GetAllOutEdgeList(const ge::NodePtr &node,
                                        std::vector<std::pair<ge::AnchorPtr, ge::AnchorPtr>> &out_edge_pair,
                                        const int32_t &edge_type) const {
  if (edge_type == 0) {
    for (auto &out_data_anchor : node->GetAllOutDataAnchors()) {
      CM_CHECK(out_data_anchor == nullptr,
               REPORT_CM_ERROR("[SubGraphOpt][Merge][GetAllOutEgList] outDataAnchor is nullptr."), return FAILED);
      for (auto &next_in_data_anchor : out_data_anchor->GetPeerInDataAnchors()) {
        out_edge_pair.push_back(make_pair(out_data_anchor, next_in_data_anchor));
      }
    }
    CM_LOGD("Get data anchors, size:%zu", out_edge_pair.size());
  } else {
    ge::OutControlAnchorPtr out_ctrl_anchor = node->GetOutControlAnchor();
    CM_CHECK(out_ctrl_anchor == nullptr,
             REPORT_CM_ERROR("[SubGraphOpt][Merge][GetAllOutEgList] outCtrlAnchor is nullptr."), return FAILED);
    for (auto &next_in_data_anchor : out_ctrl_anchor->GetPeerInControlAnchors()) {
      out_edge_pair.push_back(make_pair(out_ctrl_anchor, next_in_data_anchor));
    }
    CM_LOGD("Get ctrl anchors, size:%zu", out_edge_pair.size());
  }

  return SUCCESS;
}

void GraphCommImpl::PutEdgeToFusionDataFlowVec(const vector<ge::NodePtr> &fus_nodelist,
                                               const std::pair<ge::AnchorPtr, ge::AnchorPtr> &edge,
                                               const ge::AnchorPtr &fus_node_anchor,
                                               const ge::AnchorPtr &edge_node_anchor,
                                               std::vector<FusionDataFlow> &fus_edge_list) const {
  string dst_name = edge_node_anchor->GetOwnerNode()->GetName();
  bool is_inner_input = IsInfusNodeList(dst_name, fus_nodelist);
  if (!is_inner_input) {
    FusionDataFlow flow;
    // use uint32 data type so save index
    std::pair<string, ge::AnchorPtr> input_pair;
    input_pair.first = fus_node_anchor->GetOwnerNode()->GetName();
    input_pair.second = fus_node_anchor;
    flow.edge = edge;
    flow.node_dataindex_pair = input_pair;
    fus_edge_list.push_back(flow);
    CM_LOGD("Put edge to flow vec, fus_node:%s edge_node:%s", input_pair.first.c_str(), dst_name.c_str());
  }
}

void GraphCommImpl::SaveFusionNode(const uint32_t &scopeid,
                                   ScopeNodeMap &fus_node_map,
                                   ge::NodePtr node) const {
  ScopeNodeMap::iterator nodelist_it = fus_node_map.find(scopeid);

  if (nodelist_it == fus_node_map.end()) {
    vector<ge::NodePtr> node_list_new;
    node_list_new.clear();
    node_list_new.push_back(node);
    fus_node_map.insert(k_scope_node_pair_t(scopeid, node_list_new));
  } else {
    nodelist_it->second.push_back(node);
  }
}

void GraphCommImpl::AddFusionSrc(const uint32_t &src_op_id, const ge::AnchorPtr &src_anchor,
                                 const int32_t &fusion_src_index, const int32_t &fusion_dst_index,
                                 std::vector<FusionOpSrc> &exist_fusion_src_list) const {
  FusionOpSrc value;
  value.src_op_id = src_op_id;
  value.src_anchor = src_anchor;
  value.fusion_src_index = fusion_src_index;
  value.fusion_dst_index = fusion_dst_index;
  exist_fusion_src_list.push_back(value);
}

Status GraphCommImpl::MergeFusionNodeInputCtrlEdgeList(const ge::NodePtr &fus_node,
                                                       vector<FusionDataFlow> &fus_input_ctrl_edge_list) const {
  CM_CHECK(fus_node == nullptr,
           REPORT_CM_ERROR("[SubGraphOpt][Merge][LinkInCtrlEdge] fusNode is nullptr."), return FAILED);

  for (FusionDataFlow &data_flow : fus_input_ctrl_edge_list) {
    auto in_edge = data_flow.edge;
    in_edge.first->Unlink(in_edge.second);
    auto out_ctrl_anchor = std::static_pointer_cast<ge::OutControlAnchor>(in_edge.first);
    auto in_ctrl_anchor = std::static_pointer_cast<ge::InControlAnchor>(in_edge.second);
    CM_LOGD("Del in ctrl anchor from:%s to %s", out_ctrl_anchor->GetOwnerNode()->GetName().c_str(),
            in_ctrl_anchor->GetOwnerNode()->GetName().c_str());
  }

  for (FusionDataFlow &data_flow : fus_input_ctrl_edge_list) {
    auto in_edge = data_flow.edge;
    auto src_out_ctrl_anchor = std::static_pointer_cast<ge::OutControlAnchor>(in_edge.first);
    CM_CHECK_NOTNULL(src_out_ctrl_anchor);
    auto src_node = src_out_ctrl_anchor->GetOwnerNode();
    CM_CHECK_NOTNULL(src_node);
    if (ge::GraphUtils::AddEdge(src_out_ctrl_anchor, fus_node->GetInControlAnchor()) != ge::GRAPH_SUCCESS) {
      REPORT_CM_ERROR("[SubGraphOpt][Merge][LinkInCtrlEdge] Failed to add edge \
                      between %s's output %d and %s's input %d",
                      src_node->GetName().c_str(), src_out_ctrl_anchor->GetIdx(),
                      fus_node->GetName().c_str(), fus_node->GetInControlAnchor()->GetIdx());
      return FAILED;
    }
    CM_LOGD("Add in ctrl anchor from:%s to %s", src_out_ctrl_anchor->GetOwnerNode()->GetName().c_str(),
            fus_node->GetName().c_str());
  }

  return SUCCESS;
}

Status GraphCommImpl::MergeFusionNodeOutputCtrlEdgeList(const ge::NodePtr &fus_node,
                                                        const vector<FusionDataFlow> &fus_output_ctrl_edge_list) const
{
  CM_CHECK(fus_node == nullptr, REPORT_CM_ERROR("[SubGraphOpt][Merge][GetNodes] fusNode is nullptr."), return FAILED);

  for (const FusionDataFlow &data_flow : fus_output_ctrl_edge_list) {
    auto out_edge = data_flow.edge;
    out_edge.first->Unlink(out_edge.second);
    auto out_ctrl_anchor = std::static_pointer_cast<ge::OutControlAnchor>(out_edge.first);
    auto in_ctrl_anchor = std::static_pointer_cast<ge::InControlAnchor>(out_edge.second);
    CM_LOGD("Del out ctrl anchor from:%s to %s", out_ctrl_anchor->GetOwnerNode()->GetName().c_str(),
            in_ctrl_anchor->GetOwnerNode()->GetName().c_str());
  }

  for (const FusionDataFlow &data_flow : fus_output_ctrl_edge_list) {
    auto out_edge = data_flow.edge;
    ge::InControlAnchorPtr in_edge_ctrl_anchor_ptr = std::static_pointer_cast<ge::InControlAnchor>(out_edge.second);
    CM_CHECK_NOTNULL(fus_node->GetOutControlAnchor());
    CM_CHECK_NOTNULL(in_edge_ctrl_anchor_ptr);
    auto dst_node = in_edge_ctrl_anchor_ptr->GetOwnerNode();
    CM_CHECK_NOTNULL(dst_node);

    if (ge::GraphUtils::AddEdge(fus_node->GetOutControlAnchor(), in_edge_ctrl_anchor_ptr) != ge::GRAPH_SUCCESS) {
      REPORT_CM_ERROR("[SubGraphOpt][Merge][LinkOutCtrlEdge]Failed to add edge between %s's out %d and %s's in %d",
                      fus_node->GetName().c_str(), fus_node->GetOutControlAnchor()->GetIdx(),
                      dst_node->GetName().c_str(), in_edge_ctrl_anchor_ptr->GetIdx());
      return FAILED;
    }
    CM_LOGD("Add out ctrl anchor from:%s to %s", fus_node->GetName().c_str(),
            in_edge_ctrl_anchor_ptr->GetOwnerNode()->GetName().c_str());
  }

  return SUCCESS;
}

ge::NodePtr GraphCommImpl::FindNodeInFusNodeList(const string &node_name,
                                                 const vector<ge::NodePtr> &fus_nodelist) const {
  for (const ge::NodePtr &node : fus_nodelist) {
    CM_CHECK(node == nullptr, REPORT_CM_ERROR("[SubGraphOpt][Merge][FindFusNdListNd] node is null."), return nullptr);
    if (node->GetName() == node_name) {
      return node;
    }
  }
  return nullptr;
}

void GraphCommImpl::SetFusionEltwiseInputIndex(const ge::NodePtr &fus_node,
                                               const vector<ge::NodePtr> &fus_nodelist) const {
  CM_CHECK(fus_node == nullptr, REPORT_CM_ERROR("[SubGraphOpt][Merge][FindFusNdListNd] fusnode is nullptr."), return);

  std::string fusiontype;
  fusiontype.clear();
  for (ge::NodePtr node : fus_nodelist) {
    CM_CHECK(node == nullptr,
             REPORT_CM_ERROR("[SubGraphOpt][Merge][FindFusNdListNd] Node in fus_nodelist is null."), return);
    fusiontype += node->GetOpDesc()->GetType();
  }

  if (fusiontype != "ConvolutionEltwise") {
    CM_LOGD("Fusion type is not ConvolutionEltwise.");
    return;
  }

  ge::kFusionDataFlowVec_t fusion_input_list;
  fus_node->GetFusionInputFlowList(fusion_input_list);

  for (ge::NodePtr node : fus_nodelist) {
    CM_CHECK(node == nullptr,
             REPORT_CM_ERROR("[SubGraphOpt][Merge][FindFusNdListNd] Node in fus_nodelist is null."), return);
    if (node->GetOpDesc()->GetType() != "Eltwise") {
      continue;
    }

    for (size_t loop = 0; loop < fusion_input_list.size(); loop++) {
      std::multimap<std::string, ge::AnchorPtr> in_mmp = fusion_input_list[loop];
      for (auto iter = in_mmp.begin(); iter != in_mmp.end(); iter++) {
        if (iter->first == node->GetName()) {
          (void)ge::AttrUtils::SetInt(fus_node->GetOpDesc(), "fusion_eltwise_input_index", loop);
          CM_LOGI("Eltwise input index is %zu.", loop);
          return;
        }
      }
    }
  }
}

bool GraphCommImpl::IsInfusNodeList(const string &nodename, const vector<ge::NodePtr> &fus_nodelist) const {
  for (ge::NodePtr node : fus_nodelist) {
    CM_CHECK(node == nullptr, REPORT_CM_ERROR("[SubGraphOpt][Merge][IsInFusNdList] node is nullptr."), return false);
    if (node->GetName() == nodename) {
      return true;
    }
  }
  return false;
}

Status GraphCommImpl::AddEdge(ge::AnchorPtr src_anchor, ge::NodePtr dst_node, ge::AnchorPtr old_dst_anchor) const {
  if (src_anchor == nullptr || dst_node == nullptr || old_dst_anchor == nullptr) {
    return FAILED;
  }

  auto old_dst_data_anchor = ge::Anchor::DynamicAnchorCast<ge::InDataAnchor>(old_dst_anchor);
  auto old_dst_ctrl_anchor = ge::Anchor::DynamicAnchorCast<ge::InControlAnchor>(old_dst_anchor);
  auto src_data_anchor = ge::Anchor::DynamicAnchorCast<ge::OutDataAnchor>(src_anchor);

  if (src_data_anchor) {
    if (old_dst_data_anchor) {
      ge::graphStatus ret =
          ge::GraphUtils::AddEdge(src_data_anchor, dst_node->GetInDataAnchor(old_dst_data_anchor->GetIdx()));
      if (ret == ge::GRAPH_FAILED) {
        REPORT_CM_ERROR("[SubGraphOpt][Merge][AddEg] AddEdge failed.");
        return FAILED;
      }
    }
  }
  auto src_ctrl_anchor = ge::Anchor::DynamicAnchorCast<ge::OutControlAnchor>(src_anchor);
  if (src_ctrl_anchor && old_dst_ctrl_anchor) {
    ge::graphStatus ret = ge::GraphUtils::AddEdge(src_ctrl_anchor, dst_node->GetInControlAnchor());
    if (ret == ge::GRAPH_FAILED) {
      REPORT_CM_ERROR("[SubGraphOpt][Merge][AddEg] AddEdge failed");
      return FAILED;
    }
  }
  return SUCCESS;
}
}  // namespace fe