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

#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/quant_pass/tf_merge_conv2dbackpropinput_fusion_pass.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>
#include "common/configuration.h"
#include "common/fe_utils.h"
#include "common/util/op_info_util.h"
#include "external/graph/types.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/node_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/tensor_utils.h"
#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/quant_pass/quant_host_cpu_op_common.h"

#include "common/graph/fe_graph_utils.h"

namespace fe {
static const string OP_CONV2DBACKPROPINPUT = "Conv2DBackpropInput";

static const string OP_BIASADD = "BiasAdd";
static const string OP_QUANT = "AscendQuant";
static const string OP_SUB = "Sub";
static const string OP_WEIGHTQUANT = "AscendWeightQuant";
static const string OP_DEQUANT = "AscendDequant";

static const string PATN_QUANT = "quant";
static const string PATN_WEIGHTQUANT = "weightquant";
static const string PATN_DEQUANT = "dequant";
static const string PATN_CUBENODE = "cube_node";
static const string PATN_CONST = "Const";
static const string PATN_BIASADD = "biasadd";

static const string OP_CONST = "Const";

/* we only match 'with biasadd' and 'without biasadd' 2 kinds of pattern */
vector<FusionPattern *> TfMergeConv2DBackpropInputFusionPass::DefinePatterns() {
  vector<FusionPattern *> patterns;

  /*
   * ================================pattern0===================================
   *                       \     /             /
   *                     WeightQuant          /
   *                          |              /
   *                          V             /
   *  --> AscendQuant --> conv2dbpi --> BiasAdd --> AscendDequant -->
   *                          ^
   *                          |
   *                        const
   * ===========================================================================
   */
  FusionPattern *pattern0 = new (std::nothrow) FusionPattern("tfMergeConv2DBackpropInputFusion0");
  FE_CHECK(pattern0 == nullptr, FE_LOGW("new FusionPattern object failed!"), return patterns);
  /* Above defines ops that we need */
  pattern0->AddOpDesc(PATN_QUANT, {OP_QUANT})
      .AddOpDesc(PATN_WEIGHTQUANT, {OP_WEIGHTQUANT, OP_SUB})
      .AddOpDesc(PATN_CUBENODE, {OP_CONV2DBACKPROPINPUT})
      .AddOpDesc(PATN_BIASADD, {OP_BIASADD})
      .AddOpDesc(PATN_DEQUANT, {OP_DEQUANT})
      .AddOpDesc(PATN_CONST, {OP_CONST})
      .SetInputs(PATN_CUBENODE, {PATN_CONST, PATN_WEIGHTQUANT, PATN_QUANT})
      .SetInputs(PATN_BIASADD, {PATN_CUBENODE})
      .SetInputs(PATN_DEQUANT, {PATN_BIASADD})
      .SetOutput(PATN_DEQUANT);
  patterns.push_back(pattern0);

  /*
   * ================================pattern1===================================
   *                       \     /
   *                     WeightQuant
   *                          |
   *                          V
   *  --> AscendQuant --> conv2dbpi --> AscendDequant -->
   *                          ^
   *                          |
   *                        const
   * ===========================================================================
   */
  FusionPattern *pattern1 = new (std::nothrow) FusionPattern("tfMergeConv2DBackpropInputFusion1");
  FE_CHECK(pattern1 == nullptr, FE_LOGW("new FusionPattern object failed!"), return patterns);
  /* Above defines ops that we need */
  pattern1->AddOpDesc(PATN_QUANT, {OP_QUANT})
      .AddOpDesc(PATN_WEIGHTQUANT, {OP_WEIGHTQUANT, OP_SUB})
      .AddOpDesc(PATN_CUBENODE, {OP_CONV2DBACKPROPINPUT})
      .AddOpDesc(PATN_DEQUANT, {OP_DEQUANT})
      .AddOpDesc(PATN_CONST, {OP_CONST})
      .SetInputs(PATN_CUBENODE, {PATN_CONST, PATN_WEIGHTQUANT, PATN_QUANT})
      .SetInputs(PATN_DEQUANT, {PATN_CUBENODE})
      .SetOutput(PATN_DEQUANT);
  patterns.push_back(pattern1);

  return patterns;
}

/* add ConstValue to op_desc's Attr */
void TfMergeConv2DBackpropInputFusionPass::SetConstValueToAttr(ge::OpDescPtr op_desc, const ge::Tensor &const_tensor,
                                                               const ge::DataType &dtype, std::string attr_name) const {
  size_t size = 0;
  if (dtype == ge::DT_INT32) {
    const int32_t *const_data_ptr = reinterpret_cast<const int32_t *>(const_tensor.GetData());
    FE_CHECK(const_data_ptr == nullptr,
             REPORT_FE_ERROR("[GraphOpt][TfMrgCovBkPropInFus][SetCstValToAttr] const_data_ptr is null"), return;);
    size = const_tensor.GetSize() / sizeof(int32_t);
    if (size == 1) {
      int32_t const_data = static_cast<int32_t>(*const_data_ptr);
      (void)ge::AttrUtils::SetInt(op_desc, attr_name, const_data);
    }
    if (size > 1) {
      std::vector<int32_t> const_data;
      for (size_t i = 0; i < size; ++i) {
        const_data.push_back(static_cast<int32_t>(*(const_data_ptr + i)));
        FE_LOGD("Node:%s const data int32 proto %d", op_desc->GetName().c_str(),
                static_cast<int32_t>(*(const_data_ptr + i)));
      }
      ge::AttrUtils::SetListInt(op_desc, attr_name, const_data);
    }
  } else if (dtype == ge::DT_INT64) {
    const int64_t *const_data_ptr = reinterpret_cast<const int64_t *>(const_tensor.GetData());
    FE_CHECK(const_data_ptr == nullptr,
             REPORT_FE_ERROR("[GraphOpt][TfMrgCovBkPropInFus][SetCstValToAttr] const_data_ptr is null"), return;);
    size = const_tensor.GetSize() / sizeof(int64_t);
    if (size == 1) {
      int64_t const_data = static_cast<int64_t>(*const_data_ptr);
      ge::AttrUtils::SetInt(op_desc, attr_name, const_data);
    }
    if (size > 1) {
      std::vector<int64_t> const_data;
      for (size_t i = 0; i < size; ++i) {
        const_data.push_back((static_cast<int64_t>(*(const_data_ptr + i))));
        FE_LOGD("Node:%s const data int64 proto %ld", op_desc->GetName().c_str(),
                static_cast<int64_t>(*(const_data_ptr + i)));
      }
      ge::AttrUtils::SetListInt(op_desc, attr_name, const_data);
    }
  } else {
    FE_LOGW("%s not support %d", op_desc->GetName().c_str(), dtype);
  }
}

void TfMergeConv2DBackpropInputFusionPass::SetInputOpAndAttr(const ge::OpDescPtr &conv_op,
                                                             ge::OpDescPtr &Conv2DTransposeDOp) const {
  ge::GeTensorDesc Conv2DTransposeDInDesc0 = conv_op->GetInputDesc(2);
  ge::GeTensorDesc Conv2DTransposeDInDesc1 = conv_op->GetInputDesc(1);
  ge::GeTensorDesc Conv2DTransposeDInDesc2(ge::GeShape(), ge::FORMAT_RESERVED, ge::DT_UNDEFINED);
  ge::GeTensorDesc Conv2DTransposeDInDesc3(ge::GeShape(), ge::FORMAT_RESERVED, ge::DT_UNDEFINED);
  Conv2DTransposeDInDesc0.SetDataType(conv_op->GetInputDesc(2).GetDataType());
  Conv2DTransposeDInDesc0.SetOriginDataType(conv_op->GetInputDesc(2).GetOriginDataType());
  Conv2DTransposeDInDesc0.SetFormat(conv_op->GetInputDesc(2).GetFormat());
  Conv2DTransposeDInDesc0.SetOriginFormat(
      static_cast<ge::Format>(ge::GetPrimaryFormat(conv_op->GetInputDesc(2).GetFormat())));
  Conv2DTransposeDInDesc1.SetDataType(conv_op->GetInputDesc(1).GetDataType());
  Conv2DTransposeDInDesc1.SetOriginDataType(conv_op->GetInputDesc(1).GetOriginDataType());
  Conv2DTransposeDInDesc1.SetFormat(conv_op->GetInputDesc(1).GetFormat());
  Conv2DTransposeDInDesc1.SetOriginFormat(
      static_cast<ge::Format>(ge::GetPrimaryFormat(conv_op->GetInputDesc(1).GetFormat())));

  if (Conv2DTransposeDOp->AddInputDesc("x", Conv2DTransposeDInDesc0) != ge::GRAPH_SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][TfMrgCovBkPropInFus][SetInOpAttr] add input x for Conv2DTransposeDInDesc1 fail.");
    return;
  }
  if (Conv2DTransposeDOp->AddInputDesc("filter", Conv2DTransposeDInDesc1) != ge::GRAPH_SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][TfMrgCovBkPropInFus][SetInOpAttr] add input filter for Conv2DTransposeDInDesc1 fail.");
    return;
  }
  if (Conv2DTransposeDOp->AddInputDesc("bias", Conv2DTransposeDInDesc2) != ge::GRAPH_SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][TfMrgCovBkPropInFus][SetInOpAttr] add input bias for Conv2DTransposeDInDesc2 fail.");
    return;
  }
  if (Conv2DTransposeDOp->AddInputDesc("offset_w", Conv2DTransposeDInDesc3) != ge::GRAPH_SUCCESS) {
    REPORT_FE_ERROR(
        "[GraphOpt][TfMrgCovBkPropInFus][SetInOpAttr] add input offset_w for Conv2DTransposeDInDesc2 fail.");
    return;
  }

  std::vector<int64_t> strides_index;
  if (ge::AttrUtils::GetListInt(conv_op, "strides", strides_index)) {
    ge::AttrUtils::SetListInt(Conv2DTransposeDOp, "strides", strides_index);
  }
  std::vector<int64_t> pads_index;
  if (ge::AttrUtils::GetListInt(conv_op, "pads", pads_index)) {
    ge::AttrUtils::SetListInt(Conv2DTransposeDOp, "pads", pads_index);
  }
  std::vector<int64_t> dilations_index;
  if (ge::AttrUtils::GetListInt(conv_op, "dilations", dilations_index)) {
    ge::AttrUtils::SetListInt(Conv2DTransposeDOp, "dilations", dilations_index);
  }
  std::vector<int64_t> groups_index;
  if (ge::AttrUtils::GetListInt(conv_op, "groups", groups_index)) {
    ge::AttrUtils::SetListInt(Conv2DTransposeDOp, "groups", groups_index);
  }
  string data_format_index;
  if (ge::AttrUtils::GetStr(conv_op, "data_format", data_format_index)) {
    (void)ge::AttrUtils::SetStr(Conv2DTransposeDOp, "data_format", data_format_index);
  }
}

