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

#include "graph/partition/dynamic_shape_partition.h"
#include <algorithm>
#include <iostream>
#include <memory>
#include <queue>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>
#include "graph/partition/optimizer/dynamic_data_flow_partitioner_pass.h"
#include "common/plugin/ge_util.h"
#include "framework/common/debug/ge_log.h"
#include "framework/common/debug/log.h"
#include "framework/common/types.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "common/omg_util.h"
#include "graph/partition/base_cluster.h"

#define REQUIRE(cond, ...)                                     \
  do {                                                         \
    if (!(cond)) {                                             \
      REPORT_INNER_ERROR("E19999", __VA_ARGS__);               \
      GELOGE(FAILED, "[Dynamic shape partition]" __VA_ARGS__); \
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
const int32_t kDataTypeIndex = 0;
const int32_t kInputNodeTypeIndex = 1;
const int32_t kNetOutputTypeIndex = 2;
const int32_t kStageTypeIndex = 3;
const int32_t kKnownShapeTypeIndex = 4;
const int32_t kUnknownShapeTypeIndex = 5;
const std::set<std::string> kNotilingOps = {
    ge::MEMCPYASYNC,
    ge::STREAMMERGE,
    ge::PARTITIONEDCALL,
    ge::CASE
};

const std::set<std::string> kExportShapeOps = {
    ge::MEMCPYASYNC,
    ge::STREAMMERGE,
    ge::PARTITIONEDCALL,
    ge::CASE,
    ge::DATA
};
}  // namespace

namespace ge {
namespace {
std::vector<std::vector<int64_t>> SplitMaxShapeStr(const std::string &str) {
  std::vector<std::vector<int64_t>> result;
  std::vector<std::string> tensor_result = StringUtils::Split(str, ';');
  for (const std::string &tensor : tensor_result) {
    std::vector<std::string> dims_result = StringUtils::Split(tensor, ',');
    std::vector<int64_t> tensor_max_shape;
    for (const std::string &dim : dims_result) {
      int64_t max_shape = std::strtol(dim.c_str(), nullptr, 10);
      tensor_max_shape.emplace_back(max_shape);
    }
    result.emplace_back(tensor_max_shape);
  }
  return result;
}

Status ParseShapeRangeAttr(const OpDescPtr &op_desc, bool &has_shape_range_attr,
                           std::vector<std::vector<int64_t>> &max_shape_list) {
  std::string max_shape_str;
  has_shape_range_attr = AttrUtils::GetStr(op_desc, ATTR_NAME_OP_MAX_SHAPE, max_shape_str);
  if (has_shape_range_attr && !max_shape_str.empty()) {
    max_shape_list = SplitMaxShapeStr(max_shape_str);
    if (max_shape_list.size() != op_desc->GetOutputsSize()) {
      GELOGE(PARAM_INVALID, "Op[%s] Invalid shape range attr, shape range size[%zu], op output size[%zu]",
             op_desc->GetName().c_str(), max_shape_list.size(), op_desc->GetOutputsSize());
      has_shape_range_attr = false;
      return PARAM_INVALID;
    }
    for (size_t i = 0; i < max_shape_list.size(); i++) {
      auto tensor = op_desc->GetOutputDescPtr(static_cast<uint32_t>(i));
      if (tensor == nullptr) {
        GELOGE(PARAM_INVALID, "Op[%s] Invalid shape range attr, tensor[%zu] nullptr",
               op_desc->GetName().c_str(), i);
        has_shape_range_attr = false;
        return PARAM_INVALID;
      }
      if (max_shape_list[i].size() != tensor->GetShape().GetDimNum()) {
        GELOGE(PARAM_INVALID, "Op[%s] Invalid shape range attr, tensor[%zu] input dim size[%zu], actual dim size[%zu]",
               op_desc->GetName().c_str(), i, max_shape_list[i].size(), tensor->GetShape().GetDimNum());
        has_shape_range_attr = false;
        return PARAM_INVALID;
      }
    }
  }
  return SUCCESS;
}
} // namespace

using Cluster = BaseCluster;
using ClusterPtr = std::shared_ptr<Cluster>;

Status DynamicShapePartitioner::Partition() {
  REQUIRE_NOT_NULL(root_graph_, "[Check][Param] Graph is nullptr.");
  DumpGraph("_Before_DSP");

  std::vector<ComputeGraphPtr> independent_compile_graphs;
  REQUIRE_GRAPH_SUCCESS(GraphUtils::GetIndependentCompileGraphs(root_graph_, independent_compile_graphs),
                        "[Get][IndependentCompileGraph]failed, graph name:%s", root_graph_->GetName().c_str());
  auto root_graph = root_graph_;  // save root_graph_ for recovery
  for (auto &graph : independent_compile_graphs) {
    // independent compile graph is which need to be partitioned, so set root_graph_ by independent compile graph
    root_graph_ = graph;
    bool is_singleop = false;
    (void)AttrUtils::GetBool(root_graph_, ATTR_SINGLE_OP_SCENE, is_singleop);
    if (is_singleop) {
      GELOGD("Skip dynamic shape partition as in single op scene.");
      REQUIRE(AttrUtils::SetBool(*root_graph_, ATTR_NAME_DYNAMIC_SHAPE_PARTITIONED, false),
              "[Set][Attr] dynamic shape partitioned flag on root graph:%s failed.", root_graph_->GetName().c_str());
      continue;
    }

    GELOGD("Start dynamic shape partition graph %s.", root_graph_->GetName().c_str());
    REQUIRE_SUCCESS(MarkUnknownShapeNodes(), "[Call][MarkUnknownShapeNodes] failed, root grah name:%s.",
                    root_graph_->GetName().c_str());
    if (unknown_shape_nodes_.empty()) {
      GELOGD("Skip dynamic shape partition of graph %s as all nodes are known shape.", root_graph_->GetName().c_str());
      REQUIRE(AttrUtils::SetBool(*root_graph_, ATTR_NAME_DYNAMIC_SHAPE_PARTITIONED, false),
              "[Set][Attr] dynamic shape partitioned flag on root graph %s failed.", root_graph_->GetName().c_str());
      continue;
    }
    REQUIRE(AttrUtils::SetBool(*root_graph_, ATTR_NAME_DYNAMIC_SHAPE_PARTITIONED, true),
            "[Set][Attr] dynamic shape partitioned flag on root graph %s failed.", root_graph_->GetName().c_str());
    REQUIRE_SUCCESS(CtrlEdgeTransfer(), "[Call][CtrlEdgeTransfer] failed, graph:%s.", root_graph_->GetName().c_str());

    auto status = PartitionImpl();
    GELOGD("%s.", DebugString().c_str());
    if (status != SUCCESS) {
      REPORT_INNER_ERROR("E19999", "Call partition impl failed in dynamic shape partition, graph:%s",
                         root_graph_->GetName().c_str());
      GELOGE(status, "[Call][PartitionImpl] Failed dynamic shape partition graph:%s, ret:%s",
             root_graph_->GetName().c_str(), DebugString().c_str());
      ClearResource();
      return status;
    }
    GELOGD("Finish dynamic shape partition graph %s.", root_graph_->GetName().c_str());
    ClearResource();
  }
  bool is_pipeline_partitioned = false;
  (void)AttrUtils::GetBool(*root_graph, ATTR_NAME_PIPELINE_PARTITIONED, is_pipeline_partitioned);
  if (is_pipeline_partitioned) {
    (void)AttrUtils::SetBool(*root_graph, ATTR_NAME_DYNAMIC_SHAPE_PARTITIONED, false);
  }
  root_graph_ = root_graph; // recovery root_graph_
  DumpGraph("_After_DSP");
  return SUCCESS;
}

Status DynamicShapePartitioner::GetSubgraphsOfIndependentCompileGraph(const ComputeGraphPtr &independent_compile_graph,
    std::vector<ComputeGraphPtr> &all_subgraphs) const {
  all_subgraphs.clear();
  REQUIRE_NOT_NULL(independent_compile_graph, "[Check][Param] independent_compile_graph not valid");
  auto root_graph = GraphUtils::FindRootGraph(independent_compile_graph);
  REQUIRE_NOT_NULL(root_graph, "[Check][Param] root_graph not valid");

  bool pipeline_partitioned = false;
  (void)AttrUtils::GetBool(*root_graph, ATTR_NAME_PIPELINE_PARTITIONED, pipeline_partitioned);
  if (!pipeline_partitioned) {
    all_subgraphs = root_graph->GetAllSubgraphs();
    return SUCCESS;
  }

  for (const auto &node : independent_compile_graph->GetAllNodes()) {
    auto op_desc = node->GetOpDesc();
    REQUIRE_NOT_NULL(op_desc, "[Check][Param] op_desc not valid");
    const auto &subgraph_names = op_desc->GetSubgraphInstanceNames();
    if (subgraph_names.empty()) {
      continue;
    }
    for (const auto &subgraph_name : subgraph_names) {
      auto subgraph = root_graph->GetSubgraph(subgraph_name);
      REQUIRE_NOT_NULL(subgraph, "[Check][Param] subgraph not valid");
      all_subgraphs.emplace_back(subgraph);
    }
  }
  return SUCCESS;
}

Status DynamicShapePartitioner::CtrlEdgeTransfer() const {
  GELOGD("Do ctrl edge transfer start!");
  GE_CHECK_NOTNULL(root_graph_);

  bool is_dynamic_shape = false;
  (void)AttrUtils::GetBool(root_graph_, ATTR_NAME_DYNAMIC_SHAPE_PARTITIONED, is_dynamic_shape);
  if (!is_dynamic_shape) {
    return SUCCESS;
  }

  std::vector<ComputeGraphPtr> all_subgraphs;
  REQUIRE_SUCCESS(GetSubgraphsOfIndependentCompileGraph(root_graph_, all_subgraphs),
                  "[Get][Subgraph]for Graph:%s failed", root_graph_->GetName().c_str());

  for (auto &subgraph : all_subgraphs) {
    for (ge::NodePtr &n : subgraph->GetDirectNode()) {
      auto op_desc = n->GetOpDesc();
      if (op_desc == nullptr) {
        continue;
      }
      auto op_type = op_desc->GetType();
      if (op_type == CONSTANT || op_type == CONSTANTOP) {
        if (n->GetInAllNodes().empty()) {
          GELOGD("[CtrlEdgeTransferPass] node [%s] in nodes is empty", n->GetName().c_str());
          continue;
        }

        GELOGD("start to tranfer ctrl edge for const node [%s]", n->GetName().c_str());

        for (auto &in_control_node : n->GetInControlNodes()) {
          GE_CHECK_NOTNULL(in_control_node);
          GE_CHECK_NOTNULL(in_control_node->GetOutControlAnchor());
          GE_CHK_STATUS_RET(ge::GraphUtils::RemoveEdge(in_control_node->GetOutControlAnchor(),
                                                       n->GetInControlAnchor()),
                            "[Remove][Edge] between %s and %s failed",
                            in_control_node->GetOutControlAnchor()->GetOwnerNode()->GetName().c_str(),
                            n->GetName().c_str());
          for (auto &out_node : n->GetOutNodes()) {
            if (out_node == nullptr) {
              continue;
            }
            GE_CHECK_NOTNULL(in_control_node->GetOutControlAnchor());
            GE_CHK_STATUS_RET(ge::GraphUtils::AddEdge(in_control_node->GetOutControlAnchor(),
                                                      out_node->GetInControlAnchor()),
                              "[Add][Edge] between %s and %s failed.",
                              in_control_node->GetOutControlAnchor()->GetOwnerNode()->GetName().c_str(),
                              out_node->GetName().c_str());
          }
        }
      }
    }
  }

  GELOGD("Do ctrl edge transfer end!");
  return SUCCESS;
}

Status DynamicShapePartitioner::PartitionImpl() {
  REQUIRE_SUCCESS(root_graph_->TopologicalSorting(),
                  "[Call][TopologicalSorting] failed, graph:%s.", root_graph_->GetName().c_str());
  REQUIRE_SUCCESS(InitClusters(), "[Init][Clusters] failed, graph:%s.", root_graph_->GetName().c_str());
  REQUIRE_SUCCESS(MergeClusters(), "[Merge][Clusters] failed, graph:%s.", root_graph_->GetName().c_str());
  REQUIRE_SUCCESS(PruneUniqueClusters(), "[Prune][Clusters] failed, graph:%s.", root_graph_->GetName().c_str());
  REQUIRE_SUCCESS(ReDynamicShapePartitioner(), "[ReDynamicShapePartitioner] failed");
  REQUIRE_SUCCESS(BuildPartitionFrame(), "[Build][PartitionFrame] failed, graph:%s.", root_graph_->GetName().c_str());
  REQUIRE_SUCCESS(CombinePartitionFrame(),
                  "[Combine][PartitionFrame] failed, graph:%s.", root_graph_->GetName().c_str());
  REQUIRE_SUCCESS(BuildPartitionSubgraph(),
                  "[Build][PartitionSubgraph] failed, graph:%s.", root_graph_->GetName().c_str());
  return SUCCESS;
}

Status DynamicShapePartitioner::PruneUniqueClusters() {
  for (auto &node : root_graph_->GetDirectNode()) {
    auto cluster = node_2_cluster_[node];
    if (unique_clusters_.count(cluster) != 0) {
      continue;
    }
    if (unique_clusters_.insert(cluster).second) {
      sorted_unique_clusters_.emplace_back(cluster);
    }
  }
  auto comp_func = [](std::shared_ptr<Cluster> clu_a, std::shared_ptr<Cluster> clu_b) -> bool {
    return clu_a->Id() < clu_b->Id();
  };
  std::sort(sorted_unique_clusters_.begin(), sorted_unique_clusters_.end(), comp_func);
  return SUCCESS;
}

Status DynamicShapePartitioner::GenerateCluster() {
  REQUIRE_SUCCESS(MarkUnknownShapeNodes(), "[Call][MarkUnknownShapeNodes] failed, root grah name:%s.",
                    root_graph_->GetName().c_str());
  REQUIRE_SUCCESS(root_graph_->TopologicalSorting(),
            "[Call][TopologicalSorting] failed, graph:%s.", root_graph_->GetName().c_str());
  REQUIRE_SUCCESS(InitClusters(), "[Init][Clusters] failed, graph:%s.", root_graph_->GetName().c_str());
  REQUIRE_SUCCESS(MergeClusters(), "[Merge][Clusters] failed, graph:%s.", root_graph_->GetName().c_str());
  REQUIRE_SUCCESS(PruneUniqueClusters(), "[Prune][Clusters] failed, graph:%s.", root_graph_->GetName().c_str());
  return SUCCESS;
}

std::string DynamicShapePartitioner::DebugString() const {
  size_t unknown = 0;
  size_t known = 0;
  size_t data = 0;
  size_t netoutput = 0;
  size_t is_inputnode = 0;
  size_t stage = 0;
  std::stringstream ss;
  ss << "All unknown shape nodes:" << std::endl;
  for (const auto &node : unknown_shape_nodes_) {
    ss << "  [" << node->GetName() << "](" << node->GetType() << ")" << std::endl;
  }
  for (const auto &cluster : unique_clusters_) {
    if (cluster->IsUnknownShape()) {
      unknown++;
    } else if (cluster->IsKnownShape()) {
      known++;
    } else if (cluster->IsData()) {
      data++;
    } else if (cluster->IsNetOutput()) {
      netoutput++;
    } else if (cluster->IsInputNode()) {
      is_inputnode++;
    } else if (cluster->IsIndependent()) {
      stage++;
    }
  }
  ss << "All clusters:" << unique_clusters_.size() << ", data:" << data << ", known:" << known
     << ", unknown:" << unknown << ", netoutput:" << netoutput << ", is_inputnode:" << is_inputnode
     << ", stage:" << stage << std::endl;
  for (const auto &cluster : unique_clusters_) {
    ss << "  " << cluster->DebugString() << std::endl;
  }
  return ss.str();
}

void DynamicShapePartitioner::ClearResource() {
  BasePartitioner::ClearResource();
  unknown_shape_nodes_.clear();
  unknown_shape_no_tiling_nodes_.clear();
}

Status DynamicShapePartitioner::MarkUnknownShapeNodes() {
  for (auto &node : root_graph_->GetDirectNode()) {
    REQUIRE_SUCCESS(CollectSpreadUnknownShapeNodes(node),
                    "[Call][CollectSpreadUnknownShapeNodes] for node:%s failed.", node->GetName().c_str());
  }
  if (!unknown_shape_nodes_.empty() && !unknown_shape_no_tiling_nodes_.empty()) {
    GELOGW("Graph cannot support no tiling, cause [%zu] no tiling nodes and [%zu] unknown shape nodes coexist.",
           unknown_shape_no_tiling_nodes_.size(), unknown_shape_nodes_.size());
    // cannot support mixed scene now, move no tiling nodes to unknown set
    for (auto no_tiling_node : unknown_shape_no_tiling_nodes_) {
      RevertOpNoTilingAttr(no_tiling_node);
      unknown_shape_nodes_.insert(no_tiling_node);
    }
    unknown_shape_no_tiling_nodes_.clear();
  }
  return SUCCESS;
}

Status DynamicShapePartitioner::InitClusters() {
  auto graph = root_graph_;

  InitClusterType();

  size_t rank = 0;
  for (const auto &node : graph->GetDirectNode()) {
    int32_t type_index = kDataTypeIndex;
    bool is_input = ((node->GetType() == CONSTANT) || (node->GetType() == CONSTANTOP)) && node->GetInNodes().empty();
    REQUIRE_NOT_NULL(node->GetOpDesc(), "[Get][OpDesc] op_desc is null, graph:%s", graph->GetName().c_str());
    if (node->GetType() == DATA || node->GetType() == AIPPDATA) {
      type_index = kDataTypeIndex;
    } else if (is_input) {
      type_index = kInputNodeTypeIndex;
    } else if (node->GetType() == NETOUTPUT) {
      type_index = kNetOutputTypeIndex;
    } else if ((node->GetType() == PARTITIONEDCALL) && (node->GetOpDesc()->HasAttr(ATTR_STAGE_LEVEL))) {
      type_index = kStageTypeIndex;
    } else if (unknown_shape_nodes_.count(node) > 0) {
      type_index = kUnknownShapeTypeIndex;
    } else {
      type_index = kKnownShapeTypeIndex;
    }
    auto cluster = MakeShared<Cluster>(rank++, type_index, node, this);
    REQUIRE_NOT_NULL(cluster, "[New][Memory] for cluster failed.");
    node_2_cluster_[node] = cluster;

    int64_t group_index = -1;
    if (AttrUtils::GetInt(node->GetOpDesc(), ATTR_NAME_CONTROL_FLOW_GROUP, group_index)) {
      GELOGD("[%s] is rts control flow Op, group index: %ld", node->GetName().c_str(), group_index);
      auto &control_node = control_nodes_[group_index];
      control_node.emplace_back(node);
    }

    // Already sorted topologically, so access to the parent cluster is safe
    for (const auto &parent : node->GetInAllNodes()) {
      cluster->AddInput(node_2_cluster_[parent]);
    }
  }
  for (const auto &node : graph->GetDirectNode()) {
    GELOGD("Make cluster for node %s : %s.", node->GetName().c_str(), node_2_cluster_[node]->DebugString().c_str());
  }
  return SUCCESS;
}

Status DynamicShapePartitioner::InitClusterType() {
  type_index_to_type_.insert({kDataTypeIndex, CLUSTER_DATA});
  type_index_to_type_.insert({kInputNodeTypeIndex, CLUSTER_INPUT_NODE});
  type_index_to_type_.insert({kNetOutputTypeIndex, CLUSTER_NETOUTPUT});
  type_index_to_type_.insert({kStageTypeIndex, CLUSTER_STAGE});
  type_index_to_type_.insert({kKnownShapeTypeIndex, CLUSTER_KNOWN_SHAPE});
  type_index_to_type_.insert({kUnknownShapeTypeIndex, CLUSTER_UNKNOWN_SHAPE});
  return SUCCESS;
}

void DynamicShapePartitioner::MergeClustersUnknownShape() {
  // Merge unknown shape clusters
  for (const auto &cluster : ordered_cluster_) {
    if (cluster->IsIndependent()) {
      continue;
    }
    for (const auto &in_cluster : cluster->Inputs()) {
      if (!in_cluster->IsUnknownShape()) {
        continue;
      }
      if (!cluster->IsAdjoinNodes(in_cluster)) {
        continue;
      }
      auto merged_clusters = cluster->MergeAllPathFrom(in_cluster);
      GELOGD("Merge all path cluster from %lu to %lu %s.", in_cluster->Id(), cluster->Id(),
             ToString(merged_clusters).c_str());
      for (const auto &merged_cluster : merged_clusters) {
        for (const auto &node : merged_cluster->Nodes()) {
          node_2_cluster_[node] = cluster;
        }
      }
    }
  }
}

void DynamicShapePartitioner::MergeClustersInputData() {
  // Merge input clusters
  std::shared_ptr<Cluster> cluster_pre = nullptr;
  for (const auto &cluster : ordered_cluster_) {
    if (!cluster->IsInputNode()) {
      continue;
    }
    if (cluster_pre != nullptr) {
      cluster_pre->Merge(cluster);
    } else {
      cluster_pre = cluster;
    }
    GELOGD("Success merge input node cluster from %lu to %lu.", cluster->Id(), cluster->Id());
    for (const auto &node : cluster->Nodes()) {
      node_2_cluster_[node] = cluster_pre;
    }
  }
}

void DynamicShapePartitioner::MergeClustersKnownShape() {
  // Merge known shape clusters
  for (const auto &cluster : ordered_cluster_) {
    if (cluster->IsIndependent()) {
      continue;
    }
    if (cluster->IsRefVariable() && cluster->Inputs().size() == 1) {
      auto in_cluster = *(cluster->Inputs().begin());
      in_cluster->Merge(cluster);
      node_2_cluster_[*(cluster->Nodes().begin())] = in_cluster;
      continue;
    }

    for (const auto &in_cluster : cluster->Inputs()) {
      if (!in_cluster->IsKnownShape()) {
        continue;
      }
      if (cluster->TryMerge(in_cluster)) {
        GELOGD("Success merge known shape cluster from %lu to %lu.", in_cluster->Id(), cluster->Id());
        for (const auto &node : in_cluster->Nodes()) {
          node_2_cluster_[node] = cluster;
        }
      }
    }
  }
}

Status DynamicShapePartitioner::MergeClusters() {
  const auto filter_known = [](const ClusterPtr &cluster) {
    return cluster->IsKnownShape() || cluster->IsInputNode();
  };
  const auto filter_unknown = [](const ClusterPtr &cluster) {
    return cluster->IsUnknownShape();
  };

  MergeClustersControlFlow();
  REQUIRE_SUCCESS(TopologicalSortClusters(filter_unknown),
                  "[TopologicalSort][Clusters] after merge control flow clusters failed.");
  MergeClustersUnknownShape();
  REQUIRE_SUCCESS(TopologicalSortClusters(filter_known),
                  "[TopologicalSort][Clusters] after merge unknown shape clusters failed.");
  MergeClustersKnownShape();
  MergeClustersInputData();
  return SUCCESS;
}

bool DynamicShapePartitioner::JudgeUnknowShapeWithAttr(const OpDescPtr &opdesc) const {
  bool is_forced_unknown = false;
  if (AttrUtils::GetBool(opdesc, ATTR_NAME_IS_UNKNOWN_SHAPE, is_forced_unknown) && is_forced_unknown) {
    GELOGD("Collect node %s as unknown as it was marked unknown forcibly.", opdesc->GetName().c_str());
    return true;
  }

  bool forced_unknown = false;
  if (AttrUtils::GetBool(opdesc, ATTR_NAME_FORCE_UNKNOWN_SHAPE, forced_unknown) && forced_unknown) {
    GELOGD("Collect node %s as unknown as it was marked force unknown node forcibly.", opdesc->GetName().c_str());
    return true;
  }
  return false;
}

namespace {
bool NoTilingCheckInputNodeUpdateShape(const ConstNodePtr &node) {
  auto op_desc = node->GetOpDesc();
  for (size_t i = 0; i < op_desc->GetInputsSize(); i++) {
    auto input_desc = op_desc->GetInputDescPtr(static_cast<uint32_t>(i));
    if (!input_desc->GetShape().IsUnknownShape()) {
      continue;
    }
    const auto &in_anchor = node->GetInAnchor(static_cast<int32_t>(i));
    GE_RT_FALSE_CHECK_NOTNULL(in_anchor);
    for (auto &peer_anchor : in_anchor->GetPeerAnchors()) {
      if (peer_anchor == nullptr) {
        continue;
      }
      auto peer_node = peer_anchor->GetOwnerNode();
      GE_RT_FALSE_CHECK_NOTNULL(peer_node);
      auto peer_op_desc = peer_node->GetOpDesc();
      GE_RT_FALSE_CHECK_NOTNULL(peer_op_desc);
      if (kExportShapeOps.find(peer_op_desc->GetType()) != kExportShapeOps.end()) {
        GELOGD("System opType[%s], opName[%s] default support export shape.", peer_op_desc->GetType().c_str(),
               peer_op_desc->GetName().c_str());
        continue;
      }
      const std::string &peer_engine_name = peer_op_desc->GetOpEngineName();
      std::vector<std::string> update_shape_engine;
      (void)AttrUtils::GetListStr(peer_op_desc, ATTR_NAME_OP_EXPORT_SHAPE_ENGINE, update_shape_engine);
      auto it = find(update_shape_engine.begin(), update_shape_engine.end(), peer_engine_name);
      if (it == update_shape_engine.end()) {
        GELOGI("Op[%s] not support no tiling, cause parent node[%s] engine[%s] not support export shape.",
               op_desc->GetName().c_str(), peer_op_desc->GetName().c_str(), peer_engine_name.c_str());
        return false;
      }
    }
  }
  return true;
}
} // namespace

bool DynamicShapePartitioner::IsNodeSupportNoTiling(const ConstNodePtr &node) const {
  auto op_desc = node->GetOpDesc();
  GE_RT_FALSE_CHECK_NOTNULL(op_desc);
  if (kNotilingOps.find(op_desc->GetType()) != kNotilingOps.end()) {
    GELOGD("System opType[%s] opName[%s] default support no tiling.", op_desc->GetType().c_str(),
           op_desc->GetName().c_str());
    return true;
  }

  const std::string &op_engine_name = op_desc->GetOpEngineName();
  if (op_desc->GetType() != DATA && op_desc->GetType() != NETOUTPUT) {
    // judge engine support tiling inline
    std::vector<std::string> tiling_inline_engine;
    (void)AttrUtils::GetListStr(op_desc, ATTR_NAME_OP_TILING_INLINE_ENGINE, tiling_inline_engine);
    auto it = find(tiling_inline_engine.begin(), tiling_inline_engine.end(), op_engine_name);
    if (it == tiling_inline_engine.end()) {
      GELOGD("Op[%s] not support no tiling, cause engine[%s] not support tiling inline.",
             op_desc->GetName().c_str(), op_engine_name.c_str());
      return false;
    }

    // judge engine support update shape after execute
    std::vector<std::string> update_shape_engine;
    (void)AttrUtils::GetListStr(op_desc, ATTR_NAME_OP_EXPORT_SHAPE_ENGINE, update_shape_engine);
    it = find(update_shape_engine.begin(), update_shape_engine.end(), op_engine_name);
    if (it == update_shape_engine.end()) {
      GELOGD("Op[%s] not support no tiling, cause engine[%s] not support update shape.",
             op_desc->GetName().c_str(), op_engine_name.c_str());
      return false;
    }
  }

  // judge shape range
  std::string max_shape_str;
  bool has_shape_range_attr = AttrUtils::GetStr(op_desc, ATTR_NAME_OP_MAX_SHAPE, max_shape_str);
  if (!has_shape_range_attr) {
    for (auto out_tensor : op_desc->GetAllOutputsDescPtr()) {
      if (!out_tensor->GetShape().IsUnknownShape()) {
        continue;
      }
      std::vector<std::pair<int64_t, int64_t>> range;
      if (out_tensor->GetShapeRange(range) == GRAPH_FAILED || range.size() != out_tensor->GetShape().GetDimNum()) {
        GELOGD("Op[%s] not support no tiling, cause invalid shape range.", op_desc->GetName().c_str());
        return false;
      }
      for (const auto &it : range) {
        if (it.second < 0) {
          GELOGD("Op[%s] not support no tiling, cause shape range max has -1.", op_desc->GetName().c_str());
          return false;
        }
      }
    }
  }

  // judge input nodes can update shape
  if (!NoTilingCheckInputNodeUpdateShape(node)) {
    return false;
  }

  GELOGD("Op[%s] check no tiling ok.", op_desc->GetName().c_str());
  return true;
}

void DynamicShapePartitioner::RevertOpNoTilingAttr(const NodePtr &node) const {
  auto opdesc = node->GetOpDesc();
  bool no_tiling = false;
  (void)AttrUtils::GetBool(opdesc, ATTR_NAME_OP_NO_TILING, no_tiling);
  if (!no_tiling) {
    return;
  }

  for (auto &in_tensor : opdesc->GetAllInputsDescPtr()) {
    (void)AttrUtils::SetBool(in_tensor, ATTR_NAME_TENSOR_NO_TILING_MEM_TYPE, false);
  }
  for (auto &out_tensor : opdesc->GetAllOutputsDescPtr()) {
    (void)AttrUtils::SetBool(out_tensor, ATTR_NAME_TENSOR_NO_TILING_MEM_TYPE, false);
  }
  (void)AttrUtils::SetBool(opdesc, ATTR_NAME_OP_NO_TILING, false);
  GELOGI("revert op[%s] no tiling attr success", opdesc->GetName().c_str());
}

Status DynamicShapePartitioner::MarkOpNoTiling(const NodePtr &node, bool no_tiling) const {
  auto opdesc = node->GetOpDesc();
  if (!no_tiling) {
    (void)AttrUtils::SetBool(opdesc, ATTR_NAME_OP_NO_TILING, no_tiling);
    return SUCCESS;
  }

  // get op max shape attr and parse to max_shape_list
  bool has_shape_range_attr = false;
  std::vector<std::vector<int64_t>> max_shape_list;
  REQUIRE_SUCCESS(ParseShapeRangeAttr(opdesc, has_shape_range_attr, max_shape_list),
                  "[Call]node[%s] invalid input shape range. Use ';' between tensors, use ',' between dims",
                  root_graph_->GetName().c_str());

  for (auto &in_tensor : opdesc->GetAllInputsDescPtr()) {
    if (in_tensor->GetShape().IsUnknownShape()) {
      (void)AttrUtils::SetBool(in_tensor, ATTR_NAME_TENSOR_NO_TILING_MEM_TYPE, true);
      GELOGD("op[%s] set in tensor no tiling", opdesc->GetName().c_str());
    }
  }
  for (size_t i = 0; i < opdesc->GetOutputsSize(); i++) {
    auto out_tensor = opdesc->MutableOutputDesc(i);
    if (out_tensor->GetShape().IsUnknownShape()) {
      (void)AttrUtils::SetBool(out_tensor, ATTR_NAME_TENSOR_NO_TILING_MEM_TYPE, true);
      GELOGD("op[%s] set out tensor[%zu] no tiling", opdesc->GetName().c_str(), i);
      if (!has_shape_range_attr) {
        continue;
      }
      (void)AttrUtils::SetListInt(out_tensor, ATTR_NAME_TENSOR_MAX_SHAPE, max_shape_list[i]);
      auto anchor = node->GetOutAnchor(i);
      if (anchor == nullptr) {
        continue;
      }
      for (auto peer_anchor : anchor->GetPeerAnchors()) {
        auto peer_node = peer_anchor->GetOwnerNode();
        auto peer_op_desc = peer_node->GetOpDesc();
        auto peer_tensor = peer_op_desc->MutableInputDesc(peer_anchor->GetIdx());
        (void)AttrUtils::SetListInt(peer_tensor, ATTR_NAME_TENSOR_MAX_SHAPE, max_shape_list[i]);
        GELOGD("No tiling set max shape for peer node[%s] tensor[%d]",
               peer_op_desc->GetName().c_str(), peer_anchor->GetIdx());
      }
    }
  }
  (void)AttrUtils::SetBool(opdesc, ATTR_NAME_OP_NO_TILING, no_tiling);
  GELOGI("mark op[%s] no tiling success", opdesc->GetName().c_str());
  return SUCCESS;
}

Status DynamicShapePartitioner::CollectSpreadUnknownShapeNodes(NodePtr node) {
  if (unknown_shape_nodes_.count(node) > 0) {
    return SUCCESS;
  }
  auto opdesc = node->GetOpDesc();
  REQUIRE_NOT_NULL(opdesc, "[Get][OpDesc] Opdesc is nullptr.");
  // One can set 'ATTR_NAME_IS_UNKNOWN_SHAPE=true' on node so as to forcing the node flow into the unknown subgraph,
  // ignore the actual shape.
  if (JudgeUnknowShapeWithAttr(opdesc)) {
    unknown_shape_nodes_.insert(node);
    return SUCCESS;
  }

  bool is_unknown = false;
  REQUIRE_SUCCESS(IsUnknownShapeNode(node, is_unknown),
                  "[Call][IsUnknownShapeNode]Failed check node %s shape.", node->GetName().c_str());
  if (is_unknown) {
    unknown_shape_nodes_.insert(node);
  }

  return SUCCESS;
}

Status DynamicShapePartitioner::IsUnknownShapeNode(NodePtr node, bool &is_unknown) {
  auto opdesc = node->GetOpDesc();
  GE_CHECK_NOTNULL(opdesc);
  auto graph = root_graph_;
  bool is_shape_unknown = false;
  // judge unknown by shape
  for (auto &out_tensor : opdesc->GetAllOutputsDescPtr()) {
    if (out_tensor != nullptr && out_tensor->GetShape().IsUnknownShape()) {
      is_shape_unknown = true;
      break;
    }
  }
  if (!is_shape_unknown) {
    for (auto &in_tensor : opdesc->GetAllInputsDescPtr()) {
      if (in_tensor != nullptr && in_tensor->GetShape().IsUnknownShape()) {
        is_shape_unknown = true;
        break;
      }
    }
  }

  if (is_shape_unknown) {
    bool is_no_tiling = IsNodeSupportNoTiling(node);
    REQUIRE_SUCCESS(MarkOpNoTiling(node, is_no_tiling),
                    "[Call][MarkOpNoTiling] failed for node[%s].", node->GetName().c_str());
    // do not set tiling attr for unknown && no tiling node's subgraph, mark unknown
    if (!is_no_tiling) {
      is_unknown = true;
      GELOGD("Mark node %s unknown as shape unknown, and cannot support no tiling.", node->GetName().c_str());
      return SUCCESS;
    }
  } else {
    REQUIRE_SUCCESS(MarkOpNoTiling(node, false),
                    "[Call][MarkOpNoTiling] failed for node[%s].", node->GetName().c_str());
  }

  // loop subgraph to check unknown&no_tiling for known node or unknown&no_tiling node
  for (auto &subgraph_name : opdesc->GetSubgraphInstanceNames()) {
    auto subgraph = graph->GetSubgraph(subgraph_name);
    REQUIRE_NOT_NULL(subgraph, "[Get][Subgraph] %s of node %s on root graph failed.", subgraph_name.c_str(),
                     node->GetName().c_str());
    bool is_subgraph_unknown = false;
    REQUIRE_SUCCESS(IsUnknownShapeGraph(subgraph, is_subgraph_unknown),
                    "[Call][IsUnknownShapeGraph] Failed check subgraph %s shape of node %s.",
                    subgraph_name.c_str(), node->GetName().c_str());
    if (is_subgraph_unknown) {
      is_unknown = true;
      REQUIRE_SUCCESS(MarkOpNoTiling(node, false),
                      "[Call][MarkOpNoTiling] failed for node[%s].", node->GetName().c_str());
      GELOGD("Mark node %s unknown as subgraph unknown, owner node is_shape_unknown[%d].",
             node->GetName().c_str(), is_shape_unknown);
      return SUCCESS;
    }
  }
  if (is_shape_unknown) {
    unknown_shape_no_tiling_nodes_.insert(node);
  }
  is_unknown = false;
  GELOGD("Mark node %s known as %s.", node->GetName().c_str(), is_shape_unknown ? "no tiling" : "known shape");
  return SUCCESS;
}

Status DynamicShapePartitioner::IsUnknownShapeGraph(ComputeGraphPtr graph, bool &is_unknown) {
  for (auto &node : graph->GetDirectNode()) {
    REQUIRE_SUCCESS(IsUnknownShapeNode(node, is_unknown),
                    "[Call][IsUnknownShapeNode]Failed check node %s shape on graph %s.",
                    node->GetName().c_str(), graph->GetName().c_str());
    if (is_unknown) {
      GELOGD("Mark graph %s unknown as contains unknown node %s.",
             graph->GetName().c_str(), node->GetName().c_str());
      return SUCCESS;
    }
  }
  return SUCCESS;
}
std::string DynamicShapePartitioner::GetSubgraphName(const Cluster &cluster) {
  bool is_unknown_shape = cluster.IsUnknownShape();
  bool is_input = cluster.IsInputNode();
  std::string known_name = (is_unknown_shape ? "_unknow" : "_know");
  std::string sub_graph_name_patten = (is_input ? "_input" : known_name);
  std::string sub_graph_name = root_graph_->GetName() + "_sub_" + std::to_string(cluster.UniqueId()) +
                               sub_graph_name_patten;
  return sub_graph_name;
}

bool DynamicShapePartitioner::IsNeedBuildPartitionFrame(const Cluster &cluster) {
  auto cluster_type = type_index_to_type_[cluster.GetTypeIndex()];
  return cluster_type == CLUSTER_UNKNOWN_SHAPE || cluster_type == CLUSTER_KNOWN_SHAPE ||
         cluster_type == CLUSTER_INPUT_NODE;
}

void DynamicShapePartitioner::ClearReDataFlowResource() {
  for (const auto &cluster : unique_clusters_) {
    cluster->Clear();
  }
  unknown_shape_nodes_.clear();
  node_2_cluster_.clear();
  control_clusters_.clear();
  control_nodes_.clear();
  ordered_cluster_.clear();
  unique_clusters_.clear();
  sorted_unique_clusters_.clear();
  type_index_to_type_.clear();
  unknown_shape_no_tiling_nodes_.clear();
}
Status DynamicShapePartitioner::ReDynamicShapePartitioner() {
  std::vector<pair<std::string, std::shared_ptr<PartitionerPass>>> passes;
  passes.emplace_back(make_pair("DynamicDataFlowPartitionerPass", MakeShared<DynamicDataFlowPartitionerPass>()));
  while (true) {
    bool need_process = false;
    bool result = false;
    for (auto &pass : passes) {
      GE_CHECK_NOTNULL(pass.second);
      Status status = pass.second->Run(root_graph_, sorted_unique_clusters_, result);
      need_process = need_process || result;
      if (status == SUCCESS) {
        GELOGD("Dynamic Shape RePartitioner pass %s is SUCCESS.", pass.first.c_str());
      } else {
        GELOGD("status, [Call][Run] RePartitioner pass %s failed.", pass.first.c_str());
      }
    }

    if (!need_process) {
      break;
    }
    (void)ClearReDataFlowResource();
    REQUIRE_SUCCESS(GenerateCluster(), "[GenerateCluster] failed");
  }
  return SUCCESS;
}
thread_local size_t Cluster::unique_id_ = 0;
}  // namespace ge
