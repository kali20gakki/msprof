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

#include "graph/partition/pipeline_partition.h"

#include <algorithm>
#include <iostream>
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
#include "graph/partition/base_cluster.h"
#include "graph/utils/node_utils.h"


#define REQUIRE(cond, ...)                                     \
  do {                                                         \
    if (!(cond)) {                                             \
      REPORT_INNER_ERROR("E19999", __VA_ARGS__);               \
      GELOGE(FAILED, "[pipeline partition]" __VA_ARGS__); \
      return FAILED;                                           \
    }                                                          \
  } while (false)

#define REQUIRE_NOT_NULL(cond, ...) REQUIRE(((cond) != nullptr), __VA_ARGS__)
#define REQUIRE_SUCCESS(cond, ...) REQUIRE(((cond) == SUCCESS), __VA_ARGS__)
#define REQUIRE_GRAPH_SUCCESS(cond, ...) REQUIRE(((cond) == GRAPH_SUCCESS), __VA_ARGS__)

namespace {
const char *const CLUSTER_DATA = "DATA";
const char *const CLUSTER_NETOUTPUT = "NETOUTPUT";
const char *const CLUSTER_SUB_MODEL = "SUB_MODEL";
const char *const ATTR_SUBMODEL_INDEX = "_submodel_index";
const int32_t kDataTypeIndex = 0;
const int32_t kNetOutputTypeIndex = 1;
const int32_t kDefaultTSubModelIndex = -1;
const size_t kMinSubModelNum = 2;
}  // namespace

