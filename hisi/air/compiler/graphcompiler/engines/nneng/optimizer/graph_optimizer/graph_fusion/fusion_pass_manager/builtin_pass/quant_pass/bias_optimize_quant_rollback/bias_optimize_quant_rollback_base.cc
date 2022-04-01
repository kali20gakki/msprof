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

#include "bias_optimize_quant_rollback_base.h"
#include <sstream>
#include "common/math_util.h"
#include "common/fe_inner_attr_define.h"

namespace fe {

Status BiasOptimizeQuantRollbackBase::GetCoValue(ge::NodePtr &cube_node, int64_t &co) {
  std::vector<int64_t> filter_dims4_d;
  ge::Format filter_format =
      static_cast<ge::Format>(ge::GetPrimaryFormat(cube_node->GetOpDesc()->GetInputDesc(1).GetFormat()));
  auto filter_shape = cube_node->GetOpDesc()->MutableInputDesc(1)->MutableShape();
  if (filter_format != ge::FORMAT_ND) {
    int64_t groups = 1;
    (void)ge::AttrUtils::GetInt(cube_node->GetOpDesc(), "groups", groups);
    (void)PadShapeTo4Dim(filter_format, filter_shape.GetDims(), filter_dims4_d);
    if (filter_dims4_d.empty()) {
      REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][GetCoVal] Node[%s] filter_dims4_d is empty.",
                      cube_node->GetName().c_str());
      return FAILED;
    }
    int64_t index_co = GetAxisIndexByFormat(filter_format, "C");
    if (index_co < 0) {
      REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][GetCoVal] Node[%s] index_co is negative, Check filter_format.",
                      cube_node->GetName().c_str());
      return FAILED;
    }
    if (index_co >= static_cast<int64_t>(filter_dims4_d.size())) {
      REPORT_FE_ERROR(
          "[GraphOpt][AvgPolQntPcsFus][GetCoVal] Node[%s] index_co is larger than the size of filter dims.",
          cube_node->GetName().c_str());
      return FAILED;
    }
    FE_INT64_MULCHECK(filter_dims4_d[index_co], groups);
    co = filter_dims4_d[index_co] * groups;
  }
  return SUCCESS;
}

void BiasOptimizeQuantRollbackBase::SetCinCoutReverse(ge::NodePtr &nodePtr) {
  (void)ge::AttrUtils::SetBool(nodePtr->GetOpDesc(), ATTR_CIN_COUT_REVERSE, false);
}

Status BiasOptimizeQuantRollbackBase::SetBiasName(const std::string &bias_name) {
  bias_name_ = bias_name;
  return SUCCESS;
}

Status BiasOptimizeQuantRollbackBase::GetQuantProcessMode(ge::NodePtr &quant_node, ge::NodePtr &cube_node,
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
  // JudgeUnknownShape
  if (IsUnknownShapeValue(input_shape.GetDim(index_ci))) {
    FE_LOGW("Bias optimize cannot be applied for unknown shape.");
    quant_process_mode = QuantProcessMode::QUANT_UNDIFINED;
    return SUCCESS;
  }

  /*
   * if channel > 16; do bias optimize
   * if channel <= 16; do quant rollback
   */
  if (input_shape.GetDim(index_ci) > MIN_BIAS_OPTIMIZE_CHANNEL) {
    quant_process_mode = QuantProcessMode::BIAS_OPTIMIZE;
  } else {
    quant_process_mode = QuantProcessMode::QUANT_ROLLBACK;
  }
  return SUCCESS;
}

Status BiasOptimizeQuantRollbackBase::RemoveEnter(ge::ComputeGraph &graph, const ge::NodePtr node) const {
  for (ge::InDataAnchorPtr anchor : node->GetAllInDataAnchors()) {
    if (anchor->GetPeerOutAnchor() == nullptr) {
      continue;
    }
    ge::NodePtr pre_node = anchor->GetPeerOutAnchor()->GetOwnerNode();
    if (pre_node->GetType() == ENTER && pre_node->GetOutNodes().size() == 1 && pre_node->GetInNodes().size() == 1) {
      ge::NodePtr const_node = pre_node->GetInDataAnchor(0)->GetPeerOutAnchor()->GetOwnerNode();
      if (const_node->GetType() != CONST) {
        return SUCCESS;
      }
      FE_LOGD("start remove enter node %s", const_node->GetName().c_str());
      if (ge::GraphUtils::RemoveEdge(pre_node->GetOutDataAnchor(0), anchor) != ge::GRAPH_SUCCESS) {
        REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][RmEnter] Fail to remove edge between node[%s] and [%s].",
                        pre_node->GetName().c_str(), node->GetName().c_str());
        return FAILED;
      }
      if (ge::GraphUtils::RemoveEdge(pre_node->GetInDataAnchor(0)->GetPeerOutAnchor(), pre_node->GetInDataAnchor(0)) !=
          ge::GRAPH_SUCCESS) {
        REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][RmEnter] Fail to remove edge between node[%s] and node[%s].",
                        const_node->GetName().c_str(), pre_node->GetName().c_str());
        return FAILED;
      }
      if (ge::GraphUtils::AddEdge(const_node->GetOutDataAnchor(0), anchor) != ge::GRAPH_SUCCESS) {
        REPORT_FE_ERROR(
            "[GraphOpt][AvgPolQntPcsFus][RmEnter] Fail to add edge for the input data anchor of node[%s] .",
            node->GetName().c_str());
        return FAILED;
      }
      if (graph.RemoveNode(pre_node) != ge::GRAPH_SUCCESS) {
        REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][RmEnter] enter node %s remove failed.",
                        pre_node->GetName().c_str());
        return FAILED;
      }
    }
  }
  return SUCCESS;
}

Status BiasOptimizeQuantRollbackBase::JudgeDeqscaleShape(const ge::NodePtr &dequant_node) const {
  // get IsaArchVer info
  const ISAArchVersion isa_arch_ver = Configuration::Instance(AI_CORE_NAME).GetIsaArchVer();
  FE_LOGD("Current ISA Version is [%d].", static_cast<int32_t>(isa_arch_ver));
  // get Dequant deq_scale_tensor(64)
  vector<ge::GeTensorPtr> weights_dequant = ge::OpDescUtils::MutableWeights(dequant_node);
  if (weights_dequant.size() != 1) {
    REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][JdgDeqsclShp] weights_dequant is invalid, size %zu.",
                    weights_dequant.size());
    return PARAM_INVALID;
  }
  ge::GeTensorPtr deq_scale_tensor = weights_dequant[0];
  FE_CHECK(deq_scale_tensor == nullptr,
           REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][JdgDeqsclShp] deqScale is nullptr!"), return PARAM_INVALID);

  // translate deq_scale_tensor to scale_deq[32:63], N[24:31], offset_w[16:23]
  uint64_t *deq_scale_data =
  const_cast<uint64_t *>(reinterpret_cast<const uint64_t *>(deq_scale_tensor->GetData().data()));
  FE_CHECK(deq_scale_data == nullptr,
           REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][JdgDeqsclShp] deqScaleData is nullptr"),
           return PARAM_INVALID);
  const ge::GeShape &deq_scale_shape = deq_scale_tensor->GetTensorDesc().GetShape();
  if (deq_scale_shape.GetDimNum() != 1) {
    REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][JdgDeqsclShp] deqScale shape error, shape is %zu.",
                    deq_scale_shape.GetDimNum());
    return PARAM_INVALID;
  }
  int64_t deq_co = deq_scale_shape.GetDim(0);
  FE_LOGD("Node[%s] bias optimize, deq_scale_tensor dim is %ld.", dequant_node->GetName().c_str(), deq_co);
  return SUCCESS;
}

