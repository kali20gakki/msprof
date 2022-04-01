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

#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_GRAPH_FUSION_GRAPH_MATCHER_H_
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_GRAPH_FUSION_GRAPH_MATCHER_H_
#include <list>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "graph/anchor.h"
#include "graph/compute_graph.h"
#include "graph/node.h"

#include "common/fe_inner_error_codes.h"
#include "fusion_rule_manager/fusion_rule_data/fusion_rule_pattern.h"
#include "graph_optimizer/fusion_common/fusion_statistic_recorder.h"

namespace fe {

// Match result.
struct GraphMatchResult {
  // Matched outer inputs.
  std::map<FusionRuleAnchorPtr, ge::AnchorPtr> outer_inputs;
  std::map<FusionRuleAnchorPtr, ge::AnchorPtr> origin_outer_inputs;
  std::map<FusionRuleAnchorPtr, std::set<ge::AnchorPtr>> outer_inputs_set;

  // Matched original nodes.
  std::map<FusionRuleNodePtr, ge::NodePtr> origin_nodes;
  std::unordered_set<ge::NodePtr> origin_nodes_set;

  // Matched outer outputs.
  std::map<FusionRuleAnchorPtr, std::set<ge::AnchorPtr>> outer_outputs;
  std::map<FusionRuleAnchorPtr, std::set<ge::AnchorPtr>> origin_outer_outputs;

  /** Fusion result graph nodes */
  std::vector<ge::NodePtr> fusion_nodes;
  bool valid_flag = true;
};

// Use a rule to match subgraphs in the compute graph.
class GraphMatcher {
 public:
  GraphMatcher();
  virtual ~GraphMatcher();

  // Use rule to match graph and save the results to match_results.
  Status Match(const FusionRulePattern &rule, const ge::ComputeGraph &graph,
               std::vector<GraphMatchResult> &match_results);

 private:
  using CandidateList = std::list<std::pair<FusionRuleNodePtr, ge::NodePtr>>;

  GraphMatcher(const GraphMatcher &) = delete;
  GraphMatcher &operator=(const GraphMatcher &) = delete;

  FusionRuleNodePtr GetFirstOutputRuleNode() const;

  bool MatchFromOutput(FusionRuleNodePtr output_rule_node, ge::NodePtr output_graph_node,
                       GraphMatchResult &match_result);

  bool MatchOriginNode(FusionRuleNodePtr rule_node, ge::NodePtr graph_node, GraphMatchResult &match_result) const;
  bool MatchOriginNodeInputs(FusionRuleNodePtr rule_node, ge::NodePtr graph_node, GraphMatchResult &match_result);
  bool MatchOriginNodeOutputs(FusionRuleNodePtr rule_node, ge::NodePtr graph_node, GraphMatchResult &match_result);
  bool MatchOutputsForOuterInput(FusionRuleNodePtr rule_node, ge::NodePtr graph_node, GraphMatchResult &match_result);
  bool MatchOriginNodeOutPeers(FusionRuleAnchorPtr rule_anchor, ge::AnchorPtr graph_anchor,
                               GraphMatchResult &match_result);
  bool MatchOriginNodeInPeers(FusionRuleAnchorPtr rule_anchor, ge::AnchorPtr graph_anchor,
                              GraphMatchResult &match_result);
  bool MatchOuterInputOutPeers(FusionRuleAnchorPtr rule_anchor, ge::AnchorPtr graph_anchor,
                               GraphMatchResult &match_result);
  bool MatchInOutPeers(const std::vector<FusionRuleAnchorPtr> &peer_rule_anchors,
                       const ge::Anchor::Vistor<ge::AnchorPtr> &peer_graph_anchors, ge::AnchorPtr graph_origin_anchor,
                       GraphMatchResult &match_result);
  bool MatchPeersWithOuterOutput(const std::vector<FusionRuleAnchorPtr> &peer_rule_anchors,
                                 const ge::Anchor::Vistor<ge::AnchorPtr> &peer_graph_data_anchors,
                                 GraphMatchResult &match_result);
  bool MatchPeer(FusionRuleAnchorPtr peer_rule_anchor, ge::AnchorPtr peer_graph_anchor,
                 ge::AnchorPtr graph_origin_anchor, GraphMatchResult &match_result);

  bool IsNodeTypeMatch(FusionRuleNodePtr rule_node, ge::NodePtr graph_node) const;
  bool IsOuterOutputMatch(FusionRuleAnchorPtr rule_anchor, ge::AnchorPtr graph_anchor,
                          const GraphMatchResult &match_result) const;

  bool IsInOuterInputs(FusionRuleNodePtr rule_node) const;
  bool IsInOuterOutputs(FusionRuleNodePtr rule_node) const;
  bool IsInOriginGraph(FusionRuleNodePtr rule_node) const;
  bool IsInFusionGraph(FusionRuleNodePtr rule_node) const;

  bool IsOuterInput(FusionRuleAnchorPtr anchor) const;
  bool IsOuterOutput(FusionRuleAnchorPtr anchor) const;
  bool IsOriginAnchor(FusionRuleAnchorPtr anchor) const;
  bool IsCtrlAnchorExistMatched(FusionRuleAnchorPtr rule_ctrl_anchor, ge::ControlAnchorPtr graph_ctrl_anchor) const;
  bool CheckOuterInputCtrlAnchor(FusionRuleAnchorPtr rule_ctrl_anchor, ge::ControlAnchorPtr graph_ctrl_anchor) const;
  bool HasOuterOutput(const vector<FusionRuleAnchorPtr> &anchors) const;
  bool HasPeers(const ge::Node::Vistor<ge::OutDataAnchorPtr> &graph_anchors, size_t start_idx) const;

  std::vector<FusionRuleAnchorPtr> GetPeerOriginAnchors(FusionRuleAnchorPtr rule_anchor) const;
  std::vector<FusionRuleAnchorPtr> GetPeerAnchorsNotInFusionGraph(FusionRuleAnchorPtr rule_anchor) const;

  void SeparateOriginAndOuterOutputAnchors(const ge::Anchor::Vistor<ge::AnchorPtr> &peer_graph_anchors,
                                           const std::vector<FusionRuleAnchorPtr> &peer_rule_origin_anchors,
                                           std::map<FusionRuleAnchorPtr, ge::AnchorPtr> &peer_origin_anchor_map,
                                           std::vector<ge::AnchorPtr> &peer_graph_outer_output_anchors) const;
  void AddOuterOutputToResult(FusionRuleAnchorPtr rule_anchor, ge::AnchorPtr graph_anchor,
                              GraphMatchResult &result) const;
  bool RemoveRedundantOuterOutputs(GraphMatchResult &match_result) const;

  bool VerifyMatchResult(const GraphMatchResult &match_result) const;

  Status GetRuleOuterInOutAnchorCount();

  void DumpMatcheResults(const std::vector<GraphMatchResult> &match_results) const;
  void DumpMatchedOuterInputs(const GraphMatchResult &result) const;
  void DumpMatchedOuterOutputs(const GraphMatchResult &result) const;
  void DumpMatchedOriginNodes(const GraphMatchResult &result) const;

 private:
  const FusionRulePattern *rule_{nullptr};
  size_t rule_outer_input_count_{0};
  size_t rule_outer_output_count_{0};
  std::set<ge::NodePtr> prev_matched_origin_nodes_;

  CandidateList candidate_nodes_;
  std::map<FusionRuleAnchorPtr, int> outer_anchor_idxs_;
};

}  // namespace fe
#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_GRAPH_FUSION_GRAPH_MATCHER_H_