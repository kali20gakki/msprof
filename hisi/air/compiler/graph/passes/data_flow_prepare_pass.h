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

#ifndef GE_GRAPH_PASSES_DATA_FLOW_PREPARE_PASS_H_
#define GE_GRAPH_PASSES_DATA_FLOW_PREPARE_PASS_H_

#include <unordered_set>
#include <map>
#include "inc/graph_pass.h"

namespace ge {
class DataFlowPreparePass : public GraphPass {
 public:
  Status Run(ge::ComputeGraphPtr graph) override;
 private:
  static void SetMaxSize(const NodePtr &node);
  static Status SetHandle(const std::map<std::string, std::unordered_set<NodePtr>> &data_flow_ops_groups);
  static std::pair<NodePtr, int32_t> GetPeerOutNodeAndIndexByInputIndex(const NodePtr &node, const int32_t input_index);
  static Status GetResourceInputNode(const NodePtr &node, const int32_t in_idx, NodePtr &src_node);
};
}  // namespace ge
#endif  // GE_GRAPH_PASSES_DATA_FLOW_PREPARE_PASS_H_
