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

#include "cann_hash_cache.h"

#include "csrc/common/utils.h"

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