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

#include "graph_optimizer/ub_fusion/tbe_pass/tbe_bnupdate_eltwise_fusion_pass.h"
#include <algorithm>
#include <string>
#include <vector>

namespace fe {
using std::vector;
namespace {
static const char PATTERN_BNUPDATE[] = "bnupdate";
static const char PATTERN_OUTPUT0[] = "OUTPUT0";
static const char PATTERN_OUTPUT1[] = "OUTPUT1";
static const char PATTERN_OUTPUT2[] = "OUTPUT2";
static const char PATTERN_OUTPUT3[] = "OUTPUT3";
static const char PATTERN_OUTPUT4[] = "OUTPUT4";
static const char PATTERN_OUTPUT5[] = "OUTPUT5";
static const char PATTERN_ELTWISE[] = "eltwise";
const int NODE_OUTPUT_SIZE = 2;
const int BN_NODE_OUTPUT_SIZE = 5;
}

/*
 * @brief:  define BNUpadte and ElemWise input op fusion pattern
 *
 *   BNReduce + ElemWise
 *
 * fusion node:  BNReduce, ElemWise
 *
 * @return BufferFusionPattern: return all valid patterns.
 */
vector<BufferFusionPattern *> TbeBnupdateEltwiseFusionPass::DefinePatterns() {
  vector<BufferFusionPattern *> patterns;
  string pass_name = "TbeBNUpdateEltwiseFusionPass";

  int max_count = 6;
  BufferFusionPattern *pattern = new (std::nothrow) BufferFusionPattern(pass_name, TBE_FUSION_OP_NUM_MAX + max_count);
  FE_CHECK((pattern == nullptr),
           REPORT_FE_ERROR("[SubGraphOpt][BnUpdateFus][DefPtn] new an object failed."), return patterns);
  FE_LOGD("Start to define %s pass pattern.", pass_name.c_str());
  // define pattern rules
  pattern->AddOpDesc(PATTERN_BNUPDATE, {OP_PATTERN_BNUPDATE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
      .AddOpDesc(PATTERN_ELTWISE, {OP_PATTERN_ELEMWISE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
      .AddOpDesc(PATTERN_OUTPUT0, {TBE_PATTERN_OUTPUT_NODE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
      .AddOpDesc(PATTERN_OUTPUT1, {TBE_PATTERN_OUTPUT_NODE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
      .AddOpDesc(PATTERN_OUTPUT2, {TBE_PATTERN_OUTPUT_NODE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
      .AddOpDesc(PATTERN_OUTPUT3, {TBE_PATTERN_OUTPUT_NODE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
      .AddOpDesc(PATTERN_OUTPUT4, {TBE_PATTERN_OUTPUT_NODE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
      .AddOpDesc(PATTERN_OUTPUT5, {TBE_PATTERN_OUTPUT_NODE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
      .SetHead({PATTERN_BNUPDATE})
      .SetOutputs(PATTERN_BNUPDATE, {PATTERN_ELTWISE, PATTERN_OUTPUT0, PATTERN_OUTPUT1, PATTERN_OUTPUT2,
                                     PATTERN_OUTPUT3, PATTERN_OUTPUT4, PATTERN_OUTPUT5},
                  TBE_OUTPUT_BRANCH_MULTI);

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
Status TbeBnupdateEltwiseFusionPass::GetFusionNodes(const BufferFusionMapping &mapping,
                                                    vector<ge::NodePtr> &fusion_nodes) {
  FE_LOGD("Begin to do TbeBNUpdateEltwiseFusionPass!");

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

  FE_LOGD("End to do TbeBNUpdateEltwiseFusionPass!");
  return SUCCESS;
}
}  // namespace fe