Status TfMergeConv2DBackpropInputFusionPass::RemoveConstNode(ge::ComputeGraph &graph, const ge::NodePtr &conv_node)
    const {
  ge::NodePtr const_node = nullptr;
  /* we need forward to uplevel nodes to find const node */
  for (auto node_up : conv_node->GetInAllNodes()) {
    if (node_up->GetType() == OP_CONST) {
      const_node = node_up;
      break;
    }
  }
  for (auto out_anchor : const_node->GetAllOutDataAnchors()) {
    out_anchor->UnlinkAll();
  }
  if (graph.RemoveNode(const_node) != ge::GRAPH_SUCCESS) {
    return FAILED;
  }
  return SUCCESS;
}

Status TfMergeConv2DBackpropInputFusionPass::ConnectEdges(const ge::NodePtr &conv_node,
                                                          const ge::NodePtr &Conv2DTransposeD) const {
  if (ge::GraphUtils::AddEdge(conv_node->GetInDataAnchor(1)->GetPeerOutAnchor(),
                              Conv2DTransposeD->GetInDataAnchor(1)) != SUCCESS) {
    return FAILED;
  }
  int sub_index = 2;
  if (ge::GraphUtils::AddEdge(conv_node->GetInDataAnchor(sub_index)->GetPeerOutAnchor(),
                              Conv2DTransposeD->GetInDataAnchor(0)) != SUCCESS) {
    return FAILED;
  }
  ge::NodePtr t_node = conv_node;
  auto in_anchors = t_node->GetOutDataAnchor(0)->GetPeerInDataAnchors();
  for (auto in_anchor : in_anchors) {
    in_anchor->UnlinkAll();
    if (ge::GraphUtils::AddEdge(Conv2DTransposeD->GetOutDataAnchor(0), in_anchor) != SUCCESS) {
      return FAILED;
    }
  }
  return SUCCESS;
}

