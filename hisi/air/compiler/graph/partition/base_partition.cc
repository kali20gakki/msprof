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

#include "graph/partition/base_partition.h"
#include "graph/utils/graph_utils.h"
#include "framework/common/debug/ge_log.h"
#include "framework/common/debug/log.h"

#include <vector>
#include <queue>

#define REQUIRE(cond, ...)                                     \
  do {                                                         \
    if (!(cond)) {                                             \
      REPORT_INNER_ERROR("E19999", __VA_ARGS__);               \
      GELOGE(FAILED, "[partition]" __VA_ARGS__); \
      return FAILED;                                           \
    }                                                          \
  } while (false)

#define REQUIRE_NOT_NULL(cond, ...) REQUIRE(((cond) != nullptr), __VA_ARGS__)
#define REQUIRE_SUCCESS(cond, ...) REQUIRE(((cond) == SUCCESS), __VA_ARGS__)
#define REQUIRE_GRAPH_SUCCESS(cond, ...) REQUIRE(((cond) == GRAPH_SUCCESS), __VA_ARGS__)

namespace ge {
using Cluster = BaseCluster;
using ClusterPtr = std::shared_ptr<Cluster>;

void BasePartitioner::DumpGraph(const std::string &suffix) const {
  GraphUtils::DumpGEGraphToOnnx(*root_graph_, root_graph_->GetName() + suffix);
  for (const auto &sub_graph : root_graph_->GetAllSubgraphs()) {
    GraphUtils::DumpGEGraphToOnnx(*sub_graph, sub_graph->GetName() + suffix);
  }
}

void BasePartitioner::MergeClustersControlFlow() {
  for (const auto &item : control_nodes_) {
    const auto &control_node = item.second;
    for (auto it_front = control_node.begin(); it_front < control_node.end(); ++it_front) {
      for (auto it_back = it_front + 1; it_back < control_node.end(); ++it_back) {
        const auto &cluster_back = node_2_cluster_[*it_back];
        const auto &cluster_from = node_2_cluster_[*it_front];
        if (cluster_from == cluster_back) {
          continue;
        }
        auto merged_clusters = cluster_back->MergeAllPathFrom(cluster_from);
        GELOGD("Merge all path cluster from %lu to %lu %s.", cluster_from->Id(), cluster_back->Id(),
               ToString(merged_clusters).c_str());
        for (const auto &merged_cluster : merged_clusters) {
          for (const auto &node : merged_cluster->Nodes()) {
            node_2_cluster_[node] = cluster_back;
          }
        }
      }
    }
  }
}

Status BasePartitioner::TopologicalSortClusters(const ClusterFilter &cluster_filter) {
  ordered_cluster_.clear();
  // BFS topological sort clusters for known shape cluster
  std::queue<ClusterPtr> ready_clusters;
  std::unordered_map<ClusterPtr, size_t> cluster_pending_count;
  std::unordered_set<ClusterPtr> seen_clusters;
  for (auto &node : root_graph_->GetDirectNode()) {
    auto &cluster = node_2_cluster_[node];
    if (seen_clusters.count(cluster) != 0) {
      continue;
    }
    seen_clusters.insert(cluster);
    auto pending_count = cluster->Inputs().size();
    if (pending_count == 0) {
      ready_clusters.push(cluster);
    } else {
      cluster_pending_count[cluster] = pending_count;
    }
  }

  size_t rank = 0;
  while (!ready_clusters.empty()) {
    auto cluster = ready_clusters.front();
    ready_clusters.pop();
    cluster->UpdateRank(rank++);
    if (cluster_filter == nullptr || cluster_filter(cluster)) {
      ordered_cluster_.push_back(cluster);
    }
    for (const auto &out_cluster : cluster->Outputs()) {
      if (cluster_pending_count[out_cluster] > 0 && --cluster_pending_count[out_cluster] == 0) {
        ready_clusters.push(out_cluster);
      }
    }
  }
  if (rank != seen_clusters.size()) {
    return FAILED;
  }
  return SUCCESS;
}

Status BasePartitioner::BuildPartitionFrame() const {
  for (const auto &cluster : sorted_unique_clusters_) {
    REQUIRE_SUCCESS(cluster->BuildFrame(), "[Build][Frame] of cluster[%lu] failed.", cluster->Id());
  }
  return SUCCESS;
}

Status BasePartitioner::CombinePartitionFrame() const {
  for (const auto &cluster : sorted_unique_clusters_) {
    REQUIRE_SUCCESS(cluster->CombinePartitionFrame(), "[Combine][Frame] of cluster[%lu] failed.", cluster->Id());
  }
  return SUCCESS;
}

Status BasePartitioner::BuildPartitionSubgraph() const {
  for (const auto &cluster : sorted_unique_clusters_) {
    REQUIRE_SUCCESS(cluster->BuildPartitionSubgraph(), "[Build][SubGraph] of cluster[%lu] failed.", cluster->Id());
  }
  return SUCCESS;
}

void BasePartitioner::ClearResource() {
  for (const auto &cluster : unique_clusters_) {
    cluster->Clear();
  }
  node_2_cluster_.clear();
  control_clusters_.clear();
  control_nodes_.clear();
  ordered_cluster_.clear();
  unique_clusters_.clear();
  sorted_unique_clusters_.clear();
  type_index_to_type_.clear();
  root_graph_.reset();
}
}  // namespace ge

