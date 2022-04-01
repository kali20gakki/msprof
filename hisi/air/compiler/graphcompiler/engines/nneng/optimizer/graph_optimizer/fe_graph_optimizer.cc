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
#include "graph_optimizer/fe_graph_optimizer.h"

#include <memory>
#include <mutex>
#include <vector>

#include "common/fe_inner_attr_define.h"
#include "common/fe_utils.h"
#include "common/aicore_util_constants.h"
#include "common/op_info_common.h"
#include "ge/ge_api_types.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/ge_context.h"
#include "graph/tuning_utils.h"
#include "cmo/generate_cmo_type_manager.h"

namespace fe {
const string kStagePrepare = "[GraphOpt][Prepare]";
const string kStageBeforeQuant = "[GraphOpt][BeforeQuant]";
const string kStageOrigin = "[GraphOpt][Origin]";
const string kStageAftFmtSlct = "[GraphOpt][AftFmtSlct]";
const string kStageJudgeInsert = "[GraphOpt][JdgInst]";
const string kStageSetOpSlc = "[SubGraphOpt][SetOpSlc]";
const string kStagePreCompile = "[SubGraphOpt][PreComp]";
const string kStageParseCompRst = "[SubGraphOpt][ParseCompRst]";
const string kStageLx = "[SubGraphOpt][Lx]";
const string kStageCompile = "[SubGraphOpt][Compile]";

FEGraphOptimizer::FEGraphOptimizer(FEOpsKernelInfoStorePtr fe_ops_kernel_info_store_ptr,
                                   OpStoreAdapterManagerPtr op_store_adapter_manager_ptr, std::string engine_name)
    : ops_kernel_info_store_ptr_(fe_ops_kernel_info_store_ptr),
      op_store_adapter_manager_ptr_(op_store_adapter_manager_ptr),
      op_setter_ptr_(nullptr),
      op_impl_type_judge_ptr_(nullptr),
      format_dtype_setter_ptr_(nullptr),
      space_size_calculator_ptr_(nullptr),
      l2_optimize_ptr_(nullptr),
      graph_fusion_ptr_(nullptr),
      fusion_pass_mgr_ptr_(nullptr),
      fusion_rule_mgr_ptr_(nullptr),
      fusion_priority_mgr_ptr_(nullptr),
      graph_optimizer_attr_({engine_name, ge::ENGINE}),
      init_flag_(false) {}

FEGraphOptimizer::~FEGraphOptimizer() {}

template <typename T>
Status FEGraphOptimizer::InitialzeOneCompiler(string compiler_name) {
  std::shared_ptr<T> op_compiler;
  FE_MAKE_SHARED(
      op_compiler = std::make_shared<T>(compiler_name, graph_optimizer_attr_.engineName, op_store_adapter_manager_ptr_),
      return GRAPH_OPTIMIZER_MAKE_SHARED_FAILED);

  Status ret = op_compiler->Initialize();
  if (ret != SUCCESS) {
    FE_LOGE("[GraphOpt][InitOneComp] Failed to Initialize %s.", compiler_name.c_str());
    return FAILED;
  }

  op_compiler_ptr_.emplace_back(op_compiler);
  return SUCCESS;
}

template <typename T>
Status FEGraphOptimizer::InitialzeNormalCompiler(string compiler_name) {
  std::shared_ptr<T> op_compiler;
  FE_MAKE_SHARED(
      op_compiler = std::make_shared<T>(compiler_name, graph_optimizer_attr_.engineName, op_store_adapter_manager_ptr_),
      return GRAPH_OPTIMIZER_MAKE_SHARED_FAILED);
  BufferFusionFunc func = std::bind(&FEGraphOptimizer::BufferFusionProcess, this, std::placeholders::_1,
                                    std::placeholders::_2, std::placeholders::_3);

  Status ret = op_compiler->Initialize(func);

  if (ret != SUCCESS) {
    FE_LOGE("[GraphOpt][InitNormalComp] Failed to Initialize %s.", compiler_name.c_str());
    return FAILED;
  }

  op_compiler_ptr_.emplace_back(op_compiler);
  return SUCCESS;
}

Status FEGraphOptimizer::InitializeAllOpCompiler() {
  if (InitialzeOneCompiler<OpCompiler>("Op Compiler") != SUCCESS) {
    return FAILED;
  }

  if (InitialzeNormalCompiler<OpCompilerBaseline>("Baseline Op Compiler") != SUCCESS) {
    return FAILED;
  }

  if (InitialzeNormalCompiler<OpCompilerNormal>("Normal mode Op Compiler") != SUCCESS) {
    return FAILED;
  }

  if (InitialzeNormalCompiler<OpCompilerOpTune>("Op-Tune Op Compiler") != SUCCESS) {
    return FAILED;
  }

  if (InitialzeOneCompiler<OpCompilerMstuneBeforeUbMatch>("Before Ub Match Compiler") != SUCCESS) {
    return FAILED;
  }
  return SUCCESS;
}

Status FEGraphOptimizer::Initialize(const std::map<string, string>& options,
                                    ge::OptimizeUtility *const optimize_utility) {
  // if graph optimizer has been initialized, return success
  if (init_flag_) {
    FE_LOGW("FEGraphOptimizer has been initialized.");
    return SUCCESS;
  }

  init_flag_ = true;
  optimize_utility_ = optimize_utility;
  FE_LOGD("Begin to init FEGraphOptimizer in engine[%s]", graph_optimizer_attr_.engineName.c_str());
  // initialize op compiler
  FE_CHECK(ops_kernel_info_store_ptr_ == nullptr, FE_LOGE("[GraphOpt][Init] opsKernelInfoStorePtr_ is null."),
           return FAILED);
  FE_CHECK(op_store_adapter_manager_ptr_ == nullptr, FE_LOGE("[GraphOpt][Init] opStoreAdapterManagerPtr_ is null."),
           return FAILED);
  ops_kernel_info_store_ptr_->SetOptimizeUtility(optimize_utility);
  if (InitializeAllOpCompiler() != SUCCESS) {
    return FAILED;
  }

  FE_MAKE_SHARED(op_impl_type_judge_ptr_ =
                     std::make_shared<OpImplTypeJudge>(graph_optimizer_attr_.engineName, ops_kernel_info_store_ptr_),
                 return GRAPH_OPTIMIZER_MAKE_SHARED_FAILED);
  FE_MAKE_SHARED(op_setter_ptr_ = std::make_shared<OpSetter>(graph_optimizer_attr_.engineName,
                                                             op_store_adapter_manager_ptr_),
                 return GRAPH_OPTIMIZER_MAKE_SHARED_FAILED);
  FE_MAKE_SHARED(format_dtype_setter_ptr_ = std::make_shared<FormatDtypeSetter>(graph_optimizer_attr_.engineName,
                                                                                op_store_adapter_manager_ptr_),
                 return GRAPH_OPTIMIZER_MAKE_SHARED_FAILED);
  FE_MAKE_SHARED(space_size_calculator_ptr_ = std::make_shared<SpaceSizeCalculator>(),
                 return GRAPH_OPTIMIZER_MAKE_SHARED_FAILED);

  FE_MAKE_SHARED(op_axis_update_desc_ptr_ = std::make_shared<OpAxisUpdateDesc>(graph_optimizer_attr_.engineName),
                 return GRAPH_OPTIMIZER_MAKE_SHARED_FAILED);
  FE_MAKE_SHARED(l2_optimize_ptr_ = std::make_shared<L2Optimizer>(graph_optimizer_attr_.engineName),
                 return GRAPH_OPTIMIZER_MAKE_SHARED_FAILED);

  // init pass mgr ptr
  FE_MAKE_SHARED(fusion_pass_mgr_ptr_ = std::make_shared<FusionPassManager>(),
                 return GRAPH_OPTIMIZER_MAKE_SHARED_FAILED);
  if (fusion_pass_mgr_ptr_->Initialize(graph_optimizer_attr_.engineName) != SUCCESS) {
    FE_LOGE("[GraphOpt][Init] PassMngr initialize Fail.");
    return FAILED;
  }

  // init rule mgr ptr
  FE_MAKE_SHARED(fusion_rule_mgr_ptr_ = std::make_shared<FusionRuleManager>(ops_kernel_info_store_ptr_),
                 return GRAPH_OPTIMIZER_MAKE_SHARED_FAILED);
  if (fusion_rule_mgr_ptr_->Initialize(graph_optimizer_attr_.engineName) != SUCCESS) {
    FE_LOGE("[GraphOpt][Init] RuleMngr initialize Fail.");
    return FAILED;
  }

  // init priority mgr ptr
  FE_MAKE_SHARED(fusion_priority_mgr_ptr_ = std::make_shared<FusionPriorityManager>(
                     graph_optimizer_attr_.engineName, fusion_pass_mgr_ptr_, fusion_rule_mgr_ptr_),
                 return GRAPH_OPTIMIZER_MAKE_SHARED_FAILED);
  if (fusion_priority_mgr_ptr_->Initialize() != SUCCESS) {
    FE_LOGE("[GraphOpt][Init] FusionPriorityMgr initialize Fail.");
    return FAILED;
  }

  // init graph fusion ptr
  FE_MAKE_SHARED(graph_fusion_ptr_ = std::make_shared<GraphFusion>(fusion_rule_mgr_ptr_, ops_kernel_info_store_ptr_,
                                                                   fusion_pass_mgr_ptr_, fusion_priority_mgr_ptr_),
                 return GRAPH_OPTIMIZER_MAKE_SHARED_FAILED);
  graph_fusion_ptr_->SetEngineName(graph_optimizer_attr_.engineName);

  // clear history fusion info file
  FusionStatisticWriter::Instance().ClearHistoryFile();

  FE_LOGI("Initialize success.");

  return SUCCESS;
}

Status FEGraphOptimizer::Finalize() {
  if (!init_flag_) {
    FE_LOGW("FEGraphOptimizer finalize is not allowed, initialize first is necessary.");
    return SUCCESS;
  }

  Status ret1 = SUCCESS;
  for (auto& compiler : op_compiler_ptr_) {
    if (compiler->Finalize() != SUCCESS) {
      FE_LOGE("[GraphOpt][Finalize] Failed to finalize %s.", compiler->GetCompilerName().c_str());
      ret1 = FAILED;
    }
  }

  Status ret2 = SUCCESS;
  if (fusion_pass_mgr_ptr_ != nullptr) {
    ret2 = fusion_pass_mgr_ptr_->Finalize();
    FE_LOGE_IF(ret2 != SUCCESS, "Pass Manager Finalize failed.");
  }

  Status ret3 = SUCCESS;
  if (fusion_rule_mgr_ptr_ != nullptr) {
    ret3 = fusion_rule_mgr_ptr_->Finalize();
    FE_LOGE_IF(ret3 != SUCCESS, "Rule Manager Finalize failed.");
  }

  if ((ret1 != SUCCESS) || (ret2 != SUCCESS) || (ret3 != SUCCESS)) {
    FE_LOGW("FE graph optimizer finalize not success!");
    return FAILED;
  }
  FusionStatisticWriter::Instance().Finalize();
  GenerateCMOTypeManager::Instance().Finalize();
  init_flag_ = false;
  FE_LOGD("Finalized success.");

  return SUCCESS;
}

void FEGraphOptimizer::RefreshLicenseParameters() {
  std::string license_fusion_cache = Configuration::Instance(graph_optimizer_attr_.engineName).GetLicenseFusionStr();
  std::string license_fusion_val;
  ge::graphStatus status = ge::GetContext().GetOption("opt_module.fe", license_fusion_val);
  FE_LOGD("RefreshLicenseParameters key:[%s] val:[%s], license_fusion_cache:[%s], ret:%d",
          "opt_module.fe", license_fusion_val.c_str(), license_fusion_cache.c_str(), status);
  if (status != ge::GRAPH_SUCCESS) {
    license_fusion_val = "ALL";
    Configuration::Instance(graph_optimizer_attr_.engineName).InitLicenseFusion(license_fusion_val);
  } else if (license_fusion_val != license_fusion_cache) {
    Configuration::Instance(graph_optimizer_attr_.engineName).InitLicenseFusion(license_fusion_val);
  }
  return;
}

void FEGraphOptimizer::RefreshSmallChannelConfig() const {
  std::string enable_small_channel = "0";
  ge::graphStatus status = ge::GetContext().GetOption(ge::ENABLE_SMALL_CHANNEL, enable_small_channel);
  FE_LOGD("RefreshSmallChannelConfig key:[%s] val:[%s], status:%d",
          ge::ENABLE_SMALL_CHANNEL.c_str(), enable_small_channel.c_str(), status);
  if (status == ge::GRAPH_SUCCESS) {
    Configuration::Instance(graph_optimizer_attr_.engineName).InitSmallChannel(enable_small_channel);
  }
  return;
}

Status FEGraphOptimizer::RefreshParameters() {

  RefreshLicenseParameters();
  RefreshSmallChannelConfig();
  // when precison_mode is changed, refresh it.
  std::string precision_mode_cache = Configuration::Instance(graph_optimizer_attr_.engineName).GetPrecisionModeStr();
  std::string precision_mode;
  ge::graphStatus status = ge::GetContext().GetOption(ge::PRECISION_MODE, precision_mode);
  if (status == ge::GRAPH_SUCCESS && precision_mode != precision_mode_cache) {
    if (Configuration::Instance(graph_optimizer_attr_.engineName).InitPrecisionMode() != SUCCESS) {
      return FAILED;
    }
  }

  if (Configuration::Instance(graph_optimizer_attr_.engineName).InitBufferOptimize() != SUCCESS) {
    return FAILED;
  }
  return SUCCESS;
}

Status FEGraphOptimizer::OptimizeOriginalGraph(ge::ComputeGraph& graph) {
  if (!init_flag_) {
    REPORT_FE_ERROR("[GraphOpt][init] FEGraphOptimizer has not been initialized.");
    return FAILED;
  }

  if (RefreshParameters() != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][RefreshParam] Failed to init parameter for graph %s.", graph.GetName().c_str());
    return FAILED;
  }
  ops_kernel_info_store_ptr_->SetCheckSupportedStaticFlag(false);
  FE_TIMECOST_START(OptimizeOriginalGraph);
  FE_LOGD("Begin to optimize original graph[%s], in engine[%s], node_size:%zu", graph.GetName().c_str(),
          graph_optimizer_attr_.engineName.c_str(), graph.GetAllNodesSize());

