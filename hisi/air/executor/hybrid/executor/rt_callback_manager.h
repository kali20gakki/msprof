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

#ifndef GE_HYBRID_EXECUTOR_RT_CALLBACK_MANAGER_H_
#define GE_HYBRID_EXECUTOR_RT_CALLBACK_MANAGER_H_

#include <condition_variable>
#include <future>

#include "hybrid/executor/callback_manager.h"
#include "common/blocking_queue.h"
#include "external/ge/ge_api_error_codes.h"
#include "runtime/rt.h"
namespace ge {
namespace hybrid {
class RtCallbackManager : public CallbackManager {
 public:
  RtCallbackManager() = default;
  ~RtCallbackManager() override = default;

  Status Init() override;

  Status Destroy() override;

  Status RegisterCallbackFunc(const rtStream_t stream, const std::function<void()> &callback) override;

 private:
  Status RegisterCallback(const rtStream_t stream, const rtCallback_t callback, void *const user_data);
  Status CallbackProcess(const rtContext_t context);
  static void RtCallbackFunc(void *const data);

  BlockingQueue<std::pair<rtEvent_t, std::pair<rtCallback_t, void *>>> callback_queue_;
  std::future<Status> ret_future_;
};
}  // namespace hybrid
}  // namespace ge

#endif // GE_HYBRID_EXECUTOR_RT_CALLBACK_MANAGER_H_
