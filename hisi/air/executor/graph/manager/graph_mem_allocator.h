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

#ifndef GE_GRAPH_MANAGER_GRAPH_MEM_ALLOCATOR_H_
#define GE_GRAPH_MANAGER_GRAPH_MEM_ALLOCATOR_H_

#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <unordered_map>

#include "framework/common/debug/ge_log.h"
#include "graph/node.h"
#include "runtime/mem.h"

namespace ge {
class MemoryInfo {
 public:
  MemoryInfo() {}

  MemoryInfo(uint8_t *const memory_addr, const size_t memory_size, const uint32_t device_id = 0U)
      : memory_addr_(memory_addr), memory_size_(memory_size), device_id_(device_id) {}

  virtual ~MemoryInfo() = default;

  MemoryInfo &operator=(const MemoryInfo &op) {
    if (&op == this) {
      return *this;
    }

    this->memory_addr_ = op.memory_addr_;
    this->memory_size_ = op.memory_size_;
    this->memory_used_num_ = op.memory_used_num_;
    this->device_id_ = op.device_id_;
    return *this;
  }

  MemoryInfo(const MemoryInfo &op) {
    this->memory_addr_ = op.memory_addr_;
    this->memory_size_ = op.memory_size_;
    this->memory_used_num_ = op.memory_used_num_;
    this->device_id_ = op.device_id_;
  }

 private:
  uint8_t *memory_addr_{nullptr};
  uint64_t memory_size_{0U};
  int32_t memory_used_num_{0};
  uint32_t device_id_{0U};
  friend class MemoryAllocator;
};

class MemoryAllocator {
 public:
  explicit MemoryAllocator(const rtMemType_t memory_type) : memory_type_(memory_type) {}

  virtual ~MemoryAllocator() = default;

  ///
  /// @ingroup ge_graph
  /// @brief memory allocator init
  /// @param [in] options user config params
  /// @return Status of init
  ///
  Status Initialize();

  ///
  /// @ingroup ge_graph
  /// @brief memory allocator finalize
  /// @return void
  ///
  void Finalize();

  ///
  /// @ingroup ge_graph
  /// @brief malloc memory
  /// @param [in] purpose memory usage
  /// @param [in] size memory size
  /// @param [in] device_id device id
  /// @return  memory address
  ///
  uint8_t *MallocMemory(const std::string &purpose, const size_t memory_size, const uint32_t device_id = 0U) const;

  ///
  /// @ingroup ge_graph
  /// @brief free memory
  /// @param [in] device_id device id
  /// @param [out] memory_ptr memory address ptr
  /// @return Status result of function
  ///
  Status FreeMemory(void *memory_addr, const uint32_t device_id = 0U) const;

  ///
  /// @ingroup ge_graph
  /// @brief malloc memory
  /// @param [in] purpose memory usage
  /// @param [in] memory_key memory key
  /// @param [in] size memory size
  /// @param [in] device_id device id
  /// @return memory address
  ///
  uint8_t *MallocMemory(const std::string &purpose, const std::string &memory_key, const size_t memory_size,
                        const uint32_t device_id = 0U);

  ///
  /// @ingroup ge_graph
  /// @brief free memory
  /// @param [in] memory_key memory key
  /// @param [in] device_id device id
  /// @return Status result of function
  ///
  Status FreeMemory(const std::string &memory_key, const uint32_t device_id = 0U);

  ///
  /// @ingroup ge_graph
  /// @brief get memory address
  /// @param [in] memory_key memory key
  /// @param [in] device_id device id
  /// @return memory address (must not free memory by it)
  ///
  uint8_t *GetMemoryAddr(const std::string &memory_key, const uint32_t device_id = 0U);

  ///
  /// @ingroup ge_graph
  /// @brief get max memory malloced
  /// @param [in] memory_key memory key
  /// @param [in] device_id device id
  /// @return max memory malloced
  ///
  int64_t MemorySize(const std::string &memory_key, const uint32_t device_id = 0U) const;

 private:
  rtMemType_t memory_type_;
  bool mem_malloced_{false};
  std::map<uint32_t, map<std::string, MemoryInfo>> deviceid_2_memory_bases_map_;
};

class SessionMemAllocator {
 public:
  SessionMemAllocator();
  ~SessionMemAllocator();

  // forbidden copy
  SessionMemAllocator(const SessionMemAllocator &) = delete;
  SessionMemAllocator &operator=(const SessionMemAllocator &) = delete;

  // forbidden move
  SessionMemAllocator(const SessionMemAllocator &&) = delete;
  SessionMemAllocator &operator=(const SessionMemAllocator &&) = delete;

  static SessionMemAllocator &Instance();
  MemoryAllocator &GetMemAllocator(const uint64_t session_id);

 private:
  std::mutex map_mutex_;
  std::unordered_map<uint64_t, MemoryAllocator *> session_allocator_map_;
};
}  // namespace ge

#endif  // GE_GRAPH_MANAGER_GRAPH_MEM_ALLOCATOR_H_
