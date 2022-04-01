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

#include "pooling_quant_process_fusion_pass.h"
#include "bias_optimize_quant_rollback_base.h"
#include "common/math_util.h"

namespace fe {
static const string POOLING_MODE = "mode";
const size_t input_index_two = 2;
const size_t input_index_three = 3;
const size_t pad_index_two = 2;
const size_t pad_index_three = 3;
/*
 *  fusion pattern
 *
 *  AscendQuant--->Pooling--->AscendDequant--->output
 *                               ^
 *                              /
 *                             /
 *                         deq_scale
 */
vector<FusionPattern *> PoolingQuantProcessFusionPass::DefinePatterns() {
  vector<FusionPattern *> patterns;
  FE_LOGD("Start to define pooling quant process fusion pattern.");
  FusionPattern *pattern = new (std::nothrow) FusionPattern("PoolingQuantProcessFusion");
  FE_CHECK(pattern == nullptr, FE_LOGW("new FusionPattern object failed!"), return patterns);
  pattern->AddOpDesc(PATTERN_QUANT, {QUANT})
      .AddOpDesc(PATTERN_CUBE, {POOLING})
      .AddOpDesc(PATTERN_DEQUANT, {DEQUANT})
      .SetInputs(PATTERN_CUBE, {PATTERN_QUANT})
      .SetInputs(PATTERN_DEQUANT, {PATTERN_CUBE})
      .SetOutput(PATTERN_DEQUANT);
  patterns.push_back(pattern);

  return patterns;
}

Status PoolingQuantProcessFusionPass::SetDataTypeOfPooling(const ge::NodePtr &pooling_node) const {
  ge::DataType set_target_dtype = ge::DT_INT32;

  ge::OpDescPtr pooling_op = pooling_node->GetOpDesc();
  for (size_t i = 0; i < pooling_node->GetAllOutDataAnchors().size(); ++i) {
    ge::GeTensorDesc output_desc = pooling_op->GetOutputDesc(i);
    output_desc.SetDataType(set_target_dtype);
    output_desc.SetOriginDataType(set_target_dtype);
    if (pooling_op->UpdateOutputDesc(i, output_desc) != ge::GRAPH_SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][PoolQntPcsFus][SetPoolDtype] update output desc of Node[%s] not success.",
                      pooling_op->GetName().c_str());
      return FAILED;
    }
    auto out_data_anchor = pooling_node->GetOutDataAnchor(i);
    FE_CHECK(out_data_anchor == nullptr,
             REPORT_FE_ERROR("[GraphOpt][PoolQntPcsFus][SetPoolDtype] outDataAnchor is null"), return FAILED);
    auto after_in_data_anchor = out_data_anchor->GetPeerInDataAnchors().at(i);
    FE_CHECK(after_in_data_anchor == nullptr,
             REPORT_FE_ERROR("[GraphOpt][PoolQntPcsFus][SetPoolDtype] afterInDataAnchor is null"), return FAILED);
    /* Set the output desc data type as int32 for the output of host cpu op */
    ge::NodePtr output_node = after_in_data_anchor->GetOwnerNode();
    FE_CHECK(output_node == nullptr, REPORT_FE_ERROR("[GraphOpt][PoolQntPcsFus][SetPoolDtype] outputNode is null"),
             return FAILED);

    ge::OpDescPtr op = output_node->GetOpDesc();
    FE_CHECK(op == nullptr, REPORT_FE_ERROR("[GraphOpt][PoolQntPcsFus][SetPoolDtype] op is nullptr"), return FAILED);
    ge::GeTensorDesc input_desc = op->GetInputDesc(after_in_data_anchor->GetIdx());
    input_desc.SetDataType(set_target_dtype);
    input_desc.SetOriginDataType(set_target_dtype);
    if (op->UpdateInputDesc(after_in_data_anchor->GetIdx(), input_desc) != ge::GRAPH_SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][PoolQntPcsFus][SetPoolDtype] update input desc of [%s] not success.",
                      op->GetName().c_str());
      return FAILED;
    }
  }
  return SUCCESS;
}

