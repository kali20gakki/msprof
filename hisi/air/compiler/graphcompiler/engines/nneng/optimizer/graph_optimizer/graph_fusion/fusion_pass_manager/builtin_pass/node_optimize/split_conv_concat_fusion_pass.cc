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

#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/node_optimize/split_conv_concat_fusion_pass.h"
#include <string>
#include "graph/debug/ge_attr_define.h"

namespace fe {

vector<string> SplitConvConcatFusionPass::GetNodeTypes() {
  vector<string> result;
  result.push_back(CONCATD);
  result.push_back(CONCATV2D);
  return result;
}

string SplitConvConcatFusionPass::GetPatternName() { return "SplitConvConcatFusionPass"; }

Status SplitConvConcatFusionPass::DoFusion(ge::ComputeGraph &graph, ge::NodePtr &concat,
                                           vector<ge::NodePtr> &fusion_nodes) {
  const char *switch_str = std::getenv("OFF_CONV_CONCAT_SPLIT");
  if (switch_str != nullptr) {
    FE_LOGD("skip the SplitConvConcatFusionPass.");
    return SUCCESS;
  }
  FE_LOGI("Define SplitConvConcatFusionPass fusion begin");
  // 1. check the Concat
  if (!concat_optimize_checker.CheckWithQuant(concat)) {
    return NOT_CHANGED;
  };
  vector<ge::NodePtr> vector_quant;
  vector<ge::NodePtr> vector_dequant;
  ge::NodePtr split_node = nullptr;
  // 2. parse quant and dequant node after split node and before concat node
  if (PatternConcatSplit(concat, split_node, vector_quant, vector_dequant) != SUCCESS) {
    return NOT_CHANGED;
  }
  if (CheckOutputSingleRef(concat) != SUCCESS) {
    return NOT_CHANGED;
  }
  // 3. check the Split
  if (!split_optimize_checker.Check(split_node)) {
    FE_LOGI("Split node[%s] is not matched, not do fusion.", split_node->GetName().c_str());
    return NOT_CHANGED;
  };
  ge::NodePtr strided_read_next_node = nullptr;
  bool condition = !vector_quant.empty() && !vector_dequant.empty() &&
                   (split_node->GetAllOutDataAnchors().size() != vector_quant.size() ||
                    vector_quant.size() != vector_dequant.size());
  if (condition) {
    return NOT_CHANGED;
  }
  FE_LOGD("Start to do fusion for split node[%s].", split_node->GetName().c_str());
  if (FusionSplit(vector_quant, graph, split_node) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][SplitConvConcatFus][DoFus] quant do split failed");
    return FAILED;
  }
  FE_LOGD("Start to do fusion for concat node[%s].", concat->GetName().c_str());
  if (FusionConcat(vector_dequant, graph, concat) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][SplitConvConcatFus][DoFus] dequant do fusion concat failed");
    return FAILED;
  }

  for (size_t i = 0; i < split_node->GetAllOutDataAnchors().size(); i++) {
    for (size_t j = 0; j < split_node->GetOutDataAnchor(i)->GetPeerInDataAnchors().size(); j++) {
      ge::NodePtr strided_read_node = split_node->GetOutDataAnchor(i)->GetPeerInDataAnchors().at(j)->GetOwnerNode();
      if (strided_read_node->GetOpDesc()->GetType() == STRIDEDREAD) {
        auto split_input_dims = split_node->GetOpDesc()->MutableInputDesc(0)->MutableShape().GetDims();
        if (split_input_dims.size() < 2) {
          REPORT_FE_ERROR("[GraphOpt][SplitConvConcatFus][DoFus] split %s's input dim number is less than 2.",
                          split_node->GetName().c_str());
          return FAILED;
        }
        (void)ge::AttrUtils::SetInt(strided_read_node->GetOpDesc(), STRIDE_ATTR_STRIDE, split_input_dims[1]);
      }
    }
  }

  for (size_t i = 0; i < concat->GetAllInDataAnchors().size(); i++) {
    ge::NodePtr strided_write_node = concat->GetInDataAnchor(i)->GetPeerOutAnchor()->GetOwnerNode();
    auto concat_out_dims = concat->GetOpDesc()->MutableOutputDesc(0)->MutableShape().GetDims();
    if (concat_out_dims.size() < 2) {
      REPORT_FE_ERROR("[GraphOpt][SplitConvConcatFus][DoFus] ConcatD %s's output dim number is less than 2.",
                      concat->GetName().c_str());
      return FAILED;
    }
    (void)ge::AttrUtils::SetInt(strided_write_node->GetOpDesc(), STRIDE_ATTR_STRIDE, concat_out_dims[1]);
  }
  // 3. set the attribute of Split
  FE_LOGD("Node[%s]: set the atrribute of Split.", split_node->GetName().c_str());
  SetGeAttrForSplit(split_node->GetOpDesc(), 1);
  // set the attribute of Concat
  FE_LOGD("Node[%s]: set the atrribute of Concat.", concat->GetName().c_str());
  SetGeAttrForConcat(concat->GetOpDesc(), 1);

  FE_LOGI("Define SplitConvConcatFusionPass fusion end");
  return SUCCESS;
}

