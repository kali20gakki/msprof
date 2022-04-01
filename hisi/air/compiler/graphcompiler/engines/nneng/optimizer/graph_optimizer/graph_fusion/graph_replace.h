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

#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_GRAPH_FUSION_GRAPH_REPLACE_H_
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_GRAPH_FUSION_GRAPH_REPLACE_H_

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "common/op_info_common.h"
#include "fusion_rule_manager/fusion_rule_data/fusion_rule_pattern.h"
#include "graph/compute_graph.h"
#include "graph_optimizer/fusion_common/fusion_statistic_recorder.h"
#include "graph_optimizer/graph_fusion/graph_matcher.h"
#include "ops_kernel_store/fe_ops_kernel_info_store.h"

namespace fe {
using std::map;
using std::set;
using std::string;
using std::vector;
class GraphReplace {
 public:
  explicit GraphReplace(shared_ptr<FEOpsKernelInfoStore> ops_kernel_info_store_ptr);
  ~GraphReplace();
  /*
   * @ingroup fe
   * @brief   replace the subgraph before fusion with the subgraph after fusion
   * @param   [in] match_results, sub_graphs pre-fusion, which match the
   * fusion_rule_patter
   * @param   [in] fusion_rule_pattern, fusion rule
   * @param   [in/out] graph, origin compute graph
   * @return  SUCCESS, GRAPH_REPLACE_CREATE_FUSION_NODES_FAILED
   *          GRAPH_REPLACE_UPDATE_ATTR_FAILED,
   * GRAPH_REPLACE_REPLACE_INPUT_FAILED GRAPH_REPLACE_DELETE_NODE_FAILED,
   * GRAPH_REPLACE_REPLACE_OUTPUT_FAILED
   */
  Status ReplaceGraph(vector<GraphMatchResult> &match_results, const FusionRulePattern &fusion_rule_pattern,
                      ge::ComputeGraph &graph);

 private:
  GraphReplace(const GraphReplace &) = delete;
  GraphReplace &operator=(const GraphReplace &) = delete;
  /*
   * @ingroup fe
   * @brief   create fusion Nodes
   * @param   [in] fusion_rule_pattern, fusion rule
   * @param   [in] origin_sub_graph, subgraph with fusion rule node corresponds to
   * pre-fusion node
   * @param   [out] fusion_nodes, fusion subgraph with fusion rule node
   * corresponds to fusion node
   * @param   [in/out] graph, origin graph
   * @return  SUCCESS or GRAPH_REPLACE_CREATE_FUSION_NODES_FAILED
   */
  Status CreateFusionNodes(const FusionRulePattern &fusion_rule_pattern,
                           const map<FusionRuleNodePtr, ge::NodePtr> &origin_sub_graph,
                           map<FusionRuleNodePtr, ge::NodePtr> &fusion_graph, ge::ComputeGraph &graph) const;
  /*
   * @ingroup fe
   * @brief   update node's attr value
   * @param   [in] origin_sub_graph, subgraph with fusion rule node corresponds to
   * pre-fusion node
   * @param   [out] fusion_sub_graph, fusion subgraph with fusion rule node
   * corresponds to fusion node
   * @return  SUCCESS or GRAPH_REPLACE_UPDATE_ATTR_FAILED
   */
  Status UpdateAttr(const map<FusionRuleNodePtr, ge::NodePtr> &origin_sub_graph,
                    const map<FusionRuleNodePtr, ge::NodePtr> &fusion_sub_graph) const;
  /*
   * @ingroup fe
   * @brief   update node's special attr value
   * @param   [in] origin_sub_graph, subgraph with fusion rule node corresponds to
   * pre-fusion node
   * @param   [out] fusion_sub_graph, fusion subgraph with fusion rule node
   * corresponds to fusion node
   * @return  SUCCESS or GRAPH_REPLACE_UPDATE_ATTR_FAILED
   */
  Status UpdateSpecialAttr(const map<FusionRuleNodePtr, ge::NodePtr> &origin_sub_graph,
                           const map<FusionRuleNodePtr, ge::NodePtr> &fusion_sub_graph) const;

  /**
   * Record fusion nodes for DFX function
   * @param fusion_graph fusion result nodes will be recorded
   * @param match_result match result of a rule and will record fusion nodes
   * @return NA
   */
  void RecordFusionNodes(map<FusionRuleNodePtr, ge::NodePtr> &fusion_graph, GraphMatchResult &match_result) const;