  FE_TIMECOST_START(BeforeQuantFusion);
  Status ret = graph_fusion_ptr_->RunGraphFusionPassByType(kStageBeforeQuant, graph,
                                                           BUILT_IN_BEFORE_QUANT_OPTIMIZATION_GRAPH_PASS);
  if (ret != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][BeforeQuant] Failed to run before-quant-opt-graph-fusion for graph[%s].",
                    graph.GetName().c_str());
    return ret;
  }
  FE_TIMECOST_END(BeforeQuantFusion, "FEGraphOptimizer::BeforeQuantFusion");

  FE_TIMECOST_START(OptimizeQuantGraph);
  ret = graph_fusion_ptr_->FusionQuantOp(graph);
  if (ret != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][OptQuant] Quant optimize failed, graph[%s]", graph.GetName().c_str());
    return ret;
  }

  ret = graph.TopologicalSorting();
  if (ret != ge::GRAPH_SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][BeforeFusion]Failed to do topological sorting before graph fusion for graph %s",
                    graph.GetName().c_str());
    return FAILED;
  }
  FeGraphUtils::DumpGraphAndOnnx(graph, "OptimizeQuantGraph_FeGraphFusionAfter");

  FE_LOGI("Quant optimize success, graph[%s]", graph.GetName().c_str());
  FE_TIMECOST_END(OptimizeQuantGraph, "FEGraphOptimizer::OptimizeQuantGraph");

  ret = ops_kernel_info_store_ptr_->SetDynamicCustomOpStoreInfo(graph);
  if (ret != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][BeforeFusion]Failed to set dynamic custom op store info for graph %s. ErrNo is %u.",
                    graph.GetName().c_str(), ret);
    return ret;
  }
  FE_LOGI("Set dynamic custom op store info success.graph[%s]", graph.GetName().c_str());

  FE_TIMECOST_START(FusionGraph);
  ret = graph_fusion_ptr_->Fusion(graph);

  if (ret != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][AfterFusion]Failed to do graph fusion for graph %s. ErrNo is %u.",
                    graph.GetName().c_str(), ret);
    return ret;
  }

  FeGraphUtils::DumpGraphAndOnnx(graph, "OptimizeOriginalGraph_FeGraphFusionAfter");
  FeGraphUtils::DumpSubGraphAndOnnx(graph, "OptimizeOriginalGraph_FeGraphFusionAfter_Subgraph");
  FE_TIMECOST_END(FusionGraph, "GraphFusion::Fusion during FEGraphOptimizer::OptimizeOriginalGraph");

  ret = graph.TopologicalSorting();

  if (ret != ge::GRAPH_SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][AfterFusion]Failed to do topological sorting after graph fusion for graph %s.",
                    graph.GetName().c_str());
    return FAILED;
  }

  FeGraphUtils::DumpGraphAndOnnx(graph, "OptimizeOriginalGraph_FeTopoSortingAfter");
  FeGraphUtils::DumpSubGraphAndOnnx(graph, "OptimizeOriginalGraph_FeTopoSortingAfter_Subgraph");

  AddAssignMemAttr(graph);
  FE_LOGI("Optimize original graph[%s] success, node_size:%zu.", graph.GetName().c_str(), graph.GetAllNodesSize());
  FE_TIMECOST_END(OptimizeOriginalGraph, "FEGraphOptimizer::OptimizeOriginalGraph");
  ops_kernel_info_store_ptr_->SetCheckSupportedStaticFlag(true);
  return SUCCESS;
}

Status FEGraphOptimizer::OptimizeOriginalGraphOpJudgeAndFormatDtypeSetter(ge::ComputeGraph& graph) const {
  Status ret;
  // set the highest prior imply type for op
  ret = op_impl_type_judge_ptr_->Judge(graph);
  if (ret != SUCCESS) {
    REPORT_FE_ERROR("[GraphOptJdgInst][Judge] Judge the op implemantation failed, graph[%s].", graph.GetName().c_str());
    return ret;
  }
  FE_LOGI("Optimizing original graph[%s] judge op implemantation success.", graph.GetName().c_str());

  ret = format_dtype_setter_ptr_->SetSupportFormatDtype(graph);
  if (ret != SUCCESS) {
    REPORT_FE_ERROR(
        "[GraphOptJdgInst][SetSupportFormat] Set the support format and dtype information failed, graph[%s].",
        graph.GetName().c_str());
    return ret;
  }
  FE_LOGI("Optimizing original graph[%s] set the support format and dtype information success.",
          graph.GetName().c_str());
  return SUCCESS;
}

