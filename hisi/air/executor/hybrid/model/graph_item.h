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

#ifndef GE_HYBRID_MODEL_SUBGRAPH_ITEM_H_
#define GE_HYBRID_MODEL_SUBGRAPH_ITEM_H_

#include "hybrid/model/node_item.h"
#include "hybrid/model/infer/graph_stage_cache.h"


namespace ge {
namespace hybrid {
class GraphItem {
 public:
  GraphItem() = default;
  ~GraphItem();
  Status GroupNodes();
  const std::vector<NodeItem *> &GetAllNodes() const;
  const std::vector<NodeItem *> &GetAllNodes(const int32_t group) const;
  const std::vector<NodeItem *> &GetRootNodes(const int32_t group) const;
  const std::vector<const NodeItem *> &GetInputNodes() const;
  Status GetOutputDescList(std::vector<ConstGeTensorDescPtr> &output_desc_list) const;
  const std::vector<std::pair<const NodeItem *, int32_t>> &GetOutputEdges() const;
  int32_t TotalInputs() const {
    return total_inputs_;
  }

  int32_t TotalOutputs() const {
    return total_outputs_;
  }

  size_t GetNodeSize(const int32_t group) const;

  bool HasCtrlFlowOp() const {
    return has_ctrl_flow_op_;
  }

  const std::string& GetName() const {
    return name_;
  }

  void SetName(const std::string &name) {
    name_ = name;
  }

  size_t NumGroups() const {
    return grouped_node_items_.size();
  }

  const NodeItem *GetOutputNode() const;

  bool IsDynamic() const;
  int32_t GetParentOutputIndex(const size_t index) const;
  const std::vector<int32_t> &GetInputIndexMapping() const;

  GraphStageCache& GetStageCache() const;
 private:
  friend class HybridModelBuilder;
  Status GroupNodes(const std::vector<NodeItem *> &node_items,
                    std::vector<std::vector<NodeItem *>> &grouped_node_items) const;

  std::string name_;
  std::vector<NodeItem *> node_items_;
  std::vector<std::vector<NodeItem *>> grouped_node_items_;
  std::vector<NodeItem *> root_items_;
  std::vector<std::vector<NodeItem *>> grouped_root_items_;
  std::vector<const NodeItem *> input_nodes_;
  const NodeItem *output_node_ = nullptr;
  // <src_node, out_index>
  std::vector<std::pair<const NodeItem *, int32_t>> output_edges_;
  int32_t total_inputs_ = 0;
  int32_t total_outputs_ = 0;

  bool is_dynamic_ = true;
  bool has_ctrl_flow_op_ = false;
  bool is_root_graph_ = false;
  std::vector<int32_t> input_index_mapping_;
  std::vector<int32_t> output_index_mapping_;
  mutable GraphStageCache stage_cache_;
};
}  // namespace hybrid
}  // namespace ge
#endif // GE_HYBRID_MODEL_SUBGRAPH_ITEM_H_
