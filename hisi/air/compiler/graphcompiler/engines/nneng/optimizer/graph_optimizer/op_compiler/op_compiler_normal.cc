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

#include "graph_optimizer/op_compiler/op_compiler_normal.h"
#include <unordered_set>

using namespace std;
namespace fe {

OpCompilerNormal::OpCompilerNormal(const std::string& compiler_name, const std::string& engine_name,
                                   OpStoreAdapterManagerPtr op_store_adapter_manager_ptr)
    :OpCompiler(compiler_name, engine_name, op_store_adapter_manager_ptr) {}

OpCompilerNormal::~OpCompilerNormal() {}

Status OpCompilerNormal::Initialize(const BufferFusionFunc &func) {
  // if graph optimizer has been initialized, return SUCCESS
  if (init_flag_) {
    FE_LOGW("OpCompiler has been initialized.");
    return SUCCESS;
  }

  init_flag_ = true;
  bufferFusionFunc_ = func;
  return SUCCESS;
}

bool OpCompilerNormal::HasCompileStrategy(const vector<ge::NodePtr> &nodes_be_compiled) const {
  if (!nodes_be_compiled.empty()) {
    for (const ge::NodePtr &node_ptr : nodes_be_compiled) {
      std::string op_compile_strategy;
      if (ge::AttrUtils::GetStr(node_ptr->GetOpDesc(), ge::ATTR_NAME_OP_COMPILE_STRATEGY, op_compile_strategy) &&
          !op_compile_strategy.empty()) {
        FE_LOGI("Node[%s, %s] has compile strategy[%s] and this graph needs to be recompile.",
                node_ptr->GetName().c_str(), node_ptr->GetType().c_str(), op_compile_strategy.c_str());
        return true;
      }
    }
  }
  return false;
}

Status OpCompilerNormal::PreCompileProcess(ge::ComputeGraph& graph, bool &sgt_flag) {
  /* Some nodes needs to be re-pre-compiled after Ub fusion matching.
   * Because there format or data type. */
  Status ret;
  bool need_re_precompile_graph = false;
  (void)ge::AttrUtils::GetBool(graph, NEED_RE_PRECOMPILE, need_re_precompile_graph);
  if (need_re_precompile_graph) {
    ret = PreCompileOp(graph);
    if (ret != SUCCESS) {
      REPORT_FE_ERROR("[SubGraphOpt][Compile][Normal] PreCompileOp failed after buffer fusion for graph %s.",
                      graph.GetName().c_str());
      return ret;
    }
  }

  ret = PreCompileThreadOp(graph, sgt_flag);
  if (ret != SUCCESS) {
    REPORT_FE_ERROR("[SubGraphOpt][Compile][Normal] PreCompileThreadOp failed after buffer fusion for graph %s.",
                    graph.GetName().c_str());
    return ret;
  }
  FE_LOGI("Finish PreCompileProcess. ffts sgt flag is %u", sgt_flag);
  return SUCCESS;
}

Status OpCompilerNormal::ReCompileWithNoFusionStrategy(const ge::ComputeGraph& graph,
                                                       const vector<ge::NodePtr> &nodes_be_compiled,
                                                       CompileInfoParam &compile_info) {
  // if auto tune is on, compile should be executed again with auto tune mode param
  Status ret;
  bool compile_flag = (Configuration::Instance(engine_name_).GetAutoTuneMode() != TUNE_MODE_NO_TUNE ||
                       HasCompileStrategy(nodes_be_compiled));
  if (compile_flag) {
    ret = CompileOpOnly(graph, compile_info);
    if (ret != SUCCESS) {
      return ret;
    }
  }
  /* using the first compiling result only because no new nodes are created. */
  FE_LOGI("Lx-Fusion does not change the graph. We just use old compile result.");
  ret = ParseJsonAndCompressOp(graph, compile_info.scope_json_map, nodes_be_compiled);
  if (ret != SUCCESS) {
    REPORT_FE_ERROR("[SubGraphOpt][Compile][Normal] Failed to parse json or compress op for graph[%s].",
                    graph.GetName().c_str());
    return ret;
  }
  return SUCCESS;
}

Status OpCompilerNormal::RunCompileProcess(ge::ComputeGraph& graph, const std::shared_ptr<GraphComm>& graph_comm_ptr,
                                           std::vector<ge::NodePtr> &buff_fus_compile_failed_nodes,
                                           bool &need_post_process) {
  bool sgt_flag = false;
  Status ret = PreCompileProcess(graph, sgt_flag);
  if (ret != SUCCESS) {
    REPORT_FE_ERROR("[SubGraphOpt][Compile][Normal] PreCompile failed after buffer fusion for graph %s.",
                    graph.GetName().c_str());
    return ret;
  }

  CompileInfoParam compile_info(buff_fus_compile_failed_nodes);
  compile_info.compile_strategy = CompileStrategy::COMPILE_STRATEGY_NO_TUNE;
  vector<ge::NodePtr> nodes_be_compiled;
  vector<ge::NodePtr> buff_fus_rollback_nodes;

  ret = OpCompiler::GetFusionScope(graph, buff_fus_rollback_nodes, compile_info.fusion_nodes_map, nodes_be_compiled);
  if (ret != SUCCESS) {
    return SUCCESS;
  }

  /* Compile first to get a completely same graph as tuinng step which will lead us find
   * a optimization strategy. */
  FE_LOGI("Compile op and rollback failed fusion op first to ensure the graph will be successfully compiled.");
  FE_LOGI("Total size is [%zu]", nodes_be_compiled.size());
  ret = CompileOpOnly(graph, compile_info);
  if (ret != SUCCESS) {
    return ret;
  }

  LxFusionOptimizeResult buffer_ret = LxFusionOptimizeResult::NO_FUSION_STRATEGY;
  if (!sgt_flag) {
    FE_CHECK_NOTNULL(bufferFusionFunc_);
    /* Call Lxfusion to optimize the graph. */
    ret = bufferFusionFunc_(graph, graph_comm_ptr, buffer_ret);
    if (ret != SUCCESS) {
      REPORT_FE_ERROR("[SubGraphOpt][Compile][Normal] BufferFusionProcess failed for graph %s result %u.",
                      graph.GetName().c_str(), ret);
      return FAILED;
    }
  }
  FE_LOGD("Finish optimizing the graph[%s] by lxfusion. LxFusion optimize result[%d]",
          graph.GetName().c_str(), static_cast<int32_t>(buffer_ret));
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
    ret = ReCompileWithNoFusionStrategy(graph, nodes_be_compiled, compile_info);
    if (ret != SUCCESS) {
      REPORT_FE_ERROR("[SubGraphOpt][Compile][RunCmplProc] Failed to re-compile op with no lx fusion for graph [%s]",
                      graph.GetName().c_str());
      return ret;
    }
  }
  FE_LOGI("Successfully compile op in normal mode.");
  need_post_process = true;
  return SUCCESS;
}

}
