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

#include "ub_pass_slice_info_manager.h"
#include "common/aicore_util_attr_define.h"
#include "common/lxfusion_json_util.h"
#include "aipp_conv_slice_info.h"
#include "conv_select_slice_info.h"
#include "conv_strided_slice_info.h"
#include "conv_dequant_slice_info.h"
#include "conv_requant_slice_info.h"
#include "conv_eltwise_slice_info.h"
#include "conv_dequants16_slice_info.h"
#include "conv_requants16_slice_info.h"
#include "conv_pooling_slice_info.h"

namespace fe {
using UbPassInputOutputIndexInfo = std::unordered_map<std::string, size_t>;
static std::unordered_set<std::string> WHITELIST_OF_OP_PATTERN = {OP_PATTERN_CONV, OP_PATTERN_DEPTHWISE_CONV,
                                                                  OP_PATTERN_STRIDED_WRITE, OP_PATTERN_STRIDED_READ,
                                                                  OP_PATTERN_WRITE_SELECT, OP_PATTERN_READ_SELECT,
                                                                  OP_PATTERN_DEQUANT, OP_PATTERN_ELEMWISE, OP_PATTERN_QUANT};
static std::unordered_set<std::string> SLICE_INFO_UPDATE_OP_PATTERN = {OP_PATTERN_DEQUANT};
static std::unordered_set<std::string> SKIPPED_TYPE_FOR_SLICE_INFO = {"StridedRead", "StridedWrite",
                                                                      "ReadSelect", "WriteSelect"};

bool UbPassSliceInfoManager::CheckOpPatternSupport(const string &op_pattern) {
  return (WHITELIST_OF_OP_PATTERN.count(op_pattern) != 0);
}

bool UbPassSliceInfoManager::CheckOpPatternSliceInfoUpdate(const string &op_pattern) {
  return (SLICE_INFO_UPDATE_OP_PATTERN.count(op_pattern) != 0);
}

UbMatchedType GetMatchedPatternMode(std::string matched_pattern) {
  auto iter = op_pattern_to_matched_map.find(matched_pattern);
  if (iter == op_pattern_to_matched_map.end()) {
    return UbMatchedType::MATCHED_RESERVED;
  } else {
    return iter->second;
  }
}

bool ComparePriority(const ge::NodePtr &left, const ge::NodePtr &right) {
  std::string left_op_pattern;
  (void)ge::AttrUtils::GetStr(left->GetOpDesc(), left->GetName() + "_pattern", left_op_pattern);
  UbMatchedType left_matched_pattern = GetMatchedPatternMode(left_op_pattern);
  std::string right_op_pattern;
  (void)ge::AttrUtils::GetStr(right->GetOpDesc(), right->GetName() + "_pattern", right_op_pattern);
  UbMatchedType right_matched_pattern = GetMatchedPatternMode(right_op_pattern);
  return left_matched_pattern < right_matched_pattern;
}

bool CheckElemWiseNode(vector<ge::NodePtr> &fusion_nodes) {
  for (auto fusion_node : fusion_nodes) {
    if (fusion_node == nullptr) {
      continue;
    }
    std::string op_pattern;
    (void)ge::AttrUtils::GetStr(fusion_node->GetOpDesc(), fusion_node->GetName() + "_pattern", op_pattern);
    if (op_pattern == "ElemWise") {
      for (auto input_node : fusion_node->GetInDataNodes()) {
        if (input_node == nullptr) {
          continue;
        }
        if (input_node->GetType() == OP_TYPE_PLACE_HOLDER) {
          FE_LOGD("Node[%s] is elemwise node and has place_holder input, not set fusion slice info.",
                  fusion_node->GetName().c_str());
          return true;
        }
      }
    }
  }
  return false;
}

Status UbPassSliceInfoManager::SetSliceInfoForFusionNodes(vector<ge::NodePtr> &fusion_nodes) {
  std::string op_calc_info_str;
  fe::OpCalcInfo op_calc_info;
  if (!op_calc_info.Initialize()) {
    REPORT_FE_ERROR("[SubGraphOpt][UbSliceInfo][SetSliceInfo] op_calc_info initialize failed");
    return FAILED;
  }
  UbPassSliceInfoBasePtr slice_info_base_ptr = nullptr;
  std::sort(fusion_nodes.begin(), fusion_nodes.end(), ComparePriority);
  ge::NodePtr conv_node = nullptr;
  size_t input_size = 0;
  for (auto fusion_node : fusion_nodes) {
    if ((fusion_node->GetType() == CONV2D) || (fusion_node->GetType() == DEPTHWISECONV2D)) {
      input_size = fusion_node->GetInDataNodes().size();
      FE_LOGI("conv2d input size is %zu", input_size);
      conv_node = fusion_node;
      (void)ge::AttrUtils::GetStr(fusion_node->GetOpDesc(), OP_SLICE_INFO, op_calc_info_str);
      break;
    }
  }
  if (conv_node == nullptr) {
    FE_LOGD("Conv node is nullptr, not set fusion slice info.");
    return SUCCESS;
  }
  if (CheckElemWiseNode(fusion_nodes)) {
    return SUCCESS;
  }
  for (auto fusion_node : fusion_nodes) {
    std::string op_pattern;
    (void)ge::AttrUtils::GetStr(fusion_node->GetOpDesc(), fusion_node->GetName() + "_pattern", op_pattern);
    if (op_pattern == "bn_reduce") {
      FE_LOGD("Fusion nodes include bnreduce node, not set fusion slice info.");
      return SUCCESS;
    }
  }
  GetOpSliceInfoFromJson(op_calc_info, op_calc_info_str);
  for (auto fusion_node : fusion_nodes) {
    FE_LOGI("TRAVEL NODE name[%s] type[%s].", fusion_node->GetName().c_str(), fusion_node->GetType().c_str());
    if ((fusion_node->GetType() == CONV2D) || (fusion_node->GetType() == DEPTHWISECONV2D)) {
      FE_LOGI("TRAVEL NODE CONV name[%s] type[%s].", fusion_node->GetName().c_str(), fusion_node->GetType().c_str());
      continue;
    }
    std::string op_pattern;
    (void)ge::AttrUtils::GetStr(fusion_node->GetOpDesc(), fusion_node->GetName() + "_pattern", op_pattern);
    FE_LOGI("Get node [%s] ub fusion pattern is [%s]", fusion_node->GetName().c_str(), op_pattern.c_str());
    UbMatchedType matched_pattern = GetMatchedPatternMode(op_pattern);
    FE_LOGI("Get matched pattern is [%d] of node [%s]", static_cast<int32_t>(matched_pattern),
            fusion_node->GetName().c_str());
    slice_info_base_ptr = SwitchSliceInfoPtrByPattern(matched_pattern, fusion_node, input_size);
    FE_CHECK_NOTNULL(slice_info_base_ptr);
    bool is_head_fusion = IsHeadFusion(fusion_node, fusion_nodes);
    slice_info_base_ptr->ModifySliceInfoByPattern(fusion_node, fusion_nodes, op_calc_info, input_size,
                                                  is_head_fusion);
  }
  SetFusionOpSliceInfoToJson(op_calc_info, op_calc_info_str);
  for (auto fusion_node : fusion_nodes) {
    (void)ge::AttrUtils::SetStr(fusion_node->GetOpDesc(), FUSION_OP_SLICE_INFO, op_calc_info_str);
  }
  return SUCCESS;
}

UbPassSliceInfoBasePtr UbPassSliceInfoManager::SwitchSliceInfoPtrByPattern(UbMatchedType &ub_matched_pattern,
                                                                           ge::NodePtr &fusion_node,
                                                                           size_t &input_size) {
  UbPassSliceInfoBasePtr slice_info_base_ptr = nullptr;

  switch (ub_matched_pattern) {
    case UbMatchedType::UBMATCHTYPE_AIPP: {
      FE_MAKE_SHARED(slice_info_base_ptr = std::make_shared<AippConvSliceInfo>(), return nullptr);
      break;
    }
    case UbMatchedType::UBMATCHTYPE_READ_WRITE_SELECT: {
      FE_MAKE_SHARED(slice_info_base_ptr = std::make_shared<ConvSelectSliceInfo>(), return nullptr);
      break;
    }
    case UbMatchedType::UBMATCHTYPE_STRIDED_READ_WRITE_SELECT: {
      FE_MAKE_SHARED(slice_info_base_ptr = std::make_shared<ConvStridedSliceInfo>(), return nullptr);
      break;
    }
    case UbMatchedType::UBMATCHTYPE_DEQUANT: {
      input_size += fusion_node->GetInDataNodes().size() - 1;
      FE_MAKE_SHARED(slice_info_base_ptr = std::make_shared<ConvDequantSliceInfo>(), return nullptr);
      break;
    }
    case UbMatchedType::UBMATCHTYPE_REQUANT: {
      input_size += fusion_node->GetInDataNodes().size() - 1;
      FE_MAKE_SHARED(slice_info_base_ptr = std::make_shared<ConvRequantSliceInfo>(), return nullptr);
      break;
    }
    case UbMatchedType::UBMATCHTYPE_ELTWISE: {
      input_size += fusion_node->GetInDataNodes().size() - 1;
      FE_MAKE_SHARED(slice_info_base_ptr = std::make_shared<ConvEltwiseSliceInfo>(), return nullptr);
      break;
    }
    case UbMatchedType::UBMATCHTYPE_POOLING: {
      input_size += fusion_node->GetInDataNodes().size() - 1;
      FE_MAKE_SHARED(slice_info_base_ptr = std::make_shared<ConvPoolingSliceInfo>(), return nullptr);
      break;
    }
    case UbMatchedType::UBMATCHTYPE_DEQUANTS16: {
      input_size += fusion_node->GetInDataNodes().size() - 1;
      FE_MAKE_SHARED(slice_info_base_ptr = std::make_shared<ConvDequantS16SliceInfo>(), return nullptr);
      break;
    }
    case UbMatchedType::UBMATCHTYPE_REQUANTS16: {
      input_size += fusion_node->GetInDataNodes().size() - 1;
      FE_MAKE_SHARED(slice_info_base_ptr = std::make_shared<ConvRequantS16SliceInfo>(), return nullptr);
      break;
    }
    default: {
      FE_LOGI("not support this kind of pattern by now.");
      input_size += fusion_node->GetInDataNodes().size() - 1;
      FE_MAKE_SHARED(slice_info_base_ptr = std::make_shared<UbPassSliceInfoBase>(), return nullptr);
    }
  }
  return slice_info_base_ptr;
}

bool UbPassSliceInfoManager::IsHeadFusion(const ge::NodePtr &fusion_node, const vector<ge::NodePtr> &fusion_nodes) {
  ge::NodePtr pre_node = fusion_node->GetInDataNodes().at(0);
  if (pre_node == nullptr) {
    FE_LOGW("Node[%s] first input node is nullptr", fusion_node->GetName().c_str());
    return false;
  }
  if (std::find(fusion_nodes.begin(), fusion_nodes.end(), pre_node) != fusion_nodes.end()) {
    FE_LOGD("Node[%s] fusion mode is head fusion.", fusion_node->GetName().c_str());
    return true;
  }
  return false;
}

static bool IsSkippedNode(const ge::NodePtr &node) {
  return (node != nullptr && SKIPPED_TYPE_FOR_SLICE_INFO.count(node->GetType()) != 0);
}

static bool IsFusionNode(const ge::NodePtr &node, std::unordered_map<ge::NodePtr, bool> &node_visited) {
  return (node != nullptr && node_visited.count(node) != 0);
}

static Status GetRealIdxOfTailNode(const ge::NodePtr &tail_node, const size_t &out_idx, size_t& real_idx,
                                   std::unordered_map<ge::NodePtr, bool> &node_visited,
                                   UbPassInputOutputIndexInfo & index_info) {
  FE_CHECK_NOTNULL(tail_node);
  auto actual_tail_node = tail_node;
  size_t actual_out_idx = out_idx;
  if (tail_node->GetOutDataNodes().size() <= out_idx) {
    return FAILED;
  }
  auto out_node = tail_node->GetOutDataNodes().at(out_idx);
  if (IsFusionNode(out_node, node_visited)) {
    while (IsSkippedNode(out_node)) {
      FE_CHECK_NOTNULL(out_node);
      actual_tail_node = out_node;
      if (out_node->GetOutDataNodes().size() <= 0) {
        return FAILED;
      }
      out_node = out_node->GetOutDataNodes().at(0);
    }
    actual_out_idx = 0;
  }
  FE_CHECK_NOTNULL(actual_tail_node->GetOpDesc());
  auto actual_node_id = actual_tail_node->GetOpDesc()->GetId();
  real_idx = index_info[std::to_string(actual_node_id) + ":out:" + std::to_string(actual_out_idx)];
  return SUCCESS;
}

static Status GetRealIdxOfHeadNode(const ge::NodePtr &head_node, const size_t &in_idx, size_t &real_idx,
                                   std::unordered_map<ge::NodePtr, bool> &node_visited,
                                   UbPassInputOutputIndexInfo & index_info) {
  FE_CHECK_NOTNULL(head_node);
  auto actual_head_node = head_node;
  size_t actual_in_idx = in_idx;
  if (head_node->GetInDataNodes().size() <= in_idx) {
    return FAILED;
  }
  auto in_node = head_node->GetInDataNodes().at(in_idx);
  if (IsFusionNode(in_node, node_visited)) {
    while (IsSkippedNode(in_node)) {
      FE_CHECK_NOTNULL(in_node);
      actual_head_node = in_node;
      if (in_node->GetOutDataNodes().size() <= 0) {
        return FAILED;
      }
      in_node = in_node->GetOutDataNodes().at(0);
    }
    actual_in_idx = 0;
  }
  FE_CHECK_NOTNULL(actual_head_node->GetOpDesc());
  auto actual_node_id = actual_head_node->GetOpDesc()->GetId();
  real_idx = index_info[std::to_string(actual_node_id) + ":in:" + std::to_string(actual_in_idx)];
  return SUCCESS;
}

static Status GetIdxInfoForFusionOp(const vector<ge::NodePtr> &fusion_nodes, std::unordered_map<ge::NodePtr, bool> &node_visited,
                                    UbPassInputOutputIndexInfo &index_info) {
  size_t fusion_input_index = -1;
  size_t fusion_output_index = -1;
  for (auto &fusion_node: fusion_nodes) {
    FE_CHECK_NOTNULL(fusion_node);
    FE_CHECK_NOTNULL(fusion_node->GetOpDesc());
    auto node_id = fusion_node->GetOpDesc()->GetId();
    // calculate input index
    for (auto &in_anchor: fusion_node->GetAllInDataAnchors()) {
      FE_CHECK_NOTNULL(in_anchor);
      if (in_anchor->GetPeerOutAnchor() == nullptr ||
          IsFusionNode(in_anchor->GetPeerOutAnchor()->GetOwnerNode(), node_visited)) {
        continue; // previous condition is for case such like: offset_w, the 4th input of conv2d
      }
      index_info[std::to_string(node_id) + ":in:" + std::to_string(in_anchor->GetIdx())] = ++fusion_input_index;
    }

    // calculate output index
    for (auto &out_anchor: fusion_node->GetAllOutDataAnchors()) {
      FE_CHECK_NOTNULL(out_anchor);
      for (auto &peer_in_anchor : out_anchor->GetPeerInDataAnchors()) {
        if (peer_in_anchor == nullptr || IsFusionNode(peer_in_anchor->GetOwnerNode(), node_visited)) {
          continue;
        }
        index_info[std::to_string(node_id) +":out:" + std::to_string(out_anchor->GetIdx())]  = ++fusion_output_index;
        break;
      }
    }
  }
  return SUCCESS;
}

static Status GetOpSliceInfoForSingleOp(const ge::NodePtr &node, OpCalcInfo &op_slice_info) {
  if (node == nullptr || op_slice_info.IsPtrNull()) {
    return FAILED;
  }
  std::string op_calc_info_str;
  if (!ge::AttrUtils::GetStr(node->GetOpDesc(), OP_SLICE_INFO, op_calc_info_str) || op_calc_info_str.empty()) {
    return FAILED;
  }
  GetOpSliceInfoFromJson(op_slice_info, op_calc_info_str);
  return SUCCESS;
}

static bool GetOutputSplitInfoAtIdx(const std::vector<OutputSplitInfoPtr> &output_split_infos,  const size_t &idx,
                                    OutputSplitInfoPtr &output_split_info) {
  for (auto &split_info: output_split_infos) {
    if (split_info->GetIndex() == idx) {
      output_split_info = split_info;
      return true;
    }
  }
  return false;
}

static bool IsTailNode(const ge::NodePtr &fusion_node, std::unordered_map<ge::NodePtr, bool> &node_visited) {
  if (fusion_node == nullptr) {
    return false;
  }
  for (auto &out_node : fusion_node->GetOutDataNodes()) {
    bool need_skipped = IsSkippedNode(out_node);
    bool condition = ((need_skipped && !IsTailNode(out_node, node_visited)) ||
                      (!need_skipped && IsFusionNode(out_node, node_visited)));
    if (condition) {
      return false;
    }
  }
  return true;
}

static Status GetTailNodesOfFusionOp(vector<ge::NodePtr> &fusion_nodes,
                                     std::unordered_map<ge::NodePtr, bool> &node_visited,
                                     std::vector<ge::NodePtr> &tail_nodes) {
  for (auto &fusion_node : fusion_nodes) {
    if (!IsSkippedNode(fusion_node) && IsTailNode(fusion_node, node_visited)) {
      tail_nodes.push_back(fusion_node);
    }
  }
  return SUCCESS;
}

Status GetAxisSplitMapAtAxis(const ge::NodePtr &node, const int64_t &axis_split_choice,
                             AxisSplitMapPtr& selected_split_map) {
  OpCalcInfo op_slice_info;
  if (!op_slice_info.Initialize() || GetOpSliceInfoForSingleOp(node, op_slice_info) != SUCCESS) {
    return FAILED;
  }
  for (auto &axis_split_map: op_slice_info.GetAxisSplitMaps()) {
    auto out_split_infos = axis_split_map->GetOutputSplitInfos();
    bool condition = (out_split_infos.size() > 0 && out_split_infos.at(0) != nullptr &&
                      out_split_infos.at(0)->GetAxis().size() > 0 &&
                      out_split_infos.at(0)->GetAxis().at(0) == axis_split_choice);
    if (condition) {
      selected_split_map = axis_split_map;
      return SUCCESS;
    }
  }
  return FAILED;
}

static Status AddOutputSplitInfoForFusionNode(const ge::NodePtr &last_node,
                                              vector <OutputSplitInfoPtr> &last_output_split_infos,
                                              vector<OutputSplitInfo>& new_output_split_infos,
                                              UbPassInputOutputIndexInfo &index_info,
                                              std::unordered_map<ge::NodePtr, bool> &node_visited) {
  FE_CHECK_NOTNULL(last_node);
  for (const auto &cur_output_split_info: last_output_split_infos) {
    size_t out_idx = cur_output_split_info->GetIndex();
    auto last_out_anchors = last_node->GetAllOutDataAnchors();
    bool condition = (last_out_anchors.size() <= out_idx || last_out_anchors.at(out_idx) == nullptr);
    if (condition) {
      return FAILED;
    }
    for (const auto &last_out_peer_in_anchor : last_out_anchors.at(out_idx)->GetPeerInDataAnchors()) {
      condition = (last_out_peer_in_anchor == nullptr ||
                   IsFusionNode(last_out_peer_in_anchor->GetOwnerNode(), node_visited));
      if (condition) {
        continue; // current out_node is not a output of fusion op
      }
      OutputSplitInfo new_output_split_info;
      if (!new_output_split_info.Initialize()) {
        return FAILED;
      }

      size_t real_idx = 0;
      if (GetRealIdxOfTailNode(last_node, out_idx, real_idx, node_visited, index_info) != SUCCESS) {
        return FAILED;
      }
      auto out_axes = cur_output_split_info->GetAxis();
      new_output_split_info.SetIndex(real_idx);
      new_output_split_info.SetAxis(out_axes);
      new_output_split_infos.push_back(new_output_split_info);
      break;
    }
  }
  return SUCCESS;
}

static Status HandleInputNode(const ge::NodePtr &node, InputSplitInfoPtr &input_split_info,
                              UbPassInputOutputIndexInfo &index_info,
                              std::unordered_map<ge::NodePtr, bool> &node_visited,
                              vector<InputSplitInfo> &new_input_split_infos) {
  InputSplitInfo new_input_split_info;
  if (!new_input_split_info.Initialize()) {
    return FAILED;
  }
  size_t real_idx = 0;
  size_t in_idx = input_split_info->GetIndex();
  if (GetRealIdxOfHeadNode(node, in_idx, real_idx, node_visited, index_info) != SUCCESS) {
    return FAILED;
  }
  auto in_axes = input_split_info->GetAxis();
  new_input_split_info.SetIndex(real_idx);
  new_input_split_info.SetAxis(in_axes);
  auto head_over_lap = input_split_info->GetHeadOverLap();
  auto tail_over_lap = input_split_info->GetTailOverLap();
  new_input_split_info.SetHeadOverLap(head_over_lap);
  new_input_split_info.SetTailOverLap(tail_over_lap);
  new_input_split_infos.push_back(new_input_split_info);
  return SUCCESS;
}

static Status UpDownSearchAxisSplitMapChain(const ge::NodePtr &node, const AxisSplitMapPtr &axis_split_map,
                                            const AxisSplitMapPtr &new_axis_split_map,
                                            std::unordered_map<ge::NodePtr, bool> &node_visited,
                                            UbPassInputOutputIndexInfo &index_info) {
  // store tmp split_info for input and output nodes of fusion op
  vector<InputSplitInfo> new_input_split_infos;
  vector<OutputSplitInfo> new_output_split_infos;
  FE_CHECK_NOTNULL(node);
  FE_CHECK_NOTNULL(node->GetOpDesc());
  auto in_anchors = node->GetAllInDataAnchors();
  for (auto &input_split_info: axis_split_map->GetInputSplitInfos()) {
    FE_CHECK_NOTNULL(input_split_info);
    size_t in_idx = input_split_info->GetIndex();
    auto in_axes = input_split_info->GetAxis();
    // get last node at in_idx which is specified by input_split_info
    bool condition = (in_anchors.size() <= in_idx || in_anchors.at(in_idx) == nullptr ||
                      in_anchors.at(in_idx)->GetPeerOutAnchor() == nullptr ||
                      in_anchors.at(in_idx)->GetPeerOutAnchor()->GetOwnerNode() == nullptr);
    if (condition) {
      return FAILED;
    }
    auto last_out_anchor = in_anchors.at(in_idx)->GetPeerOutAnchor();
    auto last_node = last_out_anchor->GetOwnerNode();
    // if last node is strided_read/strided_write/read_select/write_select,
    // skip this node and visit its first input node
    while (IsSkippedNode(last_node)) {
      auto last_in_anchors = last_node->GetAllInDataAnchors();
      if (last_in_anchors.empty()) {
        return FAILED;
      }
      FE_CHECK_NOTNULL(last_in_anchors.at(0));
      FE_CHECK_NOTNULL(last_in_anchors.at(0)->GetPeerOutAnchor());
      FE_CHECK_NOTNULL(last_in_anchors.at(0)->GetPeerOutAnchor()->GetOwnerNode());
      last_out_anchor = last_in_anchors.at(0)->GetPeerOutAnchor();
      last_node = last_out_anchor->GetOwnerNode();
    }
    if (!IsFusionNode(last_node, node_visited)) {
      // last_node is an input of fusion op, add it to new_input_split_infos
      if (HandleInputNode(node, input_split_info, index_info, node_visited, new_input_split_infos) != SUCCESS) {
        return FAILED;
      }
    } else if (node_visited[last_node]) { // last_node has been visited before, continue next check
      continue;
    } else { // last_node is a node in fusion op, check whether it has a matched output_split_info
      auto last_out_idx = last_out_anchor->GetIdx();
      AxisSplitMapPtr last_axis_split_map;
      bool ret = (in_axes.size() <= 0 || GetAxisSplitMapAtAxis(last_node, in_axes.at(0), last_axis_split_map)
                        != SUCCESS);
      if (ret) {
        return FAILED;
      }
      auto last_output_split_infos = last_axis_split_map->GetOutputSplitInfos();
      OutputSplitInfoPtr last_output_split_info;
      bool search_ret = ((GetOutputSplitInfoAtIdx(last_output_split_infos, last_out_idx, last_output_split_info)) &&
                        (in_axes == last_output_split_info->GetAxis()) &&
                        (UpDownSearchAxisSplitMapChain(last_node, last_axis_split_map, new_axis_split_map,
                                                       node_visited, index_info) == SUCCESS));
      if (search_ret) {
        if (AddOutputSplitInfoForFusionNode(last_node, last_output_split_infos, new_output_split_infos,
                                            index_info, node_visited) != SUCCESS) {
          return FAILED;
        }
        node_visited[last_node] = true;
      } else {
        FE_LOGD("UpDown search split_info_map chain for node [%s] failed.", last_node->GetName().c_str());
        return FAILED;
      }
    }
  }

  for (auto &new_input_split_info : new_input_split_infos) {
    new_axis_split_map->AddInputSplitInfo(new_input_split_info);
  }
  for (auto &new_output_split_info : new_output_split_infos) {
    new_axis_split_map->AddOutputSplitInfo(new_output_split_info);
  }
  FE_LOGD("UpDown search split_info_map chain for node [%s] success.", node->GetName().c_str());
  return SUCCESS;
}

static Status GetValidAxisSplitChoices(const vector<ge::NodePtr> &tail_nodes,
                                       const unordered_set<int64_t> &invalid_split_axis,
                                       const vector<int64_t> &axis_order_reference,
                                       vector<int64_t> &valid_split_axis) {
  int64_t node_num = tail_nodes.size();
  for (auto &axis: axis_order_reference) {
    int64_t count = 0;
    // if all tail_nodes can be cut on one axis, it is considered as a valid split axis
    for (auto &tail_node: tail_nodes) {
      AxisSplitMapPtr split_map;
      if (GetAxisSplitMapAtAxis(tail_node, axis, split_map) != SUCCESS) {
        break;
      }
      count += 1;
    }
    if (count == node_num && invalid_split_axis.count(axis) == 0) {
      valid_split_axis.push_back(axis);
    }
  }
  return SUCCESS;
}

static Status GetInvalidAxisSplitChoices(const vector<ge::NodePtr> &fusion_nodes,
                                         unordered_set<int64_t> &invalid_split_axis) {
  // nodes with below type do not have op_slice_info, but have restrictions on fusion_op_slice_info
  for (auto &fusion_node : fusion_nodes) {
    FE_CHECK_NOTNULL(fusion_node);
    auto op_type = fusion_node->GetType();
    if (op_type == "StridedRead") {
      invalid_split_axis.insert(kNc1hwc0HCutAxis[0]);
      invalid_split_axis.insert(kNc1hwc0WCutAxis[0]);
    } else if (op_type == "StridedWrite") {
      invalid_split_axis.insert(kNc1hwc0HCutAxis[0]);
      invalid_split_axis.insert(kNc1hwc0WCutAxis[0]);
      invalid_split_axis.insert(kNc1hwc0CoutCutAxis[0]);
    } else if (op_type == "ReadSelect" || op_type == "WriteSelect") {
      invalid_split_axis.insert(kNc1hwc0HCutAxis[0]);
      invalid_split_axis.insert(kNc1hwc0WCutAxis[0]);
    }
  }
  return SUCCESS;
}

static Status GetHeadNodeAxisSplitOrder(const vector<ge::NodePtr> &fusion_nodes, OpCalcInfo &head_op_slice_info,
                                        std::vector<int64_t> &head_split_axis) {
  ge::NodePtr head_node = nullptr;
  for (auto &fusion_node: fusion_nodes) {
    // fusion_nodes have been TopologicalSorting, if fusion_node cannot be skipped, it must be the head node
    if (!IsSkippedNode(fusion_node)) {
      head_node = fusion_node;
      break;
    }
  }
  bool condition = (head_node == nullptr || !head_op_slice_info.Initialize() ||
                    GetOpSliceInfoForSingleOp(head_node, head_op_slice_info) != SUCCESS);
  if (condition) {
    return FAILED;
  }
  for (auto &head_axis_split_map: head_op_slice_info.GetAxisSplitMaps()) {
    auto out_split_infos = head_axis_split_map->GetOutputSplitInfos();
    bool ret = (out_split_infos.size() <= 0 || out_split_infos.at(0) == nullptr ||
                 out_split_infos.at(0)->GetAxis().size() <= 0);
    if (ret) {
      return FAILED;
    }
    head_split_axis.push_back(out_split_infos.at(0)->GetAxis()[0]);
  }
  return SUCCESS;
}

Status UbPassSliceInfoManager::CalcSliceInfoForFusionOp(vector<ge::NodePtr> &fusion_nodes, OpCalcInfo &op_slice_info) {
  /* Main idea of this function is to updown search a split_info_map chain among fusion nodes.
  * The search process follows the
  * rule that peer input-output tensors must have same split info.
  * If the chain is found, the corresponding split_info_map can
  * be added into op_slice_info for fusion_op.
  *
  * Assuming a UB fusion pass matches follwing structure: "conv1 ---> conv2",
  * op slice info for each conv node can be described as below:
  * conv1: (1) {inputList:{idx:0, axis=[0], headOverLap=[0],  TailOverLap=[0]}, outputList{idx:0, axis=[0]}}
  *        (2) {inputList:{idx:0, axis=[2], headOverLap=[-1], TailOverLap=[-1]}, outputList{idx:0, axis=[2]}}
  *        (3) {inputList:{idx:0, axis=[3], headOverLap=[-1], TailOverLap=[-1]}, outputList{idx:0, axis=[3]}}
  *        (4) {inputList:{idx:1, axis=[1], headOverLap=[0],  TailOverLap=[0]}, outputList{idx:0, axis=[1]}}
  *
  * conv2: (1) {inputList:{idx:0, axis=[0], headOverLap=[0],  TailOverLap=[0]}, outputList{idx:0, axis=[0]}}
  *        (2) {inputList:{idx:0, axis=[2], headOverLap=[-1], TailOverLap=[-1]}, outputList{idx:0, axis=[2]}}
  *        (3) {inputList:{idx:0, axis=[3], headOverLap=[-1], TailOverLap=[-1]}, outputList{idx:0, axis=[3]}}
  *        (4) {inputList:{idx:1, axis=[1], headOverLap=[0],  TailOverLap=[0]}, outputList{idx:0, axis=[1]}}
  *
  * Since the output:0 of conv1 is the input:0 of conv2, these tow tensor must have the same split info.
  * Thus, split info numbered (4)
  * of conv1 cannot be adopted as the input:0 of conv2 does not have a split info performed on axis 1,
  * which means the chain does not
  * exist. Thus, the op_slice info for fusion op is as following:
  * fusion op: (1) {inputList:{idx:0, axis=[0], headOverLap=[0],  TailOverLap=[0]}, outputList{idx:0, axis=[0]}}
  *            (2) {inputList:{idx:0, axis=[2], headOverLap=[-1], TailOverLap=[-1]}, outputList{idx:0, axis=[2]}}
  *            (3) {inputList:{idx:0, axis=[3], headOverLap=[-1], TailOverLap=[-1]}, outputList{idx:0, axis=[3]}}
  *            (4) {inputList:{idx:2, axis=[1], headOverLap=[0],  TailOverLap=[0]}, outputList{idx:0, axis=[1]}}
  *
  * [N][C1][H][W][C0] (input:0)   [K1][N1][N0][K0] (input:1)
  *   \      \  \
  *    \      \  \
  *     \      \  \
  *  (1) \  (2) \  \ (3)
  *      [N][C1][H][W][C0] (output:0)
  *       |      |  |
  *       |      |  |
  *      [N][C1][H][W][C0] (input:0)    [K1][N1][N0][K0] (input:1)
  *        \      \  \                       |
  *         \      \  \                      | (4)
  *          \    --\--\----------------------
  *           \   |  \  \
  *           [N][C1][H][W][C0]  (output:0)
  *
  */

  // mark fusion nodes. bool value is taken to identify whether this node has been visited,
  // useful when a fusion op has multi output nodes.
  std::unordered_map<ge::NodePtr, bool> node_visited;
  for (auto &fusion_node: fusion_nodes) {
    FE_CHECK_NOTNULL(fusion_node);
    node_visited[fusion_node] = false;
  }
  // calculate real input and output indexes for fusion op; get all tail nodes in a buffer fusion pass
  UbPassInputOutputIndexInfo index_info;
  std::vector<ge::NodePtr> tail_nodes;
  bool condition = ((GetIdxInfoForFusionOp(fusion_nodes, node_visited, index_info) != SUCCESS) ||
                    (GetTailNodesOfFusionOp(fusion_nodes, node_visited, tail_nodes) != SUCCESS));
  if (condition) {
    return FAILED;
  }
  // skipped invalid axis split choices. if strided_write is in op_slice_info, skip H/W cut, etc.
  unordered_set<int64_t> invalid_split_axis;
  if (GetInvalidAxisSplitChoices(fusion_nodes, invalid_split_axis) != SUCCESS) {
    return FAILED;
  }
  // split axis order of FusionOpSliceInfo should be the same as that of OpSliceInfo in head_node
  OpCalcInfo head_op_slice_info;
  std::vector<int64_t> head_split_axis;
  if (GetHeadNodeAxisSplitOrder(fusion_nodes, head_op_slice_info, head_split_axis) != SUCCESS) {
    return FAILED;
  }
  // get all possible axis split choices for fusion op
  vector<int64_t> valid_split_axis;
  if (GetValidAxisSplitChoices(tail_nodes, invalid_split_axis, head_split_axis, valid_split_axis) != SUCCESS) {
    return FAILED;
  }

  // go through each split axis to check whether it is a valid split axis
  for (auto &split_axis: valid_split_axis) {
    AxisSplitMapPtr new_axis_split_map;
    FE_MAKE_SHARED(new_axis_split_map = std::make_shared<AxisSplitMap>(), return FAILED);
    if (!new_axis_split_map->Initialize()) {
      return FAILED;
    }
    // if each tail_node has a down-to-up split map chain, axis is considered as a valid split axis
    bool is_valid_axis = false;
    for (auto &tail_node: tail_nodes) {
      is_valid_axis = true;
      AxisSplitMapPtr tail_axis_split_map;
      condition = (GetAxisSplitMapAtAxis(tail_node, split_axis, tail_axis_split_map) == SUCCESS &&
                   UpDownSearchAxisSplitMapChain(tail_node, tail_axis_split_map, new_axis_split_map,
                                                 node_visited, index_info) == SUCCESS);
      if (condition) {
        // add output_split_info of tail_node into axis_split_map
        for (auto &tail_output_split_info: tail_axis_split_map->GetOutputSplitInfos()) {
          FE_CHECK_NOTNULL(tail_output_split_info);
          OutputSplitInfo new_output_split_info;
          size_t real_idx = 0;
          size_t out_idx = tail_output_split_info->GetIndex();
          bool condition1 = (!new_output_split_info.Initialize() ||
                            (GetRealIdxOfTailNode(tail_node, out_idx, real_idx, node_visited, index_info) != SUCCESS));
          if (condition1) {
            return FAILED;
          }
          auto out_axes = tail_output_split_info->GetAxis();
          new_output_split_info.SetIndex(real_idx);
          new_output_split_info.SetAxis(out_axes);
          new_axis_split_map->AddOutputSplitInfo(new_output_split_info);
        }
      } else {
        is_valid_axis = false;
        break;
      }
    }
    if (is_valid_axis) {
      op_slice_info.AddAxisSplitMap(*new_axis_split_map);
    }
    // reset for calculation of next axis_split_map
    for (auto &elem : node_visited) {
      elem.second = false;
    }
  }

  // set l1FusionEnable and minTbeL1Space
  int64_t space_info = head_op_slice_info.GetMinTbeL1Space();
  auto is_enable = head_op_slice_info.GetL1FusionEnable();
  op_slice_info.SetMinTbeL1Space(space_info);
  op_slice_info.SetL1FusionEnable(is_enable);
  return SUCCESS;
}
}  // namespace fe