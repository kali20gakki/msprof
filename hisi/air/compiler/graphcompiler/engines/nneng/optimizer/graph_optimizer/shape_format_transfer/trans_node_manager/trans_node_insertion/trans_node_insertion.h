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

#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_SHAPE_FORMAT_TRANSFER_TRANS_NODE_MANAGER_TRANS_NODE_INSERTION_TRANS_NODE_INSERTION_H_
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_SHAPE_FORMAT_TRANSFER_TRANS_NODE_MANAGER_TRANS_NODE_INSERTION_TRANS_NODE_INSERTION_H_

#include <map>
#include <memory>
#include <vector>
#include "graph/compute_graph.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/utils/graph_utils.h"
#include "graph_optimizer/shape_format_transfer/trans_node_implementation/trans_node_base_generator.h"
#include "ops_kernel_store/fe_ops_kernel_info_store.h"

namespace fe {
const std::vector<std::string> trans_nodes_inserted_by_fe_types = {TRANSDATA,     CAST,          TRANSPOSE,
                                                                   DEPTHWISE4TO6, DEPTHWISE6TO4, RESHAPE};

using FEOpsKernelInfoStorePtr = std::shared_ptr<FEOpsKernelInfoStore>;

class TransNodeBaseGenerator;
/** @brief class of inserting all necessary trans-nodes into graph
* by specific strategy according to the format and data type
* @version 1.0 */
class TransNodeInsertion {
 public:
  explicit TransNodeInsertion(FEOpsKernelInfoStorePtr fe_ops_store_ptr);

  ~TransNodeInsertion();

  TransNodeInsertion(const TransNodeInsertion &) = delete;

  TransNodeInsertion &operator=(const TransNodeInsertion &) = delete;

  Status initialize();
  /* Insert all trans-nodes by strategy ID */
  Status InsertTransNodes(ge::ComputeGraph &fused_graph, ge::NodePtr dst_node);

 private:
  void InitReshapeType();

  void GetSrcReshapeType(const OpKernelInfoPtr &op_kernel);
  void GetDstReshapeType(const OpKernelInfoPtr &op_kernel);

  Status GetReshapeType();

  Status AddTransNodeType(TransNodeBaseGenerator *trans_node_type);

  Status CombineAllStrategy(TransInfoPtr trans_info_ptr, uint64_t global_strategy_id,
                            std::vector<std::vector<uint32_t>> &strategy_vector_combination);

  Status GenerateStrategy(std::vector<std::vector<uint32_t>> &strategy_vector_combination);

  Status GenerateStrategyByOrignalFormat(std::vector<std::vector<uint32_t>> &strategy_vector_combination);
  /* position_flag = 0 means from father node to current node
   * or from original format to current format (That means
   * insert trans node before current node.);
   * position_flag = 1 means from current format to
   * original format (That means
   * insert trans node after current node.)
   */
  Status InsertTransOpByConcecutiveStrategy(ge::ComputeGraph &fused_graph);

  /* For node A, insert trans-nodes in front of A, form A's input's original
   * format to A's current format and at the end of A, from A's output's
   * original format to it's */
  Status InsertTransOpByOriginalFormat(ge::ComputeGraph &fused_graph);

  FEOpsKernelInfoStorePtr fe_ops_store_info_ptr_;

  /* Key is Strategy Id, it's a 32 bits integer defines as :
   * bit0 ~ bit7 new format;
   * bit8 ~ bit15 original format;
   * bit16 ~ bit23 dst dtype
   * bit24 ~ bit31 source dtype
   *
   * StrategyIdMap is a map of strategy_id and a list of trans nodes which
   * will be inserted into graph.
   * The outside vector represents multiple insertion mode of enum
   * InsertionMode */
  strategy_id_map trans_nodes_insertion_strategy_;

  std::vector<TransNodeBaseGenerator *> whole_trans_nodes_vector_ = {};
  void SetTransInfoForInsertionModeFront();

  void SetTransInfoForInsertionModeEnd();

  void FillTransInfo(const ge::InDataAnchorPtr &dst_anchor, const ge::OutDataAnchorPtr &src_anchor,
                     const ge::NodePtr &src_node, const ge::NodePtr &dst_node, bool &use_concecutive_principle);

  TransInfoPtr global_trans_info_ptr_;
  bool IsAbleToUseConcecutivePrinciple();
  /* This function is used when we successfully inserted from src node's
   * current format to its original format. We need to update the second
   * element of trans_info_front_and_end_ because the src-node would be
   * different */
  Status UpdateNextTransInfo(uint32_t end_strategy_index);
  /* These two pointers below is for insertion mode
   * INSERT_TRANS_NODE_BY_ORIGINAL_FORMAT_FRONT and
   * INSERT_TRANS_NODE_BY_ORIGINAL_FORMAT_END*/
  std::vector<TransInfoPtr> trans_info_front_and_end_;
};

}  // namespace fe
#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_SHAPE_FORMAT_TRANSFER_TRANS_NODE_MANAGER_TRANS_NODE_INSERTION_TRANS_NODE_INSERTION_H_
