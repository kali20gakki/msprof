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

#include "conv2d_quant_process_fusion_pass.h"
#include "bias_optimize_quant_rollback_base.h"

namespace fe {
/*
 *  fusion pattern
 *           input
 *               \
 *             AscendQuant
 *                \
 *                 v
 *     weights--->Conv2D--->AscendDequant--->output
 *                ^          ^
 *               /          /
 *              /          /
 *           bias      deq_scale
 *
 *  fusion pattern1
 *           input
 *               \
 *             AscendQuant
 *                \
 *                Pad
 *                  \
 *                  v
 *     weights--->Conv2D--->AscendDequant--->output
 *                 /           ^
 *                /           /
 *               /           /
 *             bias       deq_scale
 */
vector<FusionPattern *> Conv2DQuantProcessFusionPass::DefinePatterns() {
  vector<FusionPattern *> patterns;
  FE_LOGD("Start to define Conv2D quant process fusion pattern.");
  FusionPattern *pattern = new (std::nothrow) FusionPattern("Conv2DQuantProcessFusion");
  FE_CHECK(pattern == nullptr, FE_LOGW("new FusionPattern object failed!"), return patterns);
  pattern->AddOpDesc(PATTERN_QUANT, {QUANT})
      .AddOpDesc(PATTERN_CUBE, {CONV2D})
      .AddOpDesc(PATTERN_DEQUANT, {DEQUANT})
      .SetInputs(PATTERN_CUBE, {PATTERN_QUANT})
      .SetInputs(PATTERN_DEQUANT, {PATTERN_CUBE})
      .SetOutput(PATTERN_DEQUANT);
  patterns.push_back(pattern);

  FusionPattern *pattern1 = new (std::nothrow) FusionPattern("Conv2DQuantProcessFusion1");
  FE_CHECK(pattern1 == nullptr, FE_LOGW("new FusionPattern object failed!"), return patterns);
  pattern1->AddOpDesc(PATTERN_QUANT, {QUANT})
      .AddOpDesc(PATTERN_PAD, {PAD})
      .AddOpDesc(PATTERN_CUBE, {CONV2D})
      .AddOpDesc(PATTERN_DEQUANT, {DEQUANT})
      .SetInputs(PATTERN_PAD, {PATTERN_QUANT})
      .SetInputs(PATTERN_CUBE, {PATTERN_PAD})
      .SetInputs(PATTERN_DEQUANT, {PATTERN_CUBE})
      .SetOutput(PATTERN_DEQUANT);
  patterns.push_back(pattern1);

  return patterns;
}

Status Conv2DQuantProcessFusionPass::GetCoValue(ge::NodePtr &cube_node, int64_t &co) {
  std::vector<int64_t> filter_dims4_d;
  ge::Format filter_format = static_cast<ge::Format>(ge::GetPrimaryFormat(
      cube_node->GetOpDesc()->GetInputDesc(1).GetFormat()));
  auto filter_shape = cube_node->GetOpDesc()->MutableInputDesc(1)->MutableShape();
  if (filter_format != ge::FORMAT_ND) {
    (void)PadShapeTo4Dim(filter_format, filter_shape.GetDims(), filter_dims4_d);
    if (filter_dims4_d.empty()) {
      REPORT_FE_ERROR("[GraphOpt][Conv2DQntPcsFus][GetCoVal] Node[%s] filter_dims4_d is empty.",
                      cube_node->GetName().c_str());
      return FAILED;
    }
    int64_t index_co = GetAxisIndexByFormat(filter_format, "N");
    if (index_co < 0) {
      REPORT_FE_ERROR("[GraphOpt][Conv2DQntPcsFus][GetCoVal] Node[%s] index_co is negative, Check filter_format.",
                      cube_node->GetName().c_str());
      return FAILED;
    }
    if (index_co >= static_cast<int64_t>(filter_dims4_d.size())) {
      REPORT_FE_ERROR("[GraphOpt][Conv2DQntPcsFus][GetCoVal] Node[%s] index_co is larger than the size of filter dims.",
                      cube_node->GetName().c_str());
      return FAILED;
    }
    co = filter_dims4_d[index_co];
  }
  return SUCCESS;
}

/*
 * Conv2D may have Pad node before
 * so we need to refresh Pad node dtype
 */
Status Conv2DQuantProcessFusionPass::SetPadNodeDataType(const ge::NodePtr &pad_node) const {
  ge::OpDescPtr pad_op_desc = pad_node->GetOpDesc();
  ge::GeTensorDesc pad_input_desc = pad_op_desc->GetInputDesc(0);
  ge::GeTensorDesc pad_output_desc = pad_op_desc->GetOutputDesc(0);
  pad_input_desc.SetDataType(ge::DT_FLOAT16);
  pad_input_desc.SetOriginDataType(ge::DT_FLOAT16);
  pad_output_desc.SetDataType(ge::DT_FLOAT16);
  pad_output_desc.SetOriginDataType(ge::DT_FLOAT16);
  if (pad_op_desc->UpdateInputDesc(0, pad_input_desc) != ge::GRAPH_SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][Conv2DQntPcsFus][SetPadNdDtype] Fail to update input desc of Node[%s].",
                    pad_op_desc->GetName().c_str());
    return FAILED;
  }
  if (pad_op_desc->UpdateOutputDesc(0, pad_output_desc) != ge::GRAPH_SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][Conv2DQntPcsFus][SetPadNdDtype] Fail to update output desc of Node[%s].",
                    pad_op_desc->GetName().c_str());
    return FAILED;
  }
  FE_LOGD("Data type of Pad[%s] has been updated to float during quant rollback", pad_op_desc->GetName().c_str());
  return SUCCESS;
}

Status Conv2DQuantProcessFusionPass::SetDataTypeOfNodes(const ge::NodePtr &cube_node) {
  ge::DataType data_type = ge::DT_FLOAT;
  ge::DataType target_data_type = ge::DT_FLOAT16;

  if (SetCubeNodeDataType(cube_node, data_type, target_data_type) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][Conv2DQntPcsFus][SetDtype] Set cube node[%s] data type failed.",
                    cube_node->GetName().c_str());
    return FAILED;
  }
  FE_LOGD("Set cube node[%s] data type success.", cube_node->GetName().c_str());

  ge::NodePtr cube_input_node = cube_node->GetInAllNodes().at(0);
  if (cube_input_node->GetType() == PAD) {
    if (SetPadNodeDataType(cube_input_node) != SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][Conv2DQntPcsFus][SetDtype] Set node[%s] data type failed.",
                      cube_input_node->GetName().c_str());
      return FAILED;
    }
    FE_LOGD("Set pad node[%s] data type success.", cube_input_node->GetName().c_str());
  }
  return SUCCESS;
}

ge::NodePtr Conv2DQuantProcessFusionPass::GetCubeNodeInputNode(const ge::NodePtr &cube_node) const {
  ge::NodePtr node_after_quant = cube_node;
  ge::NodePtr cube_input_node = cube_node->GetInAllNodes().at(0);
  if (cube_input_node->GetType() == PAD) {
    node_after_quant = cube_input_node;
  }
  return node_after_quant;
}
}  // namespace fe
