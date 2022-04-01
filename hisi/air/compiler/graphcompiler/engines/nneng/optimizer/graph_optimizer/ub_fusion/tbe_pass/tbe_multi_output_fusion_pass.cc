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

#include "graph_optimizer/ub_fusion/tbe_pass/tbe_multi_output_fusion_pass.h"
#include <string>
#include <vector>

namespace fe {

static const string PATTERN_ELTWISE1 = "eltwise1";        // desc name
static const string PATTERN_ELTWISE2 = "eltwise2";        // desc name
static const string PATTERN_ELTWISE3 = "eltwise3";        // desc name
static const string PATTERN_ELTWISE4 = "eltwise4";        // desc name
static const string PATTERN_ELTWISE5 = "eltwise5";        // desc name
static const string PATTERN_OTHER_OUTPUT = "OutputData";  // desc name
static const string PATTERN_OTHER_INPUT = "InputData";    // desc name

/*
 * @brief:  define multi output fusion pattern
 *
 * pattern1:
 *    (ElemWise_1)-->(ElemWise_2)-->ElemWise_3-->ElemWise_4
 *                                      |
 *                                      V
 *                                  ElemWise_5
 *
 * fusion node: ElemWise_1, ElemWise_2, ElemWise_3, ElemWise_4, ElemWise_5
 *
 * pattern2:
 *    (ElemWise_1)-->(ElemWise_2)-->ElemWise_3-->ElemWise_4
 *                                      |
 *                                      V
 *                                 OtherOutput
 *
 * fusion node: ElemWise_1, ElemWise_2, ElemWise_3, ElemWise_4
 *
 * @return BufferFusionPattern: return all valid patterns.
 */
vector<BufferFusionPattern *> TbeMultiOutputFusionPass::DefinePatterns() {
  vector<BufferFusionPattern *> patterns;

  string pass_name1 = "TbeMultiOutputTwoEltwiseFusion";

  BufferFusionPattern *pattern1 = new (std::nothrow) BufferFusionPattern(pass_name1);
  FE_CHECK((pattern1 == nullptr),
           REPORT_FE_ERROR("[SubGraphOpt][TbeMultiOutFus][DefPtn] new an object failed."), return patterns);

  FE_LOGD("Start to define %s pass pattern", pass_name1.c_str());
  // define pattern rules
  pattern1->AddOpDesc(PATTERN_ELTWISE1, {OP_PATTERN_ELEMWISE}, TBE_PATTERN_NUM_NONE, TBE_PATTERN_NUM_DEFAULT)
      .AddOpDesc(PATTERN_ELTWISE2, {OP_PATTERN_ELEMWISE}, TBE_PATTERN_NUM_NONE, TBE_PATTERN_NUM_MAX)
      .AddOpDesc(PATTERN_ELTWISE3, {OP_PATTERN_ELEMWISE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
      .AddOpDesc(PATTERN_ELTWISE4, {OP_PATTERN_ELEMWISE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_MAX)
      .AddOpDesc(PATTERN_ELTWISE5, {OP_PATTERN_ELEMWISE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
      .SetHead({PATTERN_ELTWISE1, PATTERN_ELTWISE3})
      .SetOutputs(PATTERN_ELTWISE1, {PATTERN_ELTWISE2})
      .SetOutputs(PATTERN_ELTWISE2, {PATTERN_ELTWISE3}, TBE_OUTPUT_BRANCH_SINGLE, true, true)
      .SetOutputs(PATTERN_ELTWISE3, {PATTERN_ELTWISE4, PATTERN_ELTWISE5}, TBE_OUTPUT_BRANCH_MULTI, true, false)
      .SetOutputs(PATTERN_ELTWISE4, {}, TBE_OUTPUT_BRANCH_DEFAULT, true, true)
      .SetOutputs(PATTERN_ELTWISE5, {}, TBE_OUTPUT_BRANCH_DEFAULT, true, true);

  // check whether there is error while create pattern, delete pattern if yes.
  patterns.push_back(pattern1);
  FE_LOGD("End to define %s pass pattern by tbe multi output", pass_name1.c_str());

  string pass_name2 = "TbeMultiOutputFusion";

  BufferFusionPattern *pattern2 = new (std::nothrow) BufferFusionPattern(pass_name2);
  FE_CHECK((pattern2 == nullptr),
           REPORT_FE_ERROR("[SubGraphOpt][TbeMultiOutFus][DefPtn] new an object failed."), return patterns);

  FE_LOGD("Start to define %s pass pattern", pass_name2.c_str());
  // define pattern rules
  pattern2->AddOpDesc(PATTERN_ELTWISE1, {OP_PATTERN_ELEMWISE}, TBE_PATTERN_NUM_NONE, TBE_PATTERN_NUM_DEFAULT)
      .AddOpDesc(PATTERN_ELTWISE2, {OP_PATTERN_ELEMWISE}, TBE_PATTERN_NUM_NONE, TBE_PATTERN_NUM_MAX)
      .AddOpDesc(PATTERN_ELTWISE3, {OP_PATTERN_ELEMWISE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
      .AddOpDesc(PATTERN_ELTWISE4, {OP_PATTERN_ELEMWISE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_MAX)
      .AddOpDesc(PATTERN_OTHER_OUTPUT, {TBE_PATTERN_OUTPUT_NODE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
      .SetHead({PATTERN_ELTWISE1, PATTERN_ELTWISE3})
      .SetOutputs(PATTERN_ELTWISE1, {PATTERN_ELTWISE2})
      .SetOutputs(PATTERN_ELTWISE2, {PATTERN_ELTWISE3}, TBE_OUTPUT_BRANCH_SINGLE, true, true)
      .SetOutputs(PATTERN_ELTWISE3, {PATTERN_ELTWISE4, PATTERN_OTHER_OUTPUT}, TBE_OUTPUT_BRANCH_MULTI, true, false)
      .SetOutputs(PATTERN_ELTWISE4, {}, TBE_OUTPUT_BRANCH_DEFAULT, true, true)
      .SetOutputs(PATTERN_OTHER_OUTPUT, {}, TBE_OUTPUT_BRANCH_DEFAULT, true, true);

  // check whether there is error while create pattern, delete pattern if yes.
  patterns.push_back(pattern2);
  FE_LOGD("End to define %s pass pattern by tbe multi output", pass_name2.c_str());
  return patterns;
}

/*
 * @brief: parse nodes matched in mapping and call DoFusion
 * @param [in] graph: original graph
 * @param [out] mapping: nodes matched by pattern
 * @return bool: fusion status ok or not.
 */
Status TbeMultiOutputFusionPass::GetFusionNodes(const BufferFusionMapping &mapping, vector<ge::NodePtr> &fusion_nodes) {
  FE_LOGD("Begin to do TbeMultiOutputFusionPass!");
  fusion_nodes = GetMatchedNodes(mapping);
  // the output_data can't be fused
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
  FE_LOGD("End to do TbeMultiOutputFusionPass!");
  return SUCCESS;
}

}  // namespace fe
