/**
 * Copyright 2022-2023 Huawei Technologies Co., Ltd
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

#include "cache_manager.h"
#include <unordered_map>
#include "inc/ffts_log.h"
#include "inc/ffts_utils.h"
#include "inc/ffts_type.h"
#include "inc/graph/debug/ge_attr_define.h"
#include "common/sgt_slice_type.h"
#include "nneng/inc/common/string_utils.h"

namespace ffts {
CacheManager::CacheManager() { }

CacheManager::~CacheManager() { }

void CacheManager::SetPersistWeightForGraph(ge::ComputeGraph& graph) {
  for (auto node : graph.GetAllNodes()) {
    if (node == nullptr) {
      continue;
    }
    SetPersistScopeIds(node);
  }
}

void CacheManager::SetPersistScopeIds(const ge::NodePtr &node) {
  bool matched = false;
  ge::NodePtr real_node = node;
  IsNodeSpecificType(kWeightTypes, real_node, matched);
  if (!matched) {
    return;
  }
  FFTS_LOGD("Check whether weight %s need persistent.", node->GetName().c_str());
  auto peer_nodes = node->GetOutDataNodes();
  if (peer_nodes.size() <= 1) {
    return;
  }

  FFTS_LOGD("Placeholder %s have more than one peer out node(%zu).",
            node->GetName().c_str(), peer_nodes.size());
  std::unordered_map<uint32_t, uint32_t> scope_count;
  for (const auto &peer_node : peer_nodes) {
    auto peer_op_desc = peer_node->GetOpDesc();
    uint32_t thread_scope_id;
    FFTS_LOGD("peer node is %s.", peer_node->GetName().c_str());
    if (!ge::AttrUtils::GetInt(peer_op_desc, ge::ATTR_NAME_THREAD_SCOPE_ID, thread_scope_id)) {
      continue;
    }
    auto iter = scope_count.find(thread_scope_id);
    if (iter == scope_count.end()) {
      scope_count[thread_scope_id] = 1;
    } else {
      ++iter->second;
    }
  }

  vector<uint32_t> persist_scope_ids;
  for (auto &ele : scope_count) {
    if (ele.second >= kPersistThreshold) {
      persist_scope_ids.emplace_back(ele.first);
    }
  }
  auto real_op_desc = real_node->GetOpDesc();
  if (!persist_scope_ids.empty()) {
    (void)ge::AttrUtils::SetListInt(real_op_desc, kCachePersistScopeIds, persist_scope_ids);
    FFTS_LOGD("%s(place holder %s) need persist in scope %s",
              real_op_desc->GetName().c_str(), node->GetName().c_str(),
              fe::StringUtils::IntegerVecToString(persist_scope_ids).c_str());
  }
}

void CacheManager::HandleCachePersist(ge::NodePtr &node) {
  uint32_t thread_scope_id = 0;
  if (node == nullptr || !ge::AttrUtils::GetInt(node->GetOpDesc(), ge::ATTR_NAME_THREAD_SCOPE_ID, thread_scope_id)) {
    return;
  }

  /* WARNING: If all the thread scopes which need persistent have the
   * following ids:
   * n, n+8, n+16..., then the persist id will not be discrete. */
  uint32_t persist_id = (thread_scope_id % kMaxPersistNum);
  FFTS_LOGD("Handle node %s, scope id %u, persist id %u.", node->GetName().c_str(), thread_scope_id, persist_id);
  bool need_persist = false;
  int64_t max_cache_size = 0;
  for (uint32_t i = 0; i < node->GetAllInDataAnchorsSize(); i++) {
    ge::NodePtr peer_out_node;
    bool is_weight = IsPeerOutWeight(node, static_cast<int>(i), peer_out_node);
    if (!is_weight || peer_out_node == nullptr) {
      FFTS_LOGD("Skip input %u of node %s.", i, node->GetName().c_str());
      continue;
    }
    vector<uint32_t> ids;
    (void)ge::AttrUtils::GetListInt(peer_out_node->GetOpDesc(), kCachePersistScopeIds, ids);
    if (ids.empty()) {
      FFTS_LOGD("The peer const %s does not have more than one peer output.", peer_out_node->GetName().c_str());
      continue;
    }

    FFTS_LOGD("Node %s do persist, id %s.", node->GetName().c_str(), fe::StringUtils::IntegerVecToString(ids).c_str());
    if (std::find(ids.begin(), ids.end(), thread_scope_id) != ids.end()) {
      auto tensor_desc = node->GetOpDesc()->MutableInputDesc(i);
      auto data_type = tensor_desc->GetDataType();
      int64_t tensor_data_size = tensor_desc->MutableShape().GetShapeSize() * ge::GetSizeByDataType(data_type);
      if (max_cache_size < tensor_data_size) {
        max_cache_size = tensor_data_size;
      }
      need_persist = true;
    }
  }
  /* Sgt sliced nodes must be in a function sub graph.
   * So their owner graph is the function sub graph.
   * Here we set the persist id on owner graph so when
   * generating task we do not need to search in the
   * whole subgraph. */
  if (need_persist) {
    auto owner_sub_graph = node->GetOwnerComputeGraph();
    auto function_node = owner_sub_graph->GetParentNode();
    if (function_node == nullptr) {
      return;
    }
    auto op_desc = function_node->GetOpDesc();
    int64_t ori_max_cache_size = 0;
    (void)ge::AttrUtils::GetInt(op_desc, kCachePersistSize, ori_max_cache_size);
    if (max_cache_size > ori_max_cache_size) {
      FFTS_LOGD("Set persist id %u, size %ld for func op %s.", persist_id, max_cache_size, op_desc->GetName().c_str());
      (void)ge::AttrUtils::SetInt(op_desc, kCachePersist, persist_id);
      (void)ge::AttrUtils::SetInt(op_desc, kCachePersistSize, max_cache_size);
    } else {
      FFTS_LOGD("Keep the original tensor size %ld for func op %s.", ori_max_cache_size, op_desc->GetName().c_str());
    }
  }
}

