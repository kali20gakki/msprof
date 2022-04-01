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

#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_OP_JUDGE_FORMAT_AND_DTYPE_STRATEGY_MATCHER_DTYPE_OP_DTYPE_MATCHER_BASE_H_
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_OP_JUDGE_FORMAT_AND_DTYPE_STRATEGY_MATCHER_DTYPE_OP_DTYPE_MATCHER_BASE_H_

#include "common/fe_inner_error_codes.h"
#include "graph/compute_graph.h"
#include "common/util/op_info_util.h"

namespace fe {
class OpDtypeMatcherBase {
 public:
  OpDtypeMatcherBase();
  virtual ~OpDtypeMatcherBase();

  /**
   * match the expected data type and the op_kernel data type
   * @param op_kernel_dtype_vec op kernel data type
   * @param expected_dtype expected data type
   * @param matched_index_vec matched index vector of the op kernel store
   * @param forbidden_dtype forbidden dtype can not store in matched_index_vec
   * @return if the matched_index_vec is not empty and FindSuitableDtype return
   * SUCCESS, return SUCCESS
   */
  Status Match(const vector<ge::DataType> &op_kernel_dtype_vec, const ge::DataType &expected_dtype,
               vector<uint32_t> &matched_index_vec, ForbiddenDtype forbidden_dtype);

  /**
   * match the expected data type and the op_kernel data type
   * @param op_kernel_dtype_vec op kernel data type
   * @param expected_dtype expected data type
   * @param matched_index_vec matched index vector of the op kernel store
   * @param forbidden_dtype forbidden dtype can not store in matched_index_vec
   * @return return SUCCESS
   */
  virtual Status FindSuitableDtype(const vector<ge::DataType> &op_kernel_dtype_vec, const ge::DataType &expected_dtype,
                                   vector<uint32_t> &matched_index_vec, const ge::DataType &dtype_forbid) = 0;

  // get the for forbidden dtype, used in matchers for delete dtype;
  ge::DataType GetForbiddenDtype(ForbiddenDtype forbidden_dtype) const;
};

}  // namespace fe

#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_OP_JUDGE_FORMAT_AND_DTYPE_STRATEGY_MATCHER_DTYPE_OP_DTYPE_MATCHER_BASE_H_