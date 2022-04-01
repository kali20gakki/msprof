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

#include "graph_optimizer/op_judge/format_and_dtype/strategy/matcher/format/op_format_matcher.h"
#include "common/fe_utils.h"

namespace fe {
OpFormatMatcher::OpFormatMatcher() {}
OpFormatMatcher::~OpFormatMatcher() {}

Status OpFormatMatcher::Match(const vector<ge::Format> &op_kernel_format_vec, const ge::Format &expected_format,
                              const ge::Format &cur_origin_format, vector<uint32_t> &matched_index_vec) {
  vector<uint32_t> matched_index_vec_temp = matched_index_vec;
  FindSuitableFormat(op_kernel_format_vec, expected_format, cur_origin_format, matched_index_vec_temp);
  if (!matched_index_vec_temp.empty()) {
    matched_index_vec = matched_index_vec_temp;
    return SUCCESS;
  } else {
    if (IsNd(cur_origin_format)) {
      uint32_t op_kernel_format_vec_size = op_kernel_format_vec.size();
      for (auto iter = matched_index_vec.begin(); iter < matched_index_vec.end();
           /* The iter will increase in loop body. */) {
        uint32_t index = *iter;
        if (index < op_kernel_format_vec_size) {
          ge::Format op_kernel_format = op_kernel_format_vec[index];
          if (op_kernel_format == ge::FORMAT_NC1HWC0) {
            iter = matched_index_vec.erase(iter);
          } else {
            iter++;
          }
        } else {
          FE_LOGD("the matched index %u is larger than or equal to the opKernelFormatVecSize %u.",
                  index, op_kernel_format_vec_size);
          iter = matched_index_vec.erase(iter);
        }
      }
    }
    return FAILED;
  }
}

Status OpFormatMatcher::FindSuitableFormat(const vector<ge::Format> &op_kernel_format_vec,
                                           const ge::Format &expected_format, const ge::Format &cur_origin_format,
                                           vector<uint32_t> &matched_index_vec) {
  uint32_t op_kernel_format_vec_size = op_kernel_format_vec.size();
  for (auto iter = matched_index_vec.begin(); iter < matched_index_vec.end();
       /* The iter will increase in loop body. */) {
    uint32_t index = *iter;
    if (index < op_kernel_format_vec_size) {
      // 1. the op_kernel_format is equal to the expected_format
      // 2. the op_kernel_format is in {ND,MD}, the origin format is equal to
      // expected_format
      ge::Format op_kernel_format = op_kernel_format_vec[index];
      if (op_kernel_format == expected_format || (IsNd(op_kernel_format) && cur_origin_format == expected_format)) {
        iter++;
        continue;
      } else {
        iter = matched_index_vec.erase(iter);
      }
    } else {
      FE_LOGD(
          "the matched index %u is larger than or equal to the "
          "opKernelFormatVecSize %u.",
          index, op_kernel_format_vec_size);
      iter = matched_index_vec.erase(iter);
    }
  }
  return SUCCESS;
}
}  // namespace fe
