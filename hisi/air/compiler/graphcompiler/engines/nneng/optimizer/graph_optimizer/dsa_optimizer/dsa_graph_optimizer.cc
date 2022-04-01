/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2022. All rights reserved.
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

#include "graph_optimizer/dsa_optimizer/dsa_graph_optimizer.h"
#include <memory>
#include <vector>
#include "graph/ge_context.h"

namespace fe {
DSAGraphOptimizer::DSAGraphOptimizer(FEOpsKernelInfoStorePtr fe_ops_kernel_info_store_ptr,
                                     OpStoreAdapterManagerPtr op_store_adapter_manager_ptr, std::string engine_name)
        : ops_kernel_info_store_ptr_(fe_ops_kernel_info_store_ptr),
          op_store_adapter_manager_ptr_(op_store_adapter_manager_ptr),
          op_impl_type_judge_ptr_(nullptr),
          format_dtype_setter_ptr_(nullptr),
          space_size_calculator_ptr_(nullptr),
          graph_optimizer_attr_({engine_name, ge::ENGINE}),
          init_flag_(false) {}

DSAGraphOptimizer::~DSAGraphOptimizer() {}

Status DSAGraphOptimizer::Initialize(const std::map<string, string>& options,
                                     ge::OptimizeUtility *const optimize_utility) {
  // if graph optimizer has been initialized, return success
  if (init_flag_) {
    FE_LOGW("DSAGraphOptimizer has been initialized.");
    return SUCCESS;
  }

  init_flag_ = true;
  FE_LOGD("Begin to init DSAGraphOptimizer in engine[%s]", graph_optimizer_attr_.engineName.c_str());
  // initialize op compiler
  FE_CHECK(ops_kernel_info_store_ptr_ == nullptr, FE_LOGE("[GraphOpt][Init] opsKernelInfoStorePtr_ is null."),
           return FAILED);
  FE_CHECK(op_store_adapter_manager_ptr_ == nullptr, FE_LOGE("[GraphOpt][Init] opStoreAdapterManagerPtr_ is null."),
           return FAILED);

  FE_MAKE_SHARED(op_impl_type_judge_ptr_ = std::make_shared<OpImplTypeJudge>(graph_optimizer_attr_.engineName,
                                                                             ops_kernel_info_store_ptr_),
                  return GRAPH_OPTIMIZER_MAKE_SHARED_FAILED);

  FE_MAKE_SHARED(format_dtype_setter_ptr_ = std::make_shared<FormatDtypeSetter>(graph_optimizer_attr_.engineName,
                                                                                op_store_adapter_manager_ptr_),
                 return GRAPH_OPTIMIZER_MAKE_SHARED_FAILED);
  FE_MAKE_SHARED(space_size_calculator_ptr_ = std::make_shared<SpaceSizeCalculator>(),
                 return GRAPH_OPTIMIZER_MAKE_SHARED_FAILED);

  FE_LOGI("Initialize success.");

  return SUCCESS;
}

Status DSAGraphOptimizer::Finalize() {
  if (!init_flag_) {
    FE_LOGW("DSAGraphOptimizer finalize is not allowed, initialize first is necessary.");
    return SUCCESS;
  }

  init_flag_ = false;
  FE_LOGD("Finalized success.");

  return SUCCESS;
}

Status DSAGraphOptimizer::OptimizeOriginalGraph(ge::ComputeGraph& graph) {
  if (!init_flag_) {
    REPORT_FE_ERROR("[GraphOpt][init] DSAGraphOptimizer has not been initialized.");
    return FAILED;
  }
  OpFormatDtypeJudgePtr op_format_dtype_judge_ptr = nullptr;
  ReflectionBuilderPtr reflection_builder_ptr = nullptr;

  FE_MAKE_SHARED(reflection_builder_ptr = std::make_shared<ge::RefRelations>(),
                 return GRAPH_OPTIMIZER_MAKE_SHARED_FAILED);
  ops_kernel_info_store_ptr_->SetCheckSupportedStaticFlag(false);
  FE_LOGD("Begin to optimize original graph to judge insert graph[%s], in engine[%s]", graph.GetName().c_str(),
          graph_optimizer_attr_.engineName.c_str());

  FE_MAKE_SHARED(op_format_dtype_judge_ptr = std::make_shared<OpFormatDtypeJudge>(
          graph_optimizer_attr_.engineName, op_store_adapter_manager_ptr_, reflection_builder_ptr),
                 return GRAPH_OPTIMIZER_MAKE_SHARED_FAILED);
  if (op_format_dtype_judge_ptr->Initialize() != SUCCESS) {
    REPORT_FE_ERROR("[GraphOptJdgInst][Init] Failed to initialize op_format_dtype_judge_ptr for graph[%s].",
                    graph.GetName().c_str());
    return FAILED;
  }

  Status ret = OptimizeOriginalGraphOpJudgeAndFormatDtypeSetter(graph);
  if (ret != SUCCESS) {
    FE_LOGE("[GraphOptJdgInst][JudgeAndFormat] Fail to judge op impl or set support format and dtype for graph[%s]!",
            graph.GetName().c_str());
    return ret;
  }
  ret = op_format_dtype_judge_ptr->Judge(graph);
  if (ret != SUCCESS) {
    REPORT_FE_ERROR("[GraphOptJdgInst][Judge] Judge the format and data type failed, graph [%s].",
                    graph.GetName().c_str());
    FeGraphUtils::DumpGraphAndOnnx(graph, "OptimizeOriginalGraph_DSAFeOpJudgeAfter_Failed");
    FeGraphUtils::DumpSubGraphAndOnnx(graph, "OptimizeOriginalGraph_DSAFeOpJudgeAfter_Failed_Subgraph");
    return ret;
  }
  FeGraphUtils::DumpGraphAndOnnx(graph, "OptimizeOriginalGraph_DSAFeOpJudgeAfter");
  FeGraphUtils::DumpSubGraphAndOnnx(graph, "OptimizeOriginalGraph_DSAFeOpJudgeAfter_Subgraph");
  FE_LOGI("Optimizing original graph[%s] judge the format and data type of the input and output desc of op success.",
          graph.GetName().c_str());

  if (SetDSAOpSliceInfo(graph) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOptJgtInst][SetSliceInfo] Set DSA op slice info failed, graph [%s]",
                    graph.GetName().c_str());
    return FAILED;
  }
  ops_kernel_info_store_ptr_->SetCheckSupportedStaticFlag(true);
  return SUCCESS;
}

