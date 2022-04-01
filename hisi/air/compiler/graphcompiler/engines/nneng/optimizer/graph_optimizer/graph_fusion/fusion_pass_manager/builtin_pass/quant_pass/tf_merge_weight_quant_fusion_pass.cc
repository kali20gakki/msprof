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

#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/quant_pass/tf_merge_weight_quant_fusion_pass.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>
#include "common/configuration.h"
#include "common/fe_utils.h"
#include "common/graph/fe_graph_utils.h"
#include "common/util/op_info_util.h"
#include "external/graph/types.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/node_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/tensor_utils.h"
#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/quant_pass/quant_host_cpu_op_common.h"

namespace fe {
static const string OP_CONV = "Conv2D";
static const string OP_MATMULV2 = "MatMulV2";
static const string OP_DECONV = "Deconvolution";
static const string OP_DEPTHWISE = "DepthwiseConv2D";
static const string OP_CONV2DTD = "Conv2DTransposeD";
static const string OP_BATCHMATMULV2 = "BatchMatMulV2";

static const string OP_PAD = "Pad";
static const string OP_BIASADD = "BiasAdd";
static const string OP_ADD = "Add";
static const string OP_QUANT = "AscendQuant";
// [begin] to be delete
static const string OP_SUB = "Sub";
// [end] to be delete
static const string OP_WEIGHTQUANT = "AscendWeightQuant";
static const string OP_DEQUANT = "AscendDequant";
static const string OP_SPLIT = "Split";
static const string OP_CONCATV2 = "ConcatV2";
static const string OP_CONCAT = "Concat";

static const string PATN_QUANT = "quant";
static const string PATN_WEIGHTQUANT = "weightquant";
static const string PATN_DEQUANT = "dequant";
static const string PATN_CUBENODE = "cube_node";
static const string PATN_BIASADD = "biasadd";
static const string PATN_PAD = "pad";
static const string PATN_SPLIT = "split";
static const string PATN_CONCATV2 = "concatv2";

static const string OP_CONST = "Const";
static const string OP_IDENTITY = "Identity";

const uint32_t kLinkIndex = 2;
const size_t kInputSize = 3;

vector<FusionPattern *> TfMergeWeightQuantFusionPass::DefinePatterns() {
  vector<FusionPattern *> patterns;

  /*
   * ================================pattern0===================================
   *                       \     /
   *                     weightquant
   *                          |
   *                          V
   *  --> AscendQuant --> CUBE_NODE --> AscendDequant -->
   *
   * ===========================================================================
   */
  FusionPattern *pattern0 = new (std::nothrow) FusionPattern("tfMergeWeightQuantFusion0");
  FE_CHECK(pattern0 == nullptr,
           REPORT_FE_ERROR("[GraphOpt][TfMrgSubFus][DefPtn] new FusionPattern object failed!"), return patterns);
  /* Above defines ops that we need */
  pattern0->AddOpDesc(PATN_QUANT, {OP_QUANT})
      .AddOpDesc(PATN_WEIGHTQUANT, {OP_WEIGHTQUANT, OP_SUB})
      .AddOpDesc(PATN_CUBENODE, {OP_CONV, OP_MATMULV2, OP_DECONV, OP_DEPTHWISE, OP_CONV2DTD})
      .AddOpDesc(PATN_DEQUANT, {OP_DEQUANT})
      .SetInputs(PATN_CUBENODE, {PATN_QUANT, PATN_WEIGHTQUANT})
      .SetInputs(PATN_DEQUANT, {PATN_CUBENODE})
      .SetOutput(PATN_DEQUANT);
  patterns.push_back(pattern0);

  /*
   * ================================pattern1===================================
   *                       \     /
   *                     WeightQuant        Const
   *                          |              /
   *                          V             /
   *  --> AscendQuant --> CUBE_NODE --> BiasAdd --> AscendDequant -->
   *
   * ===========================================================================
   */
  FusionPattern *pattern1 = new (std::nothrow) FusionPattern("tfMergeWeightQuantFusion1");
  FE_CHECK(pattern1 == nullptr, REPORT_FE_ERROR("[GraphOpt][TfMrgSubFus][DefPtn] new FusionPattern object failed!"),
           return patterns);
  /* Above defines ops that we need */
  pattern1->AddOpDesc(PATN_QUANT, {OP_QUANT})
      .AddOpDesc(PATN_WEIGHTQUANT, {OP_WEIGHTQUANT, OP_SUB})
      .AddOpDesc(PATN_CUBENODE, {OP_CONV, OP_MATMULV2, OP_DECONV, OP_DEPTHWISE, OP_CONV2DTD})
      .AddOpDesc(PATN_BIASADD, {OP_BIASADD})
      .AddOpDesc(PATN_DEQUANT, {OP_DEQUANT})
      .SetInputs(PATN_CUBENODE, {PATN_QUANT, PATN_WEIGHTQUANT})
      .SetInputs(PATN_BIASADD, {PATN_CUBENODE})
      .SetInputs(PATN_DEQUANT, {PATN_BIASADD})
      .SetOutput(PATN_DEQUANT);
  patterns.push_back(pattern1);

  /*
   * ================================pattern2===================================
   *                           \     /
   *                           weightquant
   *                              |
   *                              V
   *  --> AscendQuant -->Pad -> Conv2D --> AscendDequant -->
   *
   * ===========================================================================
   */
  FusionPattern *pattern2 = new (std::nothrow) FusionPattern("tfMergeWeightQuantFusion2");
  FE_CHECK(pattern2 == nullptr, REPORT_FE_ERROR("[GraphOpt][TfMrgSubFus][DefPtn] new FusionPattern object failed!"),
           return patterns);
  /* Above defines ops that we need */
  pattern2->AddOpDesc(PATN_QUANT, {OP_QUANT})
      .AddOpDesc(PATN_WEIGHTQUANT, {OP_WEIGHTQUANT, OP_SUB})
      .AddOpDesc(PATN_PAD, {OP_PAD})
      .AddOpDesc(PATN_CUBENODE, {OP_CONV, OP_DEPTHWISE})
      .AddOpDesc(PATN_DEQUANT, {OP_DEQUANT})
      .SetInputs(PATN_PAD, {PATN_QUANT})
      .SetInputs(PATN_CUBENODE, {PATN_PAD, PATN_WEIGHTQUANT})
      .SetInputs(PATN_DEQUANT, {PATN_CUBENODE})
      .SetOutput(PATN_DEQUANT);
  patterns.push_back(pattern2);

  /*
   * ================================pattern3===================================
   *                           \     /
   *                         weightquant        Const
   *                              |              /
   *                              V             /
   *  --> AscendQuant -->Pad -> Conv2D --> BiasAdd --> AscendDequant -->
   *
   * ===========================================================================
   */
  FusionPattern *pattern3 = new (std::nothrow) FusionPattern("tfMergeWeightQuantFusion3");
  FE_CHECK(pattern3 == nullptr, REPORT_FE_ERROR("[GraphOpt][TfMrgSubFus][DefPtn] new FusionPattern object failed!"),
           return patterns);
  /* Above defines ops that we need */
  pattern3->AddOpDesc(PATN_QUANT, {OP_QUANT})
      .AddOpDesc(PATN_WEIGHTQUANT, {OP_WEIGHTQUANT, OP_SUB})
      .AddOpDesc(PATN_PAD, {OP_PAD})
      .AddOpDesc(PATN_CUBENODE, {OP_CONV, OP_DEPTHWISE})
      .AddOpDesc(PATN_BIASADD, {OP_BIASADD})
      .AddOpDesc(PATN_DEQUANT, {OP_DEQUANT})
      .SetInputs(PATN_PAD, {PATN_QUANT})
      .SetInputs(PATN_CUBENODE, {PATN_PAD, PATN_WEIGHTQUANT})
      .SetInputs(PATN_BIASADD, {PATN_CUBENODE})
      .SetInputs(PATN_DEQUANT, {PATN_BIASADD})
      .SetOutput(PATN_DEQUANT);
  patterns.push_back(pattern3);

  /*
   * ================================pattern4===================================
   *                           \     /
   *                         weightquant     Const
   *                              |          /
   *                              V         /
   *  AscendQuant -->Split -> Conv2D --> BiasAdd --> ConcatV2 --> AscendDequant
   *
   * ===========================================================================
   */
  FusionPattern *pattern4 = new (std::nothrow) FusionPattern("tfMergeWeightQuantFusion4");
  FE_CHECK(pattern4 == nullptr, REPORT_FE_ERROR("[GraphOpt][TfMrgSubFus][DefPtn] new FusionPattern object failed!"),
           return patterns);
  /* Above defines ops that we need */
  pattern4->AddOpDesc(PATN_QUANT, {OP_QUANT})
      .AddOpDesc(PATN_SPLIT, {OP_SPLIT})
      .AddOpDesc(PATN_WEIGHTQUANT, {OP_WEIGHTQUANT, OP_SUB})
      .AddOpDesc(PATN_CUBENODE, {OP_CONV})
      .AddOpDesc(PATN_BIASADD, {OP_BIASADD})
      .AddOpDesc(PATN_CONCATV2, {OP_CONCATV2, OP_CONCAT})
      .AddOpDesc(PATN_DEQUANT, {OP_DEQUANT})
      .SetInputs(PATN_SPLIT, {PATN_QUANT})
      .SetInputs(PATN_CUBENODE, {PATN_SPLIT, PATN_WEIGHTQUANT})
      .SetInputs(PATN_BIASADD, {PATN_CUBENODE})
      .SetInputs(PATN_CONCATV2, {PATN_BIASADD})
      .SetInputs(PATN_DEQUANT, {PATN_CONCATV2})
      .SetOutput(PATN_DEQUANT);
  patterns.push_back(pattern4);

  /*
   * ================================pattern5===================================
   *                           \     /
   *                         weightquant
   *                              |
   *                              V
   *  AscendQuant -->Split -> Conv2D --> ConcatV2 --> AscendDequant
   *
   * ===========================================================================
   */
  FusionPattern *pattern5 = new (std::nothrow) FusionPattern("tfMergeWeightQuantFusion5");
  FE_CHECK(pattern5 == nullptr, REPORT_FE_ERROR("[GraphOpt][TfMrgSubFus][DefPtn] new FusionPattern object failed!"),
           return patterns);
  /* Above defines ops that we need */
  pattern5->AddOpDesc(PATN_QUANT, {OP_QUANT})
      .AddOpDesc(PATN_SPLIT, {OP_SPLIT})
      .AddOpDesc(PATN_WEIGHTQUANT, {OP_WEIGHTQUANT, OP_SUB})
      .AddOpDesc(PATN_CUBENODE, {OP_CONV})
      .AddOpDesc(PATN_CONCATV2, {OP_CONCATV2, OP_CONCAT})
      .AddOpDesc(PATN_DEQUANT, {OP_DEQUANT})
      .SetInputs(PATN_SPLIT, {PATN_QUANT})
      .SetInputs(PATN_CUBENODE, {PATN_SPLIT, PATN_WEIGHTQUANT})
      .SetInputs(PATN_CONCATV2, {PATN_CUBENODE})
      .SetInputs(PATN_DEQUANT, {PATN_CONCATV2})
      .SetOutput(PATN_DEQUANT);
  patterns.push_back(pattern5);

  /*
   * ================================pattern6===================================
   *
   *                        Const           Const
   *                          |              /
   *                          V             /
   *  --> AscendQuant --> CUBE_NODE --> Add --> AscendDequant -->
   *
   * ===========================================================================
   */
  FusionPattern *pattern6 = new (std::nothrow) FusionPattern("tfMergeWeightQuantFusion6");
  FE_CHECK(pattern6 == nullptr, REPORT_FE_ERROR("[GraphOpt][TfMrgSubFus][DefPtn] new FusionPattern object failed!"),
           return patterns);
  /* Above defines ops that we need */
  pattern6->AddOpDesc(PATN_QUANT, {OP_QUANT})
          .AddOpDesc(PATN_CUBENODE, {OP_BATCHMATMULV2})
          .AddOpDesc(PATN_BIASADD, {OP_ADD})
          .AddOpDesc(PATN_DEQUANT, {OP_DEQUANT})
          .SetInputs(PATN_CUBENODE, {PATN_QUANT})
          .SetInputs(PATN_BIASADD, {PATN_CUBENODE})
          .SetInputs(PATN_DEQUANT, {PATN_BIASADD})
          .SetOutput(PATN_DEQUANT);
  patterns.push_back(pattern6);
  return patterns;
}

static Status RemoveNodeIncConstNode(ge::ComputeGraph &graph, ge::NodePtr &node) {
  for (unsigned int node_up_idx = 0; node_up_idx < node->GetInAllNodes().size(); node_up_idx++) {
    auto node_up = node->GetInAllNodes().at(node_up_idx);
    string const_op_type;
    bool up_const_flag = ge::NodeUtils::GetConstOpType(node_up, const_op_type);
    if (node_up->GetType() == OP_IDENTITY) {
      for (unsigned int node_up_up_idx = 0; node_up_up_idx < node_up->GetInAllNodes().size(); node_up_up_idx++) {
        auto node_up_up = node_up->GetInAllNodes().at(node_up_up_idx);
        bool up_upconst_flag = ge::NodeUtils::GetConstOpType(node_up_up, const_op_type);
        if (up_upconst_flag) {
          if (ge::GraphUtils::RemoveEdge(node_up_up->GetOutDataAnchor(0), node_up->GetInDataAnchor(node_up_up_idx))
              != SUCCESS) {
            REPORT_FE_ERROR("[GraphOpt][TfMrgSubFus][RmNdIncCstNd] remove edge from src node[%s] to dst node[%s] \
                            failed.AnchorIdx : 0 ", node_up_up->GetName().c_str(), node_up->GetName().c_str());
            return FAILED;
          }
        }
      }
      if (graph.RemoveNode(node_up) != ge::GRAPH_SUCCESS) {
        REPORT_FE_ERROR("[GraphOpt][TfMrgSubFus][RmNdIncCstNd] remove node %s failed", node_up->GetName().c_str());
        return FAILED;
      }
    } else if (up_const_flag) {
      if (node_up->GetOutDataAnchor(0)->Unlink(node->GetInDataAnchor(node_up_idx)) != ge::GRAPH_SUCCESS) {
        FE_LOGD("remove edge from src node[%s] to dst node[%s] AnchorIdx:[%d] not success", node_up->GetName().c_str(),
                node->GetName().c_str(), node_up_idx);
      }
    } else {
      continue;
    }
  }

  if (graph.RemoveNode(node) != ge::GRAPH_SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][TfMrgSubFus][RmNdIncCstNd] remove node %s failed", node->GetName().c_str());
    return FAILED;
  }

