/**
 * @file memory_utils.cpp
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#include <cerrno>
#include "securec.h"
#include "log/adx_log.h"
#include "memory_utils.h"
namespace Adx {
/**
 * @brief malloc memory and memset memory
 * @param size: the size of memory to malloc
 *
 * @return
 *        NULL: malloc memory failed
 *        not NULL: malloc memory succ
 */
IdeMemHandle IdeXmalloc(size_t size)
{
    errno_t err;

    if (size == 0) {
        return nullptr;
    }

    IdeMemHandle val = malloc(size);
    if (val == nullptr) {
        IDE_LOGE("ran out of memory while trying to allocate %zu bytes", size);
        return nullptr;
    }

    err = memset_s(val, size, 0, size);
    if (err != EOK) {
        IDE_LOGE("memory clear failed, err: %d", err);
        free(val);
        val = nullptr;
        return nullptr;
    }

    return val;
}

/**
 * @brief realloc memory and copy ptr to new memory address
 * @param ptr: the pre memory
 * @param ptrsize: the pre memory size
 * @param size: the new memory size
 *
 * @return
 *        NULL: malloc memory failed
 *        not NULL: malloc memory succ
 */
IdeMemHandle IdeXrmalloc(const IdeMemHandle ptr, size_t ptrsize, size_t size)
{
    IdeMemHandle val = nullptr;
    if (size == 0) {
        return nullptr;
    }

    if (ptr != nullptr) {
        size_t cpLen = (ptrsize > size) ? size : ptrsize;
        val = IdeXmalloc(size);
        if (val != nullptr) {
            errno_t err = memcpy_s(val, size, ptr, cpLen);
            if (err != EOK) {
                IDE_XFREE_AND_SET_NULL(val);
                return nullptr;
            }
        }
    } else {
        val = IdeXmalloc(size);
    }

    return val;
}

/**
 * @brief free memory
 * @param ptr: the memory to free
 *
 * @return
 */
void IdeXfree(const IdeMemHandle ptr)
{
    if (ptr != nullptr) {
        free(ptr);
    }
}
}
