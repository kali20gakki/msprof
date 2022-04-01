/*
 * @file common_utils.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#ifndef ADX_COMMON_UTILS_H
#define ADX_COMMON_UTILS_H
#include <memory>
namespace Adx {
#define IDE_RETURN_IF_CHECK_ASSIGN_32U_ADD(A, B, result, action) do { \
    if (UINT32_MAX - (A) <= (B)) {                                 \
        action;                                                \
    }                                                              \
    (result) = (A) + (B);                                           \
} while (0)

template <typename T>
using SharedPtr = std::shared_ptr<T>;
}
#endif