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

#ifndef AIR_COMPILER_ENGINES_NNENG_OPTIMIZER_GRAPH_OPTIMIZER_OP_JUDGE_FORMAT_AND_DTYPE_STRATEGY_DTYPE_STRATEGY_OP_DTYPE_SELECTION_STRATEGY_ALLOW_MIX_PRECISION_FP16_H_
#define AIR_COMPILER_ENGINES_NNENG_OPTIMIZER_GRAPH_OPTIMIZER_OP_JUDGE_FORMAT_AND_DTYPE_STRATEGY_DTYPE_STRATEGY_OP_DTYPE_SELECTION_STRATEGY_ALLOW_MIX_PRECISION_FP16_H_

#include "op_dtype_selection_strategy_base.h"
#include "op_dtype_selection_strategy_allow_mix_precision_base.h"

namespace fe {
class OpDtypeSelectionStrategyAllowMixPrecisionFp16 : public OpDtypeSelectionStrategyAllowMixPrecisionBase {
 public:
  OpDtypeSelectionStrategyAllowMixPrecisionFp16(
      const std::string& engine_name,
      FormatDtypeQuerierPtr format_dtype_querier_ptr,
      OpDtypeMixPrecisionMatcherPtr op_dtype_mixed_precision_matcher_ptr,
      OpDtypeRiseMatcherPtr op_dtype_rise_matcher_ptr,
      OpDtypeReduceMatcherPtr op_dtype_reduce_matcher_ptr);

  ~OpDtypeSelectionStrategyAllowMixPrecisionFp16() override;

  /* In this mode we will match the dtype fp16 first. If the */
  Status Run(FormatDtypeSelectionBasicInfo& basic_info, ForbiddenDtype forbidden_dtype) override;

 private:
  Status GetPrecisionPolicy(const OpKernelInfoPtr& op_kernel_info_ptr,
                                                                           PrecisionPolicy& precision_policy);

  std::string engine_name_;
  OpDtypeMixPrecisionMatcherPtr op_dtype_mixed_precision_matcher_ptr_;
  OpDtypeRiseMatcherPtr op_dtype_rise_matcher_ptr_;
  OpDtypeReduceMatcherPtr op_dtype_reduce_matcher_ptr_;
};
}  // namespace fe
#endif  // AIR_COMPILER_ENGINES_NNENG_OPTIMIZER_GRAPH_OPTIMIZER_OP_JUDGE_FORMAT_AND_DTYPE_STRATEGY_DTYPE_STRATEGY_OP_DTYPE_SELECTION_STRATEGY_ALLOW_MIX_PRECISION_H_
