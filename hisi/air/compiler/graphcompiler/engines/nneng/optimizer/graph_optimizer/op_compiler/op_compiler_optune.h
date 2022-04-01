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
#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_OP_COMPILER_OP_COMPILER_OPTUNE_H_
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_OP_COMPILER_OP_COMPILER_OPTUNE_H_

#include "graph_optimizer/op_compiler/op_compiler.h"

namespace fe {
class OpCompilerOpTune : public OpCompiler {
 public:
  OpCompilerOpTune(const std::string& compiler_name, const std::string &engine_name,
                   OpStoreAdapterManagerPtr op_store_adapter_manager_ptr);

  ~OpCompilerOpTune() override;

  /**
   * @ingroup fe
   * @brief prohibit copy and assign construct
   */
  OpCompilerOpTune(const OpCompilerOpTune &) = delete;

  OpCompilerOpTune &operator=(const OpCompilerOpTune &) = delete;

  /* buff_fus_compile_failed_nodes is not necessary for op-tune. */
  Status RunCompileProcess(ge::ComputeGraph& graph, const std::shared_ptr<GraphComm>& graph_comm_ptr,
                           std::vector<ge::NodePtr> &buff_fus_compile_failed_nodes,
                           bool &need_post_process) override;

  Status Initialize(BufferFusionFunc &func);
 private:
  BufferFusionFunc bufferFusionFunc_{nullptr};
};
}

#endif //FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_OP_COMPILER_OP_COMPILER_OPTUNE_H_
