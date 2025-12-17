/* -------------------------------------------------------------------------
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/
#include <memory>

#include "mmpa/mmpa_api.h"

namespace Analysis {
namespace Dvvp {
namespace Adx {
void* IdeXmalloc(size_t size)
{
    if (size == 0) {
        return nullptr;
    }

    void* val = malloc(size);
    if (val == nullptr) {
        return nullptr;
    }

    return val;
}

/**
 * @brief free memory
 * @param ptr: the memory to free
 *
 * @return
 */
void IdeXfree(void* ptr)
{
    if (ptr != nullptr) {
        FREE_BUF(ptr);
    }
}
} // namespace Adx
} // namespace Dvvp
} // namespace Analysis
