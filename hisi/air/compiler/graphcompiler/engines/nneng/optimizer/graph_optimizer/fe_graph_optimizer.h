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

#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_FE_GRAPH_OPTIMIZER_H_
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_FE_GRAPH_OPTIMIZER_H_

#include <map>
#include <memory>
#include <string>
#include <vector>
#include "adapter/adapter_itf/op_store_adapter.h"
#include "common/fe_inner_error_codes.h"
#include "common/fe_utils.h"
#include "common/unknown_shape_util.h"
#include "common/fusion_statistic/buffer_fusion_info_collecter.h"
#include "common/fusion_statistic/fusion_statistic_writer.h"
#include "common/graph/fe_graph_utils.h"
#include "common/optimizer/graph_optimizer.h"
#include "common/optimizer/graph_optimizer_types.h"
#include "common/optimizer/optimize_utility.h"
#include "format_selector/manager/format_dtype_setter.h"
#include "fusion_rule_manager/fusion_rule_manager.h"
#include "graph/compute_graph.h"
#include "graph_optimizer/fuzzy_compiler/fuzzy_generalize.h"
#include "graph_optimizer/fusion_common/fusion_pass_manager.h"
#include "graph_optimizer/graph_fusion/graph_fusion.h"
#include "graph_optimizer/heavy_format_propagation/heavy_format_propagation.h"
#include "graph_optimizer/node_optimizer/concat_n_optimizer.h"
#include "graph_optimizer/node_optimizer/concat_c_to_n_optimizer.h"
#include "graph_optimizer/node_optimizer/split_c_to_n_optimizer.h"
#include "graph_optimizer/node_optimizer/split_n_optimizer.h"
#include "graph_optimizer/op_axis_update/op_axis_update_desc.h"
#include "graph_optimizer/op_compiler/op_compiler.h"
#include "graph_optimizer/op_compiler/op_compiler_baseline.h"
#include "graph_optimizer/op_compiler/op_compiler_normal.h"
#include "graph_optimizer/op_compiler/op_compiler_optune.h"
#include "graph_optimizer/op_compiler/op_compiler_mstune_before_ub_match.h"
#include "graph_optimizer/op_judge/format_and_dtype/op_format_dtype_judge.h"
#include "graph_optimizer/op_judge/imply_type/op_impl_type_judge.h"
#include "graph_optimizer/op_setter/op_setter.h"
#include "graph_optimizer/shape_format_transfer/trans_node_manager/trans_node_manager.h"
#include "graph_optimizer/spacesize_calculator/spacesize_calculator.h"
#include "graph_optimizer/stream_graph_optimizer/l2_optimizer/l2_optimizer.h"
#include "graph_optimizer/ub_fusion/automatic_buffer_fusion.h"
#include "graph_optimizer/ub_fusion/buffer_fusion.h"
#include "ops_kernel_store/fe_ops_kernel_info_store.h"
#include "common/fe_error_code.h"
#include "common/util/error_manager/error_manager.h"