  return SUCCESS;
}

Status TfMergeWeightQuantFusionPass::remove_nodes_from_map(ge::ComputeGraph &graph,
                                                           vector<ge::NodePtr> &del_nodes) const {
  for (auto node : del_nodes) {
    if (node == nullptr) {
      return FAILED;
    }
    if (node->GetOutDataNodes().size() != 0) {
      continue;
    }
    if (RemoveNodeIncConstNode(graph, node) != ge::GRAPH_SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][TfMrgSubFus][RmNdFromMap] Remove node[%s] failed", node->GetName().c_str());
      return FAILED;
    }
  }
  return SUCCESS;
}

static ge::NodePtr GetConstNode(ge::NodePtr &node) {
  ge::NodePtr const_node = nullptr;
  /* we need forward to uplevel nodes to find const node */
  string const_op_type;
  bool const_flag = ge::NodeUtils::GetConstOpType(node, const_op_type);
  if (!const_flag) {
    for (auto node_up : node->GetInAllNodes()) {
      bool up_const_flag = ge::NodeUtils::GetConstOpType(node_up, const_op_type);
      if (node_up->GetType() == OP_IDENTITY) {
        for (auto node_up_up : node_up->GetInAllNodes()) {
          bool up_up_const_flag = ge::NodeUtils::GetConstOpType(node_up_up, const_op_type);
          if (up_up_const_flag) {
            const_node = node_up_up;
            break;
          }
        }
      } else if (up_const_flag) {
        const_node = node_up;
        break;
      }
    }
  } else {
    const_node = node;
  }
  return const_node;
}

