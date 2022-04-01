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

#include "graph/partition/stage_partition.h"

#include <stack>
#include "framework/common/debug/ge_log.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "framework/common/util.h"
#include "framework/common/types.h"

namespace ge {
namespace {
const std::set<std::string> kSrcNodeTypes = { DATA, AIPPDATA, ANN_DATA };
}

Status StagePartitioner::Partition() {
  GE_CHECK_NOTNULL(root_graph_);
  if (root_graph_->GetParentGraph() != nullptr) {
    return SUCCESS;
  }

  for (const auto &node : root_graph_->GetDirectNode()) {
    auto op_desc = node->GetOpDesc();
    uint32_t level = 0;
    if (!AttrUtils::GetInt(op_desc, ATTR_STAGE_LEVEL, level)) {
      continue;
    }
    if ((kSrcNodeTypes.count(op_desc->GetType()) != 0) && node->GetInAllNodes().empty()) {
      continue;
    }
    GELOGD("original node %s for stage %u", node->GetName().c_str(), level);
    stage_nodes_[level].insert(node);
  }
  if (stage_nodes_.empty()) {
    GELOGI("Graph %s does not set stage_level, it is not_changed.", root_graph_->GetName().c_str());
    return SUCCESS;
  }

  GE_DUMP(root_graph_, "BeforeStagePartition");
  if (SplitStageLevel() != SUCCESS) {
    GELOGE(FAILED, "[Split][GraphStage] for graph %s failed.", root_graph_->GetName().c_str());
    return FAILED;
  }

  if (StagePartition() != SUCCESS) {
    GELOGE(FAILED, "[Stage][Partition] for graph %s failed.", root_graph_->GetName().c_str());
    return FAILED;
  }

  root_graph_->TopologicalSorting([](const NodePtr &a, const NodePtr &b) -> bool {
    uint32_t a_level = UINT32_MAX;
    (void)AttrUtils::GetInt(a->GetOpDesc(), ATTR_STAGE_LEVEL, a_level);
    uint32_t b_level = UINT32_MAX;
    (void)AttrUtils::GetInt(b->GetOpDesc(), ATTR_STAGE_LEVEL, b_level);
    return a_level < b_level;
  });
  if (root_graph_->TopologicalSorting() != GRAPH_SUCCESS) {
    GELOGE(FAILED, "[Call][TopologicalSorting] for graph %s after stage partition failed, "
           "maybe stage_level was not set correctly.", root_graph_->GetName().c_str());
    return FAILED;
  }
  GE_DUMP(root_graph_, "AfterStagePartition");
  return SUCCESS;
}

Status StagePartitioner::SplitStageLevel() {
  std::stack<NodePtr> nodes;
  std::unordered_set<NodePtr> visited_stage_nodes;
  for (auto &stage : stage_nodes_) {
    uint32_t cur_stage_level = stage.first;
    const auto &cur_stage_nodes = stage.second;
    for (const auto &marked_node : cur_stage_nodes) {
      nodes.push(marked_node);
    }
    visited_stage_nodes.clear();
    while (!nodes.empty()) {
      auto node = nodes.top();
      nodes.pop();
      GE_CHECK_NOTNULL(node->GetOpDesc());
      for (const auto &in_node : node->GetInAllNodes()) {
        if (visited_stage_nodes.count(in_node) != 0) {
          continue;
        }
        uint32_t tmp_level = cur_stage_level;
        (void)AttrUtils::GetInt(node->GetOpDesc(), ATTR_STAGE_LEVEL, tmp_level);
        if (tmp_level != cur_stage_level) {
          continue;
        }
        if (!AttrUtils::SetInt(in_node->GetOpDesc(), ATTR_STAGE_LEVEL, cur_stage_level)) {
          REPORT_CALL_ERROR("E19999", "Set Attr %s on node %s failed.",
                            ATTR_STAGE_LEVEL.c_str(), in_node->GetName().c_str());
          GELOGE(INTERNAL_ERROR, "[Set][Attr] %s on node %s failed.",
                 ATTR_STAGE_LEVEL.c_str(), in_node->GetName().c_str());
          return INTERNAL_ERROR;
        }
        GELOGD("Mark stage_level node %s, stage_level=%u", in_node->GetName().c_str(), cur_stage_level);
        if ((kSrcNodeTypes.count(in_node->GetType()) != 0) && in_node->GetInAllNodes().empty()) {
          GELOGD("skip data node %s for stage %u", in_node->GetName().c_str(), cur_stage_level);
          continue;
        }
        nodes.push(in_node);
      }
      visited_stage_nodes.emplace(node);
    }
    for (const auto &node : visited_stage_nodes) {
      stage.second.insert(node);
    }
  }

  return SUCCESS;
}

Status StagePartitioner::StagePartition() {
  for (const auto &stage : stage_nodes_) {
    const std::string &subgraph_name = "Subgraph_Level_" + std::to_string(stage.first);
    const auto &stage_subgraph = GraphUtils::BuildSubgraphWithNodes(root_graph_, stage.second, subgraph_name);
    if (stage_subgraph == nullptr) {
      REPORT_CALL_ERROR("E19999", "Build subgraph %s failed.", subgraph_name.c_str());
      GELOGE(FAILED, "[Build][Subgraph] %s failed.", subgraph_name.c_str());
      return FAILED;
    }
    if (!AttrUtils::SetInt(stage_subgraph, ATTR_STAGE_LEVEL, stage.first)) {
      REPORT_CALL_ERROR("E19999", "Set attr %s on graph %s failed.", ATTR_STAGE_LEVEL.c_str(),
                        stage_subgraph->GetName().c_str());
      GELOGE(FAILED, "[Set][Attr] %s on graph %s failed.", ATTR_STAGE_LEVEL.c_str(), stage_subgraph->GetName().c_str());
      return FAILED;
    }
    const auto &parent_node = stage_subgraph->GetParentNode();
    GE_CHECK_NOTNULL(parent_node);
    if (!AttrUtils::SetInt(parent_node->GetOpDesc(), ATTR_STAGE_LEVEL, stage.first)) {
      REPORT_CALL_ERROR("E19999", "Set attr %s on node %s failed", ATTR_STAGE_LEVEL.c_str(),
                        parent_node->GetName().c_str());
      GELOGE(FAILED, "[Set][Attr] %s on node %s failed", ATTR_STAGE_LEVEL.c_str(), parent_node->GetName().c_str());
      return FAILED;
    }
  }

  return SUCCESS;
}
}  // namespace ge
