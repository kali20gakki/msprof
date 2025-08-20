/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : callback_manager.h
 * Description        : Manager Callback.
 * Author             : msprof team
 * Creation Date      : 2024/05/07
 * *****************************************************************************
*/

#ifndef MSPTI_CALLBACK_CALLBACK_MANAGER_H
#define MSPTI_CALLBACK_CALLBACK_MANAGER_H

#include <atomic>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <mutex>

#include "external/mspti_result.h"
#include "external/mspti_callback.h"
#include "common/plog_manager.h"

struct msptiSubscriber_st {
    msptiCallbackFunc handle;
    void* userdata;
};

namespace Mspti {
namespace Callback {

// Singleton
class CallbackManager final {
public:
    static CallbackManager* GetInstance();
    msptiResult Init(msptiSubscriberHandle *subscriber, msptiCallbackFunc callback, void* userdata);
    msptiResult UnInit(msptiSubscriberHandle subscriber);
    msptiResult EnableCallback(uint32_t enable,
        msptiSubscriberHandle subscriber, msptiCallbackDomain domain, msptiCallbackId cbid);
    msptiResult EnableDomain(uint32_t enable, msptiSubscriberHandle subscriber, msptiCallbackDomain domain);
    void ExecuteCallback(msptiCallbackDomain domain,
        msptiCallbackId cbid, msptiApiCallbackSite site, const char* funcName);
public:
    using BitMap = uint64_t;
    using AtomicBitMap = std::atomic<BitMap>;
private:
    CallbackManager() = default;
    explicit CallbackManager(const CallbackManager &obj) = delete;
    CallbackManager& operator=(const CallbackManager &obj) = delete;
    explicit CallbackManager(CallbackManager &&obj) = delete;
    CallbackManager& operator=(CallbackManager &&obj) = delete;
    msptiResult Register(msptiCallbackDomain domain, msptiCallbackId c);
    msptiResult UnRegister(msptiCallbackDomain domain, msptiCallbackId c);
    bool IsCallbackIdEnable(msptiCallbackDomain domain, msptiCallbackId cbid);

private:
    static std::unordered_map<msptiCallbackDomain, std::unordered_set<msptiCallbackId>> domain_cbid_map_;
    std::atomic<bool> init_{false};
    std::unique_ptr<msptiSubscriber_st> subscriber_ptr_{nullptr};
    // 采用bitmap存储cbid开关情况，目前cbid均小于64，后续cbid超长之后采用拓展bitmap存储
    std::array<AtomicBitMap, MSPTI_CB_DOMAIN_SIZE> cbid_map_;
};

class CallbackScope {
public:
    CallbackScope(msptiCallbackDomain domain, msptiCallbackId cbid, const char* funcName)
        : domain_(domain), cbid_(cbid), func_name_(funcName)
    {
        CallbackManager::GetInstance()->ExecuteCallback(domain, cbid, MSPTI_API_ENTER, func_name_);
    }

    ~CallbackScope()
    {
        try {
            CallbackManager::GetInstance()->ExecuteCallback(domain_, cbid_, MSPTI_API_EXIT, func_name_);
        } catch(...) {
            // Exception occurred during destruction of CallbackScope
        }
    }

private:
    msptiCallbackDomain domain_;
    msptiCallbackId cbid_;
    const char* func_name_;
};
}  // Callback
}  // Mspti

#endif