Status SetPrefetchBitmap(const ge::NodePtr &node) {
  uint32_t curr_id = 0;
  if (!ge::AttrUtils::GetInt(node->GetOpDesc(), ge::ATTR_NAME_THREAD_SCOPE_ID, curr_id)) {
    FFTS_LOGD("Node %s is not a sgt sliced node.", node->GetName().c_str());
    return SUCCESS;
  }

  FFTS_LOGI("Start check prefetch operation for node %s.", node->GetName().c_str());
  size_t input_size = node->GetAllInDataAnchors().size();
  uint64_t prefetch_enable_bm = 0;
  for (size_t i = 0; i < input_size && i < kMaxCacheOperationSize; i++) {
    auto in_anchor = node->GetInDataAnchor((static_cast<int>(i)));
    FFTS_CHECK_NOTNULL(in_anchor);
    auto peer_out_anchor = in_anchor->GetPeerOutAnchor();
    if (peer_out_anchor == nullptr ||
        peer_out_anchor->GetOwnerNode() == nullptr) {
      continue;
    }

    auto peer_node = peer_out_anchor->GetOwnerNode();
    uint32_t peer_id = 0;
    /* If peer id does not exist or it is not equal to current node's id, which means
     * they belong to two different sgt subgraph, we need to do
     * prefetch operation. */
    if (!ge::AttrUtils::GetInt(peer_node->GetOpDesc(), ge::ATTR_NAME_THREAD_SCOPE_ID, peer_id) ||
        curr_id != peer_id) {
      SetBitOne(i, prefetch_enable_bm);
      FFTS_LOGI("prefetch bm is for input %zu of node %s, peer %s is %ld.", i, node->GetName().c_str(),
                peer_node->GetName().c_str(), prefetch_enable_bm);
    }
  }
  if (prefetch_enable_bm) {
    (void)ge::AttrUtils::SetInt(node->GetOpDesc(), kPrefetchEnableBm, prefetch_enable_bm);
  }
  return SUCCESS;
}

bool IsSubGraphBoundary(const ge::NodePtr &phony_concat, uint32_t curr_id) {
  if (phony_concat->GetType() != kTypePhonyConcat) {
    return false;
  }

  FFTS_LOGD("Check whether phony concat %s is a boundary op.", phony_concat->GetName().c_str());
  for (const auto &peer_node : phony_concat->GetOutDataNodes()) {
    uint32_t peer_id = 0;
    if (!ge::AttrUtils::GetInt(peer_node->GetOpDesc(), ge::ATTR_NAME_THREAD_SCOPE_ID, peer_id)) {
      return true;
    } else {
      if (peer_id != curr_id) {
        return true;
      }
    }
  }
  FFTS_LOGD("Phony concat %s is not a boundary op.", phony_concat->GetName().c_str());
  return false;
}