namespace fe {
extern const string kStagePrepare;
extern const string kStageBeforeQuant;
extern const string kStageOrigin;
extern const string kStageAftFmtSlct;
extern const string kStageJudgeInsert;
extern const string kStageSetOpSlc;
extern const string kStagePreCompile;
extern const string kStageParseCompRst;
extern const string kStageLx;
extern const string kStageCompile;

using FEOpsKernelInfoStorePtr = std::shared_ptr<FEOpsKernelInfoStore>;
using OpImplyTypeJudgePtr = std::shared_ptr<OpImplTypeJudge>;
using OpSetterPtr = std::shared_ptr<OpSetter>;
using FormatDtypeSetterPtr = std::shared_ptr<FormatDtypeSetter>;
using OpFormatDtypeJudgePtr = std::shared_ptr<OpFormatDtypeJudge>;
using ReflectionBuilderPtr = std::shared_ptr<ge::RefRelations>;

using OpCompilerPtr = std::shared_ptr<OpCompiler>;
using SpaceSizeCalculatorPtr = std::shared_ptr<SpaceSizeCalculator>;
using TransNodeManagerPtr = std::shared_ptr<TransNodeManager>;
using L2OptimizerPtr = std::shared_ptr<L2Optimizer>;
using GraphFusionPtr = std::shared_ptr<GraphFusion>;
using RuleMgrPtr = std::shared_ptr<FusionRuleManager>;
using BufferFusionPtr = std::shared_ptr<BufferFusion>;
using AutomaticBufferFusionPtr = std::shared_ptr<AutomaticBufferFusion>;
using PassMgrPtr = std::shared_ptr<FusionPassManager>;
using TransNodeManagerPtr = std::shared_ptr<TransNodeManager>;
using HeavyFormatPropagationPtr = std::shared_ptr<HeavyFormatPropagation>;
using OpAxisUpdateDescPtr = std::shared_ptr<OpAxisUpdateDesc>;
using ConcatOptimizerPtr = std::shared_ptr<ConcatOptimizer>;
using ConcatCToNOptimizerPtr = std::shared_ptr<ConcatCToNOptimizer>;
using SplitCToNOptimizerPtr = std::shared_ptr<SplitCToNOptimizer>;
using SplitOptimizerPtr = std::shared_ptr<SplitOptimizer>;
using FusionPriorityMgrPtr = std::shared_ptr<FusionPriorityManager>;
using BufferFusionInfoCollecterPtr = std::shared_ptr<BufferFusionInfoCollecter>;
using TbeOpInfoPtr = std::shared_ptr<te::TbeOpInfo>;
using InputOrOutputInfoPtr = std::shared_ptr<fe::InputOrOutputInfo>;
using FuzzyGeneralizePtr = std::shared_ptr<fe::FuzzyGeneralize>;

enum OpCompilerIndex{
  /* Base class. All common operations are in this class. */
  COMMON = 0,
  /* Standard l2buffer atc inference process without op-tune.
   * 1. Compile and do fusion recovery.
   * 2. Parse Json and set attributes.
   * 3. Do post process. */
  BASELINE = 1,

  /* Standard lx-fusion atc inference process without op-tune.
   * 1. Compile and do fusion recovery.
   * 2. Call LxFusion.
   * 3. Do pre-compilation and compilation for all
   * nodes with attributes NEED_RE_PRECOMPILE.
   * 4. Parse Json and set attributes.
   * 5. Do post process. */
  NORMAL = 2,

  /* Standard lx-fusion atc inference process with op-tune.
   * 1. Compile and do fusion recovery.
   * 2. Call LxFusion.
   * 3. Do pre-compilation for all nodes with attribute NEED_RE_PRECOMPILE and compilation for all nodes.
   * 4. Do not collect compilation result and return SUCCESS. */
  OPTUNE = 3,

  /* First step of Ms-tune process.
   * 1. Compile and do fusion recovery.
   * 2. return the subgraph to tuning tool. */
  MSTUNE_BEFORE_UB_MATCH = 4,
  OP_COMPILER_BOTTOM = 5,
};
class FEGraphOptimizer : public ge::GraphOptimizer {
 public:
  FEGraphOptimizer(FEOpsKernelInfoStorePtr fe_ops_kernel_info_store_ptr,
                   OpStoreAdapterManagerPtr op_store_adapter_manager_ptr, std::string engine_name = fe::AI_CORE_NAME);
  ~FEGraphOptimizer() override;

  /**
   * @ingroup fe
   * @brief prohibit copy and assign construct
   */
  FEGraphOptimizer(const FEGraphOptimizer&) = delete;
  FEGraphOptimizer& operator=(const FEGraphOptimizer&) = delete;

