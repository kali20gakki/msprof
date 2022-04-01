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

#include "tbe_eltwise_write_select_fussion_pass.h"
#include <algorithm>
#include <string>
#include <vector>

namespace fe {
using std::vector;

namespace {
static const char PATTERN_ELTWISE[] = "ElemWise";
static const char PATTERN_WRITE_SELECT[] = "write_select";
}

/*
* @brief:  define eltwise and write_select ops fusion pattern
*
*   ElemWise-->WriteSelect

* @return TbeFusionPattern: return all valid patterns.
*/
vector<BufferFusionPattern *> TbeEltwiseWriteSelectFusionPass::DefinePatterns() {
  vector<BufferFusionPattern *> patterns;

  string pass_name = "TbeEltwiseWriteSelectFusionPass";
  BufferFusionPattern *pattern = new (std::nothrow) BufferFusionPattern(pass_name);
  FE_CHECK((pattern == nullptr),
           REPORT_FE_ERROR("[SubGraphOpt][TbeElemWrtSelcFus][DefPtn] new an object failed."), return patterns);
  FE_LOGD("Start to define %s pass pattern.", pass_name.c_str());
  pattern->AddOpDesc(PATTERN_ELTWISE, {OP_PATTERN_ELEMWISE}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
      .AddOpDesc(PATTERN_WRITE_SELECT, {OP_PATTERN_WRITE_SELECT}, TBE_PATTERN_NUM_DEFAULT, TBE_PATTERN_NUM_DEFAULT)
      .SetHead({PATTERN_ELTWISE})
      .SetOutputs(PATTERN_ELTWISE, {PATTERN_WRITE_SELECT});

  patterns.push_back(pattern);
  FE_LOGD("End to define %s pass pattern.", pass_name.c_str());

  return patterns;
}

}  // namespace fe