Status DSAGraphOptimizer::OptimizeOriginalGraphOpJudgeAndFormatDtypeSetter(ge::ComputeGraph& graph) const {
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
    REPORT_FE_ERROR("[GraphOptJdgInst][SetFormat] Set the support format and dtype information failed, graph[%s].",
                    graph.GetName().c_str());
    return ret;
  }
  FE_LOGI("Optimizing original graph[%s] set the support format and dtype information success.",
          graph.GetName().c_str());
  return SUCCESS;
}

void DSAGraphOptimizer::SetInputSplitInfo(AxisSplitMap& axis_split_map, const int8_t& input_index,
                                          const int8_t& input_axis) const {
  InputSplitInfo input_split_info;
  if (!input_split_info.Initialize()) {
    REPORT_FE_ERROR("[GraphOptJdgInst][SetOpInfo][SetInSplitInfo] input_split_info initialize failed");
    return;
  }
  input_split_info.SetIndex(input_index);
  std::vector<int64_t> input_vec_first_tensor = {input_axis};
  input_split_info.SetAxis(input_vec_first_tensor);
  axis_split_map.AddInputSplitInfo(input_split_info);
}

void DSAGraphOptimizer::SetOutputSplitInfo(AxisSplitMap& axis_split_map, const int8_t& output_index,
                                           const int8_t& output_axis) const {
  OutputSplitInfo output_split_info;
  if (!output_split_info.Initialize()) {
    REPORT_FE_ERROR("[GraphOptJdgInst][SetOpInfo][SetOutSplitInfo] output_split_info initialize failed");
    return;
  }
  output_split_info.SetIndex(output_index);
  std::vector<int64_t> output_vec = {output_axis};
  output_split_info.SetAxis(output_vec);
  axis_split_map.AddOutputSplitInfo(output_split_info);
}

