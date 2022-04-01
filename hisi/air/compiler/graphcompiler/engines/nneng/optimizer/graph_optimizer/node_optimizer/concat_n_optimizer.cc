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

#include "graph_optimizer/node_optimizer/concat_n_optimizer.h"
#include <memory>
#include <vector>
#include "common/aicore_util_attr_define.h"
#include "common/fe_inner_attr_define.h"
#include "common/unknown_shape_util.h"
#include "graph/debug/ge_attr_define.h"

using namespace ge;
namespace fe {
const int kInputShapeLimit1 = 2;
const int kRealDimNchwToHwcn = 3;
const int kDeepth = 100;
const std::string kATTR_CAN_REUSED_FOR_CONCAT_OPTIMIZE = "can_reused_for_concat_optimize";
const std::set<std::string> kGeDeleteOpType = {
    "Expanddims",
    "Reshape",
    "ReFormat",
    "Squeeze",
    "Unsqueeze",
    "SqueezeV2",
    "UnsqueezeV2",
    "SqueezeV3",
    "UnsqueezeV3",
    "Size",
    "Shape",
    "ShapeN",
    "Rank"
};

bool ConcatOptimizer::CheckConcatInputAligned(const ge::OpDescPtr &op_desc_ptr) const {
  for (size_t i = 0; i < op_desc_ptr->GetAllInputsSize(); i++) {
    auto input_desc = op_desc_ptr->MutableInputDesc(i);
    if (input_desc->MutableShape().GetDimNum() != kInputShapeLimit1) {
      return false;
    }
    if (input_desc->MutableShape().GetDim(1) % GetC0(input_desc->GetDataType()) != 0) {
      FE_LOGD("Concat node[%s] has input is not aligned.", op_desc_ptr->GetName().c_str());
      return false;
    }
  }
  return true;
}

void ConcatOptimizer::GetRealConcatDimFromOriginalFormatToFormat(const ge::OpDescPtr &op_desc,
                                                                 int64_t &concat_dim) const {
  (void)ge::AttrUtils::GetInt(op_desc, CONCAT_DIM, concat_dim);
  ge::GeTensorDesc input_tensor = op_desc->GetInputDesc(0);
  auto input_format = ge::GetPrimaryFormat(input_tensor.GetFormat());
  ge::Format input_orinal_format = input_tensor.GetOriginFormat();
  ge::GeShape input_orinal_shape_shape = input_tensor.GetOriginShape();

  if (concat_dim < 0) {
    FE_LOGD("Concat_dim[%ld] is nagtive number, change it to positive.", concat_dim);
    concat_dim = static_cast<int64_t>(input_orinal_shape_shape.GetDimNum()) + concat_dim;
  }
  FE_LOGD("GetRealConcatDim start node: %s, positive concat dim: %ld, input original shape size: %zu",
          op_desc->GetName().c_str(), concat_dim, input_orinal_shape_shape.GetDimNum());

  bool condition_nd_fractalz = input_format == FORMAT_FRACTAL_NZ;
  if (condition_nd_fractalz) {
    FE_LOGD("GetRealConcatDimFromOriginalFormatToFormat condition_nd_fractalz: %d", condition_nd_fractalz);
    if (input_orinal_shape_shape.GetDimNum() == kInputShapeLimit1) {
      if (!concat_dim) {
        concat_dim = 1;
      } else if (concat_dim == 1 && CheckConcatInputAligned(op_desc)) {
        concat_dim = 0;
      }
    }
    FE_LOGD("meet condition_nd_fractalz: %d, change concat_dim to %ld.", condition_nd_fractalz, concat_dim);
  }
  bool condition_nchw_hwcn = input_orinal_format == FORMAT_NCHW && input_format == FORMAT_HWCN;
  if (condition_nchw_hwcn) {
    FE_LOGD("meet condition_nchw_to_hwcn: %d, change concat_dim to %d.", condition_nchw_hwcn, kRealDimNchwToHwcn);
    concat_dim = kRealDimNchwToHwcn;
  }
  FE_LOGD("GetRealConcatDim end node:%s, concatdim: %ld", op_desc->GetName().c_str(), concat_dim);
}

Status ConcatOptimizer::CalcTensorSize(ge::GeTensorDesc &tensor_desc, int64_t &tensor_size,
                                       int32_t &output_real_calc_flag) const {
  // verify the tensor
  if (TensorComputeUtil::VerifyTensor(tensor_desc) != SUCCESS) {
    FE_LOGD("Fail to verify this tensor.");
    return FAILED;
  }

  int64_t element_cnt;
  if (TensorComputeUtil::GetElementCountByMultiply(tensor_desc, element_cnt) != SUCCESS) {
    FE_LOGD("Fail to calculate tensor size.");
    return FAILED;
  }
  ge::DataType data_type = tensor_desc.GetDataType();
  if (TensorComputeUtil::GetTensorSizeByDataType(element_cnt, data_type, tensor_size, output_real_calc_flag) !=
      SUCCESS) {
    FE_LOGD("Fail to get tensor size by element count and datatype.");
    return FAILED;
  }
  return SUCCESS;
}

bool ConcatOptimizer::Check32Align(ge::NodePtr concat_node) const {
  for (size_t i = 0; i < concat_node->GetAllInDataAnchors().size(); i++) {
    int64_t tensor_size = 0;
    int32_t flag = 1;
    ge::GeTensorDesc tensor_desc = concat_node->GetOpDesc()->GetInputDesc(i);
    if (CalcTensorSize(tensor_desc, tensor_size, flag) != SUCCESS) {
      return false;
    }
    if (tensor_size % 32 != 0) {
      return false;
    }
  }
  return true;
}

void ConcatOptimizer::GetFirstOutAnchorNotInDeleteList(const ge::InDataAnchorPtr &input_anchor,
                                                       ge::OutDataAnchorPtr &src_anchor,
                                                       int current_deep) const {
  if (current_deep >= kDeepth) {
    return;
  }
  auto peer_out_anchor = input_anchor->GetPeerOutAnchor();
  if (peer_out_anchor == nullptr) {
      return;
  }
  auto peer_node = peer_out_anchor->GetOwnerNode();
  if (peer_node == nullptr) {
      return;
  }
  if (kGeDeleteOpType.count(peer_node->GetType()) != 0) {
    auto in_anchor = peer_node->GetInDataAnchor(0);
    if (in_anchor == nullptr) {
      FE_LOGD("node:%s in data anchor of %d is nullptr.", peer_node->GetName().c_str(), 0);
      return;
    }
    GetFirstOutAnchorNotInDeleteList(in_anchor, src_anchor, current_deep + 1);
  } else {
    src_anchor = peer_out_anchor;
  }
  return;
}

bool ConcatOptimizer::HasSameSrc(const ge::NodePtr &concat_node) const {
  set<ge::OutDataAnchorPtr> src_anchors;
  for (const ge::InDataAnchorPtr &input_anchor : concat_node->GetAllInDataAnchors()) {
    ge::OutDataAnchorPtr src_anchor = nullptr;
    GetFirstOutAnchorNotInDeleteList(input_anchor, src_anchor, 0);
    src_anchors.insert(src_anchor);
  }
  return src_anchors.size() != concat_node->GetAllInDataAnchors().size();
}

bool ConcatOptimizer::HasControlEdge(ge::NodePtr concat_node) {
  return concat_node->GetInControlNodes().size() != 0 || concat_node->GetOutControlNodes().size() != 0;
}

bool ConcatOptimizer::IsAiCoreOp(const ge::NodePtr &node) {
  ge::OpDescPtr op_desc = node->GetOpDesc();
  OpImplType op_impl_type;
  uint32_t imply_value = 0;
  (void)ge::AttrUtils::GetInt(op_desc, FE_IMPLY_TYPE, imply_value);
  op_impl_type = static_cast<OpImplType>(imply_value);
  bool is_aicore_op = false;
  is_aicore_op = op_impl_type == EN_IMPL_CUSTOM_TIK || op_impl_type == EN_IMPL_CUSTOM_TBE ||
                 op_impl_type == EN_IMPL_HW_TIK || op_impl_type == EN_IMPL_HW_TBE;
  return is_aicore_op;
}

bool ConcatOptimizer::ValidInputNode(ge::NodePtr concat_node) const {
  for (auto in_node : concat_node->GetInDataNodes()) {
    if (!IsAiCoreOp(in_node)) {
      return false;
    }
    std::string op_type = in_node->GetType();
    auto input_nodes = in_node->GetInDataNodes();
    if ((op_type == RESHAPE) && (!input_nodes.empty())) {
      op_type = input_nodes.at(0)->GetType();
    }
    std::vector<std::string> NOT_SUPPORT_TYPE = {DATA,
                                                 VARIABLE,
                                                 CONSTANTOP,
                                                 CONSTANT,
                                                 "HcomBroadcast",
                                                 "HcomAllGather",
                                                 "HcomAllReduce",
                                                 "HcomReduceScatter",
                                                 "HcomReduce"};
    bool not_support = false;
    not_support = std::find(NOT_SUPPORT_TYPE.begin(), NOT_SUPPORT_TYPE.end(), op_type) != NOT_SUPPORT_TYPE.end();
    if (not_support) {
      FE_LOGD("In node %s type is not support, %s can not optimize.", in_node->GetName().c_str(),
              concat_node->GetName().c_str());
      return false;
    }
  }
  return true;
}

bool ConcatOptimizer::InputCheck(ge::NodePtr concat_node) const {
  string node_name = concat_node->GetName();
  bool check32_align = Check32Align(concat_node) || concat_node->GetAllInDataAnchorsSize() == 1;
  bool has_same_src = HasSameSrc(concat_node);
  bool has_control_edge = HasControlEdge(concat_node);
  bool valid_input_node = ValidInputNode(concat_node);
  if (!check32_align) {
    FE_LOGD("Input tensor size of concat can not be divided by 32, %s can not optimize.", node_name.c_str());
  }
  if (has_same_src) {
    FE_LOGD("Concat has same input, %s can not optimize.", node_name.c_str());
  }
  if (has_control_edge) {
    FE_LOGD("Concat has control edge, %s can not optimize.", node_name.c_str());
  }
  if (!valid_input_node) {
    FE_LOGD("Concat node[%s] has place holder input, and it's parent op is Data.", node_name.c_str());
  }
  return check32_align && !has_same_src && !has_control_edge && valid_input_node;
}

bool ConcatOptimizer::OutputCheck(ge::NodePtr concat_node) const {
  for (auto output_anchor : concat_node->GetAllOutDataAnchors()) {
    for (size_t i = 0; i < output_anchor->GetPeerInDataAnchors().size(); i++) {
      auto peerAnchor = output_anchor->GetPeerInDataAnchors().at(i);
      FE_CHECK(peerAnchor == nullptr, FE_LOGD("Node %s in anchor is null", concat_node->GetName().c_str()),
               return false);
      auto next_node = peerAnchor->GetOwnerNode();
      auto output_nodes = next_node->GetOutDataNodes();
      if ((next_node->GetType() == RESHAPE) && (!output_nodes.empty())) {
        next_node = output_nodes.at(0);
      }
      ge::OpDescPtr next_node_desc = next_node->GetOpDesc();
      string next_node_name = next_node_desc->GetName();
      if (next_node_desc->GetType() == NETOUTPUT) {
        FE_LOGD("Next node %s is netoutput, %s can not optimize.", next_node_name.c_str(),
                concat_node->GetName().c_str());
        return false;
      }
      if (next_node_desc->GetType() == OP_TYPE_END) {
        string parent_op_type;
        (void)ge::AttrUtils::GetStr(next_node_desc, PARENT_OP_TYPE, parent_op_type);
        if (parent_op_type == NETOUTPUT) {
          FE_LOGD("Next node %s is End(netoutput), %s can not optimize.", next_node_name.c_str(),
                  concat_node->GetName().c_str());
          return false;
        }
      }
      bool is_virtual_op = false;
      bool no_task = false;
      bool output_reuse_input = false;
      bool no_padding_continuous_input = false;
      (void)ge::AttrUtils::GetBool(next_node_desc, ge::ATTR_NAME_NOTASK, no_task);
      (void)ge::AttrUtils::GetBool(next_node_desc, ge::ATTR_NAME_OUTPUT_REUSE_INPUT, output_reuse_input);
      (void)ge::AttrUtils::GetBool(next_node_desc, ge::ATTR_NAME_NOPADDING_CONTINUOUS_INPUT,
                                   no_padding_continuous_input);
      is_virtual_op = no_task || output_reuse_input || no_padding_continuous_input;
      if (is_virtual_op) {
        FE_LOGD("Next node %s has _no_task attribute, %s can't optimize.", next_node_name.c_str(),
                concat_node->GetName().c_str());
        return false;
      }
    }
  }
  return true;
}

// judge concat_node peer node whether can be reused
bool ConcatOptimizer::IsPeerNodeCanReUsed(const ge::NodePtr &concat_node) const {
  string node_name = concat_node->GetName();
  for (const ge::NodePtr &src_node : concat_node->GetInDataNodes()) {
    auto src_node_op_desc = src_node->GetOpDesc();
    if (src_node_op_desc->HasAttr(kATTR_CAN_REUSED_FOR_CONCAT_OPTIMIZE)) {
      bool can_reused = true;
      (void)ge::AttrUtils::GetBool(src_node_op_desc, kATTR_CAN_REUSED_FOR_CONCAT_OPTIMIZE, can_reused);
      if (!can_reused) {
        FE_LOGD("Concat [%s] peer node [%s] cannot reused, can not set no task flag",
                node_name.c_str(), src_node->GetName().c_str());
        return false;
      }
    }
  }
  FE_LOGD("Concat [%s] all peer node can reused, can set no task flag", node_name.c_str());
  return true;
}

void ConcatOptimizer::SetPeerNodeWhetherCanReUsed(const ge::NodePtr &concat_node) const {
  auto input_size = concat_node->GetAllInDataAnchorsSize();
  for (uint32_t index = 0; index < input_size; ++index) {
    auto input_anchor = concat_node->GetInDataAnchor(index);
    if (input_anchor == nullptr) {
      continue;
    }
    auto peer_out_anchor = input_anchor->GetPeerOutAnchor();
    if (peer_out_anchor == nullptr) {
      continue;
    }
    // set concat_node all peer node cannot reuse
    ge::AttrUtils::SetBool(peer_out_anchor->GetOwnerNode()->GetOpDesc(), kATTR_CAN_REUSED_FOR_CONCAT_OPTIMIZE, false);
  }
}

bool ConcatOptimizer::NeedSkip(const ge::NodePtr &node, const ge::OpDescPtr &op_desc) const {
  bool is_not_concat = op_desc->GetType() != fe::CONCATD && op_desc->GetType() != fe::CONCATV2D;
  string node_name = op_desc->GetName();
  if (is_not_concat) {
    return true;
  }
  int64_t concat_dim = -1;
  (void)GetRealConcatDimFromOriginalFormatToFormat(op_desc, concat_dim);
  if (concat_dim != 0) {
    FE_LOGD("concat_dim is not 0, %s can not optimize.", node_name.c_str());
    return true;
  }

  if (IsFeSupportedDynamicOp(*node->GetOpDesc())) {
    FE_LOGD("Concat op[%s] is unknown shape op, can not optimize.", node_name.c_str());
    return true;
  }

  if (!InputCheck(node)) {
    FE_LOGD("Concat input check failed, %s can not optimize.", node_name.c_str());
    return true;
  }

  if (!OutputCheck(node)) {
    return true;
  }

  if (InvalidMemType(op_desc)) {
    FE_LOGD("Concat mem type check failed, %s can not optimize.", node_name.c_str());
    return true;
  }

  if (!IsPeerNodeCanReUsed(node)) {
    return true;
  }
  return false;
}

bool ConcatOptimizer::IsPreNodeAttrValid(const ge::OpDescPtr &pre_node_desc, bool &fusion_virtual_op_flag,
                                         const string &node_name) const {
  string pre_node_name = pre_node_desc->GetName();
  bool is_continous_input = false;
  bool is_continous_output = false;
  bool is_ref = false;
  bool no_task = false;
  bool output_reuse_input = false;
  bool no_padding_continuous_input = false;
  vector<int64_t> output_index;
  (void)ge::AttrUtils::GetBool(pre_node_desc, ge::ATTR_NAME_CONTINUOUS_INPUT, is_continous_input);
  (void)ge::AttrUtils::GetBool(pre_node_desc, ge::ATTR_NAME_CONTINUOUS_OUTPUT, is_continous_output);
  (void)ge::AttrUtils::GetBool(pre_node_desc, ge::ATTR_NAME_REFERENCE, is_ref);
  (void)ge::AttrUtils::GetListInt(pre_node_desc, ge::ATOMIC_ATTR_OUTPUT_INDEX, output_index);
  (void)ge::AttrUtils::GetBool(pre_node_desc, ge::ATTR_NAME_NOTASK, no_task);
  (void)ge::AttrUtils::GetBool(pre_node_desc, ge::ATTR_NAME_OUTPUT_REUSE_INPUT, output_reuse_input);
  (void)ge::AttrUtils::GetBool(pre_node_desc, ge::ATTR_NAME_NOPADDING_CONTINUOUS_INPUT, no_padding_continuous_input);
  if (is_continous_input) {
    FE_LOGD("Previous node %s has continuous_input attribute, %s can't optimize.", pre_node_name.c_str(),
            node_name.c_str());
    fusion_virtual_op_flag = false;
    return false;
  }
  if (is_continous_output) {
    FE_LOGD("Previous node %s has continuous_output attribute, %s can't optimize.", pre_node_name.c_str(),
            node_name.c_str());
    fusion_virtual_op_flag = false;
    return false;
  }
  if (is_ref) {
    FE_LOGD("Previous node %s has reference attribute, %s can't optimize.", pre_node_name.c_str(), node_name.c_str());
    fusion_virtual_op_flag = false;
    return false;
  }
  bool is_virtual_op = no_task || output_reuse_input || no_padding_continuous_input;
  if (is_virtual_op) {
    FE_LOGD("Previous node %s has _no_task attribute, %s can't optimize.", pre_node_name.c_str(), node_name.c_str());
    fusion_virtual_op_flag = false;
    return false;
  }
  if (!output_index.empty()) {
    FE_LOGD("Previous node %s has atomic output, %s can not optimize.", pre_node_name.c_str(), node_name.c_str());
    fusion_virtual_op_flag = false;
    return false;
  }
  return true;
}

Status ConcatOptimizer::SetFusionVirtualOp(const ge::ComputeGraph &graph) const {
  FE_LOGD("start to do ConcatOptimizer");
  ge::ComputeGraph::Vistor<ge::NodePtr> nodes = graph.GetDirectNode();
  for (auto &node : nodes) {
    ge::OpDescPtr op_desc = node->GetOpDesc();
    string node_name = op_desc->GetName();
    if (NeedSkip(node, op_desc)) {
      FE_LOGD("node:[%s] need to skip", node_name.c_str());
      continue;
    }

    bool fusion_virtual_op_flag = true;
    for (auto input_anchor : node->GetAllInDataAnchors()) {
      ge::OutDataAnchorPtr pre_out_anchor = nullptr;
      GetFirstOutAnchorNotInDeleteList(input_anchor, pre_out_anchor, 0);
      if (pre_out_anchor == nullptr) {
        continue;
      }
      ge::OpDescPtr pre_node_desc = pre_out_anchor->GetOwnerNode()->GetOpDesc();
      FE_CHECK_NOTNULL(pre_node_desc);
      if (!IsPreNodeAttrValid(pre_node_desc, fusion_virtual_op_flag, node_name)) {
        break;
      }
    }

    if (fusion_virtual_op_flag) {
      FE_LOGD("node[%s] start to set concat attribute", node->GetName().c_str());
      (void)ge::AttrUtils::SetBool(op_desc, ge::ATTR_NAME_NOTASK, true);
      (void)ge::AttrUtils::SetBool(op_desc, ge::ATTR_NAME_NOPADDING_CONTINUOUS_INPUT, true);
      (void)ge::AttrUtils::SetBool(op_desc, ge::ATTR_NAME_OUTPUT_REUSE_INPUT, true);
      (void)ge::AttrUtils::SetInt(op_desc, ge::ATTR_NAME_REUSE_INPUT_ON_DIM_INDEX, 0);
      SetPeerNodeWhetherCanReUsed(node);
    }
  }
  FE_LOGD("end to do ConcatOptimizer");
  return fe::SUCCESS;
}
}  // namespace fe