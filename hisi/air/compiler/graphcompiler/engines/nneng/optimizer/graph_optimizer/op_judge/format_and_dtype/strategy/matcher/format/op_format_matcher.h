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

#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_OP_JUDGE_FORMAT_AND_DTYPE_STRATEGY_MATCHER_FORMAT_OP_FORMAT_MATCHER_H_
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_OP_JUDGE_FORMAT_AND_DTYPE_STRATEGY_MATCHER_FORMAT_OP_FORMAT_MATCHER_H_

#include "common/fe_inner_error_codes.h"
#include "common/op_info_common.h"
#include "graph/compute_graph.h"

namespace fe {
class OpFormatMatcher {
 public:
  OpFormatMatcher();
  virtual ~OpFormatMatcher();
  Status Match(const vector<ge::Format> &op_kernel_format_vec, const ge::Format &expected_format,
               const ge::Format &cur_origin_format, vector<uint32_t> &matched_index_vec);

 private:
  Status FindSuitableFormat(const vector<ge::Format> &op_kernel_format_vec, const ge::Format &expected_format,
                            const ge::Format &cur_origin_format, vector<uint32_t> &matched_index_vec);
};
}  // namespace fe

#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_OP_JUDGE_FORMAT_AND_DTYPE_STRATEGY_MATCHER_FORMAT_OP_FORMAT_MATCHER_H_