bool PoolingQuantProcessFusionPass::CheckParameters(const ge::NodePtr &pooling_node, vector<int64_t> &input,
                                                    vector<int64_t> &window, vector<int64_t> &stride,
                                                    const int64_t &ceil_mode, vector<int64_t> &pad) const {
  if (input.size() != 4) {
    REPORT_FE_ERROR("[GraphOpt][PoolQntPcsFus][ChkParm] Pooling node [%s] dim_info is not right, please check!",
                    pooling_node->GetName().c_str());
    return false;
  }
  if (window.size() != 2) {
    REPORT_FE_ERROR("[GraphOpt][PoolQntPcsFus][ChkParm] Pooling node [%s] window is not right, please check!",
                    pooling_node->GetName().c_str());
    return false;
  }
  if (stride.size() != 2) {
    REPORT_FE_ERROR("[GraphOpt][PoolQntPcsFus][ChkParm] Pooling node [%s] stride is not right, please check!",
                    pooling_node->GetName().c_str());
    return false;
  }
  if (ceil_mode != 0 && ceil_mode != 1) {
    REPORT_FE_ERROR("[GraphOpt][PoolQntPcsFus][ChkParm] Pooling node [%s] ceil_mode is not right, please check!",
                    pooling_node->GetName().c_str());
    return false;
  }
  if (pad.size() != 4) {
    REPORT_FE_ERROR("[GraphOpt][PoolQntPcsFus][ChkParm] Pooling node [%s] pad is not right, please check!",
                    pooling_node->GetName().c_str());
    return false;
  }
  return true;
}

