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

#ifndef GE_GRAPH_PASSES_MERGE_TO_STREAM_MERGE_PASS_H_
#define GE_GRAPH_PASSES_MERGE_TO_STREAM_MERGE_PASS_H_

#include "inc/graph_pass.h"

namespace ge {
class MergeToStreamMergePass : public GraphPass {
 public:
  Status Run(ComputeGraphPtr graph);

 private:
  ///
  /// @brief Replace Merge Op
  /// @param [in] graph
  /// @param [in] merge_node
  /// @return Status
  ///
  Status ReplaceMergeNode(const ComputeGraphPtr &graph, const NodePtr &merge_node) const;

  ///
  /// @brief Add StreamActive Op as StreamMerge in_node
  /// @param [in] graph
  /// @param [in] node
  /// @return Status
  ///
  Status AddActiveNodes(const ComputeGraphPtr &graph, const NodePtr &node) const;

  ///
  /// @brief Create Active Op
  /// @param [in] graph
  /// @param [in] node
  /// @return ge::NodePtr
  ///
  NodePtr CreateActiveNode(const ComputeGraphPtr &graph, const NodePtr &node) const;

  std::set<NodePtr> bypass_nodes_;
};
}  // namespace ge
#endif  // GE_GRAPH_PASSES_MERGE_TO_STREAM_MERGE_PASS_H_
