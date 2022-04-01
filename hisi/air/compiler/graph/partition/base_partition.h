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

#ifndef GE_GRAPH_PARTITION_BASE_PARTITION_H_
#define GE_GRAPH_PARTITION_BASE_PARTITION_H_

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "framework/common/ge_inner_error_codes.h"
#include "graph/compute_graph.h"
#include "graph/partition/base_cluster.h"

namespace ge {
class BasePartitioner {
 public:
  friend class BaseCluster;
  using ClusterFilter = std::function<bool(const std::shared_ptr<BaseCluster> &cluster)>;
  BasePartitioner(ge::ComputeGraphPtr graph) : root_graph_(graph) {}
  ~BasePartitioner() = default;

  virtual Status Partition() = 0;

  virtual std::string GetSubgraphName(const BaseCluster &cluster) = 0;

  virtual bool IsNeedBuildPartitionFrame(const BaseCluster &cluster) = 0;

  void ClearResource();

  void DumpGraph(const std::string &suffix) const;

  void MergeClustersControlFlow();

  Status TopologicalSortClusters(const ClusterFilter &cluster_filter);

  // Establish the input-output anchors for each partition of the cluster and record links to other clusters
  Status BuildPartitionFrame() const;
  // Establish connection between corresponding partitioned of clusters
  Status CombinePartitionFrame() const;
  // Convert the nodes in cluster into a complete ComputeGraph
  Status BuildPartitionSubgraph() const;

 protected:
  ge::ComputeGraphPtr root_graph_;

  std::unordered_map<NodePtr, std::shared_ptr<BaseCluster>> node_2_cluster_;  // Record nodes and the cluster it belongs to
  // V1 control flow cluster, need merge to one Graph.
  std::map<int64_t, std::vector<std::shared_ptr<BaseCluster>>> control_clusters_;
  std::map<int64_t, std::vector<NodePtr>> control_nodes_;
  // topological sorted clusters, this field will change with the splitting.
  // When partitioning UNKNOWN_SHAPE cluster, it is a collection of all topological sorted UNKNOWN_SHAPE clusters
  // When partitioning KNOWN_SHAPE cluster, it is a collection of all topological sorted KNOWN_SHAPE clusters
  std::vector<std::shared_ptr<BaseCluster>> ordered_cluster_;
  // Unique clusters left after merged clusters
  std::unordered_set<std::shared_ptr<BaseCluster>> unique_clusters_;
  // Unique clusters left after merged clusters sorted by rank
  std::vector<std::shared_ptr<BaseCluster>> sorted_unique_clusters_;

  std::map<int32_t, std::string> type_index_to_type_;  // key:type index. value:cluster type
};
} // namespace ge

#endif // GE_GRAPH_PARTITION_BASE_PARTITION_H_