Status TfMergeWeightQuantFusionPass::MergeWeight(ge::ComputeGraph &graph, const ge::NodePtr &cube_node,
                                                 const ge::NodePtr &weight_quant_node) const {
  // if weight quant node is nullptr
  if (weight_quant_node == nullptr) {
    if (cube_node->GetInDataNodes().size() >= 2) {
      ge::NodePtr weight_node = cube_node->GetInDataNodes().at(1);
      auto weight_desc_ptr = cube_node->GetOpDesc()->MutableInputDesc(1);
      weight_desc_ptr->SetFormat(ge::FORMAT_HWCN);
      weight_desc_ptr->SetOriginFormat(ge::FORMAT_HWCN);
      auto const_desc_ptr = weight_node->GetOpDesc()->MutableOutputDesc(0);
      const_desc_ptr->SetFormat(ge::FORMAT_HWCN);
      const_desc_ptr->SetOriginFormat(ge::FORMAT_HWCN);
      return SUCCESS;
    } else {
      REPORT_FE_ERROR("[GraphOpt][TfMrgSubFus][MrgWgt] Cube node[%s] pattern is not support, check model.",
                      cube_node->GetName().c_str());
      return FAILED;
    }
  }
  // [begin] to be delete
  std::string op_type = weight_quant_node->GetType();
  if (op_type == "Sub") {
    FE_LOGD("start to merge weight.");

    ge::NodePtr bias_node = GetConstNode(weight_quant_node->GetInAllNodes().at(0));
    FE_CHECK(bias_node == nullptr,
             REPORT_FE_ERROR("[GraphOpt][TfMrgSubFus][MrgWgt] get sub_node's first const node failed."),
    return INTERNAL_ERROR);

    // update weight op_desc
    ge::Format weight_format = ge::FORMAT_HWCN;
    ge::GeTensorDesc tensor_desc = bias_node->GetOpDesc()->GetOutputDesc(0);
    if (cube_node->GetType() == MATMULV2OP) {
      tensor_desc.SetFormat(weight_format);
      tensor_desc.SetOriginFormat(weight_format);
    }
    if (bias_node->GetOpDesc()->UpdateOutputDesc(0, tensor_desc) != ge::GRAPH_SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][TfMrgSubFus][MrgWgt]  update first output of op [name: %s, type: %s] failed.",
                      bias_node->GetName().c_str(), bias_node->GetType().c_str());
      return FAILED;
    }
    if (cube_node->GetOpDesc()->UpdateInputDesc(1, tensor_desc) != ge::GRAPH_SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][TfMrgSubFus][MrgWgt]  update second input of op [name: %s, type: %s] failed.",
                      cube_node->GetName().c_str(), cube_node->GetType().c_str());
      return FAILED;
    }

    // remove link between cube and sub
    auto in_data_anchor = cube_node->GetInDataAnchor(1);
    FE_CHECK(in_data_anchor == nullptr, REPORT_FE_ERROR("[GraphOpt][TfMrgSubFus][MrgWgt] in_data_anchor is null"),
    return FAILED);
    auto pre_out_data_anchor = in_data_anchor->GetPeerOutAnchor();
    FE_CHECK(pre_out_data_anchor == nullptr,
             REPORT_FE_ERROR("[GraphOpt][TfMrgSubFus][MrgWgt] pre_out_data_anchor is null"), return FAILED);
    if (ge::GraphUtils::RemoveEdge(pre_out_data_anchor, in_data_anchor) != ge::GRAPH_SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][TfMrgSubFus][MrgWgt] Node[%s] remove outputdata edge error.",
                      pre_out_data_anchor->GetOwnerNode()->GetName().c_str());
      return FAILED;
    }

    // link const node
    if (ge::GraphUtils::AddEdge(bias_node->GetOutDataAnchor(0), cube_node->GetInDataAnchor(1)) != SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][TfMrgSubFus][MrgWgt]  add edge from src node[%s] to dst node[%s] failed.",
                      bias_node->GetName().c_str(), cube_node->GetName().c_str());
      return FAILED;
    }

    // remove nodes
    vector<ge::NodePtr> del_nodes;
    del_nodes.push_back(weight_quant_node);
    if (remove_nodes_from_map(graph, del_nodes) != SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][TfMrgSubFus][MrgWgt] removeNodesFromMap failed.");
      return FAILED;
    }

    FE_LOGD("merge weight end.");
    return SUCCESS;
  }
  // [end] to be delete

  FE_LOGD("start to merge weight.");
  auto all_in_nodes = weight_quant_node->GetInDataNodes();
  FE_CHECK(all_in_nodes.size() < 2,
           REPORT_FE_ERROR("[GraphOpt][TfMrgSubFus][MrgWgt] the number of input nodes for weight quant [%s] should be greater than 2.",
                            weight_quant_node->GetName().c_str()),
          return INTERNAL_ERROR);
  ge::NodePtr weight_const_node = GetConstNode(all_in_nodes.at(0));
  FE_CHECK(weight_const_node == nullptr,
           REPORT_FE_ERROR("[GraphOpt][TfMrgSubFus][MrgWgt] get sub_node's first const node failed."),
           return INTERNAL_ERROR);
  ge::NodePtr offset_const_node = GetConstNode(all_in_nodes.at(1));
  FE_CHECK(offset_const_node == nullptr,
           REPORT_FE_ERROR("[GraphOpt][TfMrgSubFus][MrgWgt] get sub_node's second const node failed."),
           return INTERNAL_ERROR);

  ge::Format weight_format = ge::FORMAT_HWCN;
  auto weight_tensor_desc = weight_const_node->GetOpDesc()->MutableOutputDesc(0);
  auto offset_tensor_desc = offset_const_node->GetOpDesc()->MutableOutputDesc(0);
  if (cube_node->GetType() == MATMULV2OP) {
    // update weight_const and offset_const op_desc
    weight_tensor_desc->SetFormat(weight_format);
    weight_tensor_desc->SetOriginFormat(weight_format);
    offset_tensor_desc->SetFormat(weight_format);
    offset_tensor_desc->SetOriginFormat(weight_format);
    // update ascend_weight_quant
    auto tensor_desc_ptr0 = weight_quant_node->GetOpDesc()->MutableInputDesc(0);
    auto tensor_desc_ptr1 = weight_quant_node->GetOpDesc()->MutableInputDesc(1);
    auto tensor_desc_ptr2 = weight_quant_node->GetOpDesc()->MutableOutputDesc(0);
    tensor_desc_ptr0->SetFormat(weight_format);
    tensor_desc_ptr0->SetOriginFormat(weight_format);
    tensor_desc_ptr1->SetFormat(weight_format);
    tensor_desc_ptr1->SetOriginFormat(weight_format);
    tensor_desc_ptr2->SetFormat(weight_format);
    tensor_desc_ptr2->SetOriginFormat(weight_format);
  }

  // update cube filter input
  auto cube_filter_input_tensor_desc_ptr = cube_node->GetOpDesc()->MutableInputDesc(1);
  cube_filter_input_tensor_desc_ptr->SetFormat(weight_tensor_desc->GetFormat());
  cube_filter_input_tensor_desc_ptr->SetOriginFormat(weight_tensor_desc->GetOriginFormat());

  FE_LOGD("merge weight end.");
  return SUCCESS;
}

