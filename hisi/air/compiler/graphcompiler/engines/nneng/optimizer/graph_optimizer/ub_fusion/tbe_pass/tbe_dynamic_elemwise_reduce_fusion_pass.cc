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

#include "graph_optimizer/ub_fusion/tbe_pass/tbe_dynamic_elemwise_reduce_fusion_pass.h"
#include "common/fe_log.h"

namespace fe {

static const string PATTERN_REDUCE = "reduce";
static const string PATTERN_ELTWISE0 = "eltwise0";
static const string PATTERN_ELTWISE1 = "eltwise1";
static const string PATTERN_ELTWISE2 = "eltwise2";

/*
 * @brief: define reduce and elementwise op fusion pattern
 *
 * ElemWise_1-->Reduce-->ElemWise_2
 *
 * fusion node: ElemWise_1, ElemWise_2, Reduce
 *
 * @return BufferFusionPattern: return all valid patterns.
 */
vector<BufferFusionPattern *> TbeDynamicElemwiseReduceFusionPass::DefinePatterns() {
  vector<BufferFusionPattern *> patterns;

  string elemwise_reduce_pass_name = "TbeElemwiseReduceFusion";
  BufferFusionPattern *pattern = new (std::nothrow) BufferFusionPattern(elemwise_reduce_pass_name);
  FE_CHECK((pattern == nullptr), REPORT_FE_ERROR("[SubGraphOpt][DynmElemRdcFus][DefPtn] new an object failed."),
           return patterns);
  FE_LOGD("Start to define %s pass pattern.", elemwise_reduce_pass_name.c_str());
  // define pattern rules
  pattern
      ->AddOpDesc(PATTERN_ELTWISE0, {OP_PATTERN_ELEMWISE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT,
                  TBE_PATTERN_GROUPID_INVALID, ONLY_SUPPORT_DYNAMIC)
      .AddOpDesc(PATTERN_ELTWISE1, {OP_PATTERN_ELEMWISE}, TBE_PATTERN_NUM_NONE, TBE_PATTERN_NUM_MAX,
                 TBE_PATTERN_GROUPID_INVALID, ONLY_SUPPORT_DYNAMIC)
      .AddOpDesc(PATTERN_REDUCE, {OP_PATTERN_COMMONREDUCE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT,
                 TBE_PATTERN_GROUPID_INVALID, ONLY_SUPPORT_DYNAMIC)
      .AddOpDesc(PATTERN_ELTWISE2, {OP_PATTERN_ELEMWISE}, TBE_PATTERN_NUM_NONE, TBE_PATTERN_NUM_MAX,
                 TBE_PATTERN_GROUPID_INVALID, ONLY_SUPPORT_DYNAMIC)
      .SetHead({PATTERN_ELTWISE0})
      .SetOutputs(PATTERN_ELTWISE0, {PATTERN_ELTWISE1}, TBE_OUTPUT_BRANCH_SINGLE, true)
      .SetOutputs(PATTERN_ELTWISE1, {PATTERN_REDUCE}, TBE_OUTPUT_BRANCH_SINGLE, true)
      .SetOutputs(PATTERN_REDUCE, {PATTERN_ELTWISE2}, TBE_OUTPUT_BRANCH_SINGLE, true);
  patterns.push_back(pattern);
  FE_LOGD("End to define %s pass pattern by reduce and elemwise.", elemwise_reduce_pass_name.c_str());

  string elemwise_reduce_pass_name1 = "TbeElemwiseReduceFusion1";
  BufferFusionPattern *pattern1 = new (std::nothrow) BufferFusionPattern(elemwise_reduce_pass_name1);
  FE_CHECK((pattern1 == nullptr), REPORT_FE_ERROR("[SubGraphOpt][DynmElemRdcFus][DefPtn] new an object failed."),
           return patterns);
  FE_LOGD("Start to define %s pass pattern.", elemwise_reduce_pass_name1.c_str());
  // define pattern rules
  pattern1
      ->AddOpDesc(PATTERN_REDUCE, {OP_PATTERN_COMMONREDUCE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT,
                  TBE_PATTERN_GROUPID_INVALID, ONLY_SUPPORT_DYNAMIC)
      .AddOpDesc(PATTERN_ELTWISE2, {OP_PATTERN_ELEMWISE}, TBE_PATTERN_NUM_NONE, TBE_PATTERN_NUM_MAX,
                 TBE_PATTERN_GROUPID_INVALID, ONLY_SUPPORT_DYNAMIC)
      .SetHead({PATTERN_REDUCE})
      .SetOutputs(PATTERN_REDUCE, {PATTERN_ELTWISE2}, TBE_OUTPUT_BRANCH_SINGLE, true);
  patterns.push_back(pattern1);
  FE_LOGD("End to define %s pass pattern by reduce and elemwise.", elemwise_reduce_pass_name1.c_str());

  return patterns;
}

/*
 * @brief: parse nodes matched in mapping and call DoFusion
 * @param [in] graph: original graph
 * @param [out] mapping: nodes matched by pattern
 * @return bool: fusion status ok or not.
 */
Status TbeDynamicElemwiseReduceFusionPass::GetFusionNodes(const BufferFusionMapping &mapping,
                                                          vector<ge::NodePtr> &fusion_nodes) {
  FE_LOGD("Begin to do TbeDynamicElemwiseReduceFusionPass.");
  std::vector<ge::NodePtr> elemwise0_nodes = GetMatchedNodesByDescName(PATTERN_ELTWISE0, mapping);
  std::vector<ge::NodePtr> elemwise1_nodes = GetMatchedNodesByDescName(PATTERN_ELTWISE1, mapping);
  std::vector<ge::NodePtr> elemwise2_nodes = GetMatchedNodesByDescName(PATTERN_ELTWISE2, mapping);
  if (elemwise0_nodes.empty() && elemwise1_nodes.empty() && elemwise2_nodes.empty()) {
    FE_LOGD("Both elemwise nodes before reduce node and after reduce node are empty. No need to do fusion.");
    return SUCCESS;
  }

  fusion_nodes = GetMatchedNodes(mapping);
  FE_LOGD("End to do TbeDynamicElemwiseReduceFusionPass.");
  return SUCCESS;
}
}  // namespace fe