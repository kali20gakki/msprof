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

#ifndef GRAPH_PARTITION_OPTIMIZER_DYNAMIC_DATA_FLOW_PARTITIONER_PASS_H_
#define GRAPH_PARTITION_OPTIMIZER_DYNAMIC_DATA_FLOW_PARTITIONER_PASS_H_
#include "graph/partition/dynamic_shape_partition.h"

namespace ge {
class DynamicDataFlowPartitionerPass : public PartitionerPass {
public:
  DynamicDataFlowPartitionerPass() = default;
  ~DynamicDataFlowPartitionerPass() override = default;
  Status Run(ge::ComputeGraphPtr &graph, const std::vector<std::shared_ptr<BaseCluster>> &sorted_unique_clusters, 
             bool &result) override;
  Status MarkDataFlowOpAttr(const ge::ComputeGraphPtr &graph) const;
  Status GetDataFlowOpMapping(const ge::ComputeGraphPtr &graph);
  Status GetUnknownDataFlowOp();
  Status GetDataFlowOpNeedProcess(bool &result);
  Status GetDataFlowOpAttr(const std::vector<std::shared_ptr<BaseCluster>> &sorted_unique_clusters);
  void ClearDataFlowsource();

  std::unordered_set<int64_t> data_flow_handle_;
  std::unordered_set<NodePtr> unknown_data_flow_ops_;
  std::unordered_map<int64_t, std::unordered_set<NodePtr>> data_flow_ops_;
  std::unordered_map<NodePtr, bool> data_flow_ops_attr_;
};

}  // namespace ge

#endif  // GRAPH_PARTITION_OPTIMIZER_DYNAMIC_DATA_FLOW_PARTITIONER_PASS_H_