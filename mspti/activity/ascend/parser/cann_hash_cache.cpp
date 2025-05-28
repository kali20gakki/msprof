/* ******************************************************************************
    版权所有 (c) 华为技术有限公司 2025-2025
    Copyright, 2025, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : cann_hash_cache.cpp
 * Description        : cann侧hash缓存
 * Author             : msprof team
 * Creation Date      : 2025/05/19
 * *****************************************************************************
 */

#include "cann_hash_cache.h"

#include "common/utils.h"

namespace Mspti {
namespace Parser {
std::unordered_map<uint64_t, std::string> CannHashCache::hashInfoMap_;
std::mutex CannHashCache::hashMutex_;

CannHashCache &CannHashCache::GetInstance()
{
    static CannHashCache instance;
    return instance;
}

uint64_t CannHashCache::GenHashId(const std::string &hashInfo)
{
    // DoubleHash耗时和map find的耗时比较
    uint64_t hashId = Common::GetHashIdImple(hashInfo);
    {
        std::lock_guard<std::mutex> lock(hashMutex_);
        const auto iter = hashInfoMap_.find(hashId);
        if (iter == hashInfoMap_.end()) {
            hashInfoMap_.insert({ hashId, hashInfo });
        }
    }
    return hashId;
}

std::string &CannHashCache::GetHashInfo(uint64_t hashId)
{
    static std::string nullInfo = "";
    std::lock_guard<std::mutex> lock(hashMutex_);
    const auto iter = hashInfoMap_.find(hashId);
    if (iter != hashInfoMap_.end()) {
        return iter->second;
    }
    return nullInfo;
}
}
}