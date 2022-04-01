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

#include "avgpool_quant_process_fusion_pass.h"
#include "bias_optimize_quant_rollback_base.h"
#include "common/math_util.h"

namespace fe {
/*
 *  fusion pattern
 *
 * AscendQuant ---> AvgPool--->AscendDequant--->output
 *                                  ^
 *                                 /
 *                                /
 *                           deq_scale
 *
 */
// kernel_h * kernel_w
const int64_t kAvgKernelSizeHMulW = 255;
// Kiseze restrictions
const int64_t kAvgKernelSize = 20;
// strides and ksize limits
const int64_t kStridesKsizeLimits1 = 3;
const int64_t kStridesKsizeLimits2 = 4;

const int64_t kAvgPoolQuantRollbackStridesThd = 63;
const string kAvgPool = "AvgPool";
const string kAvgPoolV2 = "AvgPoolV2";

vector<FusionPattern *> AvgPoolQuantProcessFusionPass::DefinePatterns() {
  vector<FusionPattern *> patterns;
  FE_LOGD("Start to define AvgPool quant process fusion pattern.");
  FusionPattern *pattern = new (std::nothrow) FusionPattern("AvgPoolQuantProcessFusion");
  FE_CHECK(pattern == nullptr, FE_LOGW("new FusionPattern object failed!"), return patterns);
  pattern->AddOpDesc(PATTERN_QUANT, {QUANT})
      .AddOpDesc(PATTERN_CUBE, {kAvgPool, kAvgPoolV2})
      .AddOpDesc(PATTERN_DEQUANT, {DEQUANT})
      .SetInputs(PATTERN_CUBE, {PATTERN_QUANT})
      .SetInputs(PATTERN_DEQUANT, {PATTERN_CUBE})
      .SetOutput(PATTERN_DEQUANT);
  patterns.push_back(pattern);

  return patterns;
}

Status AvgPoolQuantProcessFusionPass::SetDataTypeOfPooling(const ge::NodePtr &pooling_node) {
  ge::DataType set_target_dtype = ge::DT_INT32;

  ge::OpDescPtr pooling_op = pooling_node->GetOpDesc();
  for (size_t i = 0; i < pooling_node->GetAllOutDataAnchors().size(); ++i) {
    ge::GeTensorDesc output_desc = pooling_op->GetOutputDesc(i);
    output_desc.SetDataType(set_target_dtype);
    output_desc.SetOriginDataType(set_target_dtype);
    if (pooling_op->UpdateOutputDesc(i, output_desc) != ge::GRAPH_SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][SetDTypeofPool] update output desc of Node[%s] not success.",
                      pooling_op->GetName().c_str());
      return FAILED;
    }
    auto out_data_anchor = pooling_node->GetOutDataAnchor(i);
    FE_CHECK(out_data_anchor == nullptr,
             REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][SetDTypeofPool] outDataAnchor is null"), return FAILED);
    auto after_in_data_anchor = out_data_anchor->GetPeerInDataAnchors().at(i);
    FE_CHECK(after_in_data_anchor == nullptr,
             REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][SetDTypeofPool] afterInDataAnchor is null"), return FAILED);
    /* Set the output desc data type as int32 for the output of host cpu op */
    ge::NodePtr output_node = after_in_data_anchor->GetOwnerNode();
    FE_CHECK(output_node == nullptr,
             REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][SetDTypeofPool] outputNode is null"), return FAILED);

    ge::OpDescPtr op = output_node->GetOpDesc();
    FE_CHECK(op == nullptr, REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][SetDTypeofPool] op is null"), return FAILED);
    ge::GeTensorDesc input_desc = op->GetInputDesc(after_in_data_anchor->GetIdx());
    input_desc.SetDataType(set_target_dtype);
    input_desc.SetOriginDataType(set_target_dtype);
    if (op->UpdateInputDesc(after_in_data_anchor->GetIdx(), input_desc) != ge::GRAPH_SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][SetDTypeofPool] update input desc of [%s] not success.",
                      op->GetName().c_str());
      return FAILED;
    }
  }
  return SUCCESS;
}

