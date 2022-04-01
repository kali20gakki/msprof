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

#include "framework/common/util.h"
#include "hybrid/model/graph_item.h"

namespace ge {
namespace hybrid {
namespace {
constexpr int32_t kInvalidOutPutIdx = -1;
}  // namespace
GraphItem::~GraphItem() {
  GELOGD("[%s] GraphItem destroyed.", name_.c_str());
}

const std::vector<NodeItem *> &hybrid::GraphItem::GetAllNodes() const {
  return node_items_;
}

const std::vector<NodeItem *> &GraphItem::GetAllNodes(const int32_t group) const {
  if (group == -1) {
    return GetAllNodes();
  }

  if (group >= static_cast<int32_t>(grouped_node_items_.size())) {
    static std::vector<NodeItem *> empty_nodes;
    return empty_nodes;
  }

  return grouped_node_items_[static_cast<size_t>(group)];
}

const std::vector<NodeItem *> &GraphItem::GetRootNodes(const int32_t group) const {
  if (group == -1) {
    return root_items_;
  }

  if (static_cast<uint32_t>(group) >= grouped_root_items_.size()) {
    static std::vector<NodeItem *> empty_nodes;
    return empty_nodes;
  }

  return grouped_root_items_[static_cast<size_t>(group)];
}

size_t GraphItem::GetNodeSize(const int32_t group) const {
  if (group == -1) {
    return node_items_.size();
  }

  return (static_cast<uint32_t>(group) < grouped_node_items_.size()) ?
          grouped_node_items_[static_cast<size_t>(group)].size() : 0U;
}

const std::vector<const NodeItem *> &GraphItem::GetInputNodes() const {
  return input_nodes_;
}

Status GraphItem::GetOutputDescList(std::vector<ConstGeTensorDescPtr> &output_desc_list) const {
  if (output_node_ == nullptr) {
    return SUCCESS;
  }

  if (is_dynamic_) {
    for (auto &tensor_desc : output_node_->GetOpDesc()->GetAllInputsDescPtr()) {
      output_desc_list.emplace_back(tensor_desc);
    }
  } else {
    for (auto &tensor_desc : output_node_->GetOpDesc()->GetAllOutputsDescPtr()) {
      output_desc_list.emplace_back(tensor_desc);
    }
  }

  return SUCCESS;
}

bool GraphItem::IsDynamic() const {
  return is_dynamic_;
}

const std::vector<int32_t> &GraphItem::GetInputIndexMapping() const {
  return input_index_mapping_;
}

int32_t GraphItem::GetParentOutputIndex(const size_t index) const {
  if (index >= output_index_mapping_.size()) {
    return kInvalidOutPutIdx;
  }

  return output_index_mapping_[index];
}

const NodeItem *GraphItem::GetOutputNode() const {
  return output_node_;
}
const std::vector<std::pair<const NodeItem *, int32_t>> &GraphItem::GetOutputEdges() const {
  return output_edges_;
}

Status GraphItem::GroupNodes(const std::vector<NodeItem *> &node_items,
                             std::vector<std::vector<NodeItem *>> &grouped_node_items) const {
  int32_t curr_group = 0;
  int32_t last_group = INT32_MIN;
  std::set<int32_t> seen_groups;
  for (auto node : node_items) {
    const int32_t group = node->group;
    if (group != last_group) {
      if (seen_groups.find(group) != seen_groups.end()) {
        REPORT_INNER_ERROR("E19999", "Unordered node group found. node:%s(%s), group:%d",
                           node->NodeName().c_str(), node->NodeType().c_str(), group);
        GELOGE(INTERNAL_ERROR,
            "[Find][Group] Unordered node group found. node:%s(%s), group:%d",
            node->NodeName().c_str(), node->NodeType().c_str(), group);
        return INTERNAL_ERROR;
      } else {
        last_group = group;
        (void)seen_groups.insert(group);
        curr_group = static_cast<int32_t>(grouped_node_items.size());
        grouped_node_items.emplace_back(std::vector<NodeItem *>());
      }
    }

    node->group = curr_group;
    GELOGD("Adding node [%s] to group %d", node->NodeName().c_str(), node->group);
    grouped_node_items.back().emplace_back(node);
  }

  return SUCCESS;
}

GraphStageCache& GraphItem::GetStageCache() const {
  return stage_cache_;
}

Status GraphItem::GroupNodes() {
  GE_CHK_STATUS_RET_NOLOG(GroupNodes(node_items_, grouped_node_items_));
  GE_CHK_STATUS_RET_NOLOG(GroupNodes(root_items_, grouped_root_items_));
  return SUCCESS;
}
}  // namespace hybrid
}  // namespace ge
