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

#ifndef FUSION_ENGINE_UTILS_COMMON_GRAPH_COMMON_IMPL_H_
#define FUSION_ENGINE_UTILS_COMMON_GRAPH_COMMON_IMPL_H_

#include "common/aicore_util_types.h"
#include "graph/compute_graph.h"
#include "register/graph_optimizer/graph_optimize_register_error_codes.h"

#include <map>
#include <string>
#include <utility>
#include <vector>

namespace fe {
using ScopeNodeMap = std::map<int64_t, std::vector<ge::NodePtr>>;
using k_scope_node_pair_t = std::pair<int64_t, std::vector<ge::NodePtr>>;

class GraphCommImpl {
 public:
  GraphCommImpl();
  virtual ~GraphCommImpl();
  GraphCommImpl(const GraphCommImpl &in) = delete;
  GraphCommImpl &operator=(const GraphCommImpl &in) = delete;

  void AddFusionSrc(const uint32_t &src_op_id, const ge::AnchorPtr &src_anchor, const int32_t &fusion_src_index,
                    const int32_t &fusion_dst_index, std::vector<FusionOpSrc> &exist_fusion_src_list) const;

  ge::NodePtr FindNodeInFusNodeList(const string &node_name, const vector<ge::NodePtr> &fus_nodelist) const;

  Status GetAllInEdgeList(const ge::NodePtr &node, std::vector<std::pair<ge::AnchorPtr, ge::AnchorPtr>> &in_edge_pair,
                          const int32_t &edge_type) const;

  Status GetAllOutEdgeList(const ge::NodePtr &node, std::vector<std::pair<ge::AnchorPtr, ge::AnchorPtr>> &out_edge_pair,
                           const int32_t &edge_type) const;

  Status MergeFusionNodeInputCtrlEdgeList(const ge::NodePtr &fus_node,
                                          vector<FusionDataFlow> &fus_input_ctrl_edge_list) const;

  Status MergeFusionNodeOutputCtrlEdgeList(const ge::NodePtr &fus_node,
                                           const vector<FusionDataFlow> &fus_output_ctrl_edge_list) const;

  void PutEdgeToFusionDataFlowVec(const vector<ge::NodePtr> &fus_nodelist,
                                  const std::pair<ge::AnchorPtr, ge::AnchorPtr> &edge,
                                  const ge::AnchorPtr &fus_node_anchor, const ge::AnchorPtr &edge_node_anchor,
                                  std::vector<FusionDataFlow> &fus_edge_list) const;

  void SaveFusionNode(const uint32_t &scopeid, ScopeNodeMap &fus_node_map, ge::NodePtr node) const;

  void SetFusionEltwiseInputIndex(const ge::NodePtr &fus_node, const std::vector<ge::NodePtr> &fus_nodelist) const;

  bool IsInfusNodeList(const string &nodename, const std::vector<ge::NodePtr> &fus_nodelist) const;

  Status AddEdge(ge::AnchorPtr src_anchor, ge::NodePtr dst_node, ge::AnchorPtr old_dst_anchor) const;
};

}  // namespace fe
#endif  // FUSION_ENGINE_UTILS_COMMON_GRAPH_COMMON_IMPL_H_