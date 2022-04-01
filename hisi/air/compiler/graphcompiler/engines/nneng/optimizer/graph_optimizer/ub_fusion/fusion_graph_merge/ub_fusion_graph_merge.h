/**
 * Copyright 2019-2021 Huawei Technologies Co., Ltd
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

#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_UB_FUSION_UB_FUSION_GRAPH_MERGE_H_
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_UB_FUSION_UB_FUSION_GRAPH_MERGE_H_

#include "graph_optimizer/ub_fusion/fusion_graph_merge/fusion_graph_merge.h"

namespace fe {
class UBFusionGraphMerge : public FusionGraphMerge {
 public:
  UBFusionGraphMerge(const std::string &scope_attr, const GraphCommPtr &graph_comm_ptr);
  ~UBFusionGraphMerge() override;

 private:
  Status AfterMergeFusionGraph(ge::ComputeGraph &graph) override;
};
}

#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_UB_FUSION_UB_FUSION_GRAPH_MERGE_H_
