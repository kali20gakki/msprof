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

#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/node_optimize/conv_concat_fusion_pass.h"
#include <map>
#include <string>
#include <vector>
#include "common/math_util.h"

namespace fe {
Status ConvConcatFusionPass::DoFusion(ge::ComputeGraph &graph, ge::NodePtr &node_ptr,
                                      vector<ge::NodePtr> &fusion_nodes) {
  string node_name = node_ptr->GetName();
  FE_LOGD("Node[%s]: start to ConvConcatFusionPass.", node_name.c_str());
  // 1. match the pattern
  if (MatchPattern(node_ptr) != SUCCESS) {
    FE_LOGD("Node[%s]: match not success, return NOT_CHANGED.", node_name.c_str());
    return NOT_CHANGED;
  }
  bool has_pooling = false;
  for (auto input_node : node_ptr->GetInAllNodes()) {
    if (input_node->GetType() == POOLING) {
      has_pooling = true;
      break;
    }
  }
  vector <ge::OpDescPtr> stride_write_op_desc_ptr_vec = {};
  if (InsertStrideWrite(graph, node_ptr, stride_write_op_desc_ptr_vec) != SUCCESS) {
      FE_LOGD("Node[%s]: insert stride write nodes failed, return failed.", node_name.c_str());
      return FAILED;
  }
  // if has quant after concat, move quant before concat
  // case1 : stridewrite-->concat-->quant to quant-->stridewrite-->concat; case2 : concat-->quant to quant-->concat
  if (concat_optimize_checker.IsDimCAlignedWithQuant(node_ptr) && !has_pooling) {
    for (size_t i = 0; i < node_ptr->GetAllOutDataAnchors().size(); i++) {
      // match and check quant
      ge::OutDataAnchorPtr out_data_anchor = node_ptr->GetOutDataAnchor(i);
      FE_CHECK_NOTNULL(out_data_anchor);
      auto in_anchors = out_data_anchor->GetPeerInDataAnchors();
      if (in_anchors.size() < 1) {
        REPORT_FE_ERROR("[GraphOpt][ConvConcatFus][DoFus] node[%s]'s peer_in_anchor size less than one.",
                        node_ptr->GetName().c_str());
        return SUCCESS;
      }
      FE_CHECK_NOTNULL(in_anchors.at(0));
      ge::NodePtr quant_node = in_anchors.at(0)->GetOwnerNode();
      FE_CHECK_NOTNULL(quant_node);
      // check all quant node is same or not
      Status ret = IsQuantNodeSame(quant_node, in_anchors);
      if (ret != SUCCESS) {
        return (ret == NOT_CHANGED ? SUCCESS : ret) ;
      }
      // fusion quant ,move quant before stridewrite
      ret = DoQuantFusion(graph, node_ptr, quant_node);
      if (ret != SUCCESS) {
        REPORT_FE_ERROR("[GraphOpt][ConvConcatFus][DoFus] fuse node:%s failed.", quant_node->GetName().c_str());
        return FAILED;
      }
    }
  }
  // set the stride attribute of StrideWrite and StrideRead
  for (ge::OpDescPtr stride_write_op_desc_ptr : stride_write_op_desc_ptr_vec) {
    (void)ge::AttrUtils::SetInt(stride_write_op_desc_ptr, STRIDE_ATTR_STRIDE,
                                node_ptr->GetOpDesc()->MutableOutputDesc(0)->MutableShape().GetDims()[1]);
  }
  FE_LOGD("Node[%s]: end to ConvConcatFusionPass.", node_name.c_str());
  return SUCCESS;
}

Status ConvConcatFusionPass::IsQuantNodeSame(const ge::NodePtr quant_node,
                                             const ge::OutDataAnchor::Vistor<ge::InDataAnchorPtr> &in_anchors) {
  if (quant_node->GetType() != QUANT) {
    FE_LOGW("Do not match quant after concat.");
    return NOT_CHANGED;
  }
  float scale = -1;
  float offset = -1;
  ge::DataType data_type = quant_node->GetOpDesc()->GetOutputDesc(0).GetDataType();
  if (!ge::AttrUtils::GetFloat(quant_node->GetOpDesc(), ATTR_SCALE, scale)) {
    REPORT_FE_ERROR("get scale attr for quant node failed!");
    return FAILED;
  }
  if (!ge::AttrUtils::GetFloat(quant_node->GetOpDesc(), ATTR_OFFSET, offset)) {
    REPORT_FE_ERROR("get offset attr for quant node failed!");
    return FAILED;
  }
  for (size_t j = 1; j < in_anchors.size(); ++j) {
    auto in_anchor = in_anchors.at(j);
    FE_CHECK_NOTNULL(in_anchor);
    ge::NodePtr next_node = in_anchor->GetOwnerNode();
    FE_CHECK_NOTNULL(next_node);
    if ((next_node->GetType() != QUANT)) {
      FE_LOGW("Do not match quant after concat.");
      return NOT_CHANGED;
    }
    float current_scale = -1;
    float current_offset = -1;
    ge::DataType current_data_type = next_node->GetOpDesc()->GetOutputDesc(0).GetDataType();
    if (!ge::AttrUtils::GetFloat(next_node->GetOpDesc(), ATTR_SCALE, current_scale)) {
      REPORT_FE_ERROR("get scale attr for quant node failed!");
      return FAILED;
    }
    if (!ge::AttrUtils::GetFloat(next_node->GetOpDesc(), ATTR_OFFSET, current_offset)) {
      REPORT_FE_ERROR("get offset attr for quant node failed!");
      return FAILED;
    }
    if (!FloatEqual(current_scale, scale) ||
        !FloatEqual(current_offset, offset) ||
        data_type != current_data_type) {
      FE_LOGW(
          "current quant sale or offset attr is different with the "
          "other quant, do not fuse quant.");
      return NOT_CHANGED;
    }
  }
  return SUCCESS;
}


Status ConvConcatFusionPass::InsertStrideWrite(ge::ComputeGraph &graph, const ge::NodePtr &node_ptr,
                                               vector<ge::OpDescPtr> &stride_write_op_desc_ptr_vec) {
  // 1. get attr value of concat_dim
  string node_name = node_ptr->GetName();
  ge::OpDescPtr op_desc_ptr = node_ptr->GetOpDesc();
  FE_CHECK_NOTNULL(op_desc_ptr);
  int64_t concat_dim_attr_value = GetDimAttrValue(op_desc_ptr, CONCAT_DIM, true);
  FE_LOGD("Node[%s]: the concat_dim_attr_value is [%ld].", node_name.c_str(), concat_dim_attr_value);

  // 2. set the attribute of Concat
  FE_LOGD("Node[%s]: set the attribute of Concat.", node_name.c_str());
  SetGeAttrForConcat(op_desc_ptr, 1);

  // 3. insert stride_write op
  FE_LOGD("Node[%s]: insert StrideWrite between the Concat and the previous op.", node_name.c_str());
  ge::Node::Vistor<ge::InDataAnchorPtr> all_in_data_anchors = node_ptr->GetAllInDataAnchors();
  for (size_t i = 0; i != all_in_data_anchors.size(); ++i) {
    ge::InDataAnchorPtr in_data_anchor_ptr = all_in_data_anchors.at(i);
    FE_CHECK_NOTNULL(in_data_anchor_ptr);
    ge::OutDataAnchorPtr peer_out_data_anchor_ptr = in_data_anchor_ptr->GetPeerOutAnchor();
    FE_CHECK_NOTNULL(peer_out_data_anchor_ptr);
    ge::NodePtr pre_node_ptr = peer_out_data_anchor_ptr->GetOwnerNode();
    FE_CHECK_NOTNULL(pre_node_ptr);

    // 4.1 create stride write op
    ge::OpDescPtr stride_write_op_desc_ptr;
    CreateStridedWrite(pre_node_ptr, stride_write_op_desc_ptr);

    // 4.2 insert stride write op
    ge::NodePtr stride_write_node = graph.AddNode(stride_write_op_desc_ptr);
    FE_CHECK_NOTNULL(stride_write_node);
    InsertNode(peer_out_data_anchor_ptr, in_data_anchor_ptr, stride_write_node);

    // 4.3 compute the stride
    stride_write_op_desc_ptr_vec.push_back(stride_write_op_desc_ptr);

    // 4.4 update the input desc of concat
    auto input_desc_ptr = node_ptr->GetOpDesc()->MutableInputDesc(i);
    if (input_desc_ptr == nullptr) {
      continue;
    }
    auto ori_data_type = node_ptr->GetOpDesc()->GetInputDesc(i).GetOriginDataType();
    (void)GetNC1HWC0Shape(input_desc_ptr, ori_data_type);
  }

  // 5. update the output desc of concat
  auto ori_data_type = node_ptr->GetOpDesc()->GetOutputDesc(0).GetOriginDataType();
  (void)GetNC1HWC0Shape(node_ptr->GetOpDesc()->MutableOutputDesc(0), ori_data_type);
  // set the stride attribute of StrideWrite and StrideRead
  int64_t stride = 0;
  for (ge::OpDescPtr stride_write_op_desc_ptr : stride_write_op_desc_ptr_vec) {
    if (node_ptr->GetOpDesc()->MutableOutputDesc(0)->MutableShape().GetDims().size() < DIM_SIZE) {
      REPORT_FE_ERROR("[GraphOpt][ConvConcatFus][InsStrdWrt] ConcatD output dim less than 2.");
      return FAILED;
    }
    stride = node_ptr->GetOpDesc()->MutableOutputDesc(0)->MutableShape().GetDims()[1];
    (void)ge::AttrUtils::SetInt(stride_write_op_desc_ptr, STRIDE_ATTR_STRIDE, stride);
  }
  FE_LOGD("Node[%s]: set the attribute of StrideWrite, the stride is [%ld]", node_name.c_str(), stride);
  return SUCCESS;
}

Status ConvConcatFusionPass::DoQuantFusion(ge::ComputeGraph &graph, ge::NodePtr concat_node, ge::NodePtr quant_node) {
  // create new quant op and add to graph
  FE_CHECK(quant_node == nullptr || quant_node->GetOpDesc()== nullptr,
    FE_LOGD("Get quant op desc failed."), return FAILED);
  ge::DataType quant_data_type = quant_node->GetOpDesc()->GetOutputDesc(0).GetDataType();
  for (size_t i = 0; i < concat_node->GetAllInDataAnchors().size(); ++i) {
    auto in_anchor = concat_node->GetInDataAnchor(i);
    FE_CHECK_NOTNULL(in_anchor);
    FE_CHECK_NOTNULL(in_anchor->GetPeerOutAnchor());
    auto pre_node = in_anchor->GetPeerOutAnchor()->GetOwnerNode();
    FE_CHECK_NOTNULL(pre_node);

    ge::OpDescPtr new_quant = ge::AttrUtils::CopyOpDesc(quant_node->GetOpDesc());
    new_quant->SetName(quant_node->GetName() + "_" + std::to_string(i));
    auto new_quant_node = graph.AddNode(new_quant);
    FE_CHECK_NOTNULL(new_quant_node);

    Status ret;
    if (pre_node->GetType() != STRIDEDWRITE) {
      FE_LOGW("concat previous node is not stride_write, move quant before concat");
      ret = InsertNode(in_anchor->GetPeerOutAnchor(), in_anchor, new_quant_node, quant_data_type);
    } else {
      FE_LOGW("concat previous node is stride_write, move quant before stride_write");
      ret = InsertNode(pre_node->GetInDataAnchor(0)->GetPeerOutAnchor(), pre_node->GetInDataAnchor(0),
                       new_quant_node, quant_data_type);
    }
    if (ret != SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][ConvConcatFus][DoQuantFus] add node:%s failed.", new_quant_node->GetName().c_str());
      return FAILED;
    }
    auto input_desc_ptr = concat_node->GetOpDesc()->MutableInputDesc(i);
    if (input_desc_ptr == nullptr) {
      continue;
    }
    GetNC1HWC0Shape(input_desc_ptr, quant_data_type);
    JudgeOp(pre_node);
  }
  GetNC1HWC0Shape(concat_node->GetOpDesc()->MutableOutputDesc(0), quant_data_type);
  // remove old quant op
  // Vector<ge::NodePtr
  ge::OutDataAnchorPtr out_data_anchor = concat_node->GetOutDataAnchor(0);
  FE_CHECK_NOTNULL(out_data_anchor);
  vector<ge::NodePtr> quant_nodes;
  for (size_t i = 0; i < out_data_anchor->GetPeerInDataAnchors().size(); ++i) {
    ge::NodePtr current_quan_node = out_data_anchor->GetPeerInDataAnchors().at(i)->GetOwnerNode();
    FE_CHECK_NOTNULL(current_quan_node);
    quant_nodes.push_back(current_quan_node);
  }
  for (size_t j = 0; j < quant_nodes.size(); ++j) {
    FE_LOGD("RemoveNode:%s", quant_nodes[j]->GetName().c_str());
    if (graph.RemoveNode(quant_nodes[j])) {
      REPORT_FE_ERROR("[GraphOpt][ConvConcatFus][DoQuantFus] remove node %s failed.",
                      quant_nodes[j]->GetName().c_str());
      return FAILED;
    }
  }

