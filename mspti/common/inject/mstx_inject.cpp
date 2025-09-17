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
#include "activity/ascend/parser/mstx_parser.h"
#include "common/utils.h"
#include "common/plog_manager.h"
#include "mstx_inject.h"

using namespace Mspti::Activity;
using namespace Mspti::Common;

namespace Mspti {

static std::string g_defaultDomainName = "default";
std::once_flag MstxDomainMgr::onceFlag;
mstxDomainHandle_t MstxDomainMgr::defaultDomainHandle_t;
std::unordered_set<std::string> MstxDomainMgr::disableDomains_;
std::unordered_map<mstxDomainHandle_t, std::shared_ptr<MstxDomainAttr>> MstxDomainMgr::domainHandleMap_;

MstxDomainMgr* MstxDomainMgr::GetInstance()
{
    static MstxDomainMgr instance;
    std::call_once(onceFlag, [] {
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

mstxDomainHandle_t MstxDomainMgr::CreateDomainHandle(const char *name)
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
    // domain默认使能，除非用户提前调用msptiActivityDisableMarkerDomain去使能该domain
    if (disableDomains_.count(*domainHandlePtr->name) != 0) {
        domainHandlePtr->isEnabled = false;
    }
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
        disableDomains_.erase(nameStr);
    } else {
        disableDomains_.insert(nameStr);
    }
    for (const auto &iter : domainHandleMap_) {
        if (nameStr == *(iter.second->name)) {
            iter.second->isEnabled = flag;
            break;
        }
    }
    return MSPTI_SUCCESS;
}

bool MstxDomainMgr::isDomainEnable(mstxDomainHandle_t domainHandle)
{
    std::lock_guard<std::mutex> lk(domainMutex_);
    auto iter = domainHandleMap_.find(domainHandle);
    return iter == domainHandleMap_.end() ? false : iter->second->isEnabled;
}

bool MstxDomainMgr::isDomainEnable(const char* name)
{
    if (!name) {
        return false;
    }
    std::string nameStr(name);
    std::lock_guard<std::mutex> lk(domainMutex_);
    return disableDomains_.count(nameStr) == 0 ? true : false;
}
}

namespace MsptiMstxApi {

static bool IsMsgValid(const char* msg)
{
    if (msg == nullptr) {
        MSPTI_LOGE("Input Params msg is null");
        return false;
    }
    if (strnlen(msg, MAX_MARK_MSG_LEN) == MAX_MARK_MSG_LEN) {
        MSPTI_LOGE("Input Params msg length exceeds the maximum value %d", MAX_MARK_MSG_LEN);
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
    if (Mspti::Parser::MstxParser::GetInstance()->ReportMark(msg, stream, Mspti::g_defaultDomainName.c_str()) !=
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
    if (Mspti::Parser::MstxParser::GetInstance()->ReportRangeStartA(msg, stream,
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
    if (Mspti::Parser::MstxParser::GetInstance()->ReportRangeEnd(rangeId) != MSPTI_SUCCESS) {
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
    if (Mspti::Parser::MstxParser::GetInstance()->ReportMark(msg, stream, namePtr->c_str()) !=
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
    if (Mspti::Parser::MstxParser::GetInstance()->ReportRangeStartA(msg, stream, markId, namePtr->c_str()) !=
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
    if (Mspti::Parser::MstxParser::GetInstance()->ReportRangeEnd(rangeId) != MSPTI_SUCCESS) {
        MSPTI_LOGE("Report RangeEnd data failed.");
    }
}

void SetMstxModuleCoreApi(MstxFuncTable outTable, unsigned int size)
{
    if (size >= static_cast<unsigned int>(MSTX_FUNC_MARKA)) {
        *(outTable[MSTX_FUNC_MARKA]) = (MstxFuncPointer)MstxMarkAFunc;
    }
    if (size >= static_cast<unsigned int>(MSTX_FUNC_RANGE_STARTA)) {
        *(outTable[MSTX_FUNC_RANGE_STARTA]) = (MstxFuncPointer)MstxRangeStartAFunc;
    }
    if (size >= static_cast<unsigned int>(MSTX_FUNC_RANGE_END)) {
        *(outTable[MSTX_FUNC_RANGE_END]) = (MstxFuncPointer)MstxRangeEndFunc;
    }
}

void SetMstxModuleCoreDomainApi(MstxFuncTable outTable, unsigned int size)
{
    if (size >= static_cast<unsigned int>(MSTX_FUNC_DOMAIN_CREATEA)) {
        *(outTable[MSTX_FUNC_DOMAIN_CREATEA]) = (MstxFuncPointer)MstxDomainCreateAFunc;
    }
    if (size >= static_cast<unsigned int>(MSTX_FUNC_DOMAIN_DESTROY)) {
        *(outTable[MSTX_FUNC_DOMAIN_DESTROY]) = (MstxFuncPointer)MstxDomainDestroyFunc;
    }
    if (size >= static_cast<unsigned int>(MSTX_FUNC_DOMAIN_MARKA)) {
        *(outTable[MSTX_FUNC_DOMAIN_MARKA]) = (MstxFuncPointer)MstxDomainMarkAFunc;
    }
    if (size >= static_cast<unsigned int>(MSTX_FUNC_DOMAIN_RANGE_STARTA)) {
        *(outTable[MSTX_FUNC_DOMAIN_RANGE_STARTA]) = (MstxFuncPointer)MstxDomainRangeStartAFunc;
    }
    if (size >= static_cast<unsigned int>(MSTX_FUNC_DOMAIN_RANGE_END)) {
        *(outTable[MSTX_FUNC_DOMAIN_RANGE_END]) = (MstxFuncPointer)MstxDomainRangeEndFunc;
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
        switch (i) {
            case MSTX_API_MODULE_CORE:
                SetMstxModuleCoreApi(outTable, outSize);
                break;
            case MSTX_API_MODULE_CORE_DOMAIN:
                SetMstxModuleCoreDomainApi(outTable, outSize);
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

msptiResult MstxRegistMstxFunc()
{
    MSPTI_LOGI("mspti registeMstxFunc to profcommon.so");
    ProfRegisteMstxFunc(InitInjectionMstx, PROF_MODULE_MSPTI);
    return MSPTI_SUCCESS;
}

void MsptiEnableMstxFunc()
{
    MSPTI_LOGI("mspti enable mstxFuncs");
    EnableMstxFunc(PROF_MODULE_MSPTI);
}

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
}

using namespace MsptiMstxApi;
static msptiResult res = MstxRegistMstxFunc();

msptiResult msptiActivityEnableMarkerDomain(const char* name)
{
    if (!IsMsgValid(name)) {
        MSPTI_LOGE("domainHandle is nullptr, check your input params");
        return MSPTI_ERROR_INVALID_PARAMETER;
    }
    MSPTI_LOGI("Set domain %s collection on", name);
    return Mspti::MstxDomainMgr::GetInstance()->SetMstxDomainEnableStatus(name, true);
}

msptiResult msptiActivityDisableMarkerDomain(const char* name)
{
    if (!IsMsgValid(name)) {
        MSPTI_LOGE("domainHandle is nullptr, check your input params");
        return MSPTI_ERROR_INVALID_PARAMETER;
    }
    MSPTI_LOGI("Set domain %s collection off", name);
    return Mspti::MstxDomainMgr::GetInstance()->SetMstxDomainEnableStatus(name, false);
}