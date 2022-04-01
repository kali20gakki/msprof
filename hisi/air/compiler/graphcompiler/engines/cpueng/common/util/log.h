/**
 * Copyright 2020 Huawei Technologies Co., Ltd
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

#ifndef AICPU_LOG_H_
#define AICPU_LOG_H_

#include <sys/syscall.h>
#include <unistd.h>

#include "securec.h"
#include "common/util/error_manager/error_manager.h"
#include "toolchain/slog.h"

inline long GetTid() {
  thread_local static long tid = syscall(__NR_gettid);
  return tid;
}

namespace aicpu {
#define AICPUE_ERROR_CODE "E39999"

#ifdef RUN_TEST
#define AICPUE_LOGD(fmt, ...)                                             \
  printf("[DEBUG] %s:%s:%d:" #fmt "\n", __FUNCTION__, __FILE__, __LINE__, \
         ##__VA_ARGS__);
#define AICPUE_LOGI(fmt, ...)                                            \
  printf("[INFO] %s:%s:%d:" #fmt "\n", __FUNCTION__, __FILE__, __LINE__, \
         ##__VA_ARGS__);
#define AICPUE_LOGW(fmt, ...)                                            \
  printf("[WARN] %s:%s:%d:" #fmt "\n", __FUNCTION__, __FILE__, __LINE__, \
         ##__VA_ARGS__);
#define AICPUE_LOGE(fmt, ...)                                             \
  printf("[ERROR] %s:%s:%d:" #fmt "\n", __FUNCTION__, __FILE__, __LINE__, \
         ##__VA_ARGS__);
#define AICPUE_LOGEVENT(fmt, ...)                                         \
  printf("[EVENT] %s:%s:%d:" #fmt "\n", __FUNCTION__, __FILE__, __LINE__, \
         ##__VA_ARGS__);
#define AICPU_REPORT_INNER_ERROR(fmt, ...) AICPUE_LOGE(fmt, ##__VA_ARGS__)
#define AICPU_REPORT_CALL_ERROR(fmt, ...) AICPUE_LOGE(fmt, ##__VA_ARGS__)
#else
#define AICPUE_LOGD(fmt, ...)                                      \
  dlog_debug(AICPU, "[%s][tid:%lu]:" #fmt, __FUNCTION__, GetTid(), \
             ##__VA_ARGS__);
#define AICPUE_LOGI(fmt, ...)                                     \
  dlog_info(AICPU, "[%s][tid:%lu]:" #fmt, __FUNCTION__, GetTid(), \
            ##__VA_ARGS__);
#define AICPUE_LOGW(fmt, ...)                                     \
  dlog_warn(AICPU, "[%s][tid:%lu]:" #fmt, __FUNCTION__, GetTid(), \
            ##__VA_ARGS__);
#define AICPUE_LOGE(fmt, ...)                                      \
  do {                                                             \
    dlog_error(AICPU, "[%s][tid:%lu]%s:" #fmt, __FUNCTION__, GetTid(),        \
        ErrorManager::GetInstance().GetLogHeader().c_str(), ##__VA_ARGS__);   \
  } while(0)
#define AICPUE_LOGEVENT(fmt, ...)                                  \
  dlog_event(AICPU, "[%s][tid:%lu]:" #fmt, __FUNCTION__, GetTid(), \
             ##__VA_ARGS__);

#define AICPU_REPORT_INNER_ERROR(fmt, ...) \
  do { \
    dlog_error(AICPU, "[%s][tid:%lu]%s:" #fmt, __FUNCTION__, GetTid(),        \
        ErrorManager::GetInstance().GetLogHeader().c_str(), ##__VA_ARGS__);   \
    REPORT_INNER_ERROR(AICPUE_ERROR_CODE, fmt, ##__VA_ARGS__);                \
  } while(0);

#define AICPU_REPORT_CALL_ERROR(fmt, ...) \
  do { \
    dlog_error(AICPU, "[%s][tid:%lu]%s:" #fmt, __FUNCTION__, GetTid(),        \
        ErrorManager::GetInstance().GetLogHeader().c_str(), ##__VA_ARGS__);   \
    REPORT_CALL_ERROR(AICPUE_ERROR_CODE, fmt, ##__VA_ARGS__);                 \
  } while(0);

#endif
}  // namespace aicpu
#endif