Status TfMergeWeightQuantFusionPass::MergeBiasWithCast(const ge::NodePtr &cube_node,
                                                       const ge::NodePtr &cast_node) const {
  FE_CHECK(cast_node->GetInAllNodes().size() == 0, REPORT_FE_ERROR("cast node[%s] don't have input!",
    cast_node->GetName().c_str()), return FAILED);
  ge::NodePtr bias_node = GetConstNode(cast_node->GetInAllNodes().at(0));
  FE_CHECK(bias_node == nullptr, REPORT_FE_ERROR("[GraphOpt][TfMrgSubFus][MrgBiasCast] get bias node failed."),
           return FAILED);

  // link const node
  ge::OpDescPtr conv_op_desc = cube_node->GetOpDesc();
  (void)conv_op_desc->UpdateInputDesc("bias", bias_node->GetOpDesc()->GetOutputDesc(0));
  (void)cube_node->UpdateOpDesc(conv_op_desc);
  if (cube_node->AddLinkFrom(2, bias_node) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][TfMrgSubFus][MrgBiasCast]  add edge from src node[%s] to dst node[%s] failed.",
                    cube_node->GetName().c_str(), cube_node->GetName().c_str());
    return FAILED;
  }
  return SUCCESS;
}

Status TfMergeWeightQuantFusionPass::MergeBiasNoCast(const ge::NodePtr &cube_node,
    const ge::NodePtr &bias_const, bool is_bias_and_cube_direct_link) const {
  ge::OpDescPtr const_desc = bias_const->GetOpDesc();
  // if output datatype is fp32, get data from const node and cast to int32
  if (const_desc->GetOutputDesc(0).GetDataType() == ge::DT_FLOAT) {
    FE_LOGD("Cast node[%s] dtype from fp32 to int32.", const_desc->GetName().c_str());
    ge::GeTensorPtr const_tensor = nullptr;
    (void)ge::AttrUtils::MutableTensor(bias_const->GetOpDesc(), "value", const_tensor);
    if (const_tensor == nullptr) {
      REPORT_FE_ERROR("[GraphOpt][TfMrgSubFus][MrgBiasNoCast] Get data pointer of node[%s] failed.",
                      bias_const->GetName().c_str());
      return FAILED;
    }
    // cast float data to int32_t
    const float *float_data = reinterpret_cast<const float *>(const_tensor->GetData().GetData());
    FE_CHECK_NOTNULL(float_data);
    size_t data_size = const_tensor->GetData().GetSize() / sizeof(float);
    std::unique_ptr<int32_t[]> int_data(new (std::nothrow) int32_t[data_size]());
    FE_CHECK_NOTNULL(int_data);
    for (size_t i = 0; i < data_size; i++) {
      int_data[i] = static_cast<int32_t>(float_data[i]);
    }
    if (const_tensor->SetData(reinterpret_cast<uint8_t *>(int_data.get()), data_size * sizeof(int32_t)) !=
        ge::GRAPH_SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][TfMrgSubFus][MrgBiasNoCast] Set data to const node[%s] failed.",
          bias_const->GetName().c_str());
      return FAILED;
    }
    // update const node output desc and link const node
    const_desc->MutableOutputDesc(0)->SetDataType(ge::DT_INT32);
    const_desc->MutableOutputDesc(0)->SetOriginDataType(ge::DT_INT32);
  }

  ge::OpDescPtr conv_op_desc = cube_node->GetOpDesc();
  (void)conv_op_desc->UpdateInputDesc("bias", bias_const->GetOpDesc()->GetOutputDesc(0));
  (void)cube_node->UpdateOpDesc(conv_op_desc);
  if (!is_bias_and_cube_direct_link) {
    if (cube_node->AddLinkFrom(kLinkIndex, bias_const) != SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][TfMrgSubFus][MrgBiasNoCast]  add edge from src node[%s] to dst node[%s] failed.",
                      bias_const->GetName().c_str(), cube_node->GetName().c_str());
      return FAILED;
    }
  }

  return SUCCESS;
}

