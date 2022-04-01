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

#ifndef GE_GRAPH_PARTITION_DYNAMIC_SHAPE_PARTITION_H_
#define GE_GRAPH_PARTITION_DYNAMIC_SHAPE_PARTITION_H_

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "framework/common/ge_inner_error_codes.h"
#include "graph/compute_graph.h"
#include "graph/partition/base_partition.h"

namespace ge {
class DynamicShapePartitioner : public BasePartitioner {
 public:
  DynamicShapePartitioner(ge::ComputeGraphPtr graph) : BasePartitioner(graph) {}
  ~DynamicShapePartitioner() = default;

  Status Partition() override;

 private:
  Status PartitionImpl();
  // Collect nodes that satisfy the unknowshape rules:
  // 1) The Tensor shape of any input or output is unknow shape(dim_size = -1) or unknow rank(dim_size=-2)
  // 2) Subgraphs of the node has an operator that satisfies rule 1)
  Status MarkUnknownShapeNodes();
  // For each node a Cluster structure, and connected according to the connection relationship of the nodes
  // An cluster means set of nodes that can be merged in same partition,
  // Corresponding relationship between cluster type and node:
  // DATA:DATA, UNKNOWN_SHAPE:unknowshape, KNOWN_SHAPE:knowshape, NETOUTPUT:NETOUTPUT
  Status InitClusters();
  // Merge clusters according to the following rules:
  // 1) Iterate through the UNKNOWN_SHAPE clusters, if the input is UNKNOWN_SHAPE,
  //    merge all the clusters in the path(s) between the two clusters
  // 2) Iterate through the KNOWN_SHAPE clusters, if the input is KNOWN_SHAPE, and
  //    and there's only one path between the two clusters , merge the two clusters
  // 3) Iterate through the INPUT_DATA clusters, merge all INPUT_DATA
  Status MergeClusters();
  Status InitClusterType();
  void MergeClustersUnknownShape();
  void MergeClustersKnownShape();
  void MergeClustersInputData();
  // Deduplicate merged clusters
  Status PruneUniqueClusters();
  // Clear resource and break circular dependency
  void ClearResource();
  // Debug functions
  std::string DebugString() const;
  bool JudgeUnknowShapeWithAttr(const OpDescPtr &opdesc) const;
  // Util functions
  Status CollectSpreadUnknownShapeNodes(NodePtr node);
  Status IsUnknownShapeGraph(ge::ComputeGraphPtr graph, bool &is_unknown);
  Status IsUnknownShapeNode(ge::NodePtr node, bool &is_unknown);
  Status CtrlEdgeTransfer() const;
  std::string GetSubgraphName(const BaseCluster &cluster);
  bool IsNeedBuildPartitionFrame(const BaseCluster &cluster);
  Status GetSubgraphsOfIndependentCompileGraph(const ComputeGraphPtr &independent_compile_graph,
                                               std::vector<ComputeGraphPtr> &all_subgraphs) const;
  bool IsNodeSupportNoTiling(const ConstNodePtr &node) const;
  Status MarkOpNoTiling(const NodePtr &node, bool no_tiling) const;
  void RevertOpNoTilingAttr(const NodePtr &node) const;
  Status ReDynamicShapePartitioner();
  void ClearReDataFlowResource();
  Status GenerateCluster();
  // Nodes of root_graph_ that satisfy the unknowshape rules
  std::unordered_set<NodePtr> unknown_shape_nodes_;
  // Nodes of root_graph_ that satisfy the unknowshape rules and support no tiling
  std::unordered_set<NodePtr> unknown_shape_no_tiling_nodes_;
};

class PartitionerPass {
public:
  PartitionerPass() = default;
  virtual ~PartitionerPass() = default;
  virtual Status Run(ge::ComputeGraphPtr &graph, const std::vector<std::shared_ptr<BaseCluster>> &sorted_unique_clusters, 
                     bool &result) = 0;
};
}  // namespace ge

#endif  // GE_GRAPH_PARTITION_DYNAMIC_SHAPE_PARTITION_H_
