/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2025-2025
            Copyright, 2025, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : mstx_domain_mgr.h
 * Description        : Common definition of mstx domain manager class.
 * Author             : msprof team
 * Creation Date      : 2025/04/27
 * *****************************************************************************
*/

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
