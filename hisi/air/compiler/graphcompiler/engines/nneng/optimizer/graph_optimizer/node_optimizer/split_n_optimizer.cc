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

#include "graph_optimizer/node_optimizer/split_n_optimizer.h"
#include <memory>
#include <vector>
#include "common/aicore_util_attr_define.h"
#include "common/fe_inner_attr_define.h"
#include "common/unknown_shape_util.h"
#include "graph/debug/ge_attr_define.h"
#include "runtime/mem.h"

using namespace ge;
namespace fe {
const int kInputShapeLimit1 = 2;
const int kRealDimNchwToHwcn = 3;

void SplitOptimizer::GetRealSplitDimFromOriginalFormatToFormat(const ge::OpDescPtr &op_desc, int64_t &split_dim) const {
  (void)ge::AttrUtils::GetInt(op_desc, SPLIT_DIM, split_dim);
  ge::GeTensorDesc input_tensor = op_desc->GetInputDesc(0);
  auto input_format = ge::GetPrimaryFormat(input_tensor.GetFormat());
  ge::Format input_orinal_format = input_tensor.GetOriginFormat();
  ge::GeShape input_orinal_shape_shape = input_tensor.GetOriginShape();

  if (split_dim < 0) {
    FE_LOGD("Split_dim[%ld] is nagtive number, change it to positive.", split_dim);
    split_dim = static_cast<int64_t>(input_orinal_shape_shape.GetDimNum()) + split_dim;
  }
  FE_LOGD("GetRealSplitDim start node: %s, positive split dim: %ld, input original shape size: %zu",
          op_desc->GetName().c_str(), split_dim, input_orinal_shape_shape.GetDimNum());

  bool condition_nd_fractalz = input_orinal_format == FORMAT_ND && input_format == FORMAT_FRACTAL_NZ;
  if (condition_nd_fractalz) {
    if (input_orinal_shape_shape.GetDimNum() == kInputShapeLimit1) {
      if (!split_dim) {
        split_dim = 1;
      } else if (split_dim == 1) {
        split_dim = 0;
      }
    }
    FE_LOGD("meet condition_nd_fractalz: %d, change split to %ld.", condition_nd_fractalz, split_dim);
  }
  bool condition_nchw_hwcn = input_orinal_format == FORMAT_NCHW  && input_format == FORMAT_HWCN;
  if (condition_nchw_hwcn) {
    FE_LOGD("meet condition_nchw_to_hwcn: %d, change split to %d.", condition_nchw_hwcn, kRealDimNchwToHwcn);
    split_dim = kRealDimNchwToHwcn;
  }
  FE_LOGD("GetRealSplitDim end node:%s, splitdim: %ld", op_desc->GetName().c_str(), split_dim);
}

bool SplitOptimizer::HasControlEdge(ge::NodePtr split_node) {
  return split_node->GetInControlNodes().size() != 0 || split_node->GetOutControlNodes().size() != 0;
}

bool SplitOptimizer::ValidInput(ge::NodePtr split_node) const {
  for (auto in_node : split_node->GetInDataNodes()) {
    std::vector<std::string> NOT_SUPPORT_TYPE = {DATA, VARIABLE, CONSTANTOP, CONSTANT};
    std::string op_type = in_node->GetType();
    bool not_support = false;
    not_support = std::find(NOT_SUPPORT_TYPE.begin(), NOT_SUPPORT_TYPE.end(), op_type) != NOT_SUPPORT_TYPE.end();
    if (not_support) {
      FE_LOGD("In node %s type is Data/Variable/Const, %s can not optimize.", in_node->GetName().c_str(),
              split_node->GetName().c_str());
      return false;
    }
    if ((in_node->GetType() == TRANSDATA || in_node->GetType() == RESHAPE) && in_node->GetAllInDataAnchors().size()) {
      auto peerOutAnchor = in_node->GetInDataAnchor(0)->GetPeerOutAnchor();
      if (peerOutAnchor == nullptr) {
        FE_LOGD("in_node[%s]'s first peer in anchor is null", in_node->GetName().c_str());
        return false;
      }
      auto pre_in_node = peerOutAnchor->GetOwnerNode();
      if (pre_in_node != nullptr && pre_in_node->GetType() == DATA) {
        FE_LOGD("Data->TransData/Reshape->split. Data: pre_in_node[%s], %s can not optimize.",
                pre_in_node->GetName().c_str(), split_node->GetName().c_str());
        return false;
      }
    }
    bool is_no_task = false;
    (void)ge::AttrUtils::GetBool(in_node->GetOpDesc(), ge::ATTR_NAME_NOTASK, is_no_task);
    if (is_no_task) {
      FE_LOGD("In node %s has no_task attribute, %s can not optimize.", in_node->GetName().c_str(),
              split_node->GetName().c_str());
        return false;
    }
    vector<int64_t> output_index;
    (void)ge::AttrUtils::GetListInt(in_node->GetOpDesc(), ge::ATOMIC_ATTR_OUTPUT_INDEX, output_index);
    if (!output_index.empty()) {
      FE_LOGD("In node [%s] has atomic_output attribute, can't optimize.", in_node->GetName().c_str());
      return false;
    }
  }
  return true;
}

bool SplitOptimizer::InputCheck(ge::NodePtr split_node) const {
  string node_name = split_node->GetName();
  bool has_control_edge = HasControlEdge(split_node);
  bool valid_input = ValidInput(split_node);
  if (has_control_edge) {
    FE_LOGD("Split has control edge, %s can not optimize.", node_name.c_str());
  }
  if (!valid_input) {
    FE_LOGD("Split node[%s] has place holder input, and it's parent op is Data.", node_name.c_str());
  }
  return !has_control_edge && valid_input;
}

bool SplitOptimizer::InvalidNodeType(const string& node_type) {
  bool invalid_type = node_type == NETOUTPUT || node_type == "HcomBroadcast" || node_type == "HcomAllGather" ||
                      node_type == "HcomAllReduce" || node_type == "HcomReduceScatter" || node_type == "HcomReduce" ||
                      node_type == SPLITD || node_type == SPLITVD;
  if (invalid_type) {
    FE_LOGD("next node type: %s, can not optimize.", node_type.c_str());
    return true;
  }
  return false;
}

bool SplitOptimizer::InvalidNodeAttr(const ge::OpDescPtr& node_desc) {
  int imply_type = 0;
  (void)ge::AttrUtils::GetInt(node_desc, ge::ATTR_NAME_IMPLY_TYPE, imply_type);
  string fusion_virtual_op = "";
  (void)ge::AttrUtils::GetStr(node_desc, ge::ATTR_NAME_FUSION_VIRTUAL_OP, fusion_virtual_op);
  bool is_continous_input = false;
  (void)ge::AttrUtils::GetBool(node_desc, ge::ATTR_NAME_CONTINUOUS_INPUT, is_continous_input);
  vector<int64_t> output_index;
  (void)ge::AttrUtils::GetListInt(node_desc, ge::ATOMIC_ATTR_OUTPUT_INDEX, output_index);
  if (imply_type != 1) {
    FE_LOGD("Next node is not aicore node, imply_type: %d.", imply_type);
    return true;
  }
  if (fusion_virtual_op != "") {
    FE_LOGD("Next node [%s] has fusion_virtual_op attribute, can't optimize.", node_desc->GetName().c_str());
    return true;
  }
  if (is_continous_input) {
    FE_LOGD("Next node [%s] has continous_input attribute, can't optimize.", node_desc->GetName().c_str());
    return true;
  }
  if (!output_index.empty()) {
    FE_LOGD("Next node [%s] has atomic attribute, can't optimize.", node_desc->GetName().c_str());
    return true;
  }
  return false;
}

bool SplitOptimizer::OutputCheck(ge::NodePtr split_node) const {
  for (auto output_anchor : split_node->GetAllOutDataAnchors()) {
    for (size_t i = 0; i < output_anchor->GetPeerInDataAnchors().size(); i++) {
      ge::NodePtr next_node = output_anchor->GetPeerInDataAnchors().at(i)->GetOwnerNode();
      ge::OpDescPtr next_node_desc = next_node->GetOpDesc();
      FE_CHECK_NOTNULL(next_node_desc);
      FE_LOGD("split node next_node name is [%s], type: %s.", next_node_desc->GetName().c_str(),
              next_node_desc->GetType().c_str());
      if (InvalidNodeType(next_node_desc->GetType())) {
        FE_LOGD("output node [%s] has invalid node type, %s can not optimize.", next_node_desc->GetName().c_str(),
                split_node->GetName().c_str());
        return false;
      }
      if (InvalidNodeAttr(next_node_desc)) {
        FE_LOGD("output node [%s] has invalid node attribute, %s can not optimize.", next_node_desc->GetName().c_str(),
                split_node->GetName().c_str());
        return false;
      }
    }
  }
  return true;
}

bool SplitOptimizer::NeedSkip(const ge::NodePtr &node, const ge::OpDescPtr &op_desc) const {
  bool is_not_split = op_desc->GetType() != fe::SPLITD && op_desc->GetType() != fe::SPLITVD;
  string node_name = op_desc->GetName();
  if (is_not_split) {
    return true;
  }
  int64_t split_dim = -1;
  (void)GetRealSplitDimFromOriginalFormatToFormat(op_desc, split_dim);
  if (split_dim != 0) {
    FE_LOGD("split_dim is not 0, %s can not optimize.", node_name.c_str());
    return true;
  }

  if (IsFeSupportedDynamicOp(*node->GetOpDesc())) {
    FE_LOGD("split op[%s] is unknown shape op, can not optimize.", node_name.c_str());
    return true;
  }

  if (!InputCheck(node)) {
    FE_LOGD("Split input check failed, %s can not optimize.", node_name.c_str());
    return true;
  }

  if (!OutputCheck(node)) {
    FE_LOGD("Split output check failed, %s can not optimize.", node_name.c_str());
    return true;
  }

  if (InvalidMemType(op_desc)) {
    FE_LOGD("Split mem type check failed, %s can not optimize.", node_name.c_str());
    return true;
  }
  return false;
}

Status SplitOptimizer::SetFusionVirtualOp(const ge::ComputeGraph &graph) const {
  FE_LOGD("start to do SplitOptimizer");
  ge::ComputeGraph::Vistor<ge::NodePtr> nodes = graph.GetDirectNode();
  for (auto &node : nodes) {
    ge::OpDescPtr op_desc = node->GetOpDesc();
    FE_CHECK_NOTNULL(op_desc);
    string node_name = op_desc->GetName();
    if (NeedSkip(node, op_desc)) {
      FE_LOGD("node:[%s] need to skip", node_name.c_str());
      continue;
    }
    FE_LOGD("node[%s] start to set attribute", node->GetName().c_str());
    (void)ge::AttrUtils::SetBool(op_desc, ge::ATTR_NAME_NOTASK, true);
    (void)ge::AttrUtils::SetBool(op_desc, ge::ATTR_NAME_NOPADDING_CONTINUOUS_OUTPUT, true);
    (void)ge::AttrUtils::SetBool(op_desc, ge::ATTR_NAME_OUTPUT_REUSE_INPUT, true);
    (void)ge::AttrUtils::SetInt(op_desc, ge::ATTR_NAME_REUSE_INPUT_ON_DIM_INDEX, 0);
  }
  FE_LOGD("end to do SplitOptimizer");
  return fe::SUCCESS;
}
}  // namespace fe