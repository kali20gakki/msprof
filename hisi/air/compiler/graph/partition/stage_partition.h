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

#ifndef GE_GRAPH_PARTITION_STAGE_PARTITION_H_
#define GE_GRAPH_PARTITION_STAGE_PARTITION_H_

#include "framework/common/ge_inner_error_codes.h"
#include "graph/compute_graph.h"

namespace ge {
class StagePartitioner {
 public:
  explicit StagePartitioner(ComputeGraphPtr graph) : root_graph_(std::move(graph)) {}
  ~StagePartitioner() = default;

  Status Partition();

 private:
  Status SplitStageLevel();

  Status StagePartition();

  ComputeGraphPtr root_graph_;
  std::map<uint32_t, std::set<NodePtr>> stage_nodes_;
};
}  // namespace ge

#endif  // GE_GRAPH_PARTITION_STAGE_PARTITION_H_
