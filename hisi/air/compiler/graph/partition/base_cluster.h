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

#ifndef GE_GRAPH_PARTITION_BASE_CLUSTER_H_
#define GE_GRAPH_PARTITION_BASE_CLUSTER_H_

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "framework/common/ge_inner_error_codes.h"
#include "graph/compute_graph.h"

namespace ge {
// An cluster means set of nodes that can be merged in same partition
class BasePartitioner;
class BaseCluster : public std::enable_shared_from_this<BaseCluster> {
 public:
  BaseCluster(size_t rank, int32_t type_index, NodePtr node, BasePartitioner *partitioner)
      : id_(rank), min_(rank), max_(rank), type_index_(type_index), partitioner_(partitioner) {
    nodes_.push_back(node);
  }
  ~BaseCluster() = default;
  std::string DebugString() const;
  // Basic bean functions
  size_t Id() const;
  size_t UniqueId() const;
  void UpdateRank(size_t rank);
  bool IsData() const;
  bool IsKnownShape() const;
  bool IsUnknownShape() const;
  bool IsIndependent() const;
  bool IsNetOutput() const;
  std::vector<std::shared_ptr<BaseCluster>> Inputs() const;
  std::vector<std::shared_ptr<BaseCluster>> Outputs() const;
  bool IsInputNode() const;
  std::vector<NodePtr> Nodes() const;
  bool IsRefVariable() const;
  // BaseCluster modify functions
  void AddInput(std::shared_ptr<BaseCluster> in);
  void RemoveInput(std::shared_ptr<BaseCluster> in);
  void AddOutput(std::shared_ptr<BaseCluster> out);
  void RemoveOutput(std::shared_ptr<BaseCluster> out);
  // Merge other cluster to this cluster, Whether it leads to a ring or not
  // Merge src to dst means:
  // All links to src will break and link to dst instead
  // All nodes of src will change its owner to dst
  // Update max and min rank of dst
  void Merge(std::shared_ptr<BaseCluster> other);
  // Try merge other cluster to this cluster, ONLY if will not leads to a ring
  bool TryMerge(std::shared_ptr<BaseCluster> other);
  // Merge all clusters on path(s) from other to this
  std::vector<std::shared_ptr<BaseCluster>> MergeAllPathFrom(std::shared_ptr<BaseCluster> other);
  // Convert cluster to functioned call functions
  bool AddFrameInput(InDataAnchorPtr anchor);
  void AddFrameOutput(OutDataAnchorPtr anchor);
  InDataAnchorPtr GetFrameInDataAnchor(InDataAnchorPtr anchor);
  OutDataAnchorPtr GetFrameOutDataAnchor(OutDataAnchorPtr anchor);
  InControlAnchorPtr GetFrameInControlAnchor();
  OutControlAnchorPtr GetFrameOutControlAnchor();
  Status BuildFrame();
  Status BuildPartitionFrame();
  Status CombinePartitionFrame();
  Status BuildPartitionSubgraph();
  // Clear resource and break circular dependency
  void Clear();
  bool IsAdjoinNodes(const std::shared_ptr<BaseCluster> &other) const {
    const auto &out_clusters = other->out_clusters_;
    return std::find(out_clusters.begin(), out_clusters.end(), shared_from_this()) != out_clusters.end();
  }
  int32_t GetTypeIndex() const { return type_index_; }

  void SetMergeInputs(bool merge_inputs);

 private:
  static thread_local size_t unique_id_;
  size_t id_;
  // Each Cluster records the maximum and minimum topological order of its node
  size_t min_;  // maximum topological order
  size_t max_;  // minimum topological order
  int32_t type_index_;
  std::vector<std::shared_ptr<BaseCluster>> in_clusters_;
  std::vector<std::shared_ptr<BaseCluster>> out_clusters_;
  std::vector<NodePtr> nodes_;
  // Fields for build partitioned call and subgraph
  BasePartitioner *partitioner_;  // Not owned, the partitioner this cluster belongs to
  std::unordered_map<InDataAnchorPtr, size_t> inputs_index_;
  std::unordered_map<OutDataAnchorPtr, size_t> outputs_index_;
  std::vector<InDataAnchorPtr> inputs_;
  std::map<size_t, std::vector<InDataAnchorPtr>> data_to_node_inputs_;
  std::map<std::string, size_t> src_key_to_frame_input_;
  std::vector<OutDataAnchorPtr> outputs_;
  std::unordered_set<std::shared_ptr<BaseCluster>> control_inputs_;
  std::unordered_set<OutControlAnchorPtr> control_outputs_;
  NodePtr partition_node_;    // corresponding partitioned call node
  ComputeGraphPtr subgraph_;  // corresponding subgraph
  bool merge_inputs_ = false;
};
}  // namespace ge

#endif // GE_GRAPH_PARTITION_BASE_CLUSTER_H_
