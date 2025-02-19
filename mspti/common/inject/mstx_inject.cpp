/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : mstx_inject.cpp
 * Description        : Injection of mstx.
 * Author             : msprof team
 * Creation Date      : 2024/05/07
 * *****************************************************************************
*/
#include "common/inject/mstx_inject.h"

#include <cstring>
#include <mutex>

#include "activity/activity_manager.h"
#include "activity/ascend/parser/parser_manager.h"
#include "common/config.h"
#include "common/plog_manager.h"

using namespace Mspti::Activity;
using namespace Mspti::Common;

namespace Mspti {

static std::string g_defaultDomainName = "default";
std::once_flag MstxDomainMgr::onceFlag;
mstxDomainHandle_t MstxDomainMgr::defaultDomainHandle_t;
std::unordered_set<std::string> MstxDomainMgr::domainSwitchs_;
std::unordered_map<mstxDomainHandle_t, std::shared_ptr<MstxDomainAttr>> MstxDomainMgr::domainHandleMap_;

MstxDomainMgr* MstxDomainMgr::GetInstance()
{
    static MstxDomainMgr instance;
    std::call_once(onceFlag, [] {
        domainSwitchs_.insert(g_defaultDomainName);
        std::shared_ptr<MstxDomainAttr> domainHandlePtr;
        Mspti::Common::MsptiMakeSharedPtr(domainHandlePtr);
        if (domainHandlePtr == nullptr) {
            MSPTI_LOGE("Failed to malloc memory for domain attribute, Init DomainMgr Fail");
            return;
        }
        Mspti::Common::MsptiMakeSharedPtr(domainHandlePtr->handle);
        if (domainHandlePtr->handle == nullptr) {
            MSPTI_LOGE("Failed to malloc memory for domain handle, Init DomainMgr Fail");
            return;
        }
        Mspti::Common::MsptiMakeSharedPtr(domainHandlePtr->name, g_defaultDomainName);
        if (domainHandlePtr->name == nullptr) {
            MSPTI_LOGE("Failed to malloc memory for domain %s, Init DomainMgr Fail", g_defaultDomainName);
            return;
        }
        defaultDomainHandle_t = domainHandlePtr->handle.get();
        domainHandleMap_.insert(std::make_pair(domainHandlePtr->handle.get(), domainHandlePtr));
    });
    return &instance;
}

mstxDomainHandle_t MstxDomainMgr::CreateDomainHandle(const char* name)
{
    if (strcmp(g_defaultDomainName.c_str(), name) == 0) {
        MSPTI_LOGE("domain name can not be 'default'!");
        return nullptr;
    }
    std::lock_guard<std::mutex> lk(domainMutex_);
    if (domainHandleMap_.size() > MARK_MAX_CACHE_NUM) {
        MSPTI_LOGE("Cache domain name failed, current size: %u, limit size: %u",
            domainHandleMap_.size(), MARK_MAX_CACHE_NUM);
        return nullptr;
    }
    for (const auto &iter : domainHandleMap_) {
        if (strncmp(iter.second->name->c_str(), name, iter.second->name->size()) == 0) {
            iter.second->isDestroyed = false;
            return iter.first;
        }
    }
    std::shared_ptr<MstxDomainAttr> domainHandlePtr;
    Mspti::Common::MsptiMakeSharedPtr(domainHandlePtr);
    if (domainHandlePtr == nullptr) {
        MSPTI_LOGE("Failed to malloc memory for domain attribute.");
        return nullptr;
    }
    Mspti::Common::MsptiMakeSharedPtr(domainHandlePtr->handle);
    if (domainHandlePtr->handle == nullptr) {
        MSPTI_LOGE("Failed to malloc memory for domain handle.");
        return nullptr;
    }
    Mspti::Common::MsptiMakeSharedPtr(domainHandlePtr->name, name);
    if (domainHandlePtr->name == nullptr) {
        MSPTI_LOGE("Failed to malloc memory for domain %s.", name);
        return nullptr;
    }
    domainHandleMap_.insert(std::make_pair(domainHandlePtr->handle.get(), domainHandlePtr));
    return domainHandlePtr->handle.get();
}

void MstxDomainMgr::DestroyDomainHandle(mstxDomainHandle_t domain)
{
    std::lock_guard<std::mutex> lk(domainMutex_);
    auto iter = domainHandleMap_.find(domain);
    if (iter == domainHandleMap_.end() || iter->second->isDestroyed) {
        MSPTI_LOGW("Input domain is invalid");
        return;
    }
    iter->second->isDestroyed = true;
}

std::shared_ptr<std::string> MstxDomainMgr::GetDomainNameByHandle(mstxDomainHandle_t domain)
{
    std::lock_guard<std::mutex> lk(domainMutex_);
    auto iter = domainHandleMap_.find(domain);
    if (iter == domainHandleMap_.end() || iter->second->isDestroyed) {
        MSPTI_LOGW("Input domain is invalid.");
        return nullptr;
    }
    return iter->second->name;
}

msptiResult MstxDomainMgr::SetMstxDomainEnableStatus(const char* name, bool flag)
{
    std::string nameStr(name);
    std::lock_guard<std::mutex> lk(domainMutex_);
    if (flag) {
        domainSwitchs_.insert(nameStr);
    } else {
        domainSwitchs_.erase(nameStr);
    }
    return MSPTI_SUCCESS;
}

bool MstxDomainMgr::isDomainEnable(mstxDomainHandle_t domainHandle)
{
    auto namePtr = GetDomainNameByHandle(domainHandle);
    if (namePtr) {
        std::lock_guard<std::mutex> lk(domainMutex_);
        return domainSwitchs_.count(*namePtr);
    }
    return false;
}

bool MstxDomainMgr::isDomainEnable(const char* name)
{
    if (!name) {
        return false;
    }
    std::string nameStr(name);
    {
        std::lock_guard<std::mutex> lk(domainMutex_);
        return domainSwitchs_.count(nameStr);
    }
}
}