bool PoolingQuantProcessFusionPass::IsMeanValueAllEqual(const ge::NodePtr &pooling_node, bool &is_mean_value_equal) {
  ge::OpDescPtr pooling_desc = pooling_node->GetOpDesc();

  // get Pooling input shape
  auto input_shape = pooling_node->GetOpDesc()->MutableInputDesc(0)->MutableShape();
  vector<int64_t> input = input_shape.GetDims();

  // get windowsize
  vector<int64_t> window;
  (void)ge::AttrUtils::GetListInt(pooling_desc, "window", window);

  // get stride
  vector<int64_t> stride;
  (void)ge::AttrUtils::GetListInt(pooling_desc, "stride", stride);

  // get pooling ceil_mode
  int64_t ceil_mode = -1;
  (void)ge::AttrUtils::GetInt(pooling_desc, "ceil_mode", ceil_mode);

  // get pad
  vector<int64_t> pad;
  (void)ge::AttrUtils::GetListInt(pooling_desc, "pad", pad);

  if (!CheckParameters(pooling_node, input, window, stride, ceil_mode, pad)) {
    REPORT_FE_ERROR("[GraphOpt][PoolQntPcsFus][IsMeanValEqu] Pooling node [%s] parameter is not right, please check!",
                    pooling_node->GetName().c_str());
    return PARAM_INVALID;
  }

  int64_t out_size_h = 0;
  int64_t out_size_w = 0;
  // calculate out_size_h and out_size_w
  FE_FLOAT_ZEROCHECK(static_cast<double>(stride[0]));
  FE_FLOAT_ZEROCHECK(static_cast<double>(stride[1]));
  FE_INT64_ADDCHECK(input[input_index_two], pad[0]);
  FE_INT64_ADDCHECK(input[input_index_two] + pad[0], pad[1]);
  FE_INT64_SUBCHECK(input[input_index_two] + pad[0] + pad[1], window[0]);
  FE_INT64_ADDCHECK(input[input_index_three], pad[pad_index_two]);
  FE_INT64_ADDCHECK(input[input_index_three] + pad[pad_index_two], pad[pad_index_three]);
  FE_INT64_SUBCHECK(input[input_index_three] + pad[pad_index_two] + pad[pad_index_three], window[1]);
  if (ceil_mode == 0) {
    out_size_h = static_cast<int>(ceil(static_cast<float>(input[2] + pad[0] + pad[1] - window[0]) / stride[0])) + 1;
    out_size_w = static_cast<int>(ceil(static_cast<float>(input[3] + pad[2] + pad[3] - window[1]) / stride[1])) + 1;
  } else {
    out_size_h = static_cast<int>(floor(static_cast<float>(input[2] + pad[0] + pad[1] - window[0]) / stride[0])) + 1;
    out_size_w = static_cast<int>(floor(static_cast<float>(input[3] + pad[2] + pad[3] - window[1]) / stride[1])) + 1;
  }

  // If we have padding, ensure that the last pooling starts strictly
  // inside the image (instead of at the padding); otherwise clip the last.
  if (pad[0] != 0 || pad[1] != 0) {
    FE_INT64_MULCHECK(out_size_h - 1, stride[0]);
    FE_INT64_MULCHECK(out_size_w - 1, stride[1]);
    FE_INT64_ADDCHECK(input[input_index_two], pad[0]);
    FE_INT64_ADDCHECK(input[input_index_three], pad[pad_index_two]);
    if ((out_size_h - 1) * stride[0] >= input[2] + pad[0]) {
      --out_size_h;
    }
    if ((out_size_w - 1) * stride[1] >= input[3] + pad[2]) {
      --out_size_w;
    }

    if ((out_size_h - 1) * stride[0] >= input[2] + pad[0]) {
      REPORT_FE_ERROR(
          "[GraphOpt][PoolQntPcsFus][IsMeanValEqu] Node:[%s] CHECK_LT((out_size_h - 1) * stride_h, \
          in_size_h + pad_top)", pooling_node->GetName().c_str());
      return PARAM_INVALID;
    }
    if ((out_size_w - 1) * stride[1] >= input[3] + pad[2]) {
      REPORT_FE_ERROR(
          "[GraphOpt][PoolQntPcsFus][IsMeanValEqu] Node:[%s] CHECK_LT((out_size_w - 1) * stride_w, \
          in_size_w + pad_left)", pooling_node->GetName().c_str());
      return PARAM_INVALID;
    }
  }

  int64_t h_start = 0;
  int64_t w_start = 0;
  int64_t h_end = 0;
  int64_t w_end = 0;
  int64_t area = 0;
  for (int64_t steps_h = 0; steps_h < out_size_h; steps_h++) {
    for (int64_t steps_w = 0; steps_w < out_size_w; steps_w++) {
      FE_INT64_MULCHECK(steps_h, stride[0]);
      FE_INT64_MULCHECK(steps_w, stride[1]);
      FE_INT64_SUBCHECK(steps_h * stride[0], pad[0]);
      FE_INT64_SUBCHECK(steps_w * stride[1], pad[pad_index_two]);
      h_start = steps_h * stride[0] - pad[0];
      w_start = steps_w * stride[1] - pad[2];
      FE_INT64_ADDCHECK(h_start, window[0]);
      FE_INT64_ADDCHECK(input[input_index_two], pad[0]);
      FE_INT64_ADDCHECK(w_start, window[1]);
      FE_INT64_ADDCHECK(input[input_index_three], pad[pad_index_two]);
      h_end = min(h_start + window[0], input[2] + pad[0]);
      w_end = min(w_start + window[1], input[3] + pad[2]);
      FE_INT64_SUBCHECK(h_end, h_start);
      FE_INT64_SUBCHECK(w_end, w_start);
      FE_INT64_MULCHECK(h_end - h_start, w_end - w_start);
      area = max((h_end - h_start) * (w_end - w_start), static_cast<int64_t>(1));
      FE_INT64_MULCHECK(window[0], window[1]);
      if (area != window[0] * window[1]) {
        is_mean_value_equal = false;
        return SUCCESS;
      }
    }
  }
  is_mean_value_equal = true;
  return SUCCESS;
}

