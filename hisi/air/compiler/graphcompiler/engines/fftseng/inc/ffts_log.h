/**
 * Copyright 2022-2023 Huawei Technologies Co., Ltd
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

#ifndef FFTS_ENGINE_INC_FFTS_LOG_H_
#define FFTS_ENGINE_INC_FFTS_LOG_H_

#include <sys/syscall.h>
#include <unistd.h>
#include <cstdint>
#include <string>
#include "toolchain/slog.h"
#include "inc/common/util/error_manager/error_manager.h"
#include "inc/ffts_error_codes.h"
namespace ffts {
/** Assigned FFTS name in log */
const std::string FFTS_MODULE_NAME = "FFTS";

inline uint64_t FFTSGetTid() {
  thread_local static uint64_t tid = static_cast<uint64_t>(syscall(__NR_gettid));
  return tid;
}

#define D_FFTS_LOGD(MOD_NAME, fmt, ...) dlog_debug(FFTS, "%lu %s:" #fmt "\n", FFTSGetTid(), __FUNCTION__, ##__VA_ARGS__)
#define D_FFTS_LOGI(MOD_NAME, fmt, ...) dlog_info(FFTS, "%lu %s:" #fmt "\n", FFTSGetTid(), __FUNCTION__, ##__VA_ARGS__)
#define D_FFTS_LOGW(MOD_NAME, fmt, ...) dlog_warn(FFTS, "%lu %s:" #fmt "\n", FFTSGetTid(), __FUNCTION__, ##__VA_ARGS__)
#define D_FFTS_LOGE(MOD_NAME, fmt, ...) dlog_error(FFTS, "%lu %s:" #fmt "\n", FFTSGetTid(), __FUNCTION__, ##__VA_ARGS__)
#define D_FFTS_LOGV(MOD_NAME, fmt, ...) dlog_event(FFTS, "%lu %s:" #fmt "\n", FFTSGetTid(), __FUNCTION__, ##__VA_ARGS__)

#define FFTS_LOGD(...) D_FFTS_LOGD(FFTS_MODULE_NAME, __VA_ARGS__)
#define FFTS_LOGI(...) D_FFTS_LOGI(FFTS_MODULE_NAME, __VA_ARGS__)
#define FFTS_LOGW(...) D_FFTS_LOGW(FFTS_MODULE_NAME, __VA_ARGS__)
#define FFTS_LOGE(...) D_FFTS_LOGE(FFTS_MODULE_NAME, __VA_ARGS__)
#define FFTS_LOGV(...) D_FFTS_LOGV(FFTS_MODULE_NAME, __VA_ARGS__)

#define FFTS_LOGI_IF(cond, ...) \
  if ((cond)) {               \
    FFTS_LOGI(__VA_ARGS__);     \
  }

#define FFTS_LOGE_IF(cond, ...) \
  if ((cond)) {               \
    FFTS_LOGE(__VA_ARGS__);     \
  }

#define FFTS_CHECK(cond, log_func, return_expr) \
  do {                                        \
    if (cond) {                               \
      log_func;                               \
      return_expr;                            \
    }                                         \
  } while (0)

// If failed to make_shared, then print log and execute the statement.
#define FFTS_MAKE_SHARED(exec_expr0, exec_expr1) \
  do {                                         \
    try {                                      \
      exec_expr0;                              \
    } catch (...) {                            \
      FFTS_LOGE("Make shared failed");           \
      exec_expr1;                              \
    }                                          \
  } while (0)

#define FFTS_CHECK_NOTNULL(val)                           \
  do {                                                  \
    if ((val) == nullptr) {                             \
      FFTS_LOGE("Parameter[%s] must not be null.", #val); \
      return PARAM_INVALID;                         \
    }                                                   \
  } while (0)

#define REPORT_FFTS_ERROR(fmt, ...)  \
  do {                                                  \
    REPORT_INNER_ERROR(EM_INNER_ERROR, fmt, ##__VA_ARGS__);     \
    FFTS_LOGE(fmt, ##__VA_ARGS__);                        \
  } while (0)
}
#endif  // FFTS_ENGINE_INC_FFTS_LOG_H_
