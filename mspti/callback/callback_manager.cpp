/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : callback_manager.cpp
 * Description        : Manager Callback.
 * Author             : msprof team
 * Creation Date      : 2024/05/07
 * *****************************************************************************
*/
#include "callback/callback_manager.h"

#include <memory>

#include "activity/ascend/ascend_manager.h"

namespace Mspti {
namespace Callback {

std::unordered_map<msptiCallbackDomain, std::unordered_set<msptiCallbackId>> CallbackManager::domain_cbid_map_ = {
    {MSPTI_CB_DOMAIN_RUNTIME, {
        MSPTI_CBID_RUNTIME_DEVICE_SET, MSPTI_CBID_RUNTIME_DEVICE_RESET,
        MSPTI_CBID_RUNTIME_DEVICE_SET_EX, MSPTI_CBID_RUNTIME_CONTEXT_CREATED_EX,
        MSPTI_CBID_RUNTIME_CONTEXT_CREATED, MSPTI_CBID_RUNTIME_CONTEXT_DESTROY,
        MSPTI_CBID_RUNTIME_STREAM_CREATED, MSPTI_CBID_RUNTIME_STREAM_DESTROY,
        MSPTI_CBID_RUNTIME_STREAM_SYNCHRONIZED
    }}
};

CallbackManager *CallbackManager::GetInstance()
{
    static CallbackManager instance;
    return &instance;
}

msptiResult CallbackManager::Init(msptiSubscriberHandle *subscriber, msptiCallbackFunc callback, void* userdata)
{
    if (subscriber == nullptr) {
        printf("[ERROR] subscriber cannot be nullptr.\n");
        return MSPTI_ERROR_INVALID_PARAMETER;
    }
    if (init_.load() || *subscriber == subscriber_ptr_.get()) {
        printf("[ERROR] subscriber cannot be register repeat.\n");
        init_.store(false);
        return MSPTI_ERROR_MULTIPLE_SUBSCRIBERS_NOT_SUPPORTED;
    }
    subscriber_ptr_ = std::make_unique<msptiSubscriber_st>();
    if (!subscriber_ptr_) {
        printf("[ERROR]Create unique_ptr failed.\n");
        return MSPTI_ERROR_MEMORY_MALLOC;
    }
    subscriber_ptr_->handle = callback;
    subscriber_ptr_->userdata = userdata;
    *subscriber = subscriber_ptr_.get();
    init_.store(true);
    return MSPTI_SUCCESS;
}

msptiResult CallbackManager::UnInit(msptiSubscriberHandle subscriber)
{
    if (!init_.load()) {
        return MSPTI_SUCCESS;
    }
    if (subscriber_ptr_.get() != subscriber) {
        printf("[ERROR] subscriber was not subscribe.\n");
        return MSPTI_ERROR_INVALID_PARAMETER;
    }
    subscriber_ptr_.reset(nullptr);
    cbid_map_.clear();
    return MSPTI_SUCCESS;
}

msptiResult CallbackManager::Register(msptiCallbackDomain domain, msptiCallbackId cbid)
{
    std::lock_guard<std::mutex> lk(cbid_mtx_);
    auto domain_entry = cbid_map_.find(domain);
    if (domain_entry == cbid_map_.end()) {
        cbid_map_.emplace(domain, std::unordered_set<msptiCallbackId>(cbid));
    } else {
        domain_entry->second.insert(cbid);
    }
    return MSPTI_SUCCESS;
}

msptiResult CallbackManager::UnRegister(msptiCallbackDomain domain, msptiCallbackId cbid)
{
    std::lock_guard<std::mutex> lk(cbid_mtx_);
    auto domain_entry = cbid_map_.find(domain);
    if (domain_entry == cbid_map_.end()) {
        printf("[WARN] callback was not enable.\n");
        return MSPTI_SUCCESS;
    } else {
        domain_entry->second.erase(cbid);
    }
    return MSPTI_SUCCESS;
}

msptiResult CallbackManager::EnableCallback(uint32_t enable,
    msptiSubscriberHandle subscriber, msptiCallbackDomain domain, msptiCallbackId cbid)
{
    if (!init_.load()) {
        printf("[WARN] callback was not init.\n");
        return MSPTI_SUCCESS;
    }
    if (subscriber != subscriber_ptr_.get()) {
        printf("[ERROR] EnableCallback: subscriber was not subscribe.\n");
        return MSPTI_ERROR_INVALID_PARAMETER;
    }
    if (domain <= MSPTI_CB_DOMAIN_INVALID || domain >= MSPTI_CB_DOMAIN_SIZE) {
        printf("[ERROR] domain was invalid.\n");
        return MSPTI_ERROR_INVALID_PARAMETER;
    }
    return (enable != 0) ? Register(domain, cbid) : UnRegister(domain, cbid);
}

msptiResult CallbackManager::EnableDomain(uint32_t enable,
    msptiSubscriberHandle subscriber, msptiCallbackDomain domain)
{
    if (!init_.load()) {
        printf("[WARN] callback was not init.\n");
        return MSPTI_SUCCESS;
    }
    if (subscriber != subscriber_ptr_.get()) {
        printf("[ERROR] EnableDomain: subscriber was not subscribe.\n");
        return MSPTI_ERROR_INVALID_PARAMETER;
    }
    if (domain <= MSPTI_CB_DOMAIN_INVALID || domain >= MSPTI_CB_DOMAIN_SIZE) {
        printf("[ERROR] domain was invalid.\n");
        return MSPTI_ERROR_INVALID_PARAMETER;
    }
    std::lock_guard<std::mutex> lk(cbid_mtx_);
    auto cbid_set = domain_cbid_map_.find(domain);
    if (cbid_set == domain_cbid_map_.end()) {
        printf("[INFO] domain was invalid.\n");
        return MSPTI_SUCCESS;
    }
    for (const auto& cbid : cbid_set->second) {
        (enable != 0) ? Register(domain, cbid) : UnRegister(domain, cbid);
    }
    return MSPTI_SUCCESS;
}

void CallbackManager::ExecuteCallback(msptiCallbackDomain domain,
    msptiCallbackId cbid, msptiApiCallbackSite site, const char* funcName)
{
    if (!init_.load()) {
        return;
    }
    auto iter = cbid_map_.end();
    {
        std::lock_guard<std::mutex> lk(cbid_mtx_);
        iter = cbid_map_.find(domain);
    }
    if (iter == cbid_map_.end()) {
        return;
    }
    if (iter->second.find(cbid) != iter->second.end() && subscriber_ptr_->handle) {
        msptiCallbackData callbackData;
        callbackData.callbackSite = site;
        callbackData.functionName = funcName;
        subscriber_ptr_->handle(subscriber_ptr_->userdata, domain, cbid, &callbackData);
    }
}

}  // Callback
}  // Mspti

msptiResult msptiSubscribe(msptiSubscriberHandle *subscriber, msptiCallbackFunc callback, void *userdata)
{
    return Mspti::Callback::CallbackManager::GetInstance()->Init(subscriber, callback, userdata);
}

msptiResult msptiUnsubscribe(msptiSubscriberHandle subscriber)
{
    return Mspti::Callback::CallbackManager::GetInstance()->UnInit(subscriber);
}

msptiResult msptiEnableCallback(
    uint32_t enable, msptiSubscriberHandle subscriber, msptiCallbackDomain domain, msptiCallbackId cbid)
{
    return Mspti::Callback::CallbackManager::GetInstance()->EnableCallback(enable, subscriber, domain, cbid);
}

msptiResult msptiEnableDomain(uint32_t enable, msptiSubscriberHandle subscriber, msptiCallbackDomain domain)
{
    return Mspti::Callback::CallbackManager::GetInstance()->EnableDomain(enable, subscriber, domain);
}
