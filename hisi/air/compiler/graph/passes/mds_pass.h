/**
 * Copyright 2021 Huawei Technologies Co., Ltd
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
#ifndef MAIN_GRAPHENGINE_GE_GRAPH_PASSES_MDS_H_
#define MAIN_GRAPHENGINE_GE_GRAPH_PASSES_MDS_H_

#include "graph/types.h"
#include "ge/ge_api.h"
#include "graph/debug/ge_attr_define.h"
#include "inc/graph_pass.h"
#include "graph/passes/mds_kernels/base_mds_kernel.h"
#include "ge/ge_api_types.h"
#include "graph/passes/mds_kernels/mds_utils.h"
#include "graph/partition/engine_place.h"
#include "opskernel_manager/ops_kernel_manager.h"
#include "init/gelib.h"


namespace ge {
class ModelDeploySchedulerPass : public GraphPass {
 public:
  Status Run(ge::ComputeGraphPtr graph) override;

 private:
  // Part0:Process Func
  // cut and dynamic cut
  Status CutProcess();
  Status CutNProcess(const ComputeGraphPtr &compute_graph, bool is_dynamic = false);
  Status CutHProcess(const ComputeGraphPtr &compute_graph, bool is_dynamic = false);
  Status DynamicCutAll(const ComputeGraphPtr &compute_graph);
  // smdp
  Status SMDPProcess(bool before_cut = true) const;
  Status SMDPModelState() const;
  Status SMDPGradient() const;
  Status SMDPWeight() const;

  // pipline
  Status PiplineProcess() const;
  // set delpoyinfo
  Status SetDeployInfo();

  // Part1: Utils Func
  Status HcomNodeFusionProcess() const;
  Status SetNodeCutInfo (const ComputeGraphPtr &compute_graph) const;
  std::vector<NodePtr> GetAllGradComputeNodes() {
    return grad_compute_nodes_;
  }
  const char *GetGraphName() const {
    return compute_graph_->GetName().c_str();
  }

  // members
  std::vector<NodePtr> model_state_vars_;
  std::vector<NodePtr> model_weight_vars_;
  std::vector<NodePtr> grad_compute_nodes_;
  NodePtr input_node_{nullptr};
  ComputeGraphPtr compute_graph_{nullptr};
};
}  // namespace ge
#endif  // MAIN_GRAPHENGINE_GE_GRAPH_PASSES_MDS_H_
