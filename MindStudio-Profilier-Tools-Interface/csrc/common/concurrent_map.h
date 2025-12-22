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

#ifndef MSPTI_COMMON_CONCURRENT_MAP_H
#define MSPTI_COMMON_CONCURRENT_MAP_H

#include <unordered_map>
#include <mutex>

namespace Mspti {
namespace Common {
template <typename K, typename V, typename Hash = std::hash<K>, size_t ConcurrentLevel = 16>
class ConcurrentMap {
    static_assert((ConcurrentLevel & (ConcurrentLevel - 1)) == 0,
                  "ConcurrentLevel must be a power of 2");

    class BucketMap {
    public:
        using iterator = typename std::unordered_map<K, V, Hash>::iterator;
        class Guard {
        public:
            Guard(std::mutex& m, BucketMap& b) : lock(m), bucketMap(b)
            {
                if (isInLock) {
                    throw std::runtime_error("Reentrant lock detected on BucketMap");
                }
                isInLock = true;
            }

            ~Guard()
            {
                isInLock = false;
            }

            Guard(Guard&&) = default;
            Guard& operator=(Guard&&) = default;

            Guard(const Guard&) = delete;
            Guard& operator=(const Guard&) = delete;

            BucketMap* operator->() { return &bucketMap; }

        private:
            std::unique_lock<std::mutex> lock;
            BucketMap& bucketMap;
        };

        std::pair<iterator, bool> UnSafeFind(const K &key)
        {
            auto it = bucket.find(key);
            return {it, it != bucket.end()};
        }

        std::pair<iterator, bool> UnSafeInsert(const K &key, const V &value)
        {
            return bucket.emplace(key, value);
        }

        template<typename... Args>
        std::pair<iterator, bool> UnSafeEmplace(Args&&... args)
        {
            return bucket.emplace(std::forward<Args>(args)...);
        }

        void UnSafeErase(const K &key)
        {
            bucket.erase(key);
        }

        Guard GetGuard()
        {
            return Guard(mapMutex, *this);
        }

        V &operator[](const K &key)
        {
            return bucket[key];
        }

    private:
        inline thread_local static bool isInLock;
        std::mutex mapMutex;
        std::unordered_map<K, V, Hash> bucket;
    };

public:
    using iterator = typename BucketMap::iterator;
    using Guard = typename BucketMap::Guard;

    bool Find(const K& key, V& val)
    {
        auto guard = EnsureBucket(key).GetGuard();
        auto ans = guard->UnSafeFind(key);
        if (ans.second) {
            val = ans.first->second;
        }
        return ans.second;
    }

    void Erase(const K& key) noexcept
    {
        auto guard = EnsureBucket(key).GetGuard();
        guard->UnSafeErase(key);
    }

    std::pair<iterator, bool> Insert(const K& key, const V& value) noexcept
    {
        auto guard = EnsureBucket(key).GetGuard();
        return guard->UnSafeInsert(key, value);
    }

    std::pair<iterator, bool> Insert(const K& key, V&& value) noexcept
    {
        auto guard = EnsureBucket(key).GetGuard();
        return guard->UnSafeInsert(key, std::forward<V>(value));
    }

    template<typename... Args>
    std::pair<iterator, bool> Emplace(K& key, Args&&... args)
    {
        auto guard = EnsureBucket(key).GetGuard();
        return guard->UnSafeEmplace(key, std::forward<Args>(args)...);
    }

    Guard GetGuard(const K &key)
    {
        return EnsureBucket(key).GetGuard();
    }

private:
    static constexpr size_t BucketSize = ConcurrentLevel;
    std::array<BucketMap, BucketSize> buckets;
    Hash hasher;
    BucketMap &EnsureBucket(const K& key)
    {
        const auto index = hasher(key) & (BucketSize - 1);
        return buckets[index];
    }
};
}
}

#endif // MSPTI_COMMON_CONCURRENT_MAP_H