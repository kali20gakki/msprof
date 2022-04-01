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

#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/quant_pass/tf_tag_no_const_folding_fusion_pass.h"
#include <string>
#include "common/fe_utils.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/node_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/tensor_utils.h"

namespace fe {

static const string OP_CONV = "Conv2D";
static const string OP_MATMULV2 = "MatMulV2";
static const string OP_DECONV = "Deconvolution";
static const string OP_DEPTHWISE = "DepthwiseConv2D";
static const string OP_CONV2DBPI = "Conv2DBackpropInput";
static const string OP_BIASADD = "BiasAdd";
static const string OP_PAD = "Pad";
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
static const string PATN_WEIGHT_QUANT = "weight_quant";
static const string PATN_DEQUANT = "dequant";
static const string PATN_CUBENODE = "cube_node";
static const string PATN_CONV2DBPI = "conv2d_backpropinput";
static const string PATN_BIASADD = "biasadd";
static const string PATN_PAD = "pad";
static const string PATN_SPLIT = "split";
static const string PATN_CONCATV2 = "concatv2";

static const size_t ALL_INPUT_NUM = 3;
static const size_t WEIGHT_INDEX = 1;
static const size_t FEATURE_MAP_INDEX = 2;

static const string ATTR_NO_FOLDING = "no_need_constant_folding";

/* we only match 'with biasadd' and 'without biasadd' 2 kinds of pattern */
vector<FusionPattern *> TfTagNoConstFolding::DefinePatterns() {
  vector<FusionPattern *> patterns;

  /*
   * ================================pattern0===================================
   *                       \     /
   *                  ascendweightquant
   *                          |             /
   *                          V            /
   *  --> AscendQuant --> CUBE_NODE --> BiasAdd --> AscendDequant -->
   *
   * ===========================================================================
   */
  FusionPattern *pattern0 = new (std::nothrow) FusionPattern("tfTagNoConstFoldingFusion0");
  FE_CHECK(pattern0 == nullptr,
           REPORT_FE_ERROR("[GraphOpt][TfTagNoCstFold][DefPtn] new FusionPattern object failed!"), return patterns);
  /* Above defines ops that we need */
  pattern0->AddOpDesc(PATN_QUANT, {OP_QUANT})
      .AddOpDesc(PATN_WEIGHT_QUANT, {OP_WEIGHTQUANT, OP_SUB})
      .AddOpDesc(PATN_CUBENODE, {OP_CONV, OP_MATMULV2, OP_DECONV, OP_DEPTHWISE})
      .AddOpDesc(PATN_BIASADD, {OP_BIASADD})
      .AddOpDesc(PATN_DEQUANT, {OP_DEQUANT})
      .SetInputs(PATN_CUBENODE, {PATN_QUANT, PATN_WEIGHT_QUANT})
      .SetInputs(PATN_BIASADD, {PATN_CUBENODE})
      .SetInputs(PATN_DEQUANT, {PATN_BIASADD})
      .SetOutput(PATN_DEQUANT);
  patterns.push_back(pattern0);

  /*
   * ================================pattern1===================================
   *                       \     /
   *                  ascendweightquant
   *                          |
   *                          V
   *  --> AscendQuant --> CUBE_NODE --> AscendDequant -->
   *
   * ===========================================================================
   */
  FusionPattern *pattern1 = new (std::nothrow) FusionPattern("tfTagNoConstFoldingFusion1");
  FE_CHECK(pattern1 == nullptr,
           REPORT_FE_ERROR("[GraphOpt][TfTagNoCstFold][DefPtn] new FusionPattern object failed!"), return patterns);
  /* Above defines ops that we need */
  pattern1->AddOpDesc(PATN_QUANT, {OP_QUANT})
      .AddOpDesc(PATN_WEIGHT_QUANT, {OP_WEIGHTQUANT, OP_SUB})
      .AddOpDesc(PATN_CUBENODE, {OP_CONV, OP_MATMULV2, OP_DECONV, OP_DEPTHWISE})
      .AddOpDesc(PATN_DEQUANT, {OP_DEQUANT})
      .SetInputs(PATN_CUBENODE, {PATN_QUANT, PATN_WEIGHT_QUANT})
      .SetInputs(PATN_DEQUANT, {PATN_CUBENODE})
      .SetOutput(PATN_DEQUANT);
  patterns.push_back(pattern1);

  /*
   * ================================pattern2===================================
   *                           \     /             /
   *                      ascendweightquant       /
   *                              |              /
   *                              V             /
   *  --> AscendQuant -->Pad -> Conv2D --> BiasAdd --> AscendDequant -->
   *
   * ===========================================================================
   */
  FusionPattern *pattern2 = new (std::nothrow) FusionPattern("tfTagNoConstFoldingFusion2");
  FE_CHECK(pattern2 == nullptr,
           REPORT_FE_ERROR("[GraphOpt][TfTagNoCstFold][DefPtn] new FusionPattern object failed!"), return patterns);
  /* Above defines ops that we need */
  pattern2->AddOpDesc(PATN_QUANT, {OP_QUANT})
      .AddOpDesc(PATN_WEIGHT_QUANT, {OP_WEIGHTQUANT, OP_SUB})
      .AddOpDesc(PATN_PAD, {OP_PAD})
      .AddOpDesc(PATN_CUBENODE, {OP_CONV, OP_DEPTHWISE})
      .AddOpDesc(PATN_BIASADD, {OP_BIASADD})
      .AddOpDesc(PATN_DEQUANT, {OP_DEQUANT})
      .SetInputs(PATN_PAD, {PATN_QUANT})
      .SetInputs(PATN_CUBENODE, {PATN_PAD, PATN_WEIGHT_QUANT})
      .SetInputs(PATN_BIASADD, {PATN_CUBENODE})
      .SetInputs(PATN_DEQUANT, {PATN_BIASADD})
      .SetOutput(PATN_DEQUANT);
  patterns.push_back(pattern2);

  /*
   * ================================pattern3===================================
   *                           \     /
   *                      ascendweightquant
   *                              |
   *                              V
   *  --> AscendQuant -->Pad -> Conv2D --> AscendDequant -->
   *
   * ===========================================================================
   */
  FusionPattern *pattern3 = new (std::nothrow) FusionPattern("tfTagNoConstFoldingFusion3");
  FE_CHECK(pattern3 == nullptr,
           REPORT_FE_ERROR("[GraphOpt][TfTagNoCstFold][DefPtn] new FusionPattern object failed!"), return patterns);
  /* Above defines ops that we need */
  pattern3->AddOpDesc(PATN_QUANT, {OP_QUANT})
      .AddOpDesc(PATN_WEIGHT_QUANT, {OP_WEIGHTQUANT, OP_SUB})
      .AddOpDesc(PATN_PAD, {OP_PAD})
      .AddOpDesc(PATN_CUBENODE, {OP_CONV, OP_DEPTHWISE})
      .AddOpDesc(PATN_DEQUANT, {OP_DEQUANT})
      .SetInputs(PATN_PAD, {PATN_QUANT})
      .SetInputs(PATN_CUBENODE, {PATN_PAD, PATN_WEIGHT_QUANT})
      .SetInputs(PATN_DEQUANT, {PATN_CUBENODE})
      .SetOutput(PATN_DEQUANT);
  patterns.push_back(pattern3);

  /*
   * ================================pattern4===================================
   *                          \     /           /
   *                     ascendweightquant     /
   *                             |            /
   *                             V           /
   *  AscendQuant -->Split -> Conv2D --> BiasAdd --> ConcatV2 --> AscendDequant
   *                    \                            /
   *                     \___ Conv2D --> BiasAdd ___/
   *                      \                        /
   *                       \_ Conv2D --> BiasAdd _/
   *                                 ...
   *
   * ===========================================================================
   */
  FusionPattern *pattern4 = new (std::nothrow) FusionPattern("tfTagNoConstFoldingFusion3");
  FE_CHECK(pattern4 == nullptr,
           REPORT_FE_ERROR("[GraphOpt][TfTagNoCstFold][DefPtn] new FusionPattern object failed!"), return patterns);
  /* Above defines ops that we need */
  pattern4->AddOpDesc(PATN_QUANT, {OP_QUANT})
      .AddOpDesc(PATN_SPLIT, {OP_SPLIT})
      .AddOpDesc(PATN_WEIGHT_QUANT, {OP_WEIGHTQUANT, OP_SUB})
      .AddOpDesc(PATN_CUBENODE, {OP_CONV})
      .AddOpDesc(PATN_BIASADD, {OP_BIASADD})
      .AddOpDesc(PATN_CONCATV2, {OP_CONCATV2, OP_CONCAT})
      .AddOpDesc(PATN_DEQUANT, {OP_DEQUANT})
      .SetInputs(PATN_SPLIT, {PATN_QUANT})
      .SetInputs(PATN_CUBENODE, {PATN_SPLIT, PATN_WEIGHT_QUANT})
      .SetInputs(PATN_BIASADD, {PATN_CUBENODE})
      .SetInputs(PATN_CONCATV2, {PATN_BIASADD})
      .SetInputs(PATN_DEQUANT, {PATN_CONCATV2})
      .SetOutput(PATN_DEQUANT);
  patterns.push_back(pattern4);

  /*
   * ================================pattern5===================================
   *                           \   /
   *                     ascendweightquant
   *                             |
   *                             V
   *  AscendQuant -->Split -> Conv2D --> ConcatV2 --> AscendDequant
   *                    \                /
   *                     \___ Conv2D ___/
   *                      \            /
   *                       \_ Conv2D _/
   *                           ...
   *
   * ===========================================================================
   */
  FusionPattern *pattern5 = new (std::nothrow) FusionPattern("tfTagNoConstFoldingFusion3");
  FE_CHECK(pattern5 == nullptr,
           REPORT_FE_ERROR("[GraphOpt][TfTagNoCstFold][DefPtn] new FusionPattern object failed!"), return patterns);
  /* Above defines ops that we need */
  pattern5->AddOpDesc(PATN_QUANT, {OP_QUANT})
      .AddOpDesc(PATN_SPLIT, {OP_SPLIT})
      .AddOpDesc(PATN_WEIGHT_QUANT, {OP_WEIGHTQUANT, OP_SUB})
      .AddOpDesc(PATN_CUBENODE, {OP_CONV})
      .AddOpDesc(PATN_CONCATV2, {OP_CONCATV2, OP_CONCAT})
      .AddOpDesc(PATN_DEQUANT, {OP_DEQUANT})
      .SetInputs(PATN_SPLIT, {PATN_QUANT})
      .SetInputs(PATN_CUBENODE, {PATN_SPLIT, PATN_WEIGHT_QUANT})
      .SetInputs(PATN_CONCATV2, {PATN_CUBENODE})
      .SetInputs(PATN_DEQUANT, {PATN_CONCATV2})
      .SetOutput(PATN_DEQUANT);
  patterns.push_back(pattern5);

  /*
   * ================================pattern6===================================
   *                       \     /             /
   *                  ascendweightquant       /
   *                          |              /
   *                          V             /
   *  --> AscendQuant --> CUBE_NODE --> BiasAdd --> AscendDequant -->
   *                          ^
   *                          |
   *                         CONST
   * ===========================================================================
   */
  FusionPattern *pattern6 = new (std::nothrow) FusionPattern("tfTagNoConstFoldingFusion6");
  FE_CHECK(pattern6 == nullptr,
           REPORT_FE_ERROR("[GraphOpt][TfTagNoCstFold][DefPtn] new FusionPattern object failed!"), return patterns);
  /* Above defines ops that we need */
  pattern6->AddOpDesc(PATN_CONV2DBPI, {OP_CONV2DBPI})
      .AddOpDesc(PATN_BIASADD, {OP_BIASADD})
      .AddOpDesc(PATN_DEQUANT, {OP_DEQUANT})
      .SetInputs(PATN_BIASADD, {PATN_CONV2DBPI})
      .SetInputs(PATN_DEQUANT, {PATN_BIASADD})
      .SetOutput(PATN_DEQUANT);
  patterns.push_back(pattern6);

  /*
     * ================================pattern7===================================
     *                       \     /
     *                  ascendweightquant
     *                          |
     *                          V
     *  --> AscendQuant --> CUBE_NODE --> AscendDequant -->
     *                          ^
     *                          |
     *                         CONST
     * ===========================================================================
     */
  FusionPattern *pattern7 = new (std::nothrow) FusionPattern("tfTagNoConstFoldingFusion7");
  FE_CHECK(pattern7 == nullptr,
           REPORT_FE_ERROR("[GraphOpt][TfTagNoCstFold][DefPtn] new FusionPattern object failed!"), return patterns);
  /* Above defines ops that we need */
  pattern7->AddOpDesc(PATN_CONV2DBPI, {OP_CONV2DBPI})
      .AddOpDesc(PATN_DEQUANT, {OP_DEQUANT})
      .SetInputs(PATN_DEQUANT, {PATN_CONV2DBPI})
      .SetOutput(PATN_DEQUANT);
  patterns.push_back(pattern7);

  return patterns;
}

Status TfTagNoConstFolding::Fusion(ge::ComputeGraph &graph, Mapping &mapping, vector<ge::NodePtr> &fusion_nodes) {
  for (Mapping::const_iterator itr = mapping.cbegin(); itr != mapping.cend(); ++itr) {
    if (!ge::AttrUtils::SetBool(itr->second[0]->GetOpDesc(), ATTR_NO_FOLDING, true)) {
      REPORT_FE_ERROR("[GraphOpt][TfTagNoCstFold][Fus] set attr no folding failed!");
      return FAILED;
    }
  }
  // if have split-conv2d-concatv2
  ge::NodePtr conv2dback_node = GetNodeFromMapping(PATN_CONV2DBPI, mapping);
  ge::NodePtr split_node = GetNodeFromMapping(PATN_SPLIT, mapping);
  ge::NodePtr bias_node = GetNodeFromMapping(PATN_BIASADD, mapping);
  if (conv2dback_node != nullptr) {
    std::vector<ge::NodePtr> alter_native_nodes;
    auto input_nodes = conv2dback_node->GetInDataNodes();
    if (input_nodes.size() == ALL_INPUT_NUM) {
      auto sub_node = input_nodes.at(WEIGHT_INDEX);
      FE_CHECK_NOTNULL(sub_node);
      auto quant_node = input_nodes.at(FEATURE_MAP_INDEX);
      FE_CHECK_NOTNULL(quant_node);
      bool check_sub_node_type = ((sub_node->GetType() == OP_WEIGHTQUANT) || (sub_node->GetType() == OP_SUB)) &&
                                  quant_node->GetType() == ASCEND_QUANT;
      if (check_sub_node_type) {
        alter_native_nodes.push_back(input_nodes.at(WEIGHT_INDEX));
        alter_native_nodes.push_back(input_nodes.at(FEATURE_MAP_INDEX));
      }
    } else {
      FE_LOGW("Node[%s] should have three input node.", conv2dback_node->GetName().c_str());
    }
    for (auto &node : alter_native_nodes) {
      (void)ge::AttrUtils::SetBool(node->GetOpDesc(), ATTR_NO_FOLDING, true);
    }
  }
  FE_CHECK(split_node == nullptr, FE_LOGD("Do not have group conv fusion pattern."), return SUCCESS);
  for (const auto &conv_node : split_node->GetOutAllNodes()) {
    std::vector<ge::NodePtr> alter_native_nodes;
    alter_native_nodes.push_back(conv_node);
    ge::NodePtr sub_node = conv_node->GetInAllNodes().at(1);

    if (sub_node != nullptr) {
      alter_native_nodes.push_back(sub_node);
    }

    if (bias_node != nullptr) {
      ge::NodePtr next_node = conv_node->GetOutAllNodes().at(0);
      if (next_node->GetType() != OP_BIASADD) {
        REPORT_FE_ERROR("[GraphOpt][TfTagNoCstFold][Fus] pattern is wrong, not fusion, conv node:[%s].",
                        conv_node->GetName().c_str());
        return FAILED;
      }
      // add baisadd node
      alter_native_nodes.push_back(next_node);
    }
    for (auto &node : alter_native_nodes) {
      if (!ge::AttrUtils::SetBool(node->GetOpDesc(), ATTR_NO_FOLDING, true)) {
        REPORT_FE_ERROR("[GraphOpt][TfTagNoCstFold][Fus] set attr no folding of node %s failed.",
                        node->GetName().c_str());
        return FAILED;
      }
    }
  }
  return SUCCESS;
}

}  // namespace fe