Status FEGraphOptimizer::InsertTransNodesForAllGraph(ge::ComputeGraph& graph, TransNodeManagerPtr& trans_node_mgr_ptr)
    const {
  Status ret;
  // insert format and data type transfer op
  FE_MAKE_SHARED(trans_node_mgr_ptr = std::make_shared<TransNodeManager>(ops_kernel_info_store_ptr_),
                 return GRAPH_OPTIMIZER_MAKE_SHARED_FAILED);
  if (trans_node_mgr_ptr->Initialize() != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][Trans][Init] Failed to init transNodeMgrPtr for graph %s.", graph.GetName().c_str());
    return FAILED;
  }

  ret = trans_node_mgr_ptr->InsertAndMergeTransNodes(graph);
  if (ret != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][Trans][Insert] Failed to insert format and dtype transfer op for graph %s.",
                    graph.GetName().c_str());
    return ret;
  }

  FeGraphUtils::DumpGraphAndOnnx(graph, "OptimizeOriginalGraph_FeInsertTransNodeAfter");

  FE_LOGI("Insert format and dtype transfer op to original graph success. graph[%s]", graph.GetName().c_str());

  for (auto& subgraph : graph.GetAllSubgraphs()) {
    ret = trans_node_mgr_ptr->InsertAndMergeTransNodes(*(subgraph.get()));
    if (ret != SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][Trans][Insert] Failed to insert format and dtype transfer op for subgraph %s.",
                      subgraph->GetName().c_str());
      return ret;
    }
    FeGraphUtils::DumpGraphAndOnnx(*(subgraph.get()), "OptimizeOriginalGraph_FeInsertTransNodeAfter_Subgraph");
    FE_LOGI("Insert format and dtype transfer op to subgraph success. subgraph[%s]", subgraph->GetName().c_str());
  }
  return SUCCESS;
}

Status FEGraphOptimizer::GraphFusionBeforeTransnodesInsertion(ge::ComputeGraph& graph) const {
  if (graph_fusion_ptr_->SetContinuousDtypeForOutput(graph) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOptJdgInst][GraphFusion][SetContinuousDtype] Failed to set continuous dtype for graph:%s.",
                    graph.GetName().c_str());
    return FAILED;
  }

  for (const auto& sub_graph : graph.GetAllSubgraphs()) {
    if (graph_fusion_ptr_->SetContinuousDtypeForOutput(*sub_graph) != SUCCESS) {
      REPORT_FE_ERROR(
          "[GraphOptJdgInst][GraphFusion][SetContinuousDtype] Failed to set continuous dtype for sub graph:%s.",
          sub_graph->GetName().c_str());
      return FAILED;
    }
  }

  Status ret = graph_fusion_ptr_->RunGraphFusionPassByType(kStageJudgeInsert, graph,
                                                           BUILT_IN_BEFORE_TRANSNODE_INSERTION_GRAPH_PASS);
  if (ret != SUCCESS) {
    REPORT_FE_ERROR(
        "[GraphOptJdgInst][GraphFusion][Run] Failed to run graph fusion for graph[%s] \
                    before trans-nodes insertion.",
        graph.GetName().c_str());
    return ret;
  }
  return SUCCESS;
}

Status FEGraphOptimizer::OptimizeOriginalGraphJudgeInsert(ge::ComputeGraph& graph) {
  FE_TIMECOST_START(OriginalGraphJudgeInsert);
  OpFormatDtypeJudgePtr op_format_dtype_judge_ptr = nullptr;
  ReflectionBuilderPtr reflection_builder_ptr = nullptr;
  HeavyFormatPropagationPtr heavy_format_propagation_ptr = nullptr;

  FE_MAKE_SHARED(reflection_builder_ptr = std::make_shared<ge::RefRelations>(),
                 return GRAPH_OPTIMIZER_MAKE_SHARED_FAILED);
  ops_kernel_info_store_ptr_->SetCheckSupportedStaticFlag(false);
  FE_LOGD("Begin to judge insert graph[%s], in engine[%s]", graph.GetName().c_str(),
          graph_optimizer_attr_.engineName.c_str());

  FE_MAKE_SHARED(op_format_dtype_judge_ptr = std::make_shared<OpFormatDtypeJudge>(
                     graph_optimizer_attr_.engineName, op_store_adapter_manager_ptr_, reflection_builder_ptr),
                 return GRAPH_OPTIMIZER_MAKE_SHARED_FAILED);
  if (op_format_dtype_judge_ptr->Initialize() != SUCCESS) {
    REPORT_FE_ERROR("[GraphOptJdgInst][Init] Failed to initialize op_format_dtype_judge_ptr for graph[%s].",
                    graph.GetName().c_str());
    return FAILED;
  }

  FE_MAKE_SHARED(heavy_format_propagation_ptr = std::make_shared<HeavyFormatPropagation>(
                     graph_optimizer_attr_.engineName, op_store_adapter_manager_ptr_, reflection_builder_ptr),
                 return GRAPH_OPTIMIZER_MAKE_SHARED_FAILED);
  if (heavy_format_propagation_ptr->Initialize() != SUCCESS) {
    REPORT_FE_ERROR("[GraphOptJdgInst][Init] Failed to initialize heavy_format_propagation_ptr_ for graph[%s].",
                    graph.GetName().c_str());
    return FAILED;
  }

  Status ret = OptimizeOriginalGraphOpJudgeAndFormatDtypeSetter(graph);
  if (ret != SUCCESS) {
    FE_LOGE(
        "[GraphOptJdgInst][JudgeAndFormat] Fail to judge op implemantation or set support format \
            and dtype information for graph[%s]!",
        graph.GetName().c_str());
    return ret;
  }
  // set the format and data type of the input and output desc of op
  FE_TIMECOST_START(BuildRefRelations);
  (void)reflection_builder_ptr->Clear();
  auto status = reflection_builder_ptr->BuildRefRelations(graph);
  if (status != ge::GRAPH_SUCCESS) {
    REPORT_FE_ERROR("[GraphOptJdgInst][BuildRef] Build reflection relations failed for main and subgraph %s.",
                    graph.GetName().c_str());
    return FAILED;
  }
  FE_TIMECOST_END(BuildRefRelations, "BuildRefRelations during FEGraphOptimizer::OptimizeOriginalGraph");
  ret = op_format_dtype_judge_ptr->Judge(graph);
  if (ret != SUCCESS) {
    REPORT_FE_ERROR(
        "[GraphOptJdgInst][Judge] Judge the format and data type of the input and output desc \
                    of op failed, graph [%s].",
        graph.GetName().c_str());
    FeGraphUtils::DumpGraphAndOnnx(graph, "OptimizeOriginalGraph_FeOpJudgeAfter_Failed");
    FeGraphUtils::DumpSubGraphAndOnnx(graph, "OptimizeOriginalGraph_FeOpJudgeAfter_Failed_Subgraph");
    return ret;
  }
  FeGraphUtils::DumpGraphAndOnnx(graph, "OptimizeOriginalGraph_FeOpJudgeAfter");
  FeGraphUtils::DumpSubGraphAndOnnx(graph, "OptimizeOriginalGraph_FeOpJudgeAfter_Subgraph");
  FE_LOGI("Optimizing original graph[%s] judge the format and data type of the input and output desc of op success.",
          graph.GetName().c_str());

  ret = heavy_format_propagation_ptr->PropagateHeavyFormat(graph);
  if (ret != SUCCESS) {
    FE_LOGE("[GraphOptJdgInst][Propagate] Distribute heavy format failed, graph [%s].", graph.GetName().c_str());
    FeGraphUtils::DumpGraphAndOnnx(graph, "OptimizeOriginalGraph_FeDistHeavyFormatAfter_Failed");
    FeGraphUtils::DumpSubGraphAndOnnx(graph, "OptimizeOriginalGraph_FeDistHeavyFormatAfter_Failed_Subgraph");
    return ret;
  }
  FeGraphUtils::DumpGraphAndOnnx(graph, "OptimizeOriginalGraph_FeDistHeavyFormatAfter");
  FeGraphUtils::DumpSubGraphAndOnnx(graph, "OptimizeOriginalGraph_FeDistHeavyFormatAfter_Subgraph");
  FE_LOGI("Optimizing original graph[%s] distribute heavy format success.", graph.GetName().c_str());

  ret = op_axis_update_desc_ptr_->UpdateAxis(graph);
  if (ret != SUCCESS) {
    REPORT_FE_ERROR("[GraphOptJdgInst][UpdateAxis] Update axis info failed, graph [%s].", graph.GetName().c_str());
    return ret;
  }

  FE_LOGI("Update graph[%s] axis info success.", graph.GetName().c_str());

  (void)CloseRcCache(graph);

  ret = GraphFusionBeforeTransnodesInsertion(graph);
  if (ret != SUCCESS) {
    return ret;
  }

  TransNodeManagerPtr trans_node_mgr_ptr;
  ret = InsertTransNodesForAllGraph(graph, trans_node_mgr_ptr);
  if (ret != SUCCESS) {
    return ret;
  }

  ret = graph_fusion_ptr_->SwitchTransDataAndCast(graph, trans_node_mgr_ptr->GetOptimizableCast());
  if (ret != SUCCESS) {
    REPORT_FE_ERROR("[GraphOptJdgInst][Switch] Failed to optimize transdata and cast for graph[%s].",
                    graph.GetName().c_str());
    return ret;
  }

  ret = graph_fusion_ptr_->RunGraphFusionPassByType(kStageJudgeInsert, graph,
                                                    SECOND_ROUND_BUILT_IN_GRAPH_PASS);
  if (ret != SUCCESS) {
    REPORT_FE_ERROR("[GraphOptJdgInst][Run] Failed to run second round fusion for graph[%s].", graph.GetName().c_str());
    return ret;
  }

  // calculate the input and output size of op.
  ret = space_size_calculator_ptr_->CalculateAICoreRunningParams(graph);
  if (ret != SUCCESS) {
    REPORT_FE_ERROR("[GraphOptJdgInst][Calculate] Calculate running parameters failed, graph [%s]",
                    graph.GetName().c_str());
    return ret;
  }

  ClearUnknowShapeAttr(graph);
  // set the op information
  ret = op_setter_ptr_->SetOpInfo(graph);
  if (ret != SUCCESS) {
    return ret;
  }
  FE_LOGI("Optimizing original graph[%s] set op information success.", graph.GetName().c_str());

  FE_LOGI("Optimize original judge the format and the data type of op and insert trans op success. graph:%s, node_size:\
          %zu", graph.GetName().c_str(), graph.GetAllNodesSize());
  FE_TIMECOST_END(OriginalGraphJudgeInsert, "FEGraphOptimizer::OriginalGraphJudgeInsert");
  ops_kernel_info_store_ptr_->SetCheckSupportedStaticFlag(true);
  return SUCCESS;
}

