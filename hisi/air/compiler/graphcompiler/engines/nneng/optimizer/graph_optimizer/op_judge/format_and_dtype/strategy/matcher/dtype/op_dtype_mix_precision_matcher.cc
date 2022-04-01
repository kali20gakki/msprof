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

#include "graph_optimizer/op_judge/format_and_dtype/strategy/matcher/dtype/op_dtype_mix_precision_matcher.h"
#include "common/fe_log.h"
#include "common/util/op_info_util.h"
#include "common/math_util.h"

namespace fe {
OpDtypeMixPrecisionMatcher::OpDtypeMixPrecisionMatcher() : OpDtypeMatcherBase() {}
OpDtypeMixPrecisionMatcher::~OpDtypeMixPrecisionMatcher() {}

Status OpDtypeMixPrecisionMatcher::FindSuitableDtype(const vector<ge::DataType> &op_kernel_dtype_vec,
    const ge::DataType &expected_dtype, vector<uint32_t> &matched_index_vec, const ge::DataType &forbidden_dtype) {
  auto expected_dtype_iter = DATATYPE_PRIORITY_MAP_AMPLIFIED.find(expected_dtype);
  if (expected_dtype_iter == DATATYPE_PRIORITY_MAP_AMPLIFIED.end()) {
    FE_LOGW("the dtype %u is not found in DATATYPE_PRIORITY_MAP.", expected_dtype);
    return FAILED;
  }

  map<int32_t, uint32_t> priority_index_map;
  int32_t prio_gap_increment_value = 1;
  // The flag below shows whether we have found the expected_dtype exactly
  // instead its higher precision version. For example, fp32 is higher precision
  // version of fp16.
  uint32_t op_kernel_dtype_vec_size = op_kernel_dtype_vec.size();
  for (auto iter = matched_index_vec.begin(); iter != matched_index_vec.end(); ++iter) {
    uint32_t index_mix = *iter;
    if (index_mix >= op_kernel_dtype_vec_size) {
      continue;
    }
    auto op_kernel_dtype = op_kernel_dtype_vec[index_mix];
    auto op_kernel_dtype_iter = DATATYPE_PRIORITY_MAP_AMPLIFIED.find(op_kernel_dtype);
    if (op_kernel_dtype_iter == DATATYPE_PRIORITY_MAP_AMPLIFIED.end()) {
      FE_LOGD("The dtype %u in the op kernel is invalid, index [%u]", op_kernel_dtype, index_mix);
      continue;
    }

    if (op_kernel_dtype == forbidden_dtype) {
      FE_LOGD("The dtype %u in the op kernel is forbidden,can not use it", op_kernel_dtype);
      continue;
    }

    // 1. the priority gap between the exptected data type and the op_kernel data
    int32_t prio_gap_mix_precision = expected_dtype_iter->second - op_kernel_dtype_iter->second;
    // 2. the exptected data type is equal to the op_kernel data type
    if (op_kernel_dtype == ge::DT_FLOAT16 || op_kernel_dtype == ge::DT_BF16) {
      if (priority_index_map.find(prio_gap_mix_precision) != priority_index_map.end()) {
        FE_INT32_ADDCHECK(prio_gap_mix_precision, prio_gap_increment_value);
        prio_gap_mix_precision += prio_gap_increment_value;
        FE_INT32_ADDCHECK(prio_gap_increment_value, 1);
        prio_gap_increment_value++;
      }
      priority_index_map.emplace(std::make_pair(prio_gap_mix_precision, index_mix));
    }
  }

  // 4. if the priority_index_map is not empty, the matched_index_vec is replaced by
  // the priority_index_map
  if (!priority_index_map.empty()) {
    vector<uint32_t> empty_vec;
    matched_index_vec.swap(empty_vec);
    // key is default in ascending order;
    for (const auto &item : priority_index_map) {
      matched_index_vec.emplace_back(item.second);
    }
  } else {
    return FAILED;
  }
  return SUCCESS;
}
}  // namespace fe
