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

#include "hybrid/common/npu_memory_allocator.h"
#include <mutex>
#include "common/plugin/ge_util.h"
#include "framework/common/debug/log.h"
#include "graph/def_types.h"
#include "graph/ge_context.h"
#include "graph/manager/graph_mem_manager.h"
#include "runtime/rt.h"

namespace ge {
namespace hybrid {
namespace {
constexpr size_t kPaddingUnit = 2U;
constexpr size_t kMaxHbmMemorySize = 1099511627776UL; // 1024UL * 1024UL * 1024UL * 1024UL; // 1024G
constexpr uint32_t kDeviceIdHost = static_cast<uint32_t>(-1);
}  // namespace

NpuMemoryAllocator::DeviceidAllocatorMap NpuMemoryAllocator::default_allocators_;
std::map<rtStream_t, std::unique_ptr<NpuMemoryAllocator::DeviceidAllocatorMap>> NpuMemoryAllocator::allocators_;
std::set<rtStream_t> NpuMemoryAllocator::streams_;
std::mutex NpuMemoryAllocator::mu_;

AllocationAttr::AllocationAttr(const int32_t padding, void *const try_reuse_addr,
                               const MemStorageType mem_type)
    : padding_(padding), try_reuse_addr_(try_reuse_addr), mem_type_(mem_type) {}
AllocationAttr::AllocationAttr(const int32_t padding) : AllocationAttr(padding, nullptr) {}
AllocationAttr::AllocationAttr(void *const try_reuse_addr) : AllocationAttr(0, try_reuse_addr) {}

NpuMemoryAllocator::~NpuMemoryAllocator() {
  if (caching_allocator_ != nullptr) {
    GELOGI("NpuMemoryAllocator caching_allocator start finalize, stream: %p.", stream_);
    caching_allocator_->Finalize();
    caching_allocator_.reset(nullptr);
  }
}

void NpuMemoryAllocator::Finalize() {
  const std::lock_guard<std::mutex> lk(mu_);
  GELOGI("NpuMemoryAllocators start finalize.");
  for (auto &map_pair : allocators_) {
    auto &allocator_map = map_pair.second;
    if (allocator_map != nullptr) {
      auto &map = *(allocator_map);
      for (auto &alloctor_pair : map) {
        auto &allocator = alloctor_pair.second;
        if (allocator != nullptr) {
          auto &caching_allocator = allocator->caching_allocator_;
          if (caching_allocator != nullptr) {
            caching_allocator->Finalize();
            caching_allocator.reset(nullptr);
          }
        }
      }
    }
  }
}

void NpuMemoryAllocator::FreeCachedMem() {
  const std::lock_guard<std::mutex> lk(mu_);
  GELOGI("NpuMemoryAllocators start free cached mem.");
  for (auto &map_pair : allocators_) {
    auto &allocator_map = map_pair.second;
    if (allocator_map != nullptr) {
      auto &map = *(allocator_map);
      for (auto &alloctor_pair : map) {
        auto &allocator = alloctor_pair.second;
        if (allocator != nullptr) {
          auto &caching_allocator = allocator->caching_allocator_;
          if (caching_allocator != nullptr) {
            caching_allocator->TryFreeBlocks();
          }
        }
      }
    }
  }
}

NpuMemoryAllocator *NpuMemoryAllocator::GetAllocator() {
  int32_t device_id = 0;
  const auto rt_result = rtGetDevice(&device_id);
  if (rt_result != RT_ERROR_NONE) {
    GELOGE(RT_FAILED, "[Get][Device] Failed, result:%d.", rt_result);
    REPORT_INNER_ERROR("E19999", "rtGetDevice failed, result:%d.", rt_result);
    return nullptr;
  }

  GELOGD("Got device id = %d from context", device_id);
  const std::lock_guard<std::mutex> lk(mu_);
  std::map<uint32_t, std::unique_ptr<NpuMemoryAllocator>>::const_iterator allocator_it =
      default_allocators_.find(static_cast<size_t>(device_id));
  if (allocator_it == default_allocators_.cend()) {
    auto allocator = MakeUnique<NpuMemoryAllocator>(device_id, nullptr);
    if (allocator == nullptr) {
      REPORT_CALL_ERROR("E19999", "New default NpuMemoryAllocator fail, device_id: %u.", device_id);
      GELOGE(ACL_ERROR_GE_MEMORY_ALLOCATION, "New default NpuMemoryAllocator fail, device_id,: %u.", device_id);
      return nullptr;
    }
    default_allocators_[static_cast<size_t>(device_id)] = std::move(allocator);
    GELOGI("Create default NpuMemoryAllocator, device_id: %u.", device_id);
  }

  return default_allocators_[static_cast<size_t>(device_id)].get();
}

NpuMemoryAllocator *NpuMemoryAllocator::GetAllocator(const rtStream_t stream) {
  int32_t device_id = 0;
  const auto rt_result = rtGetDevice(&device_id);
  if (rt_result != RT_ERROR_NONE) {
    GELOGE(RT_FAILED, "[Get][Device] Failed, result:%d.", rt_result);
    REPORT_INNER_ERROR("E19999", "rtGetDevice failed, result:%d.", rt_result);
    return nullptr;
  }

  GELOGD("Got device id = %d from context", device_id);
  return GetAllocator(static_cast<uint32_t>(device_id), stream);
}

NpuMemoryAllocator::NpuMemoryAllocator(const uint32_t device_id, const rtStream_t stream)
    : device_id_(device_id), stream_(stream) {}

Status NpuMemoryAllocator::TryFreeCachingMem() const {
  if (caching_allocator_ != nullptr) {
    GE_CHK_STATUS_RET(caching_allocator_->FreeBlocksAfterSynchronize(stream_),
                      "Stream synchronize failed! stream: %p", stream_);
  }
  return SUCCESS;
}

Status NpuMemoryAllocator::TryFreeAndMalloc(const size_t size, void **buffer) const {
  GE_CHECK_NOTNULL(buffer);
  // first try sync current stream and free blocks.
  if (caching_allocator_ != nullptr) {
    GE_CHK_STATUS_RET_NOLOG(TryFreeCachingMem());
    (*buffer) = AllocateCachingMem(size, nullptr);
  }

  // try sync all stream and free blocks.
  if ((*buffer) == nullptr) {
    for (const auto &stream : streams_) {
      const auto &allocator = GetAllocator(device_id_, stream);
      GE_CHECK_NOTNULL(allocator);
      GE_CHK_STATUS_RET_NOLOG(allocator->TryFreeCachingMem());

      (*buffer) = AllocateCachingMem(size, nullptr);
      if ((*buffer) != nullptr) {
        return SUCCESS;
      }
    }
  }

  return SUCCESS;
}

Status NpuMemoryAllocator::InitCachingllocator() {
  if (caching_allocator_ == nullptr) {
    caching_allocator_ = MakeUnique<CachingAllocator>(RT_MEMORY_HBM);
    if (caching_allocator_ == nullptr) {
      REPORT_CALL_ERROR("E19999", "New stream allocator map fail, stream: %p.", stream_);
      GELOGE(ACL_ERROR_GE_MEMORY_ALLOCATION, "New stream allocator map fail, stream: %p.", stream_);
      return ACL_ERROR_GE_MEMORY_ALLOCATION;
    }
  }

  caching_allocator_->SetBindStream(true);
  GE_CHK_STATUS_RET(caching_allocator_->Initialize(), "Allocator initialize failed, stream: %p.", stream_);
  return SUCCESS;
}

void *NpuMemoryAllocator::AllocateCachingMem(const std::size_t size, void *const try_reuse_addr) const {
  void *buffer = nullptr;
  if (caching_allocator_ != nullptr) {
    buffer = caching_allocator_->Malloc(size, PtrToPtr<void, uint8_t>(try_reuse_addr), device_id_);
  } else {
    rtError_t rt_ret = rtMalloc(&buffer, size, RT_MEMORY_HBM); // check buffer null after call
    if (rt_ret != RT_ERROR_NONE) {
      GELOGE(rt_ret, "[Call][rtMalloc] failed, size:%lu, ret = 0x%X", size, rt_ret);
      return nullptr;
    }
  }

  return buffer;
}

void *NpuMemoryAllocator::Allocate(const std::size_t size, const AllocationAttr *const attr) const {
  size_t allocate_size = size;
  MemStorageType mem_type = MemStorageType::HBM;
  if (device_id_ == kDeviceIdHost) {
    mem_type = MemStorageType::HOST_DDR;
  } else {
    if (attr != nullptr) {
      mem_type = attr->mem_type_;
    }
  }

  if (allocate_size == 0U) {
    GELOGE(MEMALLOC_FAILED, "[Check][Param:size_t]Memory size is 0, device_id = %u, size = %zu.",
        device_id_, allocate_size);
    REPORT_INNER_ERROR("E19999", "Memory size is 0, device_id = %u, size = %zu.", device_id_, allocate_size);
    return nullptr;
  }

  void *buffer = nullptr;
  if (mem_type == MemStorageType::RDMA_HBM) {
    buffer = MemManager::Instance().RdmaPoolInstance(RT_MEMORY_HBM).Malloc(allocate_size, device_id_);
  } else if (mem_type == MemStorageType::HOST_DDR) {
    buffer = MemManager::Instance().HostMemInstance(RT_MEMORY_HBM).Malloc(allocate_size);
  } else {
    if (allocate_size > kMaxHbmMemorySize) {
      GELOGE(PARAM_INVALID, "[Check][Param:size_t]Invalid HBM memory size: %zu bigger than limit:%zu, check invalid.",
             allocate_size, kMaxHbmMemorySize);
      REPORT_CALL_ERROR("E19999", "Invalid HBM memory size: %zu bigger than limit:%zu, check invalid.",
                        allocate_size, kMaxHbmMemorySize);
      return nullptr;
    }
    void *try_reuse_addr = nullptr;
    int32_t padding = kDefaultPadding;
    if (attr != nullptr) {
      try_reuse_addr = attr->try_reuse_addr_;
      if (attr->padding_ > 0) {
        padding = attr->padding_;
      }
    }
    // padding up to multiple of padding, and add extra padding
    allocate_size = ((size + (kPaddingUnit * static_cast<size_t>(padding)) - 1U) / static_cast<size_t>(padding)) *
        static_cast<size_t>(padding);
    GELOGD("Padding size %zu by %d. final size = %zu.", size, padding, allocate_size);
    buffer = AllocateCachingMem(allocate_size, try_reuse_addr);
    if (buffer == nullptr) {
      (void)TryFreeAndMalloc(allocate_size, &buffer); // check buffer nullptr after call
    }
  }
  if (buffer == nullptr) {
    GELOGE(MEMALLOC_FAILED, "[Malloc][Memory] Failed, device_id = %u, size = %zu",
           device_id_, allocate_size);
    REPORT_CALL_ERROR("E19999", "malloc memory failed, device_id = %u, size = %zu",
                      device_id_, allocate_size);
    return nullptr;
  }

  GELOGI("Allocating buffer of size %zu successfully. device_id = %u, address = %p", allocate_size, device_id_, buffer);
  return buffer;
}

void NpuMemoryAllocator::Deallocate(void *const data, const MemStorageType mem_storage_type) const {
  GELOGI("To deallocating buffer, addr = %p", data);
  if (data != nullptr) {
    const auto mem_type = (device_id_ == kDeviceIdHost) ? MemStorageType::HOST_DDR : mem_storage_type;
    GELOGI("Deallocating buffer successfully. addr = %p", data);
    if (mem_type == MemStorageType::RDMA_HBM) {
      (void)MemManager::Instance().RdmaPoolInstance(RT_MEMORY_HBM).Free(PtrToPtr<void, uint8_t>(data), device_id_);
    } else if (mem_type == MemStorageType::HOST_DDR) {
      (void)MemManager::Instance().HostMemInstance(RT_MEMORY_HBM).Free(data);
    } else if (caching_allocator_ != nullptr) {
      (void)caching_allocator_->Free(PtrToPtr<void, uint8_t>(data), device_id_);
    } else {
      (void)rtFree(data);
    }
  }
}

NpuMemoryAllocator *NpuMemoryAllocator::GetAllocator(const uint32_t device_id, const rtStream_t stream) {
  const std::lock_guard<std::mutex> lk(mu_);
  const std::map<rtStream_t, std::unique_ptr<NpuMemoryAllocator::DeviceidAllocatorMap>>::const_iterator map_it =
      allocators_.find(stream);
  if (map_it == allocators_.cend()) {
    auto allocator_map = MakeUnique<DeviceidAllocatorMap>();
    if (allocator_map == nullptr) {
      REPORT_CALL_ERROR("E19999", "New NpuMemoryAllocator map fail, stream: %p.", stream);
      GELOGE(ACL_ERROR_GE_MEMORY_ALLOCATION, "New NpuMemoryAllocator fail, stream: %p.", stream);
      return nullptr;
    }
    (void)streams_.insert(stream);
    allocators_[stream] = std::move(allocator_map);
    GELOGI("Create NpuMemoryAllocator map, stream: %p.", stream);
  }

  auto &allocator_map = *(allocators_[stream]);
  const auto allocator_it = allocator_map.find(device_id);
  if (allocator_it == allocator_map.end()) {
    auto allocator = MakeUnique<NpuMemoryAllocator>(device_id, stream);
    if ((allocator == nullptr) || (allocator->InitCachingllocator() != SUCCESS)) {
      REPORT_CALL_ERROR("E19999", "New NpuMemoryAllocator fail, device_id: %u, stream: %p.", device_id, stream);
      GELOGE(ACL_ERROR_GE_MEMORY_ALLOCATION, "New NpuMemoryAllocator fail, device_id,: %u, stream: %p.",
             device_id, stream);
      return nullptr;
    }
    allocator_map[device_id] = std::move(allocator);
    GELOGI("Create NpuMemoryAllocator, device_id: %u, stream: %p.", device_id, stream);
  }

  return allocator_map[device_id].get();
}

void NpuMemoryAllocator::ClearStream(const rtStream_t stream) {
  const std::lock_guard<std::mutex> lk(mu_);
  GELOGI("Clear stream: %p in NpuMemoryAllocator", stream);
  if (allocators_.find(stream) != allocators_.cend()) {
    for (auto &allocator_iter : *(allocators_[stream])) {
      allocator_iter.second->TryFreeCachingMem();
    }
  }
  streams_.erase(stream);
}

}  // namespace hybrid
}  // namespace ge
