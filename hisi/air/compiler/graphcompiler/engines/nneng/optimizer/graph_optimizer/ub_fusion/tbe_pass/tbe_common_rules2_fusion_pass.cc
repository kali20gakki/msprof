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

#include "graph_optimizer/ub_fusion/tbe_pass/tbe_common_rules2_fusion_pass.h"
#include <string>
#include <vector>
#include "common/op_info_common.h"

namespace fe {
namespace {
const string PATTERN_STRIDEREAD = "strideRead";        // NOLINT
const string PATTERN_TRANSDATA1 = "transdata1";        // NOLINT
const string PATTERN_CONVOLUTION = "convolution";      // NOLINT
const string PATTERN_DEPTHWISECONV = "depthwiseconv";  // NOLINT
const string PATTERN_DEQUANT = "dequant";              // NOLINT
const string PATTERN_ELEMWISE = "elemWise";            // NOLINT
const string PATTERN_QUANT = "quant";                  // NOLINT
const string PATTERN_STRIDEWRITE = "strideWrite";      // NOLINT
const string PATTERN_OTHER_INPUT = "otherInput";       // NOLINT
const string PATTERN_OUTPUT = "output";                // NOLINT

const vector<string> ELEM_WISE_WHITE_LIST = {"Eltwise", "LeakyRelu", "Vadd", "Relu",
                                             "Relu6",   "Relu6D", "PRelu",
                                             "Add", "Mul",  "Softplus", "Sigmoid", "Mish",
                                             "Minimum", "Tanh", "Swish"};  // NOLINT

const int MAX_OP_COUNT = 20;
const int MAX_ELEMWISE_COUNT = 5;
const int INPUT_MAX_SIZE = 2;
const int kConvOutputMaxSize = 2;
}
/*
* @brief:  define a common ub fusion pattern:
* (StrideRead) -> Convolution -> (Dequant) -> Elewise*N -> Quant -> (StrideWrite)
*
* pattern limits:
* 1. StrideRead, StrideWrite, Dequant are optional, Conv2D and Quant are required.
* 2. Elewise supports LeakyRelu, Vadd, Relu, Relu6, Prelu, Add, Mul. The number of Elewise can be 0 to 5.
* 3. There are two outputs from Dequant or Elewise, one is int8 or int4, the other is fp16.
*
*
* fusion node: (StrideRead), Convolution, (AscendDequant), Elewise, AscendQuant,
*
* @return BufferFusionPattern: return all valid patterns.
*/
vector<BufferFusionPattern *> TbeCommonRules2FusionPass::DefinePatterns() {
  vector<BufferFusionPattern *> patterns;
  string pass_name = "TbeCommonRules2FusionPass";
  auto *pattern = new (std::nothrow) BufferFusionPattern(pass_name, MAX_OP_COUNT);
  FE_CHECK((pattern == nullptr),
           REPORT_FE_ERROR("[SubGraphOpt][CommonRules2Fus][DefPtn] New an object failed."), return patterns);
  FE_LOGD("Start to define %s pass pattern.", pass_name.c_str());
  pattern->AddOpDesc(PATTERN_STRIDEREAD, {OP_PATTERN_STRIDED_READ}, TBE_PATTERN_NUM_NONE,
                     TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_GROUPID_INVALID, IGNORE_SHAPE_TYPE)
      .AddOpDesc(PATTERN_TRANSDATA1, {TRANSDATA}, TBE_PATTERN_NUM_NONE, TBE_PATTERN_NUM_DEFAULT,
                 TBE_PATTERN_GROUPID_INVALID, IGNORE_SHAPE_TYPE, true)
      .AddOpDesc(PATTERN_CONVOLUTION, {OP_PATTERN_CONV}, TBE_PATTERN_NUM_NONE, TBE_PATTERN_NUM_DEFAULT,
                 TBE_PATTERN_GROUPID_INVALID, IGNORE_SHAPE_TYPE)
      .AddOpDesc(PATTERN_DEPTHWISECONV, {OP_PATTERN_DEPTHWISE_CONV}, TBE_PATTERN_NUM_NONE,
                 TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_GROUPID_INVALID, IGNORE_SHAPE_TYPE)
      .AddOpDesc(PATTERN_DEQUANT, {OP_PATTERN_DEQUANT}, TBE_PATTERN_NUM_NONE, TBE_PATTERN_NUM_DEFAULT,
                 TBE_PATTERN_GROUPID_INVALID, IGNORE_SHAPE_TYPE)
      .AddOpDesc(PATTERN_OTHER_INPUT, {TBE_PATTERN_INPUT_NODE}, TBE_PATTERN_NUM_NONE,
                 TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_GROUPID_INVALID, IGNORE_SHAPE_TYPE)
      .AddOpDesc(PATTERN_ELEMWISE, {OP_PATTERN_ELEMWISE}, TBE_PATTERN_NUM_NONE,
                 MAX_ELEMWISE_COUNT, TBE_PATTERN_GROUPID_INVALID, IGNORE_SHAPE_TYPE)
      .AddOpDesc(PATTERN_QUANT, {OP_PATTERN_QUANT}, TBE_PATTERN_NUM_DEFAULT,
                 TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_GROUPID_INVALID, IGNORE_SHAPE_TYPE)
      .AddOpDesc(PATTERN_STRIDEWRITE, {OP_PATTERN_STRIDED_WRITE}, TBE_PATTERN_NUM_NONE,
                 TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_GROUPID_INVALID, IGNORE_SHAPE_TYPE)
      .SetHead({PATTERN_STRIDEREAD, PATTERN_CONVOLUTION, PATTERN_DEPTHWISECONV, PATTERN_TRANSDATA1})
      .SetOutputs(PATTERN_TRANSDATA1, {PATTERN_CONVOLUTION, PATTERN_DEPTHWISECONV})
      .SetOutputs(PATTERN_STRIDEREAD, {PATTERN_CONVOLUTION, PATTERN_DEPTHWISECONV})
      .SetOutputs(PATTERN_CONVOLUTION, {PATTERN_DEQUANT}, TBE_OUTPUT_BRANCH_SINGLE, true, true)
      .SetOutputs(PATTERN_DEPTHWISECONV, {PATTERN_DEQUANT}, TBE_OUTPUT_BRANCH_SINGLE, true, true)
      .SetOutputs(PATTERN_DEQUANT, {PATTERN_ELEMWISE}, TBE_OUTPUT_BRANCH_SINGLE, true, true)
      .SetOutputs(PATTERN_OTHER_INPUT, {PATTERN_DEQUANT})
      .SetOutputs(PATTERN_ELEMWISE, {PATTERN_QUANT}, TBE_OUTPUT_BRANCH_SINGLE, true, true)
      .SetOutputs(PATTERN_QUANT, {PATTERN_STRIDEWRITE}, TBE_OUTPUT_BRANCH_SINGLE, false, true);
  patterns.push_back(pattern);
  FE_LOGD("End to define %s pass pattern.", pass_name.c_str());

  return patterns;
}

int TbeCommonRules2FusionPass::CountOtherOutput(vector<ge::NodePtr> dequant_nodes,
                                                vector<ge::NodePtr> elem_wise_nodes) {
  int other_out_count = 0;
  // count EltWise op other output
  for (const auto &elem_wise_node : elem_wise_nodes) {
    if (elem_wise_node->GetOutDataNodes().empty()) {
      continue;
    }
    int other_elt_wise_out = static_cast<int>(elem_wise_node->GetOutDataNodes().size() - 1);
    other_out_count += other_elt_wise_out;
  }

  // count Dequant op other output
  if (!dequant_nodes.empty()) {
    int other_dequant_out = 0;
    if (dequant_nodes[0]->GetOutDataNodes().empty()) {
      other_dequant_out = 0;
    } else {
      other_dequant_out = static_cast<int>(dequant_nodes[0]->GetOutDataNodes().size() - 1);
    }
    other_out_count += other_dequant_out;
  }
  return other_out_count;
}

bool TbeCommonRules2FusionPass::JudgeElemShapeInScopeLessThanOutScope(const vector<ge::NodePtr> &pre_elemwise_nodes,
                                                                      const vector<ge::NodePtr> &elemwise_nodes) {
  if (pre_elemwise_nodes.empty()) {
    return false;
  }
  ge::NodePtr cur_node = pre_elemwise_nodes[0];
  for (auto &elemwise_node: elemwise_nodes) {
    ge::NodePtr pre_node = cur_node;
    cur_node = elemwise_node;
    if (cur_node->GetOpDesc()->GetInputsSize() != INPUT_MAX_SIZE) {
      continue;
    }

    if ((cur_node->GetInDataAnchor(0)->GetPeerOutAnchor() == nullptr) ||
        (cur_node->GetInDataAnchor(0)->GetPeerOutAnchor()->GetOwnerNode() == nullptr)) {
      return false;
    }
    auto cur_node_input0 = cur_node->GetInDataAnchor(0)->GetPeerOutAnchor()->GetOwnerNode();

    vector<int64_t> in_scope_dims;
    vector<int64_t> out_scope_dims;
    if (cur_node_input0->GetName() == pre_node->GetOpDesc()->GetName()) {
      in_scope_dims = cur_node->GetOpDesc()->MutableInputDesc(0)->MutableShape().GetDims();
      out_scope_dims = cur_node->GetOpDesc()->MutableInputDesc(1)->MutableShape().GetDims();
    } else {
      in_scope_dims = cur_node->GetOpDesc()->MutableInputDesc(1)->MutableShape().GetDims();
      out_scope_dims = cur_node->GetOpDesc()->MutableInputDesc(0)->MutableShape().GetDims();
    }
    if (in_scope_dims.size() != out_scope_dims.size()) {
      FE_LOGD("Elem_wise[node: %s] : the number of input's dims is not equal. in_scope_dims: %zu, out_scope_dims: %zu",
              cur_node->GetName().c_str(), in_scope_dims.size(), out_scope_dims.size());
      return false;
    } else {
      for (size_t i = 0; i < in_scope_dims.size(); i++) {
        if (in_scope_dims[i] < out_scope_dims[i]) {
          FE_LOGD("Elem_wise[node: %s] dims[%zu]: the value of in_scope is less than out_scope. in_scope : %ld,"
                  " out_scope : %ld", cur_node->GetName().c_str(), i, in_scope_dims[i], out_scope_dims[i]);
          return true;
        }
      }
    }
  }
  return false;
}
void TbeCommonRules2FusionPass::CheckTransNodes(vector<ge::NodePtr> &trans1_nodes,
                                                vector<ge::NodePtr> &conv_depthwise_nodes,
                                                vector<ge::NodePtr> &fusion_nodes) const {
  CubeVecState cube_vec_state = CheckCubeVecState();
  if (cube_vec_state == CubeVecState::CUBE_VEC_UNKNOWN) {
    for (auto trans_node : trans1_nodes) {
      auto iter = std::find(fusion_nodes.begin(), fusion_nodes.end(), trans_node);
      if (iter != fusion_nodes.end()) {
        fusion_nodes.erase(iter);
      }
    }
  } else {
    CheckFusionTransNode(conv_depthwise_nodes[0], trans1_nodes, fusion_nodes);
  }
}
/*
* @brief: parse nodes matched in mapping and call DoFusion
* @param [in] graph: original graph
* @param [out] mapping: nodes matched by pattern
* @return bool: fusion status ok or not.
*/
Status TbeCommonRules2FusionPass::GetFusionNodes(const BufferFusionMapping &mapping,
                                                 vector<ge::NodePtr> &fusion_nodes) {
  fusion_nodes = GetMatchedNodes(mapping);
  vector<ge::NodePtr> output_nodes = GetMatchedNodesByDescName(TBE_PATTERN_OUTPUT_NODE, mapping);
  vector<ge::NodePtr> trans1_nodes = GetMatchedNodesByDescName(PATTERN_TRANSDATA1, mapping);
  vector<ge::NodePtr> conv_nodes = GetMatchedNodesByDescName(PATTERN_CONVOLUTION, mapping);
  vector<ge::NodePtr> depthwise_nodes = GetMatchedNodesByDescName(PATTERN_DEPTHWISECONV, mapping);
  vector<ge::NodePtr> elem_wise_nodes = GetMatchedNodesByDescName(PATTERN_ELEMWISE, mapping);
  vector<ge::NodePtr> dequant_nodes = GetMatchedNodesByDescName(PATTERN_DEQUANT, mapping);
  vector<ge::NodePtr> quant_nodes = GetMatchedNodesByDescName(PATTERN_QUANT, mapping);
  vector<ge::NodePtr> stride_write_nodes = GetMatchedNodesByDescName(PATTERN_STRIDEWRITE, mapping);

  bool conv_depth_size = conv_nodes.size() == 1 || depthwise_nodes.size() == 1;
  if (!conv_depth_size) {
    FE_LOGD("There is no conv and depthwise in TbeCommonRules2FusionPass");
    fusion_nodes.clear();
    return SUCCESS;
  }
  CheckStridedReadInConv2d(conv_nodes, fusion_nodes);
  vector<ge::NodePtr> conv_depthwise_nodes = conv_nodes.size() == 1 ? conv_nodes : depthwise_nodes;
  CheckTransNodes(trans1_nodes, conv_depthwise_nodes, fusion_nodes);
  size_t conv_output_size = conv_depthwise_nodes[0]->GetOutDataNodes().size();
  // conv outputs size is more than 2, skip fused
  if (conv_output_size > kConvOutputMaxSize) {
    FE_LOGD("node: %s, outputs is more than 2, size is: %zu.",
            conv_depthwise_nodes[0]->GetName().c_str(), conv_output_size);
    fusion_nodes.clear();
    return SUCCESS;
  }

  // the output_data can't be fused
  for (const auto &outputnode : output_nodes) {
    auto node_ptr = find(fusion_nodes.begin(), fusion_nodes.end(), outputnode);
    if (node_ptr != fusion_nodes.end()) {
      fusion_nodes.erase(node_ptr);
    }
  }

  // this pattern only support one other output from dequant node or elem_wise node
  int other_out_count = CountOtherOutput(dequant_nodes, elem_wise_nodes);
  bool cond_other_out_count = (conv_output_size == 1 && other_out_count != 1) ||
                              (conv_output_size == kConvOutputMaxSize && other_out_count != 0);
  if (cond_other_out_count) {
    FE_LOGD("The number of other output from EltWise or Dequant is %d, skip fusion.", other_out_count);
    fusion_nodes.clear();
    return SUCCESS;
  }

  // if elewise has 2 input and inscope's shape less than outscope's shape, skip fusion
  bool dequant_flag = !dequant_nodes.empty() &&
  JudgeElemShapeInScopeLessThanOutScope(dequant_nodes, elem_wise_nodes);
  if (dequant_flag) {
    FE_LOGD("dequant_nodes exist, Elemwise node has 2 inputs and in scope shape is less than outscope");
    fusion_nodes.clear();
    return SUCCESS;
  }
  bool no_dequant_flag = dequant_nodes.empty() &&
  JudgeElemShapeInScopeLessThanOutScope(conv_depthwise_nodes, elem_wise_nodes);
  if (no_dequant_flag) {
    FE_LOGD("no dequant_nodes, Elemwise node has 2 inputs and in scope shape is less than outscope");
    fusion_nodes.clear();
    return SUCCESS;
  }

  // check whether the EltWise op is in the whitelist or inputsizes less then 3(only support single or double in)
  for (const auto &elem_wise_node : elem_wise_nodes) {
    bool support_flag = find(ELEM_WISE_WHITE_LIST.begin(), ELEM_WISE_WHITE_LIST.end(), elem_wise_node->GetType()) ==
            ELEM_WISE_WHITE_LIST.end() ||
        elem_wise_node->GetOpDesc()->GetInputsSize() > INPUT_MAX_SIZE;
    if (support_flag) {
      fusion_nodes.clear();
      FE_LOGD("Eltwise op[%s] type[%s] is not supported for this ub fusion pass, skip fusion.",
              elem_wise_node->GetName().c_str(), elem_wise_node->GetType().c_str());
      return SUCCESS;
    }
  }

  // if stride_write is the last node, check whether quant node has multi outputs
  bool quant_node_flag = quant_nodes[0]->GetOutDataNodes().size() > 1 && !stride_write_nodes.empty();
  if (quant_node_flag) {
    auto node_ptr = find(fusion_nodes.begin(), fusion_nodes.end(), stride_write_nodes[0]);
    if (node_ptr != fusion_nodes.end()) {
      fusion_nodes.erase(node_ptr);
    }
    FE_LOGD("Quant is not the last node of the matched pattern, \
            but has multi outpts, erase last node stride_write.");
  }
  return SUCCESS;
}

}  // namespace fe