Status BiasOptimizeQuantRollbackBase::SetQuantParameters(const ge::NodePtr &cube_node,
                                                         const ge::NodePtr &quant_node) const {
  // set offset of quant_node to cube_node
  float_t offset_a = 0;
  (void)ge::AttrUtils::GetFloat(quant_node->GetOpDesc(), ATTR_OFFSET, offset_a);
  offset_a = static_cast<int8_t>(offset_a);
  // get offset_a from cube_op
  int64_t offset_temp = offset_a;
  if (!ge::AttrUtils::SetInt(cube_node->GetOpDesc(), ATTR_OFFSET_X, offset_temp)) {
    REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][SetQntPara] set conv offset failed!");
    return FAILED;
  }
  return SUCCESS;
}

Status BiasOptimizeQuantRollbackBase::CreateBiasInput(ge::ComputeGraph &graph, ge::NodePtr &cube_node,
                                                      const int64_t &co) {
  FE_LOGD("Cube node[name:%s] has no bias, create bias and set data", cube_node->GetName().c_str());
  FE_LOGD("The cube node has %ld input data Anchors.", cube_node->GetAllInDataAnchors().size());
  /* Get Conv Weight filter */
  if (RemoveEnter(graph, cube_node) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][SetQntPara] Remove enter node failed.");
    return FAILED;
  }

  // set bias
  ge::GeTensorDesc tmp_desc;
  ge::GeTensorPtr bias_ptr = nullptr;

  std::unique_ptr<int32_t[]> bias_data_temp(new (std::nothrow) int32_t[co]());
  FE_CHECK(bias_data_temp == nullptr, FE_LOGW("biasDataTemp is nullptr"), return PARAM_INVALID);
  for (int64_t i = 0; i < co; i++) {
    bias_data_temp[i] = 0;
  }

  FE_MAKE_SHARED(
      bias_ptr = std::make_shared<ge::GeTensor>(tmp_desc, reinterpret_cast<uint8_t *>(bias_data_temp.get()),
                                                co * sizeof(int32_t)),
      bias_ptr = nullptr;
      return PARAM_INVALID);

  // update weights
  ge::GeShape bias_shape({co});
  bias_ptr->MutableTensorDesc().SetShape(bias_shape);
  bias_ptr->MutableTensorDesc().SetDataType(ge::DT_INT32);

  Status ret = bias_ptr->SetData(reinterpret_cast<uint8_t *>(bias_data_temp.get()), co * sizeof(int32_t));
  if (ret != SUCCESS) {
    FE_LOGW("set bias data failed!");
    return ret;
  }

  ge::OpDescPtr const_opdesc = ge::OpDescUtils::CreateConstOp(bias_ptr);
  FE_CHECK(const_opdesc == nullptr,
           REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][SetQntPara] Fail to create const op desc."), return FAILED);
  string constOpName = cube_node->GetName() + "_bias";
  const_opdesc->SetName(constOpName);
  ge::NodePtr const_node = graph.AddNode(const_opdesc);
  FE_CHECK(const_node == nullptr,
           REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][SetQntPara] Fail to add const node."), return FAILED);
  if (cube_node->AddLinkFrom(bias_name_, const_node) != ge::GRAPH_SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][GetCoVal] Fail to link const node with conv node[%s].",
                    cube_node->GetName().c_str());
    return FAILED;
  }

  // update the bias output_desc of bias_op_desc
  ge::GeTensorDesc input_desc0 = cube_node->GetOpDesc()->GetInputDesc(0);
  ge::Format input_desc0_origin_format = input_desc0.GetOriginFormat();
  int bias_input_index = 2;
  ge::NodePtr bias_node = cube_node->GetInDataAnchor(bias_input_index)->GetPeerOutAnchor()->GetOwnerNode();
  ge::OpDescPtr bias_op_desc = bias_node->GetOpDesc();
  FE_LOGI("The value of bias_node_name is %s", bias_node->GetName().c_str());

  // only has one output, index 0
  ge::GeTensorDesc bias_output_desc = bias_op_desc->GetOutputDesc(0);
  bias_output_desc.SetShape(bias_shape);
  bias_output_desc.SetOriginFormat(input_desc0_origin_format);
  bias_output_desc.SetOriginShape(bias_shape);
  bias_output_desc.SetOriginDataType(ge::DT_INT32);
  bias_output_desc.SetDataType(ge::DT_INT32);
  if (bias_op_desc->UpdateOutputDesc(0, bias_output_desc) != ge::GRAPH_SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][SetQntPara] Update output desc of BiasNode[%s] not success.",
                    bias_op_desc->GetName().c_str());
    return FAILED;
  }

  // update the bias input_desc of the cube_op_desc
  ge::GeTensorDesc bias_desc = cube_node->GetOpDesc()->GetInputDesc(bias_input_index);
  bias_desc.SetShape(bias_shape);
  bias_desc.SetOriginFormat(input_desc0_origin_format);
  bias_desc.SetOriginShape(bias_shape);
  bias_desc.SetOriginDataType(ge::DT_INT32);
  bias_desc.SetDataType(ge::DT_INT32);
  if (cube_node->GetOpDesc()->UpdateInputDesc(2, bias_desc) != ge::GRAPH_SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][SetQntPara] update bias input desc of cube_node[%s] not success.",
                    cube_node->GetName().c_str());
    return FAILED;
  }
  return SUCCESS;
}

bool BiasOptimizeQuantRollbackBase::IsCubeHasBiasInput(ge::NodePtr cube_node) const {
  FE_CHECK(cube_node == nullptr, FE_LOGW("cubeOp is null."), return false);
  ge::NodePtr bias_node = nullptr;
  if (cube_node->GetOpDesc()->GetInputsSize() < INPUT_SIZE_CONTAINS_BIAS) {
    return false;
  }
  int bias_input_index = INPUT_SIZE_CONTAINS_BIAS - 1;
  if (cube_node->GetInDataAnchor(bias_input_index) != nullptr) {
    if (cube_node->GetInDataAnchor(bias_input_index)->GetPeerOutAnchor() != nullptr) {
      bias_node = cube_node->GetInDataAnchor(bias_input_index)->GetPeerOutAnchor()->GetOwnerNode();
    }
  }
  return bias_node != nullptr;
}

bool BiasOptimizeQuantRollbackBase::IsCubeNeedBiasInput(ge::NodePtr cube_node) {
  return !IsCubeHasBiasInput(cube_node);
}

