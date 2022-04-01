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

#ifndef GRAPH_PARTITION_OPTIMIZER_DYNAMIC_DATA_FLOW_ENGINE_REASSIGN_PASS_H_
#define GRAPH_PARTITION_OPTIMIZER_DYNAMIC_DATA_FLOW_ENGINE_REASSIGN_PASS_H_

#include "graph/partition/engine_place.h"

namespace ge {
class DynamicDataFlowReAssign : public EngineReAssignPass {
 public:
  DynamicDataFlowReAssign() = default;
  ~DynamicDataFlowReAssign() override = default;
  Status Run(const ComputeGraphPtr &graph,
             NodeEngineMap &node_atomic_engine_map,
             NodeEngineMap &node_composite_engine_map) override;
};
}  // namespace ge
#endif  // GRAPH_PARTITION_OPTIMIZER_DYNAMIC_DATA_FLOW_ENGINE_REASSIGN_PASS_H_
