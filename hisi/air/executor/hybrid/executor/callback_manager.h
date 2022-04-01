/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2022. All rights reserved.
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

#ifndef AIR_EXECUTOR_HYBRID_EXECUTOR_CALLBACK_MANAGER_H_
#define AIR_EXECUTOR_HYBRID_EXECUTOR_CALLBACK_MANAGER_H_

#include <functional>
#include "common/plugin/ge_util.h"
#include "external/ge/ge_api_error_codes.h"
#include "runtime/stream.h"

namespace ge {
namespace hybrid {
class CallbackManager {
 public:
  CallbackManager() = default;
  virtual ~CallbackManager() = default;

  GE_DELETE_ASSIGN_AND_COPY(CallbackManager);

  virtual Status Init() = 0;

  virtual Status Destroy() = 0;

  virtual Status RegisterCallbackFunc(const rtStream_t stream, const std::function<void()> &callback) = 0;
};
}  // namespace hybrid
}  // namespace ge

#endif  // AIR_EXECUTOR_HYBRID_EXECUTOR_CALLBACK_MANAGER_H_