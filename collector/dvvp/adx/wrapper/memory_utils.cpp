/**
 * @file memory_utils.cpp
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#include "memory_utils.h"
#include <cerrno>
#include "securec.h"
#include "msprof_dlog.h"

namespace Analysis {
namespace Dvvp {
namespace Adx {
IdeMemHandle IdeXmalloc(size_t size)
{
    errno_t err;

    if (size == 0) {
        return nullptr;
    }

    IdeMemHandle val = malloc(size);
    if (val == nullptr) {
        MSPROF_LOGE("ran out of memory while trying to allocate %lu bytes", size);
        return nullptr;
    }

    err = memset_s(val, size, 0, size);
    if (err != EOK) {
        MSPROF_LOGE("[IdeXmalloc]memory clear failed, err: %d", err);
        free(val);
        val = nullptr;
        return nullptr;
    }
    return val;
}

IdeMemHandle IdeXrmalloc(const IdeMemHandle ptr, size_t ptrsize, size_t size)
{
    if (size == 0) {
        return nullptr;
    }

    if (ptr == nullptr) {
        return IdeXmalloc(size);
    }
    size_t cpLen = (ptrsize > size) ? size : ptrsize;
    IdeMemHandle val = IdeXmalloc(size);
    if (val != nullptr) {
        errno_t err = memcpy_s(val, size, ptr, cpLen);
        if (err != EOK) {
            IDE_XFREE_AND_SET_NULL(val);
            return nullptr;
        }
    }
    return val;
}

void IdeXfree(IdeMemHandle ptr)
{
    if (ptr != nullptr) {
        free(ptr);
        ptr = nullptr;
    }
}
}   // namespace Adx
}   // namespace Dvvp
}   // namespace Analysis