Status BiasOptimizeQuantRollbackBase::BiasOptimize(ge::ComputeGraph &graph, ge::NodePtr &cube_node,
                                                   ge::NodePtr &dequant_node, ge::NodePtr &quant_node,
                                                   vector<ge::NodePtr> &fusion_nodes) {
  if (RemoveEnter(graph, dequant_node) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][BiasOpti] Remove enter node failed.");
    return FAILED;
  }
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
  if (IsCubeNeedBiasInput(cube_node)) {
    int64_t co = 1;
    if (GetCoValue(cube_node, co) != SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][BiasOpti] Get node[%s] co value.", cube_node->GetName().c_str());
      return FAILED;
    }
    if (CreateBiasInput(graph, cube_node, co) != SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][BiasOpti] Create cube node[%s] bias input failed.",
                      cube_node->GetName().c_str());
      return FAILED;
    }
  }
  /* The Bias already exists, we create a new host op
   * Crete Host Cpu Op for Bias Optimization
   */
  struct PatternNodes pattern_nodes = {cube_node, dequant_node, quant_node, nullptr, nullptr};
  if (GetWeightNodesOfCubeNode(cube_node, pattern_nodes.weight_const_node,
      pattern_nodes.ascend_weight_quant_node) != SUCCESS) {
    REPORT_FE_ERROR("get weight_const node and ascend_weight_quant node of cube node[%s] failed!",
        cube_node->GetName().c_str());
    return FAILED;
  }
  Status ret = CreateNewHostCpuOp(QUANT_BIAS_OPTIMIZATION, pattern_nodes, graph,
      static_cast<uint32_t>(HostOpMode::BIAS_OPTIMIZATION_MODE), fusion_nodes);
  if (ret != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][BiasOpti] Create host op failed.");
    return FAILED;
  }
  return SUCCESS;
}

Status BiasOptimizeQuantRollbackBase::RemoveInputEdgeAndSingleConstInput(
    const ge::NodePtr &node, std::vector<ge::OutDataAnchorPtr> &peer_out_anchors_of_node) const {
  std::vector<ge::OutDataAnchorPtr> new_vec;
  peer_out_anchors_of_node.swap(new_vec);
  // remove input data edge
  for (size_t i = 0; i < node->GetAllInDataAnchors().size(); ++i) {
    if (node->GetOpDesc()->MutableInputDesc(i) == nullptr) {
      continue;
    }
    auto in_data_anchor = node->GetInDataAnchor(i);
    FE_CHECK(in_data_anchor == nullptr,
             REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][RmInEgSglCstIn] indataAnchor is null"), return FAILED);
    auto pre_out_data_anchor = in_data_anchor->GetPeerOutAnchor();
    FE_CHECK(pre_out_data_anchor == nullptr,
             REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][RmInEgSglCstIn] outdataAnchor is null"), return FAILED);
    peer_out_anchors_of_node.emplace_back(pre_out_data_anchor);
    ge::NodePtr tmp_node = pre_out_data_anchor->GetOwnerNode();
    if (ge::GraphUtils::RemoveEdge(pre_out_data_anchor, in_data_anchor) != ge::GRAPH_SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][RmInEgSglCstIn] remove inputdata edge error");
      return FAILED;
    }
    FE_CHECK(tmp_node == nullptr,
             REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][RmInEgSglCstIn] tmpNode is null"), return FAILED);
  }
  // remove input control edge
  ge::InControlAnchorPtr in_control_anchor = node->GetInControlAnchor();
  if (in_control_anchor != nullptr) {
    for (ge::OutControlAnchorPtr src_anchor : in_control_anchor->GetPeerOutControlAnchors()) {
      if (ge::GraphUtils::RemoveEdge(src_anchor, in_control_anchor) != ge::GRAPH_SUCCESS) {
        REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][RmInEgSglCstIn] Disconnect %s input control anchor failed.",
                        node->GetName().c_str());
        return FAILED;
      }
    }

    for (ge::OutDataAnchorPtr src_anchor : in_control_anchor->GetPeerOutDataAnchors()) {
      if (ge::GraphUtils::RemoveEdge(src_anchor, in_control_anchor) != ge::GRAPH_SUCCESS) {
        REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][RmInEgSglCstIn] Disconnect %s input control anchor failed.",
                        node->GetName().c_str());
        return FAILED;
      }
    }
  }

  return SUCCESS;
}

Status BiasOptimizeQuantRollbackBase::LinkOutputEdgeWithoutControl(
    ge::NodePtr old_node, ge::NodePtr new_node, const string &cube_name,
    const std::vector<ge::OutDataAnchorPtr> &peer_out_anchors_of_old_node) const {
  /* when old node is quant, we need to get the first input of quant to
   * find the new_node's output anchor */
  ge::OutDataAnchorPtr new_out_data_anchor;
  if (peer_out_anchors_of_old_node.empty()) {
    new_out_data_anchor = old_node->GetInDataAnchor(0)->GetPeerOutAnchor();
  } else {
    new_out_data_anchor = peer_out_anchors_of_old_node[0];
  }

  FE_CHECK(new_out_data_anchor == nullptr,
           REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][LkOutEg] new out data anchor is null for %s and %s",
           old_node->GetName().c_str(), new_node->GetName().c_str()),
           return FAILED);
  for (ge::OutDataAnchorPtr anchor : old_node->GetAllOutDataAnchors()) {
    FE_CHECK_NOTNULL(anchor);
    for (ge::InDataAnchorPtr dst_anchor : anchor->GetPeerInDataAnchors()) {
      if ((old_node->GetType() == QUANT) && (dst_anchor->GetOwnerNode()->GetName() != cube_name)) {
        continue;
      }
      if (ge::GraphUtils::RemoveEdge(anchor, dst_anchor) != ge::GRAPH_SUCCESS ||
          ge::GraphUtils::AddEdge(new_out_data_anchor, dst_anchor) != ge::GRAPH_SUCCESS) {
        REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][LkOutEg] Replace out data anchor Failed.");
        return FAILED;
      }
    }

    for (ge::InControlAnchorPtr dst_anchor : anchor->GetPeerInControlAnchors()) {
      if ((old_node->GetType() == QUANT) && (dst_anchor->GetOwnerNode()->GetName() != cube_name)) {
        continue;
      }
      if (ge::GraphUtils::RemoveEdge(anchor, dst_anchor) != ge::GRAPH_SUCCESS ||
          ge::GraphUtils::AddEdge(new_out_data_anchor, dst_anchor) != ge::GRAPH_SUCCESS) {
        REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][LkOutEg] Replace input control anchor Failed.");
        return FAILED;
      }
    }
  }

  return SUCCESS;
}

std::string BiasOptimizeQuantRollbackBase::GetIsaArchVersionStr() const {
  ISAArchVersion isa_arch_ver = Configuration::Instance(AI_CORE_NAME).GetIsaArchVer();
  auto iter = ISA_ARCH_VERSION_STRING_MAP.find(isa_arch_ver);
  if (iter != ISA_ARCH_VERSION_STRING_MAP.end()) {
    return iter->second;
  } else {
    REPORT_FE_ERROR("[GraphOpt][BiasOptQutRollBack][GetArchVer]ISA arch version[%d] is invalid.",
                    static_cast<int32_t>(isa_arch_ver));
    return "";
  }
}

