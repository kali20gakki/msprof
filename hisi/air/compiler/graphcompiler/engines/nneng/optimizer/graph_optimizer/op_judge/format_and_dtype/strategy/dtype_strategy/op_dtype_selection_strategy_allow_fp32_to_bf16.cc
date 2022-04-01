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

#include "op_dtype_selection_strategy_allow_fp32_to_bf16.h"

namespace fe {
OpDtypeSelectionStrategyAllowFp32ToBf16::OpDtypeSelectionStrategyAllowFp32ToBf16(
    FormatDtypeQuerierPtr format_dtype_querier_ptr, OpDtypeRiseMatcherPtr op_dtype_rise_matcher_ptr,
    OpDtypeReduceMatcherPtr op_dtype_reduce_matcher_ptr)
    : OpDtypeSeletionStrategyBase(format_dtype_querier_ptr),
      op_dtype_rise_matcher_ptr_(op_dtype_rise_matcher_ptr),
      op_dtype_reduce_matcher_ptr_(op_dtype_reduce_matcher_ptr) {}

OpDtypeSelectionStrategyAllowFp32ToBf16::~OpDtypeSelectionStrategyAllowFp32ToBf16() {}

Status OpDtypeSelectionStrategyAllowFp32ToBf16::Run(SelectionBasicInfo &basic_info, ForbiddenDtype forbidden_dtype) {
  AllowFp32ToFp16Selector allow_fp32_to_bf16_mode(new(std::nothrow)OpDtypeSelectionStrategyAllowFp32ToFp16(
    format_dtype_querier_ptr_, op_dtype_rise_matcher_ptr_, op_dtype_reduce_matcher_ptr_));
  if (allow_fp32_to_bf16_mode == nullptr) {
      return FAILED;
    }
    return allow_fp32_to_bf16_mode->Run(basic_info, forbidden_dtype);
  }
}  // namespace fe