Status TfMergeWeightQuantFusionPass::MergeBias(ge::ComputeGraph &graph,
                                               ge::NodePtr &cube_node, const ge::NodePtr &bias_add,
                                               ge::NodePtr &bias, bool is_bias_and_cube_direct_link) const {
  FE_LOGD("start to merge bias");
  vector<ge::NodePtr> del_nodes;
  if (bias->GetType() == CAST) {
    del_nodes.push_back(bias);
    if (MergeBiasWithCast(cube_node, bias) != SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][TfMrgSubFus][MrgBias] Merge bias node with cast node failed.");
      return FAILED;
    }
  } else {
    if (MergeBiasNoCast(cube_node, bias, is_bias_and_cube_direct_link) != SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][TfMrgSubFus][MrgBias] Merge bias node without cast node failed.");
      return FAILED;
    }
  }

  // remove nodes
  if (bias_add != nullptr) {
    del_nodes.push_back(bias_add);

    auto cube_out_data_anchor = cube_node->GetOutDataAnchor(0);
    FE_CHECK(cube_out_data_anchor == nullptr,
             REPORT_FE_ERROR("[GraphOpt][TfMrgSubFus][MrgBias] cube_out_data_anchor is null"), return FAILED);
    auto bias_out_data_anchor = bias_add->GetOutDataAnchor(0);
    FE_CHECK(bias_out_data_anchor == nullptr,
             REPORT_FE_ERROR("[GraphOpt][TfMrgSubFus][MrgBias] bias_out_data_anchor is null"), return FAILED);
    auto pre_in_data_anchor = bias_out_data_anchor->GetPeerInDataAnchors().at(0);
    FE_CHECK(pre_in_data_anchor == nullptr,
             REPORT_FE_ERROR("[GraphOpt][TfMrgSubFus][MrgBias] pre_in_data_anchor is null"), return FAILED);
    for (size_t i = 0; i < bias_add->GetAllInDataAnchors().size(); ++i) {
      auto in_data_anchor = bias_add->GetInDataAnchor(i);
      FE_CHECK_NOTNULL(in_data_anchor);
      in_data_anchor->UnlinkAll();
    }
    for (size_t i = 0; i < bias_add->GetAllOutDataAnchors().size(); ++i) {
      auto out_data_anchor = bias_add->GetOutDataAnchor(i);
      FE_CHECK_NOTNULL(out_data_anchor);
      out_data_anchor->UnlinkAll();
    }
    if (ge::GraphUtils::AddEdge(cube_out_data_anchor, pre_in_data_anchor) != ge::GRAPH_SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][TfMrgSubFus][MrgBias] Add edge between node[%s] and node[%s] error.",
                      cube_node->GetName().c_str(), bias_add->GetName().c_str());
      return FAILED;
    }
  }

  if (remove_nodes_from_map(graph, del_nodes) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][TfMrgSubFus][MrgBias] removeNodesFromMap failed.");
    return FAILED;
  }
  FE_LOGD("merge bias end");

  return SUCCESS;
}

