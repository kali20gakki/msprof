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

#include "graph_optimizer/op_judge/format_and_dtype/strategy/matcher/dtype/op_dtype_rise_matcher.h"
#include "common/fe_utils.h"
#include "common/util/op_info_util.h"
#include "common/math_util.h"

namespace fe {
OpDtypeRiseMatcher::OpDtypeRiseMatcher() : OpDtypeMatcherBase() {}
OpDtypeRiseMatcher::~OpDtypeRiseMatcher() {}

Status OpDtypeRiseMatcher::FindSuitableDtype(const vector<ge::DataType> &op_kernel_dtype_vec,
    const ge::DataType &expected_dtype, vector<uint32_t> &matched_index_vec,
    const ge::DataType &forbidden_dtype) {
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
  bool has_found_dtype_exactly = false;
  uint32_t op_kernel_dtype_vec_size = op_kernel_dtype_vec.size();
  bool dtype_float_flag =
      (expected_dtype == ge::DT_FLOAT || expected_dtype == ge::DT_FLOAT16 || expected_dtype == ge::DT_BF16);
  for (auto iter = matched_index_vec.begin(); iter != matched_index_vec.end(); ++iter) {
    uint32_t index_rise = *iter;
    if (index_rise >= op_kernel_dtype_vec_size) {
      continue;
    }

    auto op_kernel_dtype = op_kernel_dtype_vec[index_rise];
    auto op_kernel_dtype_iter = DATATYPE_PRIORITY_MAP_AMPLIFIED.find(op_kernel_dtype);
    if (op_kernel_dtype_iter == DATATYPE_PRIORITY_MAP_AMPLIFIED.end()) {
      FE_LOGD("The dtype %u in the op kernel is invalid, index [%u]", op_kernel_dtype, index_rise);
      continue;
    }

    if (dtype_float_flag && (op_kernel_dtype == forbidden_dtype || op_kernel_dtype == ge::DT_DOUBLE)) {
      FE_LOGD("The dtype %u in the op kernel is forbidden,can not use it", op_kernel_dtype);
      continue;
    }
    // 1. the priority gap between the exptected data type and the op_kernel data type
    int32_t prio_gap = expected_dtype_iter->second - op_kernel_dtype_iter->second;
    // 2. the exptected data type is equal to the op_kernel data type
    bool check_middle_gap = (!has_found_dtype_exactly &&
        ((prio_gap > LOW_GAP_AMPLIFIED && prio_gap < HIGH_GAP_AMPLIFIED) || expected_dtype == ge::DT_BOOL));
    if (prio_gap == LOW_GAP_AMPLIFIED) {
      if (!has_found_dtype_exactly) {
        // 2.1 first time we find that dtype in op kernel
        priority_index_map.clear();
        prio_gap_increment_value = 1;
        has_found_dtype_exactly = true;
      } else {
        // 2.2 has_found_dtype_exactly is true
        FE_INT32_ADDCHECK(prio_gap, prio_gap_increment_value);
        prio_gap += prio_gap_increment_value;
        FE_INT32_ADDCHECK(prio_gap_increment_value, 1);
        prio_gap_increment_value++;
      }
      priority_index_map.emplace(std::make_pair(prio_gap, index_rise));
    } else if (check_middle_gap) {
      // 3. the precision is rised in the same type (fp16->fp32, int32->int64)
      // or expected_dtype is BOOL
      if (priority_index_map.find(prio_gap) != priority_index_map.end()) {
        FE_INT32_ADDCHECK(prio_gap, prio_gap_increment_value);
        prio_gap += prio_gap_increment_value;
        FE_INT32_ADDCHECK(prio_gap_increment_value, 1);
        prio_gap_increment_value++;
      }
      priority_index_map.emplace(std::make_pair(prio_gap, index_rise));
    }
  }

  // 4. if the priority_index_map is not empty, the matched_index_vec is replaced by
  // the priority_index_map
  if (!priority_index_map.empty()) {
    vector<uint32_t> empty_vec_rise;
    matched_index_vec.swap(empty_vec_rise);
    // key is default in ascending order;
    for (const auto &item : priority_index_map) {
      matched_index_vec.emplace_back(item.second);
    }
    return SUCCESS;
  }
  // Here if priority_index_map is empty, that means dtype is not found for
  // input_x, we can randomly pick a dtype which op kernel store supports. So
  // just return Success and do nothing with matched_index_vec. For precisely
  // matching subsequent input, We go through input_x+1 and check whether there
  // would be any format and dtype match.
  return FAILED;
}

}  // namespace fe
