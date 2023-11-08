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

    SafeUnorderedMap(const SafeUnorderedMap &rhs);
    // 实现赋值运算符
    SafeUnorderedMap &operator=(const SafeUnorderedMap &rhs);

    // 实现[]运算符
    V &operator[](const K &key);

    // 获取map_的大小
    int Size();

    // 判断map_是否为空
    bool Empty();

    // 向map_中插入Key和Value
    // 插入成功返回true, 失败为false
    bool Insert(const K &key, const V &value);

    // 查找map_中是否存在Key和Value, 若存在将值更新到value
    bool Find(const K &key, V &value);

    // 删除map_中键为key的键值对
    void Erase(const K &key);

    // 删除map_中存储的所有键值对
    void Clear();

private:
    std::unordered_map<K, V> map_;
    std::mutex mutex_;
};
}
}
#endif // ANALYSIS_UTILS_SAFE_UNORDERED_MAP_H
