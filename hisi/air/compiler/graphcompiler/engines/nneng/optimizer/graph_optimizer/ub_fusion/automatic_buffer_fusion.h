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

#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_UB_FUSION_AUTOMATIC_BUFFER_FUSION_H_
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_UB_FUSION_AUTOMATIC_BUFFER_FUSION_H_

#include <algorithm>
#include <map>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "common/fe_inner_attr_define.h"
#include "common/fe_log.h"
#include "common/fe_utils.h"
#include "common/math_util.h"
#include "common/util/constants.h"
#include "common/util/op_info_util.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/node.h"
#include "graph/range_vistor.h"
#include "connectivity_matrix.h"

namespace fe {
template <class T>
using NodeVisitor = RangeVistor<T, std::shared_ptr<const ge::Node>>;

const size_t MAX_OUT_BRANCH_NUMBER = 6;

const size_t MAX_NODE_NUMBER_IN_ONE_SCOPE = 28;

class AutomaticBufferFusion : public GraphPass {
 public:
  explicit AutomaticBufferFusion(std::unique_ptr<ConnectionMatrix> connection_matrix);

  ~AutomaticBufferFusion() override = default;
  /**
   * @ingroup fe
   * @brief Do fusion automatically
   */
  Status Run(ge::ComputeGraph &graph) override;

 private:
  // The set of producers whose consumers we cannot fuse into.
  using NodeSet = std::unordered_set<ge::NodePtr>;

  /* Loop around all nodes to find those can be fused. Fusiable ops are those:
   * 1. elementwise op
   * 2. tbe op
   * 3. can be fused into all of its successors on all paths.
   * That means, that for:
   * A --> B (fusible)
   *   \-> C (non-fusible)
   * A will be not allowed to be fused into B, as it cannot be fused into C.
   * Similarly, for:
   * A -------------> B
   *   \-> C -> D -/
   * If:
   * - A is fusible into B and C, and D is fusible into B
   * - C is *not* fusible into D
   * A will be not allowed to be fused into B, as it cannot be fused via
   * all paths. */
  NodeSet ComputeAllUnFusibleNodes(ge::ComputeGraph &graph);

  void FuseOneProducer(const ge::NodePtr &consumer, int64_t consumer_scope_id,
                       const string &node_name, const NodeSet &unable_to_fuse);
  /* Whether or not we can fuse producer into consumer on all paths
   * from the producer to the consumer.
   *
   * A map from <producer, consumer> to a bool is required as the result cache
   * to store and query the results of calls to this function, in order to avoid
   * repeated computations. */
  bool AbleToFuseOnAllPaths(const ge::NodePtr &producer, const ge::NodePtr &consumer, const NodeSet &unable_to_fuse,
                            std::map<std::pair<ge::NodePtr, ge::NodePtr>, bool> &result);

  Status SetScopeIdLowerBound();

  bool IsScopeIdValid(const ge::NodePtr &node, int64_t &scope_id) const;

  /* The input parameter producer and consumer is for sure able to fuse.
   * The valid checking is done before fusing them. */
  Status FuseTwoNodes(const ge::NodePtr &producer, const ge::NodePtr &consumer, int64_t producer_scope_id,
                      int64_t &consumer_scope_id);

  Status SetAndRecordScopeId(const ge::NodePtr &node, int64_t scope_id);

  /* Change all nodes which has old_id as scope_id to the new_id.
   * Remove all nodes in the old_id map and them into the new_id map. */
  Status ChangeScopeId(int64_t old_scope_id, int64_t new_scope_id);
  /*
   * @brief: check if is TVM type op
   * @param [in] node: node
   * @return bool: check result
   */
  bool IsTbeOp(const ge::NodePtr &node) const;

  /*
   * @brief: get the op pattern from attributes
   * @param [in] node: graph node
   * @param [out] op_type: type represent by std::string
   * @return bool: get op type ok or not
   */
  bool GetOpAttrPattern(const ge::NodePtr &node, std::string &op_pattern) const;

  bool IsFusible(const ge::NodePtr &node, std::string &op_pattern) const;

  void GetAllProducersByScopeId(int64_t ScopeId, vector<ge::NodePtr> &all_nodes, const ge::NodePtr &producer);

  /* Check whether there is a node among all output nodes of the producer
   * (except for the consumer itself) which is :
   * 1. unfusible or fused by built-in pass(scope id is less than lower bound)
   * 2. Can reach one of the nodes in current consumers's scope_id. If the
   * consumer's scope id is -1, it means we just need to check consumer itself.
   * */
  bool CheckLoopExistAfterFusion(const ge::NodePtr &producer, const ge::NodePtr &consumer, int64_t producer_scope_id,
                                 int64_t consumer_scope_id, const NodeSet &unable_to_fuse);

  /* Check whether there is a path from node1 to one of the nodes in consumer's
   * scope. */
  bool CheckPathExists(const ge::NodePtr &node1, int64_t consumer_scope_id) const;

  size_t max_out_branch_num_;

  bool may_duplicate_;

  int64_t scope_id_lower_bound_;
  /* A adjacent matrix which stores whether there is a path between two nodes
   * in current graph. */
  std::unique_ptr<ConnectionMatrix> connection_matrix_;

  /* Stores the map of (scope_id, list of nodes) */
  std::unordered_map<int64_t, std::unordered_map<int64_t, ge::NodePtr>> scope_id_nodes_map_;
};

}  // end namespace fe
#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_UB_FUSION_AUTOMATIC_BUFFER_FUSION_H_