inline bool IsConv2DSingleRef(const ge::NodePtr &node) {
  return node->GetType() == CONV2D && node->GetOutDataNodes().size() == 1;
}

Status SplitConvConcatFusionPass::CheckOutputSingleRef(ge::NodePtr &concat_node) {
  std::vector<std::string> op_type_before = {LEAKYRELU, DEQUANT};
  ge::NodePtr pre_node = nullptr;
  ge::NodePtr pre_pre_node = nullptr;
  size_t count_output_single = 0;
  for (auto &node : concat_node->GetInDataNodes()) {
    // meet Conv2D, return
    if (IsConv2DSingleRef(node)) {
      count_output_single++;
      continue;
    }
    if (node->GetOutDataNodes().size() != 1) {
      return NOT_CHANGED;
    }
    if (std::find(op_type_before.begin(), op_type_before.end(), node->GetType()) != op_type_before.end()) {
      pre_node = node->GetInDataNodes().at(0);
      if (IsConv2DSingleRef(pre_node)) {
        count_output_single++;
        continue;
      }
      if (pre_node->GetOutDataNodes().size() != 1) {
        return NOT_CHANGED;
      }
    }
    if (pre_node->GetType() == DEQUANT) {
      pre_pre_node = pre_node->GetInDataNodes().at(0);
      if (IsConv2DSingleRef(pre_pre_node)) {
        count_output_single++;
        continue;
      }
      if (pre_pre_node->GetOutDataNodes().size() != 1) {
        return NOT_CHANGED;
      }
    }
  }
  if (count_output_single != concat_node->GetInDataNodes().size()) {
    FE_LOGD("conv_output_single's size is %zu, conv2d's size is %zu.", count_output_single,
            concat_node->GetInDataNodes().size());
    return NOT_CHANGED;
  }
  return SUCCESS;
}

