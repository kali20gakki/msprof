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

#ifndef GE_GRAPH_LOAD_TS_MEM_MALL_H_
#define GE_GRAPH_LOAD_TS_MEM_MALL_H_

#include <mutex>
#include <unordered_map>
#include <memory>

#include "runtime/base.h"
#include "framework/common/debug/ge_log.h"
#include "runtime/mem.h"

namespace ge {
class TsMemMall {
 public:
  TsMemMall() : mem_type_(RT_MEMORY_TS) {}
  explicit TsMemMall(const rtMemType_t type) : mem_type_(type) {}

  ~TsMemMall() {
    for (auto it : mem_store_size_) {
      GE_FREE_RT_LOG(it.second);
    }
    mem_store_size_.clear();
    mem_store_addr_.clear();
  }

  void *Acquire(const int64_t offset, const uint64_t size) {
    constexpr uint32_t kMaxTsMemBlock = 2097152U;   // Max block 2M 2 * 1024 * 1024
    constexpr uint64_t kTsMemAligment = 64U;   // Malloc for 64 bits align
    constexpr uint64_t kTsMemAlignMask = kTsMemAligment - 1U;
    if (size == 0U) {
      GELOGE(RT_FAILED, "[Check][Param] Acquire mem block failed, size:%lu", size);
      return nullptr;
    }

    const uint64_t bytes = (size + kTsMemAlignMask) & ~kTsMemAlignMask;
    if (bytes > kMaxTsMemBlock) {
      GELOGW("Acquire TS memory may not physical continuity, size: %lu", bytes);
    }

    const std::lock_guard<std::mutex> lock(mem_mutex_);
    const std::map<int64_t, void *>::const_iterator it = mem_store_size_.find(offset);
    if (it != mem_store_size_.cend()) {
      GELOGI("Acquire TS memory: %p, offset: %ld, size: %lu, align: %lu", it->second, offset, size, bytes);
      return it->second;
    }

    void *addr = nullptr;
    GE_CHK_RT_EXEC(rtMalloc(&addr, bytes, mem_type_), return nullptr);

    GELOGI("Acquire TS memory: %p, offset: %ld, size: %lu, align: %lu", addr, offset, size, bytes);
    mem_store_size_[offset] = addr;
    mem_store_addr_[addr] = offset;
    return addr;
  }

 private:
  std::mutex mem_mutex_;
  std::map<int64_t, void *> mem_store_size_;
  std::map<void *, int64_t> mem_store_addr_;
  rtMemType_t mem_type_{RT_MEMORY_TS};
};
}  // namespace ge
#endif  // GE_GRAPH_LOAD_TS_MEM_MALL_H_
