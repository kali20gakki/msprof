/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : safe_unordered_map.h
 * Description        : SafeUnorderedMap：提供了一个线程安全的unordered_map实现
 * Author             : msprof team
 * Creation Date      : 2023/11/6
 * *****************************************************************************
 */


#ifndef ANALYSIS_UTILS_SAFE_UNORDERED_MAP_H
#define ANALYSIS_UTILS_SAFE_UNORDERED_MAP_H

#include <unordered_map>
#include <shared_mutex>
#include <mutex>

namespace Analysis {
namespace Utils {

template<typename K, typename V>
class SafeUnorderedMap {
public:
    SafeUnorderedMap()
    {}

    ~SafeUnorderedMap()
    {}

    SafeUnorderedMap(const SafeUnorderedMap &rhs)
    {
        safeMap_ = rhs.safeMap_;
    }

    // 实现赋值运算符
    SafeUnorderedMap &operator=(const SafeUnorderedMap &rhs)
    {
        if (&rhs != this) {
            safeMap_ = rhs.safeMap_;
        }
        return *this;
    }

    // 实现[]运算符
    V &operator[](const K &key)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return safeMap_[key];
    }

    // 获取safeMap_的大小
    int Size()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return safeMap_.size();
    }

    // 判断safeMap_是否为空
    bool Empty()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return safeMap_.empty();
    }

    // 删除safeMap_中键为key的键值对
    void Erase(const K &key)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        safeMap_.erase(key);
    }

    // 向safeMap_中插入Key和Value
    // 插入成功返回true, 失败为false
    bool Insert(const K &key, const V &value)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto res = safeMap_.insert(std::pair<K, V>(key, value));
        return res.second;
    }

    // 查找safeMap_中是否存在Key和Value, 若存在将值更新到value
    bool Find(const K &key, V &value)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = safeMap_.find(key);
        if (it != safeMap_.end()) {
            value = it->second;
            return true;
        }
        return false;
    }

    // 查找safeMap_中是否存在Key
    bool Find(const K &key)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = safeMap_.find(key);
        if (it != safeMap_.end()) {
            return true;
        }
        return false;
    }

    // 查找safeMap_中是否存在Key，如果不存在则插入K，V
    bool FindAndInsertIfNotExist(const K &key)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = safeMap_.find(key);
        if (it != safeMap_.end()) {
            return true;
        }
        safeMap_.insert(std::pair<K, V>(key, V{}));
        return false;
    }

    // 删除safeMap_中存储的所有键值对
    void Clear()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        safeMap_.clear();
    }

private:
    std::unordered_map<K, V> safeMap_;
    std::mutex mutex_;
};
}
}
#endif // ANALYSIS_UTILS_SAFE_UNORDERED_MAP_H
