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

#ifndef GE_GRAPH_PASSES_FUSE_DATA_NODES_WITH_COMMON_INPUT_PASS_H_
#define GE_GRAPH_PASSES_FUSE_DATA_NODES_WITH_COMMON_INPUT_PASS_H_

#include <set>
#include <map>
#include <vector>
#include "external/graph/types.h"
#include "inc/graph_pass.h"

namespace ge {
class FuseDataNodesWithCommonInputPass : public GraphPass {
 public:
  Status Run(ge::ComputeGraphPtr graph) override;

 private:
  Status InitNeedFuseNodesInfo(ComputeGraphPtr &graph,
      std::map<ComputeGraphPtr, std::map<OutDataAnchorPtr,
      std::set<uint32_t>>> &subgraphs_to_need_fuse_nodes_info) const;
  Status FuseDataNodes(
      const std::map<ComputeGraphPtr, std::map<OutDataAnchorPtr, std::set<uint32_t>>> &subgraphs_to_need_fuse_nodes_info) const;
};
} // namespace ge
#endif // GE_GRAPH_PASSES_FUSE_DATA_NODES_WITH_COMMON_INPUT_PASS_H_
