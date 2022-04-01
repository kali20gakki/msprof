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

#include "fc_quant_process_fusion_pass.h"
#include "bias_optimize_quant_rollback_base.h"

namespace fe {
static const string FC = "FullyConnection";
static string FC_BIAS_NAME = "b";

/*
 *  fusion pattern
 *           input
 *               \
 *             AscendQuant
 *                \
 *                 v
 *     weights--->FC--->AscendDequant--->output
 *                ^          ^
 *               /          /
 *              /          /
 *           bias      deq_scale
 */
vector<FusionPattern *> FCQuantProcessFusionPass::DefinePatterns() {
  vector<FusionPattern *> patterns;
  FE_LOGD("Start to define fully connection quant process fusion pattern.");
  FusionPattern *pattern = new (std::nothrow) FusionPattern("FCQuantProcessFusion");
  FE_CHECK(pattern == nullptr, FE_LOGW("new FusionPattern object failed!"), return patterns);
  pattern->AddOpDesc(PATTERN_QUANT, {QUANT})
      .AddOpDesc(PATTERN_CUBE, {FC})
      .AddOpDesc(PATTERN_DEQUANT, {DEQUANT})
      .SetInputs(PATTERN_CUBE, {PATTERN_QUANT})
      .SetInputs(PATTERN_DEQUANT, {PATTERN_CUBE})
      .SetOutput(PATTERN_DEQUANT);
  patterns.push_back(pattern);

  return patterns;
}

Status FCQuantProcessFusionPass::GetCoValue(ge::NodePtr &cube_node, int64_t &co) {
  std::vector<int64_t> filter_dims4_d;
  ge::Format filter_format =
      static_cast<ge::Format>(ge::GetPrimaryFormat(cube_node->GetOpDesc()->GetInputDesc(1).GetFormat()));
  auto filter_shape = cube_node->GetOpDesc()->MutableInputDesc(1)->MutableShape();
  if (filter_format != ge::FORMAT_ND) {
    (void)PadShapeTo4Dim(filter_format, filter_shape.GetDims(), filter_dims4_d);
    if (filter_dims4_d.empty()) {
      REPORT_FE_ERROR("[GraphOpt][FCQntPcsFus][GetCoVal] Node[%s] filter_dims4_d is empty.",
                      cube_node->GetName().c_str());
      return FAILED;
    }
    int64_t index_co = GetAxisIndexByFormat(filter_format, "N");
    if (index_co < 0) {
      REPORT_FE_ERROR("[GraphOpt][FCQntPcsFus][GetCoVal] Node[%s] index_co is negative, Check filter_format.",
                      cube_node->GetName().c_str());
      return FAILED;
    }
    if (index_co >= static_cast<int64_t>(filter_dims4_d.size())) {
      REPORT_FE_ERROR("[GraphOpt][FCQntPcsFus][GetCoVal] Node[%s] index_co is larger than the size of filter dims.",
                      cube_node->GetName().c_str());
      return FAILED;
    }
    co = filter_dims4_d[index_co];
  }
  return SUCCESS;
}

Status FCQuantProcessFusionPass::BiasOptimize(ge::ComputeGraph &graph, ge::NodePtr &cube_node,
                                              ge::NodePtr &dequant_node, ge::NodePtr &quant_node,
                                              vector<ge::NodePtr> &fusion_nodes) {
  if (JudgeDeqscaleShape(dequant_node) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][FCQntPcsFus][BiasOpti] Judge Node[%s] deq_scale failed.",
                    dequant_node->GetName().c_str());
    return FAILED;
  }
  if (SetQuantParameters(cube_node, quant_node) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][FCQntPcsFus][BiasOpti] Set quant paras to cube node[%s] failed.",
                    cube_node->GetName().c_str());
    return FAILED;
  }

  bool bias_term = true;
  ge::OpDescPtr conv_op = cube_node->GetOpDesc();
  (void)ge::AttrUtils::GetBool(conv_op, "bias_term", bias_term);
  struct PatternNodes pattern_nodes = {cube_node, dequant_node, quant_node, nullptr, nullptr};
  if (GetWeightNodesOfCubeNode(cube_node, pattern_nodes.weight_const_node,
                               pattern_nodes.ascend_weight_quant_node) != SUCCESS) {
    REPORT_FE_ERROR("get weight_const node and ascend_weight_quant node of cube node[%s] failed!",
                    cube_node->GetName().c_str());
    return FAILED;
  }

  if (IsCubeNeedBiasInput(cube_node) && bias_term) {
    int64_t co = 1;
    if (GetCoValue(cube_node, co) != SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][FCQntPcsFus][BiasOpti] Get node[%s] co value.", cube_node->GetName().c_str());
      return FAILED;
    }

    if (SetBiasName(FC_BIAS_NAME) != SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][FCQntPcsFus][BiasOpti] SetBiasName for cube node[%s] failed.",
                      cube_node->GetName().c_str());
      return FAILED;
    }

    if (CreateBiasInput(graph, cube_node, co) != SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][FCQntPcsFus][BiasOpti] Create cube node[%s] bias input failed.",
                      cube_node->GetName().c_str());
      return FAILED;
    }
  }

  if (bias_term) {
    Status ret =
        CreateNewHostCpuOp(QUANT_BIAS_OPTIMIZATION, pattern_nodes, graph,
                           static_cast<uint32_t>(HostOpMode::BIAS_OPTIMIZATION_MODE), fusion_nodes);
    if (ret != SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][FCQntPcsFus][BiasOpti] Create host op failed.");
      return FAILED;
    }
  }
  return SUCCESS;
}

}  // namespace fe