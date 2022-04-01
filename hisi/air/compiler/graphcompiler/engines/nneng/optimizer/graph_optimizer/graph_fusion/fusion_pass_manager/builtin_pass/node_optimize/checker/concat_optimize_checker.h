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

#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_GRAPH_FUSION_FUSION_PASS_MANAGER_BUILTIN_PASS_NODE_OPTIMIZE_CHECKER_CONCAT_OPTIMIZE_CHECKER_H_
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_GRAPH_FUSION_FUSION_PASS_MANAGER_BUILTIN_PASS_NODE_OPTIMIZE_CHECKER_CONCAT_OPTIMIZE_CHECKER_H_
#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/node_optimize/checker/node_optimize_checker_base.h"

namespace fe {
const size_t CONCAT_SHAPE_DIM_DEFAULT = 4;
class ConcatOptimizeChecker : public NodeOptimizeCheckerBase {
 public:
  bool Check(const ge::NodePtr &node_ptr) const;
  bool CheckWithQuant(const ge::NodePtr &node_ptr) const;
  bool IsDimCAlignedWithQuant(const ge::NodePtr &node_ptr) const;

 private:
  /**
   * Check if all inputs comes from the same node.
   * @param node_ptr node
   * @return true or false
   */
  bool IsInputFromSameNode(const ge::NodePtr &node_ptr) const;

  /**
   * Check if the dim C of all inputs is aligned by 16(float16), 32(int8) or 64(int4).
   * @param node_ptr node
   * @return true or false
   */
  bool IsDimCAligned(const ge::NodePtr &node_ptr) const;

  /**
   * Check if the previous node is valid.
   * @param node_ptr node
   * @return true or false
   */
  bool is_pre_node_valid(const ge::NodePtr &node_ptr) const;

  bool is_next_node_valid(ge::NodePtr concat_node, uint32_t depth, bool has_relu) const;

  bool IsDCorMDC() const;

  bool IsNCHWOrNHWC(const ge::NodePtr &node_ptr) const;

  const string LEAKYRELU = "LeakyRelu";
};
}  // namespace fe
#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_GRAPH_FUSION_FUSION_PASS_MANAGER_BUILTIN_PASS_NODE_OPTIMIZE_CHECKER_CONCAT_OPTIMIZE_CHECKER_H_
