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

namespace Mspti {
namespace Common {

ContextManager *ContextManager::GetInstance()
{
    static ContextManager instance;
    return &instance;
}

static uint64_t GetDevFreq(uint32_t device)
{
    constexpr uint64_t ERR_FREQ = 0;
    int64_t freq = 0;
    DrvError ret = HalGetDeviceInfo(device, DRV_MODULE_TYPE_SYSTEM, DRV_INFO_TYPE_DEV_OSC_FREQUE, &freq);
    return (ret == DRV_ERROR_NONE) ? freq : ERR_FREQ;
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
    std::lock_guard<std::mutex> lk(dev_time_mtx_);
    auto iter = dev_time_info_.find(deviceId);
    if (iter == dev_time_info_.end()) {
        std::unique_ptr<DevTimeInfo> dev_ptr = nullptr;
        Mspti::Common::MsptiMakeUniquePtr(dev_ptr);
        if (!dev_ptr) {
            return;
        }
        dev_ptr->freq = GetDevFreq(deviceId);
        dev_ptr->hostStartMonoRawTime = Mspti::Common::Utils::GetClockMonotonicRawNs();
        dev_ptr->devStartSysCnt = GetDevStartSysCnt(deviceId);
        dev_time_info_.insert({deviceId, std::move(dev_ptr)});
    }
    return;
}

uint64_t ContextManager::GetMonotomicFromSysCnt(uint32_t deviceId, uint64_t sysCnt)
{
    std::lock_guard<std::mutex> lk(dev_time_mtx_);
    auto iter = dev_time_info_.find(deviceId);
    if (iter == dev_time_info_.end() || iter->second->freq == 0) {
        return sysCnt;
    }
    uint64_t mono_time = MSTONS * (sysCnt - iter->second->devStartSysCnt) / iter->second->freq +
        iter->second->hostStartMonoRawTime;

    return mono_time;
}

PlatformType ContextManager::GetChipType(uint32_t deviceId)
{
    int64_t versionInfo = 0;
    DrvError ret = HalGetDeviceInfo(deviceId, DRV_MODULE_TYPE_SYSTEM, DRV_INFO_TYPE_VERSION, &versionInfo);
    if (ret != DRV_ERROR_NONE) {
        MSPTI_LOGE("Call HalGetDeviceInfo failed, deviceId: %u.", deviceId);
        return PlatformType::END_TYPE;
    }
    uint32_t chipId = ((static_cast<uint64_t>(versionInfo) >> 8) & 0xff);
    if (chipId >= static_cast<uint32_t>(PlatformType::END_TYPE)) {
        MSPTI_LOGE("Get Chip Type failed, deviceId: %u.", deviceId);
        return PlatformType::END_TYPE;
    }
    return static_cast<PlatformType>(chipId);
}

uint64_t ContextManager::CorrelationId()
{
    return correlation_id_.load();
}

void ContextManager::UpdateCorrelationId()
{
    correlation_id_++;
}

}  // Common
}  // Mspti