/*
 * ***********************************tf case can refer to the following***************************************
 *
 * ================================original graph=====================================
 *
 *                    offset_const     bias_const     deq_scale_const
 *                        \               \               \
 *                         v               v               v
 * weight_const--->ascendweightquant--->cube_node--->ascenddequant
 *                                        ^
 *                                       /
 *                                  ascendquant
 *                                     ^
 *                                    /
 *                                  input
 *
 * =======================if do quant rollback, step like below=======================
 * [after weight rollback]
 *
 *                                 deq_scale_const
 *                                       |
 *                                       |
 *                                --------------
 *                               /              \
 *                              /                \
 *                             /                  \
 *                            /                    \
 *                           /        bias_const    \
 *                          /           \            \
 *                         v             v            v
 * weight_const--->QuantWeightRollback--->cube--->ascenddequant
 *                                       ^
 *                                      /
 *                                  ascendquant
 *                                     ^
 *                                    /
 *                                  input
 *
 *
 *
 * [after bias rollback]
 *
 *                                   deq_scale_const      bias_const
 *                                   /           \          |
 *                                  /             \         |
 *                                 /               \        |
 *                                /                 v       v
 *                               /                QuantBiasRollback
 *                              /                  /
 *                             /                  /
 *                            /                  /
 *                           /                  /
 *                          /                  /
 *                         /                  /
 *                        /                  /
 *                       /                  /
 *                      v                  v
 * weight_const--->QuantWeightRollback--->cube
 *                                       ^
 *                                      /
 *                                     /
 *                                   input
 *
 *
 *
 * =======================if do bias optimization, step like below=======================
 *
 *                             deq_scale_const     bias_vonst
 *                               \        \         |
 *                                \        \        |
 *                                 \        \       |
 *                                  \        v      v
 * weight_const----------------------\----->QuantBiasOptimization
 *      |                             \      /
 *      |                              \    /
 *      |                               \  /
 *      |                                \/
 *      |                                /\
 *      |       offset_const            /  \
 *      |            /                 /    \
 *      |           /                 /      \
 *      |          v                 v        v
 *      |---ascendweightquant---->cube---->ascenddequant
 *                                 ^
 *                                /
 *                          ascendquant
 *                             ^
 *                            /
 *                          input
 * */
Status BiasOptimizeQuantRollbackBase::CreateNewHostCpuOp(const string &op_type, const struct PatternNodes &pattern_node,
                                                         ge::ComputeGraph &graph, uint32_t mode /* HostOpMode */,
                                                         vector<ge::NodePtr> &fus_nodes) {
  FE_LOGI("Create host op[type:%s] for cube node %s and dequant node %s in mode %u", op_type.c_str(),
          pattern_node.cube_node->GetName().c_str(), pattern_node.dequant_node->GetName().c_str(), mode);
  bool mode_flag = (mode >= static_cast<uint32_t>(HostOpMode::MODE_BOTTOM)
      || TENSOR_NAME_OF_HOST_CPU_OP.size() < static_cast<uint32_t>(HostOpMode::MODE_BOTTOM));
  if (mode_flag) {
    REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][CrtNewHostCpuOp] mode %u is invalid for [%s] and tensor size is %zu",
                    mode, pattern_node.cube_node->GetName().c_str(), TENSOR_NAME_OF_HOST_CPU_OP.size());
  }

  std::stringstream op_name_temp;
  // The atomic id of trans nodes must be unique.(start from 0)
  op_name_temp << op_type << "_" << GetHostCpuAtomicId();
  ge::OpDescPtr quant_host_cpu_op;
  FE_MAKE_SHARED(quant_host_cpu_op = std::make_shared<ge::OpDesc>(op_name_temp.str(), op_type), return FAILED);

  if (pattern_node.weight_const_node == nullptr) {
    FE_LOGE("weight_const_node is null");
    return FAILED;
  }

  // 1. get deq_scale_tensor(64)
  /* The tensor desc of multable weights is not reliable so we get the
   * tensor desc from the node's opdesc. */
  auto deq_scale_tensor = pattern_node.dequant_node->GetOpDesc()->GetInputDesc(DEQUANT_SCALE_INDEX);
  const ge::GeTensorDesc &scale_input_of_new_host_cpu_op = deq_scale_tensor;

  bool empty_flag = TENSOR_NAME_OF_HOST_CPU_OP.empty() || ORIGINAL_CONV_ANCHOR_INDEX.empty();
  if (empty_flag) {
    REPORT_FE_ERROR("tensor name size %lu or ORIGINAL_CONV_ANCHOR_INDEX %lu is empty!",
                    TENSOR_NAME_OF_HOST_CPU_OP.size(), ORIGINAL_CONV_ANCHOR_INDEX.size());
    return FAILED;
  }
  auto anchor_indexlist = ORIGINAL_CONV_ANCHOR_INDEX[mode];
  vector<string> tensor_desc_name_list = TENSOR_NAME_OF_HOST_CPU_OP[mode];
  ge::GeTensorDesc output_desc;
  // 2. construct quant_host_cpu_op inputdesc and outputdesc
  for (size_t i = 0; i < anchor_indexlist.size(); i++) {
    size_t inputs_size = pattern_node.cube_node->GetOpDesc()->GetAllInputsSize();
    auto weight_anchor_index = anchor_indexlist[i];
    Status desc_result = ConstructDesc(i, inputs_size, weight_anchor_index,
                          pattern_node, tensor_desc_name_list, quant_host_cpu_op, output_desc);
    if (desc_result != SUCCESS) {
      return desc_result;
    }
  }

  quant_host_cpu_op->AddInputDesc(DEQUANT_SCALE, scale_input_of_new_host_cpu_op);
  for (uint32_t i = 0; i < quant_host_cpu_op->GetAllInputsSize(); i++) {
    auto tensor_desc_ptr = quant_host_cpu_op->MutableInputDesc(i);
    if (tensor_desc_ptr == nullptr) {
      FE_LOGI("Tensor_desc_ptr is null.");
      continue;
    }
    /* Keep the original data type and format the same as the current ones */
    tensor_desc_ptr->SetOriginDataType(tensor_desc_ptr->GetDataType());
    tensor_desc_ptr->SetOriginFormat(static_cast<ge::Format>(ge::GetPrimaryFormat(tensor_desc_ptr->GetFormat())));
    tensor_desc_ptr->SetOriginShape(tensor_desc_ptr->GetShape());
  }
  quant_host_cpu_op->AddOutputDesc(CUBE_QUANT_ROLL_BACK_OUTPUT, output_desc);
  quant_host_cpu_op->MutableOutputDesc(0)->SetOriginFormat(
      static_cast<ge::Format>(ge::GetPrimaryFormat(output_desc.GetFormat())));
  quant_host_cpu_op->MutableOutputDesc(0)->SetOriginDataType(output_desc.GetDataType());
  quant_host_cpu_op->MutableOutputDesc(0)->SetOriginShape(output_desc.GetShape());

  FE_LOGI("Node[name:%s] has %zu inputs and %zu outputs.", quant_host_cpu_op->GetName().c_str(),
          quant_host_cpu_op->GetInputsSize(), quant_host_cpu_op->GetOutputsSize());
  auto roll_back_node = graph.AddNode(quant_host_cpu_op);
  FE_CHECK_NOTNULL(roll_back_node);
  fus_nodes.push_back(roll_back_node);
  // 3. Handle edge
  for (size_t i = 0; i < anchor_indexlist.size(); i++) {
    if (LinkHostOpEdge(pattern_node, roll_back_node, anchor_indexlist[i], i) != SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][CrtNewHostCpuOp][LinkHostOpEdge] Add Edge of host node [%s,%s] fail, cube node is %s",
                      roll_back_node->GetName().c_str(), roll_back_node->GetType().c_str(),
                      pattern_node.cube_node->GetName().c_str());
      return FAILED;
    }
  }

  ge::OutDataAnchorPtr scale_weight_output_anchor =
      pattern_node.dequant_node->GetInDataAnchor(DEQUANT_SCALE_INDEX_OF_DEQUANT_OP)->GetPeerOutAnchor();
  FE_CHECK_NOTNULL(scale_weight_output_anchor);
  auto scale_index = anchor_indexlist.size();
  FE_LOGI("Node[%s]: scale index is %zu", roll_back_node->GetName().c_str(), scale_index);
  auto quant_rollback_host_cpu_input_anchor1 = roll_back_node->GetInDataAnchor(scale_index);
  if (ge::GraphUtils::AddEdge(scale_weight_output_anchor, quant_rollback_host_cpu_input_anchor1) != ge::GRAPH_SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][CrtNewHostCpuOp] Add Edge between dequant_weight %s and cpu op %s\
                    : %zu failed.", pattern_node.dequant_node->GetName().c_str(),
        roll_back_node->GetName().c_str(), scale_index);
    return FAILED;
  }

  // 4. set attr
  SetCinCoutReverse(roll_back_node);
  int64_t groups = 1;
  (void)ge::AttrUtils::GetInt(pattern_node.cube_node->GetOpDesc(), "groups", groups);
  (void)ge::AttrUtils::SetInt(roll_back_node->GetOpDesc(), "groups", groups);
  (void)ge::AttrUtils::SetBool(roll_back_node->GetOpDesc(), kIsComeFromConstOp, true);

  // 5. Set Addtional Attrs
  (void)ge::AttrUtils::SetStr(roll_back_node->GetOpDesc(), ATTR_CUBE_OP_TYPE, pattern_node.cube_node->GetType());
  bool roll_back_scenario = op_type == QUANT_WEIGHT_ROLL_BACK || op_type == QUANT_BIAS_ROLL_BACK;
  if (roll_back_scenario) {
    float_t scale_a = 0;
    (void)ge::AttrUtils::GetFloat(pattern_node.quant_node->GetOpDesc(), ATTR_SCALE, scale_a);
    (void)ge::AttrUtils::SetFloat(roll_back_node->GetOpDesc(), ATTR_SCALE, scale_a);
  }
  if (op_type == QUANT_BIAS_OPTIMIZATION) {
    const std::string soc_version = GetIsaArchVersionStr();
    (void)ge::AttrUtils::SetStr(roll_back_node->GetOpDesc(), "soc_version", soc_version);
    float_t offset_a = 0;
    (void)ge::AttrUtils::GetFloat(pattern_node.quant_node->GetOpDesc(), ATTR_OFFSET, offset_a);
    (void)ge::AttrUtils::SetFloat(roll_back_node->GetOpDesc(), ATTR_OFFSET, offset_a);
  }
  // set int4 or int8
  if (SetInt4OrInt8Flag(pattern_node.ascend_weight_quant_node, roll_back_node) != SUCCESS) {
    REPORT_FE_ERROR("set int4 or int8 flag for roll_back_node[name:%s] failed.", roll_back_node->GetName().c_str());
    return FAILED;
  }

  FE_LOGD("Add node[name:%s] into the graph for cube node[name:%s] successfully.", roll_back_node->GetName().c_str(),
          pattern_node.cube_node->GetName().c_str());
  return SUCCESS;
}

