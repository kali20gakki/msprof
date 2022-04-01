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

#ifndef FUSION_ENGINE_OPTIMIZER_COMMON_MATH_UTIL_H_
#define FUSION_ENGINE_OPTIMIZER_COMMON_MATH_UTIL_H_

#include <securec.h>
#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include "common/fe_inner_error_codes.h"
#include "common/fe_log.h"
#include "common/fe_utils.h"

namespace fe {
enum class BitShift {
  BIT_SHIFT_8 = 8,
  BIT_SHIFT_16 = 16,
  BIT_SHIFT_24 = 24,
  BIT_SHIFT_32 = 32,
  BIT_SHIFT_40 = 40,
  BIT_SHIFT_48 = 48,
};

float Uint16ToFloat(const uint16_t &intVal);

template <typename Dtype>
Status NnSet(const int32_t n, const Dtype alpha, Dtype &output1) {
  Dtype *output = &output1;
  FE_CHECK_NOTNULL(output);

  if (alpha == 0) {
    if (memset_s(output, sizeof(Dtype) * n, 0, sizeof(Dtype) * n)) {
      return FAILED;
    }
  }

  for (int32_t i = 0; i < n; ++i) {
    output[i] = alpha;
  }
  return SUCCESS;
}

/**
 * @ingroup math_util
 * @brief check whether uint8 addition can result in overflow
 * @param [in] a  addend
 * @param [in] b  addend
 * @return Status
 */
inline Status CheckUint8AddOverflow(uint8_t m, uint8_t n) {
  if (m > (UINT8_MAX - n)) {
    return FAILED;
  }
  return SUCCESS;
}

inline Status CheckUint32AddOverflow(uint32_t m, uint32_t n) {
  if (m > (UINT32_MAX - n)) {
    return FAILED;
  }
  return SUCCESS;
}

inline Status CheckUint32MulOverflow(uint32_t m, uint32_t n) {
  if (m == 0 || n == 0) {
    return SUCCESS;
  }

  if (m > (UINT32_MAX / n)) {
    return FAILED;
  }

  return SUCCESS;
}

inline Status CheckSizetAddOverFlow(size_t m, size_t n) {
  if (m > (static_cast<size_t>(UINT64_MAX) - n)) {
    return FAILED;
  }
  return SUCCESS;
}

inline Status CheckInt64MulOverflow(int64_t m, int64_t n) {
  if (m > 0) {
    if (n > 0) {
      if (m > (static_cast<int64_t>(INT64_MAX) / n)) {
        return FAILED;
      }
    } else {
      if (n < (static_cast<int64_t>(INT64_MIN) / m)) {
        return FAILED;
      }
    }
  } else {
    if (n > 0) {
      if (m < (static_cast<int64_t>(INT64_MIN) / n)) {
        return FAILED;
      }
    } else {
      if ((m != 0) && (n < (static_cast<int64_t>(INT64_MAX) / m))) {
        return FAILED;
      }
    }
  }
  return SUCCESS;
}

inline Status CheckUint64AddOverflow(uint64_t a, uint64_t b) {
  if (a > (static_cast<uint64_t>(UINT64_MAX) - b)) {
    return FAILED;
  }
  return SUCCESS;
}

/**
 * @ingroup math_util
 * @brief check whether int64 addition can result in overflow
 * @param [in] a  addend
 * @param [in] b  addend
 * @return Status
 */
inline Status CheckInt64AddOverflow(int64_t a, int64_t b) {
  if (((b > 0) && (a > (static_cast<int64_t>(INT64_MAX) - b))) ||
      ((b < 0) && (a < (static_cast<int64_t>(INT64_MIN) - b)))) {
    return FAILED;
  }
  return SUCCESS;
}

/**
 * @ingroup math_util
 * @brief check whether int32 subtraction can result in overflow
 * @param [in] a  subtrahend
 * @param [in] b  minuend
 * @return Status
 */
inline Status CheckInt32SubOverflow(int32_t a, int32_t b) {
  if (((b > 0) && (a < (INT32_MIN + b))) || ((b < 0) && (a > (INT32_MAX + b)))) {
    return FAILED;
  }
  return SUCCESS;
}

inline Status CheckInt64SubOverflow(int64_t a, int64_t b) {
  if (((b > 0) && (a < (INT64_MIN + b))) || ((b < 0) && (a > (INT64_MAX + b)))) {
    return FAILED;
  }
  return SUCCESS;
}

inline Status CheckUint64MulOverflow(uint64_t a, uint64_t b) {
  if (a == 0 || b == 0) {
    return SUCCESS;
  }

  if (a > (UINT64_MAX / b)) {
    return FAILED;
  }

  return SUCCESS;
}

/**
 * @ingroup math_util
 * @brief check whether int division can result in overflow
 * @param [in] a  dividend
 * @param [in] b  divisor
 * @return Status
 */
inline Status CheckIntDivOverflow(int a, int b) {
  if ((b == 0) || ((a == INT_MIN) && (b == -1))) {
    return FAILED;
  }
  return SUCCESS;
}

/**
 * @ingroup math_util
 * @brief check whether int32 division can result in overflow
 * @param [in] a  dividend
 * @param [in] b  divisor
 * @return Status
 */
inline Status CheckInt32DivOverflow(int32_t a, int32_t b) {
  if ((b == 0) || ((a == INT32_MIN) && (b == -1))) {
    return FAILED;
  }
  return SUCCESS;
}

inline Status CheckInt32AddOverflow(int32_t a, int32_t b) {
  if (((b > 0) && (a > (INT32_MAX - b))) || ((b < 0) && (a < (INT32_MIN - b)))) {
      return FAILED;
  }
  return SUCCESS;
}

/**
 * @ingroup math_util
 * @brief check whether float addition can result in overflow
 *  @param [in] a  addend
 *  @param [in] b  addend
 * @return Status
 */
inline Status CheckFloatAddOverflow(float a, float b) {
  if (!std::isfinite(static_cast<float>(a) + static_cast<float>(b))) {
    return FAILED;
  }
  return SUCCESS;
}

/**
 * @ingroup math_util
 * @brief check whether float multiplication can result in overflow
 *  @param [in] a  addend
 *  @param [in] b  addend
 * @return Status
 */
inline Status CheckFloatMulOverflow(float a, float b) {
  if (!std::isfinite(static_cast<float>(a) * static_cast<float>(b))) {
    return FAILED;
  }
  return SUCCESS;
}

inline Status CheckDoubleAddOverflow(double a, double b) {
  if (!std::isfinite(static_cast<double>(a) + static_cast<double>(b))) {
    return FAILED;
  }
  return SUCCESS;
}

inline Status CheckDoubleMulOverflow(double a, double b) {
  if (!std::isfinite(static_cast<double>(a) * static_cast<double>(b))) {
    return FAILED;
  }
  return SUCCESS;
}

inline Status CheckDoubleZero(double a) {
  if (abs(a) < 1e-15) {
    return FAILED;
  }
  return SUCCESS;
}

#define FE_UINT8_ADDCHECK(a, b)                                            \
  if (CheckUint8AddOverflow((a), (b)) != SUCCESS) {                        \
    FE_LOGE("UINT8 %d and %d division can result in overflow!", (a), (b)); \
    return INTERNAL_ERROR;                                                 \
  }

#define FE_UINT32_ADDCHECK(a, b)                                                                \
  if (CheckUint32AddOverflow((a), (b)) != SUCCESS) {                                            \
    FE_LOGE("UINT32 %u and %u addition can result in overflow!", (uint32_t)(a), (uint32_t)(b)); \
    return INTERNAL_ERROR;                                                                      \
  }

#define FE_INT64_ADDCHECK(a, b)                                              \
  if (CheckInt64AddOverflow((a), (b)) != SUCCESS) {                          \
    FE_LOGE("Int64 %ld and %ld addition can result in overflow!", (a), (b)); \
    return INTERNAL_ERROR;                                                   \
  }

#define FE_UINT64_ADDCHECK(a, b)                                                                  \
  if (CheckUint64AddOverflow((a), (b)) != SUCCESS) {                                              \
    FE_LOGE("UINT64 %lu and %lu addition can result in overflow!", (uint64_t)(a), (uint64_t)(b)); \
    return INTERNAL_ERROR;                                                                        \
  }

#define FE_SIZET_ADDCHECK(a, b)                                                             \
  if (CheckSizetAddOverFlow((a), (b)) != SUCCESS) {                                         \
    FE_LOGE("Size_t %zu and %zu addition can result in overflow!", (size_t)(a), (size_t)(b)); \
    return INTERNAL_ERROR;                                                                  \
  }

#define FE_INT64_SUBCHECK(a, b)                                                 \
  if (CheckInt64SubOverflow((a), (b)) != SUCCESS) {                             \
    FE_LOGE("INT64 %ld and %ld subtraction can result in overflow!", (a), (b)); \
    return INTERNAL_ERROR;                                                      \
  }

#define FE_INT32_SUBCHECK(a, b)                                               \
  if (CheckInt32SubOverflow((a), (b)) != SUCCESS) {                           \
    FE_LOGE("INT32 %d and %d subtraction can result in overflow!", (a), (b)); \
    return INTERNAL_ERROR;                                                    \
  }

#define FE_UINT32_MULCHECK(a, b)                                                                      \
  if (CheckUint32MulOverflow((a), (b)) != SUCCESS) {                                                  \
    FE_LOGE("UINT32 %u and %u multiplication can result in overflow!", (uint32_t)(a), (uint32_t)(b)); \
    return INTERNAL_ERROR;                                                                            \
  }

#define FE_INT64_MULCHECK(a, b)                                                                      \
  if (CheckInt64MulOverflow((a), (b)) != SUCCESS) {                                                  \
    FE_LOGE("INT64 %ld and %ld multiplication can result in overflow!", (int64_t)(a), (int64_t)(b)); \
    return INTERNAL_ERROR;                                                                           \
  }

#define FE_UINT64_MULCHECK(a, b)                                                                        \
  if (CheckUint64MulOverflow((a), (b)) != SUCCESS) {                                                    \
    FE_LOGE("UINT64 %lu and %lu multiplication can result in overflow!", (uint64_t)(a), (uint64_t)(b)); \
    return INTERNAL_ERROR;                                                                              \
  }

#define FE_INT_DIVCHECK(a, b)                                            \
  if (CheckIntDivOverflow((a), (b)) != SUCCESS) {                        \
    FE_LOGE("INT %d and %d division can result in overflow!", (a), (b)); \
    return INTERNAL_ERROR;                                               \
  }

#define FE_INT32_DIVCHECK(a, b)                                            \
  if (CheckInt32DivOverflow((a), (b)) != SUCCESS) {                        \
    FE_LOGE("INT32 %d and %d division can result in overflow!", (a), (b)); \
    return INTERNAL_ERROR;                                                 \
  }

#define FE_FLOAT_MULCHECK(a, b)                                                              \
  if (CheckFloatMulOverflow((a), (b)) != SUCCESS) {                                          \
    FE_LOGE("Float %f and %f multiplication can result in overflow!", static_cast<float>(a), \
           static_cast<float>(b));                                                           \
    return INTERNAL_ERROR;                                                                   \
  }

#define FE_FLOAT_ADDCHECK(a, b)                                                        \
  if (CheckFloatAddOverflow((a), (b)) != SUCCESS) {                                    \
    FE_LOGE("Float %f and %f addition can result in overflow!", static_cast<float>(a), \
           static_cast<float>(b));                                                     \
    return INTERNAL_ERROR;                                                             \
  }
#define FE_INT32_ADDCHECK(a, b)                                                                        \
  if (CheckInt32AddOverflow((a), (b)) != SUCCESS) {                                                    \
    FE_LOGE("INT32 %d and %d add can result in overflow!", a, b);                \
    return INTERNAL_ERROR;                                                                             \
  }

#define FE_DOUBLE_ADDCHECK(a, b)                                                                         \
  if (CheckDoubleAddOverflow((a), (b)) != SUCCESS) {                                                     \
    FE_LOGE("Double %lf and %lf addition can result in overflow!", static_cast<double>(a),               \
           static_cast<double>(b));                                                                      \
    return INTERNAL_ERROR;                                                                               \
  }

#define FE_DOUBLE_MULCHECK(a, b)                                                                         \
  if (CheckDoubleMulOverflow((a), (b)) != SUCCESS) {                                                     \
    FE_LOGE("Double %lf and %lf multiplication can result in overflow!", static_cast<double>(a),         \
           static_cast<double>(b));                                                                      \
    return INTERNAL_ERROR;                                                                               \
  }

#define FE_DOUBLE_ZEROCHECK(a)                                                                        \
  if (CheckDoubleZero(a) != SUCCESS) {                                                                \
    FE_LOGE("Double %lf is zero!", (double)(a));                                                      \
    return INTERNAL_ERROR;                                                                            \
  }

#define FE_INT64_ZEROCHECK(a)                                                                         \
  if ((a) == 0) {                                                                                     \
    FE_LOGE("Int64 %ld is zero!", a);                                                                 \
    return INTERNAL_ERROR;                                                                            \
  }

#define FLT_EPSILON 1.1920929e-07F

bool inline FloatEqual(float a, float b) {
  return fabs(a - b) < FLT_EPSILON;
}

#define FE_FLOAT_ZEROCHECK(a)                                                                         \
  if (fabs(a) < FLT_EPSILON || (a) < 0) {                                                             \
    FE_LOGE("Float %f is zero!", a);                                                                  \
    return INTERNAL_ERROR;                                                                            \
  }
}  // namespace fe
#endif  // FUSION_ENGINE_OPTIMIZER_COMMON_MATH_UTIL_H_
