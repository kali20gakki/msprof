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

#ifndef GE_GRAPH_MANAGER_BLOCK_MEMORY_H_
#define GE_GRAPH_MANAGER_BLOCK_MEMORY_H_

#include <cstdint>
#include <set>
#include <functional>

namespace ge {
struct Block;
using Comparison = std::function<bool(const Block *, const Block *)>;
using BlockBin = std::set<Block *, Comparison>;

struct Block {
  uint32_t device_id{0U};  // npu device id
  size_t size{0U};         // block size in bytes
  BlockBin *bin{nullptr};  // owning block bin
  uint8_t *ptr{nullptr};   // memory address
  bool allocated{false};   // in-use flag
  Block *prev{nullptr};    // prev block if split from a larger allocation
  Block *next{nullptr};    // next block if split from a larger allocation

  Block(const uint32_t device, const size_t block_size, BlockBin *const block_bin, uint8_t *const mem_addr)
      : device_id(device), size(block_size), bin(block_bin), ptr(mem_addr) {}

  // constructor for search key
  Block(const uint32_t device, const size_t block_size, uint8_t *const mem_addr)
      : device_id(device), size(block_size), ptr(mem_addr) {}

  bool IsSplit() const { return (prev != nullptr) || (next != nullptr); }
};
}  // namespace ge
#endif  // GE_GRAPH_MANAGER_BLOCK_MEMORY_H_
