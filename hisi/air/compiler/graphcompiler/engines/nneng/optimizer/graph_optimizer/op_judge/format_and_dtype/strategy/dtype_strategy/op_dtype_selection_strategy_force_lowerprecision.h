/**
 * Copyright 2019-2021 Huawei Technologies Co., Ltd
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

#ifndef AIR_COMPILER_ENGINES_NNENG_OPTIMIZER_GRAPH_OPTIMIZER_OP_JUDGE_FORMAT_AND_DTYPE_STRATEGY_DTYPE_STRATEGY_OP_DTYPE_SELECTION_STRATEGY_FORCE_LOWERPRECISION_H_
#define AIR_COMPILER_ENGINES_NNENG_OPTIMIZER_GRAPH_OPTIMIZER_OP_JUDGE_FORMAT_AND_DTYPE_STRATEGY_DTYPE_STRATEGY_OP_DTYPE_SELECTION_STRATEGY_FORCE_LOWERPRECISION_H_

#include "graph_optimizer/op_judge/format_and_dtype/strategy/matcher/dtype/op_dtype_rise_matcher.h"
#include "graph_optimizer/op_judge/format_and_dtype/strategy/matcher/dtype/op_dtype_precise_matcher.h"
#include "op_dtype_selection_strategy_base.h"

namespace fe {
using OpDtypeRiseMatcherPtr = std::shared_ptr<OpDtypeRiseMatcher>;
using OpDtypePreciseMatcherPtr = std::shared_ptr<OpDtypePreciseMatcher>;

class OpDtypeSelectionStrategyForceLowerPrecision : public OpDtypeSeletionStrategyBase {
 public:
  explicit OpDtypeSelectionStrategyForceLowerPrecision(FormatDtypeQuerierPtr format_dtype_querier_ptr,
    OpDtypeRiseMatcherPtr op_dtype_rise_matcher_ptr, OpDtypePreciseMatcherPtr op_dtype_precise_matcher_ptr);

  ~OpDtypeSelectionStrategyForceLowerPrecision() override;

  /* In this mode we will match the dtype fp16 first. If the */
  Status Run(FormatDtypeSelectionBasicInfo& basic_info, ForbiddenDtype forbidden_dtype) override;

 private:
  OpDtypeRiseMatcherPtr op_dtype_rise_matcher_ptr_;
  OpDtypePreciseMatcherPtr op_dtype_precise_matcher_ptr_;
};
}
#endif  // AIR_COMPILER_ENGINES_NNENG_OPTIMIZER_GRAPH_OPTIMIZER_OP_JUDGE_FORMAT_AND_DTYPE_STRATEGY_DTYPE_STRATEGY_OP_DTYPE_SELECTION_STRATEGY_FORCE_FP16_H_
