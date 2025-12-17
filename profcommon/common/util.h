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

#ifndef PROF_COMMON_UTIL_H
#define PROF_COMMON_UTIL_H

#include <cstdint>
#include <string>

inline uint64_t GetHashIdImple(const std::string &hashInfo)
{
    static const uint32_t UINT32_BITS = 32;
    uint32_t prime[2] = {29, 131};
    uint32_t hash[2] = {0};
    for (char d : hashInfo) {
        hash[0] = hash[0] * prime[0] + static_cast<uint32_t>(d);
        hash[1] = hash[1] * prime[1] + static_cast<uint32_t>(d);
    }
    return (((static_cast<uint64_t>(hash[0])) << UINT32_BITS) | hash[1]);
}

template <typename T, size_t N>
constexpr size_t LengthOf(T (&)[N]) noexcept
{
    return N;
}

#endif // PROF_COMMON_UTIL_H