  /*
   * @ingroup fe
   * @brief   replace origin sub_graph with fusion sub_graph
   * @param   [in] match_result, subgraph with fusion rule node corresponds to
   pre-fusion node
   * @param   [in] fusion_sub_graph, fusion subgraph with fusion rule node
   corresponds to fusion node
   * @param   [out] fusion_rule_pattern, fusion rule
   * @param   [int/out] graph, origin graph
   * @return  SUCCESS or GRAPH_REPLACE_REPLACE_INPUT_FAILED、
              GRAPH_REPLACE_REPLACE_OUTPUT_FAILED、
              GRAPH_REPLACE_DELETE_NODE_FAILED
*/
  Status Replace(GraphMatchResult &match_result, const map<FusionRuleNodePtr, ge::NodePtr> &fusion_sub_graph,
                 const FusionRulePattern &fusion_rule_pattern, ge::ComputeGraph &graph) const;

  /**
   * @brief process fusion result, eg. record original name, output anchor map
   * @param match_result one rule match result
   * @return NA
   */
  void PostFusion(const GraphMatchResult &match_result);

  /**
   * @brief Record origin name
   *
   * @param match_result one rule match result
   *
   * @return NA
   */
  void RecordOriginName(GraphMatchResult &match_result);

  /**
   * @brief Record origin ops name
   *
   * @param match_result one rule match result
   *
   * @return NA
   */
  void RecordOriginOpNames(const GraphMatchResult &match_result) const;

  /**
   * @brief Set fusion node output opdesc attr
   *
   * @param match_result one rule match result
   *
   * @return NA
   */
  void SetDataDumpAttr(const GraphMatchResult &match_result) const;

  /*
   * @ingroup fe
   * @brief   create compute graph node
   * @param   [in] fusion_rule_node, fusion rule node
   * @param   [in] node_name, node name
   * @param   [in/out] graph, origin graph
   * @return  NodePtr:fusion node
*/
  ge::NodePtr CreateNode(const FusionRuleNodePtr fusion_rule_node, const string &node_name,
                         ge::ComputeGraph &graph) const;
  /*
   * @ingroup fe
   * @brief   create fusion node name, prevent Name conflict whit in on scope
   * @param   [in] origin_sub_graph, subgraph with fusion rule node corresponds to
   * pre-fusion node
   * @param   [in] fusion_rule_pattern, fusion pattern
   * @param   [in] types, node types
   * @return  string
*/
  string CreateNodeName(const map<FusionRuleNodePtr, ge::NodePtr> &origin_sub_graph,
                        const FusionRulePattern &fusion_rule_pattern, const vector<string> &types) const;
  /*
   * @ingroup fe
   * @brief   using node name of fusion rule to find whether this fusion node is
   * in matched subgraph
   * @param   [in] fusion_rule_node, fusion rule node
   * @param   [in] origin_sub_graph, subgraph with fusion rule node corresponds to
   * pre-fusion node
   * @return  NodePtr, if not find return nullptr
*/
  ge::NodePtr FindSameNode(const FusionRuleNodePtr fusion_rule_node,
                           const map<FusionRuleNodePtr, ge::NodePtr> &origin_sub_graph) const;
  /*
   * @ingroup fe
   * @brief   delete nodes
   * @param   [in] nodes, node to be delete
   * @param   [in] rule_nodes, rule node
   * @param   [in/out] graph, compute graph
   * @return  SUCCESS,FAILED
   */
  Status DeleteNodes(const map<FusionRuleNodePtr, ge::NodePtr> &nodes, const set<FusionRuleNodePtr> &rule_nodes,
                     ge::ComputeGraph &graph) const;
  /*
   * @ingroup fe
   * @brief   replace fusion node input anchors
   * @param   [in] rule_node, fusion rule node corresponds to fusion node of
   * compute graph
   * @param   [in] fusion_node, fusion node of compute graph
   * @param   [in] outer_inputs, outer input anchors corresponds fusion node
   * input Fusion rule anchor
   * @param   [in] fusion_sub_graph, fusion subgraph with fusion rule node
   * corresponds to fusion node
   * @return  SUCCESS, FAILED
   */
  Status ReplaceInputAnchors(const FusionRuleNodePtr &rule_node, ge::NodePtr fusion_node,
                             const map<FusionRuleAnchorPtr, std::set<ge::AnchorPtr>> &outer_inputs,
                             const map<FusionRuleNodePtr, ge::NodePtr> &fusion_sub_graph) const;

  Status ReplaceInputDataAnchors(const FusionRuleNodePtr &rule_node, ge::NodePtr fusion_node,
                                 const map<FusionRuleAnchorPtr, std::set<ge::AnchorPtr>> &outer_inputs,
                                 const map<FusionRuleNodePtr, ge::NodePtr> &fusion_sub_graph) const;

