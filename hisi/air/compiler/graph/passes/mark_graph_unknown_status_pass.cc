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

#include "graph/passes/mark_graph_unknown_status_pass.h"
#include "graph/utils/node_utils.h"
#include "graph/debug/ge_attr_define.h"

namespace ge {
namespace {
const char *const kOwnerGraphIsUnknown = "OwnerGraphIsUnknown";
}

Status MarkGraphUnknownStatusPass::Run(ComputeGraphPtr graph) {
  GE_CHECK_NOTNULL(graph);
  bool is_unknown_shape = false;
  bool forced_unknown = false;
  bool is_graph_no_tiling = false;
  for (const auto &node : graph->GetDirectNode()) {
    auto desc = node->GetOpDesc();
    bool is_node_no_tiling = false;
    (void)AttrUtils::GetBool(desc, ATTR_NAME_OP_NO_TILING, is_node_no_tiling);
    if (is_node_no_tiling) {
      GELOGD("node [%s] is no tiling, mark known.", node->GetName().c_str());
      is_graph_no_tiling = is_graph_no_tiling || is_node_no_tiling;
      continue;
    }
    GE_CHK_GRAPH_STATUS_RET(ge::NodeUtils::GetNodeUnknownShapeStatus(*node, is_unknown_shape),
                            "[Get][ShapeStatus] of node[%s] failed!", node->GetName().c_str());
    if (is_unknown_shape) {
      break;
    }
    if (AttrUtils::GetBool(desc, ATTR_NAME_FORCE_UNKNOWN_SHAPE, forced_unknown) && forced_unknown) {
      GELOGD("node %s was marked as unknown shape.", node->GetName().c_str());
      is_unknown_shape = true;
      break;
    }
  }

  GE_CHK_BOOL_RET_STATUS(!(is_graph_no_tiling && is_unknown_shape), PARAM_INVALID,
                         "No tiling graph[%s] not support unknown shape node", graph->GetName().c_str());

  const auto &node = graph->GetParentNode();
  if (!is_unknown_shape && !is_graph_no_tiling && node != nullptr && node->GetType() == PARTITIONEDCALL) {
    GE_CHK_GRAPH_STATUS_RET(NodeUtils::GetNodeUnknownShapeStatus(*node, is_unknown_shape),
                            "[Get][ShapeStatus] of node[%s] failed!", node->GetName().c_str());
  }

  for (const auto &sub_node : graph->GetDirectNode()) {
    GELOGD("Set OwnerGraphIsUnknown attr to node[%s]", sub_node->GetName().c_str());
    (void)AttrUtils::SetBool(sub_node->GetOpDesc(), kOwnerGraphIsUnknown, is_unknown_shape);
  }
  graph->SetGraphUnknownFlag(is_unknown_shape);
  GELOGD("mark graph [%s] unknown status success! value is %d", graph->GetName().c_str(), is_unknown_shape);
  return SUCCESS;
}
}  // namespace ge