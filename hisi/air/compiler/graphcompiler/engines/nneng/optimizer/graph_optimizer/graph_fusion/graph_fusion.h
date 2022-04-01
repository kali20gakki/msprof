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

#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_GRAPH_FUSION_GRAPH_FUSION_H_
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_GRAPH_FUSION_GRAPH_FUSION_H_

#include <map>
#include <memory>
#include <string>

#include "common/pass_manager.h"
#include "fusion_config_manager/fusion_priority_manager.h"
#include "fusion_rule_manager/fusion_rule_data/fusion_rule_pattern.h"
#include "fusion_rule_manager/fusion_rule_manager.h"
#include "graph/compute_graph.h"
#include "graph_optimizer/fusion_common/fusion_pass_manager.h"
#include "register/graph_optimizer/fusion_common/pattern_fusion_base_pass.h"
#include "register/graph_optimizer/graph_fusion/fusion_pass_manager/fusion_pass_registry.h"
#include "register/graph_optimizer/graph_fusion/graph_fusion_pass_base.h"

namespace fe {
enum CastOptimizationType {
  OPTIMIZE_WITH_TRANSDATA_IN_FRONT = 0,
  OPTIMIZE_WITH_TRANSDATA_AT_TAIL,
  CAST_OPMIZATION_BOTTOM
};

using RuleMgrPtr = std::shared_ptr<FusionRuleManager>;
using FusionPassMgrPtr = std::shared_ptr<FusionPassManager>;
using FusionPriorityMgrPtr = std::shared_ptr<FusionPriorityManager>;

class GraphFusion {
 public:
  explicit GraphFusion(RuleMgrPtr fusion_rule_mgr_ptr, FEOpsKernelInfoStorePtr ops_kernel_info_store_ptr,
                       FusionPassMgrPtr fusion_pass_mgr_ptr, FusionPriorityMgrPtr fusion_priority_mgr_ptr);
  virtual ~GraphFusion();

  GraphFusion(const GraphFusion &) = delete;
  GraphFusion &operator=(const GraphFusion &) = delete;

  Status Fusion(ge::ComputeGraph &graph);

  /* After inserting TransData, we do the graph fusion second time. */
  Status RunGraphFusionPassByType(const string &stage, ge::ComputeGraph &graph, GraphFusionPassType type);

  /* if the TransData is low-performance dtype such as bool, int32, double,
   * and the edges with low-performance dtype is connected with a cast which
   * transform the special dtype to fp16 or fp32, we just
   * change the running sequence of these two op. For example:
   *
   * A -> TransData(4->5, bool) -> Cast(5HD, bool->fp16) -> B (5HD, fp16)
   * will be changed to :
   * A -> Cast(4D, bool->fp16) -> TransData(4->5, fp16) -> B (5HD, fp16)
   *
   * The following case is RARE.
   * A -> TransData(5->4, bool) -> Cast(4D, bool->fp16) -> B (4D, fp16)
   * will be changed to :
   * A -> Cast(5D, bool->fp16) -> TransData(5->4, fp16) -> B (4D, fp16)
   *
   * A -> Cast(5HD, fp16->bool) -> TransData(5->4, bool) -> B (4D, bool)
   * will be changed to :
   * A -> TransData(5->4, fp16) -> Cast(4D, fp16->bool) -> B (4D, bool)
   *
   * The following case is RARE.
   * A -> Cast(4D, fp16->bool) -> TransData(4->5, bool) -> B (5HD, bool)
   * will be changed to :
   * A -> TransData(4->5, fp16) -> Cast(5HD, fp16->bool) -> B (5HD, bool)
   *
   * An Special Case is :
   * A -> TransData(4->5, bool) -> Cast(5HD, bool->fp32)-> Cast(5HD, fp32->fp16)
   * -> B(5HD, fp16)
   * This case will be optimized as:
   * A -> TransData(4->5, bool) -> Cast(5HD, bool->fp16)-> B(5HD, fp16)
   * then:
   * A -> Cast(4D, bool->fp16) -> TransData(4->5, fp16) -> B(5HD, fp16)
   * */
  Status SwitchTransDataAndCast(ge::ComputeGraph &graph, const vector<ge::NodePtr> &special_cast_list) const;

  Status ComputeTensorSize(ge::NodePtr &cast, ge::NodePtr &transdata) const;

  Status SetContinuousDtypeForSingleNode(ge::NodePtr &node) const;

  Status SetContinuousDtypeForOutput(const ge::ComputeGraph &graph) const;

  Status FusionQuantOp(ge::ComputeGraph &graph);
  Status TagNoConstFolding(ge::ComputeGraph &graph);

  void SetEngineName(const std::string &engine_name) { engine_name_ = engine_name; }

  bool CheckGraphCycle(ge::ComputeGraph &graph) const;

 private:
  Status FusionEachGraph(ge::ComputeGraph &graph);

  Status FusionQuantOpOfEachGraph(ge::ComputeGraph &graph) const;

  Status RunBuiltInFusionByType(ge::ComputeGraph &graph, GraphFusionPassType pass_type);

  Status RunUnRegisterGraphFusionPass(ge::ComputeGraph &graph);

  /*
   *  @ingroup fe
   *  @brief   one set rule pattern graph fusion
   *  @param   [in|out] graph compute graph
   *  @return  SUCCESS or FAILED
   */
  Status RunOneRuleFusion(ge::ComputeGraph &graph, const FusionPassOrRule &pass_or_rule);

  /*
   *  @ingroup fe
   *  @brief   one set pass pattern graph fusion
   *  @param   [in|out] graph compute graph
   *  @return  SUCCESS or FAILED
   */
  Status RunOnePassFusion(ge::ComputeGraph &graph, const FusionPassOrRule &pass_or_rule);

  Status RunOnePassFusionByType(ge::ComputeGraph &graph, const FusionPassOrRule &pass_or_rule,
                                const GraphFusionPassType &pass_type);

  uint32_t GraphFusionQuantByPass(ge::ComputeGraph &graph) const;

  uint32_t JudgeQuantMode(const ge::ComputeGraph &graph) const;

  uint32_t GraphFusionTagNoConstFolding(ge::ComputeGraph &graph);

  void ReportAfterCheckGraphCycle(const ge::ComputeGraph &graph, const FusionPassOrRule &pass_or_rule,
                                  GraphFusionPassType pass_type = GRAPH_FUSION_PASS_TYPE_RESERVED) const;

  // rules mngr
  RuleMgrPtr fusion_rule_mgr_ptr_{nullptr};
  FEOpsKernelInfoStorePtr ops_kernel_info_store_ptr_{nullptr};
  FusionPassMgrPtr fusion_pass_mgr_ptr_{nullptr};
  FusionPriorityMgrPtr fusion_priority_mgr_ptr_{nullptr};
  std::string engine_name_;
};
}  // namespace fe
#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_GRAPH_FUSION_GRAPH_FUSION_H_