  return SUCCESS;
}

Status ConvConcatFusionPass::MatchPattern(const ge::NodePtr &node_ptr) {
  string node_name = node_ptr->GetName().c_str();
  // 1. check the concat node
  if (!concat_optimize_checker.Check(node_ptr)) {
    FE_LOGD("Node[%s]: check the concat node failed.", node_name.c_str());
    return FAILED;
  };
  // 2. check the graph, dequant or not
  bool is_dequant = false;
  ge::Node::Vistor<ge::InDataAnchorPtr> all_in_data_anchors = node_ptr->GetAllInDataAnchors();
  for (size_t i = 0; i != all_in_data_anchors.size(); ++i) {
    ge::InDataAnchorPtr in_data_anchor_ptr = all_in_data_anchors.at(i);
    FE_CHECK_NOTNULL(in_data_anchor_ptr);
    ge::OutDataAnchorPtr peer_out_data_anchor_ptr = in_data_anchor_ptr->GetPeerOutAnchor();
    FE_CHECK_NOTNULL(peer_out_data_anchor_ptr);
    ge::NodePtr pre_node_ptr = peer_out_data_anchor_ptr->GetOwnerNode();
    FE_CHECK_NOTNULL(pre_node_ptr);
    if (IsConvAndExpcetOp(pre_node_ptr, DEQUANT) == SUCCESS || IsDequantElemwise(pre_node_ptr) == SUCCESS) {
      is_dequant = true;
      break;
    }
  }  // 3. match the no-dequant graph
  if (!is_dequant) {
    return MatchForNoDequuant(node_ptr);
  } else {
    // 3. match the dequant graph
    return MatchForDequant(node_ptr);
  }
}

