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

#include "op_format_selection_strategy_default_mode.h"

namespace fe {
OpFormatSelectionStrategyDefaultMode::OpFormatSelectionStrategyDefaultMode(
    FormatDtypeQuerierPtr format_dtype_querier_ptr)
    : format_dtype_querier_ptr_(format_dtype_querier_ptr) {}

OpFormatSelectionStrategyDefaultMode::~OpFormatSelectionStrategyDefaultMode() {}

Status OpFormatSelectionStrategyDefaultMode::Initialize() {
  FE_MAKE_SHARED(format_matcher_ = std::make_shared<OpFormatMatcher>(), return FAILED);
  return SUCCESS;
}

Status OpFormatSelectionStrategyDefaultMode::Run(FormatDtypeSelectionBasicInfo &basic_info) {
  ge::OpDescPtr cur_op_desc_ptr = basic_info.node->GetOpDesc();
  std::string cur_op_desc_name = cur_op_desc_ptr->GetName();
  std::string cur_op_desc_type = cur_op_desc_ptr->GetType();
  ge::Format cur_origin_format = GetCurOpOriginFormat(*(basic_info.cur_tensor_desc));
  FE_LOGD("Op[name=%s,type=%s]: match the origin format and dtype for tensor %u.", cur_op_desc_name.c_str(),
          cur_op_desc_type.c_str(), basic_info.index);
  // 1. Get all supported format
  vector<ge::Format> input_or_output_format;
  if (format_dtype_querier_ptr_->GetSupportFormats(basic_info.op_kernel_info_ptr, basic_info.tensor_kernel_info_ptr,
                                                   *(cur_op_desc_ptr.get()), input_or_output_format) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOptJdgInst][FmtJdg][Run] Fail to get the support formats, return FAILED.");
    return FAILED;
  }
  FE_LOGD("Op[name=%s,type=%s]: match origin format, the expected format is %s.", cur_op_desc_name.c_str(),
          cur_op_desc_type.c_str(), ge::TypeUtils::FormatToSerialString(cur_origin_format).c_str());
  Status match_origin_format_res = format_matcher_->Match(input_or_output_format, cur_origin_format, cur_origin_format,
                                                          basic_info.matched_index_vec);
  if (match_origin_format_res == SUCCESS) {
    FE_LOGD("Op[name=%s,type=%s]: match the origin format success, some matched formats in op kernel have been found.",
            cur_op_desc_name.c_str(), cur_op_desc_type.c_str());
  } else {
    FE_LOGD("Op[name=%s,type=%s]: Failed to match origin format.", cur_op_desc_name.c_str(), cur_op_desc_type.c_str());
  }
  return SUCCESS;
}
}  // namespace fe