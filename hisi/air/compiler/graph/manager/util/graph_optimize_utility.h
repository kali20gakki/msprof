/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2021. All rights reserved.
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

#ifndef GE_GRAPH_OPTIMIZE_GRAPH_OPTIMIZE_UTILITY_H_
#define GE_GRAPH_OPTIMIZE_GRAPH_OPTIMIZE_UTILITY_H_

#include "common/optimizer/optimize_utility.h"
#include "external/ge/ge_api_types.h"
#include "graph/manager/util/graph_rebuild_state_ctrl.h"

namespace ge {
class GraphOptimizeUtility : public OptimizeUtility {
 public:
  GraphOptimizeUtility();
  ~GraphOptimizeUtility() override;

  // Deprecated: will delete later.
  // some functions required session_id on compute_graph, which set when graph_prepare init.
  // so every func which invoke InferShape need keep position after graph_prepare init.
  Status InferShape(ComputeGraph &compute_graph) override;

  // some functions required session_id on compute_graph, which set when graph_prepare init.
  // so every func which invoke InferShape need keep position after graph_prepare init.
  Status InferShape(const ComputeGraphPtr &compute_graph) override;
};
}  // namespace ge
#endif  // GE_GRAPH_OPTIMIZE_GRAPH_OPTIMIZE_UTILITY_H_