  /*
   *  @ingroup fe
   *  @brief   initialize graph optimizer
   *  @param   [in] options
   *  @return  SUCCESS or FAILED
   */
  Status Initialize(const std::map<string, string>& options, ge::OptimizeUtility *const optimize_utility) override;

  /*
   *  @ingroup fe
   *  @brief   close graph optimizer
   *  @return  SUCCESS or FAILED
   */
  Status Finalize() override;

  /*
   *  @ingroup fe
   *  @brief   optimize original graph
   *  @param   [in|out] graph  compute graph
   *  @return  SUCCESS or FAILED
   */
  Status OptimizeOriginalGraph(ge::ComputeGraph& graph) override;

  /*
   *  @ingroup fe
   *  @brief   optimize fused graph
   *  @param   [in|out] graph   compute graph
   *  @return  SUCCESS or FAILED
   */
  Status OptimizeFusedGraph(ge::ComputeGraph& graph) override;

  /*
 *  @ingroup fe
 *  @brief   optimize fused graph for LXfusion
 *  @param   [in|out] graph   compute graph
 *  @return  SUCCESS or FAILED
 */
  Status OptimizeFusedGraphAfterGraphSlice(ge::ComputeGraph& graph) override;

  /*
   *  @ingroup fe
   *  @brief   optimize stream graph
   *  @param   [in|out] graph   compute graph
   *  @param   [in] context run_context
   *  @return  SUCCESS or FAILED
  */
  Status OptimizeStreamGraph(ge::ComputeGraph& stream_graph, const ge::RunContext& context) override;


  Status OptimizeStreamedWholeGraph(ge::ComputeGraph &graph) override;

  /*
   *  @ingroup fe
   *  @brief   get attribute of graph optimizer
   *  @param   [in|out] attrs
   *  @return  SUCCESS or FAILED
   */
  Status GetAttributes(ge::GraphOptimizerAttribute& attrs) const override;

  Status OptimizeWholeGraph(ge::ComputeGraph& graph) override;

  Status OptimizeGraphPrepare(ge::ComputeGraph& graph) override;

  Status OptimizeOriginalGraphJudgeInsert(ge::ComputeGraph& graph) override;

  Status OptimizeAfterStage1(ge::ComputeGraph &graph) override;

  Status ShapeAndValueGeneralize(ge::ComputeGraph &graph) const;

 private:
  Status RefreshParameters();

  void RefreshSmallChannelConfig() const;

  void RefreshLicenseParameters();
  /**
   *  @ingroup fe
   *  @brief set atomic info for op
   *  @param [in|out] graph compute graph
   *  @return SUCCESS or FAILED
   */
  Status SetAtomicAddInfo(const ge::ComputeGraph& graph) const;

  Status SetTbeOpAtomicAttr(ge::OpDescPtr op_desc) const;

  LxFusionOptimizeResult GenerateLxFusionOptimizeResult(const Status &l1_buffer_ret, const Status &l2_buffer_ret) const;

  Status BufferFusionProcess(ge::ComputeGraph& graph, GraphCommPtr graphCommPtr, LxFusionOptimizeResult& buffer_ret);

  Status BufferFusionRecovery(ge::ComputeGraph& graph, std::vector<ge::NodePtr>& buff_fus_compile_failed_nodes);

  /*
   *  insert Compress op between conv and weight
   *  @ingroup fe
   *  @brief   insert Compress op between conv and weight
   *  @param   [in]  graph        compute graph
   *  @return  SUCCESS or FAILED
   */
  Status InsertCompressOP(ge::ComputeGraph& graph);

  Status CreateCompressOp(ge::OpDescPtr conv_desc, ge::OpDescPtr& compress_opdesc) const;

  Status OptimizeOriginalGraphOpJudgeAndFormatDtypeSetter(ge::ComputeGraph& graph) const;

  Status ConcatSplitOptimizer(ge::ComputeGraph& graph, const bool& need_set_virtual_op) const;

