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
#ifndef GE_GRAPH_PASSES_SWAP_SPACE_PASS_H_
#define GE_GRAPH_PASSES_SWAP_SPACE_PASS_H_

#include "inc/graph_pass.h"
#include "runtime/mem.h"

#include <vector>

namespace ge {
class SwapSpacePass : public GraphPass {
  struct SwapInfo {
    std::string node;
    std::map<int32_t, std::vector<std::string>> output_to_swaps;
  };

 public:
  Status Run(ComputeGraphPtr graph) override;

 private:
  Status GetAllSwapCandidates(const ComputeGraphPtr &graph, std::map<NodePtr, SwapInfo> &swapping_candidates) const;
  Status SwapOutProcess(const ComputeGraphPtr &graph, const std::pair<NodePtr, SwapInfo> &swapping_candidate,
                        std::vector<NodePtr> &d2h_mem_cpy_nodes);
  Status SwapInProcess(const ComputeGraphPtr &graph, const NodePtr &node);
  Status InsertSpaceCopyNodes(const ComputeGraphPtr &graph, const std::map<NodePtr, SwapInfo> &swapping_candidates);
  static Status PrepareForMemAssign(const NodePtr &mem_copy_node, const rtMemcpyKind_t rt_memcpy_kind);
  static Status CheckNodeCouldSwap(const NodePtr &node);
};
}  // namespace ge
#endif  // GE_GRAPH_PASSES_SWAP_SPACE_PASS_H_
