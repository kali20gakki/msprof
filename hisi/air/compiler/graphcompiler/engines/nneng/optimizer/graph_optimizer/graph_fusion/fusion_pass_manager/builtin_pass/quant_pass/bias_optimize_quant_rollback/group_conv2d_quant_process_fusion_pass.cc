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

#include "group_conv2d_quant_process_fusion_pass.h"
#include "bias_optimize_quant_rollback_base.h"

namespace fe {
static const string OP_CONV = "Conv2D";
static const string OP_QUANT = "AscendQuant";
static const string OP_DEQUANT = "AscendDequant";
static const string OP_SPLIT = "Split";
static const string OP_CONCATV2 = "ConcatV2";
static const string OP_CONCAT = "Concat";

static const string PATN_CONV2D = "conv2d";
static const string PATN_QUANT = "quant";
static const string PATN_DEQUANT = "dequant";
static const string PATN_SPLIT = "split";
static const string PATN_CONCATV2 = "concatv2";

/*
 * fusion pattern
 *              input
 *                |
 *           AscendQuant
 *                |
 *        _____ split ______
 *       /        |         \
 *    Conv2D    Conv2D    Conv2D
 *        \       |         /
 *          ConcatV2/Concat
 *                |
 *          AscendDequant
 *                |
 *
 */
vector<FusionPattern *> GroupConv2DQuantProcessFusionPass::DefinePatterns() {
  vector<FusionPattern *> patterns;
  FE_LOGD("Start to define group conv2d quant process fusion pattern.");
  FusionPattern *pattern = new (std::nothrow) FusionPattern("GroupConv2DQuantProcessFusion");
  FE_CHECK(pattern == nullptr, FE_LOGW("new FusionPattern object failed!"), return patterns);
  pattern->AddOpDesc(PATN_QUANT, {OP_QUANT})
      .AddOpDesc(PATN_SPLIT, {OP_SPLIT})
      .AddOpDesc(PATN_CONV2D, {OP_CONV})
      .AddOpDesc(PATN_CONCATV2, {OP_CONCATV2, OP_CONCAT})
      .AddOpDesc(PATN_DEQUANT, {OP_DEQUANT})
      .SetInputs(PATN_SPLIT, {PATN_QUANT})
      .SetInputs(PATN_CONV2D, {PATN_SPLIT})
      .SetInputs(PATN_CONCATV2, {PATN_CONV2D})
      .SetInputs(PATN_DEQUANT, {PATN_CONCATV2})
      .SetOutput(PATN_DEQUANT);
  patterns.push_back(pattern);

  return patterns;
}

Status GroupConv2DQuantProcessFusionPass::QuantRollback(ge::ComputeGraph &graph, ge::NodePtr &cube_node,
                                                        ge::NodePtr &dequant_node, ge::NodePtr &quant_node,
                                                        vector<ge::NodePtr> &fusion_nodes) {
  // do quant rollback
  FE_LOGD("Begin to do quant rollback fusion on cube node[name:%s, type:%s].", cube_node->GetName().c_str(),
          cube_node->GetType().c_str());
  // do fuion, quant rollback
  Status ret = DoFusion(graph, cube_node, quant_node, dequant_node, fusion_nodes);
  if (ret != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][GrpConv2dQntPcsFus][QntRolbk] do fusion failed!");
    return ret;
  }
  FE_LOGD("quant rollback fusion success.");

  return SUCCESS;
}

Status GroupConv2DQuantProcessFusionPass::GetCoValue(ge::NodePtr &cube_node, int64_t &co) {
  std::vector<int64_t> filter_dims4_d;
  ge::Format filter_format =
      static_cast<ge::Format>(ge::GetPrimaryFormat(cube_node->GetOpDesc()->GetInputDesc(1).GetFormat()));
  auto filter_shape = cube_node->GetOpDesc()->MutableInputDesc(1)->MutableShape();
  if (filter_format != ge::FORMAT_ND) {
    (void)PadShapeTo4Dim(filter_format, filter_shape.GetDims(), filter_dims4_d);
    if (filter_dims4_d.empty()) {
      REPORT_FE_ERROR("[GraphOpt][GrpConv2dQntPcsFus][GetCoVal] Node[%s] filter_dims4_d is empty.",
                      cube_node->GetName().c_str());
      return FAILED;
    }
    int64_t index_co = GetAxisIndexByFormat(filter_format, "N");
    if (index_co < 0) {
      REPORT_FE_ERROR("[GraphOpt][GrpConv2dQntPcsFus][GetCoVal] Node[%s] index_co is negative, Check filter_format.",
                      cube_node->GetName().c_str());
      return FAILED;
    }
    if (index_co >= static_cast<int64_t>(filter_dims4_d.size())) {
      REPORT_FE_ERROR(
          "[GraphOpt][GrpConv2dQntPcsFus][GetCoVal] Node[%s] index_co is larger than the size of filter dims.",
          cube_node->GetName().c_str());
      return FAILED;
    }
    co = filter_dims4_d[index_co];
  }
  return SUCCESS;
}