Status SetInvalidateAndWriteBack(const ge::NodePtr &node) {
  uint32_t curr_id = 0;
  if (!ge::AttrUtils::GetInt(node->GetOpDesc(), ge::ATTR_NAME_THREAD_SCOPE_ID, curr_id)) {
    return SUCCESS;
  }
  FFTS_LOGI("Start check output cache operation for node %s.", node->GetName().c_str());
  size_t output_size = node->GetAllOutDataAnchorsSize();
  uint64_t write_back_bm = 0;
  uint64_t invalidate_bm = 0;
  for (size_t i = 0; i < output_size && i < kMaxCacheOperationSize; i++) {
    auto out_anchor = node->GetOutDataAnchor((static_cast<int>(i)));
    FFTS_CHECK_NOTNULL(out_anchor);
    auto peer_in_anchors = out_anchor->GetPeerInDataAnchors();
    uint32_t cons_cnt = 0;
    CACHE_OPERATION operation = CACHE_OPERATION::CACHE_OPERATION_BOTTOM;
    /* If one of the peer in node is from different thread scope, we
     * need to write back the output value from cache to ddr.
     * If all the peer in nodes are from same thread scope, we
     * need to set a consumer count for this output's data context. */
    ge::NodePtr peer_in_node = nullptr;
    for (const auto &peer_in_anchor : peer_in_anchors) {
      if (peer_in_anchor == nullptr || peer_in_anchor->GetOwnerNode() == nullptr) {
        continue;
      }
      peer_in_node = peer_in_anchor->GetOwnerNode();
      uint32_t peer_id = 0;
      /* PhonyConcat is no-task node, we need to penetrate through it
       * and find the next node of PhonyConcat.
       * If the PhonyConcat is sub-graph boundary, we need writeback;
       * We assume there is no case where two PhonyConcat are directly
       * connected to each other. A-> PhonyConcat -> PhonyConcat */
      if (IsSubGraphBoundary(peer_in_node, curr_id) ||
          (!ge::AttrUtils::GetInt(peer_in_node->GetOpDesc(), ge::ATTR_NAME_THREAD_SCOPE_ID, peer_id) ||
           curr_id != peer_id)) {
        operation = CACHE_OPERATION::WRITE_BACK;
        break;
      } else {
        cons_cnt++;
        FFTS_LOGI("Output %zu of node %s, peer %s.", i, node->GetName().c_str(),
                  peer_in_node->GetName().c_str());
      }
    }

    if (operation == CACHE_OPERATION::WRITE_BACK) {
      SetBitOne(i, write_back_bm);
      FFTS_LOGI("Write back bm is for output %zu of node %s, peer %s is %ld.", i, node->GetName().c_str(),
                peer_in_node->GetName().c_str(), write_back_bm);
    } else {
      auto output_desc = node->GetOpDesc()->MutableOutputDesc(i);
      FFTS_CHECK_NOTNULL(output_desc);
      (void)ge::AttrUtils::SetInt(output_desc, "cons_cnt", cons_cnt);
      SetBitOne(i, invalidate_bm);
      FFTS_LOGI("invalidate back bm is for output %zu of node %s, invalidate_bm is %ld.",
                i, node->GetName().c_str(), invalidate_bm);
    }
  }
  (void)ge::AttrUtils::SetInt(node->GetOpDesc(), kWriteBackBm, write_back_bm);
  (void)ge::AttrUtils::SetInt(node->GetOpDesc(), kInvalidateBm, invalidate_bm);
  return SUCCESS;
}

Status CacheManager::SetCacheOperation(ge::ComputeGraph &graph) {
  for (const auto &node : graph.GetAllNodes()) {
    FFTS_CHECK_NOTNULL(node);
    int tmp_imply_type = -1;
    (void)ge::AttrUtils::GetInt(node->GetOpDesc(), FE_IMPLY_TYPE, tmp_imply_type);

    if (BUILT_IN_IMPLY_TYPE.count(tmp_imply_type) == 0) {
      continue;
    }

    Status ret = SetPrefetchBitmap(node);
    if (ret != SUCCESS) {
      return ret;
    }

    ret = SetInvalidateAndWriteBack(node);
    if (ret != SUCCESS) {
      return ret;
    }
  }
  return SUCCESS;
}
}