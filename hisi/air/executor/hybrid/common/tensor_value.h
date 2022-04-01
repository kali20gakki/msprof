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

#ifndef GE_HYBRID_COMMON_TENSOR_VALUE_H_
#define GE_HYBRID_COMMON_TENSOR_VALUE_H_

#include <atomic>
#include "framework/memory/memory_api.h"
#include "framework/common/util.h"
#include "hybrid/common/npu_memory_allocator.h"

namespace ge {
namespace hybrid {

class TensorBuffer {
 public:
  static std::unique_ptr<TensorBuffer> Create(NpuMemoryAllocator * const allocator,
                                              const size_t size,
                                              const AllocationAttr * const attr = nullptr);

  static std::unique_ptr<TensorBuffer> Create(void *const buffer, const size_t size);

  TensorBuffer(NpuMemoryAllocator *const allocator, void *const buffer, const size_t size,
               const MemStorageType mem_type = MemStorageType::HBM);
  TensorBuffer(const TensorBuffer &) = delete;
  TensorBuffer &operator = (const TensorBuffer &) = delete;
  ~TensorBuffer();

  void* Release() {
    const auto ret = buffer_;
    buffer_ = nullptr;
    return ret;
  }

  void *GetData() const {
    return buffer_;
  }

  size_t GetSize() const {
    return size_;
  }

  MemStorageType GetMemType() const {
    return mem_type_;
  }

 private:
  NpuMemoryAllocator *allocator_ = nullptr;
  void *buffer_ = nullptr;
  size_t size_ = 0U;
  MemStorageType mem_type_;
};

class TensorValue {
 public:
  TensorValue() = default;

  explicit TensorValue(const std::shared_ptr<TensorBuffer> buffer);

  TensorValue(void *const buffer, const size_t size, const MemStorageType mem_type = MemStorageType::HOST_DDR);

  ~TensorValue();

  void Destroy();

  void *Release() {
    return buffer_->Release();
  }

  const void *GetData() const;

  std::string DebugString() const;

  void SetName(const std::string &name) {
    name_ = name;
  }

  MemStorageType GetMemType() const {
    return mem_type_;
  }

  void *MutableData();

  size_t GetSize() const;

  template<typename T>
  Status CopyScalarValueToHost(T &value) const {
    GE_CHECK_GE(this->GetSize(), sizeof(value));
    GE_CHK_RT_RET(rtMemcpy(PtrToPtr<T, void>(&value), sizeof(value), this->GetData(), sizeof(value),
                           RT_MEMCPY_DEVICE_TO_HOST));
    return SUCCESS;
  }

 private:
  std::shared_ptr<TensorBuffer> buffer_;
  std::string name_;
  // for weights and variables
  void *ref_buffer_ = nullptr;
  size_t ref_size_ = 0U;
  MemStorageType mem_type_ = MemStorageType::HBM;
  // shape
};
}  // namespace hybrid
}  // namespace ge
#endif  // GE_HYBRID_COMMON_TENSOR_VALUE_H_
