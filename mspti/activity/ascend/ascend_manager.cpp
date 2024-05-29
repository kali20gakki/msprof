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

#include "activity/activity_manager.h"
#include "activity/ascend/ascend_manager.h"

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
    std::lock_guard<std::mutex> lk(dev_map_mtx_);
    auto iter = dev_map_.find(deviceId);
    if (iter == dev_map_.end()) {
        auto device = std::make_unique<Device>(deviceId);
        if (!device) {
            printf("Malloc Failed\n");
            return MSPTI_ERROR_MEMORY_MALLOC;
        }
        auto ret = device->Start(Mspti::Activity::ActivityManager::GetInstance()->GetActivities());
        if (ret != MSPTI_SUCCESS) {
            printf("[ERROR] Device %u start failed.\n", deviceId);
            return ret;
        }
        dev_map_.insert({deviceId, std::move(device)});
    } else {
        printf("[WARN] Device %u is running.\n", deviceId);
    }
    return MSPTI_SUCCESS;
}

msptiResult AscendManager::StopDevProfTask(uint32_t deviceId)
{
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