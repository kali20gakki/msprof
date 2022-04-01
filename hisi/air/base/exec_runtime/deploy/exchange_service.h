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
#ifndef BASE_EXEC_RUNTIME_DEPLOY_EXCHANGE_SERVICE_H_
#define BASE_EXEC_RUNTIME_DEPLOY_EXCHANGE_SERVICE_H_

#include <functional>
#include <memory>
#include "external/ge/ge_api_error_codes.h"
#include "graph/ge_tensor.h"

namespace ge {
struct ControlInfo {
  bool end_of_sequence_flag;
};
/// Interfaces for data exchange operations
class ExchangeService {
 public:
  using FillFunc = std::function<Status(void *buffer, size_t size)>;
  ExchangeService() = default;
  ExchangeService(const ExchangeService &) = delete;
  ExchangeService &operator=(const ExchangeService &) = delete;
  virtual ~ExchangeService() = default;

  virtual Status CreateQueue(const int32_t device_id,
                             const std::string &name,
                             const uint32_t depth,
                             const uint32_t work_mode,
                             uint32_t &queue_id) = 0;
  virtual Status DestroyQueue(const int32_t device_id, const uint32_t queue_id) = 0;
  virtual Status Enqueue(const int32_t device_id, const uint32_t queue_id, const void *const data,
                         const size_t size) = 0;
  virtual Status Enqueue(const int32_t device_id, const uint32_t queue_id, const size_t size,
                         const FillFunc &fill_func) = 0;
  virtual Status Peek(const int32_t device_id, const uint32_t queue_id, size_t &size) = 0;
  virtual Status Dequeue(const int32_t device_id, const uint32_t queue_id, void *const data, const size_t size,
                         ControlInfo &control_Info) = 0;
  virtual Status DequeueTensor(const int32_t device_id, const uint32_t queue_id, GeTensor &tensor,
                               ControlInfo &control_Info) = 0;
};
}
#endif  // BASE_EXEC_RUNTIME_DEPLOY_EXCHANGE_SERVICE_H_
