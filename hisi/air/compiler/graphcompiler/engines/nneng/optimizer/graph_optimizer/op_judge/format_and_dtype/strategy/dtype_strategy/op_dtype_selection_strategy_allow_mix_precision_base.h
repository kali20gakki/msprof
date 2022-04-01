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

#ifndef AIR_COMPILER_ENGINES_NNENG_OPTIMIZER_GRAPH_OPTIMIZER_OP_JUDGE_FORMAT_AND_DTYPE_STRATEGY_DTYPE_STRATEGY_OP_DTYPE_SELECTION_STRATEGY_ALLOW_MIX_PRECISION_BASE_H_
#define AIR_COMPILER_ENGINES_NNENG_OPTIMIZER_GRAPH_OPTIMIZER_OP_JUDGE_FORMAT_AND_DTYPE_STRATEGY_DTYPE_STRATEGY_OP_DTYPE_SELECTION_STRATEGY_ALLOW_MIX_PRECISION_BASE_H_

#include "graph_optimizer/op_judge/format_and_dtype/strategy/matcher/dtype/op_dtype_mix_precision_matcher.h"
#include "graph_optimizer/op_judge/format_and_dtype/strategy/matcher/dtype/op_dtype_reduce_matcher.h"
#include "op_dtype_selection_strategy_allow_fp32_to_bf16.h"
#include "op_dtype_selection_strategy_base.h"
#include "op_dtype_selection_strategy_default_mode.h"
#include "op_dtype_selection_strategy_force_fp16.h"
namespace fe {
using DefaultSelector = std::unique_ptr<OpDtypeSelectionStrategyDefaultMode>;
using AllowFp32ToBf16Selector = std::unique_ptr<OpDtypeSelectionStrategyAllowFp32ToBf16>;
using AllowFp32ToFp16Selector = std::unique_ptr<OpDtypeSelectionStrategyAllowFp32ToFp16>;

using OpDtypeMixPrecisionMatcherPtr = std::shared_ptr<OpDtypeMixPrecisionMatcher>;
using OpDtypeRiseMatcherPtr = std::shared_ptr<OpDtypeRiseMatcher>;
using OpDtypeReduceMatcherPtr = std::shared_ptr<OpDtypeReduceMatcher>;
class OpDtypeSelectionStrategyAllowMixPrecisionBase : public OpDtypeSeletionStrategyBase {
 public:
  OpDtypeSelectionStrategyAllowMixPrecisionBase(
      const std::string& engine_name,
      FormatDtypeQuerierPtr format_dtype_querier_ptr,
      OpDtypeMixPrecisionMatcherPtr op_dtype_mixed_precision_matcher_ptr,
      OpDtypeRiseMatcherPtr op_dtype_rise_matcher_ptr,
      OpDtypeReduceMatcherPtr op_dtype_reduce_matcher_ptr,
      const string mix_precision_type,
      const ge::DataType data_type);

  ~OpDtypeSelectionStrategyAllowMixPrecisionBase() override;

  Status RunForOpInWhiteList(FormatDtypeSelectionBasicInfo& basic_info, ForbiddenDtype forbidden_dtype);

  Status RunForOpInGrayList(FormatDtypeSelectionBasicInfo& basic_info, ForbiddenDtype forbidden_dtype);

  /* Black list op must use their original data types. */
  Status RunForOpInBlackList(FormatDtypeSelectionBasicInfo& basic_info, ForbiddenDtype forbidden_dtype) const;
 private:
  void MatchForGray(const string &cur_op_desc_type, const string &cur_op_desc_name,
       const vector<ge::DataType> &op_kernel_dtype_vec, ge::DataType father_output_dtype,
       const FormatDtypeSelectionBasicInfo& basic_info, ForbiddenDtype forbidden_dtype);
  bool IsOpFp16Bf16Fp32Cast(const ge::OpDescPtr& cur_op_desc_ptr, const uint32_t& fahter_out_anchor_index);
  Status QueryPrecisionPolicy(const ge::OpDescPtr &op_desc_ptr, PrecisionPolicy &precision_policy);

  std::string engine_name_;
  OpDtypeMixPrecisionMatcherPtr op_dtype_mixed_precision_matcher_ptr_;
  OpDtypeRiseMatcherPtr op_dtype_rise_matcher_ptr_;
  OpDtypeReduceMatcherPtr op_dtype_reduce_matcher_ptr_;
  const string mix_precision_type_;
  const ge::DataType data_type_;

};
}  // namespace fe
#endif  // AIR_COMPILER_ENGINES_NNENG_OPTIMIZER_GRAPH_OPTIMIZER_OP_JUDGE_FORMAT_AND_DTYPE_STRATEGY_DTYPE_STRATEGY_OP_DTYPE_SELECTION_STRATEGY_ALLOW_MIX_PRECISION_H_
