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

#include "graph_optimizer/op_judge/format_and_dtype/strategy/matcher/dtype/op_dtype_matcher_base.h"

namespace fe {
OpDtypeMatcherBase::OpDtypeMatcherBase() {}
OpDtypeMatcherBase::~OpDtypeMatcherBase() {}

Status OpDtypeMatcherBase::Match(const vector<ge::DataType> &op_kernel_dtype_vec, const ge::DataType &expected_dtype,
                                 vector<uint32_t> &matched_index_vec, ForbiddenDtype forbidden_dtype) {
  ge::DataType dtype_forbid = GetForbiddenDtype(forbidden_dtype);
  Status ret = FindSuitableDtype(op_kernel_dtype_vec, expected_dtype, matched_index_vec, dtype_forbid);
  if (!matched_index_vec.empty() && ret != FAILED) {
    return SUCCESS;
  } else {
    return FAILED;
  }
}

Status OpDtypeMatcherBase::FindSuitableDtype(const vector<ge::DataType> &op_kernel_dtype_vec,
    const ge::DataType &expected_dtype, vector<uint32_t> &matched_index_vec, const ge::DataType &dtype_forbid) {
  return SUCCESS;
}

ge::DataType OpDtypeMatcherBase::GetForbiddenDtype(ForbiddenDtype forbidden_dtype) const {
  ge::DataType forbid;
  if (forbidden_dtype == ForbiddenDtype::FORBIDDEN_BF16) {
    forbid = ge::DT_BF16;
  } else if (forbidden_dtype == ForbiddenDtype::FORBIDDEN_FP16) {
    forbid = ge::DT_FLOAT16;
  } else if (forbidden_dtype == ForbiddenDtype::FORBIDDEN_DOUBLE) {
    forbid = ge::DT_DOUBLE;
  } else {
    forbid = ge::DT_MAX;
  }
  return forbid;
}
}  // namespace fe