bool AvgPoolQuantProcessFusionPass::SetQuantProcessModeFromStridesKsizeDataformat(QuantProcessMode &quant_process_mode,
                                                                                  const ge::NodePtr &cube_node) const {
  // get stride
  vector<int64_t> strides;
  (void)ge::AttrUtils::GetListInt(cube_node->GetOpDesc(), "strides", strides);
  // get ksize
  vector<int64_t> ksize;
  (void)ge::AttrUtils::GetListInt(cube_node->GetOpDesc(), "ksize", ksize);
  // get data_format
  string data_format;
  (void)ge::AttrUtils::GetStr(cube_node->GetOpDesc(), "data_format", data_format);

  int64_t ksizeH = 0;
  int64_t ksizeW = 0;
  int64_t stridesH = 0;
  int64_t stridesW = 0;
  if (data_format == "NHWC") {
    if (strides.size() < kStridesKsizeLimits1 || ksize.size() < kStridesKsizeLimits1) {
      return false;
    }
    stridesH = strides[1];
    stridesW = strides[2];
    ksizeH = ksize[1];
    ksizeW = ksize[2];
  } else if (data_format == "NCHW") {
    if (strides.size() < kStridesKsizeLimits2 || ksize.size() < kStridesKsizeLimits2) {
      return false;
    }
    stridesH = strides[2];
    stridesW = strides[3];
    ksizeH = ksize[2];
    ksizeW = ksize[3];
  } else {
    FE_LOGI("data_format is not nchw or nhwc,is %s,QUANT_ROLLBACK", data_format.c_str());
    quant_process_mode = QuantProcessMode::QUANT_ROLLBACK;
    return true;
  }

  if (stridesH > 63 || stridesW > 63) {
    FE_LOGI("stridesH > 63 || stridesW > 63 stridesH is %ld,stridesW is %ld", stridesH, stridesW);
    quant_process_mode = QuantProcessMode::QUANT_ROLLBACK;
    return true;
  }

  bool AicoreSupport = true;
  FE_INT64_MULCHECK(ksizeH, ksizeW);
  AicoreSupport = (ksizeH * ksizeW <= kAvgKernelSizeHMulW) || (ksizeH < kAvgKernelSize && ksizeW < kAvgKernelSize);
  if (!AicoreSupport) {
    FE_LOGI("aicore is not support,QUANT_ROLLBACK");
    quant_process_mode = QuantProcessMode::QUANT_ROLLBACK;
    return true;
  }
  if (cube_node->GetType() == kAvgPoolV2 &&
      CheckAvgPoolV2QuantRollback(cube_node, ksizeH, ksizeW, stridesH, stridesW)) {
    quant_process_mode = QuantProcessMode::QUANT_ROLLBACK;
    return true;
  }
  return false;
}

bool AvgPoolQuantProcessFusionPass::CheckAvgPoolV2QuantRollback(const ge::NodePtr &cube_node, const int64_t &sizeH,
                                                                const int64_t &sizeW, const int64_t &stridesH,
                                                                const int64_t &stridesW) const {
  bool global_pooling = false;
  (void)ge::AttrUtils::GetBool(cube_node->GetOpDesc(), "global_pooling", global_pooling);
  if (global_pooling) {
    return true;
  }
  ge::GeTensorDescPtr input_desc = cube_node->GetOpDesc()->MutableInputDesc(0);
  int32_t inputHIdx = GetAxisIndexByFormat(static_cast<ge::Format>(ge::GetPrimaryFormat(input_desc->GetFormat())), "H");
  int32_t inputWIdx = GetAxisIndexByFormat(static_cast<ge::Format>(ge::GetPrimaryFormat(input_desc->GetFormat())), "W");
  const auto &input_shape = input_desc->GetShape();
  if (inputHIdx >= static_cast<int32_t>(input_shape.GetDimNum()) ||
      inputWIdx >= static_cast<int32_t>(input_shape.GetDimNum())) {
    return false;
  }
  int64_t inputH = input_shape.GetDim(static_cast<size_t>(inputHIdx));
  int64_t inputW = input_shape.GetDim(static_cast<size_t>(inputWIdx));
  std::vector<int64_t> pads;
  (void)ge::AttrUtils::GetListInt(cube_node->GetOpDesc(), "pads", pads);
  std::string padding_mode;
  (void)ge::AttrUtils::GetStr(cube_node->GetOpDesc(), "padding_mode", padding_mode);
  if (inputH == sizeH && inputW == sizeW) {
    for (const int64_t &pad : pads) {
      if (pad != 0) {
        return false;
      }
    }
    if (padding_mode != "SAME") {
      return true;
    } else if (inputH == stridesH && inputW == stridesW) {
      return true;
    }
  }
  return false;
}

