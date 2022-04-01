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

#include "graph_optimizer/ub_fusion/tbe_pass/tbe_dynamic_elemwise_broadcast_fusion_pass.h"
#include "common/fe_log.h"

namespace fe {

static const string PATTERN_ELTWISE_BROADCAST_1 = "eltwise_or_broadcast_1";
static const string PATTERN_ELTWISE_BROADCAST_2 = "eltwise_or_broadcast_2";

/*
 * @brief: define reduce and elementwise op fusion pattern
 *
 * pattern configuration limit:
 * 1. total min value must be 1 for all head candidated desc.
 * 2. any head candidated desc max value must be 1.
 * 3. output desc can not be itself.
 *
 *    1) Reduce-->ElemWise_1-->ElemWise_2
 *    2) ElemWise_1-->Reduce-->ElemWise_2
 *    3)ElemWise_1-->ElemWise_2-->Reduce
 *
 * fusion node: ElemWise_1, ElemWise_2, Reduce
 *
 * @return BufferFusionPattern: return all valid patterns.
 */
vector<BufferFusionPattern *> TbeDynamicElemwiseBroadcastFusionPass::DefinePatterns() {
  vector<BufferFusionPattern *> patterns;
  string elemwise_broadcast_pass_name = "TbeElemwiseBroadcastFusion";

  BufferFusionPattern *pattern = new (std::nothrow) BufferFusionPattern(elemwise_broadcast_pass_name);
  FE_CHECK((pattern == nullptr), REPORT_FE_ERROR("[SubGraphOpt][DymElemBroadFus][DefPtn] new an object failed."),
           return patterns);
  FE_LOGD("Start to define %s pass pattern.", elemwise_broadcast_pass_name.c_str());
  // define pattern rules
  pattern
      ->AddOpDesc(PATTERN_ELTWISE_BROADCAST_1, {OP_PATTERN_ELEMWISE, OP_PATTERN_BROAD_CAST}, TBE_PATTERN_NUM_DEFAULT,
                  TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_GROUPID_INVALID, ONLY_SUPPORT_DYNAMIC)
      .AddOpDesc(PATTERN_ELTWISE_BROADCAST_2, {OP_PATTERN_ELEMWISE, OP_PATTERN_BROAD_CAST}, TBE_PATTERN_NUM_DEFAULT,
                 TBE_PATTERN_NUM_MAX, TBE_PATTERN_GROUPID_INVALID, ONLY_SUPPORT_DYNAMIC)
      .SetHead({PATTERN_ELTWISE_BROADCAST_1})
      .SetOutputs(PATTERN_ELTWISE_BROADCAST_1, {PATTERN_ELTWISE_BROADCAST_2}, TBE_OUTPUT_BRANCH_SINGLE, true)
      .SetOutputs(PATTERN_ELTWISE_BROADCAST_2, {}, TBE_OUTPUT_BRANCH_SINGLE, true);

  patterns.push_back(pattern);
  FE_LOGD("End to define %s pass pattern by broadcast and elemwise.", elemwise_broadcast_pass_name.c_str());

  return patterns;
}
}  // namespace fe