Status FEGraphOptimizer::OptimizeAfterStage1(ge::ComputeGraph &graph) {
  FE_LOGD("Begin to optimize graph[%s] after stage1.", graph.GetName().c_str());
  if (Configuration::Instance(graph_optimizer_attr_.engineName).GetSocVersion() != SOC_VERSION_ASCEND310) {
    FE_LOGD("Optimizing after stage1 only takes effect for mini.");
    return SUCCESS;
  }
  std::vector<ge::NodePtr> cast_node_vec;
  for (ge::NodePtr &node : graph.GetDirectNode()) {
    if (node->GetType() == CAST) {
      cast_node_vec.push_back(node);
    }
  }

  FE_CHECK_NOTNULL(graph_fusion_ptr_);
  Status ret = graph_fusion_ptr_->SwitchTransDataAndCast(graph, cast_node_vec);
  if (ret != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][AfterStage1] Fail to optimize graph[%s] after stage1.", graph.GetName().c_str());
    return ret;
  }
  return SUCCESS;
}

Status FEGraphOptimizer::ShapeAndValueGeneralize(ge::ComputeGraph &graph) const {
  if (!IsFuzzBuild()) {
    FE_LOGD("[GraphOpt][Prepare][Generalize]No Need to generalize the current graph.");
    return SUCCESS;
  }
  FE_LOGI("[GraphOpt][Prepare][Generalize]Begin to generalize graph.");
  (void)graph.TopologicalSorting();

  FuzzyGeneralizePtr fuzzy_generalize_ptr = nullptr;
  FE_MAKE_SHARED(fuzzy_generalize_ptr = std::make_shared<FuzzyGeneralize>(
    optimize_utility_, ops_kernel_info_store_ptr_, op_store_adapter_manager_ptr_),
    return GRAPH_OPTIMIZER_MAKE_SHARED_FAILED);
  return fuzzy_generalize_ptr->GeneralizeGraph(graph);
}

void FEGraphOptimizer::AddAssignMemAttr(ge::ComputeGraph &graph) const {
  if (Configuration::Instance(graph_optimizer_attr_.engineName).CheckSupportCMO()) {
    ge::AttrUtils::SetBool(graph, ge::ATTR_NAME_MEM_RELEASE_FIRST_REUSE_FIRST, true);
    FE_LOGI("[GraphOpt][Optimize] platform support cmo, assign mem FirstReleaseFirstReuse.");
  }
}

Status FEGraphOptimizer::OptimizeGraphPrepare(ge::ComputeGraph& graph) {
  if (!init_flag_) {
    REPORT_FE_ERROR("[GraphOpt][Prepare] FEGraphOptimizer has not been initialized.");
    return FAILED;
  }

  if (ShapeAndValueGeneralize(graph) != SUCCESS) {
    FE_LOGW("[GraphOpt][Prepare] Unexpected exceptions happened when generalizing the graph, \
        so the graph has not been generalized, please check the detail log.");
  }

  FE_TIMECOST_START(OptimizeTagNoConstFoldingGraph);
  {
    std::lock_guard <std::mutex> lock_guard(sort_lock_);
    RefreshLicenseParameters();
    if (fusion_priority_mgr_ptr_->SortGraphFusion() != SUCCESS) {
      FE_LOGE("[GraphOpt][SortGraphFusion] Failed to sort graph fusion by priority.");
      return FAILED;
    }
    if (fusion_priority_mgr_ptr_->SortBufferFusion() != SUCCESS) {
      FE_LOGE("[GraphOpt][SortBufferFusion] Failed to sort buffer fusion by priority.");
      return FAILED;
    }
  }

  Status ret = graph_fusion_ptr_->TagNoConstFolding(graph);
  if (ret != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][Prepare] Failed to tag no const folding for graph %s", graph.GetName().c_str());
    return ret;
  }

  ret = graph_fusion_ptr_->RunGraphFusionPassByType(kStagePrepare, graph,
                                                    BUILT_IN_PREPARE_GRAPH_PASS);
  if (ret != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][Prepare] Failed to run prepare-graph-fusion for graph[%s].", graph.GetName().c_str());
    return ret;
  }

  FE_TIMECOST_END(OptimizeTagNoConstFoldingGraph, "FEGraphOptimizer::OptimizeTagNoConstFoldingGraph");
  FeGraphUtils::DumpGraphAndOnnx(graph, "OptimizeGraph_TagNoConstFoldingAfter");
  FeGraphUtils::DumpSubGraphAndOnnx(graph, "OptimizeGraph_TagNoConstFoldingAfter_Subgraph");
  FE_LOGI("Optimize tag no const folding graph[%s] success.", graph.GetName().c_str());
  return SUCCESS;
}

Status FEGraphOptimizer::SetAtomicAddInfo(const ge::ComputeGraph& graph) const {
  ge::ComputeGraph::Vistor<ge::NodePtr> nodes = graph.GetDirectNode();
  for (auto node : nodes) {
    ge::OpDescPtr op_desc = node->GetOpDesc();
    FE_CHECK_NOTNULL(op_desc);
    uint32_t imply_value = 0;
    (void)ge::AttrUtils::GetInt(op_desc, FE_IMPLY_TYPE, imply_value);
    OpImplType op_impl_type = static_cast<OpImplType>(imply_value);
    // now deal with TBE op
    bool is_tbe_op = op_impl_type == EN_IMPL_HW_TBE || op_impl_type == EN_IMPL_CUSTOM_TBE ||
                     op_impl_type == EN_IMPL_NON_PERSISTENT_CUSTOM_TBE || op_impl_type == EN_IMPL_PLUGIN_TBE ||
                     op_impl_type == EN_IMPL_VECTOR_CORE_HW_TBE || op_impl_type == EN_IMPL_VECTOR_CORE_CUSTOM_TBE;
    if (is_tbe_op) {
      if (SetTbeOpAtomicAttr(op_desc) != fe::SUCCESS) {
        FE_LOGW("Set tbe op[%s] atomic info not success.", op_desc->GetName().c_str());
        return FAILED;
      }
    }
  }
  return fe::SUCCESS;
}

Status FEGraphOptimizer::SetTbeOpAtomicAttr(ge::OpDescPtr op_desc) const {
  std::vector<uint32_t> output_index;
  std::map<string, std::map<int64_t, int64_t>> sub_node_workspace_info;
  // only process when get output_index success
  std::vector<uint32_t> tmp_output_index;
  if (ge::AttrUtils::GetListInt(op_desc, TBE_OP_ATOMIC_OUTPUT_INDEX, tmp_output_index)) {
    uint32_t output_size = tmp_output_index.size();
    for (uint32_t i = 0; i < output_size; i++) {
      if (tmp_output_index[i] == 1) {
        output_index.push_back(i);
      }
    }
    if (!output_index.empty()) {
      if (!ge::AttrUtils::SetListInt(op_desc, ge::ATOMIC_ATTR_OUTPUT_INDEX, output_index)) {
        REPORT_FE_ERROR("[GraphOpt][SetAttr][SetListInt] Set op [%s] output atomic info to op_desc failed!",
                        op_desc->GetName().c_str());
        return fe::FAILED;
      }
    }
    FE_LOGD("finish set tbe op [%s] output_index atomic info.", op_desc->GetName().c_str());
  }
  // process with workspace info
  std::vector<int64_t> tmp_workspace_index;
  std::vector<int64_t> workspace_index;
  if (ge::AttrUtils::GetListInt(op_desc, TBE_OP_ATOMIC_WORKSPACE_INDEX, tmp_workspace_index)) {
    size_t workspace_size = tmp_workspace_index.size();
    for (size_t i = 0; i < workspace_size; i++) {
      if (tmp_workspace_index[i] == 1) {
        workspace_index.push_back(i);
      }
    }
    std::map<int64_t, int64_t> workspace_info;
    std::vector<int64_t> workspace_bytes_vec = op_desc->GetWorkspaceBytes();
    if (!workspace_index.empty()) {
      for (int64_t index : workspace_index) {
        if (index >= static_cast<int64_t>(workspace_bytes_vec.size())) {
          continue;
        }
        workspace_info.insert(std::make_pair(index, workspace_bytes_vec[index]));
      }
      sub_node_workspace_info.insert(std::make_pair(op_desc->GetName(), workspace_info));
      if (!op_desc->SetExtAttr(ge::EXT_ATTR_ATOMIC_WORKSPACE_INFO, sub_node_workspace_info)) {
        REPORT_FE_ERROR("[GraphOpt][SetAttr][SetExtAttr] Set op [%s] workspace atomic info failed!",
                        op_desc->GetName().c_str());
        return fe::FAILED;
      }
      FE_LOGD("finish set tbe op [%s] workspace atomic info", op_desc->GetName().c_str());
    } else {
      FE_LOGD("Op [%s] has no workspace atomic info.", op_desc->GetName().c_str());
    }
  }
  return fe::SUCCESS;
}

