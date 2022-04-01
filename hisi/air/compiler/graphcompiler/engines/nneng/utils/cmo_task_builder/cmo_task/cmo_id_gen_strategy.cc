/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2022. All rights reserved.
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

#include "common/cmo_id_gen_strategy.h"
#include "common/comm_log.h"

namespace fe {
CMOIdGenStrategy::CMOIdGenStrategy() {}
CMOIdGenStrategy::~CMOIdGenStrategy() {}

CMOIdGenStrategy& CMOIdGenStrategy::Instance() {
  static CMOIdGenStrategy cmoIdGenStrategy;
  return cmoIdGenStrategy;
}

Status CMOIdGenStrategy::Finalize() {
  std::lock_guard<std::mutex> lock_guard(cmo_id_mutex_);
  reuse_cmo_id_map_.clear();
  return SUCCESS;
}

uint16_t CMOIdGenStrategy::GenerateCMOId(const ge::Node &node) {
  std::lock_guard<std::mutex> lock_guard(cmo_id_mutex_);
  int64_t topo_id = node.GetOpDesc()->GetId();
  CM_LOGD("Begin to generate cmo id for node[name:%s, type:%s]", node.GetName().c_str(), node.GetType().c_str());
  auto iter = reuse_cmo_id_map_.begin();
  while (iter != reuse_cmo_id_map_.end()) {
    if (topo_id > iter->second) {
      uint16_t cmo_id = iter->first;
      reuse_cmo_id_map_.erase(iter++);
      return cmo_id;
    } else {
      iter++;
    }
  }
  uint32_t cmo_id = GetAtomicId();
  if (cmo_id > UINT16_MAX) {
    return 0;
  }
  return static_cast<uint16_t>(cmo_id);
}

void CMOIdGenStrategy::UpdateReuseMap(const int64_t &topo_id, const uint16_t &cmo_id) {
  std::lock_guard<std::mutex> lock_guard(cmo_id_mutex_);
  reuse_cmo_id_map_[cmo_id] = topo_id;
}

uint32_t CMOIdGenStrategy::GetAtomicId() const {
  static std::atomic<uint32_t> global_cmo_id(1);
  return global_cmo_id.fetch_add(1, std::memory_order_relaxed);
}
}