Status DSAGraphOptimizer::SetDSAOpSliceInfo(ge::ComputeGraph &graph) const {
  auto nodes = graph.GetDirectNode();
  for (auto &node : nodes) {
    ge::OpDescPtr op_desc = node->GetOpDesc();
    FE_CHECK_NOTNULL(op_desc);
    int32_t imply_type = -1;
    (void)ge::AttrUtils::GetInt(op_desc, FE_IMPLY_TYPE, imply_type);
    OpImplType op_impl_type = static_cast<OpImplType>(imply_type);
    if (op_impl_type == EN_IMPL_HW_DSA) {
      OpCalcInfo op_calc_info;
      if (!op_calc_info.Initialize()) {
        REPORT_FE_ERROR("[DSAOptimizer][SetSliceInfo] op_calc_info initialize failed.");
        return FAILED;
      }
      std::vector<AxisSplitMap> axis_split_maps;
      AxisSplitMap axis_split_map;
      if (!axis_split_map.Initialize()) {
        REPORT_FE_ERROR("[DSAOptimizer][SetSliceInfo] axis_split_map initialize failed.");
        return FAILED;
      }
      size_t input_size = node->GetInDataNodes().size();
      size_t output_size = node->GetOutDataNodes().size();
      for (size_t i = 0; i < input_size; i++) {
        SetInputSplitInfo(axis_split_map, i, 0);
      }
      for (size_t i = 0; i < output_size; i++) {
        SetOutputSplitInfo(axis_split_map, i, 0);
      }
      axis_split_maps.push_back(axis_split_map);
      op_calc_info.SetAxisSplitMaps(axis_split_maps);
      std::string op_calc_info_str;
      SetOpSliceInfoToJson(op_calc_info, op_calc_info_str);
      (void)ge::AttrUtils::SetStr(op_desc, OP_SLICE_INFO, op_calc_info_str);
    }
  }
  return SUCCESS;
}

Status DSAGraphOptimizer::SetDSAOpWorkspaces(ge::ComputeGraph &graph) const {
  auto nodes = graph.GetDirectNode();
  for (auto &node : nodes) {
    ge::OpDescPtr op_desc = node->GetOpDesc();
    FE_CHECK_NOTNULL(op_desc);
    int32_t imply_type = -1;
    (void)ge::AttrUtils::GetInt(op_desc, FE_IMPLY_TYPE, imply_type);
    OpImplType op_impl_type = static_cast<OpImplType>(imply_type);
    if (op_impl_type == EN_IMPL_HW_DSA) {
      std::vector<int64_t> tvm_workspace_sizes = {48, 48};
      std::vector<int32_t> memory_no_reuse = {1, 1};
      op_desc->SetWorkspaceBytes(tvm_workspace_sizes);
      (void)ge::AttrUtils::SetListInt(op_desc, ge::ATTR_NAME_WORKSPACE_MEMORY_NO_REUSE_SCOPE, memory_no_reuse);
      FE_LOGD("Set workspace bytes to dsa node[name:%s, type:%s] success.",
              node->GetName().c_str(), node->GetType().c_str());
    }
  }
  return SUCCESS;
}

Status DSAGraphOptimizer::OptimizeFusedGraph(ge::ComputeGraph& graph) {
  FE_LOGD("Begin to optimizing fused graph in engine[%s].", graph_optimizer_attr_.engineName.c_str());

  if (!init_flag_) {
    FE_LOGW("OptimizeFusedGraph is not allowed, initialize firstly.");
    return FAILED;
  }
  FE_TIMECOST_START(OptimizeFusedGraph);
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

  if (SetDSAOpWorkspaces(graph) != SUCCESS) {
    return FAILED;
  }
  FE_TIMECOST_END(OptimizeFusedGraph, "DSAGraphOptimizer::OptimizeFusedGraph");
  return SUCCESS;
}

Status DSAGraphOptimizer::GetAttributes(ge::GraphOptimizerAttribute& attrs) const {
  attrs = graph_optimizer_attr_;
  return SUCCESS;
}
}  // namespace fe