Status FEGraphOptimizer::InsertCompressOP(ge::ComputeGraph& graph) {
  std::string build_mode = Configuration::Instance(graph_optimizer_attr_.engineName).GetBuildMode();
  std::string build_step = Configuration::Instance(graph_optimizer_attr_.engineName).GetBuildStep();
  bool is_copy_weight = build_mode == ge::BUILD_MODE_TUNING && build_step == ge::BUILD_STEP_BEFORE_UB_MATCH;
  for (ge::NodePtr node : graph.GetDirectNode()) {
    if (node->GetType() != CONV2D_COMPRESS && node->GetType() != FC_COMPRESS && node->GetType() != MATMULV2_COMPRESS) {
      continue;
    }
    auto compress_index_in_anchor = node->GetInDataAnchor(TENSOR_INDEX_COMPRESS_INDEX);
    FE_CHECK_NOTNULL(compress_index_in_anchor);
    if (compress_index_in_anchor->GetPeerOutAnchor() != nullptr) {
      FE_LOGD("The compress index anchor of node[%s, %s] is linked, can not insert compress op node.",
              node->GetType().c_str(), node->GetName().c_str());
      continue;
    }

    ge::OpDescPtr conv_desc = node->GetOpDesc();
    ge::OpDescPtr compress_opdesc = nullptr;
    Status status = CreateCompressOp(conv_desc, compress_opdesc);
    FE_CHECK(status != SUCCESS || compress_opdesc == nullptr,
             FE_LOGE("[SubGraphOpt][CmprsOp][CreatCmprsOp] Fail to create compress op for conv/fc/matmulv2 node[%s].",
                     conv_desc->GetName().c_str()),
             return FAILED);

    ge::NodePtr compress_node = graph.AddNode(compress_opdesc);
    FE_CHECK_NOTNULL(compress_node);
    // link edge for compress node
    auto weight_in_anchor = node->GetInDataAnchor(TENSOR_INDEX_FILTER_COMPRESS);
    FE_CHECK_NOTNULL(weight_in_anchor);
    FE_CHECK_NOTNULL(weight_in_anchor->GetPeerOutAnchor());
    if (is_copy_weight) {
      ge::NodePtr weight_peer_out_node = weight_in_anchor->GetPeerOutAnchor()->GetOwnerNode();
      CopyWeightAttrToPlaceHolder(weight_peer_out_node);
    }

    auto compress_weight_in_anchor = compress_node->GetInDataAnchor(COMPRESSOP_INDEX_WEIGHT_COMPRESS);
    FE_CHECK_NOTNULL(compress_weight_in_anchor);

    if (ge::GraphUtils::AddEdge(weight_in_anchor->GetPeerOutAnchor(), compress_weight_in_anchor) != ge::GRAPH_SUCCESS) {
      REPORT_FE_ERROR("[SubGraphOpt][CmprsOp][AddEdge] Fail to add edge for node[%s]'s weight indata anchor.",
                      compress_node->GetName().c_str());

      return FAILED;
    }
    node->GetInDataAnchor(TENSOR_INDEX_FILTER_COMPRESS)->UnlinkAll();

    auto compress_weight_out_anchor = compress_node->GetOutDataAnchor(COMPRESSOP_INDEX_WEIGHT_COMPRESS);
    FE_CHECK_NOTNULL(compress_weight_out_anchor);
    if (ge::GraphUtils::AddEdge(compress_weight_out_anchor, node->GetInDataAnchor(TENSOR_INDEX_FILTER_COMPRESS)) !=
        ge::GRAPH_SUCCESS) {
      REPORT_FE_ERROR("[SubGraphOpt][CmprsOp][AddEdge] Fail to add edge for conv node[%s]'s weight_compress.",
                      node->GetName().c_str());
      return FAILED;
    }
    if (ge::GraphUtils::AddEdge(compress_node->GetOutDataAnchor(COMPRESSOP_INDEX_COMPRESS_INDEX),
                                node->GetInDataAnchor(TENSOR_INDEX_COMPRESS_INDEX)) != ge::GRAPH_SUCCESS) {
      REPORT_FE_ERROR("[SubGraphOpt][CmprsOp][AddEdge] Fail to add edge for conv node[%s]'s compress_index.",
                      node->GetName().c_str());
      return FAILED;
    }
  }
  return SUCCESS;
}

Status FEGraphOptimizer::CreateCompressOp(ge::OpDescPtr conv_desc, ge::OpDescPtr& compress_opdesc) const {
  std::string compress_type = conv_desc->GetType() == CONV2D_COMPRESS ? COMPRESSOP : COMPRESSFCOP;
  std::string compress_op_name = conv_desc->GetName() + "_" + compress_type;
  FE_MAKE_SHARED(compress_opdesc = std::make_shared<ge::OpDesc>(compress_op_name, compress_type), return FAILED);
  FE_CHECK_NOTNULL(compress_opdesc);
  vector<string> input_name_vec = conv_desc->GetInputName();
  if (input_name_vec.empty()) {
    REPORT_FE_ERROR("[SubGraphOpt][CrtCmprsOp][GetInputName] The input name of node[%s] is empty.",
                    conv_desc->GetName().c_str());
    return FAILED;
  }
  vector<string> new_input_name_vec;
  new_input_name_vec.push_back(input_name_vec[0]);
  new_input_name_vec.push_back(compress_op_name + ":0");
  new_input_name_vec.push_back(compress_op_name + ":1");
  if (input_name_vec.size() > TENSOR_INDEX_COMPRESS_INDEX) {
    for (uint32_t i = TENSOR_INDEX_COMPRESS_INDEX; i < input_name_vec.size(); i++) {
      new_input_name_vec.push_back(input_name_vec[i]);
    }
  }
  conv_desc->SetInputName(new_input_name_vec);

  compress_opdesc->SetOpEngineName(AI_CORE_NAME);
  compress_opdesc->SetOpKernelLibName(AI_CORE_NAME);
  // set fe impl type to TBE, otherwise there would be exception
  // while calculating the tensor size
  if (!ge::AttrUtils::SetInt(compress_opdesc, FE_IMPLY_TYPE, static_cast<int>(EN_IMPL_HW_TBE))) {
    REPORT_FE_ERROR("[SubGraphOpt][CmprsOp][SetInt] Fail to set _fe_imply_type on op[%s]", compress_op_name.c_str());
    return FAILED;
  }

  // add input desc
  compress_opdesc->AddInputDesc("weight", conv_desc->GetInputDesc(TENSOR_INDEX_FILTER_COMPRESS));

  // add output desc
  compress_opdesc->AddOutputDesc("weight_compress", conv_desc->GetInputDesc(TENSOR_INDEX_FILTER_COMPRESS));
  compress_opdesc->AddOutputDesc("compress_index", conv_desc->GetInputDesc(TENSOR_INDEX_COMPRESS_INDEX));

  (void)ge::AttrUtils::SetStr(compress_opdesc, ge::ATTR_NAME_ENGINE_NAME_FOR_LX, AI_CORE_NAME);
  (void)ge::AttrUtils::SetStr(compress_opdesc, ge::ATTR_NAME_KKERNEL_LIB_NAME_FOR_LX, AI_CORE_NAME);
  (void)ge::AttrUtils::SetInt(compress_opdesc, FE_IMPLY_TYPE, static_cast<int>(EN_IMPL_HW_TBE));

  return SUCCESS;
}

Status FEGraphOptimizer::ConcatSplitOptimizer(ge::ComputeGraph& graph,
                                              const bool& need_set_virtual_op) const {
  Status ret = FAILED;
  if (need_set_virtual_op) {
    FE_LOGD("split_optimizer to set FusionVirtualOp");
    ret = split_optimizer_ptr_->SetFusionVirtualOp(graph);
    if (ret != SUCCESS) {
      FE_LOGD("OptimizeOriginalGraphJudgeInsertSplit set fusion_virtual_op failed, graph [%s].",
              graph.GetName().c_str());
      return ret;
    }
    FE_LOGD("concat_optimizer to set FusionVirtualOp");
    ret = concat_optimizer_ptr_->SetFusionVirtualOp(graph);
    if (ret != SUCCESS) {
      FE_LOGD("OptimizeOriginalGraphJudgeInsertConcat set fusion_virtual_op failed, graph [%s].",
              graph.GetName().c_str());
      return ret;
    }
    FE_LOGD("concat_c_to_n_optimizer to set FusionVirtualOp");
    (void)concat_c_to_n_optimizer_ptr_->SetFusionVirtualOp(graph);
    FE_LOGD("split_c_to_n_optimizer to set FusionVirtualOp");
    (void)split_c_to_n_optimizer_ptr_->SetFusionVirtualOp(graph);
  } else {
    FE_LOGD("Fusion virtual op set switch is off for graph %s", graph.GetName().c_str());
  }
  return SUCCESS;
}

Status FEGraphOptimizer::PostProcessAfterCompilingOp(ge::ComputeGraph& graph, const BufferFusionPtr& buffer_fusion_ptr,
                                                     std::vector<ge::NodePtr>& buff_fus_compile_failed_nodes) {
  if (!buff_fus_compile_failed_nodes.empty()) {
    if (BufferFusionRecovery(graph, buff_fus_compile_failed_nodes) != SUCCESS) {
      REPORT_FE_ERROR("[SubGraphOpt][PostProcess][BufferRecovery] Buffer fusion recovery failed, graph[%s].",
                      graph.GetName().c_str());
      return FAILED;
    }
  }

  // count match & effect times
  FE_MAKE_SHARED(buffer_fusion_info_collecter_ptr_ = std::make_shared<BufferFusionInfoCollecter>(), return FAILED);
  buffer_fusion_info_collecter_ptr_->CountBufferFusionTimes(graph);

  FE_TIMECOST_START(FusionGraph);
  // create fused Graph, and merge matched sub-graphs into fusion ops
  if (buffer_fusion_ptr->BuildFusionGraph(graph) != SUCCESS) {
    REPORT_FE_ERROR("[SubGraphOpt][PostProcess][BuildFusion] Build fusion graph failed, graph[%s].",
                    graph.GetName().c_str());
    return FAILED;
  }
  FE_TIMECOST_END(FusionGraph, "BuildFusionGraph during FEGraphOptimizer::OptimizeFusedGraph");

  // calculate the input and output size of op.
  FE_CHECK(space_size_calculator_ptr_ == nullptr, REPORT_FE_ERROR("[GraphOpt][PostProcess][CalcuRunPara] \
           spaceSizeCalculatorPtr_ is null."),
           return FAILED);
  Status ret = space_size_calculator_ptr_->CalculateRunningParams(graph);
  if (ret != SUCCESS) {
    REPORT_FE_ERROR("[SubGraphOpt][PostProcess][CalcuRunPara] Calculate running parameters failed, graph [%s]",
                    graph.GetName().c_str());
    return ret;
  }

  // set atomic info to op
  ret = SetAtomicAddInfo(graph);
  if (ret != SUCCESS) {
    REPORT_FE_ERROR("[SubGraphOpt][PostProcess][SetAtomicAddInfo] Set atomic attribute to nodes failed, graph [%s].",
                    graph.GetName().c_str());
    return ret;
  }

  FE_LOGI("Optimize fused graph success.");
  return SUCCESS;
}

