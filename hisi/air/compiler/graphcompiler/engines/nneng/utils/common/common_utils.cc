/**
 * @file common_utils.cpp
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2019. All rights reserved.
 *
 * @brief util for compute tensor size
 *
 * @version 1.0
 *
 */

#include "common/common_utils.h"
#include <sys/time.h>
#include "common/comm_log.h"

namespace fe {
int64_t GetMicroSecondsTime() {
  struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = 0;
  int ret = gettimeofday(&tv, nullptr);
  if (ret != 0) {
    return 0;
  }
  if (tv.tv_sec < 0 || tv.tv_usec < 0) {
    return 0;
  }
  int64_t microMultiples = 1000000;
  int64_t second = tv.tv_sec;
  return second * microMultiples + tv.tv_usec;
}

std::string L2CacheReadMode2Str(const L2CacheReadMode &read_mode) {
  if (L2CACHE_READ_MODE_STRING_MAP.end() == L2CACHE_READ_MODE_STRING_MAP.find(read_mode)) {
    return "UNDEFINED";
  }
  return L2CACHE_READ_MODE_STRING_MAP.at(read_mode);
}

std::string GetBufferOptimizeString(BufferOptimize buffer_optimize) {
  auto iter = BUFFER_OPTIMIZE_STRING_MAP.find(buffer_optimize);
  if (iter == BUFFER_OPTIMIZE_STRING_MAP.end()) {
    return BUFFER_OPTIMIZE_UNKNOWN;
  } else {
    return iter->second;
  }
}

std::string GetBufferFusionModeString(BufferFusionMode buffer_fusion_mode) {
  auto iter = BUFFER_FUSION_MODE_STRING_MAP.find(buffer_fusion_mode);
  if (iter == BUFFER_FUSION_MODE_STRING_MAP.end()) {
    return BUFFER_FUSION_MODE_UNKNOWN;
  } else {
    return iter->second;
  }
}

std::string GetAppendArgsModeString(AppendArgsMode append_args_mode) {
  auto iter = APPEND_ARGS_MODE_STRING_MAP.find(append_args_mode);
  if (iter == APPEND_ARGS_MODE_STRING_MAP.end()) {
    return "unknown";
  } else {
    return iter->second;
  }
}
}  // namespace fe
