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

#ifndef GE_GRAPH_PASSES_CREATE_SUBGRAPH_WITH_SCOPE_PASS_H_
#define GE_GRAPH_PASSES_CREATE_SUBGRAPH_WITH_SCOPE_PASS_H_

#include <map>
#include <string>
#include <vector>

#include "inc/graph_pass.h"

namespace ge {
class CreateSubGraphWithScopePass : public GraphPass {
 public:
  Status Run(ComputeGraphPtr graph);

 private:
  Status CollectScopeNodesByIndex(const ComputeGraphPtr &graph);
  Status ParseMultiDimsAttr(const std::vector<NodePtr> &scope_nodes);
  Status CollectIoNodes(const ComputeGraphPtr &subgraph);
  Status GetSubgraphFromScope(const ComputeGraphPtr &root_graph,
                              const std::pair<const int32_t,
                              std::vector<NodePtr>> &scope,
                              ComputeGraphPtr &subgraph) const;
  Status MergeNodesToSubgraph(const std::vector<NodePtr> &scope_nodes, const ComputeGraphPtr &subgraph);
  Status CheckCtrlAnchorInvalid(const NodePtr &node, const std::vector<NodePtr> &scope_nodes) const;
  Status MergeInputAnchors(const ComputeGraphPtr &subgraph,
                           const NodePtr &node, const std::vector<NodePtr> &scope_nodes);
  Status MergeOutputAnchors(const ComputeGraphPtr &subgraph,
                            const NodePtr &node, const std::vector<NodePtr> &scope_nodes);
  Status ProcPartitionInputAnchor(const ComputeGraphPtr &subgraph, const OutDataAnchorPtr &node_anchor,
                                  const InDataAnchorPtr &partition_anchor);
  Status ProcPartitionOutputAnchor(const OutDataAnchorPtr &partition_anchor,
                                   const InDataAnchorPtr &node_anchor);
  Status SetParentIndexToData(const ComputeGraphPtr &subgraph);
  Status SetParentIndexToNetOutput(const ComputeGraphPtr &subgraph);
  Status SetDataDynDimsInfo(const NodePtr &node, const NodePtr &data_node, const int32_t input_index);
  Status CopyPartitionedCallAttrToData(const ComputeGraphPtr &subgraph);
  Status CreateNewPartitionedCall(const ComputeGraphPtr &root_graph, const ComputeGraphPtr &subgraph,
                                  const std::vector<NodePtr> &scope_nodes);
  Status CopyPartitionedCallStaticInput(const NodePtr &src_node, const std::vector<NodePtr> &scope_nodes);
  Status CopyPartitionedCallStaticOutput(const NodePtr &src_node, const std::vector<NodePtr> &scope_nodes);
  Status AddSubgraphToNewPartitionedCall(const ComputeGraphPtr &root_graph, const ComputeGraphPtr &subgraph);

  std::map<int32_t, std::vector<NodePtr>> scopes_;
  std::map<InDataAnchorPtr, NodePtr> anchor_to_data_nodes_;
  std::map<OutDataAnchorPtr, InDataAnchorPtr> anchor_to_output_;
  std::map<NodePtr, std::map<int64_t, std::vector<int64_t>>> node_to_input_shape_;
  std::map<NodePtr, std::map<int64_t, std::vector<std::vector<int64_t>>>> node_to_multi_dims_;
  NodePtr new_partitioned_call_;
};
}  // namespace ge
#endif  // GE_GRAPH_PASSES_CREATE_SUBGRAPH_WITH_SCOPE_PASS_H_