Status BiasOptimizeQuantRollbackBase::LinkHostOpEdge(const struct PatternNodes &pattern_node, ge::NodePtr &host_node,
                                                     const uint32_t &anchor_index, const size_t &index) const {
  // Get filter const
  ge::InDataAnchorPtr conv_weight_input_anchor = pattern_node.cube_node->GetInDataAnchor(anchor_index);
  ge::OutDataAnchorPtr conv_weight_peer_output_anchor = conv_weight_input_anchor->GetPeerOutAnchor();
  FE_CHECK_NOTNULL(conv_weight_peer_output_anchor);
  auto quant_host_cpu_input_anchor0 = host_node->GetInDataAnchor(index);
  FE_CHECK_NOTNULL(quant_host_cpu_input_anchor0);
  // Link Const -> hostop
  if (anchor_index == 1) {
    // handle weight
    ge::OutDataAnchorPtr weight_const_output_anchor = pattern_node.weight_const_node->GetOutDataAnchor(0);
    if (ge::GraphUtils::AddEdge(weight_const_output_anchor, quant_host_cpu_input_anchor0) != ge::GRAPH_SUCCESS) {
      REPORT_FE_ERROR("Add Edge between weight_const[its cube node is %s] and host op %s: %zu failed.",
                      pattern_node.cube_node->GetName().c_str(), host_node->GetName().c_str(), index);
      return FAILED;
    }
  } else {
    // handle bias
    if (ge::GraphUtils::AddEdge(conv_weight_peer_output_anchor, quant_host_cpu_input_anchor0) != ge::GRAPH_SUCCESS) {
      REPORT_FE_ERROR("Add Edge between conv_weight %s and host op %s: %zu failed.",
                      pattern_node.cube_node->GetName().c_str(), host_node->GetName().c_str(), index);
      return FAILED;
    }
  }

  /* The first edge is always be the major edge, we insert a host op in this
   * edge, so we need to cut the original connection */
  if (index == 0) {
    if (ge::GraphUtils::RemoveEdge(conv_weight_peer_output_anchor, conv_weight_input_anchor) != ge::GRAPH_SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][CreateHostOp][LinkHostOpEdge] Remove Edge between weight and conv %s : %zu failed.",
                      pattern_node.cube_node->GetName().c_str(), index);
      return FAILED;
    }
    // Link host op -> conv
    auto quant_host_cpu_output_anchor = host_node->GetOutDataAnchor(index);
    FE_CHECK_NOTNULL(quant_host_cpu_output_anchor);
    if (ge::GraphUtils::AddEdge(quant_host_cpu_output_anchor, conv_weight_input_anchor) != ge::GRAPH_SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][CreateHostOp][LinkHostOpEdge] Add Edge between host op %s and weight %s: %zu failed.",
                      host_node->GetName().c_str(), pattern_node.cube_node->GetName().c_str(), index);
      return FAILED;
    }
  }
  return SUCCESS;
}

