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

#include "graph_optimizer/shape_format_transfer/trans_node_manager/trans_node_manager.h"
#include <memory>
#include <string>
#include "common/fe_utils.h"

namespace fe {

TransNodeManager::TransNodeManager(FEOpsKernelInfoStorePtr fe_ops_kernel_info_store_ptr)
    : trans_node_merging_ptr_(nullptr),
      trans_node_insertion_ptr_(nullptr),
      fe_ops_kernel_info_store_ptr_(fe_ops_kernel_info_store_ptr) {}

TransNodeManager::~TransNodeManager() {}

Status TransNodeManager::Initialize() {
  FE_MAKE_SHARED(trans_node_insertion_ptr_ = std::make_shared<TransNodeInsertion>(fe_ops_kernel_info_store_ptr_),
                 return GRAPH_OPTIMIZER_MAKE_SHARED_FAILED);
  FE_CHECK(trans_node_insertion_ptr_ == nullptr,
           REPORT_FE_ERROR("[GraphOptJdgInst][ShapeTrans][TransNdManager] spaceSizeCalculatorPtr_ is null."),
           return FAILED);

  if (trans_node_insertion_ptr_->initialize() != SUCCESS) {
    REPORT_FE_ERROR("[GraphOptJdgInst][ShapeTrans][TransNdManager] TransNode Insert init failed.");
    return FAILED;
  }

  FE_MAKE_SHARED(trans_node_merging_ptr_ = std::make_shared<TransNodeMerging>(),
                 return GRAPH_OPTIMIZER_MAKE_SHARED_FAILED);
  FE_CHECK(trans_node_merging_ptr_ == nullptr,
           REPORT_FE_ERROR("[GraphOptJdgInst][ShapeTrans][TransNdManager] opJudgePtr_ is null."), return FAILED);
  return SUCCESS;
}

Status TransNodeManager::InsertAndMergeTransNodes(ge::ComputeGraph &graph) {
  FE_TIMECOST_START(InsertAndMergeTransNodes);
  FE_LOGD("Function InsertTransNode begins.");
  if (graph.TopologicalSorting() != ge::GRAPH_SUCCESS) {
    REPORT_FE_ERROR("[GraphOptJdgInst][ShapeTrans][TopoSort] Fail to do topological sorting for graph %s.",
                    graph.GetName().c_str());

    return fe::SHAPE_FORMAT_TRANSFER_SORTING_FAILED;
  }

  for (ge::NodePtr &dst_node : graph.GetDirectNode()) {
    if (dst_node == nullptr) {
      continue;
    }

    if (dst_node->GetOpDesc() == nullptr) {
      continue;
    }

    /* Store Cast in original grpah. */
    if (dst_node->GetType() == CAST) {
      optimizable_cast_list_.emplace_back(dst_node);
    }

    /* Loop all input data anchor of dst_node's and insert transop
     * if the input's dtype or format is not the same as it's father
     * node's output dtype and format */
    Status ret = trans_node_insertion_ptr_->InsertTransNodes(graph, dst_node);
    if (ret != SUCCESS) {
      return ret;
    }
  }

  // re-order nodes in graph
  if (graph.TopologicalSorting() != ge::GRAPH_SUCCESS) {
    REPORT_FE_ERROR("[GraphOptJdgInst][ShapeTrans][TopoSort] TopologicalSorting failed!");
    return fe::SHAPE_FORMAT_TRANSFER_SORTING_FAILED;
  }

  // dump
  FE_LOGD("After processed, the graph [%s] is as below:", graph.GetName().c_str());
  for (auto &node : graph.GetDirectNode()) {
    FE_LOGD("Node named [%s]", node->GetName().c_str());
  }

  if (trans_node_merging_ptr_->MergeAllTransOps(graph) == FAILED) {
    return FAILED;
  }
  FE_TIMECOST_END(InsertAndMergeTransNodes, "InsertAndMergeTransNodes during FEGraphOptimizer::OptimizeOriginalGraph");
  return SUCCESS;
}

const std::vector<ge::NodePtr> &TransNodeManager::GetOptimizableCast() { return optimizable_cast_list_; }
}  // namespace fe
