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

#ifndef GE_GRAPH_MANAGER_GRAPH_MEM_MANAGER_H_
#define GE_GRAPH_MANAGER_GRAPH_MEM_MANAGER_H_

#include <iostream>
#include <map>
#include <mutex>
#include <vector>

#include "framework/common/debug/ge_log.h"
#include "framework/common/ge_inner_error_codes.h"
#include "graph/manager/graph_mem_allocator.h"
#include "graph/manager/graph_caching_allocator.h"
#include "graph/manager/host_mem_allocator.h"
#include "graph/manager/rdma_pool_allocator.h"
#include "graph/manager/host_mem_allocator.h"
#include "graph/manager/session_scope_mem_allocator.h"
#include "graph/node.h"
#include "runtime/mem.h"
#include "common/mem_manager.h"

namespace ge {
class MemManager : public MemoryManager {
 public:
  MemManager();
  ~MemManager() override;
  static MemManager &Instance();

  uint8_t *MallocMemory(const rtMemType_t memory_type, const std::string &purpose, 
		                const std::string &memory_key,const size_t memory_size,
				const uint32_t device_id) override;

  Status FreeMemory(const rtMemType_t memory_type, const std::string &memory_key,
		            const uint32_t device_id) override;

  uint8_t *GetMemoryBase(const rtMemType_t memory_type, const std::string &memory_key,
		                 const uint32_t device_id) override;

  uint8_t *GetMemoryAddr(const rtMemType_t memory_type, const std::string &memory_key,
		                 const uint32_t device_id) override;

  uint8_t *GetRdmaPoolMemory(const rtMemType_t memory_type, const size_t memory_size,
		                     const uint32_t device_id) override;

  void FreeSessionMemory(const uint64_t session_id, const uint32_t device_id = 0U);

  MemoryAllocator &MemInstance(const rtMemType_t memory_type);
  CachingAllocator &CachingInstance(const rtMemType_t memory_type);
  RdmaPoolAllocator &RdmaPoolInstance(const rtMemType_t memory_type);
  HostMemAllocator &HostMemInstance(const rtMemType_t memory_type = RT_MEMORY_HBM);
  SessionScopeMemAllocator &SessionScopeMemInstance(const rtMemType_t memory_type);

  ///
  /// @ingroup ge_graph
  /// @brief memory allocator manager init
  /// @param [in] options user config params
  /// @return Status result of function
  ///
  Status Initialize(const std::vector<rtMemType_t> &memory_type);

  ///
  /// @ingroup ge_graph
  /// @brief memory allocator finalize
  /// @return void
  ///
  void Finalize() noexcept;

 private:
  MemManager(const MemManager &) = delete;
  MemManager &operator=(const MemManager &) = delete;

  ///
  /// @ingroup ge_graph
  /// @param [in] memory_type memory type
  /// @param [in] allocate_map memory allocator map
  /// @return Status result of function
  ///
  template <typename T>
  static Status InitAllocator(const std::vector<rtMemType_t> &memory_type, std::map<rtMemType_t, T *> &allocate_map) {
    T *allocator = nullptr;
    for (const rtMemType_t index : memory_type) {
      const auto it = allocate_map.find(index);
      if (it == allocate_map.end()) {
        allocator = new (std::nothrow) T(index);
        if (allocator != nullptr) {
          allocate_map[index] = allocator;
          GELOGI("Create Allocator memory type[%u] success.", index);
        } else {
          REPORT_CALL_ERROR("E19999", "New MemoryAllocator fail, index:%u", index);
          GELOGE(ACL_ERROR_GE_MEMORY_ALLOCATION, "Alloc Allocator failed.");
        }
      } else {
        allocator = it->second;
      }

      if (allocator == nullptr) {
        GELOGE(ACL_ERROR_GE_MEMORY_ALLOCATION, "Create Allocator failed.");
        return ACL_ERROR_GE_MEMORY_ALLOCATION;
      } else {
        if (allocator->Initialize() != SUCCESS) {
          return ACL_ERROR_GE_INTERNAL_ERROR;
        }
      }
    }
    return SUCCESS;
  }
  ///
  /// @ingroup ge_graph
  /// @param [in] memory_type memory type
  /// @param [in] allocate_map memory allocator map
  /// @return Allocator ptr
  ///
  template <typename T>
  T &GetAllocator(const rtMemType_t memory_type, const std::map<rtMemType_t, T *> allocate_map) {
    const std::lock_guard<std::recursive_mutex> lock(allocator_mutex_);
    T *allocator = nullptr;
    const auto it = allocate_map.find(memory_type);
    if (it != allocate_map.end()) {
      allocator = it->second;
    }

    // Usually impossible
    if (allocator == nullptr) {
      GELOGW("Get allocator failed, memory type is %u.", memory_type);
      static T default_allocator(RT_MEMORY_RESERVED);
      return default_allocator;
    }
    return *allocator;
  }

  template <typename T>
  void FinalizeAllocatorMap(std::map<rtMemType_t, T *> &allocate_map) const;

  std::map<rtMemType_t, MemoryAllocator *> memory_allocator_map_;
  std::map<rtMemType_t, CachingAllocator *> caching_allocator_map_;
  std::map<rtMemType_t, RdmaPoolAllocator *> rdma_allocator_map_;
  std::map<rtMemType_t, HostMemAllocator *> host_allocator_map_;
  std::map<rtMemType_t, SessionScopeMemAllocator *> session_scope_allocator_map_;
  std::recursive_mutex allocator_mutex_;
  std::vector<rtMemType_t> memory_type_;
  bool init_ = false;
};
}  // namespace ge

#endif  // GE_GRAPH_MANAGER_GRAPH_MEM_ALLOCATOR_H_
