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

#ifndef AIR_COMPILER_ENGINES_NNENG_OPTIMIZER_GRAPH_OPTIMIZER_OP_JUDGE_FORMAT_AND_DTYPE_STRATEGY_DTYPE_STRATEGY_OP_DTYPE_SELECTION_STRATEGY_DEFAULT_MODE_H_
#define AIR_COMPILER_ENGINES_NNENG_OPTIMIZER_GRAPH_OPTIMIZER_OP_JUDGE_FORMAT_AND_DTYPE_STRATEGY_DTYPE_STRATEGY_OP_DTYPE_SELECTION_STRATEGY_DEFAULT_MODE_H_

#include "graph_optimizer/op_judge/format_and_dtype/strategy/matcher/dtype/op_dtype_rise_matcher.h"
#include "op_dtype_selection_strategy_base.h"

namespace fe {
using OpDtypeRiseMatcherPtr = std::shared_ptr<OpDtypeRiseMatcher>;

/** @brief This class is created for selecting data type in default mode.
* Default mode is described as:
* we must select the original data type and if the original data type is not
* supported in the ops kernel, we will try it's higher precision version and if
* the higher precision version is not supported, we will just return ERROR and
* stop the program. */
class OpDtypeSelectionStrategyDefaultMode : public OpDtypeSeletionStrategyBase {
 public:
  explicit OpDtypeSelectionStrategyDefaultMode(FormatDtypeQuerierPtr format_dtype_querier_ptr,
                                               OpDtypeRiseMatcherPtr op_dtype_rise_matcher_ptr);

  ~OpDtypeSelectionStrategyDefaultMode() override;

  /* In this mode we will match the original dtype first. The matching function
   * allows using higher precision. */
  Status Run(FormatDtypeSelectionBasicInfo& basic_info, ForbiddenDtype forbidden_dtype) override;

 private:
  OpDtypeRiseMatcherPtr op_dtype_rise_matcher_ptr_;
};
}
#endif  // AIR_COMPILER_ENGINES_NNENG_OPTIMIZER_GRAPH_OPTIMIZER_OP_JUDGE_FORMAT_AND_DTYPE_STRATEGY_DTYPE_STRATEGY_OP_DTYPE_SELECTION_STRATEGY_DEFAULT_MODE_H_