namespace MsptiMstxApi {

static bool IsMsgValid(const char* msg)
{
    if (msg == nullptr || strnlen(msg, MAX_MARK_MSG_LEN + 1) > MAX_MARK_MSG_LEN) {
        MSPTI_LOGE("Input Params msg is invalid");
        return false;
    }
    return true;
}

void MstxMarkAFunc(const char* msg, RtStreamT stream)
{
    if (!ActivityManager::GetInstance()->IsActivityKindEnable(MSPTI_ACTIVITY_KIND_MARKER)) {
        return;
    }
    if (!IsMsgValid(msg)) {
        return;
    }
    if (!Mspti::MstxDomainMgr::GetInstance()->isDomainEnable(Mspti::g_defaultDomainName.c_str())) {
        return;
    }
    if (Mspti::Parser::ParserManager::GetInstance()->ReportMark(msg, stream, Mspti::g_defaultDomainName.c_str()) !=
        MSPTI_SUCCESS) {
        MSPTI_LOGE("Report Mark data failed.");
    }
}
 
uint64_t MstxRangeStartAFunc(const char* msg, RtStreamT stream)
{
    if (!ActivityManager::GetInstance()->IsActivityKindEnable(MSPTI_ACTIVITY_KIND_MARKER)) {
        return MSTX_INVALID_RANGE_ID;
    }
    if (!IsMsgValid(msg)) {
        return MSTX_INVALID_RANGE_ID;
    }
    if (!Mspti::MstxDomainMgr::GetInstance()->isDomainEnable((Mspti::g_defaultDomainName.c_str()))) {
        return MSTX_INVALID_RANGE_ID;
    }
    uint64_t markId = MSTX_INVALID_RANGE_ID;
    if (Mspti::Parser::ParserManager::GetInstance()->ReportRangeStartA(msg, stream,
        markId, Mspti::g_defaultDomainName.c_str()) != MSPTI_SUCCESS) {
        MSPTI_LOGE("Report RangeStart data failed.");
        return MSTX_INVALID_RANGE_ID;
    }
    return markId;
}
 
void MstxRangeEndFunc(uint64_t rangeId)
{
    if (!ActivityManager::GetInstance()->IsActivityKindEnable(MSPTI_ACTIVITY_KIND_MARKER)) {
        return;
    }
    if (!Mspti::MstxDomainMgr::GetInstance()->isDomainEnable((Mspti::g_defaultDomainName.c_str()))) {
        return;
    }
    if (Mspti::Parser::ParserManager::GetInstance()->ReportRangeEnd(rangeId) != MSPTI_SUCCESS) {
        MSPTI_LOGE("Report RangeEnd data failed.");
    }
}

mstxDomainHandle_t MstxDomainCreateAFunc(const char* name)
{
    if (!IsMsgValid(name)) {
        return nullptr;
    }
    return Mspti::MstxDomainMgr::GetInstance()->CreateDomainHandle(name);
}

void MstxDomainDestroyFunc(mstxDomainHandle_t domain)
{
    Mspti::MstxDomainMgr::GetInstance()->DestroyDomainHandle(domain);
}

void MstxDomainMarkAFunc(mstxDomainHandle_t domain, const char* msg, RtStreamT stream)
{
    if (!ActivityManager::GetInstance()->IsActivityKindEnable(MSPTI_ACTIVITY_KIND_MARKER)) {
        return;
    }
    if (!Mspti::MstxDomainMgr::GetInstance()->isDomainEnable(domain)) {
        return;
    }
    if ((!IsMsgValid(msg))) {
        return;
    }
    auto namePtr = Mspti::MstxDomainMgr::GetInstance()->GetDomainNameByHandle(domain);
    if (namePtr == nullptr) {
        return;
    }
    if (Mspti::Parser::ParserManager::GetInstance()->ReportMark(msg, stream, namePtr->c_str()) !=
        MSPTI_SUCCESS) {
        MSPTI_LOGE("Report Mark data failed.");
    }
}

uint64_t MstxDomainRangeStartAFunc(mstxDomainHandle_t domain, const char* msg, RtStreamT stream)
{
    if (!ActivityManager::GetInstance()->IsActivityKindEnable(MSPTI_ACTIVITY_KIND_MARKER)) {
        return MSTX_INVALID_RANGE_ID;
    }
    if (!Mspti::MstxDomainMgr::GetInstance()->isDomainEnable(domain)) {
        return MSTX_INVALID_RANGE_ID;
    }
    if ((!IsMsgValid(msg))) {
        return MSTX_INVALID_RANGE_ID;
    }
    auto namePtr = Mspti::MstxDomainMgr::GetInstance()->GetDomainNameByHandle(domain);
    if (namePtr == nullptr) {
        return MSTX_INVALID_RANGE_ID;
    }
    uint64_t markId = MSTX_INVALID_RANGE_ID;
    if (Mspti::Parser::ParserManager::GetInstance()->ReportRangeStartA(msg, stream, markId, namePtr->c_str()) !=
        MSPTI_SUCCESS) {
        MSPTI_LOGE("Report RangeStart data failed.");
        return MSTX_INVALID_RANGE_ID;
    }
    return markId;
}

void MstxDomainRangeEndFunc(mstxDomainHandle_t domain, uint64_t rangeId)
{
    if (!ActivityManager::GetInstance()->IsActivityKindEnable(MSPTI_ACTIVITY_KIND_MARKER)) {
        return;
    }
    if (!Mspti::MstxDomainMgr::GetInstance()->isDomainEnable(domain)) {
        MSPTI_LOGW("Domain is disable, range end will not take effect");
        return;
    }
    if (Mspti::MstxDomainMgr::GetInstance()->GetDomainNameByHandle(domain) == nullptr) {
        return;
    }
    if (Mspti::Parser::ParserManager::GetInstance()->ReportRangeEnd(rangeId) != MSPTI_SUCCESS) {
        MSPTI_LOGE("Report RangeEnd data failed.");
    }
}

int GetModuleTableFunc(MstxGetModuleFuncTableFunc getFuncTable)
{
    int retVal = MSTX_SUCCESS;
    unsigned int outSize = 0;
    MstxFuncTable outTable;
    static std::vector<unsigned int> CheckOutTableSizes = {
        0,
        MSTX_FUNC_END,
        MSTX_FUNC_DOMAIN_END,
        0
    };
    for (size_t i = MSTX_API_MODULE_CORE; i < MSTX_API_MODULE_SIZE; i++) {
        if (getFuncTable(static_cast<MstxFuncModule>(i), &outTable, &outSize) != MSPTI_SUCCESS) {
            MSPTI_LOGW("Failed to get func table for module %zu", i);
            continue;
        }
        if (outSize != CheckOutTableSizes[i]) {
            MSPTI_LOGE("outSize is invalid, Failed to init mstx funcs.");
            retVal = MSTX_FAIL;
            break;
        }
        switch (i) {
            case MSTX_API_MODULE_CORE:
                *(outTable[MSTX_FUNC_MARKA]) = (MstxFuncPointer)MstxMarkAFunc;
                *(outTable[MSTX_FUNC_RANGE_STARTA]) = (MstxFuncPointer)MstxRangeStartAFunc;
                *(outTable[MSTX_FUNC_RANGE_END]) = (MstxFuncPointer)MstxRangeEndFunc;
                break;
            case MSTX_API_MODULE_CORE2:
                *(outTable[MSTX_FUNC_DOMAIN_CREATEA]) = (MstxFuncPointer)MstxDomainCreateAFunc;
                *(outTable[MSTX_FUNC_DOMAIN_DESTROY]) = (MstxFuncPointer)MstxDomainDestroyFunc;
                *(outTable[MSTX_FUNC_DOMAIN_MARKA]) = (MstxFuncPointer)MstxDomainMarkAFunc;
                *(outTable[MSTX_FUNC_DOMAIN_RANGE_STARTA]) = (MstxFuncPointer)MstxDomainRangeStartAFunc;
                *(outTable[MSTX_FUNC_DOMAIN_RANGE_END]) = (MstxFuncPointer)MstxDomainRangeEndFunc;
                break;
            default:
                MSPTI_LOGE("Invalid func module type");
                retVal = MSTX_FAIL;
                break;
        }
        if (retVal == MSTX_FAIL) {
            break;
        }
        MSPTI_LOGI("Succeed to get func table for module %zu", i);
    }
    return retVal;
}

}

using namespace MsptiMstxApi;

int InitInjectionMstx(MstxGetModuleFuncTableFunc getFuncTable)
{
    if (getFuncTable == nullptr) {
        MSPTI_LOGE("Input null mstx getfunctable pointer");
        return MSTX_FAIL;
    }
    if (MsptiMstxApi::GetModuleTableFunc(getFuncTable) != MSPTI_SUCCESS) {
        MSPTI_LOGE("Failed to init mstx funcs.");
        return MSTX_FAIL;
    }
    return MSPTI_SUCCESS;
}

msptiResult msptiActivityEnableMarkerDomain(const char* name)
{
    if (!name) {
        MSPTI_LOGE("domainHandle is nullptr, check your input params");
        return MSPTI_ERROR_INVALID_PARAMETER;
    }
    return Mspti::MstxDomainMgr::GetInstance()->SetMstxDomainEnableStatus(name, true);
}

msptiResult msptiActivityDisableMarkerDomain(const char* name)
{
    if (!name) {
        MSPTI_LOGE("domainHandle is nullptr, check your input params");
        return MSPTI_ERROR_INVALID_PARAMETER;
    }
    return Mspti::MstxDomainMgr::GetInstance()->SetMstxDomainEnableStatus(name, false);
}