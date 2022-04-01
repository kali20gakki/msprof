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

#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_SHAPE_FORMAT_TRANSFER_TRANS_NODE_MANAGER_TRANS_NODE_MANAGER_H_
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_SHAPE_FORMAT_TRANSFER_TRANS_NODE_MANAGER_TRANS_NODE_MANAGER_H_

#include "common/fe_inner_error_codes.h"
#include "common/util/op_info_util.h"
#include "graph/compute_graph.h"
#include "graph_optimizer/shape_format_transfer/trans_node_manager/trans_node_insertion/trans_node_insertion.h"
#include "graph_optimizer/shape_format_transfer/trans_node_manager/trans_node_merging/trans_node_merging.h"
#include "ops_kernel_store/fe_ops_kernel_info_store.h"
namespace fe {
using TransNodeMergingPtr = std::shared_ptr<TransNodeMerging>;
using TransNodeInsertionPtr = std::shared_ptr<TransNodeInsertion>;

/** @brief This class manage all operations of trans-nodes including insertion
* and merging.
* @version 1.0 */
class TransNodeManager {
 public:
  explicit TransNodeManager(FEOpsKernelInfoStorePtr fe_ops_kernel_info_store_ptr);

  ~TransNodeManager();

  TransNodeManager(const TransNodeManager &) = delete;

  TransNodeManager &operator=(const TransNodeManager &) = delete;

  Status Initialize();

  /* This is interface for graph optimizer. In this function we  */
  Status InsertAndMergeTransNodes(ge::ComputeGraph &graph);

  const std::vector<ge::NodePtr> &GetOptimizableCast();

 private:
  TransNodeMergingPtr trans_node_merging_ptr_;

  TransNodeInsertionPtr trans_node_insertion_ptr_;

  FEOpsKernelInfoStorePtr fe_ops_kernel_info_store_ptr_;

  std::vector<ge::NodePtr> optimizable_cast_list_;
};
}  // namespace fe
#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_SHAPE_FORMAT_TRANSFER_TRANS_NODE_MANAGER_TRANS_NODE_MANAGER_H_