Status FEGraphOptimizer::GetOpCompiler(const std::string& build_mode_value,
                                       const std::string& step_mode_value,
                                       OpCompilerPtr& op_compiler) const {
  auto size = op_compiler_ptr_.size();
  if (size != static_cast<size_t>(OP_COMPILER_BOTTOM)) {
    FE_LOGE("[SubGraphOpt][Compile][GetCompiler] Op compiler's size(%zu) is less than %u", size, OP_COMPILER_BOTTOM);
    return FAILED;
  }

  if (build_mode_value != ge::BUILD_MODE_TUNING) {
    /* Baseline(using l2 buffer) or atc normal mode(using l2 fusion). */
    if (build_mode_value == ge::BUILD_MODE_BASELINE) {
      op_compiler = op_compiler_ptr_[BASELINE];
    } else {
      op_compiler = op_compiler_ptr_[NORMAL];
    }
  } else {
    if (step_mode_value == ge::BUILD_STEP_AFTER_BUILDER || step_mode_value == ge::BUILD_STEP_AFTER_BUILDER_SUB) {
      /* Op-tune scenario. */
      op_compiler = op_compiler_ptr_[OPTUNE];
    } else if (step_mode_value == ge::BUILD_STEP_BEFORE_UB_MATCH) {
      /* Ms-tune stage 2.1: BUILD_STEP_BEFORE_UB_MATCH.
       * Try to compile first and roll back all nodes which cannot be compiled in a fusion scope to single node.
       * Single node can always be compiled successfully.
       * The purpose is to make sure all the fusion scope can be correctly compiled before ms-tuing.
       * */
      op_compiler = op_compiler_ptr_[MSTUNE_BEFORE_UB_MATCH];
    } else if (step_mode_value.empty()) {
      op_compiler = op_compiler_ptr_[BASELINE];
    } else {
      REPORT_FE_ERROR("[SubGraphOpt][Compile][GetCompiler] Step %s is invalid. Build mode is %s.",
                      step_mode_value.c_str(), build_mode_value.c_str());
      return FAILED;
    }
  }
  FE_LOGI("The compiler for mode %s step %s is %s.", build_mode_value.c_str(), step_mode_value.c_str(),
          op_compiler->GetCompilerName().c_str());
  return SUCCESS;
}

Status FEGraphOptimizer::OptimizeFusedCompileOpAndCalcTensorSize(ge::ComputeGraph& graph,
                                                                 const BufferFusionPtr& buffer_fusion_ptr) {
  // compile tbe op
  std::vector<ge::NodePtr> buff_fus_compile_failed_nodes;
  std::vector<ge::NodePtr> buff_fus_rollback_nodes;
  std::vector<ge::NodePtr> buff_fus_to_del_nodes;
  Status ret = op_compiler_ptr_[COMMON]->CompileOp(graph, buff_fus_compile_failed_nodes, buff_fus_rollback_nodes,
      buff_fus_to_del_nodes);
  if (ret != SUCCESS) {
    REPORT_FE_ERROR("[SubGraphOpt][SubGraphOptAfterSlice][Compile] CompileOp failed, graph name = %s.",
                    graph.GetName().c_str());
    return ret;
  }

  FE_LOGD("Optimizing fused graph: compile op success.");
  ret = PostProcessAfterCompilingOp(graph, buffer_fusion_ptr, buff_fus_compile_failed_nodes);
  if (ret != SUCCESS) {
    ClearLxFusionResource(graph);
    return ret;
  }

  ClearLxFusionResource(graph);
  return SUCCESS;
}

Status FEGraphOptimizer::BufferFusionMatch(ge::ComputeGraph& graph,
                                           const std::shared_ptr<FusionCycleDetector> &cycle_detector,
                                           const BufferFusionPtr& buffer_fusion_ptr) const {
  FE_TIMECOST_START(FusionMatch);

  // find sub-graphs that match UB fusion pattern
  if (buffer_fusion_ptr->MatchFusionPatternFromGraph(graph) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][FusionMatch] Fusion pattern match failed, graph[%s].", graph.GetName().c_str());
    return FAILED;
  }
  FE_TIMECOST_END(FusionMatch, "MatchFusionPattern during FEGraphOptimizer::OptimizeFusedGraph");
  std::unique_ptr<ConnectionMatrix> connection_matrix;
  /* Here, because we do not need to use cycle detector anymore,
   * we do not set connection matrix to cycle detector. */
  cycle_detector->GetConnectionMatrix(connection_matrix);
  AutomaticBufferFusionPtr auto_buffer_fusion_ptr;
  FE_MAKE_SHARED(auto_buffer_fusion_ptr =
                 std::make_shared<AutomaticBufferFusion>(std::move(connection_matrix)),
                 return GRAPH_OPTIMIZER_MAKE_SHARED_FAILED);
  if (fusion_priority_mgr_ptr_->GetFusionConfigParserPtr() == nullptr ||
      fusion_priority_mgr_ptr_->GetFusionConfigParserPtr()->GetFusionSwitchByName("AutomaticUbFusion", UB_FUSION)) {
    if (auto_buffer_fusion_ptr->Run(graph) != SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][FusionMatch] Failed to do automatic buffer fusion for graph[%s].",
                      graph.GetName().c_str());
      return FAILED;
    }
  } else {
    FE_LOGI("Automatic buffer fusion is off for graph %s", graph.GetName().c_str());
  }
  return SUCCESS;
}

Status FEGraphOptimizer::OptimizeFusedGraph(ge::ComputeGraph& graph) {
  FE_LOGD("Begin to optimizing fused graph in engine[%s].", graph_optimizer_attr_.engineName.c_str());

  if (!init_flag_) {
    FE_LOGW("OptimizeFusedGraph is not allowed, initialize firstly.");
    return FAILED;
  }
  FE_TIMECOST_START(OptimizeFusedGraph);
  Status ret;

  std::shared_ptr<GraphComm> graph_comm_ptr = nullptr;
  FE_MAKE_SHARED(graph_comm_ptr = std::make_shared<GraphComm>(graph_optimizer_attr_.engineName),
                 return GRAPH_OPTIMIZER_MAKE_SHARED_FAILED);
  if (graph_comm_ptr->Initialize() != SUCCESS) {
    FE_LOGW("GraphComm initialize failed");
    return FAILED;
  }

  /* set the highest prior imply type for op which is inserted between original graph
  optimization and fused graph optimization */
  FE_CHECK(op_impl_type_judge_ptr_ == nullptr, REPORT_FE_ERROR("[GraphOpt][FusedGraph] opImplTypeJudgePtr_ is null."),
           return FAILED);
  ret = op_impl_type_judge_ptr_->JudgeInSubGraph(graph);
  if (ret != SUCCESS) {
    return ret;
  }
  FE_LOGI("Optimizing fused graph:%s judge op success.", graph.GetName().c_str());

  // set the op information
  ret = op_setter_ptr_->SetOpInfo(graph);
  if (ret != SUCCESS) {
    return ret;
  }
  FE_LOGI("Optimizing fused graph:%s set the op information success.", graph.GetName().c_str());

  // Insert compress op
  ret = InsertCompressOP(graph);
  if (ret != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][FusedGraph][InsCmprsOP] Fail to insert compress op for graph[%s].",
                    graph.GetName().c_str());
    return ret;
  }

  // pre compile tbe op
  FE_CHECK(op_compiler_ptr_.empty() || op_compiler_ptr_[COMMON] == nullptr,
           REPORT_FE_ERROR("[GraphOpt][FusedGraph][InsCmprsOP] opCompilerPtr_ is null."), return FAILED);
  ret = op_compiler_ptr_[COMMON]->PreCompileOp(graph);
  if (ret != SUCCESS) {
    return ret;
  }
  FE_LOGI("Optimizing fused graph name = %s: precompile op success.", graph.GetName().c_str());

  std::shared_ptr<FusionCycleDetector> cycle_detector;
  FE_MAKE_SHARED(cycle_detector = std::make_shared<FusionCycleDetector>(), return FAILED);
  cycle_detector->Initialize(graph);

  BufferFusionPtr buffer_fusion_ptr;
  FE_MAKE_SHARED(buffer_fusion_ptr = std::make_shared<BufferFusion>(
                 graph_comm_ptr, fusion_pass_mgr_ptr_, fusion_priority_mgr_ptr_, cycle_detector),
                 return GRAPH_OPTIMIZER_MAKE_SHARED_FAILED);
  buffer_fusion_ptr->SetEngineName(graph_optimizer_attr_.engineName);

  ret = BufferFusionMatch(graph, cycle_detector, buffer_fusion_ptr);
  if (ret != SUCCESS) {
    return ret;
  }

  std::string build_mode_value =
      Configuration::Instance(graph_optimizer_attr_.engineName).GetGeContextOptionValue(ge::BUILD_MODE);
  std::string step_mode_value =
      Configuration::Instance(graph_optimizer_attr_.engineName).GetGeContextOptionValue(ge::BUILD_STEP);
  FE_LOGD("Get build status from option: build_mode [%s], step [%s].", build_mode_value.c_str(),
          step_mode_value.c_str());

  OpCompilerPtr op_compiler = nullptr;
  if (GetOpCompiler(build_mode_value, step_mode_value, op_compiler) != SUCCESS) {
    return FAILED;
  }

  FE_CHECK_NOTNULL(op_compiler);

  std::vector<ge::NodePtr> buff_fus_compile_failed_nodes;
  bool need_post_process = true;
  ret = op_compiler->RunCompileProcess(graph, graph_comm_ptr, buff_fus_compile_failed_nodes,
                                       need_post_process);
  if (ret != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][FusedGraph][RunCompile] Failed to compile graph with compiler %s",
                    op_compiler->GetCompilerName().c_str());
    return ret;
  }

  if (!need_post_process) {
    FE_LOGD("In build mode %s and build step %s, we do not need post process.", build_mode_value.c_str(),
            step_mode_value.c_str());
    return SUCCESS;
  }
  FE_LOGI("Optimizing fused graph: compile op success.");

  if (PostProcessAfterCompilingOp(graph, buffer_fusion_ptr, buff_fus_compile_failed_nodes) != SUCCESS) {
    ClearLxFusionResource(graph);
    return FAILED;
  }

  ClearLxFusionResource(graph);

  FE_TIMECOST_END(OptimizeFusedGraph, "FEGraphOptimizer::OptimizeFusedGraph");
  return SUCCESS;
}

