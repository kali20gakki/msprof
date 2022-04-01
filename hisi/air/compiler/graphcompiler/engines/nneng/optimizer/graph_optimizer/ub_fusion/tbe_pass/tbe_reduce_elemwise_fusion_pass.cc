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

#include "graph_optimizer/ub_fusion/tbe_pass/tbe_reduce_elemwise_fusion_pass.h"
#include <string>
#include <vector>

namespace fe {

static const string PATTERN_REDUCE1 = "reduce1";
static const string PATTERN_REDUCE2 = "reduce2";
static const string PATTERN_ELTWISE1 = "eltwise1";
static const string PATTERN_ELTWISE2 = "eltwise2";
static const string PATTERN_ELTWISE3 = "eltwise3";
static const string PATTERN_ELTWISE4 = "eltwise4";
static const string PATTERN_OTHER_INPUT = "otherInput";

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
vector<BufferFusionPattern *> TbeReduceElemwiseFusionPass::DefinePatterns() {
  vector<BufferFusionPattern *> patterns;
  string pass_name = "TbeReduceEltwiseFusion";
  int64_t group_id = 0;

  BufferFusionPattern *pattern0 = new (std::nothrow) BufferFusionPattern(pass_name);
  FE_CHECK((pattern0 == nullptr),
           REPORT_FE_ERROR("[SubGraphOpt][TbeRdcElemFus][DefPtn] new an object failed."), return patterns);
  FE_LOGD("Start to define %s pass pattern0.", pass_name.c_str());
  // define pattern rules
  pattern0->AddOpDesc(PATTERN_ELTWISE1, {OP_PATTERN_ELEMWISE}, TBE_PATTERN_NUM_NONE, TBE_PATTERN_NUM_DEFAULT)
      .AddOpDesc(PATTERN_ELTWISE2, {OP_PATTERN_ELEMWISE}, TBE_PATTERN_NUM_NONE, TBE_PATTERN_NUM_MAX)
      .AddOpDesc(PATTERN_ELTWISE3, {OP_PATTERN_ELEMWISE}, TBE_PATTERN_NUM_NONE, TBE_PATTERN_NUM_MAX)
      .AddOpDesc(PATTERN_ELTWISE4, {OP_PATTERN_ELEMWISE}, TBE_PATTERN_NUM_NONE, TBE_PATTERN_NUM_MAX)
      .AddOpDesc(PATTERN_REDUCE1, {OP_PATTERN_COMMONREDUCE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT, group_id)
      .AddOpDesc(PATTERN_REDUCE2, {OP_PATTERN_COMMONREDUCE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT, group_id)
      .SetHead({PATTERN_REDUCE1, PATTERN_ELTWISE1})
      .SetOutputs(PATTERN_ELTWISE1, {PATTERN_ELTWISE2, PATTERN_REDUCE1}, TBE_OUTPUT_BRANCH_SINGLE)
      .SetOutputs(PATTERN_ELTWISE2, {PATTERN_REDUCE2})
      .SetOutputs(PATTERN_REDUCE1, {PATTERN_ELTWISE3})
      .SetOutputs(PATTERN_REDUCE2, {PATTERN_ELTWISE4});

  patterns.push_back(pattern0);
  FE_LOGD("End to define %s pass pattern by reduce elemwise.", pass_name.c_str());

  return patterns;
}
}  // namespace fe
