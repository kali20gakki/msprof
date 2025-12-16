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

#ifndef MSPTI_PARSER_CANN_HASH_CACHE_H
#define MSPTI_PARSER_CANN_HASH_CACHE_H

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

#endif // MSPTI_PARSER_CANN_HASH_CACHE_H
