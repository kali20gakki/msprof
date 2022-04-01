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

#ifndef GE_GRAPH_PASSES_VARIABLE_PREPARE_OP_PASS_H_
#define GE_GRAPH_PASSES_VARIABLE_PREPARE_OP_PASS_H_

#include <map>
#include <stack>
#include <string>

#include "framework/common/ge_inner_error_codes.h"
#include "inc/graph_pass.h"

namespace ge {
class VariablePrepareOpPass : public GraphPass {
 public:
  Status Run(ge::ComputeGraphPtr graph);

 private:
  Status DealVariableNode(ge::NodePtr &var_node);
  Status DealWritableNode(const ge::NodePtr &writable_node, int32_t input_index, int32_t output_index,
                          const ge::NodePtr &var_node);
  Status GetPeerNodeOfRefOutput(const ge::NodePtr &node, int32_t output_index,
                                std::stack<std::pair<NodePtr, std::pair<int32_t, int32_t>>> &nodes);
  Status AddVariableRef(ge::NodePtr &final_writable_node, const ge::NodePtr &var_node, int32_t index) const;
  Status InsertVariableRef(ge::NodePtr &node, int32_t in_index, const ge::NodePtr &var_node) const;
  Status AddControlEdge(const ge::NodePtr &node, const ge::NodePtr &variable_ref_node) const;
  NodePtr CreateVariableRef(const std::string &variable_ref_name, const ge::NodePtr &var_node) const;
  NodePtr CreateRefIdentity(const std::string &ref_identity_name, const ge::NodePtr &node, uint32_t input_index) const;
  void GetWritableNodeOutIndex(const NodePtr &node, int32_t input_index, std::vector<int32_t> &output_indexes);
  void GenerateRefTypeAndInputOutputMap(const NodePtr &node);
  void FindRefOutIndex(const std::string &node_type, int32_t input_index,
                       const std::map<std::string, std::map<int32_t, std::vector<int32_t>>> &ref_map,
                       std::vector<int32_t> &output_indexes) const;
  Status CheckStreamLabel(const ge::NodePtr &var_ref_node, const ge::NodePtr &final_writable_node) const;
  bool HasControlOut(const ge::NodePtr &node) const;

  std::map<std::string, std::map<int32_t, std::vector<int32_t>>> ref_input_output_map_;
  static std::map<std::string, std::map<int32_t, std::vector<int32_t>>> ref_node_without_prototype_map_;
};
}  // namespace ge

#endif  // GE_GRAPH_PASSES_VARIABLE_PREPARE_OP_PASS_H_
