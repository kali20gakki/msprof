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

#include "graph_optimizer/fuzzy_compiler/input_node_generalize.h"
#include <sstream>

namespace fe {
using namespace nlohmann;

InputNodeGeneralize::InputNodeGeneralize(const std::unordered_set<ge::NodePtr> &input_nodes,
                                         const bool &is_limited_graph,
                                         const std::map<ge::NodePtr, NodeGeneralInfoPtr> &node_info_map,
                                         const OpStoreAdapterPtr &op_store_adapter)
    : input_nodes_(input_nodes),
      is_limited_graph_(is_limited_graph),
      node_info_map_(node_info_map),
      op_store_adapter_(op_store_adapter) {}

InputNodeGeneralize::InputNodeGeneralize(const OpStoreAdapterPtr &op_store_adapter)
    : op_store_adapter_(op_store_adapter) {}

InputNodeGeneralize::~InputNodeGeneralize() {}

std::string RangeToString(const std::vector<std::pair<int64_t, int64_t>> &ranges) {
  bool first = true;
  std::stringstream ss;
  ss << "[";
  for (const auto &range : ranges) {
    if (first) {
      first = false;
    } else {
      ss << ",";
    }
    ss << "{";
    ss << range.first << "," << range.second;
    ss << "}";
  }
  ss << "]";
  return ss.str();
}

std::string ShapeToString(const std::vector<int64_t> &shapes) {
  bool first = true;
  std::stringstream ss;
  ss << "[";
  for (const auto &shape : shapes) {
    if (first) {
      first = false;
    } else {
      ss << ",";
    }
    ss << shape;
  }
  ss << "]";
  return ss.str();
}

std::vector<ge::ComputeGraphPtr> InputNodeGeneralize::GetSubgraphsByCurNode(const ge::NodePtr &node_ptr) const {
  std::vector<ge::ComputeGraphPtr> cur_node_subgraph;
  const std::string op_name = node_ptr->GetName();
  const auto op_desc = node_ptr->GetOpDesc();
  const auto sub_graph_names = op_desc->GetSubgraphInstanceNames();
  if (sub_graph_names.empty()) {
    FE_LOGW("[GraphOpt][Prepare][GetSubgraphsByCurNode] node[%s] do not contains subgraph name.", op_name.c_str());
    return cur_node_subgraph;
  }

  const auto root_graph = ge::GraphUtils::FindRootGraph(node_ptr->GetOwnerComputeGraph());
  for (const auto &name : sub_graph_names) {
    if (name.empty()) {
      FE_LOGW("[GraphOpt][Prepare][GetSubgraphsByCurNode] node[%s] contains empty subgraph instance name.",
              op_name.c_str());
      continue;
    }

    auto sub_graph = root_graph->GetSubgraph(name);
    if (sub_graph == nullptr) {
      FE_LOGW("[GraphOpt][Prepare][GetSubgraphsByCurNode] the graph[%s] for node[%s] is null.", name.c_str(),
              op_name.c_str());
      continue;
    }
    cur_node_subgraph.emplace_back(sub_graph);
  }
  return cur_node_subgraph;
}

void UpdateTensorDesc(const ge::GeTensorDescPtr &src, ge::GeTensorDescPtr &dst) {
  if (src == nullptr || dst == nullptr) {
    FE_LOGW("[GraphOpt][Prepare][UpdateTensorDesc] Null pointer is unexpected, please check the params.");
    return;
  }
  std::vector<int64_t> ori_shape = src->GetOriginShape().GetDims();
  std::vector<int64_t> shape = src->GetShape().GetDims();
  dst->SetOriginShape(src->GetOriginShape());
  dst->SetShape(src->GetOriginShape());
  std::vector<std::pair<int64_t, int64_t>> src_shape_range;
  src->GetOriginShapeRange(src_shape_range);
  dst->SetShapeRange(src_shape_range);
  dst->SetOriginShapeRange(src_shape_range);
  std::vector<std::pair<int64_t, int64_t>> value_range;
  src->GetValueRange(value_range);
  dst->SetValueRange(value_range);
  FE_LOGD("[GraphOpt][Prepare][UpdateTensorDesc] Ori shape is %s, shape is %s, range is %s, value_range is %s.",
          ShapeToString(ori_shape).c_str(), ShapeToString(shape).c_str(),
          RangeToString(src_shape_range).c_str(), RangeToString(value_range).c_str());
}

Status InputNodeGeneralize::MergeRangeWithUpperLimitMax(
    const std::pair<int64_t, int64_t> &upper_limit_max_range, const std::pair<int64_t, int64_t> &range,
    const size_t &dim_index, std::vector<std::pair<int64_t, int64_t>> &dst_shape_range) const {
  if (range.second != -1 && upper_limit_max_range.first > range.second) {
    FE_LOGW("[GraphOpt][Prepare][MergeRangeWithUpperLimitMax]dim[%zu], range[%ld, %ld] \
            has no intersect with [%ld,%ld].",
            dim_index, upper_limit_max_range.first, upper_limit_max_range.second, range.first, range.second);
    return FAILED;
  }
  if (dim_index >= dst_shape_range.size()) {
    FE_LOGW("[GraphOpt][Prepare][MergeRangeWithUpperLimitMax] Dim_idx[%zu] is larger than ShapeRange size[%zu].",
            dim_index, dst_shape_range.size());
    return FAILED;
  }

  dst_shape_range[dim_index].first = std::max(upper_limit_max_range.first, range.first);
  dst_shape_range[dim_index].second = range.second;

  return SUCCESS;
}

Status InputNodeGeneralize::MergeRange(const std::vector<std::pair<int64_t, int64_t>> &src_range,
                                       std::vector<std::pair<int64_t, int64_t>> &dst_range) const {
  if (dst_range.empty()) {
    dst_range.assign(src_range.begin(), src_range.end());
    return SUCCESS;
  }

  if (dst_range.size() != src_range.size()) {
    FE_LOGW("[GraphOpt][Prepare][Generalize] dst range size[%zu] not equal src range size[%zu].",
            dst_range.size(), src_range.size());
    return FAILED;
  }

  Status ret;
  for (size_t i = 0; i < src_range.size(); ++i) {
    if (src_range[i].second == -1) {
      ret = MergeRangeWithUpperLimitMax(src_range[i], dst_range[i], i, dst_range);
      if (ret != SUCCESS) {
        FE_LOGW("[GraphOpt][Prepare][Generalize] Merge dim[%zu] range with upper limit max failed.", i);
        return FAILED;
      }
      continue;
    }

    if (dst_range[i].second == -1) {
      ret = MergeRangeWithUpperLimitMax(dst_range[i], src_range[i], i, dst_range);
      if (ret != SUCCESS) {
        FE_LOGW("[GraphOpt][Prepare][Generalize] Merge dim[%zu] range with upper limit max failed.", i);
        return FAILED;
      }
      continue;
    }

    const bool no_intersect = (src_range[i].first > dst_range[i].second || src_range[i].second < dst_range[i].first);
    if (no_intersect) {
      FE_LOGW("[GraphOpt][Prepare][Generalize] dim:%zu, src range[%ld, %ld] has no intersect with dst range[%ld, %ld].",
              i, src_range[i].first, src_range[i].second, dst_range[i].first, dst_range[i].second);
      return FAILED;
    }

    dst_range[i].first = std::max(src_range[i].first, dst_range[i].first);
    dst_range[i].second = std::min(src_range[i].second, dst_range[i].second);
  }

  return SUCCESS;
}

Status InputNodeGeneralize::MergeTensorDesc(const ge::GeTensorDescPtr &src, const ge::GeTensorDescPtr &dst) const {
  Status ret;
  auto ori_shape = src->GetOriginShape();
  dst->SetOriginShape(ori_shape);
  dst->SetShape(ori_shape);
  std::vector<std::pair<int64_t, int64_t>> src_shape_range;
  std::vector<std::pair<int64_t, int64_t>> dst_shape_range;
  std::vector<std::pair<int64_t, int64_t>> src_value_range;
  std::vector<std::pair<int64_t, int64_t>> dst_value_range;
  src->GetOriginShapeRange(src_shape_range);
  dst->GetOriginShapeRange(dst_shape_range);
  src->GetValueRange(src_value_range);
  dst->GetValueRange(dst_value_range);

  FE_LOGD("[GraphOpt][Prepare][MergeTensorDesc] ori shape is %s, src range is %s, dst range is %s.",
          ShapeToString(ori_shape.GetDims()).c_str(), RangeToString(src_shape_range).c_str(),
          RangeToString(dst_shape_range).c_str());
  ret = MergeRange(src_shape_range, dst_shape_range);
  if (ret != SUCCESS) {
    FE_LOGW("[GraphOpt][Prepare][MergeTensorDesc] merge shape range of %s and %s failed.",
            RangeToString(src_shape_range).c_str(), RangeToString(dst_shape_range).c_str());
    return FAILED;
  }
  dst->SetShapeRange(dst_shape_range);
  dst->SetOriginShapeRange(dst_shape_range);

  FE_LOGD("[GraphOpt][Prepare][MergeTensorDesc] src_value_range is %s, dst_value_range is %s.",
          RangeToString(src_value_range).c_str(), RangeToString(dst_value_range).c_str());
  ret = MergeRange(src_value_range, dst_value_range);
  if (ret != SUCCESS) {
    FE_LOGW("[GraphOpt][Prepare][MergeTensorDesc] merge value range of %s and %s failed.",
            RangeToString(src_value_range).c_str(), RangeToString(dst_value_range).c_str());
    return FAILED;
  }
  dst->SetValueRange(dst_value_range);

  return SUCCESS;
}

Status InputNodeGeneralize::GetParentNodeBySubGraphNode(const ge::NodePtr &sub_node, ge::NodePtr &parent_node) const {
  ge::OpDescPtr data_op_desc_ptr = sub_node->GetOpDesc();
  FE_CHECK_NOTNULL(data_op_desc_ptr);
  uint32_t parent_node_index = 0;
  if (!ge::AttrUtils::GetInt(data_op_desc_ptr, ge::ATTR_NAME_PARENT_NODE_INDEX, parent_node_index)) {
    FE_LOGW("[GraphOpt][Prepare][Generalize] attr[%s] is missing for node[%s].",
            ge::ATTR_NAME_PARENT_NODE_INDEX.c_str(), data_op_desc_ptr->GetName().c_str());
    return FAILED;
  }

  auto owner_graph = sub_node->GetOwnerComputeGraph();
  FE_CHECK_NOTNULL(owner_graph);

  ge::NodePtr parent_node_ptr = owner_graph->GetParentNode();
  FE_CHECK_NOTNULL(parent_node_ptr);
  auto in_data_anchor = parent_node_ptr->GetInDataAnchor(parent_node_index);
  FE_CHECK_NOTNULL(in_data_anchor);
  auto out_data_anchor = in_data_anchor->GetPeerOutAnchor();
  FE_CHECK_NOTNULL(out_data_anchor);

  parent_node = out_data_anchor->GetOwnerNode();
  FE_LOGD(
      "[GraphOpt][Prepare][GetParentNodeBySubGraphNode] Finish to get parent_tensor_desc \
  by sub graph. parent_node[%s], parent_graph[%s].",
      parent_node->GetName().c_str(), owner_graph->GetName().c_str());

  return SUCCESS;
}

Status InputNodeGeneralize::LimitedNodeGeneralize(const ge::NodePtr &limited_node,
                                                  const NodeGeneralInfoPtr &node_info_ptr) const {
  const std::string op_name = limited_node->GetName();
  Status ret = op_store_adapter_->GeneralizeNode(limited_node, *(node_info_ptr->op_info), te::REGISTER_FUNC);
  if (ret != SUCCESS) {
    FE_LOGW("[GraphOpt][Prepare][Generalize] limited node[%s] generalize failed.", op_name.c_str());
    return FAILED;
  }

  return SUCCESS;
}

Status InputNodeGeneralize::UnlimitedNodeGeneralize(const ge::NodePtr &unlimited_node,
                                                    const NodeGeneralInfoPtr &node_info_ptr) const {
  Status ret;
  const std::string op_name = unlimited_node->GetName();

  if (node_info_ptr->is_found_in_opstore) {
    FE_LOGD("[GraphOpt][Prepare][UnlimitedNodeGeneralize] node[%s] is not found in op_store,\
        generalize_type is [%d].", op_name.c_str(), te::DEFAULT_LIMITED_TBE_OP_INFO);
    ret = op_store_adapter_->GeneralizeNode(unlimited_node, *(node_info_ptr->op_info),
                                            te::DEFAULT_LIMITED_TBE_OP_INFO);
    if (ret != SUCCESS) {
      FE_LOGW("[GraphOpt][Prepare][UnlimitedNodeGeneralize] node[%s] generalize with default rule failed.",
              op_name.c_str());
      return FAILED;
    }
  } else {
    FE_LOGD("[GraphOpt][Prepare][UnlimitedNodeGeneralize] node[%s] is found in op_store,\
        generalize_type is [%d].", op_name.c_str(), te::DEFAULT_NODE);
    ret = op_store_adapter_->GeneralizeNode(unlimited_node, *(node_info_ptr->op_info), te::DEFAULT_NODE);
    if (ret != SUCCESS) {
      FE_LOGW("[GraphOpt][Prepare][UnlimitedNodeGeneralize] node[%s] generalize failed.", op_name.c_str());
      return FAILED;
    }
  }
  return SUCCESS;
}

Status InputNodeGeneralize::UpdateSubGraphInputToRootGraph(const std::unordered_set<ge::NodePtr> &sub_graph_input_nodes,
                                                           const ge::ComputeGraphPtr &sub_graph) const {
  for (const auto &sub_graph_input_node : sub_graph_input_nodes) {
    ge::GeTensorDescPtr parent_input;
    ge::GeTensorDescPtr parent_output;
    ge::NodePtr parent_node;
    FE_LOGD("[GraphOpt][Prepare][UpdateSubGraphInputToRootGraph] Begin to update subgraph[%s] node[%s].",
            sub_graph->GetName().c_str(), sub_graph_input_node->GetName().c_str());
    if (GetParentNodeBySubGraphNode(sub_graph_input_node, parent_node) != SUCCESS) {
      FE_LOGW("[GraphOpt][Prepare][UpdateSubGraphInputToRootGraph] subgraph[%s] node[%s] get parent_td failed.",
              sub_graph->GetName().c_str(), sub_graph_input_node->GetName().c_str());
      return FAILED;
    }

    parent_input = parent_node->GetOpDesc()->MutableInputDesc(0);
    parent_output = parent_node->GetOpDesc()->MutableOutputDesc(0);

    auto data_input = sub_graph_input_node->GetOpDesc()->MutableInputDesc(0);
    UpdateTensorDesc(data_input, parent_input);

    auto data_output = sub_graph_input_node->GetOpDesc()->MutableOutputDesc(0);
    UpdateTensorDesc(data_output, parent_output);

    bool is_value_depend = false;
    (void)ge::AttrUtils::GetBool(sub_graph_input_node->GetOpDesc(), ge::ATTR_NAME_VALUE_DEPEND, is_value_depend);
    if (is_value_depend) {
      if (!ge::AttrUtils::SetBool(parent_node->GetOpDesc(), ge::ATTR_NAME_VALUE_DEPEND, is_value_depend)) {
        FE_LOGW("[GraphOpt][Prepare][UpdateSubGraphInputToRootGraph] Node[%s] set is_value_depend failed.",
                parent_node->GetName().c_str());
        return FAILED;
      }
    }
  }

  return SUCCESS;
}

Status InputNodeGeneralize::GeneralizeSubGraphs(const ge::NodePtr &root_graph_first_node) {
  Status ret;
  std::unordered_set<ge::NodePtr> sub_graph_input_nodes;

  for (const auto &sub_graph : GetSubgraphsByCurNode(root_graph_first_node)) {
    for (const auto &node_sub : sub_graph->GetDirectNode()) {
      if (node_sub->GetType() != DATA) {
        continue;
      }

      sub_graph_input_nodes.insert(node_sub);
    }

    InputNodeGeneralize sub_graph_fuzzy_generalize(sub_graph_input_nodes, is_limited_graph_, node_info_map_,
                                                   op_store_adapter_);
    ret = sub_graph_fuzzy_generalize.GeneralizeAllInputNodesInGraph();
    if (ret != SUCCESS) {
      FE_LOGW("[GraphOpt][Prepare][Generalize] sub graph[%s] generalize failed.", sub_graph->GetName().c_str());
      return FAILED;
    }

    ret = UpdateSubGraphInputToRootGraph(sub_graph_input_nodes, sub_graph);
    if (ret != SUCCESS) {
      FE_LOGW("[GraphOpt][Prepare][Generalize] update subGraph[%s] input to rootGraph failed.",
              sub_graph->GetName().c_str());
      return FAILED;
    }
  }

  return SUCCESS;
}

Status InputNodeGeneralize::UpdateFirstNodeTensorDescToInputNodes(const ge::NodePtr &first_node) {
  for (const auto &in_anchor : first_node->GetAllInDataAnchors()) {
    const int32_t in_index = in_anchor->GetIdx();
    const auto in_tensor_desc = first_node->GetOpDesc()->MutableInputDesc(in_index);
    if (in_tensor_desc == nullptr) {
      continue;
    }
    auto peer_anchor = in_anchor->GetPeerOutAnchor();
    if (peer_anchor == nullptr) {
      continue;
    }
    const auto input_node = peer_anchor->GetOwnerNode();
    if (std::find(input_nodes_.begin(), input_nodes_.end(), input_node) == input_nodes_.end()) {
      continue;
    }

    const auto peer_anchor_opdesc = input_node->GetOpDesc();
    if (peer_anchor_opdesc == nullptr) {
      continue;
    }

    const int32_t out_index = peer_anchor->GetIdx();
    const auto peer_output_desc = peer_anchor_opdesc->MutableOutputDesc(out_index);
    if (peer_output_desc == nullptr) {
      continue;
    }

    auto peer_input_desc = peer_anchor_opdesc->MutableInputDesc(0);
    if (peer_input_desc == nullptr) {
      continue;
    }

    if (ge::AttrUtils::HasAttr(in_tensor_desc, ge::ATTR_NAME_VALUE) &&
        ge::AttrUtils::HasAttr(peer_input_desc, ge::ATTR_NAME_VALUE)) {
      FE_LOGD(
          "[GraphOpt][Prepare][UpdateFirstNodeTensorDescToInputNodes] Peer node[%s] optype is %s, \
      has ATTR_NAME_VALUE.",
          input_node->GetOpDesc()->GetName().c_str(), input_node->GetType().c_str());
      continue;
    }

    FE_LOGD(
        "[GraphOpt][Prepare][UpdateFirstNodeTensorDescToInputNodes] Begin to merge first \
    node[name:%s, index:%d] and node[name:%s, index:%d].",
        first_node->GetName().c_str(), in_index, input_node->GetName().c_str(), out_index);
    if (MergeTensorDesc(in_tensor_desc, peer_output_desc) != SUCCESS) {
      FE_LOGW(
          "[GraphOpt][Prepare][UpdateFirstNodeTensorDescToInputNodes] node[%s, index:%d] and \
      node[%s, index:%d] mergeTensorDesc failed.",
          first_node->GetName().c_str(), in_index, input_node->GetName().c_str(), out_index);
      return FAILED;
    }
    UpdateTensorDesc(peer_output_desc, peer_input_desc);
    FE_LOGD("[GraphOpt][Prepare][UpdateFirstNodeTensorDescToInputNodes] Update successfully.");

    if (ge::AttrUtils::HasAttr(peer_input_desc, ge::ATTR_NAME_VALUE)) {
      peer_input_desc->DelAttr(ge::ATTR_NAME_VALUE);
      FE_LOGD(
          "[GraphOpt][Prepare][UpdateFirstNodeTensorDescToInputNodes] Peer node[%s] optype is %s, \
      has ATTR_NAME_VALUE.",
          input_node->GetOpDesc()->GetName().c_str(), input_node->GetType().c_str());
    }
  }

  return SUCCESS;
}

Status InputNodeGeneralize::SetValueDependFlagToInputNodes(const ge::NodePtr &first_node,
                                                           const NodeGeneralInfoPtr &node_info_ptr) const {
  auto op_kernel_ptr = node_info_ptr->op_kernel;
  if (op_kernel_ptr == nullptr) {
    FE_LOGW("[GraphOpt][Prepare][SetValueDependFlagToInputNodes] OpkernelPtr from node[%s] is nullptr.",
            first_node->GetName().c_str());
    return SUCCESS;
  }

  for (const auto &in_anchor : first_node->GetAllInDataAnchors()) {
    const std::string input_name = first_node->GetOpDesc()->GetInputNameByIndex(in_anchor->GetIdx());
    InputOrOutputInfoPtr input_info_ptr = nullptr;
    if ((op_kernel_ptr->GetInputInfoByName(input_name, input_info_ptr) != SUCCESS) || input_info_ptr == nullptr) {
      FE_LOGW("[GraphOpt][Prepare][SetValueDependFlagToInputNodes] input_info_ptr of node[%s:%s] get failed.",
              first_node->GetName().c_str(), input_name.c_str());
      continue;
    }
    OpConstValueDepend value_depend = input_info_ptr->GetConstValueDepend();
    if (value_depend == CONST_IGNORE) {
      continue;
    }

    auto peer_anchor = in_anchor->GetPeerOutAnchor();
    if (peer_anchor == nullptr) {
      continue;
    }
    const auto input_node = peer_anchor->GetOwnerNode();
    if (std::find(input_nodes_.begin(), input_nodes_.end(), input_node) == input_nodes_.end()) {
      continue;
    }

    const auto peer_anchor_opdesc = input_node->GetOpDesc();
    if (peer_anchor_opdesc == nullptr) {
      continue;
    }

    if (!ge::AttrUtils::SetBool(peer_anchor_opdesc, ge::ATTR_NAME_VALUE_DEPEND, true)) {
      FE_LOGW("[GraphOpt][Prepare][SetValueDependFlagToInputNodes] Node[%s] set is_value_depend failed.",
              first_node->GetName().c_str());
    }
  }

  return SUCCESS;
}

Status InputNodeGeneralize::GeneralizeFirstNodeOfGraph(ge::NodePtr &first_node) {
  Status ret;
  const std::string op_name = first_node->GetName();

  const auto sub_graph_names = first_node->GetOpDesc()->GetSubgraphInstanceNames();
  if (!sub_graph_names.empty()) {
    return GeneralizeSubGraphs(first_node);
  }
  std::map<ge::NodePtr, NodeGeneralInfoPtr>::const_iterator iter = node_info_map_.find(first_node);
  if (iter == node_info_map_.end()) {
    FE_LOGW("[GraphOpt][Prepare][GeneralizeFirstNodeOfGraph] not find op info for node[%s].", op_name.c_str());
    return FAILED;
  }
  NodeGeneralInfoPtr node_info_ptr = iter->second;
  FE_CHECK_NOTNULL(node_info_ptr);
  if (!is_limited_graph_) {
    te::TE_GENERALIZE_TYPE generalize_type_tmp;
    generalize_type_tmp = node_info_ptr->is_found_in_opstore ? te::DEFAULT_TBE_OP_INFO : te::DEFAULT_NODE;
    if (op_store_adapter_->GeneralizeNode(first_node, *(node_info_ptr->op_info), generalize_type_tmp) != SUCCESS) {
      FE_LOGW("[GraphOpt][Prepare][GeneralizeFirstNodeOfGraph] Node[%s] generalize failed, is_limited_graph[false].",
              op_name.c_str());
      return FAILED;
    }
  } else {
    ret = GeneralizeOneNode(first_node, iter->second);
    if (ret != SUCCESS) {
      return FAILED;
    }
  }

  ret = UpdateFirstNodeTensorDescToInputNodes(first_node);
  if (ret != SUCCESS) {
    FE_LOGW("[GraphOpt][Prepare][GeneralizeFirstNodeOfGraph] update tensor desc of node[%s] to inpeer node failed.",
            op_name.c_str());
    return FAILED;
  }

  ret = SetValueDependFlagToInputNodes(first_node, node_info_ptr);
  if (ret != SUCCESS) {
    FE_LOGW("[GraphOpt][Prepare][GeneralizeFirstNodeOfGraph] set valuedepend flag of node[%s] to inpeer node failed.",
            op_name.c_str());
    return FAILED;
  }

  return SUCCESS;
}

Status InputNodeGeneralize::GeneralizeAllInputNodesInGraph() {
  Status ret;
  FE_TIMECOST_START(GeneralizeAllInputNodesInGraph);
  for (const auto &input_node : input_nodes_) {
    FE_LOGD("[GraphOpt][Prepare][GeneralizeAllInputNodesInGraph] Input node is %s.", input_node->GetName().c_str());
    for (auto &out_node : input_node->GetOutDataNodes()) {
      FE_LOGD("[GraphOpt][Prepare][GeneralizeAllInputNodesInGraph] Output node is %s.", out_node->GetName().c_str());
      if (prime_nodes_.count(out_node) != 0) {
        continue;
      }
      ret = GeneralizeFirstNodeOfGraph(out_node);
      if (ret != SUCCESS) {
        FE_LOGW("[GraphOpt][Prepare][GeneralizeAllInputNodesInGraph] node[%s] generalize failed.",
                out_node->GetName().c_str());
        return FAILED;
      }
      prime_nodes_.emplace(out_node);
    }
  }

  FE_LOGD("[GraphOpt][Prepare][GeneralizeAllInputNodesInGraph] Generalize all input nodes success.");
  FE_TIMECOST_END(GeneralizeAllInputNodesInGraph, "InputNodeGeneralize::GeneralizeAllInputNodesInGraph")
  return SUCCESS;
}

Status InputNodeGeneralize::GeneralizeOneNode(const ge::NodePtr &node_ptr,
                                              const NodeGeneralInfoPtr &node_info_ptr) const {
  Status ret;
  if (!node_info_ptr->is_limited_range) {
    ret = UnlimitedNodeGeneralize(node_ptr, node_info_ptr);
    if (ret != SUCCESS) {
      FE_LOGW("[GraphOpt][Prepare][GeneralizeFirstNodeOfGraph] unlimited node[%s] generalize failed.",
              node_ptr->GetName().c_str());
      return FAILED;
    }
  } else {
    ret = LimitedNodeGeneralize(node_ptr, node_info_ptr);
    if (ret != SUCCESS) {
      FE_LOGW("[GraphOpt][Prepare][GeneralizeFirstNodeOfGraph] limited node[%s] generalize failed.",
              node_ptr->GetName().c_str());
      return FAILED;
    }
  }
  return SUCCESS;
}
}  // namespace fe