Status BiasOptimizeQuantRollbackBase::SetGroupInfoForAscendWeightQuantNodes(const ge::NodePtr &cube_node) const {
  auto all_input_nodes = cube_node->GetInDataNodes();
  FE_CHECK(all_input_nodes.size() < 2,
           FE_LOGD("cube node [%s] dose not have a weight node, no need to set group info.",
                   cube_node->GetName().c_str()),
           return SUCCESS);
  auto ascend_weight_quant_node = all_input_nodes.at(1);
  FE_CHECK(ascend_weight_quant_node == nullptr, FE_LOGE("get AscendWeightQuant node failed."), return FAILED);
  int64_t groups_val = GROUPS_DEFAULT_VALUE;
  (void) ge::AttrUtils::GetInt(cube_node->GetOpDesc(), ATTR_NAME_GROUPS, groups_val);
  (void) ge::AttrUtils::SetInt(ascend_weight_quant_node->GetOpDesc(), ATTR_NAME_GROUPS, groups_val);
  FE_LOGD("set group info for ascend_weight_quant_node [name:%s] succeed, group value is %ld.",
          ascend_weight_quant_node->GetName().c_str(), groups_val);
  return SUCCESS;
}

Status BiasOptimizeQuantRollbackBase::ConstructDesc(const size_t &i, const size_t &inputs_size,
                                                    const uint32_t &weight_anchor_index,
                                                    const struct PatternNodes &pattern_node,
                                                    const vector<string> &tensor_desc_name_list,
                                                    const ge::OpDescPtr &quant_host_cpu_op,
                                                    ge::GeTensorDesc &output_desc) const {
  if (weight_anchor_index >= inputs_size) {
    REPORT_FE_ERROR("Weight Index %u >= input size %zu.", weight_anchor_index, inputs_size);
    return PARAM_INVALID;
    }

  if (i >= tensor_desc_name_list.size()) {
    REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][CrtNewHostCpuOp] i %zu is larger than tensor name size %zu.",
                    i, tensor_desc_name_list.size());
    return PARAM_INVALID;
  }

  ge::GeTensorDesc filter_or_bias_input_of_new_host_cpu_op = ((weight_anchor_index == 1) ?
      pattern_node.weight_const_node->GetOpDesc()->GetOutputDesc(0) :
      pattern_node.cube_node->GetOpDesc()->GetInputDesc(weight_anchor_index));
  FE_LOGI("Tensor index [%zu]'s name is [%s]", i, tensor_desc_name_list[i].c_str());

  quant_host_cpu_op->AddInputDesc(tensor_desc_name_list[i], filter_or_bias_input_of_new_host_cpu_op);

  /* The output desc should be the same as the main opdesc */
  if (i == 0) {
    output_desc = filter_or_bias_input_of_new_host_cpu_op;
  }
  return SUCCESS;
}
/*
 * *****************************caffe case**********************************
 *
 *                 bias_const
 *                   \
 *                    v
 * weight_const--->cube_node
 *                    ^
 *                   /
 *              ascendquant
 *
 * *****************************other case**********************************
 *
 *                    offset_const     bias_const
 *                        \               \
 *                         v               v
 * weight_const--->ascendweightquant--->cube_node
 *                                        ^
 *                                       /
 *                                  ascendquant
 *
 * weightnodes include weight_const and ascendweightquant(ascendweightquant is nullptr if caffe case)
 * */
Status BiasOptimizeQuantRollbackBase::GetWeightNodesOfCubeNode(const ge::NodePtr &cube_node,
    ge::NodePtr &weight_const_node, ge::NodePtr &ascend_weight_quant_node) const {
  auto cube_weight_input_anchor = cube_node->GetInDataAnchor(1);
  if (cube_weight_input_anchor == nullptr) {
    FE_LOGE("cube_weight_input_anchor is null");
    return FAILED;
  }
  auto peer_out_anchor_of_cube_weight_input_anchor = cube_weight_input_anchor->GetPeerOutAnchor();
  if (peer_out_anchor_of_cube_weight_input_anchor == nullptr) {
    FE_LOGE("peer_out_anchor_of_cube_weight_input_anchor is null");
    return FAILED;
  }
  auto cube_weight_input_node = peer_out_anchor_of_cube_weight_input_anchor->GetOwnerNode();
  if (cube_weight_input_node == nullptr) {
    FE_LOGE("cube_weight_input_node is null");
    return FAILED;
  }
  auto cube_weight_input_node_first_input_anchor = cube_weight_input_node->GetInDataAnchor(0);
  // if dynamic batch or dynamic shape, cube_weight_input_node will be a Data node
  if (cube_weight_input_node_first_input_anchor == nullptr || cube_weight_input_node->GetType() == fe::DATA) {
    // caffe case
    FE_LOGD("in caffe case, set ascend_weight_quant_node = nullptr");
    ascend_weight_quant_node = nullptr;
    weight_const_node = cube_weight_input_node;
  } else {
    // other case
    ascend_weight_quant_node = cube_weight_input_node;
    auto weight_const_output_anchor = cube_weight_input_node_first_input_anchor->GetPeerOutAnchor();
    if (weight_const_output_anchor == nullptr) {
      FE_LOGE("weight_const_output_anchor is null");
      return FAILED;
    }
    weight_const_node = weight_const_output_anchor->GetOwnerNode();
    if (weight_const_node == nullptr) {
      FE_LOGE("weight_const_node is null");
      return FAILED;
    }
  }

  return SUCCESS;
}

/*
 * *****************************caffe case**********************************
 *
 *                 bias_const
 *                   \
 *                    v
 * weight_const--->cube_node
 *                    ^
 *                   /
 *              ascendquant
 *
 * *****************************other case**********************************
 *
 *                    offset_const     bias_const
 *                        \               \
 *                         v               v
 * weight_const--->ascendweightquant--->cube_node
 *                                        ^
 *                                       /
 *                                  ascendquant
 *
 * after weightRollback, ascendweightquant and offset_const is isolate,
 * delete them(NOTICE: IN CAFFE CASE DONT NEED DO THIS)
 * */
Status BiasOptimizeQuantRollbackBase::WeightRollbackDeleteIsolateNodes(ge::ComputeGraph &graph,
    ge::NodePtr &ascend_weight_quant_node) const {
  if (ascend_weight_quant_node == nullptr) {
    // caffe case
    return SUCCESS;
  }

  // find offset_const
  ge::NodePtr offset_const_node = nullptr;
  auto ascend_weight_quant_offset_input_anchor = ascend_weight_quant_node->GetInDataAnchor(1);
  if (ascend_weight_quant_offset_input_anchor == nullptr) {
    FE_LOGE("node: %s indataAnchor is null", ascend_weight_quant_node->GetName().c_str());
    return FAILED;
  }
  auto offset_const_output_anchor = ascend_weight_quant_offset_input_anchor->GetPeerOutAnchor();
  if (offset_const_output_anchor == nullptr) {
    FE_LOGE("node: %s peerOutAnchor is null", ascend_weight_quant_node->GetName().c_str());
    return FAILED;
  }
  offset_const_node = offset_const_output_anchor->GetOwnerNode();
  if (offset_const_node == nullptr) {
    FE_LOGE("offset_const_node is null");
    return FAILED;
  }

  if (ascend_weight_quant_node->GetOutDataNodes().empty()) {
    (void)ge::GraphUtils::RemoveEdge(offset_const_output_anchor, ascend_weight_quant_offset_input_anchor);
    if (offset_const_node->GetOutDataNodes().empty()) {
      // delete ascendweightquant and offset_const
      if (graph.RemoveNode(offset_const_node) == ge::GRAPH_FAILED) {
        REPORT_FE_ERROR("offset_const node remove failed!");
        return FAILED;
      }
    }
    if (graph.RemoveNode(ascend_weight_quant_node) == ge::GRAPH_FAILED) {
      REPORT_FE_ERROR("ascend_weight_quant node remove failed!");
      return FAILED;
    }
  }
  return SUCCESS;
}

