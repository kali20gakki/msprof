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

#include "graph/passes/start_of_sequence_pass.h"

#include "common/plugin/ge_util.h"
#include "common/profiling/profiling_properties.h"

namespace ge {
Status StartOfSequencePass::Run(ComputeGraphPtr graph) {
  if (graph->GetParentGraph() != nullptr) {
    return SUCCESS;
  }
  const bool is_profiling =
      ProfilingProperties::Instance().ProfilingOn() || ProfilingProperties::Instance().ProfilingTrainingTraceOn();
  if (!is_profiling) {
    GELOGD("Profiling is not open.");
    return SUCCESS;
  }

  GELOGI("StartOfSequencePass begin.");
  std::vector<NodePtr> target_nodes;
  for (const NodePtr &node : graph->GetDirectNode()) {
    if (node->GetInNodes().empty()) {
      target_nodes.push_back(node);
    }
  }

  const OpDescPtr op_desc = MakeShared<OpDesc>("StartOfSequence", STARTOFSEQUENCE);
  GE_CHECK_NOTNULL(op_desc);

  const NodePtr start_of_sequence = graph->AddNode(op_desc);
  GE_CHECK_NOTNULL(start_of_sequence);

  // Insert control edge
  for (const auto &node : target_nodes) {
    GELOGI("Add ctrl edge between %s and %s", start_of_sequence->GetName().c_str(), node->GetName().c_str());
    if (GraphUtils::AddEdge(start_of_sequence->GetOutControlAnchor(), node->GetInControlAnchor()) != GRAPH_SUCCESS) {
      REPORT_CALL_ERROR("E19999", "Add ctrl edge between %s and %s failed", start_of_sequence->GetName().c_str(),
                        node->GetName().c_str());
      GELOGE(FAILED, "[Add][CtrlEdge] between %s and %s failed", start_of_sequence->GetName().c_str(),
             node->GetName().c_str());
      return FAILED;
    }
  }

  GELOGI("StartOfSequencePass end.");
  return SUCCESS;
}
}  // namespace ge
