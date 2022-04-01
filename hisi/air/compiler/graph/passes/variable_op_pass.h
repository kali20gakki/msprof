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

#ifndef GE_GRAPH_PASSES_VARIABLE_OP_PASS_H_
#define GE_GRAPH_PASSES_VARIABLE_OP_PASS_H_
#include <map>
#include <set>
#include "common/transop_util.h"
#include "external/graph/graph.h"
#include "graph/manager/graph_var_manager.h"
#include "graph/manager/util/graph_rebuild_state_ctrl.h"
#include "inc/graph_pass.h"

namespace ge {
struct SameVariable {
    std::string var_name;
    std::set<NodePtr> var_nodes;
};
using SameVarPtr = std::shared_ptr<SameVariable>;

class VariableOpPass : public GraphPass {
 public:
  explicit VariableOpPass(GraphRebuildStateCtrl *ctrl) : var_accelerate_ctrl_(ctrl) {}

  ~VariableOpPass() override = default;

  Status Run(ge::ComputeGraphPtr graph) override;

 private:
  Status DealFusion(const SameVarPtr &same_vars);

  Status CheckVariableRefLegally(const SameVarPtr &same_vars, bool &is_var_ref_legally);

  Status UpdateVarAndRefOutputFormatInfo(const GeTensorDesc &final_output, const ge::NodePtr &node,
                                         const SameVarPtr &same_vars);

  Status GenerateVariableVariableRefMap(const ComputeGraphPtr &compute_graph);

  Status CheckVarAndVarRefAreAlike(const NodePtr &var_node, const NodePtr &var_ref_node,
                                   bool &is_var_and_variable_ref_are_alike) const;

  bool IsOpDescSame(const GeTensorDescPtr &op_desc_a, const GeTensorDescPtr &op_desc_b) const;

  Status CheckTransNodeAreInverse(const NodePtr &node_a, const NodePtr &node_b, bool &is_same) const;

  void CopyVariableFormatDataTypeAndShape(const GeTensorDesc &src_tensor_desc, GeTensorDesc &dst_tensor_desc) const;

  Status CheckSameAndTransOp(const SameVarPtr &same_vars, bool &is_matched, VarTransRoad &fusion_road) const;

  Status CheckIfCouldBeOptimized(const SameVarPtr &same_vars, bool &flag, VarTransRoad &fusion_road);

  Status FusionIfNeed(const SameVarPtr &same_vars, VarTransRoad &fusion_road);

  Status UpdateIOFormatInfo(const GeTensorDesc &final_output, const SameVarPtr &same_vars);
  Status RenewVarDesc(const ge::ComputeGraphPtr &graph) const;
  Status RenewVarDesc(uint64_t session_id, const NodePtr &node, const VarTransRoad &fusion_road) const;

  std::vector<NodePtr> GetRefVars(const SameVarPtr &same_vars);
  std::map<SameVarPtr, std::set<NodePtr>> var_and_var_ref_map_;

  GraphRebuildStateCtrl *var_accelerate_ctrl_;
};
}  // namespace ge
#endif  // GE_GRAPH_PASSES_VARIABLE_OP_PASS_H_