Status BiasOptimizeQuantRollbackBase::SetInt4OrInt8Flag(const ge::NodePtr &ascend_weight_quant_node,
                                                        const ge::NodePtr &quant_host_cpu_op_node) const {
  int dst_type = ge::DT_INT8;
  if (ascend_weight_quant_node != nullptr) {
    (void)ge::AttrUtils::GetInt(ascend_weight_quant_node->GetOpDesc(), "dst_type", dst_type);
  }
  (void)ge::AttrUtils::SetInt(quant_host_cpu_op_node->GetOpDesc(), "dst_type", dst_type);
  return SUCCESS;
}

Status BiasOptimizeQuantRollbackBase::SetCubeNodeDataType(const ge::NodePtr &cube_node, const ge::DataType &data_type,
                                                          const ge::DataType &target_data_type) const {
  ge::OpDescPtr OpDesc = cube_node->GetOpDesc();
  FE_LOGD("Node[%s] input size is %zu.", OpDesc->GetName().c_str(), cube_node->GetAllInDataAnchors().size());
  for (size_t i = 0; i < cube_node->GetAllInDataAnchors().size(); ++i) {
    if (OpDesc->MutableInputDesc(i) == nullptr) {
      continue;
    }
    ge::GeTensorDesc input_desc = OpDesc->GetInputDesc(i);
    input_desc.SetDataType(target_data_type);
    input_desc.SetOriginDataType(target_data_type);
    if (OpDesc->UpdateInputDesc(i, input_desc) != ge::GRAPH_SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][SetCubeNdDtype] update input desc of filter_node[%s] not success.",
                      OpDesc->GetName().c_str());
      return FAILED;
    }
    auto in_data_anchor = cube_node->GetInDataAnchor(i);
    FE_CHECK(in_data_anchor == nullptr,
             REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][SetCubeNdDtype] indataAnchor is null"), return FAILED);

    /* Set the output desc data type as fp32 for the output of host cpu op */
    if (i == 0) {
      /* We don't need to set input 0's peer out anchor because input0 is quant.
       * And input1 and input2 is the filter and bias which will be rolled back.
       * */
      continue;
    }

    FE_LOGD("Now update the %zuth input desc of filterNode[%s].", i, OpDesc->GetName().c_str());
    auto pre_out_data_anchor = in_data_anchor->GetPeerOutAnchor();
    if (pre_out_data_anchor == nullptr) {
      continue;
    }

    ge::NodePtr input_node = pre_out_data_anchor->GetOwnerNode();
    FE_CHECK(input_node == nullptr,
             REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][SetCubeNdDtype] inputNode is null"), return FAILED);

    ge::OpDescPtr op = input_node->GetOpDesc();
    FE_CHECK(op == nullptr, REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][SetCubeNdDtype] op is null"), return FAILED);
    ge::GeTensorDesc output_desc = op->GetOutputDesc(pre_out_data_anchor->GetIdx());
    output_desc.SetDataType(data_type);
    output_desc.SetOriginDataType(data_type);
    if (op->UpdateOutputDesc(pre_out_data_anchor->GetIdx(), output_desc) != ge::GRAPH_SUCCESS) {
      REPORT_FE_ERROR(
          "[GraphOpt][AvgPolQntPcsFus][SetCubeNdDtype] update output desc of filter_node[%s] not success.",
          op->GetName().c_str());
      return FAILED;
    }
  }
  ge::GeTensorDesc output_desc = OpDesc->GetOutputDesc(0);
  output_desc.SetDataType(target_data_type);
  output_desc.SetOriginDataType(target_data_type);
  if (OpDesc->UpdateOutputDesc(0, output_desc) != ge::GRAPH_SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][SetCubeNdDtype] update output desc of filter_node[%s] not success.",
                    OpDesc->GetName().c_str());
    return FAILED;
  }
  return SUCCESS;
}

Status BiasOptimizeQuantRollbackBase::SetDataTypeOfNodes(const ge::NodePtr &cube_node) {
  ge::DataType data_type = ge::DT_FLOAT;
  ge::DataType target_data_type = ge::DT_FLOAT16;

  if (SetCubeNodeDataType(cube_node, data_type, target_data_type) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][SetDtypeofNd] Set cube node[%s] data type failed.",
                    cube_node->GetName().c_str());
    return FAILED;
  }
  FE_LOGD("Set cube node[%s] data type success.", cube_node->GetName().c_str());

  return SUCCESS;
}

Status BiasOptimizeQuantRollbackBase::DoFusion(ge::ComputeGraph &graph, const ge::NodePtr &cube_node,
                                               const ge::NodePtr &quant_node, const ge::NodePtr &dequant_node,
                                               vector<ge::NodePtr> &fus_nodes) {
  FE_CHECK(cube_node == nullptr,
           REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][DoFus] cubeNode is null, fusion failed."),
           return PARAM_INVALID);
  FE_CHECK(quant_node == nullptr,
           REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][DoFus] quantNode is null, fusion failed."),
           return PARAM_INVALID);
  FE_CHECK(dequant_node == nullptr,
           REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][DoFus] dequantNode is null, fusion failed."),
           return PARAM_INVALID);

  ge::OpDescPtr cube_op = cube_node->GetOpDesc();
  ge::OpDescPtr quant_op = quant_node->GetOpDesc();
  ge::OpDescPtr dequant_op = dequant_node->GetOpDesc();

  FE_CHECK(cube_op == nullptr,
           REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][DoFus] cubeOp is null, fusion failed."),
           return PARAM_INVALID);
  FE_CHECK(quant_op == nullptr,
           REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][DoFus] quantOp is null, fusion failed."),
           return PARAM_INVALID);
  FE_CHECK(dequant_node == nullptr,
           REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][DoFus] dequantNode is null, fusion failed."),
           return PARAM_INVALID);

  /* Crete Host Cpu Op for conv + weight roll back */
  struct PatternNodes pattern_nodes = {cube_node, dequant_node, quant_node, nullptr, nullptr};
  if (GetWeightNodesOfCubeNode(cube_node, pattern_nodes.weight_const_node,
      pattern_nodes.ascend_weight_quant_node) != SUCCESS) {
    REPORT_FE_ERROR("get weight_const node and ascend_weight_quant node of cube node[%s] failed!",
        cube_node->GetName().c_str());
    return FAILED;
  }
  Status ret = CreateNewHostCpuOp(QUANT_WEIGHT_ROLL_BACK, pattern_nodes, graph,
      static_cast<uint32_t>(HostOpMode::WEIGHT_ROLL_BACK_MODE), fus_nodes);
  if (ret != SUCCESS) {
    return FAILED;
  }

  if (IsCubeHasBiasInput(cube_node)) {
    ret = CreateNewHostCpuOp(QUANT_BIAS_ROLL_BACK, pattern_nodes, graph,
        static_cast<uint32_t>(HostOpMode::BIAS_ROLL_BACK_MODE), fus_nodes);
    if (ret != SUCCESS) {
      return FAILED;
    }
  }

  // ascendweightquant and offset_const is isolate, delete them
  if (WeightRollbackDeleteIsolateNodes(graph, pattern_nodes.ascend_weight_quant_node) != SUCCESS) {
    REPORT_FE_ERROR("in mode quant_roll_back, delete ascendweightquant and offset_const failed.");
    return FAILED;
  }

  return SetDataTypeOfNodes(cube_node);
}

