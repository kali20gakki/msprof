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

#include "activity/activity_manager.h"
#include "activity/ascend/dev_task_manager.h"
#include "common/plog_manager.h"
#include "common/context_manager.h"
#include "common/utils.h"

namespace Mspti {
namespace Callback {

namespace {
inline bool IsValidCBDomain(msptiCallbackDomain domain)
{
    return domain > MSPTI_CB_DOMAIN_INVALID && domain < MSPTI_CB_DOMAIN_SIZE;
}

inline bool IsValidCBId(msptiCallbackId cbid)
{
    return cbid < sizeof(CallbackManager::BitMap) * 8;
}
}

std::unordered_map<msptiCallbackDomain, std::unordered_set<msptiCallbackId>> CallbackManager::domain_cbid_map_ = {
    {MSPTI_CB_DOMAIN_RUNTIME, {
        MSPTI_CBID_RUNTIME_DEVICE_SET, MSPTI_CBID_RUNTIME_DEVICE_RESET,
        MSPTI_CBID_RUNTIME_DEVICE_SET_EX, MSPTI_CBID_RUNTIME_CONTEXT_CREATED_EX,
        MSPTI_CBID_RUNTIME_CONTEXT_CREATED, MSPTI_CBID_RUNTIME_CONTEXT_DESTROY,
        MSPTI_CBID_RUNTIME_STREAM_CREATED, MSPTI_CBID_RUNTIME_STREAM_DESTROY,
        MSPTI_CBID_RUNTIME_STREAM_SYNCHRONIZED, MSPTI_CBID_RUNTIME_LAUNCH,
        MSPTI_CBID_RUNTIME_CPU_LAUNCH, MSPTI_CBID_RUNTIME_AICPU_LAUNCH, MSPTI_CBID_RUNTIME_AIV_LAUNCH,
        MSPTI_CBID_RUNTIME_FFTS_LAUNCH, MSPTI_CBID_RUNTIME_MALLOC, MSPTI_CBID_RUNTIME_FREE,
        MSPTI_CBID_RUNTIME_MALLOC_HOST, MSPTI_CBID_RUNTIME_FREE_HOST, MSPTI_CBID_RUNTIME_MALLOC_CACHED,
        MSPTI_CBID_RUNTIME_FLUSH_CACHE, MSPTI_CBID_RUNTIME_INVALID_CACHE, MSPTI_CBID_RUNTIME_MEMCPY,
        MSPTI_CBID_RUNTIME_MEMCPY_HOST, MSPTI_CBID_RUNTIME_MEMCPY_ASYNC,
        MSPTI_CBID_RUNTIME_MEM_CPY2D, MSPTI_CBID_RUNTIME_MEM_CPY2D_ASYNC,
        MSPTI_CBID_RUNTIME_MEM_SET, MSPTI_CBID_RUNTIME_MEM_SET_ASYNC, MSPTI_CBID_RUNTIME_MEM_GET_INFO,
        MSPTI_CBID_RUNTIME_RESERVE_MEM_ADDRESS, MSPTI_CBID_RUNTIME_RELEASE_MEM_ADDRESS,
        MSPTI_CBID_RUNTIME_MALLOC_PHYSICAL, MSPTI_CBID_RUNTIME_FREE_PHYSICAL,
        MSPTI_CBID_RUNTIME_MEM_EXPORT_TO_SHAREABLE_HANDLE, MSPTI_CBID_RUNTIME_MEM_IMPORT_FROM_SHAREABLE_HANDLE,
        MSPTI_CBID_RUNTIME_MEM_SET_PID_TO_SHAREABLE_HANDLE
    }},
    {MSPTI_CB_DOMAIN_HCCL, {
        MSPTI_CBID_HCCL_ALLREDUCE, MSPTI_CBID_HCCL_BROADCAST, MSPTI_CBID_HCCL_ALLGATHER,
        MSPTI_CBID_HCCL_REDUCE_SCATTER, MSPTI_CBID_HCCL_REDUCE, MSPTI_CBID_HCCL_ALL_TO_ALL,
        MSPTI_CBID_HCCL_ALL_TO_ALLV, MSPTI_CBID_HCCL_BARRIER,
        MSPTI_CBID_HCCL_SCATTER, MSPTI_CBID_HCCL_SEND, MSPTI_CBID_HCCL_RECV, MSPTI_CBID_HCCL_SENDRECV
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
        MSPTI_LOGE("subscriber cannot be nullptr.");
        return MSPTI_ERROR_INVALID_PARAMETER;
    }
    if (init_.load()) {
        MSPTI_LOGE("subscriber cannot be register repeat.");
        return MSPTI_ERROR_MULTIPLE_SUBSCRIBERS_NOT_SUPPORTED;
    }
    Mspti::Common::MsptiMakeUniquePtr(subscriber_ptr_);
    if (!subscriber_ptr_) {
        MSPTI_LOGE("Failed to init subscriber.");
        return MSPTI_ERROR_INNER;
    }
    for (auto& bitmap : cbid_map_) {
        bitmap.store(0, std::memory_order_relaxed);
    }
    subscriber_ptr_->handle = callback;
    subscriber_ptr_->userdata = userdata;
    *subscriber = subscriber_ptr_.get();
    init_.store(true);
    Mspti::Ascend::DevTaskManager::GetInstance()->RegisterReportCallback();
    Mspti::Common::ContextManager::GetInstance()->StartSyncTime();
    MSPTI_LOGI("CallbackManager Init success.");
    return MSPTI_SUCCESS;
}

msptiResult CallbackManager::UnInit(msptiSubscriberHandle subscriber)
{
    if (!init_.load()) {
        return MSPTI_SUCCESS;
    }
    if (subscriber_ptr_.get() != subscriber) {
        MSPTI_LOGE("[ERROR] subscriber was not subscribe.");
        return MSPTI_ERROR_INVALID_PARAMETER;
    }
    subscriber_ptr_.reset(nullptr);
    for (auto& bitmap : cbid_map_) {
        bitmap.store(0, std::memory_order_relaxed);
    }
    init_.store(false);
    Mspti::Common::ContextManager::GetInstance()->StopSyncTime();
    MSPTI_LOGI("CallbackManager UnInit success.");
    return MSPTI_SUCCESS;
}

msptiResult CallbackManager::Register(msptiCallbackDomain domain, msptiCallbackId cbid)
{
    if (!IsValidCBDomain(domain) || !IsValidCBId(cbid)) {
        return MSPTI_ERROR_INVALID_PARAMETER;
    }
    auto idx = static_cast<size_t>(domain);
    uint64_t mask = 1ULL << static_cast<int>(cbid);
    cbid_map_[idx].fetch_or(mask, std::memory_order_relaxed);
    MSPTI_LOGI("CallbackManager Register domain: %d, cbid: %d.", domain, cbid);
    return MSPTI_SUCCESS;
}

msptiResult CallbackManager::UnRegister(msptiCallbackDomain domain, msptiCallbackId cbid)
{
    if (!IsValidCBDomain(domain) || !IsValidCBId(cbid)) {
        return MSPTI_ERROR_INVALID_PARAMETER;
    }
    auto idx = static_cast<size_t>(domain);
    uint64_t mask = ~(1ULL << static_cast<int>(cbid));
    cbid_map_[idx].fetch_and(mask, std::memory_order_relaxed);
    MSPTI_LOGI("CallbackManager UnRegister domain: %d, cbid: %d.", domain, cbid);
    return MSPTI_SUCCESS;
}

msptiResult CallbackManager::EnableCallback(uint32_t enable,
    msptiSubscriberHandle subscriber, msptiCallbackDomain domain, msptiCallbackId cbid)
{
    if (!init_.load()) {
        MSPTI_LOGW("CallbackManager was not init.");
        return MSPTI_SUCCESS;
    }
    if (subscriber != subscriber_ptr_.get()) {
        MSPTI_LOGE("subscriber was not subscribe.");
        return MSPTI_ERROR_INVALID_PARAMETER;
    }
    if (!IsValidCBDomain(domain)) {
        MSPTI_LOGE("domain: %d was invalid.", domain);
        return MSPTI_ERROR_INVALID_PARAMETER;
    }
    return (enable != 0) ? Register(domain, cbid) : UnRegister(domain, cbid);
}

msptiResult CallbackManager::EnableDomain(uint32_t enable,
    msptiSubscriberHandle subscriber, msptiCallbackDomain domain)
{
    if (!init_.load()) {
        MSPTI_LOGW("CallbackManager was not init.");
        return MSPTI_SUCCESS;
    }
    if (subscriber != subscriber_ptr_.get()) {
        MSPTI_LOGE("subscriber was not subscribe.");
        return MSPTI_ERROR_INVALID_PARAMETER;
    }
    if (!IsValidCBDomain(domain)) {
        MSPTI_LOGE("domain: %d was invalid.", domain);
        return MSPTI_ERROR_INVALID_PARAMETER;
    }
    auto cbid_set = domain_cbid_map_.find(domain);
    if (cbid_set == domain_cbid_map_.end()) {
        return MSPTI_SUCCESS;
    }
    msptiResult ret = MSPTI_SUCCESS;
    for (const auto& cbid : cbid_set->second) {
        auto reg_ret = (enable != 0) ? Register(domain, cbid) : UnRegister(domain, cbid);
        ret = (reg_ret != MSPTI_SUCCESS) ? reg_ret : ret;
    }
    return ret;
}

void CallbackManager::ExecuteCallback(msptiCallbackDomain domain,
    msptiCallbackId cbid, msptiApiCallbackSite site, const char* funcName)
{
    if (!init_.load()) {
        return;
    }
    if (!IsCallbackIdEnable(domain, cbid)) {
        return;
    }
    if (subscriber_ptr_->handle) {
        MSPTI_LOGD("CallbackManager execute Callbackfunc, funcName is %s", funcName);
        msptiCallbackData callbackData;
        callbackData.callbackSite = site;
        callbackData.functionName = funcName;
        subscriber_ptr_->handle(subscriber_ptr_->userdata, domain, cbid, &callbackData);
    }
}

bool CallbackManager::IsCallbackIdEnable(msptiCallbackDomain domain, msptiCallbackId cbid)
{
    if (!IsValidCBDomain(domain) || !IsValidCBId(cbid)) {
        return false;
    }
    auto idx = static_cast<size_t>(domain);
    uint64_t bits = cbid_map_[idx].load(std::memory_order_relaxed);
    return (bits >> static_cast<int>(cbid)) & 1;
}

}  // Callback
}  // Mspti

msptiResult msptiSubscribe(msptiSubscriberHandle *subscriber, msptiCallbackFunc callback, void *userdata)
{
    return Mspti::Callback::CallbackManager::GetInstance()->Init(subscriber, callback, userdata);
}

msptiResult msptiUnsubscribe(msptiSubscriberHandle subscriber)
{
    if (Mspti::Activity::ActivityManager::GetInstance()->ResetAllDevice() != MSPTI_SUCCESS) {
        MSPTI_LOGE("Reset all device failed.");
    }
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
