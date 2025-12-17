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
 * -------------------------------------------------------------------------*/

#ifndef MSTX_DOMAIN_MGR_H
#define MSTX_DOMAIN_MGR_H

#include <atomic>
#include <map>
#include <memory>
#include <unordered_set>
#include "singleton/singleton.h"
#include "mstx_data_type.h"

namespace Collector {
namespace Dvvp {
namespace Mstx {

constexpr uint32_t MARK_MAX_CACHE_NUM = std::numeric_limits<uint32_t>::max();

struct mstxDomainAttr {
    mstxDomainAttr() = default;
    ~mstxDomainAttr() = default;

    std::shared_ptr<MstxDomainHandle> handle{nullptr};
    uint64_t nameHash{0};

    // true by default;
    // will set to false if this domain in mstx domain exclude or not in mstx domain include
    bool enabled{true};
};

class MstxDomainMgr : public analysis::dvvp::common::singleton::Singleton<MstxDomainMgr> {
public:
    MstxDomainMgr() = default;
    ~MstxDomainMgr() = default;

    mstxDomainHandle_t CreateDomainHandle(const char* name);
    void DestroyDomainHandle(mstxDomainHandle_t domain);
    bool GetDomainNameHashByHandle(mstxDomainHandle_t domain, uint64_t &name);
    uint64_t GetDefaultDomainNameHash();
    void SetMstxDomainsEnabled(const std::string &mstxDomainInclude, const std::string &mstxDomainExclude);
    bool IsDomainEnabled(const uint64_t &domainNameHash);

private:
    struct mstxDomainSetting {
        mstxDomainSetting() = default;
        ~mstxDomainSetting() = default;

        // true if set mstx domain include;
        // false if set mstx domain exclude
        bool domainInclude{false};
        std::unordered_set<uint64_t> setDomains_;
    };

    // domainHandle, domainAttr
    static std::map<mstxDomainHandle_t, std::shared_ptr<mstxDomainAttr>> domainHandleMap_;
    std::mutex domainHandleMutex_;

    // save mstx domain include/exclude setting
    std::atomic<bool> domainSet_{false};
    mstxDomainSetting domainSetting_;
};
}
}
}
#endif
