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

#ifndef GE_GRAPH_PASSES_SUBGRAPH_MULTI_DIMS_CLONE_PASS_H_
#define GE_GRAPH_PASSES_SUBGRAPH_MULTI_DIMS_CLONE_PASS_H_

#include <map>
#include <string>
#include <vector>

#include "inc/graph_pass.h"

namespace ge {
class SubgraphMultiDimsClonePass : public GraphPass {
 public:
  Status Run(ComputeGraphPtr graph);

 private:
  Status MergeDataDynDims();
  Status CreateOriGraph(const ComputeGraphPtr &subgraph);
  Status CollectIoNodes(const ComputeGraphPtr &subgraph);
  Status ParseInputShapeAndDynDims(ComputeGraphPtr &subgraph);
  Status CreateGetShapeNode(const ComputeGraphPtr &subgraph);
  Status CreateMapIndexNode(const ComputeGraphPtr &subgraph);
  Status CreateIndexConstNode(const ComputeGraphPtr &subgraph);
  Status CreateCaseNode(const ComputeGraphPtr &subgraph);
  Status CreateConstNode(const ComputeGraphPtr &subgraph);
  Status CreateInputNode(const ComputeGraphPtr &subgraph);
  Status CreateOutputNode(const ComputeGraphPtr &subgraph);
  Status CreateSubgraphs(const ComputeGraphPtr &root_graph, const ComputeGraphPtr &subgraph,
                         const ComputeGraphPtr &branch);
  Status UpdateSubgraphData(const NodePtr &data, size_t grade_index) const;

  std::vector<NodePtr> all_data_nodes_;
  std::vector<NodePtr> all_const_nodes_;
  NodePtr output_node_;

  std::string input_shapes_;
  std::string input_dynamic_dims_;
  std::string input_mode_;

  NodePtr get_shape_node_;
  NodePtr map_index_node_;
  NodePtr const_node_;
  NodePtr case_node_;

  std::vector<std::vector<int64_t>> merged_multi_dims_;
};
}  // namespace ge
#endif  // GE_GRAPH_PASSES_MULTI_BATCH_CLONE_PASS_H_
