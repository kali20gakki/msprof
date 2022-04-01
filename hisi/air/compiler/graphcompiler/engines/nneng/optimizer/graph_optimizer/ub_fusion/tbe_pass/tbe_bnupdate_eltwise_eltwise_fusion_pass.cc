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

#include "graph_optimizer/ub_fusion/tbe_pass/tbe_bnupdate_eltwise_eltwise_fusion_pass.h"
#include <algorithm>
#include <string>
#include <vector>

namespace fe {
using std::vector;
namespace {
static const char PATTERN_BNUPDATE[] = "bnupdate";
static const char PATTERN_ELTWISE1[] = "eltwise1";
static const char PATTERN_ELTWISE0[] = "eltwise0";
static const char PATTERN_OUTPUT0[] = "Output0";
static const char PATTERN_OUTPUT1[] = "Output1";
static const char PATTERN_OUTPUT2[] = "Output2";
static const char PATTERN_OUTPUT3[] = "Output3";
static const char PATTERN_OUTPUT4[] = "Output4";
static const char PATTERN_OUTPUT5[] = "Output5";
static const char PATTERN_OTHER_INPUT[] = "otherInput";
const int NODE_OUTPUT_SIZE = 2;
const int NODE_INPUT_SIZE = 2;
}

/*
 * @brief:  define_bn_update and Elemwise and Elemwiseinput op fusion pattern
 *
 *   BnUpdate and Elemwise and Elemwise
 *
 * fusion node: BnUpdate and Elemwise and Elemwise
 *
 * @return BufferFusionPattern: return all valid patterns.
 */
vector<BufferFusionPattern *> TbeBnupdateEltwiseEltwiseFusionPass::DefinePatterns() {
  vector<BufferFusionPattern *> patterns;
  string pass_name = "TbeBNUpdateEltwiseEltwiseFusionPass";

  int max_count = 7;
  BufferFusionPattern *pattern = new (std::nothrow) BufferFusionPattern(pass_name, TBE_FUSION_OP_NUM_MAX + max_count);
  FE_CHECK((pattern == nullptr),
           REPORT_FE_ERROR("[SubGraphOpt][BnUpdateFus][DefPtn] new an object failed."), return patterns);
  FE_LOGD("Start to define %s pass pattern.", pass_name.c_str());
  // define pattern rules
  pattern->AddOpDesc(PATTERN_BNUPDATE, {OP_PATTERN_BNUPDATE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
      .AddOpDesc(PATTERN_ELTWISE0, {OP_PATTERN_ELEMWISE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
      .AddOpDesc(PATTERN_ELTWISE1, {OP_PATTERN_ELEMWISE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
      .AddOpDesc(PATTERN_OUTPUT0, {TBE_PATTERN_OUTPUT_NODE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
      .AddOpDesc(PATTERN_OUTPUT1, {TBE_PATTERN_OUTPUT_NODE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
      .AddOpDesc(PATTERN_OUTPUT2, {TBE_PATTERN_OUTPUT_NODE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
      .AddOpDesc(PATTERN_OUTPUT3, {TBE_PATTERN_OUTPUT_NODE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
      .AddOpDesc(PATTERN_OUTPUT4, {TBE_PATTERN_OUTPUT_NODE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
      .AddOpDesc(PATTERN_OUTPUT5, {TBE_PATTERN_OUTPUT_NODE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
      .AddOpDesc(PATTERN_OTHER_INPUT, {TBE_PATTERN_INPUT_NODE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
      .SetHead({PATTERN_BNUPDATE})
      .SetOutputs(PATTERN_BNUPDATE, {PATTERN_ELTWISE0, PATTERN_OUTPUT0, PATTERN_OUTPUT1, PATTERN_OUTPUT2,
                                     PATTERN_OUTPUT3, PATTERN_OUTPUT4, PATTERN_OUTPUT5},
                  TBE_OUTPUT_BRANCH_MULTI)
      .SetOutputs(PATTERN_ELTWISE0, {PATTERN_ELTWISE1})
      .SetOutputs(PATTERN_OTHER_INPUT, {PATTERN_ELTWISE0});

  patterns.push_back(pattern);
  FE_LOGD("End to define %s pass pattern.", pass_name.c_str());
  return patterns;
}

/*
 * @brief: parse nodes matched in mapping and call DoFusion
 * @param [in] graph: original graph
 * @param [out] mapping: nodes matched by pattern
 * @return bool: fusion status ok or not.
 */
Status TbeBnupdateEltwiseEltwiseFusionPass::GetFusionNodes(const BufferFusionMapping &mapping,
                                                           vector<ge::NodePtr> &fusion_nodes) {
  FE_LOGD("Begin to do TbeBNUpdateEltwiseEltwiseFusionPass!");
  for (auto &item : mapping) {
    auto elem_opdesc = find(item.first->types.begin(), item.first->types.end(), OP_PATTERN_ELEMWISE);
    if (elem_opdesc == item.first->types.end()) {
      continue;
    }
    for (auto &node : item.second) {
      if (node->GetOpDesc()->GetOutputsSize() != NODE_OUTPUT_SIZE &&
          node->GetOpDesc()->GetAllInputsSize() != NODE_INPUT_SIZE) {
        FE_LOGD("The number of node[%s] output is [%zu], which is not equal to two, no need to do fusion.",
            node->GetName().c_str(), node->GetOpDesc()->GetAllOutputsDesc().size());
        return SUCCESS;
      }
    }
  }

  fusion_nodes = GetMatchedNodes(mapping);
  for (auto &item : mapping) {
    auto opdesc = find(item.first->types.begin(), item.first->types.end(), TBE_PATTERN_OUTPUT_NODE);
    if (opdesc != item.first->types.end()) {
      for (auto &node : item.second) {
        auto node_ptr = find(fusion_nodes.begin(), fusion_nodes.end(), node);
        if (node_ptr != fusion_nodes.end()) {
          fusion_nodes.erase(node_ptr);
        }
      }
    }
  }
  FE_LOGD("End to do TbeBNUpdateEltwiseEltwiseFusionPass!");
  return SUCCESS;
}
}  // namespace fe
