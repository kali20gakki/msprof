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

#include "op_dtype_selection_strategy_allow_mix_precision_bf16.h"
#include "ops_store/ops_kernel_manager.h"

namespace fe {
const string KMixPrecisionType = "Bf16";
const ge::DataType KDataType = ge::DT_BF16;
OpDtypeSelectionStrategyAllowMixPrecisionBf16::OpDtypeSelectionStrategyAllowMixPrecisionBf16(
    const std::string &engine_name, FormatDtypeQuerierPtr format_dtype_querier_ptr,
    OpDtypeMixPrecisionMatcherPtr op_dtype_mixed_precision_matcher_ptr, OpDtypeRiseMatcherPtr op_dtype_rise_matcher_ptr,
    OpDtypeReduceMatcherPtr op_dtype_reduce_matcher_ptr)
    : OpDtypeSelectionStrategyAllowMixPrecisionBase(engine_name, format_dtype_querier_ptr,
                                                    op_dtype_mixed_precision_matcher_ptr, op_dtype_rise_matcher_ptr,
                                                    op_dtype_reduce_matcher_ptr, KMixPrecisionType, KDataType) {}

OpDtypeSelectionStrategyAllowMixPrecisionBf16::~OpDtypeSelectionStrategyAllowMixPrecisionBf16() {}

Status OpDtypeSelectionStrategyAllowMixPrecisionBf16::GetPrecisionPolicy(const OpKernelInfoPtr &op_kernel_info_ptr,
                                                                         PrecisionPolicy &precision_policy) {
  FE_CHECK_NOTNULL(op_kernel_info_ptr);
  precision_policy = op_kernel_info_ptr->GetOpStoreInfoBf16().precision_policy;
  return SUCCESS;
}

/* In this mode we will match the dtype fp16 first. If the */
Status OpDtypeSelectionStrategyAllowMixPrecisionBf16::Run(FormatDtypeSelectionBasicInfo &basic_info,
                                                          ForbiddenDtype forbidden_dtype) {
  FE_CHECK_NOTNULL(basic_info.node);
  auto cur_op_desc_ptr = basic_info.node->GetOpDesc();
  FE_CHECK_NOTNULL(cur_op_desc_ptr);
  PrecisionPolicy precision_policy;

  std::string cur_op_desc_name = cur_op_desc_ptr->GetName();
  std::string cur_op_desc_type = cur_op_desc_ptr->GetType();
  if (GetPrecisionPolicy(basic_info.op_kernel_info_ptr, precision_policy) != SUCCESS) {
    FE_LOGD("Op[name=%s,type=%s]: Failed to get precision policy in mix_bf16.", cur_op_desc_name.c_str(),
            cur_op_desc_type.c_str());
    return FAILED;
  }
  FE_LOGD("Op[name=%s,type=%s]: [mix_bf16]precision policy is %u.", cur_op_desc_name.c_str(), cur_op_desc_type.c_str(),
          precision_policy);
  if (precision_policy == BLACK) {
    /* If the ops is in black list, we must use its original data type */
    return RunForOpInBlackList(basic_info, forbidden_dtype);
  } else if (precision_policy == WHITE) {
    return RunForOpInWhiteList(basic_info, forbidden_dtype);
  } else {
    return RunForOpInGrayList(basic_info, forbidden_dtype);
  }
}
}  // namespace fe