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

#include "graph/manager/graph_mem_manager.h"

#include <string>

namespace ge {
MemManager::MemManager() : MemoryManager() {}

MemManager::~MemManager() { Finalize(); }

MemManager &MemManager::Instance() {
  static MemManager mem_manager;
  return mem_manager;
}

Status MemManager::Initialize(const std::vector<rtMemType_t> &memory_type) {
  const std::lock_guard<std::recursive_mutex> lock(allocator_mutex_);
  if (init_) {
    GELOGW("MemManager has been inited.");
    return SUCCESS;
  }

  auto ret = InitAllocator(memory_type, memory_allocator_map_);
  if (ret != SUCCESS) {
    GELOGE(ret, "Create MemoryAllocator failed.");
    return ret;
  }

  ret = InitAllocator(memory_type, caching_allocator_map_);
  if (ret != SUCCESS) {
    GELOGE(ret, "Create CachingAllocator failed.");
    return ret;
  }

  ret = InitAllocator(memory_type, rdma_allocator_map_);
  if (ret != SUCCESS) {
    GELOGE(ret, "Create RdmaAllocator failed.");
    return ret;
  }

  ret = InitAllocator(memory_type, host_allocator_map_);
  if (ret != SUCCESS) {
    GELOGE(ret, "Create HostMemAllocator failed.");
    return ret;
  }

  ret = InitAllocator(memory_type, session_scope_allocator_map_);
  if (ret != SUCCESS) {
    GELOGE(ret, "Create HostMemAllocator failed.");
    return ret;
  }
  init_ = true;
  memory_type_ = memory_type;
  return SUCCESS;
}

uint8_t *MemManager::MallocMemory(const rtMemType_t memory_type, const std::string &purpose,
                                  const std::string &memory_key, const size_t memory_size, const uint32_t device_id) {
  return MemManager::Instance().MemInstance(memory_type).MallocMemory(purpose, memory_key, memory_size, device_id);
}

Status MemManager::FreeMemory(const rtMemType_t memory_type, const std::string &memory_key, const uint32_t device_id) {
  return MemManager::Instance().MemInstance(memory_type).FreeMemory(memory_key, device_id);
}

uint8_t *MemManager::GetMemoryBase(const rtMemType_t memory_type, const std::string &memory_key,
                                   const uint32_t device_id) {
  if (memory_type == RT_MEMORY_RDMA_HBM) {
    return MemManager::Instance().RdmaPoolInstance(RT_MEMORY_HBM).GetRdmaBaseAddr();
  }

  return MemManager::Instance().MemInstance(memory_type).GetMemoryAddr(memory_key, device_id);
}

uint8_t *MemManager::GetMemoryAddr(const rtMemType_t memory_type, const std::string &memory_key,
                                   const uint32_t device_id) {
  return MemManager::Instance().MemInstance(memory_type).GetMemoryAddr(memory_key, device_id);
}

uint8_t *MemManager::GetRdmaPoolMemory(const rtMemType_t memory_type, const size_t memory_size,
                                       const uint32_t device_id) {
  return MemManager::Instance().RdmaPoolInstance(memory_type).Malloc(memory_size, device_id);
}

void MemManager::FreeSessionMemory(const uint64_t session_id, const uint32_t device_id) {
  for (const auto &memory_type : memory_type_) {
    (void)SessionScopeMemInstance(memory_type).Free(session_id, device_id);
  }
}

template <typename T>
void MemManager::FinalizeAllocatorMap(std::map<rtMemType_t, T *> &allocate_map) const {
  for (auto &allocator : allocate_map) {
    if (allocator.second != nullptr) {
      allocator.second->Finalize();
      delete allocator.second;
      allocator.second = nullptr;
    }
  }
  allocate_map.clear();
}

void MemManager::Finalize() noexcept {
  GELOGI("Finalize.");
  const std::lock_guard<std::recursive_mutex> lock(allocator_mutex_);
  // caching and rdma allocator use memory allocator, so finalize them first
  FinalizeAllocatorMap(session_scope_allocator_map_);
  FinalizeAllocatorMap(caching_allocator_map_);
  FinalizeAllocatorMap(rdma_allocator_map_);
  FinalizeAllocatorMap(host_allocator_map_);
  FinalizeAllocatorMap(memory_allocator_map_);
  init_ = false;
  memory_type_.clear();
}

MemoryAllocator &MemManager::MemInstance(const rtMemType_t memory_type) {
  return GetAllocator(memory_type, memory_allocator_map_);
}

CachingAllocator &MemManager::CachingInstance(const rtMemType_t memory_type) {
  return GetAllocator(memory_type, caching_allocator_map_);
}

RdmaPoolAllocator &MemManager::RdmaPoolInstance(const rtMemType_t memory_type) {
  return GetAllocator(memory_type, rdma_allocator_map_);
}

HostMemAllocator &MemManager::HostMemInstance(const rtMemType_t memory_type) {
  return GetAllocator(memory_type, host_allocator_map_);
}

SessionScopeMemAllocator &MemManager::SessionScopeMemInstance(const rtMemType_t memory_type) {
  return GetAllocator(memory_type, session_scope_allocator_map_);
}
}  // namespace ge
