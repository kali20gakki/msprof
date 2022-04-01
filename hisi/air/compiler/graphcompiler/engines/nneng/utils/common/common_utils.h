/**
 * Copyright 2019-2020 Huawei Technologies Co., Ltd
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

#ifndef FUSION_ENGINE_UTILS_COMMON_COMMON_UTIL_H_
#define FUSION_ENGINE_UTILS_COMMON_COMMON_UTIL_H_

#include <string>
#include "graph_optimizer/graph_optimize_register_error_codes.h"
#include "common/constants_define.h"

namespace fe {
int64_t GetMicroSecondsTime();
std::string L2CacheReadMode2Str(const L2CacheReadMode &read_mode);
std::string GetBufferOptimizeString(BufferOptimize buffer_optimize);
std::string GetBufferFusionModeString(BufferFusionMode buffer_fusion_mode);
std::string GetAppendArgsModeString(AppendArgsMode append_args_mode);

template <typename T>
inline std::string IntegerVecToString(std::vector<T> &integerVec) {
  std::string result = "{";
  for (auto ele : integerVec) {
    result += std::to_string(ele);
    result += ",";
  }
  result += "}";
  return result;
}

inline Status CheckUint32AddOverflow(uint32_t a, uint32_t b) {
  if (a > (UINT32_MAX - b)) {
    return FAILED;
  }
  return SUCCESS;
}

inline Status CheckUint32MulOverflow(uint32_t a, uint32_t b) {
  if (a == 0 || b == 0) {
    return SUCCESS;
  }

  if (a > (UINT32_MAX / b)) {
    return FAILED;
  }

  return SUCCESS;
}

inline Status CheckInt64MulOverflow(int64_t a, int64_t b) {
  if (a > 0) {
    if (b > 0) {
      if (a > (static_cast<int64_t>(INT64_MAX) / b)) {
        return FAILED;
      }
    } else {
      if (b < (static_cast<int64_t>(INT64_MIN) / a)) {
        return FAILED;
      }
    }
  } else {
    if (b > 0) {
      if (a < (static_cast<int64_t>(INT64_MIN) / b)) {
        return FAILED;
      }
    } else {
      if ((a != 0) && (b < (static_cast<int64_t>(INT64_MAX) / a))) {
        return FAILED;
      }
    }
  }
  return SUCCESS;
}

inline Status CheckInt64AddOverflow(int64_t a, int64_t b) {
  if (((b > 0) && (a > (static_cast<int64_t>(INT64_MAX) - b))) ||
      ((b < 0) && (a < (static_cast<int64_t>(INT64_MIN) - b)))) {
    return FAILED;
  }
  return SUCCESS;
}

inline Status CheckUint64AddOverflow(uint64_t a, uint64_t b) {
  if (a > (static_cast<uint64_t>(UINT64_MAX) - b)) {
    return FAILED;
  }
  return SUCCESS;
}

#define CM_INT64_MULCHECK(a, b)                     \
  if (CheckInt64MulOverflow((a), (b)) != SUCCESS) { \
    return FAILED;                                  \
  }

#define CM_INT64_ADDCHECK(a, b)                     \
  if (CheckInt64AddOverflow((a), (b)) != SUCCESS) { \
    return FAILED;                                   \
  }

#define CM_UINT64_ADDCHECK(a, b)                     \
  if (CheckUint64AddOverflow((a), (b)) != SUCCESS) { \
    return FAILED;                                   \
  }

}  // namespace fe
#endif  // FUSION_ENGINE_UTILS_COMMON_COMMON_UTIL_H_