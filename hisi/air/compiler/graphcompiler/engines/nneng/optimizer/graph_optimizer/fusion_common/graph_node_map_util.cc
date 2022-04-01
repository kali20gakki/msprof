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

#include "graph_optimizer/fusion_common/graph_node_map_util.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "common/fe_log.h"
#include "common/fe_utils.h"
#include "common/op_info_common.h"
#include "graph/compute_graph.h"

namespace fe {
Status GraphNodeMapUtil::CreatNodeOpTypeMap(NodeMapInfoPtr &node_map_info, ge::ComputeGraph &graph) {
  FE_TIMECOST_START(createmap);
  NodeTypeMapPtr node_map = nullptr;
  FE_MAKE_SHARED((node_map = std::make_shared<NodeTypeMap>()), return FAILED);

  string op_type;
  for (auto &node_ptr : graph.GetDirectNode()) {
    op_type = GetRealNodeType(node_ptr->GetOpDesc());
    GraphPassUtil::AddNodeToNodeTypeMap(node_map, op_type, node_ptr);
  }
  FE_MAKE_SHARED((node_map_info = std::make_shared<NodeMapInfo>()), return FAILED);
  *node_map_info = {0, node_map};

  for (auto &iter : *node_map) {
    FE_LOGD("type:%s, nodenum:%zu", iter.first.c_str(), iter.second.size());
  }
  FE_TIMECOST_END(createmap, "create node_type_map");
  return SUCCESS;
}

void GraphNodeMapUtil::DelNodeFromOpTypeMap(NodeMapInfoPtr &node_map_info, ge::NodePtr &node_ptr) {
  if (node_map_info == nullptr || node_ptr == nullptr) {
    return;
  }
  NodeTypeMapPtr node_type_map = node_map_info->node_type_map;
  string real_op_type = GetRealNodeType(node_ptr->GetOpDesc());
  GraphPassUtil::RemoveNodeFromNodeTypeMap(node_type_map, real_op_type, node_ptr);
}

Status GraphNodeMapUtil::SetOpTypeMapToGraph(NodeMapInfoPtr &node_map_info, ge::ComputeGraph &graph) {
  if (!graph.SetExtAttr("NodeMapInfo", node_map_info)) {
    REPORT_FE_ERROR("[GraphOpt][FirstRoundFusion][SetOpTypeMap] set NodeTypeMap failed");
    return FAILED;
  }
  return SUCCESS;
}

Status GraphNodeMapUtil::ClearOpTypeMapToGraph(ge::ComputeGraph &graph) {
  NodeMapInfoPtr node_map_info = nullptr;
  node_map_info = graph.TryGetExtAttr("NodeMapInfo", node_map_info);
  if (node_map_info != nullptr) {
    node_map_info = nullptr;
    if (!graph.SetExtAttr("NodeMapInfo", node_map_info)) {
      REPORT_FE_ERROR("[GraphOpt][FirstRoundFusion][clrOpTypeMap] set NodeTypeMap null failed");
      return FAILED;
    }
  }
  return SUCCESS;
}

Status GraphNodeMapUtil::ReCreateNodeTypeMapInGraph(ge::ComputeGraph &graph) {
  FE_TIMECOST_START(updatemap);
  NodeMapInfoPtr node_map_info = nullptr;
  node_map_info = graph.TryGetExtAttr("NodeMapInfo", node_map_info);
  if (node_map_info == nullptr) {
    FE_LOGW("Can not get node_type_map from graph.");
    return SUCCESS;
  }

  string op_type;
  NodeTypeMapPtr node_type_map_ptr = node_map_info->node_type_map;
  node_type_map_ptr->clear();
  for (auto &node_ptr : graph.GetDirectNode()) {
    op_type = GetRealNodeType(node_ptr->GetOpDesc());
    GraphPassUtil::AddNodeToNodeTypeMap(node_type_map_ptr, op_type, node_ptr);
  }
  FE_TIMECOST_END(updatemap, "update node_type_map");
  return SUCCESS;
}

Status GraphNodeMapUtil::CreatAndSetOpTypeMap(NodeMapInfoPtr &node_map_info, ge::ComputeGraph &graph) {
  NodeMapInfoPtr tmp_node_map_info = nullptr;
  if (CreatNodeOpTypeMap(tmp_node_map_info, graph) == FAILED) {
    return FAILED;
  }

  if (SetOpTypeMapToGraph(tmp_node_map_info, graph) == FAILED) {
    return FAILED;
  }

  node_map_info = tmp_node_map_info;
  return SUCCESS;
}
}  // namespace fe
