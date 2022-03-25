/**
 * @file common/utils.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef ADX_COMMON_MEMORY_UTILS_H
#define ADX_COMMON_MEMORY_UTILS_H
#include <cstddef>
#include "extra_config.h"
#define IDE_XFREE_AND_SET_NULL(ptr) do {                    \
    IdeXfree(ptr);                                          \
    ptr = nullptr;                                          \
} while (0)

namespace Analysis {
namespace Dvvp {
namespace Adx {
IdeMemHandle IdeXmalloc(size_t size);
IdeMemHandle IdeXrmalloc(const IdeMemHandle ptr, size_t ptrsize, size_t size);
void IdeXfree(IdeMemHandle ptr);
}   // namespace Adx
}   // namespace Dvvp
}   // namespace Analysis

#endif // ADX_COMMON_MEMORY_UTILS_H