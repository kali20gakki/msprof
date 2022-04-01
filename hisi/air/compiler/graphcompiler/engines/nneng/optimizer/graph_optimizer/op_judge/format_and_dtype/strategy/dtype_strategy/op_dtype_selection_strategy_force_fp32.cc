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

#include "op_dtype_selection_strategy_force_fp32.h"

namespace fe {
OpDtypeSelectionStrategyForceFp32::OpDtypeSelectionStrategyForceFp32(
    FormatDtypeQuerierPtr format_dtype_querier_ptr, OpDtypeRiseMatcherPtr op_dtype_rise_matcher_ptr,
    OpDtypeReduceMatcherPtr op_dtype_reduce_matcher_ptr)
    : OpDtypeSeletionStrategyBase(format_dtype_querier_ptr),
      op_dtype_rise_matcher_ptr_(op_dtype_rise_matcher_ptr),
      op_dtype_reduce_matcher_ptr_(op_dtype_reduce_matcher_ptr) {}

OpDtypeSelectionStrategyForceFp32::~OpDtypeSelectionStrategyForceFp32() {}

/* In this mode we will match the dtype fp32 first. If the */
Status OpDtypeSelectionStrategyForceFp32::Run(FormatDtypeSelectionBasicInfo &basic_info,
                                              ForbiddenDtype forbidden_dtype) {
  FE_CHECK_NOTNULL(basic_info.node);
  auto cur_op_desc_ptr = basic_info.node->GetOpDesc();
  FE_CHECK_NOTNULL(cur_op_desc_ptr);
  std::string cur_op_desc_name = cur_op_desc_ptr->GetName();
  std::string cur_op_desc_type = cur_op_desc_ptr->GetType();
  FE_LOGD("Op %s: match dtype for tensor %u in force fp32 mode.", cur_op_desc_name.c_str(), basic_info.index);

  // 2. Match fp32 if original Dtype is fp16 or bf16
  ge::DataType origin_dtype = basic_info.cur_tensor_desc->GetDataType();
  if (origin_dtype == ge::DT_FLOAT16 || origin_dtype == ge::DT_BF16) {
    origin_dtype = ge::DT_FLOAT;
  }
  vector<ge::DataType> input_or_output_dtype_vec;
  if (format_dtype_querier_ptr_->GetSupportDataTypes(basic_info.op_kernel_info_ptr, basic_info.tensor_kernel_info_ptr,
                                                     *(cur_op_desc_ptr.get()), input_or_output_dtype_vec) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][DtypeJdg][ForceFp32] Fail to get the support data_types.");
    return FAILED;
  }

  FE_LOGD("Op %s: match the dtype in force_fp32 mode, the expected dtype is %u.", cur_op_desc_name.c_str(),
          origin_dtype);
  Status match_origin_dtype_res = op_dtype_rise_matcher_ptr_->Match(input_or_output_dtype_vec, origin_dtype,
                                                                    basic_info.matched_index_vec, forbidden_dtype);
  /* if force fp32 match failed, we will use the data type that it supports and
   * will not return FAILED. */
  if (match_origin_dtype_res != SUCCESS) {
    vector<uint32_t> bak_matched_index_vec = basic_info.matched_index_vec;
    vector<ge::DataType> classify_types;
    FE_LOGD("Precision loss is allowed, try to match low precision dtype.");
    match_origin_dtype_res = op_dtype_reduce_matcher_ptr_->Match(input_or_output_dtype_vec, origin_dtype,
                                                                 basic_info.matched_index_vec, forbidden_dtype);
    // force_fp32 reducing, we should use fp16
  }
  if (match_origin_dtype_res != SUCCESS) {
    FE_LOGD("Cannot find any matched data type for tensor %u %s of dtype %u.", basic_info.index,
            cur_op_desc_name.c_str(), origin_dtype);
  }
  return SUCCESS;
}
}  // namespace fe