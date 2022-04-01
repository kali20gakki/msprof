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

#ifndef AIR_COMPILER_ENGINES_NNENG_OPTIMIZER_GRAPH_OPTIMIZER_OP_JUDGE_FORMAT_AND_DTYPE_STRATEGY_DTYPE_STRATEGY_OP_DTYPE_SELECTION_STRATEGY_ALLOW_FP32_TO_FP16_H_
#define AIR_COMPILER_ENGINES_NNENG_OPTIMIZER_GRAPH_OPTIMIZER_OP_JUDGE_FORMAT_AND_DTYPE_STRATEGY_DTYPE_STRATEGY_OP_DTYPE_SELECTION_STRATEGY_ALLOW_FP32_TO_FP16_H_

#include "graph_optimizer/op_judge/format_and_dtype/strategy/matcher/dtype/op_dtype_reduce_matcher.h"
#include "graph_optimizer/op_judge/format_and_dtype/strategy/matcher/dtype/op_dtype_rise_matcher.h"
#include "op_dtype_selection_strategy_base.h"

namespace fe {
using OpDtypeRiseMatcherPtr = std::shared_ptr<OpDtypeRiseMatcher>;
using OpDtypeReduceMatcherPtr = std::shared_ptr<OpDtypeReduceMatcher>;
class OpDtypeSelectionStrategyAllowFp32ToFp16 : public OpDtypeSeletionStrategyBase {
 public:
  explicit OpDtypeSelectionStrategyAllowFp32ToFp16(FormatDtypeQuerierPtr format_dtype_querier_ptr,
      OpDtypeRiseMatcherPtr op_dtype_rise_matcher_ptr, OpDtypeReduceMatcherPtr op_dtype_reduce_matcher_ptr);

  ~OpDtypeSelectionStrategyAllowFp32ToFp16() override;

  void ProcessReduceMatch(const vector<ge::DataType> &op_kernel_dtype_vec,
                          const ge::DataType &origin_dtype,
                          const SelectionBasicInfo &basic_info,
                          ForbiddenDtype forbidden_dtype,
                          Status &match_origin_dtype_res);
  /* In this mode we will match the dtype fp16 first. If the */
  Status Run(SelectionBasicInfo& basic_info, ForbiddenDtype forbidden_dtype) override;

 private:
  OpDtypeRiseMatcherPtr op_dtype_rise_matcher_ptr_;
  OpDtypeReduceMatcherPtr op_dtype_reduce_matcher_ptr_;
};
}  // namespace fe
#endif  // AIR_COMPILER_ENGINES_NNENG_OPTIMIZER_GRAPH_OPTIMIZER_OP_JUDGE_FORMAT_AND_DTYPE_STRATEGY_DTYPE_STRATEGY_OP_DTYPE_SELECTION_STRATEGY_ALLOW_FP32_TO_FP16_H_
