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

#ifndef GE_GRAPH_PASSES_SAME_TRANSDATA_BREADTH_FUSION_PASS_H_
#define GE_GRAPH_PASSES_SAME_TRANSDATA_BREADTH_FUSION_PASS_H_

#include <utility>
#include <vector>
#include "inc/graph_pass.h"

namespace ge {
///
/// Transform operators depth fusion
///
class SameTransdataBreadthFusionPass : public GraphPass {
 public:
  SameTransdataBreadthFusionPass() {}
  virtual ~SameTransdataBreadthFusionPass() {}

  graphStatus Run(ComputeGraphPtr graph) override;

 private:
  graphStatus ExtractTransNode(const ComputeGraphPtr &graph);
  graphStatus GetSubGraphsBetweenNormalAndTransdataNode(OutDataAnchorPtr &out_anchor,
      std::vector<std::vector<std::pair<OutDataAnchorPtr, InDataAnchorPtr>>> &sub_graphs_out,
      std::vector<std::pair<OutDataAnchorPtr, InDataAnchorPtr>> &nodes_list);

  void GetSubGraphNodesInfo();

  void EraseInvalidAnchorsPair();
  std::set<std::string> GetInControlIdentityNodes(const NodePtr &node, int32_t subgraph_index);
  OpDescPtr GetCastOp(const GeTensorDesc &in_desc, const GeTensorDesc &out_desc) const;

  graphStatus AddCastNode(const ComputeGraphPtr &graph,
                          int32_t anchors_index,
                          OutDataAnchorPtr &pre_out_anchor,
                          NodePtr &first_link_node);

  void GetSameTransdataNode(std::vector<int32_t> &same_transdata_nodes);

  graphStatus ReLinkTransdataOutput2PreNode(const NodePtr &transdata_node, const OutDataAnchorPtr &pre_out_anchor,
                                            const NodePtr &relink_node) const;

  graphStatus RelinkTransdataControlEdge(ComputeGraphPtr graph,
                                         NodePtr transdata_node_remove,
                                         NodePtr transdata_node_keep);

  graphStatus LinkNewCastNode2RemainTransdata(const ComputeGraphPtr &graph,
                                              const std::vector<int32_t> &same_transdata_nodes,
                                              const OutDataAnchorPtr &transdata_out_anchor,
                                              const NodePtr &transdata_node_keep);

  void UpdateTransdataDesc(const InDataAnchorPtr &transdata_in_anchor, const OpDescPtr &transdata_op_desc,
                           const ConstGeTensorDescPtr &head_output_desc) const;

  graphStatus RelinkRemainTransdata(const ComputeGraphPtr &graph, const std::vector<int32_t> &same_transdata_nodes);

  graphStatus ReLinkTransdataControlOutput2PreNode(const NodePtr &transdata_node_keep,
                                                   const OutDataAnchorPtr &pre_out_anchor,
                                                   const OutControlAnchorPtr &transdata_peer_out_control_anchor) const;

  graphStatus ReuseNodesBeforeTransdata(int32_t anchors_index, const OutDataAnchorPtr &transdata_out_anchor,
                                        NodePtr &relink_node);

  bool AllNodeBeforeTransdataHasOneDataOut(int32_t anchors_index);

  graphStatus RelinkInControlEdgeToDstPeer(const NodePtr &node_src, const NodePtr &node_dst) const;

  graphStatus ReLinkDataOutput2PreNode(const NodePtr &transdata_node,
                                       const OutDataAnchorPtr &pre_out_anchor,
                                       const NodePtr &relink_node) const;

  graphStatus ReLinkOutDataPeerInControlNodes2PreNode(const NodePtr &transdata_node,
                                                      const OutDataAnchorPtr &pre_out_anchor) const;

  void InsertSameTransdataNodeIndex(int32_t anchors_index, std::vector<int32_t> &same_transdata_nodes);

  graphStatus ReLinkOutControlPeerInControlAnchors(const NodePtr &transdata_node_keep,
                                                   const OutDataAnchorPtr &pre_out_anchor,
                                                   const OutControlAnchorPtr &transdata_peer_out_control_anchor) const;

  graphStatus ReLinkOutControlPeerInDataAnchors(const NodePtr &transdata_node_keep,
                                                const OutDataAnchorPtr &pre_out_anchor,
                                                const OutControlAnchorPtr &transdata_peer_out_control_anchor) const;

  void CopyTensorDesc(const ConstGeTensorDescPtr &src_desc, GeTensorDesc &dst_desc) const;

  ///
  /// judge whether an operator is a transform op or not
  /// @param node
  /// @return True or False
  ///
  static bool IsTransOp(const NodePtr &node);

  static bool IsHandleOp(const NodePtr &node);

  static graphStatus CheckTransDataSupported(const NodePtr &node, bool &is_supported);

  static graphStatus CheckAccuracySupported(const OpDescPtr &op_desc, bool &is_supported);

  std::vector<vector<std::pair<OutDataAnchorPtr, InDataAnchorPtr>>> sub_graph_anchors_;
  std::vector<vector<NodePtr>> before_transdata_nodes_;
  std::vector<std::pair<int32_t, InDataAnchorPtr>> all_transdata_nodes_;
  std::vector<vector<NodePtr>> sub_graph_nodes_;
  std::vector<int32_t> transop_num_count_;
  std::vector<bool> sub_graph_has_reshape_node_;
  std::vector<vector<OutControlAnchorPtr>> peer_out_control_anchors_;
  std::vector<vector<InControlAnchorPtr>> peer_in_control_anchors_;
  std::vector<bool> sub_graph_has_control_edge_;
};
}  // namespace ge

#endif  // GE_GRAPH_PASSES_SAME_TRANSDATA_BREADTH_FUSION_PASS_H_