Status SplitConvConcatFusionPass::FusionSplit(vector<ge::NodePtr> &vector_quant, ge::ComputeGraph &graph,
                                              ge::NodePtr &split_node) {
  if (!vector_quant.empty()) {
    ge::OpDescPtr new_move_quant = ge::AttrUtils::CopyOpDesc(vector_quant[0]->GetOpDesc());
    auto quant_data_type = vector_quant[0]->GetOpDesc()->GetOutputDesc(0).GetDataType();
    new_move_quant->SetName(new_move_quant->GetName() + "_move");
    ge::NodePtr new_move_quant_node = graph.AddNode(new_move_quant);
    FE_CHECK_NOTNULL(new_move_quant_node);
    InsertNode(split_node->GetInDataAnchor(0)->GetPeerOutAnchor(), split_node->GetInDataAnchor(0),
               new_move_quant_node, quant_data_type);
  }

  for (size_t i = 0; i < split_node->GetAllOutDataAnchors().size(); i++) {
    auto quant_data_type = split_node->GetOpDesc()->GetOutputDesc(i).GetOriginDataType();
    ge::NodePtr strided_read_node = nullptr;
    if (!vector_quant.empty() && i < vector_quant.size()) {
      FE_CHECK(vector_quant[i] == nullptr || vector_quant[i]->GetOpDesc()== nullptr,
        FE_LOGD("Get quant op desc failed."), return FAILED);
      quant_data_type = vector_quant[i]->GetOpDesc()->GetOutputDesc(0).GetDataType();
      std::shared_ptr<ge::OpDesc> strided_read_opdesc;
      CreateStridedRead(vector_quant[i], strided_read_opdesc);
      strided_read_node = graph.AddNode(strided_read_opdesc);
      FE_CHECK_NOTNULL(strided_read_node);
      InsertNode(split_node->GetOutDataAnchor(i), vector_quant[i]->GetInDataAnchor(0), strided_read_node);
      if (graph.RemoveNode(vector_quant[i]) != SUCCESS) {
        REPORT_FE_ERROR("[GraphOpt][SplitConvConcatFus][FusSplit] remove %s failed",
                        vector_quant[i]->GetName().c_str());
        return FAILED;
      }
    } else {
      for (size_t j = 0; j < split_node->GetOutDataAnchor(i)->GetPeerInDataAnchors().size(); j++) {
        ge::NodePtr conv2d_node = split_node->GetOutDataAnchor(i)->GetPeerInDataAnchors().at(j)->GetOwnerNode();
        if (conv2d_node->GetType() == CONV2D) {
          std::shared_ptr<ge::OpDesc> strided_read_opdesc;
          CreateStridedRead(conv2d_node, strided_read_opdesc);
          strided_read_node = graph.AddNode(strided_read_opdesc);
          FE_CHECK_NOTNULL(strided_read_node);
          InsertNode(split_node->GetOutDataAnchor(i), conv2d_node->GetInDataAnchor(0), strided_read_node);
        }
      }
    }
    auto output_desc = split_node->GetOpDesc()->MutableOutputDesc(i);
    if (output_desc == nullptr) {
      continue;
    }
    if (GetNC1HWC0Shape(output_desc, quant_data_type) != SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][SplitConvConcatFus][FusSplit] Get shape of NC1HWC0 failed.");
      return FAILED;
    }
    JudgeOp(strided_read_node);
  }
  auto quant_data_type = split_node->GetOpDesc()->GetInputDesc(0).GetOriginDataType();
  if (!vector_quant.empty()) {
    quant_data_type = vector_quant[0]->GetOpDesc()->GetOutputDesc(0).GetDataType();
  }
  if (GetNC1HWC0Shape(split_node->GetOpDesc()->MutableInputDesc(0), quant_data_type) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][SplitConvConcatFus][FusSplit] Get shape of NC1HWC0 failed.");
    return FAILED;
  }
  return SUCCESS;
}

Status SplitConvConcatFusionPass::FusionConcat(vector<ge::NodePtr> &vector_dequant, ge::ComputeGraph &graph,
                                               ge::NodePtr &concat) {
  ge::NodePtr concat_next_node = concat->GetOutDataAnchor(0)->GetPeerInDataAnchors().at(0)->GetOwnerNode();
  ge::OpDescPtr new_move_quant_after_concat;
  if (concat_next_node->GetType() == QUANT || (concat_next_node->GetType() == LEAKYRELU) ||
      concat_next_node->GetType() == RELU) {
    if (graph.RemoveNode(concat_next_node) != SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][SplitConvConcatFus][FusConcat] remove %s failed",
                      concat_next_node->GetName().c_str());
      return FAILED;
    }
  }
  ge::DataType quant_data_type = concat_next_node->GetOpDesc()->GetOutputDesc(0).GetDataType();
  for (size_t i = 0; i < concat->GetAllInDataAnchors().size(); i++) {
    ge::NodePtr strided_write_node = nullptr;
    if (!vector_dequant.empty() && i < vector_dequant.size()) {
      std::shared_ptr<ge::OpDesc> strided_write_opdesc;
      CreateStridedWrite(vector_dequant[i], strided_write_opdesc);
      strided_write_node = graph.AddNode(strided_write_opdesc);
      FE_CHECK_NOTNULL(strided_write_node);
      InsertNode(concat->GetInDataAnchor(i)->GetPeerOutAnchor(), concat->GetInDataAnchor(i), strided_write_node);

      if (concat_next_node->GetType() == QUANT) {
        ge::OpDescPtr move_quant = ge::AttrUtils::CopyOpDesc(concat_next_node->GetOpDesc());
        move_quant->SetName(move_quant->GetName() + "_" + std::to_string(i));
        ge::NodePtr move_quant_node = graph.AddNode(move_quant);
        FE_CHECK_NOTNULL(move_quant_node);
        InsertNode(strided_write_node->GetInDataAnchor(0)->GetPeerOutAnchor(), strided_write_node->GetInDataAnchor(0),
                   move_quant_node, quant_data_type);
      }
    } else {
      ge::NodePtr conv2d_node = concat->GetInDataAnchor(i)->GetPeerOutAnchor()->GetOwnerNode();
      std::shared_ptr<ge::OpDesc> strided_write_opdesc;
      CreateStridedWrite(conv2d_node, strided_write_opdesc);
      strided_write_node = graph.AddNode(strided_write_opdesc);
      FE_CHECK_NOTNULL(strided_write_node);
      InsertNode(concat->GetInDataAnchor(i)->GetPeerOutAnchor(), concat->GetInDataAnchor(i), strided_write_node);

      if ((concat_next_node->GetType() == LEAKYRELU) || concat_next_node->GetType() == RELU) {
        ge::OpDescPtr move_relu = ge::AttrUtils::CopyOpDesc(concat_next_node->GetOpDesc());
        move_relu->SetName(move_relu->GetName() + "_" + std::to_string(i));
        ge::NodePtr move_relu_node = graph.AddNode(move_relu);
        FE_CHECK_NOTNULL(move_relu_node);
        InsertNode(strided_write_node->GetInDataAnchor(0)->GetPeerOutAnchor(), strided_write_node->GetInDataAnchor(0),
                   move_relu_node);
      }
    }
    auto input_desc_ptr = concat->GetOpDesc()->MutableInputDesc(i);
    if (input_desc_ptr == nullptr) {
      continue;
    }
    GetNC1HWC0Shape(input_desc_ptr, quant_data_type);
    JudgeOp(strided_write_node);
  }
  GetNC1HWC0Shape(concat->GetOpDesc()->MutableOutputDesc(0), quant_data_type);
  return SUCCESS;
}

