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

#include "graph_optimizer/op_judge/format_and_dtype/strategy/matcher/dtype/op_dtype_precise_matcher.h"
#include "common/fe_inner_attr_define.h"
#include "common/fe_utils.h"
#include "common/util/op_info_util.h"

namespace fe {
OpDtypePreciseMatcher::OpDtypePreciseMatcher() : OpDtypeMatcherBase() {}
OpDtypePreciseMatcher::~OpDtypePreciseMatcher() {}

Status OpDtypePreciseMatcher::FindSuitableDtype(const vector<ge::DataType> &op_kernel_dtype_vec,
                                                const ge::DataType &expected_dtype, vector<uint32_t> &matched_index_vec,
                                                const ge::DataType &dtype_forbid) {
  auto op_kernel_dtype_vec_size = op_kernel_dtype_vec.size();
  vector<uint32_t> priority_index_vec;
  for (auto iter = matched_index_vec.begin(); iter != matched_index_vec.end();
       /* iter will not increase automatically */) {
    uint32_t index = *iter;
    if (index >= op_kernel_dtype_vec_size) {
      iter = matched_index_vec.erase(iter);
      /* Delete the illegal iter and continue. */
      continue;
    }

    auto op_kernel_dtype = op_kernel_dtype_vec[index];
    if (op_kernel_dtype == expected_dtype) {
      priority_index_vec.push_back(index);
      iter++;
      continue;
    }
    iter++;
  }

  if (!priority_index_vec.empty()) {
    // key is default in ascending order;
    matched_index_vec.swap(priority_index_vec);
    return SUCCESS;
  } else {
    return FAILED;
  }
}

}  // namespace fe