  /* Do the following stuffs:
   * 1. Do statistics of all ub fusion pass.
   * 2. Merge all fused nodes.
   * 3. Calculate tensor size.
   * 4. Set atomic add info. */
  Status PostProcessAfterCompilingOp(ge::ComputeGraph& graph, const BufferFusionPtr& buffer_fusion_ptr,
                                     std::vector<ge::NodePtr> &buff_fus_compile_failed_nodes);

  Status OptimizeFusedCompileOpAndCalcTensorSize(ge::ComputeGraph& graph,
                                                 const BufferFusionPtr& buffer_fusion_ptr);

  Status InsertTransNodesForAllGraph(ge::ComputeGraph& graph, TransNodeManagerPtr &trans_node_mgr_ptr) const;

  Status GraphFusionBeforeTransnodesInsertion(ge::ComputeGraph& graph) const;

  template <typename T>
  Status InitialzeNormalCompiler(string compiler_name);

  template <typename T>
  Status InitialzeOneCompiler(string compiler_name);

  Status InitializeAllOpCompiler();

  Status GetOpCompiler(const std::string& build_mode_value, const std::string& step_mode_value,
                       OpCompilerPtr &op_compiler) const;

  Status BufferFusionMatch(ge::ComputeGraph& graph,
                           const std::shared_ptr<FusionCycleDetector> &cycle_detector,
                           const BufferFusionPtr &buffer_fusion_ptr) const;

  void ClearUnknowShapeAttr(const ge::ComputeGraph& graph) const;
  /*
   *  @ingroup fe
   *  @brief   when finish lx fusion for each sub graph, need clear lx resource.
   *           notice: clear resource must after lx recovery.
   *  @param   sub graph
 */
  void ClearLxFusionResource(const ge::ComputeGraph& graph) const;

  Status CloseRcCache(const ge::ComputeGraph &graph);

  void AddAssignMemAttr(ge::ComputeGraph &graph) const;

  void GetAOEOptionValue(AOEOption &aoe_opt) const;
 private:
  // op info mgr
  FEOpsKernelInfoStorePtr ops_kernel_info_store_ptr_;

  // op adapter mgr
  OpStoreAdapterManagerPtr op_store_adapter_manager_ptr_;

  // op compiler shared pointer
  std::vector<OpCompilerPtr> op_compiler_ptr_;

  OpSetterPtr op_setter_ptr_;

  // op judge pointer
  OpImplyTypeJudgePtr op_impl_type_judge_ptr_;

  // format_dtype_setter
  FormatDtypeSetterPtr format_dtype_setter_ptr_;

  // Space Size calculator pointer
  SpaceSizeCalculatorPtr space_size_calculator_ptr_;

  // l2 buffer optimizer pointer
  L2OptimizerPtr l2_optimize_ptr_;

  // graph fusion ptr
  GraphFusionPtr graph_fusion_ptr_;

  // passes mngr
  PassMgrPtr fusion_pass_mgr_ptr_;

  // rules mngr
  RuleMgrPtr fusion_rule_mgr_ptr_;

  // priority mngr
  FusionPriorityMgrPtr fusion_priority_mgr_ptr_;

  // attribute of graph optimizer
  ge::GraphOptimizerAttribute graph_optimizer_attr_;

  // init flag
  bool init_flag_;
  std::mutex sort_lock_;

  // update op axis.
  OpAxisUpdateDescPtr op_axis_update_desc_ptr_;

  ConcatOptimizerPtr concat_optimizer_ptr_;

  ConcatCToNOptimizerPtr concat_c_to_n_optimizer_ptr_;

  SplitCToNOptimizerPtr split_c_to_n_optimizer_ptr_;

  SplitOptimizerPtr split_optimizer_ptr_;

  BufferFusionInfoCollecterPtr buffer_fusion_info_collecter_ptr_;

  ge::OptimizeUtility *optimize_utility_;
};
}  // namespace fe
#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_FE_GRAPH_OPTIMIZER_H_