Status PoolingQuantProcessFusionPass::GetQuantProcessMode(ge::NodePtr &quant_node, ge::NodePtr &cube_node,
                                                          QuantProcessMode &quant_process_mode) {
  if (!IsAvgPooling(cube_node)) {
    FE_LOGW("Only AvgPooling need quant optimize. op name %s.", cube_node->GetName().c_str());
    return FAILED;
  }

  int32_t index_ci;
  ge::OpDescPtr op_desc_ptr = quant_node->GetOpDesc();
  auto input_shape = op_desc_ptr->MutableInputDesc(0)->MutableShape();
  ge::Format input_format = static_cast<ge::Format>(ge::GetPrimaryFormat(op_desc_ptr->GetInputDescPtr(0)->GetFormat()));
  index_ci = GetAxisIndexByFormat(input_format, "C");
  if (index_ci < 0) {
    REPORT_FE_ERROR("[GraphOpt][PoolQntPcsFus][GetQntPcsMode] Can not get C index of format [%s]",
                    ge::TypeUtils::FormatToSerialString(input_format).c_str());
    return FAILED;
  }

  // JudgeUnknownShape
  if (IsUnknownShapeValue(input_shape.GetDim(index_ci))) {
    FE_LOGW("Bias optimize cannot be applied for unknown shape.");
    quant_process_mode = QuantProcessMode::QUANT_UNDIFINED;
    return SUCCESS;
  }

  // judge whether need quant rollback
  // avgpool output shape must be 4-D
  auto output_tensor = cube_node->GetOpDesc()->MutableOutputDesc(0);
  auto input_tensor = cube_node->GetOpDesc()->MutableInputDesc(0);
  if (output_tensor->MutableShape().GetDimNum() != NCHW_DIMENSION_NUM) {
    REPORT_FE_ERROR("[GraphOpt][PoolQntPcsFus][GetQntPcsMode] AvgPool node [%s] output shape is not 4-D.",
                    cube_node->GetName().c_str());
    return FAILED;
  }
  // if out_w == 1, do rollback
  int32_t out_w_index =
      GetAxisIndexByFormat(static_cast<ge::Format>(ge::GetPrimaryFormat(output_tensor->GetFormat())), "W");
  if (out_w_index < 0) {
    REPORT_FE_ERROR("[GraphOpt][PoolQntPcsFus][GetQntPcsMode] Node [%s] output w index is negative",
                    cube_node->GetName().c_str());
    return FAILED;
  }
  int32_t in_c_index =
      GetAxisIndexByFormat(static_cast<ge::Format>(ge::GetPrimaryFormat(input_tensor->GetFormat())), "C");
  if (in_c_index < 0) {
    REPORT_FE_ERROR("[GraphOpt][PoolQntPcsFus][GetQntPcsMode] Node [%s] input c index is negative",
                    cube_node->GetName().c_str());
    return FAILED;
  }

  bool is_mean_value_equal = false;
  if (IsMeanValueAllEqual(cube_node, is_mean_value_equal) != SUCCESS) {
    return FAILED;
  }

  ISAArchVersion isa_arch_ver = Configuration::Instance(AI_CORE_NAME).GetIsaArchVer();
  (void)ge::AttrUtils::SetInt(cube_node->GetOpDesc(), "isaArchVer", static_cast<int64_t>(isa_arch_ver));
  if (output_tensor->MutableShape().GetDim(out_w_index) == OUT_W_DIM_VALUE ||
      (!is_mean_value_equal && (isa_arch_ver == ISAArchVersion::EN_ISA_ARCH_V200))) {
    quant_process_mode = QuantProcessMode::QUANT_ROLLBACK;
    return SUCCESS;
  }
  quant_process_mode = QuantProcessMode::BIAS_OPTIMIZE;
  return SUCCESS;
}

