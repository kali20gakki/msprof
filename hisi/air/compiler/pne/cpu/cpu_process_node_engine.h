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

#ifndef COMPILE_PROCESS_NODE_ENGINE_CPU_PROCESS_NODE_ENGINE_H_
#define COMPILE_PROCESS_NODE_ENGINE_CPU_PROCESS_NODE_ENGINE_H_

#include <map>
#include <string>
#include <vector>

#include "framework/pne/process_node_engine.h"
#include "framework/common/ge_inner_error_codes.h"
#include "framework/common/ge_types.h"
#include "graph/types.h"

namespace ge {
class GraphManager;
class CPUProcessNodeEngine : public ProcessNodeEngine {
 public:
  CPUProcessNodeEngine() = default;
  ~CPUProcessNodeEngine() override = default;

  Status Initialize(const std::map<std::string, std::string> &options) override;

  Status Finalize() override;

  Status OptimizeGraph(const std::vector<GeTensor> &inputs, ComputeGraphPtr &compute_graph) override;

  Status BuildGraph(ComputeGraphPtr &compute_graph, PneModelPtr &model) override;

  const std::string &GetEngineName(const ge::NodePtr &node_ptr = nullptr) const override {
    return engine_id_;
  }

  inline void SetImpl(GraphManager *impl) {
    impl_ = impl;
  }

 private:
  GraphManager *impl_ = nullptr;
};
}  // namespace ge

#endif  // INC_FRAMEWORK_ENGINE_DNNENGINE_H_