  Status ReplaceInputCtrlAnchors(const FusionRuleNodePtr &rule_node, ge::NodePtr fusion_node,
                                 const map<FusionRuleAnchorPtr, std::set<ge::AnchorPtr>> &outer_inputs,
                                 const map<FusionRuleNodePtr, ge::NodePtr> &fusion_sub_graph) const;
  /*
   * @ingroup fe
   * @brief   replace fusion node output anchors
   * @param   [in] rule_node, fusion rule node corresponds to fusion node of
   * compute graph
   * @param   [in] fusion_node, fusion node of compute graph
   * @param   [in] outer_outputs, outer output anchors corresponds fusion node
   * output Fusion rule anchor
   * @param   [in] fusion_sub_graph, fusion subgraph with fusion rule node
   * corresponds to fusion node
   * @return  SUCCESS, FAILED
   */
  Status ReplaceOutputAnchors(const FusionRuleNodePtr &rule_node, ge::NodePtr fusion_node,
                              const map<FusionRuleAnchorPtr, std::set<ge::AnchorPtr>> &outer_outputs,
                              const map<FusionRuleNodePtr, ge::NodePtr> &fusion_sub_graph) const;
  Status ReplaceOutputDataAnchors(const FusionRuleNodePtr &rule_node, ge::NodePtr fusion_node,
                                  const map<FusionRuleAnchorPtr, std::set<ge::AnchorPtr>> &outer_outputs,
                                  const map<FusionRuleNodePtr, ge::NodePtr> &fusion_sub_graph) const;

  Status ReplaceOutputCtrlAnchors(const FusionRuleNodePtr &rule_node, ge::NodePtr fusion_node,
                                  const map<FusionRuleAnchorPtr, std::set<ge::AnchorPtr>> &outer_outputs,
                                  const map<FusionRuleNodePtr, ge::NodePtr> &fusion_sub_graph) const;
  /*
   * @ingroup fe
   * @brief link node output data anchor with other input anchors
   */
  Status LinkOuterOutputEdges(ge::AnchorPtr src_anchor, const set<ge::AnchorPtr> &outer_anchors) const;
  /*
   * @ingroup fe
   * @brief check if the fusion node supports
   */
  Status CheckFusionNode(GraphMatchResult &match_result,
                         const map<FusionRuleNodePtr, ge::NodePtr> &fusion_nodes);
  /*
   * @ingroup fe
   * @brief establish the edge between the fusion nodes
   */
  bool LinkFusionNode(const map<FusionRuleNodePtr, ge::NodePtr> &fusion_nodes) const;
  /*
   * @ingroup fe
   * @brief Sorting the fusion nodes topology
   */
  bool TopoSortFusionNode(const map<FusionRuleNodePtr, ge::NodePtr> &fusion_nodes,
                          vector<ge::NodePtr> &sort_nodes) const;
  /*
   * @ingroup fe
   * @brief Establish the edge of the fusion nodes and the outer input nodes
   */
  bool LinkOuterInputsEdge(const map<FusionRuleNodePtr, ge::NodePtr> &fusion_nodes,
                           const map<FusionRuleAnchorPtr, std::set<ge::AnchorPtr>> &outer_inputs) const;
  /*
   * @ingroup fe
   * @brief infer the shape, data type and origin foramt of fusion node
   */
  bool InferShapeDtypeAndFormat(const vector<ge::NodePtr> &sort_nodes,
                                const map<ge::NodePtr, FusionRuleNodePtr> &fusion_nodes,
                                const map<FusionRuleAnchorPtr, set<ge::AnchorPtr>> &outer_outputs) const;
  /*
   * @ingroup fe
   * @brief check if the shape and data type supports
   */
  bool CheckSupported(const vector<ge::NodePtr> &sort_nodes);
  /*
   * @ingroup fe
   * @brief check if the shape and data type of fusion node's output is the same
   * as the input of the child node
   */
  bool CheckShapeAndTypeContinuous(const map<FusionRuleAnchorPtr, set<ge::AnchorPtr>> &outer_outputs,
                                   const map<ge::NodePtr, FusionRuleNodePtr> &fusion_nodes,
                                   const vector<ge::NodePtr> &sort_nodes) const;
  bool CheckDataType(ge::OutDataAnchorPtr out_anchor, ge::InDataAnchorPtr peer_in_anchor) const;
  bool CheckShape(ge::OutDataAnchorPtr out_anchor, ge::InDataAnchorPtr peer_in_anchor) const;

 private:
  shared_ptr<FEOpsKernelInfoStore> ops_kernel_info_store_ptr_{nullptr};

  void UpdateOuterInputs(const string &pattern_name, GraphMatchResult &match_result,
                         std::map<FusionRuleAnchorPtr, ge::AnchorPtr> &outer_inputs) const;

  Status UpdateMatchedOuterAnchor(GraphMatchResult &match_result, string &pattern_name) const;
};
}  // namespace fe
#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_GRAPH_FUSION_GRAPH_REPLACE_H_
