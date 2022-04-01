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

#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_OP_JUDGE_FORMAT_AND_DTYPE_STRATEGY_FORMAT_STRATEGY_OP_FORMAT_SELECTION_STRATEGY_FOLLOW_PREDECESSOR_H_
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_OP_JUDGE_FORMAT_AND_DTYPE_STRATEGY_FORMAT_STRATEGY_OP_FORMAT_SELECTION_STRATEGY_FOLLOW_PREDECESSOR_H_

#include "common/fe_inner_error_codes.h"
#include "common/op_info_common.h"
#include "format_selector/manager/format_dtype_querier.h"
#include "graph/compute_graph.h"
#include "graph_optimizer/op_judge/format_and_dtype/strategy/dtype_strategy/op_dtype_selection_strategy_base.h"
#include "graph_optimizer/op_judge/format_and_dtype/strategy/matcher/format/op_format_matcher.h"

namespace fe {
using FormatDtypeQuerierPtr = std::shared_ptr<FormatDtypeQuerier>;
using OpFormatMatcherPtr = std::shared_ptr<OpFormatMatcher>;

/** @brief This class is created for selecting format by the following
* strategy:
* For tensor x of node A, pick the same format as it's predecessor.
* @version 1.0 */
class OpFormatSelectionStrategyFollowPredecessor {
 public:
  explicit OpFormatSelectionStrategyFollowPredecessor(FormatDtypeQuerierPtr format_dtype_querier_ptr);

  virtual ~OpFormatSelectionStrategyFollowPredecessor();

  Status Initialize();

  /**
   * run the current strategy
   * @param basic_info: Basic information for format selection.
   * @return SUCCESS or FAIL
   */
  Status Run(FormatDtypeSelectionBasicInfo &basic_info);

 protected:
  // next strategy
  OpFormatMatcherPtr format_matcher_;
  FormatDtypeQuerierPtr format_dtype_querier_ptr_;

 private:
};
}  // namespace fe
#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_OP_JUDGE_FORMAT_AND_DTYPE_STRATEGY_FORMAT_STRATEGY_OP_FORMAT_SELECTION_STRATEGY_FOLLOW_PREDECESSOR_H_
