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

#ifndef GE_HYBRID_COMMON_NPU_MEMORY_ALLOCATOR_H_
#define GE_HYBRID_COMMON_NPU_MEMORY_ALLOCATOR_H_

#include <mutex>
#include "framework/memory/memory_api.h"
#include "graph/manager/graph_caching_allocator.h"

namespace ge {
namespace hybrid {
class AllocationAttr {
 public:
  AllocationAttr() = default;
  explicit AllocationAttr(const int32_t padding);
  explicit AllocationAttr(void *const try_reuse_addr);
  AllocationAttr(const int32_t padding, void *const try_reuse_addr,
                 const MemStorageType mem_type = MemStorageType::HBM);
  ~AllocationAttr() = default;
  void SetMemType(const MemStorageType memType) { mem_type_ = memType; }
  MemStorageType GetMemType() const { return mem_type_; }

 private:
  friend class NpuMemoryAllocator;
  int32_t padding_ = 0;
  void *try_reuse_addr_ = nullptr;
  MemStorageType mem_type_ = MemStorageType::HBM;
};

class NpuMemoryAllocator {
 public:
  NpuMemoryAllocator(const uint32_t device_id, const rtStream_t stream);
  ~NpuMemoryAllocator();
  static NpuMemoryAllocator *GetAllocator(const uint32_t device_id, const rtStream_t stream);
  static NpuMemoryAllocator *GetAllocator(const rtStream_t stream);
  static NpuMemoryAllocator *GetAllocator();
  static void Finalize();
  static void FreeCachedMem();
  static void ClearStream(const rtStream_t stream);

  static AllocationAttr* AttrWithDefaultPadding() {
    static AllocationAttr attr(kDefaultPadding, nullptr);
    return &attr;
  }

  Status InitCachingllocator();
  void *Allocate(const std::size_t size, const AllocationAttr *const attr = nullptr) const;
  void Deallocate(void *const data, const MemStorageType mem_storage_type = MemStorageType::HBM) const;

  static constexpr int32_t kDefaultPadding = 32;
 private:
  Status TryFreeAndMalloc(const size_t size, void **buffer) const;
  Status TryFreeCachingMem() const;
  void *AllocateCachingMem(const std::size_t size, void *const try_reuse_addr) const;

  uint32_t device_id_;
  rtStream_t stream_;
  std::unique_ptr<CachingAllocator> caching_allocator_;

  using DeviceidAllocatorMap = std::map<uint32_t, std::unique_ptr<NpuMemoryAllocator>>;
  static DeviceidAllocatorMap default_allocators_;
  static std::map<rtStream_t, std::unique_ptr<DeviceidAllocatorMap>> allocators_;
  static std::set<rtStream_t> streams_;
  static std::mutex mu_;
};
}  // namespace hybrid
}  // namespace ge
#endif // GE_HYBRID_COMMON_NPU_MEMORY_ALLOCATOR_H_
