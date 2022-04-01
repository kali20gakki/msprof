/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2021. All rights reserved.

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at

 * http://www.apache.org/licenses/LICENSE-2.0

 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

#include "graph/passes/ref_identity_delete_op_pass.h"
#include <map>
#include <stack>
#include "common/transop_util.h"

namespace ge {
Status RefIdentityDeleteOpPass::Run(ComputeGraphPtr graph) {
  GE_CHECK_NOTNULL(graph);
  for (auto &node : graph->GetAllNodes()) {
    if (node->GetType() != REFIDENTITY) {
      continue;
    }
    int32_t input_index = 0;
    NodePtr ref_node = GetRefNode(node, input_index);
    CHECK_FALSE_EXEC(GetRefNode(node, input_index) != nullptr,
                     REPORT_CALL_ERROR("E19999", "Get Ref node of node:%s(%s) failed",
                                       node->GetName().c_str(), node->GetType().c_str());
                     GELOGE(FAILED, "[Get][RefNode] of node:%s(%s) failed",
                            node->GetName().c_str(), node->GetType().c_str());
                     return FAILED);
    CHECK_FALSE_EXEC(DealNoOutputRef(node, graph) == SUCCESS,
                     GELOGE(FAILED, "[Deal][NoOutputRef] for node:%s failed, index:%d",
                            node->GetName().c_str(), input_index);
                     return FAILED);
  }
  return SUCCESS;
}

NodePtr RefIdentityDeleteOpPass::GetRefNode(const NodePtr &node, int32_t &input_index) const {
  OutDataAnchorPtr out_anchor = node->GetOutDataAnchor(0);
  CHECK_FALSE_EXEC(out_anchor != nullptr, return nullptr);
  for (const auto &peer_in_anchor : out_anchor->GetPeerInDataAnchors()) {
    auto peer_node = peer_in_anchor->GetOwnerNode();
    const auto &peer_op_desc = peer_node->GetOpDesc();
    CHECK_FALSE_EXEC(peer_op_desc != nullptr, return nullptr);
    const auto &peer_input_desc = peer_op_desc->GetInputDescPtr(static_cast<uint32_t>(peer_in_anchor->GetIdx()));
    CHECK_FALSE_EXEC(peer_input_desc != nullptr, return nullptr);
    if (!peer_input_desc->GetRefPortIndex().empty()) {
      input_index = peer_in_anchor->GetIdx();
      return peer_node;
    }
  }
  return nullptr;
}

Status RefIdentityDeleteOpPass::DealNoOutputRef(const NodePtr &ref_identity, const ComputeGraphPtr &graph) const {
  // remove ref identity
  if (GraphUtils::IsolateNode(ref_identity, {0}) != GRAPH_SUCCESS) {
    REPORT_CALL_ERROR("E19999", "Isolate op:%s(%s) failed",
                      ref_identity->GetName().c_str(), ref_identity->GetType().c_str());
    GELOGE(INTERNAL_ERROR, "[Isolate][Node] %s, type:%s failed", ref_identity->GetName().c_str(),
           ref_identity->GetType().c_str());
    return FAILED;
  }
  if (GraphUtils::RemoveNodeWithoutRelink(graph, ref_identity) != GRAPH_SUCCESS) {
    REPORT_CALL_ERROR("E19999", "Remove node:%s(%s) without relink in graph:%s failed",
                      ref_identity->GetName().c_str(), ref_identity->GetType().c_str(), graph->GetName().c_str());
    GELOGE(INTERNAL_ERROR, "[Remove][Node] %s, type:%s without relink in graph:%s failed",
           ref_identity->GetName().c_str(), ref_identity->GetType().c_str(), graph->GetName().c_str());
    return FAILED;
  }
  return SUCCESS;
}
}  // namespace ge