// all inputs: conv2d+concat
// all inputs: conv2d+relu+concat
// all inputs: conv2d+leakyrelu+concat
Status ConvConcatFusionPass::MatchForNoDequuant(const ge::NodePtr &node_ptr) {
  ge::Node::Vistor<ge::InDataAnchorPtr> all_in_data_anchors = node_ptr->GetAllInDataAnchors();
  ge::InDataAnchorPtr in_data_anchor_ptr0 = all_in_data_anchors.at(0);
  FE_CHECK_NOTNULL(in_data_anchor_ptr0);
  ge::OutDataAnchorPtr peer_out_data_anchor_ptr0 = in_data_anchor_ptr0->GetPeerOutAnchor();
  ge::NodePtr pre_node_ptr0 = peer_out_data_anchor_ptr0->GetOwnerNode();
  FE_CHECK_NOTNULL(pre_node_ptr0);
  ConvConcatFusionPattern match_pattern = GetMatchPattern(pre_node_ptr0);
  if (match_pattern == UN_SUPPORTED) {
    FE_LOGD("Node[%s]: check the no-dequant graph failed.", node_ptr->GetName().c_str());
    return FAILED;
  }

  for (size_t i = 1; i != all_in_data_anchors.size(); ++i) {
    ge::InDataAnchorPtr in_data_anchor_ptr = all_in_data_anchors.at(i);
    FE_CHECK_NOTNULL(in_data_anchor_ptr);
    ge::OutDataAnchorPtr peer_out_data_anchor_ptr = in_data_anchor_ptr->GetPeerOutAnchor();
    FE_CHECK_NOTNULL(peer_out_data_anchor_ptr);
    ge::NodePtr pre_node_ptr = peer_out_data_anchor_ptr->GetOwnerNode();
    FE_CHECK_NOTNULL(pre_node_ptr);
    Status is_max_pool = IsMaxPool(pre_node_ptr);
    ConvConcatFusionPattern pre_node_match_pattern = GetMatchPattern(pre_node_ptr);
    bool fail_pattern1 =
        match_pattern == PATTERN_CONV2D_CONCAT && (IsConv(pre_node_ptr) != SUCCESS && is_max_pool != SUCCESS);
    bool fail_pattern2 = match_pattern == PATTERN_CONV2D_RELU_CONCAT &&
                         (IsConvAndExpcetOp(pre_node_ptr, RELU) != SUCCESS && is_max_pool != SUCCESS);
    bool fail_pattern3 = match_pattern == PATTERN_CONV2D_LEAKYRELU_CONCAT &&
                         (IsLeakyRelu(pre_node_ptr) != SUCCESS && is_max_pool != SUCCESS);
    bool fail_pattern4 = match_pattern == PATTERN_MAXPOOL_CONCAT && (pre_node_match_pattern == UN_SUPPORTED);
    bool fail_pattern5 =
        match_pattern == PATTERN_CONV2D_REQUANT_CONCAT && (pre_node_match_pattern != PATTERN_CONV2D_REQUANT_CONCAT);
    bool fail_pattern6 =
        match_pattern == PATTERN_CONV2D_MISH_CONCAT && pre_node_match_pattern != PATTERN_CONV2D_MISH_CONCAT;
    bool fail_flag = fail_pattern1 || fail_pattern2 || fail_pattern3 || fail_pattern4 || fail_pattern5 || fail_pattern6;
    if (fail_flag) {
      FE_LOGD("Node[%s]: check the no-dequant graph failed.", node_ptr->GetName().c_str());
      return FAILED;
    }
  }
  return SUCCESS;
}

