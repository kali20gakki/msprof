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

#include "graph/partition/base_cluster.h"

#include <algorithm>
#include <memory>
#include <queue>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>
#include "common/plugin/ge_util.h"
#include "framework/common/debug/ge_log.h"
#include "framework/common/debug/log.h"
#include "framework/common/types.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "common/omg_util.h"
#include "graph/partition/base_partition.h"

#define REQUIRE(cond, ...)                                     \
  do {                                                         \
    if (!(cond)) {                                             \
      REPORT_INNER_ERROR("E19999", __VA_ARGS__);               \
      GELOGE(FAILED, "[partition]" __VA_ARGS__); \
      return FAILED;                                           \
    }                                                          \
  } while (false)

#define REQUIRE_NOT_NULL(cond, ...) REQUIRE(((cond) != nullptr), __VA_ARGS__)
#define REQUIRE_SUCCESS(cond, ...) REQUIRE(((cond) == SUCCESS), __VA_ARGS__)
#define REQUIRE_GRAPH_SUCCESS(cond, ...) REQUIRE(((cond) == GRAPH_SUCCESS), __VA_ARGS__)

namespace {
const char *const CLUSTER_DATA = "DATA";
const char *const CLUSTER_INPUT_NODE = "INPUT_NODE";
const char *const CLUSTER_NETOUTPUT = "NETOUTPUT";
const char *const CLUSTER_STAGE = "STAGE";
const char *const CLUSTER_KNOWN_SHAPE = "KNOWN_SHAPE";
const char *const CLUSTER_UNKNOWN_SHAPE = "UNKNOWN_SHAPE";
}  // namespace

