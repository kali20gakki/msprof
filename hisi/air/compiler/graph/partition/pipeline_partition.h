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

#ifndef GE_GRAPH_PARTITION_PIPELINE_PARTITION_H_
#define GE_GRAPH_PARTITION_PIPELINE_PARTITION_H_

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "framework/common/ge_inner_error_codes.h"
#include "graph/compute_graph.h"
#include "graph/partition/base_cluster.h"
#include "graph/partition/base_partition.h"

namespace ge {
class PipelinePartitioner : public BasePartitioner {
 public:
  PipelinePartitioner(ge::ComputeGraphPtr graph) : BasePartitioner(graph) { sub_model_num_ = 0; }
  ~PipelinePartitioner() = default;

  Status Partition() override;

 private:
  Status PartitionImpl();
  // Mark ATTR_SUBMODEL_INDEX for each node on root graph, this attr means the index of sub model the node belong to
  Status MarkNodesModelIndex();
  Status InitClusters();
  Status InitClusterType();
  Status MergeClusters();
  // Merge clusters belong to the same sub model
  void MergeClustersSubModel();
  Status PruneUniqueClusters();
  // Clear resource and break circular dependency
  void ClearResource();
  // Debug functions
  std::string DebugString() const;
  std::string GetSubgraphName(const BaseCluster &cluster);
  // 1) If const node shared by multiple sub model, then we copy the const node to make sure each sub model have a copy of
  // the const node
  // 2) If the const node belong to sub model A according to our original partition rule, but the output of
  // the const node only serve as input in sub model B, then we unlink the const node from sub model A and link it to
  // sub model B
  Status AdjustInputNodes() const;
  // Decide whether current cluster need to build partition frame
  bool IsNeedBuildPartitionFrame(const BaseCluster &cluster);
  // Decide whether current graph need to execute pipeline partition through ATTR_NAME_OUTPUT_PIPELINE marked by user
  static bool IsPipelineNeeded(const ComputeGraphPtr &root_graph);
  Status CopyTensorAttrs(const OpDescPtr &dst_desc, const NodePtr &src_node) const;
  // Check if the result of pipeline partition is correct
  Status CheckPipelinePartitonResult() const;

  int32_t sub_model_num_;
};

} // namespace ge

#endif // GE_GRAPH_PARTITION_PIPELINE_PARTITION_H_