Status TfMergeConv2DBackpropInputFusionPass::Conv2DBPIToConv2DTD(ge::ComputeGraph &graph, Mapping &mapping,
                                                                 vector<ge::NodePtr> &fusion_nodes) const {
  FE_LOGD("Start to do TfMergeConv2DBackpropInputFusionPass::Fusion.");
  ge::NodePtr conv_node = GetNodeFromMapping(PATN_CUBENODE, mapping);
  FE_LOGD("Merge sub graph cube op [name:%s].", conv_node->GetName().c_str());

  // build a new node named Conv2DTransposeD
  auto conv_op = conv_node->GetOpDesc();
  ge::OpDescPtr Conv2DTransposeDOp = nullptr;
  FE_MAKE_SHARED(Conv2DTransposeDOp = std::make_shared<ge::OpDesc>(conv_op->GetName(), "Conv2DTransposeD"),
                 return FAILED);

  /*
   * ===========================================================================
   *                 weightquant(1)
   *                      |
   *                      V
   * input_size(0) --> Conv2DBackpropInput <-- quant(2)
   *
   *                 weightquant(1)
   *                      |
   *                      V
   * quant(0) --> Conv2DTransposeD
   *                  (input_size)
   *
   * ===========================================================================
   */
  // Set Conv2DTransposeDInputOp and attr
  SetInputOpAndAttr(conv_op, Conv2DTransposeDOp);

  ge::GeTensorDesc Conv2DTransposeDOutDesc0 = conv_op->GetOutputDesc(0);
  Conv2DTransposeDOutDesc0.SetDataType(ge::DT_INT32);
  Conv2DTransposeDOutDesc0.SetOriginDataType(ge::DT_INT32);
  Conv2DTransposeDOutDesc0.SetShape(conv_op->GetOutputDesc(0).GetShape());
  Conv2DTransposeDOutDesc0.SetOriginShape(conv_op->GetOutputDesc(0).GetShape());
  Conv2DTransposeDOutDesc0.SetFormat(conv_op->GetOutputDesc(0).GetFormat());
  Conv2DTransposeDOutDesc0.SetOriginFormat(
      static_cast<ge::Format>(ge::GetPrimaryFormat(conv_op->GetOutputDesc(0).GetFormat())));
  if (Conv2DTransposeDOp->AddOutputDesc("y", Conv2DTransposeDOutDesc0) != ge::GRAPH_SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][TfMrgCovBkPropInFus][CovBPIToCovTD] add output y1 for Conv2DTransposeDOp fail.");
    return FAILED;
  }

  ge::NodePtr Conv2DTransposeD = graph.AddNode(Conv2DTransposeDOp);
  FE_CHECK(Conv2DTransposeD == nullptr,
           REPORT_FE_ERROR("[GraphOpt][TfMrgCovBkPropInFus][CovBPIToCovTD] %s is nullptr",
                           Conv2DTransposeDOp->GetName().c_str()),
           return PARAM_INVALID);
  fusion_nodes.push_back(Conv2DTransposeD);

  // set Conv2DBackpropInput's input_size  as Conv2DTransposeD's attr
  ge::Tensor const_tensor;
  ge::Operator op = ge::OpDescUtils::CreateOperatorFromNode(conv_node);
  op.GetInputConstData(conv_op->GetInputNameByIndex(0).c_str(), const_tensor);
  SetConstValueToAttr(Conv2DTransposeDOp, const_tensor, conv_op->GetInputDesc(0).GetDataType(), "input_size");

  if (RemoveConstNode(graph, conv_node) != SUCCESS) {
    return FAILED;
  }
  if (ConnectEdges(conv_node, Conv2DTransposeD) != SUCCESS) {
    return FAILED;
  }

  // delete Conv2DBackpropInput
  for (auto in_anchor : conv_node->GetAllInDataAnchors()) {
    if (in_anchor != nullptr) {
      in_anchor->UnlinkAll();
    }
  }

  if (graph.RemoveNode(conv_node) == ge::GRAPH_FAILED) {
    REPORT_FE_ERROR("[GraphOpt][TfMrgCovBkPropInFus][CovBPIToCovTD] convNode remove failed");
    return FAILED;
  }

  return SUCCESS;
}

Status TfMergeConv2DBackpropInputFusionPass::Fusion(ge::ComputeGraph &graph, Mapping &mapping,
                                                    vector<ge::NodePtr> &fusion_nodes) {
  FE_LOGD("start to fusion Conv2DBackpropInput");
  if (Conv2DBPIToConv2DTD(graph, mapping, fusion_nodes) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][TfMrgCovBkPropInFus][Fus] Merge Conv2DBackpropInput to Conv2DTransposeD failed");
    return FAILED;
  }
  return SUCCESS;
}

}  // namespace fe