Status CheckQuantAndConvBeforeConcat(ge::InDataAnchorPtr &quant_in_data_anchor_ptr, ge::NodePtr &concat_prev_node,
                                     bool &is_quant, bool &is_conv, int32_t &has_quant) {
  is_quant = (has_quant == -1 || has_quant == 1) && quant_in_data_anchor_ptr != nullptr;
  is_conv = (has_quant == -1 || has_quant == 0) && concat_prev_node->GetType() == CONV2D;
  ge::NodePtr leaky_relu_prev_node;
  if (concat_prev_node->GetType() == "LeakyRelu") {
    leaky_relu_prev_node = concat_prev_node->GetInDataAnchor(0)->GetPeerOutAnchor()->GetOwnerNode();
    is_conv = (has_quant == -1 || has_quant == 0) && leaky_relu_prev_node->GetType() == CONV2D;
  }
  if (is_quant) {
    has_quant = 1;
  } else if (is_conv) {
    has_quant = 0;
  } else {
    return NOT_CHANGED;
  }
  return SUCCESS;
}

Status ParseAndCheckSplitNode(ge::InDataAnchorPtr &quant_in_data_anchor_ptr, ge::NodePtr &concat_prev_node,
                              ge::NodePtr &split, int32_t &has_quant, string &split_name) {
  if (has_quant == 1) {
    split = quant_in_data_anchor_ptr->GetPeerOutAnchor()->GetOwnerNode();
  } else if (concat_prev_node->GetType() == "LeakyRelu") {
    ge::NodePtr leaky_relu_prev_node = concat_prev_node->GetInDataAnchor(0)->GetPeerOutAnchor()->GetOwnerNode();
    split = leaky_relu_prev_node->GetInDataAnchor(0)->GetPeerOutAnchor()->GetOwnerNode();
  } else {
    split = concat_prev_node->GetInDataAnchor(0)->GetPeerOutAnchor()->GetOwnerNode();
  }
  if (split->GetType() != SPLITD && split->GetType() != SPLITVD) {
    return NOT_CHANGED;
  }
  if (split_name.empty()) {
    split_name = split->GetName();
  } else if (split_name != split->GetName()) {
    return NOT_CHANGED;
  }
  return SUCCESS;
}

