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
#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_OP_COMPILER_OP_COMPILER_NORMAL_MODE_H_
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_OP_COMPILER_OP_COMPILER_NORMAL_MODE_H_

#include "graph_optimizer/op_compiler/op_compiler.h"
#include "common/configuration.h"

namespace fe {
class OpCompilerNormal : public OpCompiler {
 public:
  using ConstComputeGraph = const ge::ComputeGraph;
  template <class T>
  using Vistor = RangeVistor<T, std::shared_ptr<ConstComputeGraph>>;

  OpCompilerNormal(const std::string& compiler_name, const std::string &engine_name,
                   OpStoreAdapterManagerPtr op_store_adapter_manager_ptr);

  ~OpCompilerNormal() override;

  /**
   * @ingroup fe
   * @brief prohibit copy and assign construct
   */
  OpCompilerNormal(const OpCompilerNormal &) = delete;

  OpCompilerNormal &operator=(const OpCompilerNormal &) = delete;

  /* There are two scenarios which are different from baseline:
   * 1. Some of the nodes in a fusion scope are set attribute "need_re_precompile":
   *     In this case, we need to find all nodes with that scope id compile them together.
   *     That means if one node of a fusion scope need to be re-pre-compiled, the whole scope
   *     needs to be RE-COMPILED.
   *
   * 2. some fusion nodes with negative scope id are duplicated by lx-fusion:
   *     In this case, we need to give them different new scope id. Because they
   *     are expect to be compiled alone.
   *     And the new (negative) scope id should be different from the other negative ones*/
  Status RunCompileProcess(ge::ComputeGraph& graph, const std::shared_ptr<GraphComm>& graph_comm_ptr,
                           std::vector<ge::NodePtr> &buff_fus_compile_failed_nodes,
                           bool &need_post_process) override;

  Status Initialize(const BufferFusionFunc &func);
 private:
  bool HasCompileStrategy(const vector<ge::NodePtr> &nodes_be_compiled) const;
  Status PreCompileProcess(ge::ComputeGraph& graph, bool &sgt_flag);
  Status ReCompileWithNoFusionStrategy(const ge::ComputeGraph& graph, const vector<ge::NodePtr> &nodes_be_compiled,
                                       CompileInfoParam &compile_info);
  BufferFusionFunc bufferFusionFunc_{nullptr};
};
}

#endif //FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_OP_COMPILER_OP_COMPILER_NORMAL_MODE_H_
