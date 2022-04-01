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

#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_GRAPH_FUSION_FUSION_PASS_MANAGER_BUILTIN_PASS_NODE_OPTIMIZE_CHECKER_SPLIT_OPTIMIZE_CHECKER_H_
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_GRAPH_FUSION_FUSION_PASS_MANAGER_BUILTIN_PASS_NODE_OPTIMIZE_CHECKER_SPLIT_OPTIMIZE_CHECKER_H_

#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/node_optimize/checker/node_optimize_checker_base.h"
namespace fe {
class SplitOptimizeChecker : public NodeOptimizeCheckerBase {
 public:
  bool Check(const ge::NodePtr &node_ptr) const;

 private:
  /**
   * Check if ther ara multiple data outputs.
   * @param node_ptr node
   * @return true or false
   */
  bool IsMultiDataOutputs(const ge::NodePtr &node_ptr) const;

  /**
   * Check if the dim C is aligned by 16(dtype=float16) 32(dtype=int8) or 64(dtype=int4).
   * @param node_ptr node
   * @return true or false
   */
  bool IsDimCAligned(const ge::NodePtr &node_ptr) const;
};
}  // namespace fe

#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_GRAPH_FUSION_FUSION_PASS_MANAGER_BUILTIN_PASS_NODE_OPTIMIZE_CHECKER_SPLIT_OPTIMIZE_CHECKER_H_
