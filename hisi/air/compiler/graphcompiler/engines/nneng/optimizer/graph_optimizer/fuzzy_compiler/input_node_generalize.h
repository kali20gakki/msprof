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

#ifndef COMPILER_GRAPHCOMPILER_ENGINES_NNENG_OPTIMIZER_GRAPH_OPTIMIZER_FUZZY_COMPILER_INPUT_NODE_GENERALIZE_H_
#define COMPILER_GRAPHCOMPILER_ENGINES_NNENG_OPTIMIZER_GRAPH_OPTIMIZER_FUZZY_COMPILER_INPUT_NODE_GENERALIZE_H_
#include <map>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "nlohmann/json.hpp"
#include "graph/compute_graph.h"
#include "graph/utils/type_utils.h"
#include "ops_kernel_store/sub_ops_store.h"
#include "adapter/tbe_adapter/tbe_op_store_adapter.h"

namespace fe {

std::string RangeToString(const std::vector<std::pair<int64_t, int64_t>> &ranges);
std::string ShapeToString(const std::vector<int64_t> &shapes);
void UpdateTensorDesc(const ge::GeTensorDescPtr &src, ge::GeTensorDescPtr &dst);

class InputNodeGeneralize {
 public:
  InputNodeGeneralize(const std::unordered_set<ge::NodePtr> &input_nodes, const bool &is_limited_graph,
                      const std::map<ge::NodePtr, NodeGeneralInfoPtr> &node_info_map, const OpStoreAdapterPtr &op_store_adapter);

  explicit InputNodeGeneralize(const OpStoreAdapterPtr &op_store_adapter);

  ~InputNodeGeneralize();

  Status GeneralizeAllInputNodesInGraph();

  Status GeneralizeOneNode(const ge::NodePtr &node_ptr, const NodeGeneralInfoPtr &node_info_ptr) const;

 private:
  std::vector<ge::ComputeGraphPtr> GetSubgraphsByCurNode(const ge::NodePtr &node_ptr) const;

  Status MergeRangeWithUpperLimitMax(const std::pair<int64_t, int64_t> &upper_limit_max_range,
                                     const std::pair<int64_t, int64_t> &range, const size_t &dim_index,
                                     std::vector<std::pair<int64_t, int64_t>> &dst_shape_range) const;

  Status MergeTensorDesc(const ge::GeTensorDescPtr &src, const ge::GeTensorDescPtr &dst) const;

  Status GetParentNodeBySubGraphNode(const ge::NodePtr &sub_node, ge::NodePtr &parent_node) const;

  Status UpdateSubGraphInputToRootGraph(const std::unordered_set<ge::NodePtr> &sub_graph_input_nodes,
                                        const ge::ComputeGraphPtr &sub_graph) const;

  Status GeneralizeSubGraphs(const ge::NodePtr &root_graph_first_node);

  Status UpdateFirstNodeTensorDescToInputNodes(const ge::NodePtr &first_node);

  Status GeneralizeFirstNodeOfGraph(ge::NodePtr &first_node);

  Status UnlimitedNodeGeneralize(const ge::NodePtr &unlimited_node, const NodeGeneralInfoPtr &node_info_ptr) const;

  Status LimitedNodeGeneralize(const ge::NodePtr &limited_node, const NodeGeneralInfoPtr &node_info_ptr) const;

  Status SetValueDependFlagToInputNodes(const ge::NodePtr &first_node, const NodeGeneralInfoPtr &node_info_ptr) const;

  Status MergeRange(const std::vector<std::pair<int64_t, int64_t>> &src_range,
                    std::vector<std::pair<int64_t, int64_t>> &dst_range) const;

  std::unordered_set<ge::NodePtr> input_nodes_;
  std::unordered_set<ge::NodePtr> prime_nodes_;
  bool is_limited_graph_;
  std::map<ge::NodePtr, NodeGeneralInfoPtr> node_info_map_;
  OpStoreAdapterPtr op_store_adapter_;
};
} // namespace fe
#endif // COMPILER_GRAPHCOMPILER_ENGINES_NNENG_OPTIMIZER_GRAPH_OPTIMIZER_FUZZY_COMPILER_INPUT_NODE_GENERALIZE_H_
