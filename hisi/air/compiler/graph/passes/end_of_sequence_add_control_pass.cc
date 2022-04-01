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

#include "graph/passes/end_of_sequence_add_control_pass.h"

#include <string>
#include <vector>

#include "init/gelib.h"

namespace ge {

Status EndOfSequenceAddControlPass::Run(ComputeGraphPtr graph) {
  if (graph->GetParentGraph() != nullptr) {
    return SUCCESS;
  }
  const auto &end_of_sequence = graph->FindFirstNodeMatchType(ENDOFSEQUENCE);
  if (end_of_sequence == nullptr) {
    return SUCCESS;
  }

  GELOGI("EndOfSequenceAddControlPass begin.");
  std::vector<NodePtr> target_nodes;
  for (NodePtr &node : graph->GetDirectNode()) {
    // op_desc of node should not be null
    if (node->GetOpDesc()->HasAttr(ATTR_NAME_STREAM_LABEL) ||
        DNNEngineManager::GetInstance().IsStreamAssignSkip(node)) {
      continue;
    }
    // Save the nodes whose pre-nodes are all data-like node
    bool flag = false;
    for (const auto &in_node : node->GetInDataNodes()) {
      if (!DNNEngineManager::GetInstance().IsStreamAssignSkip(in_node)) {
        flag = true;
        break;
      }
    }
    if (flag) {
      continue;
    }
    target_nodes.push_back(node);
  }

  // Insert control edge
  for (const auto &node : target_nodes) {
    GELOGI("Add ctrl edge between %s and %s", end_of_sequence->GetName().c_str(), node->GetName().c_str());
    if (GraphUtils::AddEdge(end_of_sequence->GetOutControlAnchor(), node->GetInControlAnchor()) != GRAPH_SUCCESS) {
      REPORT_CALL_ERROR("E19999", "Add ctrl edge between %s and %s failed", end_of_sequence->GetName().c_str(),
                        node->GetName().c_str());
      GELOGE(FAILED, "[Add][CtrlEdge] between %s and %s failed", end_of_sequence->GetName().c_str(),
             node->GetName().c_str());
      return FAILED;
    }
  }

  GELOGI("EndOfSequenceAddControlPass end.");
  return SUCCESS;
}
}  // namespace ge
