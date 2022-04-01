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

#ifndef GE_GRAPH_PASSES_PASS_UTILS_H_
#define GE_GRAPH_PASSES_PASS_UTILS_H_

#include <vector>
#include "framework/common/debug/ge_log.h"
#include "framework/common/ge_inner_error_codes.h"
#include "graph/compute_graph.h"

namespace ge {
class PassUtils {
 public:
  PassUtils() = delete;
  ~PassUtils() = delete;

  static NodePtr GetInDataNode(const ConstNodePtr &node, int32_t index);

  static NodePtr GetInNodeCrossSubgraphByIndex(const ConstNodePtr &node, int32_t index);

  static bool IsConstant(const ConstNodePtr &node);

  static Status SetOutNodeWeight(const OutDataAnchorPtr &out_data_anchor, const NodePtr &src_node);

  static Status RemoveBranch(const NodePtr &node, std::vector<NodePtr> &delete_nodes, std::vector<NodePtr> &end_nodes);

  static Status RemoveInactiveBranchToMerge(const OutDataAnchorPtr &inactive_output_anchor,
      std::vector<NodePtr> &delete_nodes, std::vector<NodePtr> &end_nodes);

  ///
  /// check is need iter flow ctrl.
  /// @param compute_graph graph
  /// @return true:need iter flow ctrl.
  ///         false:no need
  ///
  static bool IsNeedTrainIteFlowCtrl(const ComputeGraphPtr &compute_graph);

  ///
  /// find in data anchor index with a valid peer out node existed
  /// @param node_ptr
  /// @return index
  ///
  static int32_t GetUniqueInDataAnchorIndex(const NodePtr &node_ptr);
  ///
  /// unlink node's in data anchors[index]'s father node with node itself
  /// then link father node's all in control nodes to node
  /// if any and not connected yet
  /// @param node
  /// @param index: in data anchor index
  /// @return
  ///
  static Status UnlinkNodeWithControlCopy(NodePtr &node, int32_t index);
};
}  // namespace ge

#endif  // GE_GRAPH_PASSES_PASS_UTILS_H_
