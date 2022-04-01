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

#include "graph_optimizer/ub_fusion/tbe_pass/tbe_eltwise_fusion_pass.h"
#include <string>
#include <vector>

namespace fe {

static const string PATTERN_ELTWISE1 = "eltwise1";
static const string PATTERN_ELTWISE2 = "eltwise2";
static const string PATTERN_OTHER_INPUT = "otherInput";

/*
 * @brief:  define multi elementwise fusion pattern
 *
 * pattern configuration limit:
 * 1. total min value must be 1 for all head candidated desc.
 * 2. any head candidated desc max value must be 1.
 * 3. output desc can not be itself.
 *
 *    1) ElemWise_1-->ElemWise_2
 *
 * fusion node: ElemWise_1, ElemWise_2
 *
 * @return BufferFusionPattern: return all valid patterns.
 */
vector<BufferFusionPattern *> TbeEltwiseFusionPass::DefinePatterns() {
  vector<BufferFusionPattern *> patterns;
  const string pass_name = "TbeEltwiseFusion";

  BufferFusionPattern *pattern = new (std::nothrow) BufferFusionPattern(pass_name);

  FE_CHECK((pattern == nullptr),
           REPORT_FE_ERROR("[SubGraphOpt][TbeElemFus][DefPtn] new an object failed."), return patterns);

  FE_LOGD("Start to define %s pass pattern.", pass_name.c_str());

  // define pattern rules
  pattern->AddOpDesc(PATTERN_ELTWISE1, {OP_PATTERN_ELEMWISE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
      .AddOpDesc(PATTERN_ELTWISE2, {OP_PATTERN_ELEMWISE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_MAX)
      .SetHead({PATTERN_ELTWISE1})
      .SetOutputs(PATTERN_ELTWISE1, {PATTERN_ELTWISE2});

  patterns.push_back(pattern);
  FE_LOGD("End to define %s pass pattern.", pass_name.c_str());
  return patterns;
}
}  // namespace fe
