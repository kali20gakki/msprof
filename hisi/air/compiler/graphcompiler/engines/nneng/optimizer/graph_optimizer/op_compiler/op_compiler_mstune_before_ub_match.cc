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

#include "graph_optimizer/op_compiler/op_compiler_mstune_before_ub_match.h"

namespace fe {

OpCompilerMstuneBeforeUbMatch::OpCompilerMstuneBeforeUbMatch(const std::string& compiler_name,
    const std::string& engine_name, OpStoreAdapterManagerPtr op_store_adapter_manager_ptr)
    : OpCompiler(compiler_name, engine_name, op_store_adapter_manager_ptr) {}

OpCompilerMstuneBeforeUbMatch::~OpCompilerMstuneBeforeUbMatch() {}

Status OpCompilerMstuneBeforeUbMatch::RunCompileProcess(ge::ComputeGraph& graph,
    const std::shared_ptr<GraphComm>& graph_comm_ptr,
    std::vector<ge::NodePtr> &buff_fus_compile_failed_nodes,
    bool &need_post_process) {
  /* Some nodes needs to be re-pre-compiled after Ub fusion matching.
   * Because there format or data type. */
  bool need_re_precompile_graph = false;
  (void)ge::AttrUtils::GetBool(graph, NEED_RE_PRECOMPILE, need_re_precompile_graph);
  if (need_re_precompile_graph) {
    Status ret = PreCompileOp(graph);
    if (ret != SUCCESS) {
      REPORT_FE_ERROR("[SubGraphOpt][Compile][OpCmplMstune] PreCompileOp failed after buffer fusion.");
      return ret;
    }
  }

  CompileInfoParam compile_info(buff_fus_compile_failed_nodes);
  vector<ge::NodePtr> nodes_be_compiled;
  vector<ge::NodePtr> buff_fus_rollback_nodes;

  Status ret = GetFusionScope(graph, buff_fus_rollback_nodes, compile_info.fusion_nodes_map, nodes_be_compiled);

  if (ret != SUCCESS) {
    return ret;
  }

  /* compile and roll-back(fusion op if fused compiling failed.) all tbe op first to make sure
   * the sub-graph return to tuning tools can be successfully compiled. */
  ret = CompileOpOnly(graph, compile_info);
  if (ret != SUCCESS) {
    REPORT_FE_ERROR("[SubGraphOpt][Compile][OpCmplMstune] Failed to compile graph %s in step before ub matching.",
                    graph.GetName().c_str());
    return ret;
  }

  FE_LOGI("Stop to do optimize fused graph in step before ub matching of tuning process.");
  need_post_process = false;
  return SUCCESS;
}
}