bool AvgPoolQuantProcessFusionPass::SetQuantProcessModeForUnknownShape(QuantProcessMode &quant_process_mode,
                                                                       const ge::NodePtr &cube_node) const {
  vector<int64_t> strides;
  (void)ge::AttrUtils::GetListInt(cube_node->GetOpDesc(), "strides", strides);
  string data_format;
  (void)ge::AttrUtils::GetStr(cube_node->GetOpDesc(), "data_format", data_format);

  int64_t stridesH = 0;
  int64_t stridesW = 0;
  if (data_format == "NHWC") {
    if (strides.size() < kStridesKsizeLimits1) {
      FE_LOGI("Strides size is %zu", strides.size());
      return false;
    }
    stridesH = strides[1];
    stridesW = strides[2];
  } else if (data_format == "NCHW") {
    if (strides.size() < kStridesKsizeLimits2) {
      FE_LOGI("Strides size is %zu", strides.size());
      return false;
    }
    stridesH = strides[2];
    stridesW = strides[3];
  } else {
    FE_LOGI("Data format: %s is not NHWC or NCHW", data_format.c_str());
    return false;
  }

  if (stridesH > kAvgPoolQuantRollbackStridesThd || stridesW > kAvgPoolQuantRollbackStridesThd) {
    FE_LOGI("Quant need rollback because stridesH: %ld or stridesW: %ld is over 63", stridesH, stridesW);
    quant_process_mode = QuantProcessMode::QUANT_ROLLBACK;
    return true;
  }
  return false;
}

Status AvgPoolQuantProcessFusionPass::GetQuantProcessMode(ge::NodePtr &quant_node, ge::NodePtr &cube_node,
                                                          QuantProcessMode &quant_process_mode) {
  int32_t index_ci;
  ge::OpDescPtr op_desc_ptr = quant_node->GetOpDesc();
  auto input_shape = op_desc_ptr->MutableInputDesc(0)->MutableShape();
  ge::Format input_format = static_cast<ge::Format>(ge::GetPrimaryFormat(op_desc_ptr->GetInputDescPtr(0)->GetFormat()));
  index_ci = GetAxisIndexByFormat(input_format, "C");
  if (index_ci < 0) {
    REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][GetQntPcsMode] Can not get C index of format [%s]",
                    ge::TypeUtils::FormatToSerialString(input_format).c_str());
    return FAILED;
  }
  if (input_shape.IsUnknownShape()) {
    if (SetQuantProcessModeForUnknownShape(quant_process_mode, cube_node)) {
      return SUCCESS;
    }
  }
  // JudgeUnknownShape
  if (IsUnknownShapeValue(input_shape.GetDim(index_ci))) {
    FE_LOGW("Bias optimize cannot be applied for unknown shape.");
    quant_process_mode = QuantProcessMode::QUANT_UNDIFINED;
    return SUCCESS;
  }

  // judge whether need quant rollback
  // avgpool output shape must be 4-D
  ge::GeTensorDescPtr ge_tensor_desc_ptr = cube_node->GetOpDesc()->MutableOutputDesc(0);
  if (ge_tensor_desc_ptr->MutableShape().GetDimNum() != NCHW_DIMENSION_NUM) {
    REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][SetDTypeofPool] AvgPool node [%s] output shape is not 4-D.",
                    cube_node->GetName().c_str());
    return FAILED;
  }
  // if out_w == 1, do rollback
  int32_t out_w_index =
      GetAxisIndexByFormat(static_cast<ge::Format>(ge::GetPrimaryFormat(ge_tensor_desc_ptr->GetFormat())), "W");
  if (out_w_index < 0) {
    REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][GetQntPcsMode]  Can not get W index of format[%s] ",
                    ge::TypeUtils::FormatToSerialString(ge_tensor_desc_ptr->GetFormat()).c_str());
    return FAILED;
  }
  if (ge_tensor_desc_ptr->MutableShape().GetDim(out_w_index) == OUT_W_DIM_VALUE) {
    FE_LOGI("output_w is 1, QUANT_ROLLBACK");
    quant_process_mode = QuantProcessMode::QUANT_ROLLBACK;
    return SUCCESS;
  }
  if (SetQuantProcessModeFromStridesKsizeDataformat(quant_process_mode, cube_node)) {
    return SUCCESS;
  }
  quant_process_mode = QuantProcessMode::BIAS_OPTIMIZE;
  return SUCCESS;
}

/*
 * if avgpool node attr [padding] is [SAME], the output of avgpool need to be modified.
 * so we can not fusion dequant node to requant node, we set attr ATTR_DEQUANT_NO_REQUANT here
 * when we do requant fusion pass, need to judge attr: ATTR_DEQUANT_NO_REQUANT
 */
