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

#ifndef GE_GRAPH_MANAGER_RDMA_POOL_ALLOCATOR_H_
#define GE_GRAPH_MANAGER_RDMA_POOL_ALLOCATOR_H_

#include <iostream>
#include <mutex>
#include <unordered_map>

#include "graph/manager/block_memory.h"
#include "graph/manager/graph_mem_allocator.h"
#include "graph/node.h"
#include "runtime/mem.h"

namespace ge {
class RdmaPoolAllocator {
 public:
  explicit RdmaPoolAllocator(const rtMemType_t memory_type);

  RdmaPoolAllocator(const RdmaPoolAllocator &) = delete;

  RdmaPoolAllocator &operator=(const RdmaPoolAllocator &) = delete;

  ~RdmaPoolAllocator() = default;

  Status Initialize();
  void Finalize();

  Status InitMemory(const size_t mem_size);

  uint8_t *Malloc(const size_t size, const uint32_t device_id = 0U);

  Status Free(uint8_t *const memory_addr, const uint32_t device_id = 0U);

  Status GetBaseAddr(uint64_t &base_addr, uint64_t &mem_size) const;

  uint8_t *GetRdmaBaseAddr() const { return rdma_base_addr_; }

 private:
  void MergeBlocks(Block &dst, Block &src);

  rtMemType_t memory_type_;
  size_t rdma_mem_size_ = 0U;  // Total rdma memory size to be allocated.
  uint8_t *rdma_base_addr_ = nullptr;
  MemoryAllocator *memory_allocator_ = nullptr;
  BlockBin block_bin_;  // Save all rdma blocks.
  std::unordered_map<uint8_t *, Block *> allocated_blocks_;
  // lock around all operations
  mutable std::recursive_mutex mutex_;
};
}  // namespace ge

#endif  // GE_GRAPH_MANAGER_RDMA_POOL_ALLOCATOR_H_
