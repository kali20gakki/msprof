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

#include "hybrid/executor/rt_callback_manager.h"
#include "framework/common/ge_inner_error_codes.h"
#include "framework/common/debug/ge_log.h"
#include "framework/common/util.h"
#include "common/plugin/ge_util.h"
#include "common/profiling_definitions.h"

namespace ge {
namespace hybrid {
Status RtCallbackManager::RegisterCallback(const rtStream_t stream,
                                           const rtCallback_t callback,
                                           void *const user_data) {
  GELOGD("To register callback");
  rtEvent_t event = nullptr;
  PROFILING_START(-1, profiling::kRtEventCreateRecord);
  GE_CHK_RT_RET(rtEventCreate(&event));
  const auto rt_ret = rtEventRecord(event, stream);
  PROFILING_END(-1, profiling::kRtEventCreateRecord);
  if (rt_ret != RT_ERROR_NONE) {
    GELOGE(RT_FAILED, "[Invoke][rtEventRecord] failed, error code = %d", rt_ret);
    REPORT_CALL_ERROR("E19999", "Invoke rtEventRecord failed, error code = %d", rt_ret);
    (void)rtEventDestroy(event);
    return RT_FAILED;
  }

  auto cb = std::pair<rtCallback_t, void *>(callback, user_data);
  const auto entry = std::pair<rtEvent_t, std::pair<rtCallback_t, void *>>(event, std::move(cb));
  if (!callback_queue_.Push(entry)) {
    (void) rtEventDestroy(event);
    return INTERNAL_ERROR;
  }

  GELOGD("Registering callback successfully");
  return SUCCESS;
}

Status RtCallbackManager::Init() {
  rtContext_t ctx = nullptr;
  GE_CHK_RT_RET(rtCtxGetCurrent(&ctx));
  ret_future_ = std::async(std::launch::async, [this](const rtContext_t context,
      const struct error_message::Context &error_context) ->Status {
    ErrorManager::GetInstance().SetErrorContext(error_context);
    return CallbackProcess(context);
  }, ctx, ErrorManager::GetInstance().GetErrorManagerContext());
  if (!ret_future_.valid()) {
    GELOGE(INTERNAL_ERROR, "[Check][ShareState]Failed to init callback manager.");
    REPORT_INNER_ERROR("E19999", "Failed to init callback manager.");
    return INTERNAL_ERROR;
  }

  return SUCCESS;
}

Status RtCallbackManager::CallbackProcess(const rtContext_t context) {
  GE_CHK_RT_RET(rtCtxSetCurrent(context));
  std::pair<rtEvent_t, std::pair<rtCallback_t, void *>> entry;
  while (true) {
    if (!callback_queue_.Pop(entry)) {
      GELOGI("CallbackManager stopped");
      return INTERNAL_ERROR;
    }

    const auto event = entry.first;
    if (event == nullptr) {
      return SUCCESS;
    }

    PROFILING_START(-1, profiling::kRtEventSync);
    const auto rt_err = rtEventSynchronize(event);
    if (rt_err != RT_ERROR_NONE) {
      GELOGE(RT_FAILED, "[Invoke][rtEventSynchronize] failed. ret = %d", rt_err);
      REPORT_CALL_ERROR("E19999", "Invoke rtEventSynchronize failed, ret = %d.", rt_err);
      GE_CHK_RT(rtEventDestroy(event));
      return RT_FAILED;
    }
    PROFILING_END(-1, profiling::kRtEventSync);

    PROFILING_START(-1, profiling::kRtEventDestroy);
    GE_CHK_RT(rtEventDestroy(event));
    PROFILING_END(-1, profiling::kRtEventDestroy);

    const auto cb_func = entry.second.first;
    const auto cb_args = entry.second.second;
    cb_func(cb_args);
  }
}

Status RtCallbackManager::Destroy() {
  GELOGI("To destroy callback manager.");
  if (!ret_future_.valid()) {
    GELOGI("RtCallbackManager not initialized.");
    return SUCCESS;
  }

  std::pair<rtEvent_t, std::pair<rtCallback_t, void *>> eof_entry;
  eof_entry.first = nullptr;
  (void) callback_queue_.Push(eof_entry);

  const auto ret = ret_future_.get();
  GELOGI("Callback manager ended. ret = %u", ret);
  return ret;
}

void RtCallbackManager::RtCallbackFunc(void *const data) {
  GELOGD("To invoke callback function");
  const auto callback_func = PtrToPtr<void, std::function<void()>>(data);
  (*callback_func)();
  delete callback_func;
}

Status RtCallbackManager::RegisterCallbackFunc(const rtStream_t stream, const std::function<void()> &callback) {
  auto func = MakeUnique<std::function<void()>>(std::function<void()>(callback));
  GE_CHECK_NOTNULL(func);
  GELOGD("Callback registered");
  return RegisterCallback(stream, &RtCallbackFunc, func.release());
}
}  // namespace hybrid
}  // namespace ge