// some inputs: conv2d+dequant, other inputs: conv2d/conv2d+relu/conv2d+leakyrelu/conv2d+dequant
Status ConvConcatFusionPass::MatchForDequant(const ge::NodePtr &node_ptr) const {
  ge::Node::Vistor<ge::InDataAnchorPtr> all_in_data_anchors = node_ptr->GetAllInDataAnchors();
  for (size_t i = 0; i != all_in_data_anchors.size(); ++i) {
    ge::InDataAnchorPtr in_data_anchor_ptr = all_in_data_anchors.at(i);
    FE_CHECK_NOTNULL(in_data_anchor_ptr);
    ge::OutDataAnchorPtr peer_out_data_anchor_ptr = in_data_anchor_ptr->GetPeerOutAnchor();
    FE_CHECK_NOTNULL(peer_out_data_anchor_ptr);
    ge::NodePtr pre_node_ptr = peer_out_data_anchor_ptr->GetOwnerNode();
    FE_CHECK_NOTNULL(pre_node_ptr);
    ConvConcatFusionPattern pattern = GetMatchPattern(pre_node_ptr);
    if (pattern == UN_SUPPORTED) {
      FE_LOGD("Node[%s]: check the dequant graph failed.", node_ptr->GetName().c_str());
      return FAILED;
    }
  }
  return SUCCESS;
}