/*
 *  @ingroup fe
 *  @brief   optimize fused graph for LXfusion
 *  @param   [in|out] graph   compute graph
 *  @return  SUCCESS or FAILED
 */
Status FEGraphOptimizer::OptimizeFusedGraphAfterGraphSlice(ge::ComputeGraph& graph) {
  FE_LOGD("Begin to optimizing fused graph for second stage in engine[%s].", graph_optimizer_attr_.engineName.c_str());
  FE_TIMECOST_START(OptimizeFusedGraphAfterGraphSlice);
  if (!init_flag_) {
    FE_LOGW("OptimizeFusedGraphAfterGraphSlice is not allowed, initialize firstly.");
    return FAILED;
  }

  std::string build_mode_value =
      Configuration::Instance(graph_optimizer_attr_.engineName).GetGeContextOptionValue(ge::BUILD_MODE);
  std::string step_mode_value =
      Configuration::Instance(graph_optimizer_attr_.engineName).GetGeContextOptionValue(ge::BUILD_STEP);
  FE_LOGD("Optimizing fused graph second stage. build_mode is [%s] build_step is [%s].", build_mode_value.c_str(),
          step_mode_value.c_str());
  Status ret;
  // Insert compress op
  ret = InsertCompressOP(graph);
  if (ret != SUCCESS) {
    REPORT_FE_ERROR("[SubGraphOpt][CompileAfterSlice][InsCmprss] Fail to insert compress op for graph[%s].",
                    graph.GetName().c_str());
    return ret;
  }

  ret = op_compiler_ptr_[COMMON]->PreCompileOp(graph);
  if (ret != SUCCESS) {
    REPORT_INNER_ERROR(EM_INNER_ERROR,
                       "[SubGraphOpt][CompileAfterSlice][Pre-Comp] Failed to precompile graph %s after slice.",
                       graph.GetName().c_str());
    return ret;
  }
  FE_LOGD("Optimizing fused graph name = %s: precompile op success.", graph.GetName().c_str());

  std::shared_ptr<GraphComm> graph_comm_ptr = nullptr;
  BufferFusionPtr buffer_fusion_ptr;

  FE_MAKE_SHARED(graph_comm_ptr = std::make_shared<GraphComm>(graph_optimizer_attr_.engineName),
                 return GRAPH_OPTIMIZER_MAKE_SHARED_FAILED);
  if (graph_comm_ptr->Initialize() != SUCCESS) {
    FE_LOGW("GraphComm initialize failed");
    return FAILED;
  }

  std::shared_ptr<FusionCycleDetector> cycle_detector;
  FE_MAKE_SHARED(cycle_detector = std::make_shared<FusionCycleDetector>(), return FAILED);
  cycle_detector->Initialize(graph);
  FE_MAKE_SHARED(buffer_fusion_ptr = std::make_shared<BufferFusion>(
                     graph_comm_ptr, fusion_pass_mgr_ptr_, fusion_priority_mgr_ptr_, cycle_detector),
                 return GRAPH_OPTIMIZER_MAKE_SHARED_FAILED);
  buffer_fusion_ptr->SetEngineName(graph_optimizer_attr_.engineName);
  ret = OptimizeFusedCompileOpAndCalcTensorSize(graph, buffer_fusion_ptr);
  if (ret != SUCCESS) {
    REPORT_FE_ERROR(
        "[SubGraphOpt][CompileAfterSlice][FusComp] Failed to do optimize fused graph after UB match \
                    for graph[%s].",
        graph.GetName().c_str());
    return ret;
  }
  FE_TIMECOST_END(OptimizeFusedGraphAfterGraphSlice, "FEGraphOptimizer::OptimizeFusedGraphAfterGraphSlice");
  return SUCCESS;
}

/*
 *  @ingroup fe
 *  @brief   optimize stream graph (now only for single stream graph)
 */
Status FEGraphOptimizer::OptimizeStreamGraph(ge::ComputeGraph& stream_graph, const ge::RunContext& context) {
  FE_LOGI("FEGraphOptimizer start optimizing stream graph.");
  string session_graph_id = "";
  if (ge::AttrUtils::GetStr(stream_graph, ge::ATTR_NAME_SESSION_GRAPH_ID, session_graph_id) &&
      !session_graph_id.empty()) {
    FE_LOGD("stream session_graph_id=%s", session_graph_id.c_str());
  }
  // write fusion info to file
  string context_graph_id = "";
  FeGraphUtils::GetGraphIdFromAttr(stream_graph, context_graph_id);
  FE_LOGD("stream sessionId %lu graph_id %s", context.sessionId, context_graph_id.c_str());

  if (!init_flag_) {
    FE_LOGW("OptimizeStreamGraph is not allowed, initialize first is necessary.");
    return FAILED;
  }

  // if find any unknown shape node, return success
  for (auto& node_ptr : stream_graph.GetDirectNode()) {
    bool unknown_shape = true;
    if (ge::NodeUtils::GetNodeUnknownShapeStatus(*(node_ptr.get()), unknown_shape) == ge::GRAPH_SUCCESS) {
      if (unknown_shape) {
        FE_LOGI("Optimize stream graph get unknown_shape node. name:%s", stream_graph.GetName().c_str());
        return SUCCESS;
      }
    }
  }
  // choose L2 cache or L2 buffer mode, if L2 buffer mode, dynamic batch judge
  FE_CHECK(l2_optimize_ptr_ == nullptr, REPORT_FE_ERROR("[GraphOpt][OptStream] l2OptimizePtr_ is null."),
           return fe::FAILED);
  Status ret = l2_optimize_ptr_->GetL2DataAlloc(stream_graph,
                                                static_cast<uint64_t>(reinterpret_cast<uintptr_t>(context.dataMemBase)),
                                                context.stream);
  if (ret != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][OptStream][GetL2DataAlloc] GetL2DataAlloc failed, graph[%s].",
                    stream_graph.GetName().c_str());
    return ret;
  }

  FE_LOGI("Optimize stream graph successfully.");
  return SUCCESS;
}

Status FEGraphOptimizer::OptimizeStreamedWholeGraph(ge::ComputeGraph &graph) {
  if (!Configuration::Instance(graph_optimizer_attr_.engineName).CheckSupportCMO()) {
    FE_LOGI("FEGraphOptimizer not use cmo mode.");
    return SUCCESS;
  }

  FE_LOGI("FEGraphOptimizer start optimize streamed whole graph.");
  if (!init_flag_) {
    FE_LOGW("OptimizeStreamedWholeGraph is not allowed, initialize first is necessary.");
    return FAILED;
  }

  GenerateCMOTypeManager::Instance().Initialize();
  for (auto& node_ptr : graph.GetDirectNode()) {
    GenerateCMOTypeManager::Instance().GenerateType(node_ptr);
  }
  FE_LOGI("FEGraphOptimizer end optimize streamed whole graph.");
  return SUCCESS;
}

Status FEGraphOptimizer::GetAttributes(ge::GraphOptimizerAttribute& attrs) const {
  attrs = graph_optimizer_attr_;
  return SUCCESS;
}

Status FEGraphOptimizer::OptimizeWholeGraph(ge::ComputeGraph& graph) {
  // set fusion_virtual_op info to op
  bool need_set_virtual_op = fusion_priority_mgr_ptr_->GetFusionConfigParserPtr() == nullptr ||
                             fusion_priority_mgr_ptr_->GetFusionConfigParserPtr()->GetFusionSwitchByName(
                                 "FusionVirtualOpSetSwitch", UB_FUSION);
  if (ConcatSplitOptimizer(graph, need_set_virtual_op) != SUCCESS) {
    FE_LOGD("ConcatSplitOptimizer failed, graph [%s].", graph.GetName().c_str());
    return FAILED;
  }
  return SUCCESS;
}

LxFusionOptimizeResult FEGraphOptimizer::GenerateLxFusionOptimizeResult(const Status &l1_buffer_ret,
                                                                        const Status &l2_buffer_ret) const {
  bool is_l1_optimize = l1_buffer_ret == tune::SUCCESS || l1_buffer_ret == tune::HIT_FUSION_STRATEGY;
  bool is_l2_optimize = l2_buffer_ret == tune::SUCCESS || l2_buffer_ret == tune::HIT_FUSION_STRATEGY;
  if (is_l1_optimize && !is_l2_optimize) {
    return LxFusionOptimizeResult::L1_FUSION_STRATEGY;
  }
  if (!is_l1_optimize && is_l2_optimize) {
    return LxFusionOptimizeResult::L2_FUSION_STRATEGY;
  }
  if (is_l1_optimize && is_l2_optimize) {
    return LxFusionOptimizeResult::BOTH_FUSION_STRATEGY;
  }
  return LxFusionOptimizeResult::NO_FUSION_STRATEGY;
}

