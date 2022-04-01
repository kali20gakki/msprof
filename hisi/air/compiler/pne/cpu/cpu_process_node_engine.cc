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

#include "pne/cpu/cpu_process_node_engine.h"

#include <cstdio>
#include <map>

#include "pne/manager/process_node_engine_manager.h"
#include "framework/common/debug/log.h"
#include "common/plugin/ge_util.h"
#include "common/util/error_manager/error_manager.h"
#include "framework/common/debug/ge_log.h"
#include "analyzer/analyzer.h"
#include "graph/ge_context.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/node_utils.h"
#include "init/gelib.h"
#include "graph/manager/graph_manager.h"

namespace ge {

Status CPUProcessNodeEngine::Initialize(const std::map<std::string, std::string> &options) {
  (void) options;
  engine_id_ = PNE_ID_CPU;
  return SUCCESS;
}

Status CPUProcessNodeEngine::Finalize() {
  return SUCCESS;
}

Status CPUProcessNodeEngine::OptimizeGraph(const std::vector<GeTensor> &inputs, ComputeGraphPtr &compute_graph) {
  if (impl_ != nullptr) {
    return impl_->OptimizeGraph(inputs, compute_graph);
  }
  return FAILED;
}

Status CPUProcessNodeEngine::BuildGraph(ComputeGraphPtr &compute_graph, PneModelPtr &model) {
  if (impl_ != nullptr) {
    return impl_->BuildGraph(compute_graph, model);
  }
  return FAILED;
}

REGISTER_PROCESS_NODE_ENGINE(PNE_ID_CPU, CPUProcessNodeEngine);
}  // namespace ge
