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
#ifndef AIR_EXECUTOR_HYBRID_EXECUTOR_HOST_CPU_CALLBACK_MANAGER_H_
#define AIR_EXECUTOR_HYBRID_EXECUTOR_HOST_CPU_CALLBACK_MANAGER_H_

#include "hybrid/executor/callback_manager.h"

namespace ge {
namespace hybrid {
class HostCpuCallbackManager : public CallbackManager {
 public:
  Status Init() override;
  Status Destroy() override;
  Status RegisterCallbackFunc(const rtStream_t stream, const std::function<void()> &callback) override;
};
}  // namespace hybrid
}  // namespace ge
#endif  // AIR_EXECUTOR_HYBRID_EXECUTOR_HOST_CPU_CALLBACK_MANAGER_H_
