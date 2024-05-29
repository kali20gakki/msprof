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
        auto dev_ptr = std::make_unique<DevTimeInfo>();
        dev_ptr->freq = GetDevFreq(deviceId);
        dev_ptr->hostStartMonoRawTime = Mspti::Common::Utils::GetClockMonotonicRawNs();
        dev_ptr->devStartSysCnt = GetDevStartSysCnt(deviceId);
        dev_time_info_.insert({deviceId, std::move(dev_ptr)});
    }
    return;
}

uint64_t ContextManager::GetMonotomicFromSysCnt(uint32_t deviceId, uint64_t sysCnt)
{
    // 性能优化，避免频繁加减锁
    std::lock_guard<std::mutex> lk(dev_time_mtx_);
    auto iter = dev_time_info_.find(deviceId);
    if (iter == dev_time_info_.end() || iter->second->freq == 0) {
        return sysCnt;
    }
    uint64_t mono_time = MSTONS * (sysCnt - iter->second->devStartSysCnt) / iter->second->freq +
        iter->second->hostStartMonoRawTime;

    return mono_time;
}

}  // Common
}  // Mspti
