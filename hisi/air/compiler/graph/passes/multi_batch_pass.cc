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

#include "graph/passes/multi_batch_pass.h"

#include <stack>
#include <unordered_set>
#include "common/plugin/ge_util.h"
#include "common/omg_util.h"
#include "graph/utils/type_utils.h"
#include "common/formats/utils/formats_trans_utils.h"

namespace ge {
Status MultiBatchPass::Run(ComputeGraphPtr graph) {
  GELOGD("MultiBatchPass Enter");

  if (graph->GetParentGraph() != nullptr) {
    GELOGI("Subgraph %s skip the MultiBatchPass.", graph->GetName().c_str());
    return SUCCESS;
  }
  for (const NodePtr &node : graph->GetDirectNode()) {
    if (node->GetType() == CASE) {
      GE_CHK_STATUS_RET(SetCaseLabel(graph, node),
                        "[Set][CaseLabel] for node:%s(%s) in graph:%s failed",
                        node->GetName().c_str(), node->GetType().c_str(), graph->GetName().c_str());
    }
  }
  return SUCCESS;
}

///
/// @ingroup ge
/// @brief Set batch label for Case mode.
/// @param [in] const ComputeGraphPtr &graph: Root/Case graph.
/// @param [in] const NodePtr &case_node: Case Node.
/// @return 0: SUCCESS / others: FAILED
///
Status MultiBatchPass::SetCaseLabel(const ComputeGraphPtr &graph, const NodePtr &case_node) const {
  const auto &func_desc = case_node->GetOpDesc();
  GE_CHECK_NOTNULL(func_desc);
  if (!func_desc->HasAttr(ATTR_NAME_BATCH_NUM)) {
    GELOGD("Graph: %s Not multi-batch, Node: %s", graph->GetName().c_str(), case_node->GetName().c_str());
    return SUCCESS;
  }

  const auto &dynamic_branch_names = func_desc->GetSubgraphInstanceNames();
  for (size_t i = 0; i < dynamic_branch_names.size(); ++i) {
    const auto &subgraph = graph->GetSubgraph(dynamic_branch_names[i]);
    GE_CHECK_NOTNULL(subgraph);

    const std::string batch_label = "Batch_" + std::to_string(i);
    for (const auto &node : subgraph->GetDirectNode()) {
      (void)AttrUtils::SetStr(node->GetOpDesc(), ATTR_NAME_BATCH_LABEL, batch_label);
    }
  }

  return SUCCESS;
}
}  // namespace ge
