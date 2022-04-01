/**
 * Copyright 2020-2021 Huawei Technologies Co., Ltd
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

#include "graph/passes/base_pass.h"

#include <queue>
#include <unordered_set>

#include "common/debug/log.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/type_utils.h"

namespace ge {
namespace {
constexpr size_t kMaxOneInNodes = 1000;
// Each iteration, we take about 0.3k memory on the stack, we should change the recursion to loop later
constexpr int32_t kMaxRecursiveDepth = 20;
// All nodes in graph can passed 10 times at most. If passed node size reached more than all_node_size*10,
// which mean abnormal things happen.
constexpr uint64_t kMaxPassNodesSizeMutiple = 10;

void GetAllNodesNoInputEdge(const ComputeGraphPtr &graph,
                            GEPass::GraphLevelState &graph_state) {
  for (auto &node : graph->GetDirectNode()) {
    if (node == nullptr) {
      continue;
    }
    size_t in_nums = node->GetInNodes().size();
    if (in_nums == 0) {
      graph_state.AddNodeToQueueIfNotSeen(node);
    } else if (in_nums > kMaxOneInNodes) {
      graph_state.nodes_last.insert(node);
    }
  }
}

bool AnyNodesIn(const Node::Vistor<NodePtr> &nodes, const std::unordered_set<NodePtr> &nodes_set) {
  return std::any_of(nodes.begin(), nodes.end(), [&nodes_set](const NodePtr &n) {
    return nodes_set.count(n) > 0;
  });
}


bool IsNodeReadyToQueue(const NodePtr &node, GEPass::GraphLevelState &graph_state) {
  if (node == nullptr) {
    GELOGW("node is null");
    return false;
  }
  if (graph_state.nodes_deleted.count(node) > 0) {
    GELOGD("The node %s was deleted before, skip it.", node->GetName().c_str());
    return false;
  }

  if (graph_state.nodes_last.count(node) != 0) {
    return false;
  }

  // all in_node seen && all in_node not suspend
  if (!node->IsAllInNodesSeen(graph_state.nodes_seen)) {
    return false;
  }

  if (graph_state.nodes_suspend.count(node) > 0) {
    GELOGD("The node %s has been added to suspend-iteration nodes list, the iteration of it will be suspend.",
           node->GetName().c_str());
    return false;
  }

  if (AnyNodesIn(node->GetInAllNodes(), graph_state.nodes_suspend)) {
    GELOGD("The node %s has been added to suspend-iteration nodes list, the iteration of it will be suspend.",
           node->GetName().c_str());
    return false;
  }
  return true;
}

void AddNextIterNodes(const NodePtr &cur_node,
                      std::unordered_set<NodePtr> &out_nodes_before_pass,
                      GEPass::GraphLevelState &graph_state) {
  for (auto &node : cur_node->GetOutNodes()) {
    if (node == nullptr) {
      continue;
    }
    if (out_nodes_before_pass.erase(node) == 0) {
      // after pass node, new output node come up
      GELOGI("New output node %s come up after pass %s.",
             node->GetName().c_str(), cur_node->GetName().c_str());
    }

    // all in_node seen && all in_node not suspend
    if (IsNodeReadyToQueue(node, graph_state)) {
      graph_state.AddNodeToQueueIfNotSeen(node);
    }
  }

  //
  for (const auto &node : out_nodes_before_pass) {
    // A-->B-->C  if B was
    // unlink edge may happend, add these node to queue if needed
    if (node->GetInAllNodes().empty() && IsNodeReadyToQueue(node, graph_state)) {
      GELOGI("Node %s may lost from cur node %s, add to queue if not seen.",
             node->GetName().c_str(), cur_node->GetName().c_str());
      graph_state.AddNodeToQueueIfNotSeen(node);
    }
  }
}

void AddImmediateRepassNodesToQueue(const NodePtr &cur_node,
                                    std::unordered_map<NodePtr, std::string> re_pass_imm_nodes_to_pass_names,
                                    GEPass::GraphLevelState &graph_state) {
  for (const auto &node_2_pass_names : re_pass_imm_nodes_to_pass_names) {
    auto imme_repass_node = node_2_pass_names.first;
    if (imme_repass_node == nullptr) {
      GELOGW("Found null immediately re-pass node when executing pass %s on node %s type %s",
             node_2_pass_names.second.c_str(),
             cur_node->GetName().c_str(), cur_node->GetType().c_str());
      continue;
    }
    if (graph_state.nodes_passed.count(imme_repass_node) > 0) {
      GELOGD("The node %s specified by pass %s has been passed, it will repass immediately",
             imme_repass_node->GetName().c_str(), node_2_pass_names.second.c_str());
      graph_state.AddNodeToQueueFront(imme_repass_node);
      continue;
    }
    GELOGW("The node %s specified by pass %s has un-passed, it will not repass immediately",
           node_2_pass_names.first->GetName().c_str(), node_2_pass_names.second.c_str());
  }
}

void AddLastNodesToQueue(GEPass::GraphLevelState &graph_state) {
  for (auto &node : graph_state.nodes_last) {
    if (node->IsAllInNodesSeen(graph_state.nodes_seen)) {
      graph_state.AddNodeToQueueIfNotSeen(node);
    }
  }
  graph_state.nodes_last.clear();
}

void AddResumeNodesToQueue(const std::unordered_map<NodePtr, std::string> &resume_node_2_pass_names,
                           GEPass::GraphLevelState &graph_state) {
  // Now base pass doesnt record the order of suspend & resume, so we dont know which one come first in a node pass.
  // Here if one node pass suspend and resume a node ,consider it resume that node.
  // Better way to record the order, and here suspend or resume in order.
  for (const auto &node_2_pass_names : resume_node_2_pass_names) {
    auto node = node_2_pass_names.first;
    if (graph_state.nodes_suspend.erase(node) > 0) {
      if (graph_state.nodes_seen.count(node.get()) > 0 || node->IsAllInNodesSeen(graph_state.nodes_seen)) {
        graph_state.nodes.push_back(node);
        GELOGD("Node %s has been resumed by pass %s, and add to pass queue",
               node->GetName().c_str(), node_2_pass_names.second.c_str());
      }
    }
  }
}

void PushToRePassIfSeen(const NodePtr &node, const std::pair<std::string, BaseNodePass *> &name_to_pass,
                        std::unordered_set<Node *> &nodes_seen, const std::vector<NodePtr> &nodes_to_re_pass,
                        GEPass::RepassLevelState &rp_state) {
  for (const auto &node_to_re_pass : nodes_to_re_pass) {
    if (node_to_re_pass == nullptr) {
      GELOGW("Found null re-pass node when executing %s on node %s type %s", name_to_pass.first.c_str(),
             node->GetName().c_str(), node->GetType().c_str());
      continue;
    }
    if (nodes_seen.count(node_to_re_pass.get()) > 0 || node_to_re_pass->IsAllInNodesSeen(nodes_seen)) {
      if (rp_state.AddNodeToRepass(node_to_re_pass)) {
        GELOGD("The node %s will be re-pass.", node_to_re_pass->GetName().c_str());
        continue;
      }
      GELOGD("Node %s has been added to repass queue, no need to add again.",  node_to_re_pass->GetName().c_str());
    } else {
      GELOGD("The node %s are not all seen, don't set repass this time", node_to_re_pass->GetName().c_str());
    }
  }
}

void SetFlagOption(NodePassOption option, NamesToPass names_to_pass) {
  for (auto &name_to_pass : names_to_pass) {
    name_to_pass.second->SetOption(option, "");
  }
}

void ClearOption(NamesToPass names_to_pass) {
  for (auto &name_to_pass : names_to_pass) {
    name_to_pass.second->ClearOptions();
  }
}

NodePtr GetParentNodeOnRootGraph(const NodePtr &node) {
  auto top_parent_node = node;
  while (top_parent_node != nullptr) {
    const auto owner_graph = top_parent_node->GetOwnerComputeGraph();
    if (owner_graph == nullptr) {
      return nullptr;
    }
    if (owner_graph->GetParentNode() == nullptr) {
      return top_parent_node;
    }
    top_parent_node = owner_graph->GetParentNode();
  }
  return top_parent_node;
}
}  // namespace

Status BaseNodePass::IsolateAndDeleteNode(NodePtr &node, const std::vector<int32_t> &io_map,
                                          bool is_repass_io_immediately) {
  if (node == nullptr) {
    REPORT_INNER_ERROR("E19999", "Param node is nullptr, check invalid.");
    GELOGE(FAILED, "[Check][Param] parameter node is nullptr.");
    return FAILED;
  }
  GELOGI("Prepare to isolate and delete node, name:%s, type:%s.", node->GetName().c_str(),
         node->GetType().c_str());
  ComputeGraphPtr graph = node->GetOwnerComputeGraph();
  if (graph == nullptr) {
    REPORT_INNER_ERROR("E19999", "The owner graph of node:%s must not be null.", node->GetName().c_str());
    GELOGE(FAILED, "[Get][OwnerComputeGraph] failed, The owner graph of node:%s must not be null.",
           node->GetName().c_str());
    return FAILED;
  }

  is_repass_io_immediately ? AddImmediateRePassNodesWithInOut(node) : AddRePassNodesWithInOut(node);

  if (GraphUtils::IsolateNode(node, io_map) != GRAPH_SUCCESS) {
    REPORT_CALL_ERROR("E19999", "Isolate Node:%s failed", node->GetName().c_str());
    GELOGE(FAILED, "[Isolate][Node] %s failed.", node->GetName().c_str());
    return FAILED;
  }

  if (GraphUtils::RemoveNodeWithoutRelink(graph, node) != SUCCESS) {
    REPORT_CALL_ERROR("E19999", "call RemoveNodeWithoutRelink for node:%s failed.", node->GetName().c_str());
    GELOGE(FAILED, "[Call][RemoveNodeWithoutRelink] for node:%s failed.", node->GetName().c_str());
    return FAILED;
  }

  AddNodeDeleted(node);
  return SUCCESS;
}

Status GEPass::Run(const NamesToPass &names_to_passes) {
  if (graph_ == nullptr || repass_nodes_on_root_graph_ == nullptr) {
    REPORT_INNER_ERROR("E19999", "graph_ or repass_nodes_on_root_graph_ is nullptr, check invalid.");
    GELOGE(INTERNAL_ERROR, "[Check][Param] The graph or repass_nodes_on_root_graph_ is nullptr");
    return INTERNAL_ERROR;
  }

  if (names_to_passes.empty()) {
    GELOGW("No passes input, the GEPass will do nothing");
    return INTERNAL_ERROR;
  }
  for (const auto &name_to_pass : names_to_passes) {
    if (name_to_pass.second == nullptr) {
      GELOGE(INTERNAL_ERROR, "[Check][Param] There is null pointer in passes(%s)", name_to_pass.first.c_str());
      return INTERNAL_ERROR;
    }
  }

  if (depth_ > kMaxRecursiveDepth) {
    // root_graph_ can not be null, checked in the beginning of Run()
    GELOGE(PARAM_INVALID,
        "[Check][Param] The pass for root graph %s will be terminated because too many nesting"
        " levels(%d) of subgraphs, last subgraph is %s",
        root_graph_->GetName().c_str(), depth_, graph_->GetName().c_str());
    return PARAM_INVALID;
  }

  return RunPassesOneGraph(names_to_passes);
  // todo debug mode is on, find first node in topo order which is not passed. and give a warning
}

Status GEPass::AddPassAfterGraphOptimized(const NamesToPass &names_to_passes) {
  pass_after_graph_ = names_to_passes;
  return SUCCESS;
}

void NotifyPassGraphStart(const ComputeGraphPtr &graph, const NamesToPass &names_to_pass) {
  for (auto &name_to_pass : names_to_pass) {
    name_to_pass.second->OnStartPassGraph(graph);
  }
}

Status GEPass::HandleLeakedSuspendNodes(const NamesToPass &names_to_passes, GraphLevelState &graph_state) const {
  std::unordered_map<NodePtr, std::string> resume_nodes_to_pass_names;
  for (auto &name_to_pass : names_to_passes) {
    name_to_pass.second->Init();
    auto ret = name_to_pass.second->OnSuspendNodesLeaked();
    if (ret != SUCCESS) {
      GELOGE(ret, "[Check][Param] Internal error with OnSuspendNodesLeaked on pass %s.",
             name_to_pass.first.c_str());
      return ret;
    }
    for (const auto &resume_node : name_to_pass.second->GetNodesResume()) {
      resume_nodes_to_pass_names[resume_node].append(name_to_pass.first + ",");
    }
  }
  AddResumeNodesToQueue(resume_nodes_to_pass_names, graph_state);
  return SUCCESS;
}

Status GEPass::RunPassesAfterFinishGraph(GraphLevelState &graph_state) {
  if (pass_after_graph_.empty()) {
    return SUCCESS;
  }
  if (graph_->GetParentNode() != nullptr) {
    return SUCCESS;
  }
  std::vector<NodePtr> repass_nodes_next_run;
  for (const auto &pass : pass_after_graph_) {
    pass.second->OnFinishGraph(graph_, repass_nodes_next_run);
    const auto &nodes_deleted_by_pass = pass.second->GetNodesDeleted();
    graph_state.nodes_deleted.insert(nodes_deleted_by_pass.begin(), nodes_deleted_by_pass.end());
    pass.second->Init();
  }

   // add repass node reorded to queue
  std::unordered_set<NodePtr> current_nodes_set;
  for (auto &node : repass_nodes_next_run) {
    // get parent node on root_graph
    auto top_parent_node = GetParentNodeOnRootGraph(node);
    if (top_parent_node == nullptr) {
      GELOGD("Node %s top parent node is null, skip repass.", node->GetName().c_str());
      continue;
    }
    if (current_nodes_set.insert(top_parent_node).second) {
      GELOGD("Add node %s to queue for next run", top_parent_node->GetName().c_str());
      graph_state.AddNodeToQueue(top_parent_node);
    }
  }
  return SUCCESS;
}

Status GEPass::RunPassesOneGraph(const NamesToPass &names_to_passes) {
  GELOGD("Begin to run pass on graph, passes count %zu", names_to_passes.size());
  NotifyPassGraphStart(graph_, names_to_passes);
  GraphLevelState graph_state;
  GetAllNodesNoInputEdge(graph_, graph_state);
  GELOGD("Start points count %zu", graph_state.nodes.size());
  // calculate max pass node_size
  uint64_t node_size_in_graph = static_cast<uint64_t>(graph_->GetDirectNodesSize());
  graph_state.max_pass_node_size = node_size_in_graph;
  if (TypeUtils::CheckUint64MulOverflow(node_size_in_graph, kMaxPassNodesSizeMutiple) == SUCCESS) {
    graph_state.max_pass_node_size = node_size_in_graph * kMaxPassNodesSizeMutiple;
  }
  GELOGD("In graph %s, max pass node size %lu, node size %lu.", graph_->GetName().c_str(),
         graph_state.max_pass_node_size, node_size_in_graph);

  do {
    if (!graph_state.nodes_suspend.empty()) {
      auto ret = HandleLeakedSuspendNodes(names_to_passes, graph_state);
      if (ret != SUCCESS) {
        // log inside upper function
        return ret;
      }
      if (graph_state.nodes.empty()) {
        GELOGE(INTERNAL_ERROR, "There are some suspended nodes leaked and no pass resume them.");
        return INTERNAL_ERROR;
      }
    }
    auto ret = RunPassesGraphRepass(names_to_passes, graph_state);
    if (ret != SUCCESS) {
      return ret;
    }
    if (graph_state.nodes_suspend.empty()) {
      ret = RunPassesAfterFinishGraph(graph_state);
      if (ret != SUCCESS) {
        // log inside upper function
        return ret;
      }
    }
  } while ((!graph_state.nodes_suspend.empty() || !graph_state.nodes.empty()) &&
           (graph_state.passed_node_size < graph_state.max_pass_node_size));

  return SUCCESS;
}

Status GEPass::RunPassesGraphRepass(const NamesToPass &names_to_passes, GraphLevelState &graph_state) {
  RepassLevelState rp_state;
  do {
    for (auto &node : rp_state.nodes_re_pass) {
      if (rp_state.nodes_re_pass_set.count(node) > 0) {
        GELOGD("Add node %s to queue for re-pass", node->GetName().c_str());
        graph_state.AddNodeToQueue(node);
      }
    }
    rp_state.ClearRepass();

    while (!graph_state.nodes.empty() && (graph_state.passed_node_size < graph_state.max_pass_node_size)) {
      auto node = graph_state.PopFront();
      if (graph_state.nodes_deleted.count(node) > 0) {
        GELOGD("The node %s was deleted before, skip it.", node->GetName().c_str());
        continue;
      }
      rp_state.EraseNodeFromRepass(node);
      graph_state.nodes_seen.insert(node.get());

      // collect out nodes before pass
      std::unordered_set<NodePtr> out_nodes_before_pass;
      for (const auto &out_node : node->GetOutNodes()) {
        out_nodes_before_pass.insert(out_node);
      }
      auto ret = RunPassesNodeOnce(node, names_to_passes, graph_state, rp_state);
      if (ret != SUCCESS) {
        GELOGE(ret, "[Process][Passes] on node %s type %s failed, error code:%u", node->GetName().c_str(),
               node->GetType().c_str(), ret);
        return ret;
      }
      AddGlobalImmediateRepassNodeToQueueIfSeen(graph_state);
      AddNextIterNodes(node, out_nodes_before_pass, graph_state);
    }
    AddLastNodesToQueue(graph_state);
  } while ((!rp_state.nodes_re_pass.empty() || !graph_state.nodes.empty()) &&
           (graph_state.passed_node_size < graph_state.max_pass_node_size));

  if (graph_state.passed_node_size >= graph_state.max_pass_node_size) {
    GELOGW("pass nodes size should not come to %ld", graph_state.passed_node_size);
  }
  GELOGD("All passes runs end");
  return SUCCESS;
}

Status GEPass::RunPassesOnSubGraph(const NodePtr &node, const NamesToPass &names_to_passes, bool &has_sub_graph) {
  auto sub_graph_names = node->GetOpDesc()->GetSubgraphInstanceNames();
  has_sub_graph = false;
  for (const auto &name : sub_graph_names) {
    auto graph = root_graph_->GetSubgraph(name);
    if (graph == nullptr) {
      GELOGW("Can not find the sub graph %s from node %s, the pass-process will skip it",
          name.c_str(), node->GetName().c_str());
      continue;
    }
    has_sub_graph = true;
    GELOGI("Begin to run passes on the sub graph %s of node %s", name.c_str(), node->GetName().c_str());
    GEPass pass(graph, root_graph_, repass_nodes_on_root_graph_, depth_ + 1);
    auto ret = pass.Run(names_to_passes);
    if (ret != SUCCESS) {
      GELOGE(ret, "[Run][Passes] for sub graph:%s from node:%s failed", name.c_str(), node->GetName().c_str());
      return ret;
    }
  }
  return SUCCESS;
}

Status GEPass::RunPassesNodeOnce(NodePtr &node, const NamesToPass &names_to_passes,
                                 GraphLevelState &graph_state, RepassLevelState &rp_state) {
  auto ret = RunPassesOnNode(node, names_to_passes, graph_state, rp_state);
  if (ret != SUCCESS) {
    GELOGE(ret, "[Process][Passes] on node %s type %s failed, error code:%u", node->GetName().c_str(),
           node->GetType().c_str(), ret);
    return ret;
  }

  bool has_sub_graph = false;
  ret = RunPassesOnSubGraph(node, names_to_passes, has_sub_graph);
  if (ret != SUCCESS) {
    GELOGE(ret, "[Run][Passes] on the sub graph of node %s failed", node->GetName().c_str());
    return ret;
  }

  if (has_sub_graph) {
    NotifyPassGraphStart(graph_, names_to_passes);
    GELOGD("There are subgraphs on node %s, run passes for for the second time", node->GetName().c_str());
    SetFlagOption(kOptimizeAfterSubGraph, names_to_passes);
    ret = RunPassesOnNode(node, names_to_passes, graph_state, rp_state);
    if (ret != SUCCESS) {
      GELOGE(ret, "[Process][Passes] on node %s type %s failed, error code: %u", node->GetName().c_str(),
             node->GetType().c_str(), ret);
      return ret;
    }

    // There is only one option scene, so set and clear options around the `RunPasses` func.
    // if there are more than one scene to set options, the `ClearOption` function
    // should be called each time at the begin of the iteration
    ClearOption(names_to_passes);
  }
  graph_state.passed_node_size++;
  return SUCCESS;
}

Status GEPass::RunPassesOnNode(NodePtr &node, const NamesToPass &names_to_passes, GraphLevelState &graph_state,
                               RepassLevelState &rp_state) {
  if (node == nullptr) {
    REPORT_INNER_ERROR("E19999", "Param node is nullptr, check invalid.");
    GELOGE(FAILED, "[Check][Param] parameter node is nullptr.");
    return FAILED;
  }
  GELOGD("Begin to run pass for node %s", node->GetName().c_str());
  for (const auto &name_to_pass : names_to_passes) {
    GELOGD("Begin to run pass %s for node %s", name_to_pass.first.c_str(), node->GetName().c_str());
    name_to_pass.second->Init();
    auto result = name_to_pass.second->Run(node);
    if (result != SUCCESS) {
      REPORT_CALL_ERROR("E19999", "process pass %s on node:%s failed, ret:%u",
                        name_to_pass.first.c_str(), node->GetName().c_str(), result);
      GELOGE(INTERNAL_ERROR, "[Process][Pass] %s on node %s failed, result "
                             "%u, the passes will be terminated immediately.",
             name_to_pass.first.c_str(), node->GetName().c_str(), result);
      return result;
    }
    if (name_to_pass.second->GetNodesDeleted().count(node) > 0) {
      GELOGD("The node %s was deleted by pass %s, stop the remain passes", node->GetName().c_str(),
             name_to_pass.first.c_str());
      break;
    }
  }
  graph_state.nodes_passed.insert(node);
  std::unordered_map<NodePtr, std::string> re_pass_imm_nodes_to_pass_names;
  std::unordered_map<NodePtr, std::string> resume_nodes_to_pass_names;
  // if muti psss repass one same node, it will add to queue many times, so collect and duplicate
  for (const auto &name_to_pass : names_to_passes) {
    PushToRePassIfSeen(node, name_to_pass, graph_state.nodes_seen,
                       name_to_pass.second->GetNodesNeedRePass(), rp_state);
    // collect imm_node && resume_node among these passes
    for (const auto &imm_node : name_to_pass.second->GetNodesNeedRePassImmediately()) {
      re_pass_imm_nodes_to_pass_names[imm_node].append(name_to_pass.first + ",");
    }
    for (const auto &resume_node : name_to_pass.second->GetNodesResume()) {
      resume_nodes_to_pass_names[resume_node].append(name_to_pass.first + ",");
    }
    for (const auto &suspend_node : name_to_pass.second->GetNodesSuspend()) {
      GELOGD("The iteration suspend of node %s has been set by pass %s", suspend_node->GetName().c_str(),
             name_to_pass.first.c_str());
      graph_state.nodes_suspend.insert(suspend_node);
    }
    const auto &nodes_deleted_by_pass = name_to_pass.second->GetNodesDeleted();
    graph_state.nodes_deleted.insert(nodes_deleted_by_pass.begin(), nodes_deleted_by_pass.end());
    // add global repass node
    for (const auto &global_repass_node : name_to_pass.second->GetGlobalNodesNeedRePassImmediately()) {
      if (repass_nodes_on_root_graph_->AddNodeToRepass(global_repass_node)) {
        GELOGD("The global node %s immediate repass triggered node %s when execute pass %s.",
               global_repass_node->GetName().c_str(), node->GetName().c_str(), name_to_pass.first.c_str());
      }
    }
  }
  AddImmediateRepassNodesToQueue(node, re_pass_imm_nodes_to_pass_names, graph_state);
  AddResumeNodesToQueue(resume_nodes_to_pass_names, graph_state);
  return SUCCESS;
}

void GEPass::AddGlobalImmediateRepassNodeToQueueIfSeen(GraphLevelState &graph_state) const {
  // if not pass on root graph ,just return
  if (!IsCurrentPassRootGraph()) {
    return;
  }
  // now its pass on root graph
  for (auto &repass_node : repass_nodes_on_root_graph_->nodes_re_pass) {
    if (IsNodeReadyToQueue(repass_node, graph_state)) {
      graph_state.AddNodeToQueueFront(repass_node);
    }
  }
  repass_nodes_on_root_graph_->ClearRepass();
}
}  // namespace ge