Status SplitConvConcatFusionPass::PatternConcatSplit(ge::NodePtr &concat, ge::NodePtr &split_node,
                                                     vector<ge::NodePtr> &vector_quant,
                                                     vector<ge::NodePtr> &vector_dequant) {
  int32_t has_quant = -1;
  string split_name;
  ge::NodePtr quant = nullptr;
  ge::NodePtr dequant = nullptr;
  ge::NodePtr split = nullptr;
  bool is_quant = false;
  bool is_conv = false;
  for (ge::InDataAnchorPtr &concat_in_data_anchor : concat->GetAllInDataAnchors()) {
    ge::OutDataAnchorPtr concat_peer_out_anchor = concat_in_data_anchor->GetPeerOutAnchor();
    FE_CHECK_NOTNULL(concat_peer_out_anchor);
    ge::NodePtr concat_prev_node = concat_peer_out_anchor->GetOwnerNode();
    FE_CHECK_NOTNULL(concat_prev_node);
    ge::InDataAnchorPtr quant_in_data_anchor_ptr;
    if (concat_prev_node->GetType() == LEAKYRELU) {
      ge::InDataAnchorPtr concat_prev_in_data_anchor = concat_prev_node->GetInDataAnchor(0);
      FE_CHECK_NOTNULL(concat_prev_in_data_anchor);
      ge::OutDataAnchorPtr concat_prev_peer_out_anchor = concat_prev_in_data_anchor->GetPeerOutAnchor();
      FE_CHECK_NOTNULL(concat_prev_peer_out_anchor);
      quant_in_data_anchor_ptr = PatternPrevConv2dWithQuant(concat_prev_peer_out_anchor, quant, dequant);
    } else {
      quant_in_data_anchor_ptr = PatternPrevConv2dWithQuant(concat_peer_out_anchor, quant, dequant);
    }
    if (quant_in_data_anchor_ptr != nullptr) {
      vector_quant.push_back(quant);
      vector_dequant.push_back(dequant);
    }
    Status ret =
        CheckQuantAndConvBeforeConcat(quant_in_data_anchor_ptr, concat_prev_node, is_quant, is_conv, has_quant);
    if (ret != SUCCESS) {
      return ret;
    }
    ret = ParseAndCheckSplitNode(quant_in_data_anchor_ptr, concat_prev_node, split, has_quant, split_name);
    if (ret != SUCCESS) {
      return ret;
    }
  }
  size_t count = 0;
  for (size_t i = 0; i < split->GetAllOutDataAnchors().size(); i++) {
    for (size_t j = 0; j < split->GetOutDataAnchor(i)->GetPeerInDataAnchors().size(); j++) {
      count++;
    }
  }

  if (concat->GetAllInDataAnchors().size() != count) {
    return NOT_CHANGED;
  }
  split_node = split;
  return SUCCESS;
}

ge::InDataAnchorPtr SplitConvConcatFusionPass::PatternPrevConv2dWithQuant(ge::OutDataAnchorPtr out_anchor,
                                                                          ge::NodePtr &quant, ge::NodePtr &dequant) {
  ge::NodePtr ascend_dequant = out_anchor->GetOwnerNode();
  if (ascend_dequant == nullptr || ascend_dequant->GetType() != DEQUANT) {
    return nullptr;
  }

  ge::InDataAnchorPtr dequant_in_anchor = ascend_dequant->GetInDataAnchor(0);
  if (dequant_in_anchor == nullptr) {
    return nullptr;
  }
  ge::OutDataAnchorPtr dequant_peer_out_anchor = dequant_in_anchor->GetPeerOutAnchor();
  if (dequant_peer_out_anchor == nullptr) {
    return nullptr;
  }
  ge::NodePtr conv2d = dequant_peer_out_anchor->GetOwnerNode();
  if (conv2d == nullptr || conv2d->GetType() != CONV2D) {
    return nullptr;
  }

  ge::InDataAnchorPtr conv2d_in_anchor = conv2d->GetInDataAnchor(0);
  if (conv2d_in_anchor == nullptr) {
    return nullptr;
  }
  ge::OutDataAnchorPtr conv2d_peer_out_anchor = conv2d_in_anchor->GetPeerOutAnchor();
  if (conv2d_peer_out_anchor == nullptr) {
    return nullptr;
  }
  ge::NodePtr ascend_quant = conv2d_peer_out_anchor->GetOwnerNode();
  if (ascend_quant == nullptr || ascend_quant->GetType() != QUANT) {
    return nullptr;
  }
  quant = ascend_quant;
  dequant = ascend_dequant;
  return quant->GetInDataAnchor(0);
}
}  // namespace fe
