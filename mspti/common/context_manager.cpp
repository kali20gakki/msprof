/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : context_manager.cpp
 * Description        : Global context manager.
 * Author             : msprof team
 * Creation Date      : 2024/05/07
 * *****************************************************************************
*/

#include "common/context_manager.h"

#include "common/config.h"
#include "common/plog_manager.h"
#include "common/inject/driver_inject.h"
#include "common/utils.h"
#include "activity/activity_manager.h"
#include "activity/ascend/reporter/external_correlation_reporter.h"

namespace Mspti {
namespace Common {

ContextManager *ContextManager::GetInstance()
{
    static ContextManager instance;
    return &instance;
}

static int64_t GetDrvVersion(uint32_t deviceId)
{
    constexpr int64_t ERR_VERSION = -1;
    int64_t version = 0;
    DrvError ret = HalGetDeviceInfo(deviceId, DRV_MODULE_TYPE_SYSTEM, DRV_INFO_TYPE_VERSION, &version);
    return (ret == DRV_ERROR_NONE) ? version : ERR_VERSION;
}

static PlatformType GetChipTypeImpl(uint32_t deviceId)
{
    int64_t versionInfo = GetDrvVersion(deviceId);
    if (versionInfo < 0) {
        MSPTI_LOGE("Call GetDrvVersion failed, deviceId: %u.", deviceId);
        return PlatformType::END_TYPE;
    }
    uint32_t chipId = ((static_cast<uint64_t>(versionInfo) >> 8) & 0xff);
    if (chipId >= static_cast<uint32_t>(PlatformType::END_TYPE)) {
        MSPTI_LOGE("Get Chip Type failed, deviceId: %u.", deviceId);
        return PlatformType::END_TYPE;
    }
    return static_cast<PlatformType>(chipId);
}

static uint64_t GetDevFreq(uint32_t device)
{
    constexpr uint64_t DEFAULT_FREQ = 50;
    static const std::unordered_map<PlatformType, uint64_t> FREQ_MAP = {
        {PlatformType::CHIP_910B, 50},
        {PlatformType::CHIP_310B, 50},
    };
    int64_t freq = 0;
    DrvError ret = HalGetDeviceInfo(device, DRV_MODULE_TYPE_SYSTEM, DRV_INFO_TYPE_DEV_OSC_FREQUE, &freq);
    if (ret != DRV_ERROR_NONE) {
        auto platform = GetChipTypeImpl(device);
        auto iter = FREQ_MAP.find(platform);
        uint64_t defaultFreq = (iter == FREQ_MAP.end()) ? DEFAULT_FREQ : iter->second;
        MSPTI_LOGW("Get Device: %u osc freq failed. use default freq: %lu", device, defaultFreq);
        return defaultFreq;
    }
    return freq;
}

static uint64_t GetHostFreq()
{
    constexpr uint64_t ERR_FREQ = 0;
    int64_t freq = 0;
    DrvError ret = HalGetDeviceInfo(0, DRV_MODULE_TYPE_SYSTEM, DRV_INFO_TYPE_HOST_OSC_FREQUE, &freq);
    return (ret == DRV_ERROR_NONE) ? freq : ERR_FREQ;
}

static bool HostFreqIsEnableImpl()
{
    int32_t apiVersion = 0;
    constexpr int32_t SUPPORT_OSC_FREQ_API_VERSION = 0x071905; // 支持获取host freq的驱动版本号
    if (halGetAPIVersion(&apiVersion) != DRV_ERROR_NONE) {
        return false;
    }
    bool ret = false;
    if (apiVersion >= SUPPORT_OSC_FREQ_API_VERSION) {
        ret = GetHostFreq() > 0;
    }
    return ret;
}

static uint64_t GetDevStartSysCnt(uint32_t device)
{
    constexpr uint64_t ERR_SYSCNT = 0;
    int64_t syscnt = 0;
    DrvError ret = HalGetDeviceInfo(device, DRV_MODULE_TYPE_SYSTEM, DRV_INFO_TYPE_SYS_COUNT, &syscnt);
    return (ret == DRV_ERROR_NONE) ? static_cast<uint64_t>(syscnt) : ERR_SYSCNT;
}

void ContextManager::InitDevTimeInfo(uint32_t deviceId)
{
    static constexpr uint32_t AVE_NUM = 2;
    std::lock_guard<std::mutex> lk(devTimeMtx_);
    auto iter = devTimeInfo_.find(deviceId);
    if (iter == devTimeInfo_.end()) {
        std::unique_ptr<DevTimeInfo> dev_ptr = nullptr;
        Mspti::Common::MsptiMakeUniquePtr(dev_ptr);
        if (!dev_ptr) {
            return;
        }
        dev_ptr->freq = GetDevFreq(deviceId);
        auto t1 = Mspti::Common::Utils::GetClockRealTimeNs();
        dev_ptr->startSysCnt = GetDevStartSysCnt(deviceId);
        auto t2 = Mspti::Common::Utils::GetClockRealTimeNs();
        dev_ptr->startRealTime = (t2 + t1) / AVE_NUM;
        devTimeInfo_.insert({deviceId, std::move(dev_ptr)});
    }
}

bool ContextManager::HostFreqIsEnable()
{
    static bool flag = HostFreqIsEnableImpl();
    return flag;
}

void ContextManager::InitHostTimeInfo()
{
    hostTimeInfo_ = nullptr;
    static constexpr uint32_t AVE_NUM = 2;
    if (!HostFreqIsEnable()) {
        return;
    }
    Mspti::Common::MsptiMakeUniquePtr(hostTimeInfo_);
    if (!hostTimeInfo_) {
        return;
    }
    hostTimeInfo_->freq = GetHostFreq();
    auto t1 = Mspti::Common::Utils::GetClockRealTimeNs();
    hostTimeInfo_->startSysCnt = Mspti::Common::Utils::GetHostSysCnt();
    auto t2 = Mspti::Common::Utils::GetClockRealTimeNs();
    hostTimeInfo_->startRealTime = (t2 + t1) / AVE_NUM;
}

uint64_t ContextManager::GetRealTimeFromSysCnt(uint32_t deviceId, uint64_t sysCnt)
{
    std::lock_guard<std::mutex> lk(devTimeMtx_);
    auto iter = devTimeInfo_.find(deviceId);
    if (iter == devTimeInfo_.end() || iter->second->freq == 0) {
        return sysCnt;
    }
    uint64_t real_time = MSTONS * (sysCnt - iter->second->startSysCnt) / iter->second->freq +
        iter->second->startRealTime;
    return real_time;
}

uint64_t ContextManager::GetRealTimeFromSysCnt(uint64_t sysCnt)
{
    if (!hostTimeInfo_ || hostTimeInfo_->freq == 0) {
        return sysCnt;
    }
    return MSTONS * (sysCnt - hostTimeInfo_->startSysCnt) / hostTimeInfo_->freq +
        hostTimeInfo_->startRealTime;
}

uint64_t ContextManager::GetHostTimeStampNs()
{
    return HostFreqIsEnable() ? GetRealTimeFromSysCnt(Mspti::Common::Utils::GetHostSysCnt()):
        Mspti::Common::Utils::GetClockRealTimeNs();
}

PlatformType ContextManager::GetChipType(uint32_t deviceId)
{
    return GetChipTypeImpl(deviceId);
}

uint64_t ContextManager::GetCorrelationId(uint32_t threadId)
{
    std::lock_guard<std::mutex> lk(correlationIdMtx_);
    uint32_t tid = (threadId > 0 ? threadId : Common::Utils::GetTid());
    auto iter = threadCorrelationIdInfo_.find(tid);
    if (iter == threadCorrelationIdInfo_.end()) {
        MSPTI_LOGE("Thread %u get correlation id failed.", tid);
        return MSPTI_INVALID_CORRELATION_ID;
    } else {
        return iter->second;
    }
}

void ContextManager::UpdateAndReportCorrelationId()
{
    std::lock_guard<std::mutex> lk(correlationIdMtx_);
    correlationId_++;
    Mspti::Reporter::ExternalCorrelationReporter::GetInstance()->ReportExternalCorrelationId(correlationId_);
    uint32_t tid = Common::Utils::GetTid();
    auto iter = threadCorrelationIdInfo_.find(tid);
    if (iter == threadCorrelationIdInfo_.end()) {
        threadCorrelationIdInfo_.insert({tid, correlationId_});
    } else {
        iter->second = correlationId_;
    }
}

}  // Common
}  // Mspti
