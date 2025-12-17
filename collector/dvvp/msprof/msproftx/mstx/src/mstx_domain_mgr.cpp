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
#include "mstx_domain_mgr.h"
#include "msprof_dlog.h"
#include "errno/error_code.h"
#include "utils/utils.h"
#include "transport/hash_data.h"

using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::utils;
using namespace analysis::dvvp::transport;

namespace Collector {
namespace Dvvp {
namespace Mstx {

std::map<mstxDomainHandle_t, std::shared_ptr<mstxDomainAttr>> MstxDomainMgr::domainHandleMap_;

mstxDomainHandle_t MstxDomainMgr::CreateDomainHandle(const char* name)
{
    if (strcmp("default", name) == 0) {
        MSPROF_LOGE("domain name can not be 'default'!");
        return nullptr;
    }
    std::lock_guard<std::mutex> lk(domainHandleMutex_);
    if (domainHandleMap_.size() > MARK_MAX_CACHE_NUM) {
        MSPROF_LOGE("Cache domain name failed, current size: %u, limit size: %u",
            domainHandleMap_.size(), MARK_MAX_CACHE_NUM);
        return nullptr;
    }
    std::string nameStr(name);
    uint64_t hashId = HashData::instance()->GenHashId(nameStr);
    for (const auto &iter : domainHandleMap_) {
        if (iter.second->nameHash == hashId) {
            return iter.first;
        }
    }
    SHARED_PTR_ALIA<mstxDomainAttr> domainAttrPtr;
    MSVP_MAKE_SHARED0_RET(domainAttrPtr, mstxDomainAttr, nullptr);
    if (domainAttrPtr == nullptr) {
        MSPROF_LOGE("Failed to malloc for domain attr for %s", name);
        return nullptr;
    }
    MSVP_MAKE_SHARED0_RET(domainAttrPtr->handle, MstxDomainHandle, nullptr);
    if (domainAttrPtr->handle == nullptr) {
        MSPROF_LOGE("Failed to malloc for domain handle for %s", name);
        return nullptr;
    }
    domainAttrPtr->nameHash = hashId;
    domainAttrPtr->enabled = IsDomainEnabled(hashId);
    domainHandleMap_.insert(std::make_pair(domainAttrPtr->handle.get(), domainAttrPtr));
    return domainAttrPtr->handle.get();
}

void MstxDomainMgr::DestroyDomainHandle(mstxDomainHandle_t domain)
{
    std::lock_guard<std::mutex> lk(domainHandleMutex_);
    domainHandleMap_.erase(domain);
}

bool MstxDomainMgr::GetDomainNameHashByHandle(mstxDomainHandle_t domain, uint64_t &name)
{
    std::lock_guard<std::mutex> lk(domainHandleMutex_);
    auto iter = domainHandleMap_.find(domain);
    if (iter == domainHandleMap_.end()) {
        MSPROF_LOGW("input domain is invalid");
        return false;
    }
    name = iter->second->nameHash;
    return true;
}

uint64_t MstxDomainMgr::GetDefaultDomainNameHash()
{
    static uint64_t defaultDomainNameHash = HashData::instance()->GenHashId("default");
    return defaultDomainNameHash;
}

bool MstxDomainMgr::IsDomainEnabled(const uint64_t &domainNameHash)
{
    if (!domainSet_.load()) {
        return true;
    }
    if (domainSetting_.domainInclude) {
        return domainSetting_.setDomains_.count(domainNameHash) != 0;
    } else {
        return domainSetting_.setDomains_.count(domainNameHash) == 0;
    }
}

void MstxDomainMgr::SetMstxDomainsEnabled(const std::string &mstxDomainInclude,
    const std::string &mstxDomainExclude)
{
    // reset these params in case that repeat prof with different switches in one process;
    domainSet_.store(false);
    domainSetting_.domainInclude = false;
    domainSetting_.setDomains_.clear();

    if (!mstxDomainInclude.empty() && !mstxDomainExclude.empty()) {
        MSPROF_LOGW("mstx domain include and exclude are both set at the same time");
        return;
    }
    std::vector<std::string> setDomains;
    if (!mstxDomainInclude.empty()) {
        domainSetting_.domainInclude = true;
        setDomains = Utils::Split(mstxDomainInclude, false, "", ",");
        for (auto &domain : setDomains) {
            domainSetting_.setDomains_.insert(HashData::instance()->GenHashId(domain));
        }
    } else if (!mstxDomainExclude.empty()) {
        domainSetting_.domainInclude = false;
        setDomains = Utils::Split(mstxDomainExclude, false, "", ",");
        for (auto &domain : setDomains) {
            domainSetting_.setDomains_.insert(HashData::instance()->GenHashId(domain));
        }
    } else {
        MSPROF_LOGI("neither mstx domain include nor exclude is set");
        return;
    }
    std::lock_guard<std::mutex> lk(domainHandleMutex_);
    for (auto &domainHandle : domainHandleMap_) {
        domainHandle.second->enabled = (domainSetting_.setDomains_.count(domainHandle.second->nameHash) > 0) ?
            domainSetting_.domainInclude : !domainSetting_.domainInclude;
    }
    domainSet_.store(true);
}

}
}
}