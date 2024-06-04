/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : ascend_manager.cpp
 * Description        : Manager all ascend device prof task.
 * Author             : msprof team
 * Creation Date      : 2024/05/07
 * *****************************************************************************
*/
#include "activity/ascend/ascend_manager.h"

#include "activity/activity_manager.h"
#include "common/plog_manager.h"
#include "common/utils.h"

namespace Mspti {
namespace Ascend {

AscendManager *AscendManager::GetInstance()
{
    static AscendManager instance;
    return &instance;
}

AscendManager::~AscendManager()
{
    for (auto iter = dev_map_.begin(); iter != dev_map_.end(); iter++) {
        iter->second->Stop();
    }
    dev_map_.clear();
}

msptiResult AscendManager::StartDevProfTask(uint32_t deviceId)
{
    MSPTI_LOGI("Start device: %u task.", deviceId);
    std::lock_guard<std::mutex> lk(dev_map_mtx_);
    auto iter = dev_map_.find(deviceId);
    if (iter == dev_map_.end()) {
        std::unique_ptr<Device> device = nullptr;
        Mspti::Common::MsptiMakeUniquePtr(device, deviceId);
        if (!device) {
            MSPTI_LOGE("Failed to init Device for device: %u.", deviceId);
            return MSPTI_ERROR_INNER;
        }
        auto ret = device->Start(Mspti::Activity::ActivityManager::GetInstance()->GetActivities());
        if (ret != MSPTI_SUCCESS) {
            MSPTI_LOGE("The device %u start failed.", deviceId);
            return ret;
        }
        dev_map_.insert({deviceId, std::move(device)});
    } else {
        MSPTI_LOGW("The device %u is already running.", deviceId);
    }
    return MSPTI_SUCCESS;
}

msptiResult AscendManager::StopDevProfTask(uint32_t deviceId)
{
    MSPTI_LOGI("Stop device: %u task.", deviceId);
    std::lock_guard<std::mutex> lk(dev_map_mtx_);
    auto iter = dev_map_.find(deviceId);
    if (iter != dev_map_.end()) {
        iter->second->Stop();
        iter->second.reset(nullptr);
        dev_map_.erase(deviceId);
    }
    return MSPTI_SUCCESS;
}

}  // Ascend
}  // Mspti