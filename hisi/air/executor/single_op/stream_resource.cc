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

#include "single_op/stream_resource.h"

#include "framework/common/debug/ge_log.h"
#include "framework/common/debug/log.h"
#include "runtime/rt.h"
#include "single_op/single_op_model.h"

namespace ge {
namespace {
// limit available device mem size  1M
constexpr int32_t kThreadNumDefault = 8;
}

StreamResource::StreamResource(const uintptr_t resource_id) : resource_id_(resource_id) {
}

StreamResource::~StreamResource() {
  for (const auto mem : memory_list_) {
    if (mem != nullptr) {
      const auto rt_ret = rtFree(mem);
      GE_IF_BOOL_EXEC(rt_ret != RT_ERROR_NONE, GELOGE(RT_FAILED, "[Free][Rt] failed."));
    }
  }

  for (const auto weight : weight_list_) {
    if (weight != nullptr) {
      const auto rt_ret = rtFree(weight);
      GE_IF_BOOL_EXEC(rt_ret != RT_ERROR_NONE, GELOGE(RT_FAILED, "[Free][Rt] failed."));
    }
  }

  if (device_buffer_ != nullptr) {
    const auto rt_ret = rtFree(device_buffer_);
    GE_IF_BOOL_EXEC(rt_ret != RT_ERROR_NONE, GELOGE(RT_FAILED, "[Free][Rt] failed."));
  }

  // Free mem for overflow detection
  if (overflow_addr_ != nullptr) {
    const auto rt_ret = rtFree(overflow_addr_);
    GE_IF_BOOL_EXEC(rt_ret != RT_ERROR_NONE, GELOGE(RT_FAILED, "[Free][Rt] failed."));
  }

  if (callback_manager_ != nullptr) {
    (void)callback_manager_->Destroy();
  }
}

Status StreamResource::Init() {
  const auto rt_ret = rtMalloc(&device_buffer_, kFuzzDeviceBufferSize, RT_MEMORY_HBM);
  GE_IF_BOOL_EXEC(rt_ret != RT_ERROR_NONE, GELOGE(RT_FAILED, "[Malloc][Rt] failed."));
  return SUCCESS;
}

SingleOp *StreamResource::GetOperator(const uint64_t key) {
  const std::lock_guard<std::mutex> lk(mu_);
  const auto it = op_map_.find(key);
  if (it == op_map_.end()) {
    return nullptr;
  }
  return it->second.get();
}

Status StreamResource::DeleteOperator(const uint64_t key) {
  const std::lock_guard<std::mutex> lk(mu_);
  const auto it = op_map_.find(key);
  if (it != op_map_.end()) {
    // need to stream sync before erase
    GELOGI("static op %lu need to be deleted, start to sync stream %p", key, stream_);
    GE_CHK_RT_RET(rtStreamSynchronize(stream_));
    (void)op_map_.erase(it);
    GELOGI("static op %lu delete success", key);
  }
  return SUCCESS;
}

Status StreamResource::DeleteDynamicOperator(const uint64_t key) {
  const std::lock_guard<std::mutex> lk(mu_);
  const auto it = dynamic_op_map_.find(key);
  if (it != dynamic_op_map_.end()) {
    // need to stream sync before erase
    GELOGI("dynamic op %lu need to be deleted, start to sync stream %p", key, stream_);
    GE_CHK_RT_RET(rtStreamSynchronize(stream_));
    (void)dynamic_op_map_.erase(it);
    GELOGI("dynamic op %lu delete success", key);
  }
  return SUCCESS;
}

DynamicSingleOp *StreamResource::GetDynamicOperator(const uint64_t key) {
  const std::lock_guard<std::mutex> lk(mu_);
  const auto it = dynamic_op_map_.find(key);
  if (it == dynamic_op_map_.end()) {
    return nullptr;
  }
  return it->second.get();
}

rtStream_t StreamResource::GetStream() const {
  return stream_;
}

void StreamResource::SetStream(const rtStream_t stream) {
  stream_ = stream;
}

uint8_t *StreamResource::DoMallocMemory(const std::string &purpose, const size_t size,
                                        size_t &max_allocated, std::vector<uint8_t *> &allocated) const {
  if (size == 0U) {
    GELOGD("Mem size == 0");
    return nullptr;
  }

  if ((size <= max_allocated) && (!allocated.empty())) {
    GELOGD("reuse last memory");
    return allocated.back();
  }

  if (!allocated.empty()) {
    uint8_t *const current_buffer = allocated.back();
    allocated.pop_back();
    if (rtStreamSynchronize(stream_) != RT_ERROR_NONE) {
      GELOGW("Failed to invoke rtStreamSynchronize");
    }
    (void)rtFree(current_buffer);
  }

  uint8_t *buffer = nullptr;
  auto ret = rtMalloc(PtrToPtr<uint8_t *, void *>(&buffer), size, RT_MEMORY_HBM);
  if (ret != RT_ERROR_NONE) {
    GELOGE(RT_FAILED, "[RtMalloc][Memory] failed, size = %zu, ret = %d", size, ret);
    REPORT_INNER_ERROR("E19999", "rtMalloc failed, size = %zu, ret = %d.", size, ret);
    return nullptr;
  }
  GE_PRINT_DYNAMIC_MEMORY(rtMalloc, purpose.c_str(), size);

  ret = rtMemset(buffer, size, 0U, size);
  if (ret != RT_ERROR_NONE) {
    GELOGE(RT_FAILED, "[RtMemset][Memory] failed, ret = %d", ret);
    REPORT_INNER_ERROR("E19999", "rtMemset failed, ret = %d.", ret);
    const auto rt_ret = rtFree(buffer);
    GE_IF_BOOL_EXEC(rt_ret != RT_ERROR_NONE, GELOGE(RT_FAILED, "[RtFree][Memory] failed"));
    return nullptr;
  }

  GELOGD("Malloc new memory succeeded. size = %zu", size);
  max_allocated = size;
  allocated.emplace_back(buffer);
  return buffer;
}

Status StreamResource::MallocOverflowMemory(const std::string &purpose, const int64_t size) {
  GELOGD("To Malloc overflow memory, size = %ld", size);
  void *buffer = nullptr;
  if (overflow_addr_ != nullptr) {
    GELOGE(ACL_ERROR_GE_MEMORY_ALLOCATION, "[Check][Param] failed.");
    REPORT_CALL_ERROR("E19999", "overflow addr is not null.");
    return ACL_ERROR_GE_MEMORY_ALLOCATION;
  }
  GE_CHK_RT_RET(rtMalloc(&buffer, static_cast<uint64_t>(size), RT_MEMORY_HBM));
  GE_PRINT_DYNAMIC_MEMORY(rtMalloc, purpose.c_str(), size);
  overflow_addr_ = buffer;
  globalworkpace_overflow_size_ = size;
  return SUCCESS;
}

uint8_t *StreamResource::MallocMemory(const std::string &purpose, const size_t size, const bool holding_lock) {
  GELOGD("To Malloc memory, size = %zu", size);
  if (holding_lock) {
    return DoMallocMemory(purpose, size, max_memory_size_, memory_list_);
  } else {
    const std::lock_guard<std::mutex> lk(stream_mu_);
    return DoMallocMemory(purpose, size, max_memory_size_, memory_list_);
  }
}

uint8_t *StreamResource::MallocWeight(const std::string &purpose, const size_t size) {
  GELOGD("To Malloc weight, size = %zu", size);
  uint8_t *buffer = nullptr;
  const auto ret = rtMalloc(PtrToPtr<uint8_t *, void *>(&buffer), size, RT_MEMORY_HBM);
  if (ret != RT_ERROR_NONE) {
    GELOGE(RT_FAILED, "[RtMalloc][Memory] failed, size = %zu, ret = %d", size, ret);
    REPORT_INNER_ERROR("E19999", "rtMalloc failed, size = %zu, ret = %d.", size, ret);
    return nullptr;
  }

  GE_PRINT_DYNAMIC_MEMORY(rtMalloc, purpose.c_str(), size);
  weight_list_.emplace_back(buffer);
  return buffer;
}

Status StreamResource::BuildDynamicOperator(const ModelData &model_data,
                                            DynamicSingleOp **const single_op,
                                            const uint64_t model_id) {
  const std::string &model_name = std::to_string(model_id);
  const std::lock_guard<std::mutex> lk(mu_);
  const auto it = dynamic_op_map_.find(model_id);
  if (it != dynamic_op_map_.end()) {
    *single_op = it->second.get();
    return SUCCESS;
  }

  SingleOpModel model(model_name, model_data.model_data, model_data.model_len);
  const auto ret = model.Init();
  if (ret != SUCCESS) {
    GELOGE(ret, "[Init][SingleOpModel] failed. model = %s, ret = %u", model_name.c_str(), ret);
    REPORT_CALL_ERROR("E19999", "SingleOpModel init failed, model = %s, ret = %u", model_name.c_str(), ret);
    return ret;
  }

  auto new_op = MakeUnique<DynamicSingleOp>(&tensor_pool_, resource_id_, &stream_mu_, stream_);
  GE_CHECK_NOTNULL(new_op);

  GELOGI("To build operator: %s", model_name.c_str());
  GE_CHK_STATUS_RET(model.BuildDynamicOp(*this, *new_op),
                    "[Build][DynamicOp]failed. op = %s, ret = %u", model_name.c_str(), ret);
  *single_op = new_op.get();
  dynamic_op_map_[model_id] = std::move(new_op);
  return SUCCESS;
}

Status StreamResource::BuildOperator(const ModelData &model_data, SingleOp **const single_op, const uint64_t model_id) {
  const std::string &model_name = std::to_string(model_id);
  const std::lock_guard<std::mutex> lk(mu_);
  const auto it = op_map_.find(model_id);
  if (it != op_map_.end()) {
    *single_op = it->second.get();
    return SUCCESS;
  }

  SingleOpModel model(model_name, model_data.model_data, model_data.model_len);
  const auto ret = model.Init();
  if (ret != SUCCESS) {
    GELOGE(ret, "[Init][SingleOpModel] failed. model = %s, ret = %u", model_name.c_str(), ret);
    REPORT_CALL_ERROR("E19999", "SingleOpModel init failed, model = %s, ret = %u", model_name.c_str(), ret);
    return ret;
  }

  auto new_op = MakeUnique<SingleOp>(this, &stream_mu_, stream_);
  if (new_op == nullptr) {
    GELOGE(ACL_ERROR_GE_MEMORY_ALLOCATION, "[New][SingleOp] failed.");
    REPORT_CALL_ERROR("E19999", "new SingleOp failed.");
    return ACL_ERROR_GE_MEMORY_ALLOCATION;
  }

  GELOGI("To build operator: %s", model_name.c_str());
  GE_CHK_STATUS_RET(model.BuildOp(*this, *new_op), "[Build][Op] failed. op = %s, ret = %u", model_name.c_str(), ret);

  *single_op = new_op.get();
  op_map_[model_id] = std::move(new_op);
  return SUCCESS;
}

Status StreamResource::GetThreadPool(ThreadPool **const thread_pool) {
  GE_CHECK_NOTNULL(thread_pool);
  if (thread_pool_ == nullptr) {
    thread_pool_ = MakeUnique<ThreadPool>(kThreadNumDefault);
    GE_CHECK_NOTNULL(thread_pool_);
  }
  *thread_pool = thread_pool_.get();
  return SUCCESS;
}

Status StreamResource::GetCallbackManager(hybrid::CallbackManager **const callback_manager) {
  GE_CHECK_NOTNULL(callback_manager);
  if (callback_manager_ == nullptr) {
    callback_manager_ = MakeUnique<hybrid::RtCallbackManager>();
    GE_CHECK_NOTNULL(callback_manager_);
    GE_CHK_STATUS_RET_NOLOG(callback_manager_->Init());
  }
  *callback_manager = callback_manager_.get();
  return SUCCESS;
}

uint8_t *StreamResource::GetMemoryBase() const {
  if (memory_list_.empty()) {
    return nullptr;
  }

  return memory_list_.back();
}
}  // namespace ge
