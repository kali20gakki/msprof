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

#ifndef GE_HYBRID_EXECUTOR_EXECUTOR_FFTS_PLUS_SUBGRAPH_EXECUTOR_H_
#define GE_HYBRID_EXECUTOR_EXECUTOR_FFTS_PLUS_SUBGRAPH_EXECUTOR_H_

#include "hybrid/executor/subgraph_executor.h"

namespace ge {
namespace hybrid {
class FftsPlusSubgraphExecutor : public SubgraphExecutor {
 public:
  FftsPlusSubgraphExecutor(const GraphItem *const graph_item, GraphExecutionContext *const context)
      : SubgraphExecutor(graph_item, context) {}
  virtual ~FftsPlusSubgraphExecutor() = default;

 protected:
  Status InitCallback(NodeState *const node_state, std::function<void()> &callback,
                      const std::shared_ptr<ScopeGuard> tensor_guard = nullptr) override;
  Status PrepareNode(const NodeItem &node_item) override;
};
} // namespace hybrid
} // namespace ge
#endif // GE_HYBRID_EXECUTOR_EXECUTOR_FFTS_PLUS_SUBGRAPH_EXECUTOR_H_
