/**
 * Copyright 2020-2021 Huawei Technologies Co., Ltd
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

#include "graph_optimizer/node_optimizer/concat_c_to_n_optimizer.h"
#include "graph_optimizer/graph_fusion/fusion_pass_manager/fusion_pass_registry.h"
#include "common/unknown_shape_util.h"
#include "graph/utils/node_utils.h"

namespace fe {
const int kRealDimNchwTo5Hd = 0;
const int kValidNcDimSize = 2;
const int kDeepth = 1000;

bool ConcatCToNOptimizer::MeetAlignmentConditionFromNCHWTo5HD(const ge::OpDescPtr &op_desc) const {
  for (const auto &cur_desc_ptr : op_desc->GetAllInputsDescPtr()) {
    if (cur_desc_ptr == nullptr) {
      return false;
    }
    auto ori_shape = cur_desc_ptr->GetOriginShape().GetDims();
    int64_t origin_c = ori_shape[1];
    ge::DataType data_type = cur_desc_ptr->GetDataType();
    bool cond = (data_type == ge::DT_FLOAT16 && origin_c % 16 == 0) ||
                (data_type == ge::DT_INT8 && origin_c % 32 == 0) ||
                (data_type == ge::DT_INT4 && origin_c % 64 == 0);
    if (!cond) {
      FE_LOGD("[op:%s] Not meet condition_nchw_to_nc1hwc0.", op_desc->GetName().c_str());
      return false;
    }
  }
  FE_LOGD("[op:%s] Meet condition_nchw_to_nc1hwc0, change concat_dim to %d.", op_desc->GetName().c_str(),
          kRealDimNchwTo5Hd);
  return true;
}

bool ConcatCToNOptimizer::CheckAxis(const ge::OpDescPtr &op_desc) const {
  for (const auto &cur_desc_ptr : op_desc->GetAllInputsDescPtr()) {
    if (cur_desc_ptr == nullptr) {
      return false;
    }
    auto ori_shape = cur_desc_ptr->GetOriginShape().GetDims();
    if (ori_shape.size() < kValidNcDimSize) {
      FE_LOGE("[%s]: original input shape is invalid", op_desc->GetName().c_str());
      return false;
    }
    int64_t origin_n = ori_shape[0];
    if (origin_n != 1) {
      FE_LOGD("First axis is %ld, not meet optimize condition.", origin_n);
      return false;
    }
  }
  return true;
}

bool ConcatCToNOptimizer::IsDynamicGraph(const ge::ComputeGraph &graph) const {
  bool is_unknown = false;
  for (const auto &node : graph.GetDirectNode()) {
    auto ret = ge::NodeUtils::GetNodeUnknownShapeStatus(*node, is_unknown);
    if (ret != fe::SUCCESS) {
      FE_LOGW("Get node unknown status failed, node name:%s, type:%s.", node->GetName().c_str(),
              node->GetType().c_str());
      continue;
    }
    if (is_unknown) {
      FE_LOGI("Current node %s, type %s is unknown shape,graph %s is dynamic graph.", node->GetName().c_str(),
              node->GetType().c_str(), graph.GetName().c_str());
      return true;
    }
  }
  return false;
}

// check whether pre node is valid
bool ConcatCToNOptimizer::CheckPreNodeValid(const ge::NodePtr &node, const string &node_name) const {
  bool fusion_virtual_op_flag = true;
  for (const auto &input_anchor : node->GetAllInDataAnchors()) {
    if (input_anchor == nullptr || input_anchor->GetPeerOutAnchor() == nullptr ||
        input_anchor->GetPeerOutAnchor()->GetOwnerNode() == nullptr) {
      return false;
    }
    ge::OutDataAnchorPtr pre_out_anchor = nullptr;
    checker_helper.GetFirstOutAnchorNotInDeleteList(input_anchor, pre_out_anchor, 0);
    if (pre_out_anchor == nullptr) {
      continue;
    }
    ge::OpDescPtr pre_node_desc = pre_out_anchor->GetOwnerNode()->GetOpDesc();
    FE_CHECK_NOTNULL(pre_node_desc);
    if (!checker_helper.IsPreNodeAttrValid(pre_node_desc, fusion_virtual_op_flag, node_name)) {
      return false;
    }
  }
  return true;
}

bool ConcatCToNOptimizer::CheckCommonCondition(const ge::ComputeGraph &graph, const ge::NodePtr &node,
                                               const ge::OpDescPtr &op_desc) const {
  string node_name = op_desc->GetName();
  // check whether is unknown shape op
  if (IsUnKnownShapeOp(*(op_desc.get()))) {
    FE_LOGD("Concat Unknown Shape check failed, %s can not optimize.", node_name.c_str());
    return false;
  }

  if (IsFeSupportedDynamicOp(*op_desc)) {
    FE_LOGD("Concat op[%s] is unknown shape op, can not optimize.", node_name.c_str());
    return false;
  }

  if (!checker_helper.InputCheck(node)) {
    FE_LOGD("Concat input check failed, %s can not optimize.", node_name.c_str());
    return false;
  }

  if (!checker_helper.OutputCheck(node)) {
    FE_LOGD("Concat output check failed, %s can not optimize.", node_name.c_str());
    return false;
  }
  // check whether mem type is valid
  if (InvalidMemType(op_desc)) {
    FE_LOGD("Concat mem type check failed, %s can not optimize.", node_name.c_str());
    return false;
  }

  if (!checker_helper.IsPeerNodeCanReUsed(node)) {
    FE_LOGD("Concat peer node cannot reused, %s can not optimize.", node_name.c_str());
    return false;
  }
  // check whether pre node is valid
  if (!CheckPreNodeValid(node, op_desc->GetName())) {
    FE_LOGD("Concat pre node valid check failed, %s can not optimize.", node_name.c_str());
    return false;
  }
  // check whether node has same input
  if (checker_helper.HasSameSrc(node)) {
    FE_LOGD("Concat has same input, %s can not optimize.", node_name.c_str());
    return false;
  }
  // check whether graph has unknownshape node
  if (IsDynamicGraph(graph)) {
    FE_LOGD("graph %s is dynamic graph, %s can not optimize.", graph.GetName().c_str(), node_name.c_str());
    return false;
  }
  return true;
}

bool ConcatCToNOptimizer::NeedSkip(const ge::ComputeGraph &graph, const ge::NodePtr &node,
                                   const ge::OpDescPtr &op_desc) const {
  bool is_not_concat = op_desc->GetType() != fe::CONCATD && op_desc->GetType() != fe::CONCATV2D;
  if (is_not_concat) {
    return true;
  }
  // check whether concat_dim can be changed from C(1) to N(0)
  int64_t concat_dim = -1;
  (void)ge::AttrUtils::GetInt(op_desc, CONCAT_DIM, concat_dim);
  auto input_tensor = op_desc->MutableInputDesc(0);
  if (input_tensor == nullptr) {
    return true;
  }
  auto input_format = ge::GetPrimaryFormat(input_tensor->GetFormat());
  ge::Format input_orinal_format = input_tensor->GetOriginFormat();
  ge::GeShape input_orinal_shape_shape = input_tensor->GetOriginShape();

  bool condition_nchw_5hd = (input_orinal_format == ge::FORMAT_NCHW && input_format == ge::FORMAT_NC1HWC0);
  if (concat_dim < 0) {
    FE_LOGD("Concat_dim[%ld] is nagtive number, change it to positive.", concat_dim);
    concat_dim = static_cast<int64_t>(input_orinal_shape_shape.GetDimNum()) + concat_dim;
  }

  if (concat_dim == 1 && CheckAxis(op_desc) && ((input_orinal_format == input_format) ||
     (condition_nchw_5hd && MeetAlignmentConditionFromNCHWTo5HD(op_desc))) &&
     CheckCommonCondition(graph, node, op_desc)) {
    return false;
  } else {
    return true;
  }
}

Status ConcatCToNOptimizer::SetFusionVirtualOp(const ge::ComputeGraph &graph) const {
  FE_LOGD("start to do ConcatCToNOptimizer.");
  ge::ComputeGraph::Vistor<ge::NodePtr> nodes = graph.GetDirectNode();
  for (auto &node : nodes) {
    ge::OpDescPtr op_desc = node->GetOpDesc();
    string node_name = op_desc->GetName();
    if (NeedSkip(graph, node, op_desc)) {
      FE_LOGD("node:[%s] need to skip", node_name.c_str());
      continue;
    }

    FE_LOGD("node[%s] start to set concat attribute", node->GetName().c_str());
    (void)ge::AttrUtils::SetBool(op_desc, ge::ATTR_NAME_NOTASK, true);
    (void)ge::AttrUtils::SetBool(op_desc, ge::ATTR_NAME_NOPADDING_CONTINUOUS_INPUT, true);
    (void)ge::AttrUtils::SetBool(op_desc, ge::ATTR_NAME_OUTPUT_REUSE_INPUT, true);
    (void)ge::AttrUtils::SetInt(op_desc, ge::ATTR_NAME_REUSE_INPUT_ON_DIM_INDEX, 0);
    checker_helper.SetPeerNodeWhetherCanReUsed(node);
  }
  FE_LOGD("end to do ConcatCToNOptimizer");
  return fe::SUCCESS;
}
}