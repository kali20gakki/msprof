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

#include "graph_optimizer/ub_fusion/tbe_pass/tbe_common_rules0_fusion_pass.h"
#include <algorithm>
#include <string>
#include <vector>
#include "common/op_info_common.h"

namespace fe {
using std::vector;

namespace {
static const char PATTERN_STRIDED_READ[] = "stridedread";
static const char PATTERN_TRANSDATA1[] = "transdata1";
static const char PATTERN_CONV[] = "convolution";
static const char PATTERN_DEPTHWISECONV[] = "depthwiseconv";
static const char PATTERN_DEQUANT[] = "dequant";
static const char PATTERN_ELEMWISE[] = "elemwise";
static const char PATTERN_QUANT[] = "quant";
static const char PATTERN_STRIDED_WRITE[] = "stridedwrite";
static const char PATTERN_OTHER_INPUT[] = "otherInput";
static const int FUSION_OP_NUM_MAX = 10;
static const int INPUT_MAX_SIZE = 2;
// white list of OP_PATTERN_ELEMWISE
static const vector<string> WHITELIST_OF_OP_PATTERN_ELEMWISE = {"Eltwise", "LeakyRelu", "Vadd",    "Relu", "Relu6",
                                                                "Relu6D",  "PRelu",     "Add",     "Mul",  "Softplus",
                                                                "Sigmoid", "Mish",      "Minimum", "Tanh", "Swish"};
// black list of OP_PATTERN_ELEMWISE
static const vector<string> BLACKLIST_OF_OP_PATTERN_ELEMWISE = {"ReluGradV2"};
}  // namespace

/*
 * @brief:  define common rules0 ops fusion pattern
 *
 *   (StrideRead) + conv2_d + (dequant) + ele-wise*N + (quant) + (StrideWrite)
 *   restriction: 1.each node must be single output and single reference
 *                2.the range of N is 0 to 5
 *                3.allow multiple input, but only one input can be fusion
 *
 * @return BufferFusionPattern: return all valid patterns.
 */
vector<BufferFusionPattern *> TbeCommonRules0FusionPass::DefinePatterns() {
  vector<BufferFusionPattern *> patterns;
  string pass_name = "TbeCommonRules0FusionPass";
  BufferFusionPattern *pattern = new (std::nothrow) BufferFusionPattern(pass_name, FUSION_OP_NUM_MAX);
  FE_CHECK((pattern == nullptr), REPORT_FE_ERROR("[SubGraphOpt][CommonRules0Fus][DefPtn] New an object failed."),
           return patterns);
  FE_LOGD("Start to define %s pass pattern.", pass_name.c_str());
  // define pattern rules
  pattern
      ->AddOpDesc(PATTERN_STRIDED_READ, {OP_PATTERN_STRIDED_READ}, TBE_PATTERN_NUM_NONE, TBE_PATTERN_NUM_DEFAULT,
                  TBE_PATTERN_GROUPID_INVALID, IGNORE_SHAPE_TYPE)
      .AddOpDesc(PATTERN_TRANSDATA1, {TRANSDATA}, TBE_PATTERN_NUM_NONE, TBE_PATTERN_NUM_DEFAULT,
                 TBE_PATTERN_GROUPID_INVALID, IGNORE_SHAPE_TYPE, true)
      .AddOpDesc(PATTERN_CONV, {OP_PATTERN_CONV}, TBE_PATTERN_NUM_NONE, TBE_PATTERN_NUM_DEFAULT,
                 TBE_PATTERN_GROUPID_INVALID, IGNORE_SHAPE_TYPE)
      .AddOpDesc(PATTERN_DEPTHWISECONV, {OP_PATTERN_DEPTHWISE_CONV}, TBE_PATTERN_NUM_NONE, TBE_PATTERN_NUM_DEFAULT,
                 TBE_PATTERN_GROUPID_INVALID, IGNORE_SHAPE_TYPE)
      .AddOpDesc(PATTERN_DEQUANT, {OP_PATTERN_DEQUANT}, TBE_PATTERN_NUM_NONE, TBE_PATTERN_NUM_DEFAULT,
                 TBE_PATTERN_GROUPID_INVALID, IGNORE_SHAPE_TYPE)
      .AddOpDesc(PATTERN_ELEMWISE, {OP_PATTERN_ELEMWISE}, TBE_PATTERN_NUM_NONE, TBE_PATTERN_NUM_MAX,
                 TBE_PATTERN_GROUPID_INVALID, IGNORE_SHAPE_TYPE)
      .AddOpDesc(PATTERN_QUANT, {OP_PATTERN_QUANT}, TBE_PATTERN_NUM_NONE, TBE_PATTERN_NUM_DEFAULT,
                 TBE_PATTERN_GROUPID_INVALID, IGNORE_SHAPE_TYPE)
      .AddOpDesc(PATTERN_STRIDED_WRITE, {OP_PATTERN_STRIDED_WRITE}, TBE_PATTERN_NUM_NONE, TBE_PATTERN_NUM_DEFAULT,
                 TBE_PATTERN_GROUPID_INVALID, IGNORE_SHAPE_TYPE)
      .AddOpDesc(PATTERN_OTHER_INPUT, {TBE_PATTERN_INPUT_NODE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT,
                 TBE_PATTERN_GROUPID_INVALID, IGNORE_SHAPE_TYPE)
      .SetHead({PATTERN_STRIDED_READ, PATTERN_CONV, PATTERN_DEPTHWISECONV, PATTERN_TRANSDATA1})
      .SetOutputs(PATTERN_TRANSDATA1, {PATTERN_CONV, PATTERN_DEPTHWISECONV})
      .SetOutputs(PATTERN_STRIDED_READ, {PATTERN_CONV, PATTERN_DEPTHWISECONV})
      .SetOutputs(PATTERN_CONV, {PATTERN_DEQUANT}, TBE_OUTPUT_BRANCH_SINGLE, true)
      .SetOutputs(PATTERN_DEPTHWISECONV, {PATTERN_DEQUANT}, TBE_OUTPUT_BRANCH_SINGLE, true)
      .SetOutputs(PATTERN_OTHER_INPUT, {PATTERN_DEQUANT})
      .SetOutputs(PATTERN_DEQUANT, {PATTERN_ELEMWISE}, TBE_OUTPUT_BRANCH_SINGLE, true)
      .SetOutputs(PATTERN_ELEMWISE, {PATTERN_QUANT}, TBE_OUTPUT_BRANCH_SINGLE, true)
      .SetOutputs(PATTERN_QUANT, {PATTERN_STRIDED_WRITE});

  patterns.push_back(pattern);

  FE_LOGD("End to define %s pass pattern.", pass_name.c_str());
  return patterns;
}

static void DelNotMatchNodesFromFusionNodes(ge::NodePtr node_ptr, vector<ge::NodePtr> &fusion_nodes) {
  auto node = find(fusion_nodes.begin(), fusion_nodes.end(), node_ptr);
  if (node != fusion_nodes.end()) {
    fusion_nodes.erase(node);
  } else {
    return;
  }

  auto curr_nodes = node_ptr->GetOutDataNodes();
  if (curr_nodes.size() != 1) {
    return;
  } else {
    DelNotMatchNodesFromFusionNodes(curr_nodes.at(0), fusion_nodes);
  }
  return;
}

static bool IsInWhiteListOfOpPatternElemwise(vector<ge::NodePtr> &elemwise_nodes, ge::NodePtr &node_ptr) {
  for (auto &elemwise_node : elemwise_nodes) {
    string elemwise_type = elemwise_node->GetType();
    auto op_type =
        find(WHITELIST_OF_OP_PATTERN_ELEMWISE.begin(), WHITELIST_OF_OP_PATTERN_ELEMWISE.end(), elemwise_type);
    if (op_type == WHITELIST_OF_OP_PATTERN_ELEMWISE.end()) {
      FE_LOGD("node:%s[type:%s] not in elemwise white_list.", elemwise_node->GetName().c_str(), elemwise_type.c_str());
      node_ptr = elemwise_node;
      return false;
    }
  }
  return true;
}

bool TbeCommonRules0FusionPass::IsInBlackListOfOpPatternElemwise(vector<ge::NodePtr> &elemwise_nodes,
                                                                 ge::NodePtr &node_ptr) {
  for (auto &elemwise_node : elemwise_nodes) {
    string elemwise_type = elemwise_node->GetType();
    auto op_type =
        find(BLACKLIST_OF_OP_PATTERN_ELEMWISE.begin(), BLACKLIST_OF_OP_PATTERN_ELEMWISE.end(), elemwise_type);
    if (op_type != BLACKLIST_OF_OP_PATTERN_ELEMWISE.end()) {
      FE_LOGD("node:%s[type:%s] in elemwise black_list.", elemwise_node->GetName().c_str(), elemwise_type.c_str());
      node_ptr = elemwise_node;
      return true;
    }
  }
  return false;
}

static void CheckElewiseInputSize(vector<ge::NodePtr> &elemwise_nodes, vector<ge::NodePtr> &fusion_nodes) {
  for (auto elemwise_node : elemwise_nodes) {
    if (elemwise_node->GetOpDesc()->GetInputsSize() > INPUT_MAX_SIZE) {
      DelNotMatchNodesFromFusionNodes(elemwise_node, fusion_nodes);
      return;
    }
  }
}

bool TbeCommonRules0FusionPass::DealWithSameInAndOutScopeDimSize(const vector<int64_t> &in_scope_dims,
                                                                 const vector<int64_t> &out_scope_dims,
                                                                 const vector<ge::NodePtr> &elemwise_nodes,
                                                                 const ge::NodePtr &cur_node, const size_t &i,
                                                                 vector<ge::NodePtr> &fusion_nodes) {
  for (size_t j = 0; j < in_scope_dims.size(); j++) {
    if (in_scope_dims[j] < out_scope_dims[j]) {
      FE_LOGD(
          "Elem_wise[node: %s] dims[%zu] : the value of in_scope is less than out_scope. in_scope : %ld,"
          " out_scope : %ld",
          cur_node->GetName().c_str(), j, in_scope_dims[j], out_scope_dims[j]);
      vector<ge::NodePtr> new_elemwise_nodes;
      for (size_t z = i; z < elemwise_nodes.size(); z++) {
        new_elemwise_nodes.push_back(elemwise_nodes[z]);
      }
      for (auto new_elemwise_node : new_elemwise_nodes) {
        DelNotMatchNodesFromFusionNodes(new_elemwise_node, fusion_nodes);
      }
      return true;
    }
  }
  return false;
}

bool TbeCommonRules0FusionPass::JudgeElemShapeInScopeLessThanOutScope(const vector<ge::NodePtr> &pre_elemwise_nodes,
                                                                      const vector<ge::NodePtr> &elemwise_nodes,
                                                                      vector<ge::NodePtr> &fusion_nodes) {
  if (pre_elemwise_nodes.empty()) {
    return false;
  }
  ge::NodePtr cur_node = pre_elemwise_nodes[0];
  for (size_t i = 0; i < elemwise_nodes.size(); i++) {
    ge::NodePtr elemwise_node = elemwise_nodes[i];
    ge::NodePtr pre_node = cur_node;
    cur_node = elemwise_node;
    if (cur_node->GetOpDesc()->GetInputsSize() != INPUT_MAX_SIZE) {
      continue;
    }
    auto peerOutAnchor = cur_node->GetInDataAnchor(0)->GetPeerOutAnchor();
    if (peerOutAnchor == nullptr) {
      FE_LOGD("node[%s]'s first peer in anchor is null", cur_node->GetName().c_str());
      continue;
    }
    auto cur_node_input0 = peerOutAnchor->GetOwnerNode();
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
      FE_LOGD("Elem_wise[node: %s] : the number of input's dims is not equal. in_scope : %zu, out_scope : %zu",
              cur_node->GetName().c_str(), in_scope_dims.size(), out_scope_dims.size());
      return false;
    } else {
      if (DealWithSameInAndOutScopeDimSize(in_scope_dims, out_scope_dims, elemwise_nodes, cur_node, i, fusion_nodes)) {
        return true;
      }
    }
  }
  return false;
}

static void DelNotMatchNodes(vector<ge::NodePtr> &elemwise_nodes, vector<ge::NodePtr> &fusion_nodes) {
  if (!elemwise_nodes.empty()) {
    ge::NodePtr node = nullptr;
    if (!IsInWhiteListOfOpPatternElemwise(elemwise_nodes, node)) {
      DelNotMatchNodesFromFusionNodes(node, fusion_nodes);
    }
  }
}
/*
 * @brief: parse nodes matched in mapping and call DoFusion
 * @param [in] graph: original graph
 * @param [out] mapping: nodes matched by pattern
 * @return bool: fusion status ok or not.
 */
void TbeCommonRules0FusionPass::CheckTransNodes(vector<ge::NodePtr> &trans1_nodes,
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

Status TbeCommonRules0FusionPass::GetFusionNodes(const BufferFusionMapping &mapping,
                                                 vector<ge::NodePtr> &fusion_nodes) {
  FE_LOGD("Begin to do TbeCommonRules0FusionPass!");
  fusion_nodes = GetMatchedNodes(mapping);

  vector<ge::NodePtr> elemwise_nodes = GetMatchedNodesByDescName(PATTERN_ELEMWISE, mapping);
  // elewise only support single in or double in
  if (!elemwise_nodes.empty()) {
    CheckElewiseInputSize(elemwise_nodes, fusion_nodes);
  }

  vector<ge::NodePtr> trans1_nodes = GetMatchedNodesByDescName(PATTERN_TRANSDATA1, mapping);
  vector<ge::NodePtr> conv_nodes = GetMatchedNodesByDescName(PATTERN_CONV, mapping);
  vector<ge::NodePtr> depthwise_nodes = GetMatchedNodesByDescName(PATTERN_DEPTHWISECONV, mapping);
  bool conv_depth_size = conv_nodes.size() == 1 || depthwise_nodes.size() == 1;
  if (!conv_depth_size) {
    FE_LOGD("There is no conv and depthwise in TbeCommonRules0FusionPass");
    fusion_nodes.clear();
    return SUCCESS;
  }
  CheckStridedReadInConv2d(conv_nodes, fusion_nodes);
  vector<ge::NodePtr> conv_depthwise_nodes = conv_nodes.size() == 1 ? conv_nodes : depthwise_nodes;
  CheckTransNodes(trans1_nodes, conv_depthwise_nodes, fusion_nodes);
  vector<ge::NodePtr> dequant_nodes = GetMatchedNodesByDescName(PATTERN_DEQUANT, mapping);

  // if elewise has 2 input and inscope's shape less than outscope's shape, skip fusion
  if (!dequant_nodes.empty()) {
    if (JudgeElemShapeInScopeLessThanOutScope(dequant_nodes, elemwise_nodes, fusion_nodes)) {
      FE_LOGD(
          "dequant_nodes exist, Elemwise node has 2 inputs and in scope shape is less than outscope, try to fuse"
          " before elemwise nodes");
      return SUCCESS;
    }
  } else {
    if (JudgeElemShapeInScopeLessThanOutScope(conv_depthwise_nodes, elemwise_nodes, fusion_nodes)) {
      FE_LOGD(
          "no dequant_nodes, Elemwise node has 2 inputs and in scope shape is less than outscope, try to fuse"
          " before elemwise nodes");
      return SUCCESS;
    }
  }
  // elewise is in the blacklist, skip fusion
  if (!elemwise_nodes.empty()) {
    ge::NodePtr node = nullptr;
    if (IsInBlackListOfOpPatternElemwise(elemwise_nodes, node)) {
      FE_LOGD("node is in elemwise black_list, skip ub fusion!");
      fusion_nodes.clear();
      return SUCCESS;
    }
  }

  // in conv2_d+elewise(1~3) pattern, elewise has no restrictions,
  // if nums of elewise more then 3 and either one is not in the whitelist, skip fusion
  bool ret = (fusion_nodes.size() == (elemwise_nodes.size() + conv_depthwise_nodes.size())) &&
             (conv_depthwise_nodes.size() == 1) && !elemwise_nodes.empty();
  if (ret) {
    if (elemwise_nodes.size() <= 3) {
      return SUCCESS;
    } else {
      ge::NodePtr node = nullptr;
      if (!IsInWhiteListOfOpPatternElemwise(elemwise_nodes, node)) {
        fusion_nodes.clear();
      }
      return SUCCESS;
    }
  }

  DelNotMatchNodes(elemwise_nodes, fusion_nodes);

  if (fusion_nodes.size() == 1) {
    fusion_nodes.clear();
  }
  FE_LOGD("End to do TbeCommonRules0FusionPass!");
  return SUCCESS;
}
}  // namespace fe