namespace ge {
using ClusterPtr = std::shared_ptr<BaseCluster>;

std::string BaseCluster::DebugString() const {
  std::stringstream ss;
  if (type_index_ < static_cast<int32_t>(partitioner_->type_index_to_type_.size())) {
    ss << partitioner_->type_index_to_type_[type_index_];
  }

  ss << "[" << id_ << "](size:" << nodes_.size() << ")";
  ss << "(" << min_ << "," << max_ << ")(";
  for (const auto &cluster : in_clusters_) {
    ss << cluster->id_ << ",";
  }
  ss << ")->(";
  for (const auto &cluster : out_clusters_) {
    ss << cluster->id_ << ",";
  }
  ss << ")|";
  for (const auto &node : nodes_) {
    ss << (node->GetName() + "|");
  }
  return ss.str();
}

size_t BaseCluster::Id() const { return id_; }
size_t BaseCluster::UniqueId() const { return unique_id_; }
void BaseCluster::UpdateRank(size_t rank) {
  max_ = rank;
  min_ = rank;
};

bool BaseCluster::IsData() const { return partitioner_->type_index_to_type_[type_index_] == CLUSTER_DATA; };
bool BaseCluster::IsKnownShape() const {
  return partitioner_->type_index_to_type_[type_index_] == CLUSTER_KNOWN_SHAPE;
};
bool BaseCluster::IsUnknownShape() const {
  return partitioner_->type_index_to_type_[type_index_] == CLUSTER_UNKNOWN_SHAPE;
};
bool BaseCluster::IsIndependent() const { return partitioner_->type_index_to_type_[type_index_] == CLUSTER_STAGE; };
bool BaseCluster::IsNetOutput() const { return partitioner_->type_index_to_type_[type_index_] == CLUSTER_NETOUTPUT; };
bool BaseCluster::IsInputNode() const { return partitioner_->type_index_to_type_[type_index_] == CLUSTER_INPUT_NODE; };


bool BaseCluster::IsRefVariable() const {
  if ((nodes_.size() == 1) && ((nodes_[0]->GetType() == VARIABLE) || (nodes_[0]->GetType() == VARIABLEV2))) {
    std::string ref_variable_name;
    return (AttrUtils::GetStr(nodes_[0]->GetOpDesc(), REF_VAR_SRC_VAR_NAME, ref_variable_name) &&
            !ref_variable_name.empty());
  }
  return false;
}

void BaseCluster::AddInput(ClusterPtr in) {
  if (std::find(in_clusters_.begin(), in_clusters_.end(), in) != in_clusters_.end()) { return; }
  in_clusters_.insert(in_clusters_.end(), in);
  if (std::find(in->out_clusters_.begin(), in->out_clusters_.end(), shared_from_this()) != in->out_clusters_.end()) {
    return;
  }
  in->out_clusters_.insert(in->out_clusters_.end(), shared_from_this());
};
void BaseCluster::RemoveInput(ClusterPtr in) {
  in_clusters_.erase(std::remove(in_clusters_.begin(), in_clusters_.end(), in), in_clusters_.end());
  in->out_clusters_.erase(std::remove(in->out_clusters_.begin(), in->out_clusters_.end(), shared_from_this()),
                          in->out_clusters_.end());
};
void BaseCluster::AddOutput(ClusterPtr out) {
  if (std::find(out_clusters_.begin(), out_clusters_.end(), out) != out_clusters_.end()) { return; }
  out_clusters_.insert(out_clusters_.end(), out);
  if (std::find(out->in_clusters_.begin(), out->in_clusters_.end(), shared_from_this()) != out->in_clusters_.end()) {
    return;
  }
  out->in_clusters_.insert(out->in_clusters_.end(), shared_from_this());
};
void BaseCluster::RemoveOutput(ClusterPtr out) {
  out_clusters_.erase(std::remove(out_clusters_.begin(), out_clusters_.end(), out), out_clusters_.end());
  out->in_clusters_.erase(std::remove(out->in_clusters_.begin(), out->in_clusters_.end(), shared_from_this()),
                          out->in_clusters_.end());
};

void BaseCluster::Merge(ClusterPtr other) {
  if (other->IsIndependent()) {
    return;
  }
  nodes_.insert(nodes_.end(), other->nodes_.begin(), other->nodes_.end());
  other->in_clusters_.erase(std::remove(other->in_clusters_.begin(), other->in_clusters_.end(), shared_from_this()),
                            other->in_clusters_.end());
  other->out_clusters_.erase(std::remove(other->out_clusters_.begin(), other->out_clusters_.end(), shared_from_this()),
                             other->out_clusters_.end());
  in_clusters_.erase(std::remove(in_clusters_.begin(), in_clusters_.end(), other), in_clusters_.end());
  out_clusters_.erase(std::remove(out_clusters_.begin(), out_clusters_.end(), other), out_clusters_.end());
  auto in_clusters = other->in_clusters_;
  for (const auto &cluster : in_clusters) {
    cluster->RemoveOutput(other);
    cluster->AddOutput(shared_from_this());
  }
  auto out_clusters = other->out_clusters_;
  for (const auto &cluster : out_clusters) {
    cluster->RemoveInput(other);
    cluster->AddInput(shared_from_this());
  }
  if (other->max_ > max_) {
    max_ = other->max_;
  }
  if (other->min_ < min_) {
    min_ = other->min_;
  }

  if (!IsUnknownShape() && other->IsUnknownShape()) {
    type_index_ = other->GetTypeIndex();
  }
}

bool BaseCluster::TryMerge(ClusterPtr other) {
  std::queue<ClusterPtr> forward_reached;
  forward_reached.push(other);

  // Try merge other cluster to this cluster, ONLY if will not leads to a ring
  while (!forward_reached.empty()) {
    auto current_cluster = forward_reached.front();
    forward_reached.pop();
    for (const auto &cluster : current_cluster->out_clusters_) {
      if (cluster->max_ == max_ && current_cluster != other) {
        return false;
      } else if (cluster->min_ < max_) {
        forward_reached.push(cluster);
      }
    }
  }
  Merge(other);
  return true;
};

std::vector<ClusterPtr> BaseCluster::MergeAllPathFrom(ClusterPtr other) {
  std::queue<ClusterPtr> forward_reached_queue;
  std::queue<ClusterPtr> backward_reached_queue;

  std::unordered_set<ClusterPtr> forward_reached_clusters;
  std::unordered_set<ClusterPtr> backward_reached_clusters;
  std::vector<ClusterPtr> path_clusters;
  if (other->IsIndependent()) {
    return path_clusters;
  }

  path_clusters.push_back(other);
  forward_reached_queue.push(other);
  backward_reached_queue.push(shared_from_this());
  while (!forward_reached_queue.empty()) {
    auto current_cluster = forward_reached_queue.front();
    forward_reached_queue.pop();
    for (const auto &cluster : current_cluster->out_clusters_) {
      if (cluster->min_ < max_ && cluster->max_ != max_ && forward_reached_clusters.count(cluster) == 0) {
        forward_reached_clusters.insert(cluster);
        forward_reached_queue.push(cluster);
      }
    }
  }
  while (!backward_reached_queue.empty()) {
    auto current_cluster = backward_reached_queue.front();
    backward_reached_queue.pop();
    for (const auto &cluster : current_cluster->in_clusters_) {
      if (cluster->max_ > other->min_ && cluster->max_ != other->max_ &&
          backward_reached_clusters.count(cluster) == 0) {
        backward_reached_clusters.insert(cluster);
        backward_reached_queue.push(cluster);
        if (forward_reached_clusters.count(cluster) != 0) {
          path_clusters.push_back(cluster);
        }
      }
    }
  }
  for (const auto &cluster : path_clusters) {
    Merge(cluster);
  }
  return path_clusters;
}
std::vector<ClusterPtr> BaseCluster::Inputs() const { return in_clusters_; };
std::vector<ClusterPtr> BaseCluster::Outputs() const { return out_clusters_; };
std::vector<NodePtr> BaseCluster::Nodes() const { return nodes_; };

bool BaseCluster::AddFrameInput(InDataAnchorPtr anchor) {
  bool added = true;
  if (anchor != nullptr && anchor->GetPeerOutAnchor() != nullptr) {
    auto index = inputs_.size();
    if (merge_inputs_) {
      GELOGD("Merge inputs is enabled");
      auto src_node = anchor->GetPeerOutAnchor()->GetOwnerNode();
      std::string src_key = src_node->GetName() + ":" + std::to_string(anchor->GetPeerOutAnchor()->GetIdx());
      std::map<std::string, size_t>::const_iterator it = src_key_to_frame_input_.find(src_key);
      if (it != src_key_to_frame_input_.cend()) {
        index = it->second;
        GELOGD("[%s:%d] Reuse data index: %zu",
               anchor->GetOwnerNode()->GetName().c_str(),
               anchor->GetIdx(),
               it->second);
        added = false;
      } else {
        inputs_.push_back(anchor);
        inputs_index_[anchor] = index;
        GELOGD("[%s:%d] Assign data index: %zu",
               anchor->GetOwnerNode()->GetName().c_str(),
               anchor->GetIdx(),
               index);
        src_key_to_frame_input_[src_key] = index;
      }
    } else {
      inputs_index_[anchor] = index;
      inputs_.push_back(anchor);
    }
    data_to_node_inputs_[index].emplace_back(anchor);
  }
  return added;
}

void BaseCluster::AddFrameOutput(OutDataAnchorPtr anchor) {
  if (anchor != nullptr) {
    outputs_index_[anchor] = outputs_.size();
    outputs_.push_back(anchor);
  }
}

InDataAnchorPtr BaseCluster::GetFrameInDataAnchor(InDataAnchorPtr anchor) {
  return partition_node_->GetInDataAnchor(static_cast<int32_t>(inputs_index_[anchor]));
}

OutDataAnchorPtr BaseCluster::GetFrameOutDataAnchor(OutDataAnchorPtr anchor) {
  return partition_node_->GetOutDataAnchor(static_cast<int32_t>(outputs_index_[anchor]));
}

InControlAnchorPtr BaseCluster::GetFrameInControlAnchor() { return partition_node_->GetInControlAnchor(); };

OutControlAnchorPtr BaseCluster::GetFrameOutControlAnchor() { return partition_node_->GetOutControlAnchor(); };

Status BaseCluster::BuildFrame() {
  if (partitioner_->IsNeedBuildPartitionFrame(*this)) {
    return BuildPartitionFrame();
  } else {
    auto node = nodes_.front();
    auto in_control_anchor = node->GetInControlAnchor();
    if (in_control_anchor != nullptr) {
      for (const auto &peer_out_control_anchor : in_control_anchor->GetPeerOutControlAnchors()) {
        auto src_cluster = partitioner_->node_2_cluster_[peer_out_control_anchor->GetOwnerNode()];
        if (src_cluster->id_ != id_) {
          REQUIRE_GRAPH_SUCCESS(
              GraphUtils::RemoveEdge(peer_out_control_anchor, in_control_anchor),
              "[Remove][Edge] from node %s index %d to node %s failed, index %d.",
              peer_out_control_anchor->GetOwnerNode()->GetName().c_str(), AnchorUtils::GetIdx(peer_out_control_anchor),
              in_control_anchor->GetOwnerNode()->GetName().c_str(), AnchorUtils::GetIdx(in_control_anchor));
          control_inputs_.insert(src_cluster);
          src_cluster->control_outputs_.insert(peer_out_control_anchor);
        }
      }
    }
    if (IsData() || IsIndependent()) {
      for (const auto &anchor : node->GetAllOutDataAnchors()) {
        AddFrameOutput(anchor);
      }
    } else {
      for (const auto &anchor : node->GetAllInDataAnchors()) {
        (void) AddFrameInput(anchor);
      }
    }
    partition_node_ = node;
  }
  return SUCCESS;
}

Status BaseCluster::BuildPartitionFrame() {
  auto graph = partitioner_->root_graph_;
  std::string sub_graph_name = partitioner_->GetSubgraphName(*this);

  subgraph_ = MakeShared<ComputeGraph>(sub_graph_name);
  REQUIRE_NOT_NULL(subgraph_, "[New][Memory] for subgraph failed, name:%s.", sub_graph_name.c_str());
  subgraph_->SetGraphUnknownFlag(IsUnknownShape());
  auto partition_op = MakeShared<OpDesc>("PartitionedCall_" + std::to_string(unique_id_++), "PartitionedCall");
  REQUIRE_NOT_NULL(partition_op, "[New][Memory] for partition op failed.");
  REQUIRE(AttrUtils::SetBool(partition_op, ATTR_NAME_IS_UNKNOWN_SHAPE, IsUnknownShape()),
          "[Set][Attr] _is_unknown_shape flag on partitioned op %s failed.", partition_op->GetName().c_str());

  REQUIRE_GRAPH_SUCCESS(partition_op->AddSubgraphName(subgraph_->GetName()),
                        "[Add][SubgraphName] %s for op:%s.",
                        subgraph_->GetName().c_str(), partition_op->GetName().c_str());
  REQUIRE_GRAPH_SUCCESS(partition_op->SetSubgraphInstanceName(0, subgraph_->GetName()),
                        "[Call][SetSubgraphInstanceName] for op:%s failed, index:0, name:%s.",
                        partition_op->GetName().c_str(), subgraph_->GetName().c_str());
  for (auto &node : nodes_) {
    REQUIRE_NOT_NULL(subgraph_->AddNode(node),
                     "[Add][Node] %s to subgraph:%s failed.", node->GetName().c_str(), subgraph_->GetName().c_str());
    REQUIRE(AttrUtils::SetBool(node->GetOpDesc(), ATTR_NAME_IS_UNKNOWN_SHAPE, IsUnknownShape()),
            "[Set][Attr] %s to op:%s failed.", ATTR_NAME_IS_UNKNOWN_SHAPE.c_str(), node->GetName().c_str());

    REQUIRE_GRAPH_SUCCESS(GraphUtils::RemoveJustNode(graph, node),
                          "[Remove][JustNode] failed, graph:%s, node:%s.",
                          graph->GetName().c_str(), node->GetName().c_str());
    REQUIRE_GRAPH_SUCCESS(node->SetOwnerComputeGraph(subgraph_),
                          "[Set][OwnerComputeGraph] %s for node:%s failed.",
                          subgraph_->GetName().c_str(), node->GetName().c_str());
    for (const auto &anchor : node->GetAllInDataAnchors()) {
      auto peer_out_anchor = anchor->GetPeerOutAnchor();
      if (peer_out_anchor == nullptr) {
        continue;  // Skip overhang input.
      }
      auto src_cluster = partitioner_->node_2_cluster_[peer_out_anchor->GetOwnerNode()];
      if (src_cluster->id_ != id_) {
        if (AddFrameInput(anchor)) {
          REQUIRE_GRAPH_SUCCESS(partition_op->AddInputDesc(node->GetOpDesc()->GetInputDesc(anchor->GetIdx())),
                                "[Add][InputDesc] to op:%s failed.", partition_op->GetName().c_str());
        }
      }
    }
    auto in_control_anchor = node->GetInControlAnchor();
    if (in_control_anchor != nullptr) {
      for (const auto &peer_out_control_anchor : in_control_anchor->GetPeerOutControlAnchors()) {
        if (peer_out_control_anchor == nullptr) {
          continue;
        }
        auto src_cluster = partitioner_->node_2_cluster_[peer_out_control_anchor->GetOwnerNode()];
        if (src_cluster->id_ != id_) {
          REQUIRE_GRAPH_SUCCESS(
              GraphUtils::RemoveEdge(peer_out_control_anchor, in_control_anchor),
              "[Remove][Edge] from %s:%d to %s:%d failed.", peer_out_control_anchor->GetOwnerNode()->GetName().c_str(),
              peer_out_control_anchor->GetIdx(), node->GetName().c_str(), in_control_anchor->GetIdx());
          control_inputs_.insert(src_cluster);
          src_cluster->control_outputs_.insert(peer_out_control_anchor);
        }
      }
    }
    for (const auto &anchor : node->GetAllOutDataAnchors()) {
      auto peer_in_anchors = anchor->GetPeerInDataAnchors();
      for (const auto &peer_in_anchor : peer_in_anchors) {
        auto src_cluster = partitioner_->node_2_cluster_[peer_in_anchor->GetOwnerNode()];
        if (src_cluster->id_ != id_) {
          AddFrameOutput(anchor);
          REQUIRE_GRAPH_SUCCESS(partition_op->AddOutputDesc(node->GetOpDesc()->GetOutputDesc(anchor->GetIdx())),
                                "[Add][OutputDesc] to op:%s failed.", partition_op->GetName().c_str());
          break;
        }
      }
    }
  }
  partition_node_ = graph->AddNode(partition_op);
  REQUIRE_NOT_NULL(partition_node_,
                   "[Add][Node] %s to graph:%s failed.", partition_op->GetName().c_str(), graph->GetName().c_str());
  REQUIRE_GRAPH_SUCCESS(partition_node_->SetOwnerComputeGraph(graph),
                        "[Set][OwnerComputeGraph] %s for node:%s failed.",
                        graph->GetName().c_str(), partition_op->GetName().c_str());
  subgraph_->SetParentNode(partition_node_);
  subgraph_->SetParentGraph(graph);
  REQUIRE_GRAPH_SUCCESS(graph->AddSubgraph(subgraph_),
                        "[Add][Subgraph] %s to root graph:%s failed.",
                        subgraph_->GetName().c_str(), graph->GetName().c_str());
  std::string session_graph_id;
  REQUIRE(AttrUtils::GetStr(*graph, ATTR_NAME_SESSION_GRAPH_ID, session_graph_id),
          "[Get][Attr] %s on root graph:%s failed.", ATTR_NAME_SESSION_GRAPH_ID.c_str(), graph->GetName().c_str());
  REQUIRE(AttrUtils::SetStr(*subgraph_, ATTR_NAME_SESSION_GRAPH_ID, session_graph_id),
          "[Set][Attr] %s on subgraph:%s failed.", ATTR_NAME_SESSION_GRAPH_ID.c_str(), subgraph_->GetName().c_str());
  return SUCCESS;
}

Status BaseCluster::CombinePartitionFrame() {
  size_t index = 0U;
  for (const auto &anchor : inputs_) {
    auto peer_out_anchor = anchor->GetPeerOutAnchor();
    auto src_cluster = partitioner_->node_2_cluster_[peer_out_anchor->GetOwnerNode()];
    auto src_anchor = src_cluster->GetFrameOutDataAnchor(peer_out_anchor);
    auto dst_anchor = GetFrameInDataAnchor(anchor);
    for (const auto &node_anchor : data_to_node_inputs_[index]) {
      REQUIRE_GRAPH_SUCCESS(GraphUtils::RemoveEdge(peer_out_anchor, node_anchor),
                            "[Remove][Edge] from %s:%d to %s:%d fail.",
                            peer_out_anchor->GetOwnerNode()->GetName().c_str(), peer_out_anchor->GetIdx(),
                            node_anchor->GetOwnerNode()->GetName().c_str(), node_anchor->GetIdx());
    }
    REQUIRE_GRAPH_SUCCESS(GraphUtils::AddEdge(src_anchor, dst_anchor), "[Add][Edge] from %s:%d to %s:%d failed.",
                          src_anchor->GetOwnerNode()->GetName().c_str(), src_anchor->GetIdx(),
                          dst_anchor->GetOwnerNode()->GetName().c_str(), dst_anchor->GetIdx());
    ++index;
  }
  for (const auto &src_cluster : control_inputs_) {
    auto src_anchor = src_cluster->GetFrameOutControlAnchor();
    auto dst_anchor = GetFrameInControlAnchor();
    REQUIRE_GRAPH_SUCCESS(GraphUtils::AddEdge(src_anchor, dst_anchor), "[Add][Edge] from %s:%d to %s:%d failed.",
                          src_anchor->GetOwnerNode()->GetName().c_str(), src_anchor->GetIdx(),
                          dst_anchor->GetOwnerNode()->GetName().c_str(), dst_anchor->GetIdx());
  }
  return SUCCESS;
}

Status BaseCluster::BuildPartitionSubgraph() {
  if (IsData() || IsNetOutput() || IsIndependent()) {
    return SUCCESS;
  }
  int64_t parent_node_index = 0;
  for (const auto &anchor : inputs_) {
    GE_CHECK_NOTNULL(subgraph_);
    auto data_op =
        MakeShared<OpDesc>(subgraph_->GetName() + std::string("Data_") + std::to_string(parent_node_index), ge::DATA);
    REQUIRE_NOT_NULL(data_op, "[New][Memory] for data op failed.");
    auto input_desc = anchor->GetOwnerNode()->GetOpDesc()->GetInputDesc(anchor->GetIdx());
    REQUIRE_GRAPH_SUCCESS(data_op->AddInputDesc(input_desc),
                          "[Add][InputDesc] to op:%s failed.", data_op->GetName().c_str());
    REQUIRE_GRAPH_SUCCESS(data_op->AddOutputDesc(input_desc),
                          "[Add][OutputDesc] to op:%s failed.", data_op->GetName().c_str());
    REQUIRE(AttrUtils::SetInt(data_op, ATTR_NAME_PARENT_NODE_INDEX, parent_node_index),
            "[Set][Attr] %s on subgraph data node:%s failed.",
            ATTR_NAME_PARENT_NODE_INDEX.c_str(), data_op->GetName().c_str());
    bool is_unknown_shape = IsUnknownShape();
    REQUIRE(AttrUtils::SetBool(data_op, ATTR_NAME_IS_UNKNOWN_SHAPE, is_unknown_shape),
            "[Set][Attr] %s on data op %s failed.", ATTR_NAME_IS_UNKNOWN_SHAPE.c_str(), data_op->GetName().c_str());
    auto data_node = subgraph_->AddNode(data_op);
    REQUIRE_NOT_NULL(data_node,
                     "[Add][Node] %s to subgraph:%s failed.", data_op->GetName().c_str(), subgraph_->GetName().c_str());
    REQUIRE_GRAPH_SUCCESS(data_node->SetOwnerComputeGraph(subgraph_),
                          "[Set][OwnerGraph] %s of data node:%s failed.",
                          subgraph_->GetName().c_str(), data_op->GetName().c_str());
    for (const auto &node_anchor : data_to_node_inputs_[parent_node_index]) {
      REQUIRE_GRAPH_SUCCESS(GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), node_anchor),
                            "[Call][AddEdge] Failed add data input edge to %s:%d",
                            node_anchor->GetOwnerNode()->GetName().c_str(), node_anchor->GetIdx());
    }
    parent_node_index++;
  }
  if (outputs_.empty() && control_outputs_.empty()) {
    return SUCCESS;
  }

  auto net_output_op = MakeShared<OpDesc>(subgraph_->GetName() + "_" + NODE_NAME_NET_OUTPUT, ge::NETOUTPUT);
  REQUIRE_NOT_NULL(net_output_op, "[New][Memory] for netoutput op failed.");
  bool is_unknown_shape = IsUnknownShape();
  REQUIRE(AttrUtils::SetBool(net_output_op, ATTR_NAME_IS_UNKNOWN_SHAPE, is_unknown_shape),
          "[Set][Attr] %s on op:%s failed.", ATTR_NAME_IS_UNKNOWN_SHAPE.c_str(), net_output_op->GetName().c_str());
  for (size_t i = 0; i < outputs_.size(); ++i) {
    GeTensorDesc input_desc;
    REQUIRE_GRAPH_SUCCESS(net_output_op->AddInputDesc(input_desc),
                          "[Add][InputDesc] to op:%s failed.", net_output_op->GetName().c_str());
  }
  auto net_output_node = subgraph_->AddNode(net_output_op);
  REQUIRE_NOT_NULL(net_output_node,
                   "[Call][AddNode] Failed add netoutput node:%s to subgraph:%s.",
                   net_output_op->GetName().c_str(), subgraph_->GetName().c_str());
  REQUIRE_GRAPH_SUCCESS(net_output_node->SetOwnerComputeGraph(subgraph_),
                        "[Set][OwnerGraph] %s of netoutput node:%s failed.",
                        subgraph_->GetName().c_str(), net_output_node->GetName().c_str());
  parent_node_index = 0;
  for (const auto &anchor : outputs_) {
    auto output_desc = anchor->GetOwnerNode()->GetOpDesc()->GetOutputDesc(static_cast<uint32_t>(anchor->GetIdx()));
    REQUIRE(AttrUtils::SetInt(output_desc, ATTR_NAME_PARENT_NODE_INDEX, parent_node_index),
            "[Set][Attr] parent_node_index on subgraph node:%s netoutput's input failed.",
            anchor->GetOwnerNode()->GetName().c_str());
    REQUIRE_GRAPH_SUCCESS(net_output_op->UpdateInputDesc(parent_node_index, output_desc),
                          "[Update][InputDesc] of netoutput node:%s failed.", net_output_op->GetName().c_str());

    REQUIRE_GRAPH_SUCCESS(GraphUtils::AddEdge(anchor, net_output_node->GetInDataAnchor(parent_node_index)),
                          "[Add][Edge] from %s:%d to netoutput node:%s failed.",
                          anchor->GetOwnerNode()->GetName().c_str(), anchor->GetIdx(),
                          net_output_op->GetName().c_str());
    parent_node_index++;
  }
  for (const auto &anchor : control_outputs_) {
    REQUIRE_GRAPH_SUCCESS(GraphUtils::AddEdge(anchor, net_output_node->GetInControlAnchor()),
                          "[Add][ControlEdge] from %s:%d to netoutput node:%s failed.",
                          anchor->GetOwnerNode()->GetName().c_str(), anchor->GetIdx(),
                          net_output_op->GetName().c_str());
  }
  return SUCCESS;
}

void BaseCluster::Clear() {
  in_clusters_.clear();
  out_clusters_.clear();
  nodes_.clear();
  inputs_index_.clear();
  outputs_index_.clear();
  inputs_.clear();
  outputs_.clear();
  control_inputs_.clear();
  control_outputs_.clear();
  partition_node_.reset();
  subgraph_.reset();
  unique_id_ = 0;
}

void BaseCluster::SetMergeInputs(bool merge_inputs) {
  merge_inputs_ = merge_inputs;
}
}  // namespace ge
