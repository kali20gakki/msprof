/**
 * Copyright 2022-2023 Huawei Technologies Co., Ltd
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

#ifndef AIR_COMPILER_GRAPH_COMPILER_ENGINES_NNENG_OPTIMIZER_GRAPH_OPTIMIZER_NODE_OPTIMIZER_SPLIT_C_TO_N_OPTIMIZER_H_
#define AIR_COMPILER_GRAPH_COMPILER_ENGINES_NNENG_OPTIMIZER_GRAPH_OPTIMIZER_NODE_OPTIMIZER_SPLIT_C_TO_N_OPTIMIZER_H_
#include "graph_optimizer/node_optimizer/split_n_optimizer.h"
#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/node_optimize/node_optimize_pass_base.h"

namespace fe {
class SplitCToNOptimizer {
 public:
  Status SetFusionVirtualOp(const ge::ComputeGraph &graph) const;
 private:
  bool NeedSkip(const ge::ComputeGraph &graph, const ge::NodePtr &node, const ge::OpDescPtr &op_desc) const;
  bool CheckSplitDim(const ge::OpDescPtr &op_desc) const;
  bool CheckCommonCondition(const ge::ComputeGraph &graph, const ge::NodePtr &node,
                            const ge::OpDescPtr &op_desc) const;
  bool CheckAxis(const ge::OpDescPtr &op_desc) const;
  bool MeetAlignmentConditionFromNCHWTo5HD(const ge::OpDescPtr &op_desc) const;
  bool MeetDimNumConditionFromNDToNZ(const ge::OpDescPtr &op_desc) const;
  bool IsDynamicGraph(const ge::ComputeGraph &graph) const;
  SplitOptimizer checker_helper;
};
}  // namespace fe
#endif  // AIR_COMPILER_GRAPH_COMPILER_ENGINES_NNENG_OPTIMIZER_GRAPH_OPTIMIZER_NODE_OPTIMIZER_SPLIT_C_TO_N_OPTIMIZER_H_