void FEGraphOptimizer::GetAOEOptionValue(AOEOption &aoe_opt) const {
  std::string license_aoe_val;
  ge::graphStatus status = ge::GetContext().GetOption("opt_module.aoe", license_aoe_val);
  bool status_flag = status == ge::GRAPH_SUCCESS && license_aoe_val.empty();
  if (status_flag) {
    aoe_opt = AOE_OPT_NOT_USE_KB;
  }

  std::string build_mode_value = Configuration::Instance(graph_optimizer_attr_.engineName).GetBuildMode();
  if (build_mode_value == ge::BUILD_MODE_BASELINE) {
    aoe_opt = AOE_OPT_ONLY_USE_KB;
  }
}

Status FEGraphOptimizer::BufferFusionProcess(ge::ComputeGraph& graph, GraphCommPtr graphCommPtr,
                                             LxFusionOptimizeResult& buffer_ret) {
  // 1. if find any unknown shape node, return success
  for (auto& node_ptr : graph.GetDirectNode()) {
    bool unknown_shape = false;
    bool unknown_sharp_flag = ge::NodeUtils::GetNodeUnknownShapeStatus(*(node_ptr.get()), unknown_shape) ==
        ge::GRAPH_SUCCESS && unknown_shape;
    if (unknown_sharp_flag) {
      FE_LOGI("Graph [%s] contains unknown shape op, can not do lx fusion optimize.", graph.GetName().c_str());
      return SUCCESS;
    }
  }

  // 2. get the l1_fusion_flag and l2_fusion_flag
  bool l1_fusion_flag = Configuration::Instance(graph_optimizer_attr_.engineName).EnableL1Fusion();
  bool l2_fusion_flag = Configuration::Instance(graph_optimizer_attr_.engineName).EnableL2Fusion();
  bool none_l1_and_l2_fusion_flag = !l1_fusion_flag && !l2_fusion_flag;
  if (none_l1_and_l2_fusion_flag) {
    FE_LOGD("Both l1 fusion and l2 fusion is not enabled.");
    return SUCCESS;
  }

  // TODO:pass flag to new func
  AOEOption aoe_opt = AOE_OPT_USE_KB;
  GetAOEOptionValue(aoe_opt);
  FE_LOGD("lxfusion license aoe :%d", aoe_opt);

  Status l1_buffer_ret = tune::NO_FUSION_STRATEGY;
  if (l1_fusion_flag) {
    LxFusionOptimizerFunc func = fusion_pass_mgr_ptr_->GetL1FusionOptimizerFunc();
    FE_CHECK(func == nullptr, REPORT_FE_ERROR("[SubGraphOpt][BufFusProc] L1FusionOptimizerFunc is nullptr."),
             return FAILED);
    l1_buffer_ret = func(graph, graphCommPtr, nullptr, graph_optimizer_attr_.engineName, aoe_opt);
    bool condition = (l1_buffer_ret == tune::SUCCESS || l1_buffer_ret == tune::HIT_FUSION_STRATEGY);
    if (condition) {
      FE_LOGI("L1 FUSION SUCCESS or HIT_FUSION_STRATEGY");
    } else if (l1_buffer_ret == tune::NO_FUSION_STRATEGY) {
      FE_LOGI("Lx fusion strategy is invalid.");
    } else {
      REPORT_INNER_ERROR("EF9999", "[SubGraphOpt][BufFusProc][fuc] L1FusionOptimizer failed.");
      return FAILED;
    }
    // allow both L1 and L2 while soc version is 1951
    if (!Configuration::Instance(graph_optimizer_attr_.engineName).IsDCSoc()) {
      l2_fusion_flag = false;
    }
  }

  Status l2_buffer_ret = tune::NO_FUSION_STRATEGY;
  if (l2_fusion_flag) {
    LxFusionOptimizerFunc func = fusion_pass_mgr_ptr_->GetL2FusionOptimizerFunc();
    FE_CHECK(func == nullptr, REPORT_FE_ERROR("[SubGraphOpt][BufFusProc] L2FusionOptimizerFunc is nullptr."),
             return FAILED);
    l2_buffer_ret = func(graph, graphCommPtr, nullptr, graph_optimizer_attr_.engineName, aoe_opt);
    bool condition = (l2_buffer_ret == tune::SUCCESS || l2_buffer_ret == tune::HIT_FUSION_STRATEGY);
    if (condition) {
      FE_LOGI("L2 FUSION SUCCESS or HIT_FUSION_STRATEGY");
    } else if (l2_buffer_ret == tune::NO_FUSION_STRATEGY) {
      FE_LOGI("Lx fusion strategy is invalid, go to l2 buffer.");
    } else {
      REPORT_INNER_ERROR("EF9999", "[SubGraphOpt][BufFusProc][fuc] L2FusionOptimizer failed.");
      return FAILED;
    }
  }
  FE_LOGI("[sg_kb_hit][%s][%u]", graph.GetName().c_str(), l2_buffer_ret);
  buffer_ret = GenerateLxFusionOptimizeResult(l1_buffer_ret, l2_buffer_ret);
  if (buffer_ret != LxFusionOptimizeResult::NO_FUSION_STRATEGY) {
    (void) ge::AttrUtils::SetBool(graph, NEED_RE_PRECOMPILE, true);
    ge::GraphUtils::DumpGEGraph(graph.shared_from_this(), "AfterBufferFusion");
    ge::GraphUtils::DumpGEGraphToOnnx(graph, "AfterBufferFusion");
  }

  return SUCCESS;
}

Status FEGraphOptimizer::BufferFusionRecovery(ge::ComputeGraph& graph,
                                              std::vector<ge::NodePtr>& buff_fus_compile_failed_nodes) {
  std::vector<ge::NodePtr> buff_fus_rollback_nodes;
  std::vector<ge::NodePtr> buff_fus_to_del_nodes;
  tune::Status ret_pass = tune::SUCCESS;
  std::string build_mode_value =
      Configuration::Instance(graph_optimizer_attr_.engineName).GetGeContextOptionValue(ge::BUILD_MODE);
  if (Configuration::Instance(graph_optimizer_attr_.engineName).EnableL1Fusion()) {
    // ADD L1 block rollback
    LxFusionRecoveryFunc func = fusion_pass_mgr_ptr_->GetL1FusionRecoveryFunc();
    if (func == nullptr) {
      REPORT_FE_ERROR("[SubGraphOpt][Compile][L1FusionRecovery] L1FusionRecoveryFunc is nullptr.");
      return FAILED;
    }
    ret_pass = func(graph, buff_fus_compile_failed_nodes, &buff_fus_rollback_nodes, &buff_fus_to_del_nodes);
  } else if (Configuration::Instance(graph_optimizer_attr_.engineName).GetBufferFusionMode() == EN_L2_FUSION ||
             build_mode_value == ge::BUILD_MODE_TUNING) {
    // ADD L2 block rollback
    LxFusionRecoveryFunc func = fusion_pass_mgr_ptr_->GetL2FusionRecoveryFunc();
    if (func == nullptr) {
      REPORT_FE_ERROR("[SubGraphOpt][Compile][L2FusionRecovery] L2FusionRecoveryFunc is nullptr.");
      return FAILED;
    }
    ret_pass = func(graph, buff_fus_compile_failed_nodes, &buff_fus_rollback_nodes, &buff_fus_to_del_nodes);
  } else {
    return SUCCESS;
  }

  bool recovery_status =
      (ret_pass != tune::SUCCESS || buff_fus_rollback_nodes.empty() || buff_fus_to_del_nodes.empty());
  if (recovery_status) {
    REPORT_FE_ERROR("[SubGraphOpt][Compile][Recovery] Failed to recover graph %s. Result is %u.",
                    graph.GetName().c_str(), recovery_status);
    return FAILED;
  }

  buff_fus_compile_failed_nodes.clear();
  Status ret = op_compiler_ptr_[COMMON]->CompileOp(graph, buff_fus_compile_failed_nodes, buff_fus_rollback_nodes,
                                                   buff_fus_to_del_nodes);
  if (ret != SUCCESS) {
    REPORT_FE_ERROR(
        "[SubGraphOpt][Compile][CompileFailed] Failed to compile roll-backed nodes for graph %s. Result is %u.",
        graph.GetName().c_str(), ret);
    return ret;
  }

  return SUCCESS;
}

void FEGraphOptimizer::ClearUnknowShapeAttr(const ge::ComputeGraph& graph) const {
  for (auto& node : graph.GetAllNodes()) {
    (void)node->GetOpDesc()->DelAttr(fe::ATTR_NAME_UNKNOWN_SHAPE);
  }
}

void FEGraphOptimizer::ClearLxFusionResource(const ge::ComputeGraph& graph) const {
  LxFusionFinalizeFunc func = fusion_pass_mgr_ptr_->GetLxFusionFinalizeFunc();
  if (func == nullptr) {
    FE_LOGW("LxFusionFinalizeFunc is nullptr.");
    return;
  }
  tune::Status ret_pass = func(graph);
  if (ret_pass != tune::SUCCESS) {
    FE_LOGW("Failed to recover graph %s, Result is %u.", graph.GetName().c_str(), ret_pass);
  }
}

Status FEGraphOptimizer::CloseRcCache(const ge::ComputeGraph &graph) {
  if (Configuration::Instance(graph_optimizer_attr_.engineName).IsCloudSoc()) {
    auto nodes = graph.GetAllNodes();
    for (auto &node : nodes) {
      if (IsUnKnownShapeOp(*(node->GetOpDesc().get()))) {
        FE_LOGD("Node[%s] is unknown node, set append args mode as no args.", node->GetName().c_str());
        Configuration::Instance(graph_optimizer_attr_.engineName).SetAppendArgsMode(AppendArgsMode::NO_ARGS);
        return SUCCESS;
      }
    }
  }
  return SUCCESS;
}
}  // namespace fe
