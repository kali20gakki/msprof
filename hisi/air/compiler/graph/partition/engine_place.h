/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2021. All rights reserved.
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

#ifndef GE_GRAPH_PARTITION_ENGINE_PLACE_H_
#define GE_GRAPH_PARTITION_ENGINE_PLACE_H_

#include <unordered_map>

#include "framework/common/ge_inner_error_codes.h"
#include "graph/compute_graph.h"

namespace ge {
using NodeEngineMap = std::unordered_map<ConstNodePtr, std::string>;

///
/// @ingroup graph/partition
/// @brief Assigned individual DNNEngine to each node in the origin graph
/// @author
///
class EnginePlacer {
 public:
  explicit EnginePlacer(const ComputeGraphPtr &graph) : compute_graph_(graph) {}
  EnginePlacer() = default;
  ~EnginePlacer() = default;

  Status SelectEngine(const NodePtr &node, bool &is_check_support_success);
  Status Run(bool direct_node_flag = true);
  Status AssignCompositeEngine();
  Status ReAssignEngine();

  // Get the unique node-engine map
  const NodeEngineMap &GetNodeEngineMap(bool is_composite_engine_mode) const;

  void SetComputeGraph(const ComputeGraphPtr &compute_graph) { compute_graph_ = compute_graph; }

 private:
  Status Check() const;
  static void UpdateAttrWithOpdesc(const OpDescPtr op_desc, const std::string attr_engine_name,
                                   const std::string attr_kernel_name);
  static void UpdateOpdescWithAttr(const OpDescPtr op_desc, const std::string attr_engine_name,
                                   const std::string attr_kernel_name);

  ComputeGraphPtr compute_graph_;
  NodeEngineMap node_atomic_engine_map_;
  NodeEngineMap node_composite_engine_map_;
};

class EngineReAssignPass {
 public:
  EngineReAssignPass() = default;
  virtual ~EngineReAssignPass() = default;
  virtual Status Run(const ComputeGraphPtr &graph,
                     NodeEngineMap &node_atomic_engine_map,
                     NodeEngineMap &node_composite_engine_map) = 0;
};
}  // namespace ge

#endif  // GE_GRAPH_PARTITION_ENGINE_PLACE_H_
