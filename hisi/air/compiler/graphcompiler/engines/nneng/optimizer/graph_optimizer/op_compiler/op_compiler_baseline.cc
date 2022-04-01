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

#include "graph_optimizer/op_compiler/op_compiler_baseline.h"
#include "common/graph/fe_graph_utils.h"
#include <unordered_set>
#include "common/configuration.h"

namespace fe {

OpCompilerBaseline::OpCompilerBaseline(const std::string &compiler_name, const std::string &engine_name,
                                       OpStoreAdapterManagerPtr op_store_adapter_manager_ptr)
    : OpCompiler(compiler_name, engine_name, op_store_adapter_manager_ptr) {}

OpCompilerBaseline::~OpCompilerBaseline() {}

Status OpCompilerBaseline::Initialize(const BufferFusionFunc &func) {
  // if graph optimizer has been initialized, return SUCCESS
  if (init_flag_) {
    FE_LOGW("OpCompiler has been initialized.");
    return SUCCESS;
  }

  init_flag_ = true;
  bufferFusionFunc_ = func;
  return SUCCESS;
}

Status OpCompilerBaseline::GetFusionScope(ge::ComputeGraph &graph, ScopeNodeIdMap &fusion_nodes_map,
                                          std::vector<ge::NodePtr> &nodes_be_compiled,
                                          std::vector<ge::NodePtr> &all_nodes_after_lx_fusion) {
  std::string graph_name = graph.GetName();
  auto all_nodes = graph.GetDirectNode();
  /* Find the minimum scope id in this graph. */
  for (auto &node : all_nodes) {
    (void)ge::AttrUtils::SetStr(node->GetOpDesc(), "_graph_name", graph_name);
  }

  // if auto tune is on, all nodes should be re-compiled
  for (auto &node : all_nodes) {
    nodes_be_compiled.emplace_back(node);
    all_nodes_after_lx_fusion.emplace_back(node);
  }

  if (GetScopeNodeMap(graph, nodes_be_compiled, fusion_nodes_map) != SUCCESS) {
    REPORT_FE_ERROR("[SubGraphOpt][Compile][GetFusScope] GetScopeNodeMap failed, graph [%s].", graph.GetName().c_str());
    return FAILED;
  }

  return SUCCESS;
}

Status OpCompilerBaseline::RunCompileProcess(ge::ComputeGraph &graph, const std::shared_ptr<GraphComm> &graph_comm_ptr,
                                             std::vector<ge::NodePtr> &buff_fus_compile_failed_nodes,
                                             bool &need_post_process) {
  Status ret;

  CompileInfoParam compile_info(buff_fus_compile_failed_nodes);
  compile_info.compile_strategy = CompileStrategy::COMPILE_STRATEGY_NO_TUNE;
  vector<ge::NodePtr> nodes_be_compiled;
  vector<ge::NodePtr> buff_fus_rollback_nodes;

  ret = OpCompiler::GetFusionScope(graph, buff_fus_rollback_nodes, compile_info.fusion_nodes_map, nodes_be_compiled);
  if (ret != SUCCESS) {
    return SUCCESS;
  }
  ret = CompileOpOnly(graph, compile_info);
  if (ret != SUCCESS) {
    return ret;
  }

  LxFusionOptimizeResult buffer_ret = LxFusionOptimizeResult::NO_FUSION_STRATEGY;

  FE_CHECK_NOTNULL(bufferFusionFunc_);
  ret = bufferFusionFunc_(graph, graph_comm_ptr, buffer_ret);
  /* Call Lxfusion to optimize the graph. */
  if (ret != SUCCESS) {
    REPORT_FE_ERROR("[SubGraphOpt][Compile][Normal] BufferFusionProcess failed for graph %s result %u.",
                    graph.GetName().c_str(), ret);
    return FAILED;
  }
  FE_LOGD("Finish optimizing the graph by lxfusion.");

  // pre compile op which l1fusion or l2fusion changed
  compile_info.compile_strategy = CompileStrategy::COMPILE_STRATEGY_OP_SPEC;
  if (buffer_ret != LxFusionOptimizeResult::NO_FUSION_STRATEGY) {
    FE_LOGI("Lx-fusion change the graph and we need re-precompile graph.");
    ret = ReCompileOpAfterLxFusion(graph, compile_info, buffer_ret);
    if (ret != SUCCESS) {
      REPORT_FE_ERROR("[SubGraphOpt][Compile][RunCmplProc] Failed to re-compile op after lx fusion for graph [%s]",
                      graph.GetName().c_str());
      return FAILED;
    }
  } else {
    ret = CompileOpOnly(graph, compile_info);
    if (ret != SUCCESS) {
      return ret;
    }
    /* using the first compiling result only because no new nodes are created. */
    FE_LOGI("Lx-Fusion does not change the graph. We just use old compile result.");
    ret = ParseJsonAndCompressOp(graph, compile_info.scope_json_map, nodes_be_compiled);
    if (ret != SUCCESS) {
      REPORT_FE_ERROR("[SubGraphOpt][Compile][Baseline] Failed to parse json or compress op for graph[%s].",
                      graph.GetName().c_str());
      return ret;
    }
  }
  FE_LOGI("Successfully compile op in Baseline mode.");
  need_post_process = true;
  return SUCCESS;
}
}  // namespace fe