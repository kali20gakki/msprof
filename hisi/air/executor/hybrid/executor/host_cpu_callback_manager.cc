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

#include "hybrid/executor/host_cpu_callback_manager.h"
#include "framework/common/debug/ge_log.h"

namespace ge {
namespace hybrid {
Status HostCpuCallbackManager::Init() {
  return SUCCESS;
}
Status HostCpuCallbackManager::Destroy() {
  return SUCCESS;
}
Status HostCpuCallbackManager::RegisterCallbackFunc(const rtStream_t stream, const std::function<void()> &callback) {
  (void) stream;
  GELOGD("callback start");
  callback();
  GELOGD("callback ended");
  return SUCCESS;
}
}  // namespace hybrid
}  // namespace ge
