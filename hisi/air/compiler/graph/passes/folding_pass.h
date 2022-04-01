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


#ifndef GE_GRAPH_PASSES_FOLDING_PASS_H_
#define GE_GRAPH_PASSES_FOLDING_PASS_H_

#include <map>
#include <memory>
#include <vector>

#include "graph/passes/base_pass.h"
#include "inc/kernel.h"

namespace ge {
namespace folding_pass {
shared_ptr<Kernel> GetKernelByType(const NodePtr &node);
bool IsNoNeedConstantFolding(const NodePtr &node);
}

using IndexsToAnchors = std::map<int32_t, std::vector<InDataAnchorPtr>>;

class FoldingPass : public BaseNodePass {
 protected:
  Status Folding(NodePtr &node, std::vector<GeTensorPtr> &outputs);
  bool IsResultSuccess(const Status compute_ret) const;

 private:
  Status AddConstNode(const NodePtr &node,
                      IndexsToAnchors indexes_to_anchors,
                      std::vector<GeTensorPtr> &v_weight);
  Status DealWithInNodes(const NodePtr &node) const;
  Status RemoveNodeKeepingCtrlEdges(const NodePtr &node);
  Status ConnectNodeToInAnchor(const InDataAnchorPtr &in_anchor, const NodePtr &node, int32_t node_index);
};
}  // namespace ge

#endif  // GE_GRAPH_PASSES_FOLDING_PASS_H_