Status GroupConv2DQuantProcessFusionPass::Fusion(ge::ComputeGraph &graph, fe::PatternFusionBasePass::Mapping &mapping,
                                                 vector<ge::NodePtr> &fusion_nodes) {
  ge::NodePtr quant_node = GetNodeFromMapping(PATN_QUANT, mapping);
  ge::NodePtr dequant_node = GetNodeFromMapping(PATN_DEQUANT, mapping);
  ge::NodePtr cube_node = GetNodeFromMapping(PATN_CONV2D, mapping);
  ge::NodePtr split_node = GetNodeFromMapping(PATN_SPLIT, mapping);
  ge::NodePtr concat_node = GetNodeFromMapping(PATN_CONCATV2, mapping);

  FE_CHECK(split_node == nullptr,
           REPORT_FE_ERROR("[GraphOpt][GrpConv2dQntPcsFus][Fus] splitNode is null, fusion failed."),
           return PARAM_INVALID);
  FE_CHECK(quant_node == nullptr,
           REPORT_FE_ERROR("[GraphOpt][GrpConv2dQntPcsFus][Fus] quantNode is null, fusion failed."),
           return PARAM_INVALID);
  FE_CHECK(dequant_node == nullptr,
           REPORT_FE_ERROR("[GraphOpt][GrpConv2dQntPcsFus][Fus] dequantNode is null, fusion failed."),
           return PARAM_INVALID);
  FE_CHECK(cube_node == nullptr,
           REPORT_FE_ERROR("[GraphOpt][GrpConv2dQntPcsFus][Fus] cube_node is null, fusion failed."),
           return PARAM_INVALID);

  // if the pattern is not correct, not fusion
  for (ge::NodePtr conv_node : split_node->GetOutAllNodes()) {
    if (conv_node->GetType() != OP_CONV) {
      FE_LOGD("Node[%s] op type is not Conv2D, not fusion.", conv_node->GetName().c_str());
      return NOT_CHANGED;
    }
    if (conv_node->GetOutAllNodes().size() != 1 || (conv_node->GetOutAllNodes().at(0)->GetType() != OP_CONCAT &&
                                                    conv_node->GetOutAllNodes().at(0)->GetType() != OP_CONCATV2)) {
      FE_LOGD("Node[%s] is not single reference or next_node is not Concat or ConcatV2.", conv_node->GetName().c_str());
      return NOT_CHANGED;
    }
  }

  QuantProcessMode quant_process_mode;
  if (GetQuantProcessMode(quant_node, cube_node, quant_process_mode) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][GrpConv2dQntPcsFus][Fus] Get quant process failed.");
    return FAILED;
  }
  if (quant_process_mode == QuantProcessMode::QUANT_UNDIFINED) {
    FE_LOGI("BiasOptimizeQuantRollback Pass.");
    return SUCCESS;
  }

  // bias optimize
  if (quant_process_mode == QuantProcessMode::BIAS_OPTIMIZE) {
    for (ge::NodePtr conv_node : split_node->GetOutAllNodes()) {
      if (BiasOptimize(graph, conv_node, dequant_node, quant_node, fusion_nodes) != SUCCESS) {
        REPORT_FE_ERROR("[GraphOpt][GrpConv2dQntPcsFus][Fus] Node[%s] bias optimize failed.",
                        conv_node->GetName().c_str());
        return FAILED;
      }
      FE_LOGD("Node[%s] bias optimize success.", conv_node->GetName().c_str());
    }
    return SUCCESS;
  }

  // quant rollback
  for (ge::NodePtr conv_node : split_node->GetOutAllNodes()) {
    if (QuantRollback(graph, conv_node, dequant_node, quant_node, fusion_nodes) != SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][GrpConv2dQntPcsFus][Fus] Node[%s] quant rollback failed.",
                      conv_node->GetName().c_str());
      return FAILED;
    }
    FE_LOGD("Node[%s] quant rollback success.", conv_node->GetName().c_str());
  }

  if (ChangeQuantNodeEdge(graph, split_node, quant_node) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][GrpConv2dQntPcsFus][Fus] Split node[%s]: change quant node edge failed.",
                    split_node->GetName().c_str());
    return FAILED;
  }
  FE_LOGD("Split node[%s]: change quant node edge success.", split_node->GetName().c_str());

  if (ChangeDequantNodeEdge(graph, concat_node, dequant_node) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][GrpConv2dQntPcsFus][Fus] Concat node[%s]: change dequant node edge failed.",
                    concat_node->GetName().c_str());
    return FAILED;
  }
  FE_LOGD("Concat node[%s]: change dequant node edge success.", concat_node->GetName().c_str());
  return SUCCESS;
}
}  // namespace fe
