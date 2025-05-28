/* ******************************************************************************
    版权所有 (c) 华为技术有限公司 2025-2025
    Copyright, 2025, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
* File Name          : cann_hash_cache.h
* Description        : cann侧hash缓存
* Author             : msprof team
* Creation Date      : 2025/05/19
* *****************************************************************************
*/

#ifndef MSPTI_PROJECT_CANN_HASH_CACHE_H
#define MSPTI_PROJECT_CANN_HASH_CACHE_H

#include <unordered_map>
#include <mutex>

namespace Mspti {
namespace Parser {
class CannHashCache {
public:
    static CannHashCache& GetInstance();
    std::string& GetHashInfo(uint64_t hashId);
    uint64_t GenHashId(const std::string &hashInfo);
private:
    CannHashCache() = default;
    // <hashID, hashInfo>
    static std::unordered_map<uint64_t, std::string> hashInfoMap_;
    static std::mutex hashMutex_;
};
}
}

#endif // MSPTI_PROJECT_CANN_HASH_CACHE_H