Status PoolingQuantProcessFusionPass::BiasOptimize(ge::ComputeGraph &graph, ge::NodePtr &cube_node,
                                                   ge::NodePtr &dequant_node, ge::NodePtr &quant_node,
                                                   vector<ge::NodePtr> &fusion_nodes) {
  if (JudgeDeqscaleShape(dequant_node) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][PoolQntPcsFus][BiasOpti] Judge Node[%s] deq_scale failed.",
                    dequant_node->GetName().c_str());
    return FAILED;
  }
  if (SetQuantParameters(cube_node, quant_node) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][PoolQntPcsFus][BiasOpti] Set quant paras to cube node[%s] failed.",
                    cube_node->GetName().c_str());
    return FAILED;
  }
  if (SetDataTypeOfPooling(cube_node) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][PoolQntPcsFus][BiasOpti] Set data type Of Pooling[%s] failed.",
                    cube_node->GetName().c_str());
    return FAILED;
  }
  /*
   * set isa version to pooling op
   * for in original graph fusion pass, pooling may be coverted to conv2d
   * then do bias optimize
   */
  ISAArchVersion isa_arch_ver = Configuration::Instance(AI_CORE_NAME).GetIsaArchVer();
  (void)ge::AttrUtils::SetInt(cube_node->GetOpDesc(), "isaArchVer", static_cast<int64_t>(isa_arch_ver));
  return SUCCESS;
}

bool PoolingQuantProcessFusionPass::IsAvgPooling(const ge::NodePtr &node) const {
  int64_t mode = -1;
  if (!ge::AttrUtils::GetInt(node->GetOpDesc(), POOLING_MODE, mode)) {
    FE_LOGD("node[%s] failed to get mode attr.", node->GetName().c_str());
    return false;
  }
  int64_t AVGPOOLING_MODE = 1;
  if (mode != AVGPOOLING_MODE) {
    FE_LOGW("node:%s mode is not avg pooling, mode %ld.", node->GetName().c_str(), mode);
    return false;
  }
  return true;
}

Status PoolingQuantProcessFusionPass::QuantRollback(ge::ComputeGraph &graph, ge::NodePtr &cube_node,
                                                    ge::NodePtr &dequant_ndoe, ge::NodePtr &quant_node,
                                                    vector<ge::NodePtr> &fusion_nodes) {
  // deal quant node; if quant is single output&reference, delete quant
  // if quant node single output, muti reference, add antiquant node
  if (quant_node->GetOutAllNodes().size() == 1) {
    if (ChangeQuantNodeEdge(graph, cube_node, quant_node) != SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][PoolQntPcsFus][QntRolbk] Delete quant node [%s] failed.",
                      cube_node->GetName().c_str());
      return FAILED;
    }
  } else {
    // add antiquant node
    if (AddAntiQuantNode(graph, cube_node, quant_node, fusion_nodes) != SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][PoolQntPcsFus][QntRolbk] Add AntiQuant node before node [%s] failed.",
                      cube_node->GetName().c_str());
      return FAILED;
    }
  }

  // deal dequant node: remove deqaunt node
  if (ChangeDequantNodeEdge(graph, cube_node, dequant_ndoe) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][PoolQntPcsFus][QntRolbk] Delete dequant node [%s] failed.",
                    cube_node->GetName().c_str());
    return FAILED;
  }

  // refresh pooling node input and output data type
  if (SetDataTypeOfNodes(cube_node) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][PoolQntPcsFus][QntRolbk] Refresh AvgPool node [%s] data type failed.",
                    cube_node->GetName().c_str());
    return FAILED;
  }
  return SUCCESS;
}

