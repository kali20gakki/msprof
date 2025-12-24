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

#include "csrc/activity/ascend/parser/cann_hash_cache.h"

#include "csrc/common/utils.h"

namespace Mspti {
namespace Parser {
Common::ConcurrentMap<uint64_t, std::string> CannHashCache::hashInfoMap_;

uint64_t CannHashCache::GenHashId(const std::string &hashInfo)
{
    uint64_t hashId = Common::GetHashIdImple(hashInfo);
    hashInfoMap_.Insert(hashId, hashInfo);
    return hashId;
}

uint64_t CannHashCache::GenHashId(std::string &&hashInfo)
{
    // DoubleHash耗时和map find的耗时比较
    uint64_t hashId = Common::GetHashIdImple(hashInfo);
    hashInfoMap_.Emplace(hashId, hashInfo);
    return hashId;
}

std::string &CannHashCache::GetHashInfo(uint64_t hashId)
{
    static std::string nullInfo = "";
    auto guard = hashInfoMap_.GetGuard(hashId);
    const auto iter = guard->UnSafeFind(hashId);
    if (iter.second) {
        return iter.first->second;
    }
    return nullInfo;
}
}
}