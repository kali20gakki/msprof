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

#include "op_dtype_selection_strategy_default_mode.h"

namespace fe {
OpDtypeSelectionStrategyDefaultMode::OpDtypeSelectionStrategyDefaultMode(
    FormatDtypeQuerierPtr format_dtype_querier_ptr, OpDtypeRiseMatcherPtr op_dtype_rise_matcher_ptr)
    : OpDtypeSeletionStrategyBase(format_dtype_querier_ptr),
      op_dtype_rise_matcher_ptr_(op_dtype_rise_matcher_ptr) {}

OpDtypeSelectionStrategyDefaultMode::~OpDtypeSelectionStrategyDefaultMode() {}

Status OpDtypeSelectionStrategyDefaultMode::Run(FormatDtypeSelectionBasicInfo& basic_info,
                                                ForbiddenDtype forbidden_dtype) {
  FE_CHECK_NOTNULL(basic_info.node);
  auto cur_op_desc_ptr = basic_info.node->GetOpDesc();
  FE_CHECK_NOTNULL(cur_op_desc_ptr);
  std::string cur_op_desc_name = cur_op_desc_ptr->GetName();
  std::string cur_op_desc_type = cur_op_desc_ptr->GetType();
  FE_LOGD("Op %s: match dtype for tensor %u in default mode.", cur_op_desc_name.c_str(), basic_info.index);

  // 2. Match Original Dtype
  ge::DataType origin_dtype = basic_info.cur_tensor_desc->GetDataType();
  vector<ge::DataType> input_or_output_dtype_vec;
  if (format_dtype_querier_ptr_->GetSupportDataTypes(basic_info.op_kernel_info_ptr, basic_info.tensor_kernel_info_ptr,
                                                     *(cur_op_desc_ptr.get()), input_or_output_dtype_vec) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][DtypeJdg][DefaultMode][Op %s type %s] Fail to get the support data_types.",
                    cur_op_desc_name.c_str(), cur_op_desc_type.c_str());
    return FAILED;
  }

  FE_LOGD("Op %s: match the origin dtype, the expected dtype is %u.", cur_op_desc_name.c_str(), origin_dtype);
  Status match_origin_dtype_res =
      op_dtype_rise_matcher_ptr_->Match(input_or_output_dtype_vec, origin_dtype,
                                        basic_info.matched_index_vec, forbidden_dtype);
  FE_LOGI("After matching, size of vec is %zu", basic_info.matched_index_vec.size());
  if (match_origin_dtype_res != SUCCESS) {
    string ori_dtype = ge::TypeUtils::DataTypeToSerialString(origin_dtype);
    std::vector<std::string> key = {EM_OP_NAME, EM_OP_TYPE, EM_ORIGIN_DTYPE};
    std::vector<std::string> value = {cur_op_desc_ptr->GetName(), cur_op_desc_ptr->GetType(),
                                      ori_dtype};
    REPORT_INPUT_ERROR(EM_ORIGINAL_DATATYPE_IS_NOT_SUPPORTED, key, value);
    FE_LOGE("[GraphOpt][DtypeJdg][DefaultMode][Op %s type %s] Precision loss is not allowed! %s is not supported.",
            cur_op_desc_name.c_str(), cur_op_desc_type.c_str(), ori_dtype.c_str());

    return FAILED;
  }
  return SUCCESS;
}
}