ConvConcatFusionPattern ConvConcatFusionPass::GetMatchPattern(const ge::NodePtr &pre_node_ptr) const {
  if (IsConv(pre_node_ptr) == SUCCESS) {
    return PATTERN_CONV2D_CONCAT;
  } else if (IsConvAndExpcetOp(pre_node_ptr, RELU) == SUCCESS) {
    return PATTERN_CONV2D_RELU_CONCAT;
  } else if (IsLeakyRelu(pre_node_ptr) == SUCCESS) {
    return PATTERN_CONV2D_LEAKYRELU_CONCAT;
  } else if (IsConvAndExpcetOp(pre_node_ptr, DEQUANT) == SUCCESS) {
    return PATTERN_CONV2D_DEQUANT_CONCAT;
  } else if (IsConvAndExpcetOp(pre_node_ptr, REQUANT) == SUCCESS) {
    return PATTERN_CONV2D_REQUANT_CONCAT;
  } else if (IsMaxPool(pre_node_ptr) == SUCCESS) {
    return PATTERN_MAXPOOL_CONCAT;
  } else if (IsDequantElemwise(pre_node_ptr) == SUCCESS) {
    return PATTERN_CONV2D_DEQUANT_LEAKYRELU_CONCAT;
  } else if (IsConvAndExpcetOp(pre_node_ptr, MISH) == SUCCESS) {
    return PATTERN_CONV2D_MISH_CONCAT;
  }
  return UN_SUPPORTED;
}

