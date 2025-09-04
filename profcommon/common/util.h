/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2025-2025
            Copyright, 2025, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : utils.h
 * Description        : Common Utils.
 * Author             : msprof team
 * Creation Date      : 2025/08/30
 * *****************************************************************************
*/

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
