/**
 * Copyright 2020-2021 Huawei Technologies Co., Ltd
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

#ifndef AIR_COMPILER_GRAPHCOMPILER_ENGINES_NNENG_OPTIMIZER_FUSION_RULE_MANAGER_FUSION_CYCLE_DETECTOR_H_
#define AIR_COMPILER_GRAPHCOMPILER_ENGINES_NNENG_OPTIMIZER_FUSION_RULE_MANAGER_FUSION_CYCLE_DETECTOR_H_
#include "register/graph_optimizer/fusion_common/pattern_fusion_base_pass.h"
namespace fe {
class FusionCycleDetector : public PatternFusionBasePass {
 public:
  FusionCycleDetector();

  ~FusionCycleDetector() override;

  std::vector<FusionPattern *> DefinePatterns() override;

  Status Fusion(ge::ComputeGraph &graph, Mapping &mapping,
                std::vector<ge::NodePtr> &new_nodes) override;

  Status Initialize(const ge::ComputeGraph &graph);

  Status UpdateConnectionMatrix(const ge::ComputeGraph &graph, vector<ge::NodePtr> &fusion_nodes);

  bool IsConnected(const ge::NodePtr &a, const ge::NodePtr &b);
};


}
#endif // AIR_COMPILER_GRAPHCOMPILER_ENGINES_NNENG_OPTIMIZER_FUSION_RULE_MANAGER_FUSION_CYCLE_DETECTOR_H_
