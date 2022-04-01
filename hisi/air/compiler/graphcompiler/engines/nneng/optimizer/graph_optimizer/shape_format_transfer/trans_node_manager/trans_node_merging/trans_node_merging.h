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

#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_SHAPE_FORMAT_TRANSFER_TRANS_NODE_MANAGER_TRANS_NODE_MERGING_TRANS_NODE_MERGING_H_
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_SHAPE_FORMAT_TRANSFER_TRANS_NODE_MANAGER_TRANS_NODE_MERGING_TRANS_NODE_MERGING_H_

#include <stack>
#include "common/fe_inner_error_codes.h"
#include "graph/compute_graph.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/utils/graph_utils.h"

namespace fe {

template <class T>
using Vistor = RangeVistor<T, std::shared_ptr<ge::ConstAnchor>>;

struct BasicInfoForRemovingNode {
  ge::NodePtr src_node;
  ge::OpDescPtr src_op_desc;
  ge::NodePtr dst_node;
  ge::OpDescPtr dst_op_desc;
  ge::NodePtr node;
  ge::InDataAnchorPtr dst_anchor;
  ge::OutDataAnchorPtr src_anchor;
  ge::InDataAnchorPtr in_anchor_of_node;
  ge::OutDataAnchorPtr out_anchor_of_node;
  size_t dst_in_anchor_size_of_node;
};

/** @brief Class of merging all reversed trans-nodes
* pairs. */
class TransNodeMerging {
 public:
  explicit TransNodeMerging();

  ~TransNodeMerging();

  TransNodeMerging(const TransNodeMerging &) = delete;

  TransNodeMerging &operator=(const TransNodeMerging &) = delete;

  Status MergeAllTransOps(ge::ComputeGraph &fused_graph);

 private:
  /* Remove edge between src_anchor and dst Anchor. Then, add two edges which are
   * frome src_anchor to new_node and from new_node to dst_anchor. */
  Status AddEdgesForNewNode(ge::OutDataAnchorPtr src_anchor, ge::InDataAnchorPtr dst_anchor, ge::NodePtr new_node);

  Status MergeOneNode(ge::ComputeGraph &fused_graph, ge::NodePtr node, const uint32_t &current_in_anchor_index,
                            std::stack<uint32_t> &in_anchor_index_stack);

  Status MergeTransOpBetweenTwoNormalOp(ge::ComputeGraph &fused_graph, ge::NodePtr src_node,
                                        const std::string &normal_op_type, const string &normal_op_name,
                                        const ge::InDataAnchorPtr &in_anchor);
  uint32_t FindIndexOfCurrentNode(const Vistor<ge::InDataAnchorPtr> &in_data_anchor_ptr_vec,
                                  const ge::InDataAnchorPtr &in_anchor) const;
};
}  // namespace fe
#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_SHAPE_FORMAT_TRANSFER_TRANS_NODE_MANAGER_TRANS_NODE_MERGING_TRANS_NODE_MERGING_H_
