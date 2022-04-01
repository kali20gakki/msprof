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

#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_SPACESIZE_CALCULATOR_SPACESIZE_CALCULATOR_H_
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_SPACESIZE_CALCULATOR_SPACESIZE_CALCULATOR_H_

#include <map>
#include "graph/compute_graph.h"
#include "graph/op_desc.h"
#include "graph_optimizer/graph_optimize_register_error_codes.h"

namespace fe {

/** @brief provide the capability of calculating
* workspace and input/output size */
class SpaceSizeCalculator {
 public:
  SpaceSizeCalculator();

  ~SpaceSizeCalculator();

  /**
   * @ingroup fe
   * @brief prohibit copy and assign construct
   */
  SpaceSizeCalculator(const SpaceSizeCalculator &) = delete;

  SpaceSizeCalculator &operator=(const SpaceSizeCalculator &) = delete;

  /**
  * Calculate params of OP in the graph
  * @param graph ComputeGraph
  *      false : calculate the param of OP whose impl type is not tbe
  */
  Status CalculateRunningParams(const ge::ComputeGraph &graph) const;

  Status CalculateAICoreRunningParams(const ge::ComputeGraph &graph) const;
};
}  // namespace fe

#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_SPACESIZE_CALCULATOR_SPACESIZE_CALCULATOR_H_
