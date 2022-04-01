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

#include "automatic_buffer_fusion.h"
#include <utility>
#include "common/configuration.h"
#include "common/fusion_statistic/fusion_statistic_writer.h"
#include "common/unknown_shape_util.h"
#include "common/scope_allocator.h"

namespace fe {
AutomaticBufferFusion::AutomaticBufferFusion(std::unique_ptr<ConnectionMatrix> connection_matrix)
    : max_out_branch_num_(MAX_OUT_BRANCH_NUMBER),
      may_duplicate_(false),
      scope_id_lower_bound_(0),
      connection_matrix_(std::move(connection_matrix)) {
  may_duplicate_ = Configuration::Instance(AI_CORE_NAME).GetDuplicationSwitch();
}

Status AutomaticBufferFusion::SetScopeIdLowerBound() {
  scope_id_lower_bound_ = ScopeAllocator::Instance().GetCurrentScopeId();
  FE_LOGD("Lower bound of scope Id is %ld", scope_id_lower_bound_);
  return SUCCESS;
}

bool AutomaticBufferFusion::AbleToFuseOnAllPaths(const ge::NodePtr &producer, const ge::NodePtr &consumer,
                                                 const NodeSet &unable_to_fuse,
                                                 std::map<std::pair<ge::NodePtr, ge::NodePtr>, bool> &result) {
  if (producer == consumer) {
    return true;
  }

  string op_pattern;
  if (!IsFusible(consumer, op_pattern)) {
    FE_LOGD("consumer %s is not fusible, pattern is %s.", consumer->GetName().c_str(), op_pattern.c_str());
    return false;
  }

  auto iter = result.find(std::make_pair(producer, consumer));
  if (iter != result.end()) {
    FE_LOGD("result of %s and %s is %u.", producer->GetName().c_str(), consumer->GetName().c_str(), iter->second);
    return iter->second;
  }

  bool curr_result = true;
  auto all_producers = consumer->GetInAllNodes();

  size_t input_size_of_consumer = all_producers.size();
  for (size_t i = 0; i < input_size_of_consumer; ++i) {
    ge::NodePtr consumer_operand = all_producers.at(i);
    // If the operand is not on a path to the producer, it doesn't matter
    // whether it's fusible.
    if (!connection_matrix_->IsConnected(producer, consumer_operand)) {
      FE_LOGD("producer %s and consumer operand %s is not reachable", producer->GetName().c_str(),
              consumer_operand->GetName().c_str());
      continue;
    }

    FE_LOGD("producer %s and consumer operand %s is reachable", producer->GetName().c_str(),
            consumer_operand->GetName().c_str());
    if (unable_to_fuse.count(consumer_operand) > 0) {
      FE_LOGD("consumerOperand %s is not fusible.", consumer_operand->GetName().c_str());
      curr_result = false;
      break;
    }
    // The producer is reachable from consumer_operand which means we need
    // to be able to fuse consumer_operand into consumer in order for
    // producer to be fusible into consumer on all paths.
    // Perform the recursive step: make sure producer can be fused into
    // consumer_operand on all paths.
    if (!AbleToFuseOnAllPaths(producer, consumer_operand, unable_to_fuse, result)) {
      FE_LOGD("producer %s and consumer operand %s can not fuse on all path.", producer->GetName().c_str(),
              consumer_operand->GetName().c_str());
      curr_result = false;
      break;
    }
  }
  result.emplace(std::make_pair(producer, consumer), curr_result);
  return curr_result;
}

size_t GetMaxUsersCountForSingleOutput(const ge::NodePtr &producer) {
  size_t max_users_count = 0;
  for (auto &output_anchor : producer->GetAllOutDataAnchors()) {
    auto current_output_users_size = output_anchor->GetPeerInDataAnchors().size();
    if (current_output_users_size > max_users_count) {
      max_users_count = current_output_users_size;
    }
  }
  FE_LOGD("Max out branch number for %s is %zu", producer->GetName().c_str(), max_users_count);
  return max_users_count;
}

AutomaticBufferFusion::NodeSet AutomaticBufferFusion::ComputeAllUnFusibleNodes(ge::ComputeGraph &graph) {
  /* All nodes in unable_to_fuse is unfusible as producer.
   * So they may be fusible as consumer. */
  NodeSet unable_to_fuse;
  std::map<std::pair<ge::NodePtr, ge::NodePtr>, bool> result;
  auto nodes = graph.GetDirectNode();
  if (nodes.empty()) {
    return unable_to_fuse;
  }
  int last_index = static_cast<int>(nodes.size() - 1);
  for (int loop_index = last_index; loop_index >= 0; loop_index--) {
    ge::NodePtr producer = nodes.at(loop_index);
    if (producer == nullptr) {
      continue;
    }
    string producer_name = producer->GetName();
    string op_pattern;

    if (IsFusible(producer, op_pattern)) {
      if (GetMaxUsersCountForSingleOutput(producer) > max_out_branch_num_) {
        FE_LOGI("One input of producer %s contains more than %zu users.", producer_name.c_str(), max_out_branch_num_);
        unable_to_fuse.insert(producer);
        continue;
      }
      /* For each producer and its consumer, if they can fused together,
       * count the total fusible nodes, it's depends on the consumers's fusible
       * node number. If the consumer can not be fused with the consumer's
       * consumer, than the total fusible nodes by fuse producer and consumer
       * is 1. For example:
       * A  -> B -> C
       *        \-> D
       * If B cannot be fused with C and D, than fuse A and B will make B
       * disappear and it becomes:
       * AB -> C
       *   \-> D,
       * So size of fusible nodes is 2 and if C and D is also fusible with B,
       * all these four nodes will be fused to one. The size of fusible nodes
       * is four.
       *
       * The total of duplicated nodes is 1 (only need to duplicate the prodcuer
       * once no matter how many consumers are unfusible).
       * Duplication is because there are some consumers which can
       * not be fused and they need the original output of the producer. We must
       * duplicate that producer to make the calculation result correct. The
       * duplication will introducer more nodes in the graph. So we need to
       * balance the duplication.
       * For example:
       * Data- >A -> B -> C -> E
       *             \-> D -> F
       * given A,B,C is fusible and B and D is not fusible, if we want to fuse
       * A,B,C the graph will be look like:
       * Data -> ABC -> E
       *      \->  A -> D -> F
       * A is duplicated for D and D's consumers.
       * In this case, when we fuse C and B, the income is 2 nodes (actually
       * the time income will be less than 2 * coefficient = 1 and the
       * coefficient is
       * because B and C still will be computed in AI-Core,
       * so the computation time can not be omitted and we assume the time
       * of moving data from memory to UB is as same as computational time.)
       * And if total size of fusible nodes is larger than 2 or duplication is
       * not necessary. We will consider the producer is fusible. */
      auto all_consumers = producer->GetOutAllNodes();
      vector<bool> fusible_with_all_consumers(all_consumers.size(), false);
      bool need_duplicate = false;
      uint32_t all_fusible_size = 0;
      for (size_t i = 0; i < all_consumers.size(); i++) {
        auto &consumer = all_consumers.at(i);
        if (consumer == nullptr) {
          continue;
        }
        bool ret = AbleToFuseOnAllPaths(producer, consumer, unable_to_fuse, result);
        if (!ret) {
          FE_LOGI("Can not fuse producer %s and consumer %s", producer_name.c_str(), consumer->GetName().c_str());
          /* producer and consumer cannot be fused, */
          need_duplicate = true;
          if (!may_duplicate_ &&  producer->GetType() != "BNTrainingUpdateV2") {
            FE_LOGD("Do not allow duplicate, producer %s is unfusible", producer_name.c_str());
            unable_to_fuse.insert(producer);
            break;
          }
        } else {
          fusible_with_all_consumers[i] = true;
          all_fusible_size += 1;
        }
      }
      /* Set extra attributes to record which consumer is fusible. */
      producer->GetOpDesc()->SetExtAttr(CONSUMER_FUSIBLE_LIST, fusible_with_all_consumers);
      producer->GetOpDesc()->SetExtAttr(NEED_DUPLICATE, need_duplicate);
      if (may_duplicate_ && need_duplicate && all_fusible_size <= 3) {
        FE_LOGD("Can not fuse all consumers with producer %s.", producer_name.c_str());
        unable_to_fuse.insert(producer);
      }
      continue;
    }
    unable_to_fuse.insert(producer);
  }
  return unable_to_fuse;
}

bool AutomaticBufferFusion::IsScopeIdValid(const ge::NodePtr &node, int64_t &scope_id) const {
  if (ScopeAllocator::GetScopeAttr(node->GetOpDesc(), scope_id)) {
    FE_LOGD("node %s is a fusion node which scope_id is %ld.", node->GetName().c_str(), scope_id);
    if (scope_id <= scope_id_lower_bound_) {
      FE_LOGD("node %s is fused, skip this node. Lower bound is %ld.", node->GetName().c_str(), scope_id_lower_bound_);
      return false;
    }
  } else {
    scope_id = -1;
  }
  return true;
}

/* If the producer is fusible, but it may be unfusible on some
 * output edges. */
bool CheckConsumerFusibleWithProducer(const ge::NodePtr &producer, const ge::NodePtr &consumer) {
  vector<bool> all_consumers_fusible_default;
  vector<bool> all_consumers_fusible =
      producer->GetOpDesc()->TryGetExtAttr(CONSUMER_FUSIBLE_LIST, all_consumers_fusible_default);
  uint32_t loop_count = 0;

  for (auto &consumer_temp : producer->GetOutAllNodes()) {
    if (consumer == consumer_temp) {
      if (loop_count >= all_consumers_fusible.size()) {
        FE_LOGW("The consumer index %u >= the size of attr %s of %s.", loop_count, CONSUMER_FUSIBLE_LIST.c_str(),
                consumer->GetName().c_str());
        return false;
      }
      return all_consumers_fusible[loop_count];
    }
    loop_count++;
  }

  FE_LOGW("Can not find this consumer %s from producer %s's peer out nodes.", consumer->GetName().c_str(),
          producer->GetName().c_str());
  return false;
}

bool AutomaticBufferFusion::CheckPathExists(const ge::NodePtr &node1, int64_t consumer_scope_id) const {
  auto old_scope_id_iter = scope_id_nodes_map_.find(consumer_scope_id);
  if (old_scope_id_iter == scope_id_nodes_map_.end()) {
    FE_LOGW("Can not find any nodes in old_scope_id %ld.", consumer_scope_id);
    return false;
  }

  for (auto &node_iter : old_scope_id_iter->second) {
    if (connection_matrix_->IsConnected(node1, node_iter.second)) {
      FE_LOGD("node1 %s and node2 %s in scope id %ld is reachable.", node1->GetName().c_str(),
              node_iter.second->GetName().c_str(), consumer_scope_id);
      return true;
    }
  }
  return false;
}

void AutomaticBufferFusion::GetAllProducersByScopeId(int64_t ScopeId, vector<ge::NodePtr> &all_nodes,
                                                     const ge::NodePtr &producer) {
  auto iter = scope_id_nodes_map_.find(ScopeId);
  if (iter == scope_id_nodes_map_.end()) {
    all_nodes.emplace_back(producer);
  } else {
    for (auto &pair : iter->second) {
      all_nodes.emplace_back(pair.second);
    }
  }
}

bool AutomaticBufferFusion::CheckLoopExistAfterFusion(const ge::NodePtr &producer, const ge::NodePtr &consumer,
                                                      int64_t producer_scope_id, int64_t consumer_scope_id,
                                                      const NodeSet &unable_to_fuse) {
  if (consumer_scope_id == -1) {
    /* That means consumer is the first node trying to fuse into producer.
     * Before this function we have checked whether there is a loop if we fuse
     * producer and consumer. */
    auto count_unable_to_fuse = unable_to_fuse.count(producer);
    return (count_unable_to_fuse != 0);
  }

  /* For all nodes in producer's scope_id, we need to do loop check */
  vector<ge::NodePtr> all_producers;
  GetAllProducersByScopeId(producer_scope_id, all_producers, producer);
  auto iter_consumer_scope = scope_id_nodes_map_.find(consumer_scope_id);

  for (auto &node : all_producers) {
    for (auto &output : node->GetOutAllNodes()) {
      if (output == consumer) {
        continue;
      }
      /* Because there contains control edges, so we
          always check the loop no matter whether the output node is
          fused by automatic ub fusion or not.
          Only when the output node is from the consumer's scope,
          we do not need to check cycle. */
      if (iter_consumer_scope != scope_id_nodes_map_.end()) {
        const std::unordered_map<int64_t, ge::NodePtr> &all_consumer_ids = iter_consumer_scope->second;
        if (all_consumer_ids.count(output->GetOpDesc()->GetId()) != 0) {
          continue;
        }
      }

      /* Check whether there is a path from output to any node in consumer's scope */
      if (CheckPathExists(output, consumer_scope_id)) {
        FE_LOGD("Loop exists if fusing %s and %s", node->GetName().c_str(), consumer->GetName().c_str());
        return true;
      }
    }
  }

  FE_LOGD("No loop exists if fusing %s and %s.", producer->GetName().c_str(), consumer->GetName().c_str());
  return false;
}

void AutomaticBufferFusion::FuseOneProducer(const ge::NodePtr &consumer, int64_t consumer_scope_id,
    const string &node_name, const NodeSet &unable_to_fuse) {
  for (auto &producer : consumer->GetInAllNodes()) {
    string op_pattern;
    if (!IsFusible(producer, op_pattern)) {
      FE_LOGD("Operand %s of consumer %s is not fusible. pattern is %s", producer->GetName().c_str(),
              node_name.c_str(), op_pattern.c_str());
      continue;
    }
    FE_LOGD("Fuse consumer %s and producer %s.", consumer->GetName().c_str(), producer->GetName().c_str());
    int64_t producer_scope_id = -1;
    bool is_producer_scope_id_valid = IsScopeIdValid(producer, producer_scope_id);
    auto CountUnableToFuse = unable_to_fuse.count(producer);
    if (!is_producer_scope_id_valid) {
      continue;
    }

    if (CountUnableToFuse != 0) {
      FE_LOGD("producer %s is in the unfusible list. ScopeIdValid: %u", producer->GetName().c_str(),
              is_producer_scope_id_valid);
      continue;
    }
    /* Producer must be one of:
     * 1. unfused node.
     * 2. fused in automatic ub fusion instead of built-in ub fusion. */
    bool loop_exist_after_fusion =
        CheckLoopExistAfterFusion(producer, consumer, producer_scope_id, consumer_scope_id, unable_to_fuse);

    if (!loop_exist_after_fusion && CheckConsumerFusibleWithProducer(producer, consumer)) {
      Status ret = FuseTwoNodes(producer, consumer, producer_scope_id, consumer_scope_id);
      if (ret == FAILED) {
        FE_LOGD("Failed to fuse producer %s and consumer %s.", producer->GetName().c_str(),
                consumer->GetName().c_str());

        break;
      }
    }
  }
}

Status AutomaticBufferFusion::Run(ge::ComputeGraph &graph) {
  FE_LOGD("Start doing automatic buffer fusion for graph %s", graph.GetName().c_str());
  /* Get the reverse post order traverse result by topo logical sorting. */
  Status ret = graph.TopologicalSorting();
  if (ret != ge::GRAPH_SUCCESS) {
    REPORT_FE_ERROR("[SubGraphOpt][AutoUb][TopoSort] Topo logical sorting for graph[%s] before auto fusion failed.",
                    graph.GetName().c_str());
    return FAILED;
  }
  if (SetScopeIdLowerBound() != SUCCESS) {
    REPORT_FE_ERROR("[SubGraphOpt][AutoUb][Run] Set ScopeId lower bound failed for graph %s", graph.GetName().c_str());
    return FAILED;
  }
  if (connection_matrix_ == nullptr) {
    connection_matrix_ = std::unique_ptr<ConnectionMatrix>(new (nothrow) ConnectionMatrix(graph));
    FE_CHECK_NOTNULL(connection_matrix_);
    connection_matrix_->Generate(graph);
  }

  NodeSet unable_to_fuse;

  unable_to_fuse = ComputeAllUnFusibleNodes(graph);
  auto nodes = graph.GetDirectNode();
  /* Loop using reverse post order */
  auto size_of_all_nodes = nodes.size();
  for (int loop_index = static_cast<int>(size_of_all_nodes - 1); loop_index >= 0; loop_index--) {
    const auto consumer = nodes.at(loop_index);
    FE_CHECK_NOTNULL(consumer);
    auto node_name = consumer->GetName();
    string op_pattern;
    if (!IsFusible(consumer, op_pattern)) {
      FE_LOGD("consumer %s is not fusible, pattern is %s", node_name.c_str(), op_pattern.c_str());
      continue;
    }

    int64_t consumer_scope_id = -1;
    if (!IsScopeIdValid(consumer, consumer_scope_id)) {
      continue;
    }
    FuseOneProducer(consumer, consumer_scope_id, node_name, unable_to_fuse);
  }

  std::unordered_set<int64_t> scope_id_set;
  for (auto &node : nodes) {
    int64_t scope_id = 0;
    (void)ScopeAllocator::GetScopeAttr(node->GetOpDesc(), scope_id);
    if (scope_id > scope_id_lower_bound_) {
      FE_LOGD("After auto ub fusion, node %s's scope_id is %ld", node->GetName().c_str(), scope_id);
      if (ge::AttrUtils::SetStr(node->GetOpDesc(), PASS_NAME_ATTR, "AutomaticUbFusion")) {
        FE_LOGD("Node[%s]: set pass_name[AutomaticUbFusion] success.", node->GetName().c_str());
      }
      scope_id_set.emplace(scope_id);
    }
  }

  FE_LOGD("Finish auto-ub-fusion for graph %s. Total fused times is %zu", graph.GetName().c_str(), scope_id_set.size());
  return SUCCESS;
}

Status AutomaticBufferFusion::FuseTwoNodes(const ge::NodePtr &producer, const ge::NodePtr &consumer,
                                           int64_t producer_scope_id, int64_t &consumer_scope_id) {
  FE_LOGD("Fuse producer %s and consumer %s with id %ld and %ld", producer->GetName().c_str(),
          consumer->GetName().c_str(), producer_scope_id, consumer_scope_id);

  int64_t final_scope_id;
  if (producer_scope_id == -1) {
    /* The producer is not fused. Set scope id according to the consumer. */
    if (consumer_scope_id == -1) {
      /* The consumer is also not fused. Use a completely new scope id */
      int64_t new_scope_id = ScopeAllocator::Instance().AllocateScopeId();
      /* node number in side new scope id will not exceed
       * the limit of MAX_NODE_NUMBER_IN_ONE_SCOPE nodes. */
      (void)SetAndRecordScopeId(producer, new_scope_id);

      (void)SetAndRecordScopeId(consumer, new_scope_id);
      final_scope_id = new_scope_id;
      consumer_scope_id = new_scope_id;
    } else {
      /* Use consumer's scope Id. */
      if (SetAndRecordScopeId(producer, consumer_scope_id) != SUCCESS) {
        FE_LOGI("Failed to set scope %ld, Id for producer %s", consumer_scope_id, producer->GetName().c_str());
        return FAILED;
      }
      final_scope_id = consumer_scope_id;
    }
  } else {
    /* The producer is fused by automatic ub fusion. */
    if (consumer_scope_id == -1) {
      /* Follow the producer's scope id */
      if (SetAndRecordScopeId(consumer, producer_scope_id) != SUCCESS) {
        FE_LOGI("Failed to set scope %ld, Id for producer %s", producer_scope_id, consumer->GetName().c_str());
        return FAILED;
      }
      consumer_scope_id = producer_scope_id;
    } else {
      /* In this case, both producer and consumer has its own scope id, we
       * change the consumer's id using the producer's because topologically,
       * the producer is set with a scope id, it must be fused from another path
       * to the leaf node. And the nodes count in that path should be equal or
       * larger than the path we are currently on.
       *
       * Change all nodes with consumer's original scope id to the producer's
       * scope id. */
      Status ret = ChangeScopeId(consumer_scope_id, producer_scope_id);
      if (ret != SUCCESS && ret != GRAPH_OPTIMIZER_NOT_FUSE_TWO_SCOPE) {
        FE_LOGI("Failed to change scope for producer %s and consumer %s", producer->GetName().c_str(),
                consumer->GetName().c_str());
        return FAILED;
      }
      if (ret != GRAPH_OPTIMIZER_NOT_FUSE_TWO_SCOPE) {
        consumer_scope_id = producer_scope_id;
      }
    }
    final_scope_id = producer_scope_id;
  }
  FE_LOGD("Final scope Id is %ld", final_scope_id);
  return SUCCESS;
}

Status AutomaticBufferFusion::SetAndRecordScopeId(const ge::NodePtr &node, int64_t scope_id) {
  if (scope_id_nodes_map_[scope_id].size() > MAX_NODE_NUMBER_IN_ONE_SCOPE) {
    FE_LOGW("Total element wise fusion op number is larger than 30!");
    return FAILED;
  }
  ScopeAllocator::SetScopeAttr(node->GetOpDesc(), scope_id);
  auto node_id = node->GetOpDesc()->GetId();
  scope_id_nodes_map_[scope_id][node_id] = node;
  return SUCCESS;
}

Status AutomaticBufferFusion::ChangeScopeId(int64_t old_scope_id, int64_t new_scope_id) {
  if (old_scope_id == -1) {
    return SUCCESS;
  }
  if (old_scope_id == new_scope_id) {
    FE_LOGD("Old scope id is as same as new scope id which is %ld.", new_scope_id);
    return SUCCESS;
  } else {
    auto old_scope_id_iter = scope_id_nodes_map_.find(old_scope_id);
    if (old_scope_id_iter == scope_id_nodes_map_.end()) {
      FE_LOGW("Can not find any nodes in old_scope_id %ld.", old_scope_id);
      return SUCCESS;
    } else {
      auto fused_node_size_of_old_scope_id = old_scope_id_iter->second.size();
      auto new_scope_id_iter = scope_id_nodes_map_.find(new_scope_id);
      if (new_scope_id_iter == scope_id_nodes_map_.end()) {
        FE_LOGW("Can not find any nodes in new_scope_id %ld.", new_scope_id);
        return SUCCESS;
      }
      auto fused_node_size_of_new_scope_id = new_scope_id_iter->second.size();
      if (fused_node_size_of_old_scope_id + fused_node_size_of_new_scope_id > MAX_NODE_NUMBER_IN_ONE_SCOPE) {
        FE_LOGW("Sub of two scope is larger than 28. Size are %zu and %zu.", fused_node_size_of_old_scope_id,
                fused_node_size_of_new_scope_id);
        return GRAPH_OPTIMIZER_NOT_FUSE_TWO_SCOPE;
      }
      for (auto &node_id_map : old_scope_id_iter->second) {
        FE_LOGD("Change the scope_id of %s from %ld to %ld.", node_id_map.second->GetName().c_str(), old_scope_id,
                new_scope_id);
        if (SetAndRecordScopeId(node_id_map.second, new_scope_id) != SUCCESS) {
          FE_LOGW("Failed to set scope %ld, Id for producer %s", new_scope_id, node_id_map.second->GetName().c_str());
          return FAILED;
        }
      }
      scope_id_nodes_map_.erase(old_scope_id);
    }
  }
  return SUCCESS;
}

bool AutomaticBufferFusion::IsTbeOp(const ge::NodePtr &node) const {
  FE_CHECK((node == nullptr), FE_LOGD("null node in judging TVMOp"), return false);
  int64_t type = 0;
  (void)ge::AttrUtils::GetInt(node->GetOpDesc(), ge::ATTR_NAME_IMPLY_TYPE, type);
  const bool res = (type == static_cast<int64_t>(domi::ImplyType::TVM));
  return res;
}

bool AutomaticBufferFusion::GetOpAttrPattern(const ge::NodePtr &node, string &op_pattern) const {
  FE_CHECK((node == nullptr), FE_LOGD("node is nullptr."), return false);
  string name = node->GetName();
  auto key_str = name + "_pattern";

  if (!ge::AttrUtils::GetStr(node->GetOpDesc(), key_str, op_pattern)) {
    FE_LOGD("node[%s] failed to get pattern [%s].", name.c_str(), key_str.c_str());
    return false;
  }

  if (op_pattern == "") {
    FE_LOGD("optype is empty for node name [%s].", name.c_str());
    return false;
  }

  return true;
}

bool AutomaticBufferFusion::IsFusible(const ge::NodePtr &node, std::string &op_pattern) const {
  bool result = IsTbeOp(node) && GetOpAttrPattern(node, op_pattern) && op_pattern == OP_PATTERN_ELEMWISE;
  auto opdesc = node->GetOpDesc();
  if (opdesc->GetType() == "Relu6Grad") {
    /* loss will not converge if we fuse this op。 */
    return false;
  }

  uint32_t thread_scope_id = 0;
  (void)ge::AttrUtils::GetInt(opdesc, kThreadScopeId, thread_scope_id);
  FE_LOGD("op name and type, [%s: %s], thread_id: %d.", opdesc->GetName().c_str(), opdesc->GetType().c_str(),
          thread_scope_id);
  if (thread_scope_id != 0) {
    /* for ffts functionop sub node. */
    return false;
  }

  string matched_pass_name;
  if (ge::AttrUtils::GetStr(opdesc, PASS_NAME_ATTR, matched_pass_name) && !matched_pass_name.empty()) {
    return false;
  }
  int64_t scope_id = -1;
  if (ScopeAllocator::GetScopeAttr(node->GetOpDesc(), scope_id) && scope_id <= scope_id_lower_bound_) {
    /* If a node has been fused by built-in pass, it's not fusible. */
    return false;
  }
  bool add_n_condition = opdesc->GetType() == "AddN" && node->GetAllInDataAnchorsSize() > MAX_OUT_BRANCH_NUMBER;
  /* Restrict that one AddN can only contain 6 input tensors. */
  FE_CHECK(add_n_condition, FE_LOGD("AddN %s has more than 6 input, skip it.", node->GetName().c_str()), return false);

  if (IsFeSupportedDynamicOp(*(opdesc.get()))) {
    return false;
  }

  return result;
}
}