/*
[case1]
  cube don't have third input, graph is cube + biasadd, usually in tf case

 *                       \     /
 *                     WeightQuant        bias
 *                          |              /
 *                          V             /
 *  --> AscendQuant --> CUBE_NODE --> bias_add --> AscendDequant -->
 *
 * in this case is_bias_and_cube_direct_link = false

[case2]
  cube has third input as its bias, usually in onnx case

 *                       \     /
 *                     WeightQuant
 *                          |
 *                          V
 *  --> AscendQuant --> CUBE_NODE --> AscendDequant -->
 *                          ^
 *                          |
 *                         bias
 *
 * in this case bias_add = nullptr, is_bias_and_cube_direct_link = true
*/
Status TfMergeWeightQuantFusionPass::Fusion(ge::ComputeGraph &graph,
    Mapping &mapping, vector<ge::NodePtr> &fusion_nodes) {
  ge::NodePtr cube_node = GetNodeFromMapping(PATN_CUBENODE, mapping);
  FE_CHECK(cube_node == nullptr, REPORT_FE_ERROR("[GraphOpt][TfMrgSubFus][Fus] get cube node failed."),
           return FAILED);
  FE_LOGD("Start to do TfMergeSubFusionPass, cube node:[%s].", cube_node->GetName().c_str());

  ge::NodePtr weight_quant_node = GetNodeFromMapping(PATN_WEIGHTQUANT, mapping);
  if (weight_quant_node == nullptr) {
    FE_LOGD("Cube node[%s] weight node is not ascend_weight.", cube_node->GetName().c_str());
  }

  ge::NodePtr split_node = GetNodeFromMapping(PATN_SPLIT, mapping);

  if (split_node != nullptr) {
    // [case1]
    for (auto conv_node : split_node->GetOutAllNodes()) {
      weight_quant_node = conv_node->GetInAllNodes().at(1);
      FE_CHECK(weight_quant_node == nullptr,
          REPORT_FE_ERROR("[GraphOpt][TfMrgSubFus][Fus] get sub node failed."), return FAILED);

      // deal with weight quant node
      if (MergeWeight(graph, conv_node, weight_quant_node) != SUCCESS) {
        REPORT_FE_ERROR("[GraphOpt][TfMrgSubFus][Fus] mergeWeight failed, conv %s, weight quant node %s",
                        conv_node->GetName().c_str(), weight_quant_node->GetName().c_str());
        return FAILED;
      }

      ge::NodePtr bias_node = conv_node->GetOutAllNodes().at(0);
      if (bias_node->GetType() == OP_BIASADD) {
        if (MergeBias(graph, conv_node, bias_node, bias_node->GetInAllNodes().at(1), false) != SUCCESS) {
          REPORT_FE_ERROR("[GraphOpt][TfMrgSubFus][Fus]  mergeBias failed, conv %s, bias node %s ",
                          conv_node->GetName().c_str(), bias_node->GetName().c_str());
          return FAILED;
        }
      }
    }
  } else {
    // deal with weight quant node
    if (MergeWeight(graph, cube_node, weight_quant_node) != SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][TfMrgSubFus][Fus] mergeWeight failed");
      return FAILED;
    }

    ge::NodePtr bias_add = GetNodeFromMapping(PATN_BIASADD, mapping);
    auto input_nodes = cube_node->GetInAllNodes();
    if (bias_add != nullptr) {
      // [case1]
      ge::NodePtr bias = bias_add->GetInAllNodes().at(1);
      if (MergeBias(graph, cube_node, bias_add, bias, false) != SUCCESS) {
        REPORT_FE_ERROR("[GraphOpt][TfMrgSubFus][Fus] mergeBias failed");
        return FAILED;
      }
    } else if (input_nodes.size() == kInputSize) {
      // [case2]
      if (MergeBias(graph, cube_node, nullptr, input_nodes.at(2), true) != SUCCESS) {
        REPORT_FE_ERROR("[GraphOpt][TfMrgSubFus][Fus]  process bias input of cube node[%s] failed.",
                        cube_node->GetName().c_str());
        return FAILED;
      }
    }
  }

  return SUCCESS;
}

}  // namespace fe
