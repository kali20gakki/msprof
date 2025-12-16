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
 * -------------------------------------------------------------------------
*/

#ifndef MSPTI_COMMON_UTILS_H
#define MSPTI_COMMON_UTILS_H

#include <cstdint>
#include <exception>
#include <memory>
#include <string>
#include <tuple>
#include <cstddef>
#include <functional>
#include <utility>
#include <initializer_list>

#ifndef UNLIKELY
#ifdef _MSC_VER
#define UNLIKELY(x) (x)
#else
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#endif
#endif

namespace {
    constexpr std::size_t GOLDEN_RATIO = 0x9e3779b9;
    constexpr int SHIFT_LEFT  = 6;
    constexpr int SHIFT_RIGHT = 2;
    constexpr uint32_t SECTONSEC = 1000000000UL;
}

template<typename T>
void UNUSED(T&& x)
{
    (void)x;
}

namespace Mspti {
namespace Common {

template<typename Types, typename... Args>
inline void MsptiMakeSharedPtr(std::shared_ptr<Types> &ptr, Args&&... args)
{
    try {
        ptr = std::make_shared<Types>(std::forward<Args>(args)...);
    } catch (std::bad_alloc &e) {
        throw;
    } catch (...) {
        ptr = nullptr;
        return;
    }
}

template<typename Types, typename... Args>
inline void MsptiMakeUniquePtr(std::unique_ptr<Types> &ptr, Args&&... args)
{
    try {
        ptr = std::make_unique<Types>(std::forward<Args>(args)...);
    } catch (std::bad_alloc &e) {
        throw;
    } catch (...) {
        ptr = nullptr;
        return;
    }
}

template<typename T, typename V>
inline T ReinterpretConvert(V ptr)
{
    return reinterpret_cast<T>(ptr);
}

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

struct TupleHash {
    template <typename Tuple>
    std::size_t operator()(const Tuple& t) const
    {
        return HashTupleImpl(t, std::make_index_sequence<std::tuple_size<Tuple>::value>{});
    }

private:
    template <typename Tuple, std::size_t... I>
    static std::size_t HashTupleImpl(const Tuple& t, std::index_sequence<I...>)
    {
        std::size_t seed = 0;
        (void)std::initializer_list<int>{
                (HashCombine(seed, std::get<I>(t)), 0)...
        };
        return seed;
    }

    template <typename T>
    static void HashCombine(std::size_t& seed, const T& val)
    {
        std::size_t h = std::hash<T>{}(val);
        seed ^= h + GOLDEN_RATIO
                + (seed << SHIFT_LEFT)
                + (seed >> SHIFT_RIGHT);
    }
};

class Utils {
public:
    static uint64_t GetClockMonotonicRawNs();
    static uint64_t GetClockRealTimeNs();
    static uint64_t GetHostSysCnt();
    static uint32_t GetPid();
    static uint32_t GetTid();
    static std::string RealPath(const std::string& path);
    static std::string RelativeToAbsPath(const std::string& path);
    static bool FileExist(const std::string &path);
    static bool FileReadable(const std::string &path);
    static bool CheckCharValid(const std::string &str);
};

}  // Common
}  // Mspti

#endif
