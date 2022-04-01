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

#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_HEAVY_FORMAT_PROPAGATION_HEAVY_FORMAT_SELECTOR_H_
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_HEAVY_FORMAT_PROPAGATION_HEAVY_FORMAT_SELECTOR_H_
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include "common/fe_inner_error_codes.h"
#include "common/fe_log.h"
#include "common/fe_utils.h"
#include "common/math_util.h"
#include "common/op_info_common.h"
#include "common/util/op_info_util.h"
#include "format_selector/manager/format_dtype_querier.h"
#include "graph/compute_graph.h"
#include "graph_optimizer/op_judge/format_and_dtype/strategy/matcher/dtype/op_dtype_precise_matcher.h"

namespace fe {
using FormatDtypeQuerierPtr = std::shared_ptr<FormatDtypeQuerier>;

enum class FormatScore {
  DISTRIBUTED_HEAVY_FORMAT_SCORE = 2000,
  OTHER_HEAVY_FORMAT_SCORE = 200,
  ORIGINAL_FORMAT_SCORE = 100,
  OTHER_FORMAT_SCORE = 1
};

constexpr int32_t INVALID_KERNEL_INDEX = -1;

/** @brief First select qualified format index by the current data type of each
* input and out put. */
class HeavyFormatSelector {
 public:
  using PreciseDtypeMatcherPtr = std::shared_ptr<OpDtypePreciseMatcher>;
  explicit HeavyFormatSelector(FormatDtypeQuerierPtr format_dtype_querier_ptr);

  ~HeavyFormatSelector();

  Status Initalize();
  /* Sort the format by loop around all inputs and outputs in ops kernel.
   * Sort format in ops-kernel by the following priority:
   * The highest priority: as same as the input heavy format
   * The second priority: other heavy format
   * The third priority: original format
   * Other: other format
   * And we consider that the input anchor which peer out anchor's owner node
   * is const or variable has more weight, because const or variable can be merged
   * with trans-nodes.
   * The format score is define as below:
   * Same as the heavy format which is distributed right now : 2000;
   * Other heavy format : 200
   * Same as the tensor's original format(ND included) : 100
   * Other format: 1 */
  Status GetMostSuitableFormatIndex(const fe::OpKernelInfoPtr& op_kernel_info_ptr, const ge::NodePtr& current_node,
                                    const HeavyFormatInfo& heavy_format_info,
                                    const std::vector<IndexNameMap>& tensor_map, int32_t& most_suitable_index);

 private:
  /* For specifc input or output (which the heavy format is distributed from),
   * we need to ensure its format is exactly the same as the heavy format.
   * So we first select the format index which qualify this requirement. */
  Status SelectQualifiedFormat(const OpKernelInfoPtr& op_kernel_info_ptr, const ge::NodePtr& current_node,
                               const HeavyFormatInfo& heavy_format_info, const std::vector<IndexNameMap>& tensor_map);

  /* Get the initial heavy format index by the heavy format and the
   * corresponding input's kernel info. If and only if the format of
   * the input's or output's kernel is exactly as same as the heavy format and
   * the data type of the input's or output's kernel is exactly as same as the
   * current tensor's data type, we will push it to the matched_index_
   * */
  Status SearchHeavyFormatInKernel(const OpKernelInfoPtr& op_kernel_info_ptr, const ge::OpDescPtr& op_desc_ptr,
                                   const HeavyFormatInfo& heavy_format_info);

  /* When doing distribution, we should ensure the data type which selected by
   * opjudge will not change because the precision is much more important
   * than the format. So the data types of the heavy format in the ops kernel
   * should be exactly the same as current ones */
  Status MatchDtypeForAllInputAndOutput(const OpKernelInfoPtr& op_kernel_info_ptr, const ge::NodePtr& current_node);

  Status CalcFormatScore(const ge::OpDesc::Vistor<ge::GeTensorDescPtr>& all_tensors,
                         const fe::OpKernelInfoPtr& op_kernel_info_ptr, const ge::OpDescPtr& op_desc_ptr,
                         uint32_t kernel_format_index, const HeavyFormatInfo& heavy_format_info,
                         InputOrOutputIndex in_or_out, uint64_t& score);

  Status Match(const OpKernelInfoPtr& op_kernel_info_ptr,
               const ge::OpDescPtr& op_desc_ptr, const ge::OpDesc::Vistor<ge::GeTensorDescPtr>& all_tensors,
               InputOrOutputIndex in_or_out);

  FormatDtypeQuerierPtr format_dtype_querier_ptr_;

  PreciseDtypeMatcherPtr precise_dtype_matcher_ptr_;

  std::vector<std::vector<InputOrOutputInfoPtr>> input_and_output_kernel_;

  std::vector<uint32_t> matched_index_;
};

Status GetInputAndOutputIndexNameMap(const OpKernelInfoPtr& op_kernel_info_ptr, const ge::NodePtr& current_node,
                                     std::vector<IndexNameMap>& tensor_map);
}  // namespace fe

#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_HEAVY_FORMAT_PROPAGATION_HEAVY_FORMAT_SELECTOR_H_
