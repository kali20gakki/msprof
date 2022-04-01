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
#ifndef EXECUTOR_RUNTIME_DEPLOY_LOCAL_EXCHANGE_SERVICE_H_
#define EXECUTOR_RUNTIME_DEPLOY_LOCAL_EXCHANGE_SERVICE_H_

#include <mutex>
#include <set>
#include "runtime/base.h"
#include "exec_runtime/deploy/exchange_service.h"

namespace ge {
class LocalExchangeService : public ExchangeService {
 public:
  Status CreateQueue(const int32_t device_id,
                     const std::string &name,
                     const uint32_t depth,
                     const uint32_t work_mode,
                     uint32_t &queue_id) override;
  Status DestroyQueue(const int32_t device_id, const uint32_t queue_id) override;
  Status Enqueue(const int32_t device_id, const uint32_t queue_id, const void *const data,
                 const size_t size) override;
  Status Enqueue(const int32_t device_id, const uint32_t queue_id, const size_t size,
                 const FillFunc &fill_func) override;
  Status Peek(const int32_t device_id, const uint32_t queue_id, size_t &size) override;
  Status Dequeue(const int32_t device_id, const uint32_t queue_id, void *const data, const size_t size,
                 ControlInfo &control_Info) override;
  Status DequeueTensor(const int32_t device_id, const uint32_t queue_id, GeTensor &tensor,
                       ControlInfo &control_Info) override;

 private:
  Status EnsureInitialized(const int32_t device_id);
  Status DequeueBuf(const int32_t device_id, const uint32_t queue_id, void *const data, const size_t size) const;
  std::mutex mu_;
  std::set<int32_t> initialized_devices_;
};
}  // namespace ge

#endif  // EXECUTOR_RUNTIME_DEPLOY_LOCAL_EXCHANGE_SERVICE_H_
