/**
 * Copyright 2019-2020 Huawei Technologies Co., Ltd
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

#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_UB_FUSION_BUFFER_FUSION_H_
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_UB_FUSION_BUFFER_FUSION_H_

#include <memory>
#include <utility>

#include "common/fe_inner_attr_define.h"
#include "common/graph_comm.h"
#include "common/pass_manager.h"
#include "fusion_config_manager/fusion_priority_manager.h"
#include "graph/compute_graph.h"
#include "graph_optimizer/fusion_common/fusion_pass_manager.h"
#include "graph_optimizer/ub_fusion/buffer_fusion_pass_runner.h"
#include "graph_optimizer/ub_fusion/connectivity_matrix.h"
#include "graph_optimizer/ub_fusion/fusion_graph_merge/fusion_graph_merge.h"
#include "graph_optimizer/ub_fusion/fusion_graph_merge/ub_fusion_graph_merge.h"
#include "graph_optimizer/ub_fusion/fusion_graph_merge/l1_fusion_graph_merge.h"
#include "register/graph_optimizer/buffer_fusion/buffer_fusion_pass_registry.h"

namespace fe {
/** @brief ub fusion: find subgraphs that match fusion patterns from graph firstly,
*        and fusion ops into one, change graph topology structure correspondingly. */
class BufferFusion {
  using GraphCommPtr = std::shared_ptr<GraphComm>;
  using FusionPassMgrPtr = std::shared_ptr<FusionPassManager>;
  using BufferFusionPassRunnerPtr = std::shared_ptr<BufferFusionPassRunner>;
  using FusionPriorityMgrPtr = std::shared_ptr<FusionPriorityManager>;
  using ConnectivityPtr = std::shared_ptr<ConnectivityMatrix>;
  using FusionCycleDetectorPtr = std::shared_ptr<FusionCycleDetector>;
  using FusionGraphMergeUniquePtr = std::unique_ptr<FusionGraphMerge>;

 public:
  BufferFusion(GraphCommPtr graph_comm_ptr, FusionPassMgrPtr fusion_pass_mgr_ptr,
               FusionPriorityMgrPtr fusion_priority_mgr_ptr,
               FusionCycleDetectorPtr cycle_detector = nullptr)
      : fusion_pass_mgr_ptr_(std::move(fusion_pass_mgr_ptr)),
        fusion_priority_mgr_ptr_(std::move(fusion_priority_mgr_ptr)),
        cycle_detector_(std::move(cycle_detector)) {
    ub_fusion_graph_merge_ptr_ =
        std::unique_ptr<FusionGraphMerge>(new (std::nothrow) UBFusionGraphMerge(SCOPE_ID_ATTR, graph_comm_ptr));
    l1_fusion_graph_merge_ptr_ =
        std::unique_ptr<FusionGraphMerge>(new (std::nothrow) L1FusionGraphMerge(L1_SCOPE_ID_ATTR, graph_comm_ptr));
  }

  ~BufferFusion() {}

  /*
   * @brief: match defined fusion pattern from graph and assign scope id to fusion op
   */
  Status MatchFusionPatternFromGraph(ge::ComputeGraph &graph);

  /*
   * @brief: create fusion graph with scope_id create by MatchFusionPatternFromGraph,
   *        i.e. nodes have same scope_id will be fused into one fusion op,
   *        the topo of graph will be changed.
   */
  Status BuildFusionGraph(ge::ComputeGraph &graph);

  void SetEngineName(std::string engine_name) { engine_name_ = engine_name; }
  Status MatchFusionPattern(ge::ComputeGraph &graph);

 private:
  Status AddVectorCorePass(const std::shared_ptr<PassManager> &tbe_ub_fusion_pass);
  Status RunBuiltInFusion(ge::ComputeGraph &graph);
  Status RunUnRegisterBufferFusionPass(ge::ComputeGraph &graph);
  Status RunRegisterBufferFusionPass(ge::ComputeGraph &graph, BufferFusionPassType pass_type);

  FusionPassMgrPtr fusion_pass_mgr_ptr_;
  FusionPriorityMgrPtr fusion_priority_mgr_ptr_;

  FusionGraphMergeUniquePtr ub_fusion_graph_merge_ptr_;
  FusionGraphMergeUniquePtr l1_fusion_graph_merge_ptr_;
  std::shared_ptr<fe::FusionCycleDetector> cycle_detector_;
  std::string engine_name_;
};

}  // namespace fe

#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_UB_FUSION_BUFFER_FUSION_H_
