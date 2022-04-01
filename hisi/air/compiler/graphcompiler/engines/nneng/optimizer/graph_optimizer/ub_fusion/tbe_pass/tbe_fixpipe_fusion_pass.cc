/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2021. All rights reserved.
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
#include "graph_optimizer/ub_fusion/tbe_pass/tbe_fixpipe_fusion_pass.h"
#include <algorithm>
#include <string>
#include <vector>
#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/fixpipe_common.h"
#include "common/aicore_util_constants.h"
#include "common/configuration.h"
#include "common/fe_utils.h"
#include "common/op_info_common.h"

namespace fe {
using std::vector;

const std::string kPatternTransData1 = "transdata1";
const std::string kPatternCube = "cube";
const std::string kPatternFixpipe = "fixpipe";
const std::string kPatternElemwise = "elemwise";
const std::string kPatternStrideWrite = "stridedwrite";

vector<BufferFusionPattern *> TbeFixPipeFusionPass::DefinePatterns() {
  vector<BufferFusionPattern *> patterns;
  string pass_name = "TbeFixPipeFusionPass";
  BufferFusionPattern *pattern = new (std::nothrow) BufferFusionPattern(pass_name);
  FE_CHECK((pattern == nullptr), REPORT_FE_ERROR("[SubGraphOpt][FixPipeRules][DefPtn] New an object failed."),
           return patterns);
  FE_LOGD("Start to define %s pass pattern.", pass_name.c_str());
  // define pattern rules
  pattern
      ->AddOpDesc(kPatternTransData1, {TRANSDATA}, TBE_PATTERN_NUM_NONE, TBE_PATTERN_NUM_DEFAULT,
                  TBE_PATTERN_GROUPID_INVALID, IGNORE_SHAPE_TYPE, true)
      .AddOpDesc(kPatternCube, {OP_PATTERN_CONV, OP_PATTERN_GEMM}, TBE_PATTERN_NUM_NONE, TBE_PATTERN_NUM_DEFAULT,
                 TBE_PATTERN_GROUPID_INVALID, IGNORE_SHAPE_TYPE)
      .AddOpDesc(kPatternFixpipe, {OP_PATTERN_FIXPIPE}, TBE_PATTERN_NUM_NONE, TBE_PATTERN_NUM_DEFAULT,
                 TBE_PATTERN_GROUPID_INVALID, IGNORE_SHAPE_TYPE)
      .AddOpDesc(kPatternStrideWrite, {OP_PATTERN_STRIDED_WRITE}, TBE_PATTERN_NUM_NONE, TBE_PATTERN_NUM_DEFAULT,
                 TBE_PATTERN_GROUPID_INVALID, IGNORE_SHAPE_TYPE)
      .SetHead({kPatternCube, kPatternTransData1})
      .SetOutputs(kPatternTransData1, {kPatternCube})
      .SetOutputs(kPatternCube, {kPatternFixpipe}, TBE_OUTPUT_BRANCH_SINGLE, true)
      .SetOutputs(kPatternFixpipe, {kPatternStrideWrite}, TBE_OUTPUT_BRANCH_SINGLE, true, true)
      .SetOutputs(kPatternStrideWrite, {}, TBE_OUTPUT_BRANCH_SINGLE, true, true);
  if (CheckCubeVecState() == CubeVecState::CUBE_VEC_MIX) {
    pattern->AddOpDesc(kPatternElemwise, {OP_PATTERN_ELEMWISE}, TBE_PATTERN_NUM_NONE, TBE_PATTERN_NUM_MAX,
                       TBE_PATTERN_GROUPID_INVALID, IGNORE_SHAPE_TYPE)
           .SetOutputs(kPatternFixpipe, {kPatternElemwise}, TBE_OUTPUT_BRANCH_SINGLE, true, true)
           .SetOutputs(kPatternElemwise, {}, TBE_OUTPUT_BRANCH_SINGLE, true, true);
  }

  patterns.push_back(pattern);

  FE_LOGD("End to define %s pass pattern.", pass_name.c_str());
  return patterns;
}

void TbeFixPipeFusionPass::CheckFixPipeNode(const ge::NodePtr &cube_node, const ge::NodePtr &fixpipe_node,
                                            vector<ge::NodePtr> &fusion_nodes) const {
  for (const auto &out_anchor : cube_node->GetAllOutDataAnchors()) {
    if (out_anchor == nullptr) {
      continue;
    }
    auto peer_in_anchors = out_anchor->GetPeerInDataAnchors();
    for (const auto &peer_in_anchor : peer_in_anchors) {
      if (peer_in_anchor == nullptr || peer_in_anchor->GetOwnerNode() == nullptr) {
        continue;
      }
      if (peer_in_anchor->GetOwnerNode() == fixpipe_node) {
        if (peer_in_anchor->GetIdx() == 0) {
          continue;
        } else {
          fusion_nodes.clear();
        }
      } else {
        continue;
      }
    }
  }
}

/*
 * @brief: parse nodes matched in mapping and call DoFusion
 * @param [in] graph: original graph
 * @param [out] mapping: nodes matched by pattern
 * @return bool: fusion status ok or not.
 */
Status TbeFixPipeFusionPass::GetFusionNodes(const BufferFusionMapping &mapping, vector<ge::NodePtr> &fusion_nodes) {
  FE_LOGD("Begin to do TbeFixPipeFusionPass!");
  CubeVecState cube_vec_state = CheckCubeVecState();
  if (cube_vec_state == CubeVecState::CUBE_VEC_UNKNOWN) {
    return SUCCESS;
  }
  vector<ge::NodePtr> cube_nodes = GetMatchedNodesByDescName(kPatternCube, mapping);
  vector<ge::NodePtr> trans1_nodes = GetMatchedNodesByDescName(kPatternTransData1, mapping);
  vector<ge::NodePtr> fixpipe_nodes = GetMatchedNodesByDescName(kPatternFixpipe, mapping);
  if (cube_nodes.empty() || cube_nodes[0] == nullptr) {
    return SUCCESS;
  }
  fusion_nodes = GetMatchedNodes(mapping);

  CheckFusionTransNode(cube_nodes[0], trans1_nodes, fusion_nodes);
  // check fixpipe node should be final action
  if (!fixpipe_nodes.empty()) {
    CheckFixPipeNode(cube_nodes[0], fixpipe_nodes[0], fusion_nodes);
  }
  if (fusion_nodes.size() == 1) {
    fusion_nodes.clear();
  }
  return SUCCESS;
}
}  // namespace fe