Status AvgPoolQuantProcessFusionPass::JudgePadAttr(const ge::NodePtr &cube_node,
                                                   const ge::NodePtr &dequant_node) const {
  std::string padding;
  std::string padding_attr = "padding";
  bool is_avgpoolv2 = cube_node->GetType() == kAvgPoolV2;
  if (is_avgpoolv2) {
    padding_attr = "padding_mode";
  }
  (void)ge::AttrUtils::GetStr(cube_node->GetOpDesc(), padding_attr, padding);
  bool padding_is_same = (padding == "SAME");
  bool padding_is_calc = (padding == "CALCULATED");
  bool no_requant = (padding_is_same) || (is_avgpoolv2 && padding_is_calc);
  if (no_requant) {
    (void)ge::AttrUtils::SetBool(dequant_node->GetOpDesc(), ATTR_DEQUANT_NO_REQUANT, true);
  }
  return SUCCESS;
}

Status AvgPoolQuantProcessFusionPass::BiasOptimize(ge::ComputeGraph &graph, ge::NodePtr &cube_node,
                                                   ge::NodePtr &dequant_node, ge::NodePtr &quant_node,
                                                   vector<ge::NodePtr> &fusion_nodes) {
  if (JudgeDeqscaleShape(dequant_node) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][BiasOpti] Judge Node[%s] deq_scale failed.",
                    dequant_node->GetName().c_str());
    return FAILED;
  }
  if (SetQuantParameters(cube_node, quant_node) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][BiasOpti] Set quant paras to cube node[%s] failed.",
                    cube_node->GetName().c_str());
    return FAILED;
  }
  if (SetDataTypeOfPooling(cube_node) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][BiasOpti] Set data type Of Pooling[%s] failed.",
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
  (void)JudgePadAttr(cube_node, dequant_node);
  return SUCCESS;
}

Status AvgPoolQuantProcessFusionPass::QuantRollback(ge::ComputeGraph &graph, ge::NodePtr &cube_node,
                                                    ge::NodePtr &dequant_ndoe, ge::NodePtr &quant_node,
                                                    vector<ge::NodePtr> &fusion_nodes) {
  // deal quant node: if quant is single reference, delete it; if not, change edge
  if (ChangeQuantNodeEdge(graph, cube_node, quant_node) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][QntRollback] Delete quant node [%s] failed.",
                    cube_node->GetName().c_str());
    return FAILED;
  }

  // deal dequant node: remove deqaunt node
  if (ChangeDequantNodeEdge(graph, cube_node, dequant_ndoe) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][QntRollback] Delete dequant node [%s] failed.",
                    cube_node->GetName().c_str());
    return FAILED;
  }

  // refresh avgpool node input and output data type
  if (SetDataTypeOfNodes(cube_node) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][QntRollback] Refresh AvgPool node [%s] data type failed.",
                    cube_node->GetName().c_str());
    return FAILED;
  }
  return SUCCESS;
}

Status AvgPoolQuantProcessFusionPass::ChangeDequantNodeEdge(ge::ComputeGraph &graph, ge::NodePtr &cube_node,
                                                            ge::NodePtr &dequant_node) {
  std::vector<ge::OutDataAnchorPtr> peer_out_anchors_of_node;
  string cube_name = cube_node->GetName();
  ge::NodePtr deq_scale = dequant_node->GetInDataNodes().at(1);
  FE_CHECK_NOTNULL(deq_scale);

  if (RemoveInputEdgeAndSingleConstInput(dequant_node, peer_out_anchors_of_node) == FAILED) {
    REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][ChgDeqntNdEg] link output edge Failed.");
    return FAILED;
  }

  if (LinkOutputEdgeWithoutControl(dequant_node, cube_node, cube_name, peer_out_anchors_of_node) == FAILED) {
    REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][ChgDeqntNdEg] link output edge Failed.");
    return FAILED;
  }

  if (graph.RemoveNode(dequant_node) == ge::GRAPH_FAILED) {
    REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][ChgDeqntNdEg] dequant node remove failed");
    return FAILED;
  }

  if (deq_scale->GetOutNodes().empty()) {
    if (graph.RemoveNode(deq_scale) == ge::GRAPH_FAILED) {
      REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][ChgDeqntNdEg] deq scale node remove failed");
      return FAILED;
    }
  }
  return SUCCESS;
}

}  // namespace fe