Status PoolingQuantProcessFusionPass::AddAntiQuantNode(ge::ComputeGraph &graph, ge::NodePtr &cube_node,
                                                       ge::NodePtr &quant_node,
                                                       vector<ge::NodePtr> &fusion_nodes) const {
  ge::OpDescPtr anti_quant = ge::AttrUtils::CopyOpDesc(quant_node->GetOpDesc());
  auto quant_data_type = quant_node->GetOpDesc()->GetOutputDesc(0).GetDataType();
  anti_quant->SetName(cube_node->GetName() + "_anti_quant_layer");
  anti_quant->SetType("AscendAntiQuant");
  // update input and output tensor
  anti_quant->MutableInputDesc(0)->SetDataType(quant_data_type);
  anti_quant->MutableInputDesc(0)->SetOriginDataType(quant_data_type);
  anti_quant->MutableOutputDesc(0)->SetDataType(ge::DT_FLOAT);
  anti_quant->MutableOutputDesc(0)->SetOriginDataType(ge::DT_FLOAT);

  // update attr value
  float scale = 1;
  float offset = 0;
  (void)ge::AttrUtils::GetFloat(anti_quant, ATTR_SCALE, scale);
  (void)ge::AttrUtils::GetFloat(anti_quant, ATTR_OFFSET, offset);
  (void)ge::AttrUtils::SetFloat(anti_quant, ATTR_SCALE, 1.0 / scale);
  (void)ge::AttrUtils::SetFloat(anti_quant, ATTR_OFFSET, -offset);
  ge::NodePtr anti_quant_node = graph.AddNode(anti_quant);
  FE_CHECK_NOTNULL(anti_quant_node);
  fusion_nodes.push_back(anti_quant_node);

  // link edge of antiquant node
  ge::OutDataAnchorPtr quant_out_anchor = quant_node->GetOutDataAnchor(0);
  ge::InDataAnchorPtr avg_pool_in_anchor = cube_node->GetInDataAnchor(0);
  if (ge::GraphUtils::RemoveEdge(quant_out_anchor, avg_pool_in_anchor) != ge::GRAPH_SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][PoolQntPcsFus][AddAntiQntNd] Remove edge between [%s] with [%s] failed.",
                    quant_node->GetName().c_str(), cube_node->GetName().c_str());
    return FAILED;
  }
  (void)ge::GraphUtils::AddEdge(quant_out_anchor, anti_quant_node->GetInDataAnchor(0));
  (void)ge::GraphUtils::AddEdge(anti_quant_node->GetOutDataAnchor(0), avg_pool_in_anchor);
  return SUCCESS;
}

Status PoolingQuantProcessFusionPass::ChangeDequantNodeEdge(ge::ComputeGraph &graph, ge::NodePtr &cube_node,
                                                            ge::NodePtr &dequant_node) {
  std::vector<ge::OutDataAnchorPtr> peer_out_anchors_of_node;
  string cube_name = cube_node->GetName();
  ge::NodePtr deq_scale = dequant_node->GetInDataNodes().at(1);
  FE_CHECK_NOTNULL(deq_scale);
  if (RemoveInputEdgeAndSingleConstInput(dequant_node, peer_out_anchors_of_node) == FAILED) {
    REPORT_FE_ERROR("[GraphOpt][PoolQntPcsFus][ChgDeqntNdEg] link output edge Failed.");
    return FAILED;
  }
  if (LinkOutputEdgeWithoutControl(dequant_node, cube_node, cube_name, peer_out_anchors_of_node) == FAILED) {
    REPORT_FE_ERROR("[GraphOpt][PoolQntPcsFus][ChgDeqntNdEg] link output edge Failed.");
    return FAILED;
  }

  if (graph.RemoveNode(dequant_node) == ge::GRAPH_FAILED) {
    REPORT_FE_ERROR("[GraphOpt][PoolQntPcsFus][ChgDeqntNdEg] dequant node remove failed");
    return FAILED;
  }

  if (deq_scale->GetOutNodes().size() == 0) {
    if (graph.RemoveNode(deq_scale) == ge::GRAPH_FAILED) {
      REPORT_FE_ERROR("[GraphOpt][PoolQntPcsFus][ChgDeqntNdEg] deq scale node remove failed");
      return FAILED;
    }
  }
  return SUCCESS;
}
}  // namespace fe