Status ConvConcatFusionPass::IsConv(const ge::NodePtr &pre_node_ptr) const {
  return (pre_node_ptr->GetType() == CONV2D || pre_node_ptr->GetType() == CONV2D_COMPRESS) &&
                 is_single_out_and_ref(pre_node_ptr)
             ? SUCCESS
             : FAILED;
}

Status ConvConcatFusionPass::IsMaxPool(const ge::NodePtr &pre_node_ptr) const {
  if (pre_node_ptr->GetType() == POOLING) {
    int64_t mode = 0;
    (void)ge::AttrUtils::GetInt(pre_node_ptr->GetOpDesc(), "mode", mode);
    if (mode == 1) {
      FE_LOGD("pooling is avgpool, not fuse.");
      return FAILED;
    }
  }
  bool is_pool = pre_node_ptr->GetType() == MAXPOOL ||
                 pre_node_ptr->GetType() == MAXPOOLV3 ||
                 pre_node_ptr->GetType() == POOLING;
  return (is_pool) && is_single_out_and_ref(pre_node_ptr) ? SUCCESS : FAILED;
}

Status ConvConcatFusionPass::IsConvAndExpcetOp(const ge::NodePtr &pre_node_ptr,
                                               const string &expect_op_type) const {
  if (pre_node_ptr->GetType() == expect_op_type && is_single_out_and_ref(pre_node_ptr)) {
    ge::NodePtr conv2d_node_ptr;
    Status status = NodeOptimizeUtils::GetPreNode(pre_node_ptr, 0, conv2d_node_ptr);
    if (status != SUCCESS) {
      FE_LOGD("Node[%s]: get previous node not success.", pre_node_ptr->GetName().c_str());
      return FAILED;
    }
    if ((conv2d_node_ptr->GetType() == CONV2D || conv2d_node_ptr->GetType() == CONV2D_COMPRESS) &&
        is_single_out_and_ref(conv2d_node_ptr)) {
      return SUCCESS;
    }
  }
  return FAILED;
}

Status ConvConcatFusionPass::IsDequantElemwise(const ge::NodePtr &pre_node_ptr) const {
  if ((pre_node_ptr->GetType() == LEAKYRELU || pre_node_ptr->GetType() == MISH) &&
      is_single_out_and_ref(pre_node_ptr)) {
    ge::NodePtr dequant_node_ptr;
    Status status = NodeOptimizeUtils::GetPreNode(pre_node_ptr, 0, dequant_node_ptr);
    if (status != SUCCESS) {
      FE_LOGD("Node[%s]: get previous node not success.", pre_node_ptr->GetName().c_str());
      return FAILED;
    }
    status = IsConvAndExpcetOp(dequant_node_ptr, DEQUANT);
    if (status == SUCCESS) {
      FE_LOGD("Node[%s]: The node structure matches with dequant-leakyrelu-concat.", pre_node_ptr->GetName().c_str());
      return SUCCESS;
    }
  }
  return FAILED;
}

Status ConvConcatFusionPass::IsLeakyRelu(const ge::NodePtr &pre_node_ptr) const {
  Status status = IsConvAndExpcetOp(pre_node_ptr, LEAKYRELU);
  if (status != SUCCESS) {
    FE_LOGD("Node[%s]: the node is not leakyrelu.", pre_node_ptr->GetName().c_str());
    return FAILED;
  }
  return SUCCESS;
}

vector<string> ConvConcatFusionPass::GetNodeTypes() {
  vector<string> result;
  result.push_back(CONCATD);
  result.push_back(CONCATV2D);
  return result;
}

string ConvConcatFusionPass::GetPatternName() { return "ConvConcatDPass"; }
}  // namespace fe
