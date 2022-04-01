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

#include "graph/manager/graph_mem_allocator.h"
#include "common/math/math_util.h"

#include <string>

namespace ge {
Status MemoryAllocator::Initialize() {
  GELOGI("MemoryAllocator::Initialize");

  // when redo Initialize free memory
  for (auto &it_map : deviceid_2_memory_bases_map_) {
    for (auto &it : it_map.second) {
      if (FreeMemory(it.second.memory_addr_, it_map.first) != ge::SUCCESS) {
        GELOGW("Initialize: FreeMemory failed");
      }
    }
    it_map.second.clear();
  }
  deviceid_2_memory_bases_map_.clear();
  return SUCCESS;
}

void MemoryAllocator::Finalize() {
  // free memory
  for (auto &it_map : deviceid_2_memory_bases_map_) {
    for (auto &it : it_map.second) {
      if (FreeMemory(it.second.memory_addr_, it_map.first) != ge::SUCCESS) {
        GELOGW("Finalize: FreeMemory failed");
      }
    }
    it_map.second.clear();
  }
  deviceid_2_memory_bases_map_.clear();
}

uint8_t *MemoryAllocator::MallocMemory(const std::string &purpose, const size_t memory_size,
                                       const uint32_t device_id) const {
  void *memory_addr = nullptr;

  if (rtMalloc(&memory_addr, memory_size, memory_type_) != RT_ERROR_NONE) {
    REPORT_CALL_ERROR("E19999", "Call rtMalloc fail, purpose:%s, size:%zu, device_id:%u",
                      purpose.c_str(), memory_size, device_id);
    GELOGE(ge::INTERNAL_ERROR, "[Malloc][Memory] failed, device_id = %u, size= %lu",
           device_id, memory_size);

    return static_cast<uint8_t *>(memory_addr);
  }

  GELOGI("MemoryAllocator::MallocMemory device_id = %u, size= %lu", device_id, memory_size);
  GE_PRINT_DYNAMIC_MEMORY(rtMalloc, purpose.c_str(), memory_size);
  return static_cast<uint8_t *>(memory_addr);
}

Status MemoryAllocator::FreeMemory(void *memory_addr, const uint32_t device_id) const {
  GELOGI("MemoryAllocator::FreeMemory device_id = %u", device_id);
  const auto rt_ret = rtFree(memory_addr);
  if (rt_ret != RT_ERROR_NONE) {
    REPORT_CALL_ERROR("E19999", "Call rtFree fail, device_id:%u", device_id);
    GELOGE(FAILED, "[Call][RtFree] failed, rt_ret = %d, device_id = %u", rt_ret, device_id);
    return RT_ERROR_TO_GE_STATUS(rt_ret);
  }
  memory_addr = nullptr;
  return ge::SUCCESS;
}

uint8_t *MemoryAllocator::MallocMemory(const std::string &purpose, const std::string &memory_key,
                                       const size_t memory_size, const uint32_t device_id) {
  std::map<string, MemoryInfo> memory_base_map;
  const auto it_map = deviceid_2_memory_bases_map_.find(device_id);
  if (it_map != deviceid_2_memory_bases_map_.end()) {
    memory_base_map = it_map->second;
    const auto it = it_map->second.find(memory_key);
    if (it != it_map->second.end()) {
      if (CheckInt32AddOverflow(it->second.memory_used_num_, 1) == SUCCESS) {
        it->second.memory_used_num_++;
      }
      return it->second.memory_addr_;
    }
  }
  uint8_t *const memory_addr = MallocMemory(purpose, memory_size, device_id);

  if (memory_addr == nullptr) {
    REPORT_CALL_ERROR("E19999", "Malloc Memory fail, purpose:%s, memory_key:%s, memory_size:%zu, device_id:%u",
                      purpose.c_str(), memory_key.c_str(), memory_size, device_id);
    GELOGE(ge::INTERNAL_ERROR, "[Malloc][Memory] failed, memory_key[%s], size = %lu, device_id:%u.",
           memory_key.c_str(), memory_size, device_id);
    return nullptr;
  }

  MemoryInfo memory_info(memory_addr, memory_size, device_id);
  if (CheckInt32AddOverflow(memory_info.memory_used_num_, 1) == SUCCESS) {
    memory_info.memory_used_num_++;
  }
  memory_base_map[memory_key] = memory_info;
  deviceid_2_memory_bases_map_[device_id] = memory_base_map;
  mem_malloced_ = true;
  return memory_addr;
}

Status MemoryAllocator::FreeMemory(const std::string &memory_key, const uint32_t device_id) {
  const auto it_map = deviceid_2_memory_bases_map_.find(device_id);
  if (it_map == deviceid_2_memory_bases_map_.end()) {
    if (mem_malloced_) {
      GELOGW("MemoryAllocator::FreeMemory failed, device_id not exist, device_id = %u.", device_id);
    }
    return ge::INTERNAL_ERROR;
  }
  const auto it = it_map->second.find(memory_key);
  if (it == it_map->second.end()) {
    if (mem_malloced_) {
      GELOGW("MemoryAllocator::FreeMemory failed,memory_key[%s] was not exist, device_id = %u.", memory_key.c_str(),
             device_id);
    }
    return ge::INTERNAL_ERROR;
  }

  if (it->second.memory_used_num_ > 1) {
    GELOGW("MemoryAllocator::FreeMemory memory_key[%s] should not be released, reference count %d", memory_key.c_str(),
           it->second.memory_used_num_);
    // reference count greater than 1 represnt that static memory is used by
    // someone else, reference count decrement
    it->second.memory_used_num_--;
    return ge::SUCCESS;
  }

  if (FreeMemory(it->second.memory_addr_, device_id) != ge::SUCCESS) {
    REPORT_CALL_ERROR("E19999", "Free Memory fail, memory_key:%s, device_id:%u",
                      memory_key.c_str(), device_id);
    GELOGE(ge::INTERNAL_ERROR, "[Free][Memory] failed, memory_key[%s], device_id:%u",
           memory_key.c_str(), device_id);
    return ge::INTERNAL_ERROR;
  }

  GELOGI("MemoryAllocator::FreeMemory device_id = %u", device_id);

  (void)it_map->second.erase(it);
  return ge::SUCCESS;
}

uint8_t *MemoryAllocator::GetMemoryAddr(const std::string &memory_key, const uint32_t device_id) {
  const auto it_map = deviceid_2_memory_bases_map_.find(device_id);
  if (it_map == deviceid_2_memory_bases_map_.cend()) {
    GELOGW("MemoryAllocator::GetMemoryAddr failed, device_id not exist, device_id = %u.", device_id);
    return nullptr;
  }
  const map<std::string, MemoryInfo>::const_iterator it = it_map->second.find(memory_key);
  if (it == it_map->second.cend()) {
    GELOGW("MemoryAllocator::GetMemoryAddr failed, memory_key[%s] was not exist, device_id = %u.", memory_key.c_str(),
           device_id);
    return nullptr;
  }

  return it->second.memory_addr_;
}

int64_t MemoryAllocator::MemorySize(const std::string &memory_key, const uint32_t device_id) const {
  const auto it_map = deviceid_2_memory_bases_map_.find(device_id);
  if (it_map == deviceid_2_memory_bases_map_.cend()) {
    return 0;
  }
  const map<std::string, MemoryInfo>::const_iterator it = it_map->second.find(memory_key);
  if (it == it_map->second.cend()) {
    return 0;
  }
  return it->second.memory_size_;
}

SessionMemAllocator::SessionMemAllocator() {}

SessionMemAllocator::~SessionMemAllocator() {
  GELOGI("Deconstruct");
  std::lock_guard<std::mutex> lock(map_mutex_);
  for (auto &session_allocator : session_allocator_map_) {
    delete session_allocator.second;
    session_allocator.second = nullptr;
  }
  session_allocator_map_.clear();
}

SessionMemAllocator &SessionMemAllocator::Instance() {
  static SessionMemAllocator session_allocator;
  return session_allocator;
}

MemoryAllocator &SessionMemAllocator::GetMemAllocator(const uint64_t session_id) {
  const std::lock_guard<std::mutex> lock(map_mutex_);
  auto iter = session_allocator_map_.find(session_id);
  if (iter != session_allocator_map_.end()) {
    return *iter->second;
  }

  auto mem_allocator = new (std::nothrow) MemoryAllocator(RT_MEMORY_HBM);
  // usually impossible
  if (mem_allocator == nullptr) {
    REPORT_INNER_ERROR("E19999", "New MemoryAllocator fail, session_id:%lu", session_id);
    GELOGE(INTERNAL_ERROR, "[New][MemoryAllocator] fail, session_id:%lu", session_id);
    static auto default_allocator = MemoryAllocator(RT_MEMORY_RESERVED);
    return default_allocator;
  }
  session_allocator_map_[session_id] = mem_allocator;
  return *mem_allocator;
}
}  // namespace ge