namespace ge {
using Cluster = BaseCluster;
using ClusterPtr = std::shared_ptr<Cluster>;

bool PipelinePartitioner::IsPipelineNeeded(const ComputeGraphPtr &root_graph) {
  for (const auto &node : root_graph->GetAllNodes()) {
    if (node->GetOpDesc() == nullptr) {
      GELOGW("op desc is nullptr, node name:%s", node->GetName().c_str());
      return false;
    }
    if (AttrUtils::HasAttr(node->GetOpDesc(), ATTR_NAME_OUTPUT_PIPELINE)) {
      GELOGD("Attribute \"_output_pipeline\" was set, node = [%s]", node->GetName().c_str());
      // split node in subgraph is not support for now.
      auto owner_graph = node->GetOwnerComputeGraph();
      if (owner_graph == nullptr) {
        GELOGW("owner graph of node:%s is nullptr.", node->GetName().c_str());
        return false;
      }
      if (owner_graph->GetParentGraph() != nullptr) {
        GELOGE(FAILED, "Split node in subgraph is not support for now in pipeline partition.");
	      return false;
      }
      return true;
    }
  }
  return false;
}

Status PipelinePartitioner::CheckPipelinePartitonResult() const {
  std::vector<ComputeGraphPtr> independent_compile_graphs;
  REQUIRE_GRAPH_SUCCESS(GraphUtils::GetIndependentCompileGraphs(root_graph_, independent_compile_graphs),
                        "[Get][IndependentCompileGraph] for Graph:%s failed.", root_graph_->GetName().c_str());
  if (independent_compile_graphs.size() < kMinSubModelNum) {
    REPORT_INNER_ERROR("E19999", "Check result of pipeline partition failed, graph:%s",
                       root_graph_->GetName().c_str());
    GELOGE(FAILED, "[Check][Result] of pipeline partition failed, graph:%s. Only one sub model generated, please check "
                   "whether the split node is correct.", root_graph_->GetName().c_str());
    return FAILED;
  }
  return SUCCESS;
}

Status PipelinePartitioner::Partition() {
  REQUIRE_SUCCESS(root_graph_->TopologicalSorting(),
		                    "[Call][TopologicalSorting] failed, graph:%s.", root_graph_->GetName().c_str());
  REQUIRE_NOT_NULL(root_graph_, "[Check][Param] Graph is nullptr.");
  if (!IsPipelineNeeded(root_graph_)) {
    GELOGD("Skip pipeline partition as no pipeline attribute specified or split node not support.");
    REQUIRE(AttrUtils::SetBool(*root_graph_, ATTR_NAME_PIPELINE_PARTITIONED, false),
            "[Set][Attr] pipeline partitioned flag on root graph:%s failed.", root_graph_->GetName().c_str());
    return SUCCESS;
  }

  GELOGD("Start pipeline partition graph %s.", root_graph_->GetName().c_str());
  REQUIRE_SUCCESS(MarkNodesModelIndex(), "[Call][MarkNodesModelIndex] failed, root graph name: %s.",
                  root_graph_->GetName().c_str());

  REQUIRE(AttrUtils::SetBool(*root_graph_, ATTR_NAME_PIPELINE_PARTITIONED, true),
          "[Set][Attr] pipeline partitioned flag on root graph %s failed.", root_graph_->GetName().c_str());

  DumpGraph("_Before_Pipeline_Partition");
  auto status = PartitionImpl();
  GELOGD("%s.", DebugString().c_str());
  if (status != SUCCESS) {
    REPORT_INNER_ERROR("E19999", "Call partition impl of pipeline partition failed, graph:%s",
                       root_graph_->GetName().c_str());
    GELOGE(status, "[Call][PartitionImpl] Failed pipeline partition graph:%s, ret:%s",
           root_graph_->GetName().c_str(), DebugString().c_str());
    ClearResource();
    return status;
  }
  DumpGraph("_After_Pipeline_Partition");
  status = CheckPipelinePartitonResult();
  if (status != SUCCESS) {
    REPORT_INNER_ERROR("E19999", "Check result of pipeline partition failed, graph:%s",
                       root_graph_->GetName().c_str());
    GELOGE(status, "[Check][Result] of pipeline partition failed, graph:%s",
           root_graph_->GetName().c_str());
  }
  GELOGD("Finish pipeline partition graph %s.", root_graph_->GetName().c_str());
  ClearResource();
  return status;
}

Status PipelinePartitioner::MarkNodesModelIndex() {
  std::map<std::string, NodePtr> last_nodes;
  for (auto &node : root_graph_->GetDirectNode()) {
    auto op_desc = node->GetOpDesc();
    REQUIRE_NOT_NULL(op_desc, "[Get][OpDesc] OpDesc is nullptr");
    if (AttrUtils::HasAttr(op_desc, ATTR_NAME_OUTPUT_PIPELINE)) {
      std::string output_pipeline_id;
      (void)AttrUtils::GetStr(op_desc, ATTR_NAME_OUTPUT_PIPELINE, output_pipeline_id);
      last_nodes[output_pipeline_id] = node;
      GELOGD("Output pipeline [%s] update cutting node to [%s]",
             output_pipeline_id.c_str(),
             node->GetName().c_str());
    }
  }

  std::set<NodePtr> cut_points;
  for (const auto &it : last_nodes) {
    cut_points.emplace(it.second);
  }

  int32_t sub_model_index = 0;
  int32_t max_sub_model_index = sub_model_index;
  // Mark ATTR_SUBMODEL_INDEX for each node according to topo order and attr ATTR_NAME_OUTPUT_PIPELINE
  for (auto &node : root_graph_->GetDirectNode()) {
    auto op_desc = node->GetOpDesc();
    REQUIRE_NOT_NULL(op_desc, "[Get][OpDesc] OpDesc is nullptr");
    REQUIRE(AttrUtils::SetInt(op_desc, ATTR_SUBMODEL_INDEX, sub_model_index),
            "[Set][Attr] sub model index on node: %s failed.", node->GetName().c_str());
    max_sub_model_index = sub_model_index;
    if (cut_points.find(node) != cut_points.end()) {
      ++sub_model_index;
      GELOGD("Meeting cutting node: [%s], update sub_model_index to %d", node->GetName().c_str(), sub_model_index);
    }
  }
  sub_model_num_ = max_sub_model_index + 1;
  return SUCCESS;
}

Status PipelinePartitioner::PartitionImpl() {
  REQUIRE_SUCCESS(InitClusters(), "[Init][Clusters] failed, graph:%s.", root_graph_->GetName().c_str());
  REQUIRE_SUCCESS(MergeClusters(), "[Merge][Clusters] failed, graph:%s.", root_graph_->GetName().c_str());
  REQUIRE_SUCCESS(PruneUniqueClusters(), "[Prune][Cluster] failed, graph:%s.", root_graph_->GetName().c_str());
  REQUIRE_SUCCESS(BuildPartitionFrame(), "[Build][PartitionFrame] failed, graph:%s.", root_graph_->GetName().c_str());
  REQUIRE_SUCCESS(CombinePartitionFrame(),
                  "[Combine][PartitionFrame] failed, graph:%s.", root_graph_->GetName().c_str());
  REQUIRE_SUCCESS(BuildPartitionSubgraph(),
                  "[Build][PartitionSubgraph] failed, graph:%s.", root_graph_->GetName().c_str());
  return SUCCESS;
}

Status PipelinePartitioner::AdjustInputNodes() const {
  for (const auto &node : root_graph_->GetDirectNode()) {
    bool is_input_node =
        ((node->GetType() == CONSTANT) || (node->GetType() == CONSTANTOP)) && node->GetInNodes().empty();
    if (is_input_node) {
      // key: sub_model_index   value: vector of InDataAnchor
      std::map<int32_t, std::vector<InDataAnchorPtr>> sub_model_index_to_anchors;
      for (const auto &anchor : node->GetAllOutDataAnchors()) {
        auto peer_in_anchors = anchor->GetPeerInDataAnchors();
        for (const auto &peer_in_anchor : peer_in_anchors) {
          REQUIRE_NOT_NULL(peer_in_anchor, "[Check][Param] Input param peer_in_anchor not valid");
          int32_t sub_model_index = kDefaultTSubModelIndex;
          auto owner_node = peer_in_anchor->GetOwnerNode();
          REQUIRE_NOT_NULL(owner_node, "[Check][Param] Input param owner_node not valid");
          auto op_desc = owner_node->GetOpDesc();
          REQUIRE_NOT_NULL(op_desc, "[Check][Param] Input param op_desc not valid");
          REQUIRE(AttrUtils::GetInt(op_desc, ATTR_SUBMODEL_INDEX, sub_model_index),
                  "[Get][Attr] sub model index on node: %s failed.", node->GetName().c_str());
          if (sub_model_index_to_anchors.find(sub_model_index) == sub_model_index_to_anchors.end()) {
            sub_model_index_to_anchors.insert({sub_model_index, {peer_in_anchor}});
          } else {
            sub_model_index_to_anchors[sub_model_index].emplace_back(peer_in_anchor);
          }
        }
      }
      if (sub_model_index_to_anchors.size() == 1) {
        int32_t sub_model_index = kDefaultTSubModelIndex;
        auto op_desc = node->GetOpDesc();
        REQUIRE_NOT_NULL(op_desc, "[Check][Param] Input param op_desc not valid");
        REQUIRE(AttrUtils::GetInt(op_desc, ATTR_SUBMODEL_INDEX, sub_model_index),
                "[Get][Attr] sub model index on node: %s failed.", node->GetName().c_str());
        auto connected_output_sub_model_index = sub_model_index_to_anchors.begin()->first;
        if (sub_model_index != connected_output_sub_model_index) {
          REQUIRE(AttrUtils::SetInt(op_desc, ATTR_SUBMODEL_INDEX, connected_output_sub_model_index),
                  "[Set][Attr] sub model index on node: %s failed.", node->GetName().c_str());
        }
      } else {
        auto iter = sub_model_index_to_anchors.begin();
        auto op_desc = node->GetOpDesc();
        REQUIRE_NOT_NULL(op_desc, "[Check][Param] Input param op_desc not valid");
        REQUIRE(AttrUtils::SetInt(op_desc, ATTR_SUBMODEL_INDEX, iter->first),
                "[Set][Attr] sub model index on node: %s failed.", node->GetName().c_str());
        ++iter;
        while (iter != sub_model_index_to_anchors.end()) {
          OpDescPtr const_op_desc = OpDescUtils::CopyOpDesc(node->GetOpDesc());
          REQUIRE(const_op_desc != nullptr,
                  "[Copy][OpDesc] from node: %s failed.", node->GetName().c_str());
          REQUIRE_SUCCESS(CopyTensorAttrs(const_op_desc, node),
                          "[Copy][TensorAttrs] from node:%s failed.", node->GetName().c_str());
          const_op_desc->SetName(node->GetName() + "_" + std::to_string(iter->first));

          GeTensorPtr weight = nullptr;
          if (AttrUtils::MutableTensor(node->GetOpDesc(), ATTR_NAME_WEIGHTS, weight)) {
            GeTensor copy_weight = weight->Clone();
            REQUIRE(AttrUtils::SetTensor(const_op_desc, ATTR_NAME_WEIGHTS, copy_weight),
                    "[Set][Tensor]copy ATTR_NAME_WEIGHTS for node:%s failed.", const_op_desc->GetName().c_str());
          }

          NodePtr const_node = root_graph_->AddNode(const_op_desc);
          REQUIRE_NOT_NULL(const_node, "[Add][Node][%s] to graph:%s failed",
                           const_op_desc->GetName().c_str(), root_graph_->GetName().c_str());
          REQUIRE(AttrUtils::SetInt(const_op_desc, ATTR_SUBMODEL_INDEX, iter->first),
                  "[Set][Attr] sub model index on node: %s failed.", const_node->GetName().c_str());
          (void)AttrUtils::SetListStr(const_op_desc, ATTR_NAME_DATA_DUMP_ORIGIN_OP_NAMES,
                                      std::move(std::vector<std::string>()));
          for (const auto &in_data_anchor : iter->second) {
            REQUIRE_GRAPH_SUCCESS(GraphUtils::RemoveEdge(node->GetOutDataAnchor(0), in_data_anchor),
                                  "[Remove][Edge] between %s and node:%s failed.",
                                  node->GetName().c_str(), in_data_anchor->GetOwnerNode()->GetName().c_str());
            REQUIRE_GRAPH_SUCCESS(GraphUtils::AddEdge(const_node->GetOutDataAnchor(0), in_data_anchor),
                                  "[Add][Edge] from %s to node:%s failed.",
                                  const_node->GetName().c_str(), in_data_anchor->GetOwnerNode()->GetName().c_str());
          }
          ++iter;
        }
      }
    }
  }
  return SUCCESS;
}

Status PipelinePartitioner::CopyTensorAttrs(const OpDescPtr &dst_desc, const NodePtr &src_node) const {
  REQUIRE_NOT_NULL(dst_desc, "[Check][Param] Input param dst node not valid");
  REQUIRE(src_node != nullptr && src_node->GetOpDesc() != nullptr, "[Check][Param] Input param src node not valid");

  const auto &src_desc = src_node->GetOpDesc();
  REQUIRE_NOT_NULL(src_desc, "[Check][Param] src desc not valid");
  dst_desc->CopyAttrsFrom(*src_desc);

  for (uint32_t i = 0; i < src_node->GetAllInDataAnchorsSize(); ++i) {
    auto input_desc = dst_desc->MutableInputDesc(i);
    if (input_desc == nullptr) {
      continue;
    }
    input_desc->CopyAttrsFrom(src_desc->GetInputDesc(i));
  }

  for (uint32_t i = 0; i < src_node->GetAllOutDataAnchorsSize(); ++i) {
    auto output_desc = dst_desc->MutableOutputDesc(i);
    REQUIRE_NOT_NULL(output_desc, "[Check][Param] Param dst node:%s not valid", dst_desc->GetName().c_str());
    output_desc->CopyAttrsFrom(src_desc->GetOutputDesc(i));
  }

  return SUCCESS;
}

Status PipelinePartitioner::InitClusters() {
  auto graph = root_graph_;
  REQUIRE_SUCCESS(InitClusterType(), "[Init][Clusters] failed, graph:%s.", root_graph_->GetName().c_str());
  REQUIRE_SUCCESS(AdjustInputNodes(), "[Split][InputNodes] failed, graph:%s.", root_graph_->GetName().c_str());

  // Since const add may added in graph, so topological sorting is needed.
  Status ret = graph->TopologicalSorting();
  if (ret != SUCCESS) {
    REPORT_CALL_ERROR("E19999", "TopologicalSorting fail, graph_id:%u", graph->GetGraphID());
    GELOGE(ret, "[Call][TopologicalSorting] for graph failed, graph_id:%u",
           graph->GetGraphID());
    return ret;
  }
  size_t rank = 0;
  for (const auto &node : graph->GetDirectNode()) {
    int32_t type_index = kDataTypeIndex;
    auto op_desc = node->GetOpDesc();
    REQUIRE_NOT_NULL(op_desc, "[Get][OpDesc] op_desc is null, node:%s", node->GetName().c_str());

    if (node->GetType() == DATA) {
      type_index = kDataTypeIndex;
    } else if (node->GetType() == NETOUTPUT) {
      type_index = kNetOutputTypeIndex;
    } else {
      int32_t sub_model_index = kDefaultTSubModelIndex;
      REQUIRE(AttrUtils::GetInt(op_desc, ATTR_SUBMODEL_INDEX, sub_model_index),
              "[Set][Attr] sub model index on node: %s failed.", node->GetName().c_str());
      type_index = type_index_to_type_.size() - sub_model_num_ + sub_model_index;
    }
    auto cluster = MakeShared<Cluster>(rank++, type_index, node, this);
    REQUIRE_NOT_NULL(cluster, "[New][Memory] for cluster failed.");
    cluster->SetMergeInputs(true);
    node_2_cluster_[node] = cluster;

    int64_t group_index = -1;
    if (AttrUtils::GetInt(node->GetOpDesc(), ATTR_NAME_CONTROL_FLOW_GROUP, group_index)) {
      GELOGD("[%s] is rts control flow Op, group index: %ld", node->GetName().c_str(), group_index);
      if (control_clusters_.find(group_index) == control_clusters_.end()) {
        REPORT_INNER_ERROR("E19999",
                           "[Check][Param] failed, cannot find group index:%ld in control_clusters.", group_index);
        GELOGE(FAILED, "[Check][Param] failed, cannot find group index:%ld in control_clusters.", group_index);
        return FAILED;
      }
      auto &control_cluster = control_clusters_[group_index];
      control_cluster.emplace_back(cluster);
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

Status PipelinePartitioner::InitClusterType() {
  type_index_to_type_.insert({kDataTypeIndex, CLUSTER_DATA});
  type_index_to_type_.insert({kNetOutputTypeIndex, CLUSTER_NETOUTPUT});

  int32_t previous_sub_model_index = kDefaultTSubModelIndex;
  for (auto &node : root_graph_->GetDirectNode()) {
    int32_t sub_model_index = kDefaultTSubModelIndex;
    auto op_desc = node->GetOpDesc();
    REQUIRE_NOT_NULL(op_desc, "[Get][OpDesc] OpDesc is nullptr");
    REQUIRE(AttrUtils::GetInt(op_desc, ATTR_SUBMODEL_INDEX, sub_model_index),
            "[Set][Attr] sub model index on node: %s failed.", node->GetName().c_str());
    if (sub_model_index != previous_sub_model_index) {
      type_index_to_type_.insert({type_index_to_type_.size(), CLUSTER_SUB_MODEL + std::to_string(sub_model_index)});
      previous_sub_model_index = sub_model_index;
      ++sub_model_index;
    }
  }
  return SUCCESS;
}

Status PipelinePartitioner::MergeClusters() {
  MergeClustersControlFlow();
  // split node in v1 control flow, not support for now
  for (const auto &item : control_clusters_) {
    const auto &control_cluster = item.second;
    for (const auto &cluster : control_cluster) {
      for (const auto &node : cluster->Nodes()) {
        REQUIRE_NOT_NULL(node, "[Check][Param] Input param node not valid");
        auto op_desc = node->GetOpDesc();
        REQUIRE_NOT_NULL(op_desc, "[Check][Param] Input param op_desc not valid");
        if (AttrUtils::HasAttr(op_desc, ATTR_NAME_OUTPUT_PIPELINE)) {
          GELOGE(FAILED, "Split node in V1 control flow is not support for now");
          return FAILED;
        }
      }
    }
  }
  for (int32_t i = sub_model_num_; i > 0; --i) {
    const auto filter_sub_model = [this, i](const ClusterPtr &cluster) {
      return cluster->GetTypeIndex() == static_cast<int32_t>(type_index_to_type_.size() - i);
    };
    REQUIRE_SUCCESS(TopologicalSortClusters(filter_sub_model),
                    "[TopologicalSort][Cluster] before merge sub model cluster index[%d] failed.", i);
    GELOGD("Begin to merge clusters belong to sub model:%d", sub_model_num_ - i);
    MergeClustersSubModel();
  }
  return SUCCESS;
}

void PipelinePartitioner::MergeClustersSubModel() {
  if (ordered_cluster_.empty()) {
    return;
  }
  // Merge clusters according to the linking relationship
  for (const auto &cluster : ordered_cluster_) {
    if (cluster->IsRefVariable() && cluster->Inputs().size() == 1) {
      auto in_cluster = *(cluster->Inputs().begin());
      in_cluster->Merge(cluster);
      node_2_cluster_[*(cluster->Nodes().begin())] = in_cluster;
      continue;
    }

    for (const auto &in_cluster : cluster->Inputs()) {
      if (cluster->GetTypeIndex() != in_cluster->GetTypeIndex()) {  // only merge clusters in the same sub model
        continue;
      }
      if (cluster->TryMerge(in_cluster)) {
        GELOGD("Success merge cluster from %lu to %lu.", in_cluster->Id(), cluster->Id());
        for (const auto &node : in_cluster->Nodes()) {
          node_2_cluster_[node] = cluster;
        }
      }
    }
  }
  // Clusters may belong to the same sub model but unlinking with other clusters after merge, record these clusters.
  // key: node correspond cluster id. record current cluster have been added to merged clusters.
  std::map<int32_t, int32_t> is_cluster_in_merged_clusters;
  std::vector<std::shared_ptr<Cluster>> merged_clusters;  // record exist merged clusters
  for (const auto &cluster : ordered_cluster_) {
    for (const auto &node : cluster->Nodes()) {
      if (is_cluster_in_merged_clusters.find(node_2_cluster_[node]->Id()) == is_cluster_in_merged_clusters.end()) {
        merged_clusters.emplace_back(node_2_cluster_[node]);
        is_cluster_in_merged_clusters.insert({node_2_cluster_[node]->Id(), 1});
      }
    }
  }
  // Collect merged clusters which belong to the same sub model and form a big cluster
  if (merged_clusters.empty()) {
    GELOGW("There is no merged clusters after merge in current sub model.");
    return;
  }
  auto cluster = merged_clusters[0];
  for (size_t i = 1; i < merged_clusters.size(); ++i) {
    cluster->Merge(merged_clusters[i]);
    GELOGD("Success merge cluster from %lu to %lu.", cluster->Id(), merged_clusters[i]->Id());
    for (const auto &node : merged_clusters[i]->Nodes()) {
      node_2_cluster_[node] = cluster;
    }
  }
}

Status PipelinePartitioner::PruneUniqueClusters() {
  for (auto &node : root_graph_->GetDirectNode()) {
    auto cluster = node_2_cluster_[node];
    if (unique_clusters_.count(cluster) != 0) {
      continue;
    }
    if (unique_clusters_.insert(cluster).second) {
      sorted_unique_clusters_.emplace_back(cluster);
    }
  }
  // Sort clusters according to the index of sub model they belong to
  auto comp_func = [](std::shared_ptr<Cluster> clu_a, std::shared_ptr<Cluster> clu_b) -> bool {
    auto op_desc_a = clu_a->Nodes()[0]->GetOpDesc();  // cluster always have at least one node.
    if (op_desc_a == nullptr) {
      GELOGW("Op desc of first node in cluster:%zu is nullptr, node name:%s",
             clu_a->Id(), clu_a->Nodes()[0]->GetName().c_str());
      return true;
    }
    int32_t sub_model_index_a = kDefaultTSubModelIndex;
    (void)AttrUtils::GetInt(op_desc_a, ATTR_SUBMODEL_INDEX, sub_model_index_a);

    auto op_desc_b = clu_b->Nodes()[0]->GetOpDesc();
    if (op_desc_b == nullptr) {
      GELOGW("Op desc of first node in cluster:%zu is nullptr, node name:%s",
             clu_b->Id(), clu_b->Nodes()[0]->GetName().c_str());
      return true;
    }
    int32_t sub_model_index_b = kDefaultTSubModelIndex;
    (void)AttrUtils::GetInt(op_desc_b, ATTR_SUBMODEL_INDEX, sub_model_index_b);

    return sub_model_index_a < sub_model_index_b;
  };
  std::sort(sorted_unique_clusters_.begin(), sorted_unique_clusters_.end(), comp_func);
  return SUCCESS;
}

std::string PipelinePartitioner::DebugString() const {
  std::vector<size_t> cluster_num(type_index_to_type_.size(), 0);
  std::stringstream ss;
  for (const auto &cluster : unique_clusters_) {
    ++cluster_num[cluster->GetTypeIndex()];
  }
  ss << "All clusters:" << unique_clusters_.size();
  for (const auto &iter : type_index_to_type_) {
    ss << "," + iter.second + ":" << cluster_num[iter.first];
  }
  ss << std::endl;

  for (const auto &cluster : unique_clusters_) {
    ss << "  " << cluster->DebugString() << std::endl;
  }
  return ss.str();
}

void PipelinePartitioner::ClearResource() {
  BasePartitioner::ClearResource();
}

bool PipelinePartitioner::IsNeedBuildPartitionFrame(const Cluster &cluster) {
  auto cluster_type = type_index_to_type_[cluster.GetTypeIndex()];
  return cluster_type.find(CLUSTER_SUB_MODEL) != std::string::npos;
}

std::string PipelinePartitioner::GetSubgraphName(const Cluster &cluster) {
  return root_graph_->GetName() + "_sub_" + std::to_string(cluster.UniqueId());
}
}  // namespace ge