ge::NodePtr BiasOptimizeQuantRollbackBase::GetCubeNodeInputNode(const ge::NodePtr &cube_node) const {
  return cube_node;
}

Status BiasOptimizeQuantRollbackBase::ChangeQuantNodeEdge(ge::ComputeGraph &graph, const ge::NodePtr &cube_node,
                                                          ge::NodePtr &quant_node) const {
  std::vector<ge::OutDataAnchorPtr> peer_out_anchors_of_node;
  ge::NodePtr node_after_quant = GetCubeNodeInputNode(cube_node);
  ge::NodePtr input_node = quant_node->GetInDataNodes().at(0);
  if (quant_node->GetOutDataAnchor(0)->GetPeerInDataAnchors().size() == 1) {
    if (RemoveInputEdgeAndSingleConstInput(quant_node, peer_out_anchors_of_node) == FAILED) {
      REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][ChgQntNdEg] link output edge Failed.");
      return FAILED;
    }
  } else {
    // remove the edge between conv and quant
    if (ge::GraphUtils::RemoveEdge(quant_node->GetOutDataAnchor(0), node_after_quant->GetInDataAnchor(0)) !=
        ge::GRAPH_SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][ChgQntNdEg] Fail to remove edge between node[%s] and node[%s].",
                      quant_node->GetName().c_str(), node_after_quant->GetName().c_str());
      return FAILED;
    }

    if (ge::GraphUtils::AddEdge(quant_node->GetInDataAnchor(0)->GetPeerOutAnchor(),
                                node_after_quant->GetInDataAnchor(0)) != ge::GRAPH_SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][ChgQntNdEg] Fail to add edge for the input data anchor of node[%s].",
                      node_after_quant->GetName().c_str());
      return FAILED;
    }
  }

  string node_name = node_after_quant->GetName();
  if (LinkOutputEdgeWithoutControl(quant_node, input_node, node_name, peer_out_anchors_of_node) == FAILED) {
    REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][ChgQntNdEg] link output edge Failed.");
    return FAILED;
  }

  if (quant_node->GetOutDataAnchor(0)->GetPeerInDataAnchors().empty()) {
    if (graph.RemoveNode(quant_node) == ge::GRAPH_FAILED) {
      REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][ChgQntNdEg] cast node remove failed");
      return FAILED;
    }
  }
  return SUCCESS;
}

Status BiasOptimizeQuantRollbackBase::ChangeDequantNodeEdge(ge::ComputeGraph &graph, ge::NodePtr &cube_node,
                                                            ge::NodePtr &dequant_node) {
  std::vector<ge::OutDataAnchorPtr> peer_out_anchors_of_node;
  string cube_name = cube_node->GetName();
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

  return SUCCESS;
}

Status BiasOptimizeQuantRollbackBase::QuantRollback(ge::ComputeGraph &graph, ge::NodePtr &cube_node,
                                                    ge::NodePtr &dequant_node, ge::NodePtr &quant_node,
                                                    vector<ge::NodePtr> &fusion_nodes) {
  // do quant rollback
  FE_LOGD("Begin to do quant rollback fusion on cube node[name:%s, type:%s].", cube_node->GetName().c_str(),
          cube_node->GetType().c_str());
  // do fuion, quant rollback
  Status ret = DoFusion(graph, cube_node, quant_node, dequant_node, fusion_nodes);
  if (ret != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][QntRlbk] do fusion failed!");
    return ret;
  }
  FE_LOGD("quant rollback fusion success.");

  if (ChangeQuantNodeEdge(graph, cube_node, quant_node) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][QntRlbk] Cube node[%s]: change quant node edge failed.",
                    cube_node->GetName().c_str());
    return FAILED;
  }
  FE_LOGD("Cube node[%s]: change quant node edge success.", cube_node->GetName().c_str());

  if (ChangeDequantNodeEdge(graph, cube_node, dequant_node) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][QntRlbk] Cube node[%s]: change dequant node edge failed.",
                    cube_node->GetName().c_str());
    return FAILED;
  }
  FE_LOGD("Cube node[%s]: change dequant node edge success.", cube_node->GetName().c_str());
  return SUCCESS;
}

Status BiasOptimizeQuantRollbackBase::Fusion(ge::ComputeGraph &graph, fe::PatternFusionBasePass::Mapping &mapping,
                                             vector<ge::NodePtr> &fusion_nodes) {
  ge::NodePtr quant_node = GetNodeFromMapping(PATTERN_QUANT, mapping);
  ge::NodePtr dequant_node = GetNodeFromMapping(PATTERN_DEQUANT, mapping);
  ge::NodePtr cube_node = GetNodeFromMapping(PATTERN_CUBE, mapping);

  FE_CHECK(quant_node == nullptr,
           REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][Fus] quantNode is null, fusion failed."),
           return PARAM_INVALID);
  FE_CHECK(dequant_node == nullptr,
           REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][Fus] dequantNode is null, fusion failed."),
           return PARAM_INVALID);
  FE_CHECK(cube_node == nullptr,
           REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][Fus] cube_node is null, fusion failed."),
           return PARAM_INVALID);
  if (SetGroupInfoForAscendWeightQuantNodes(cube_node) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][Fus] set group info for AscendWeightQuant node failed.");
    return FAILED;
  }

  QuantProcessMode quant_process_mode;
  if (GetQuantProcessMode(quant_node, cube_node, quant_process_mode) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][Fus] Get quant process failed.");
    return FAILED;
  }
  if (quant_process_mode == QuantProcessMode::QUANT_UNDIFINED) {
    FE_LOGI("BiasOptimizeQuantRollback Pass.");
    return SUCCESS;
  }

  // bias optimize
  if (quant_process_mode == QuantProcessMode::BIAS_OPTIMIZE) {
    if (BiasOptimize(graph, cube_node, dequant_node, quant_node, fusion_nodes) != SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][Fus] Node[%s] bias optimize failed.",
                      cube_node->GetName().c_str());
      return FAILED;
    }
    FE_LOGD("Node[%s] bias optimize success.", cube_node->GetName().c_str());
    return SUCCESS;
  }

  // quant rollback
  if (QuantRollback(graph, cube_node, dequant_node, quant_node, fusion_nodes) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][AvgPolQntPcsFus][Fus] Node[%s] quant rollback failed.", cube_node->GetName().c_str());
    return FAILED;
  }

  if (find(fusion_nodes.begin(), fusion_nodes.end(), cube_node) == fusion_nodes.end()) {
    fusion_nodes.push_back(cube_node);
  }
  FE_LOGD("Node[%s] quant rollback success.", cube_node->GetName().c_str());
  return SUCCESS;
}
}  // namespace fe
