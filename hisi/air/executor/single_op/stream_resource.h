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

#ifndef GE_SINGLE_OP_STREAM_RESOURCE_H_
#define GE_SINGLE_OP_STREAM_RESOURCE_H_

#include <cstdint>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "framework/common/ge_inner_error_codes.h"
#include "runtime/stream.h"
#include "single_op/single_op.h"
#include "hybrid/executor/node_done_manager.h"
#include "graph/object_pool.h"

namespace ge {
class StreamResource {
 public:
  explicit StreamResource(const uintptr_t resource_id);
  ~StreamResource();

  StreamResource(const StreamResource &) = delete;
  StreamResource(StreamResource &&) = delete;
  StreamResource &operator=(const StreamResource &) = delete;
  StreamResource &operator=(StreamResource &&) = delete;
  rtStream_t GetStream() const;
  void SetStream(const rtStream_t stream);

  Status Init();
  SingleOp *GetOperator(const uint64_t key);
  Status DeleteOperator(const uint64_t key);
  Status DeleteDynamicOperator(const uint64_t key);
  DynamicSingleOp *GetDynamicOperator(const uint64_t key);

  Status BuildOperator(const ModelData &model_data, SingleOp **const single_op, const uint64_t model_id);
  Status BuildDynamicOperator(const ModelData &model_data, DynamicSingleOp **const single_op, const uint64_t model_id);

  Status MallocOverflowMemory(const std::string &purpose, const int64_t size);
  uint8_t *MallocMemory(const std::string &purpose, const size_t size, const bool holding_lock = true);
  uint8_t *MallocWeight(const std::string &purpose, const size_t size);
  uint8_t *GetMemoryBase() const;
  void *GetDeviceBufferAddr() const {
    return static_cast<void *>(device_buffer_);
  }

  Status GetThreadPool(ThreadPool **const thread_pool);
  Status GetCallbackManager(hybrid::CallbackManager **const callback_manager);
  
  void *GetOverflowAddr() const {
    return overflow_addr_;
  }

  int64_t GetOverflowSize() const {
    return globalworkpace_overflow_size_;
  }

 private:
  uint8_t *DoMallocMemory(const std::string &purpose, const size_t size,
                          size_t &max_allocated, std::vector<uint8_t *> &allocated) const;

  uintptr_t resource_id_;
  size_t max_memory_size_ = 0U;
  std::vector<uint8_t *> memory_list_;
  std::vector<uint8_t *> weight_list_;
  std::unordered_map<uint64_t, std::unique_ptr<SingleOp>> op_map_;
  std::unordered_map<uint64_t, std::unique_ptr<DynamicSingleOp>> dynamic_op_map_;
  std::unique_ptr<ThreadPool> thread_pool_;
  std::unique_ptr<hybrid::CallbackManager> callback_manager_;
  ObjectPool<GeTensor> tensor_pool_;
  rtStream_t stream_ = nullptr;
  std::mutex mu_;
  std::mutex stream_mu_;
  void *device_buffer_ = nullptr;
  void *overflow_addr_ = nullptr;
  int64_t globalworkpace_overflow_size_ = 0;
};
}  // namespace ge

#endif  // GE_SINGLE_OP_STREAM_RESOURCE_H_
