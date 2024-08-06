/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : dev_task_manager.cpp
 * Description        : Manager all ascend device prof task.
 * Author             : msprof team
 * Creation Date      : 2024/05/07
 * *****************************************************************************
*/
#include "activity/ascend/dev_task_manager.h"

#include "activity/activity_manager.h"
#include "activity/ascend/channel/channel_pool_manager.h"
#include "common/context_manager.h"
#include "common/plog_manager.h"
#include "common/inject/driver_inject.h"
#include "common/utils.h"

namespace Mspti {
namespace Ascend {

DevTaskManager *DevTaskManager::GetInstance()
{
    static DevTaskManager instance;
    return &instance;
}

DevTaskManager::~DevTaskManager()
{
    for (auto iter = task_map_.begin(); iter != task_map_.end(); iter++) {
        iter->second->Stop();
    }
    task_map_.clear();
    Mspti::Ascend::Channel::ChannelPoolManager::GetInstance()->UnInit();
}

DevTaskManager::DevTaskManager()
{
    Mspti::Ascend::Channel::ChannelPoolManager::GetInstance()->Init();
}

msptiResult DevTaskManager::StartDevProfTask(uint32_t deviceId, msptiActivityKind kind)
{
    if (!CheckDeviceOnline(deviceId)) {
        MSPTI_LOGW("Device: %u is offline.", deviceId);
        return MSPTI_SUCCESS;
    }
    Mspti::Ascend::Channel::ChannelPoolManager::GetInstance()->GetAllChannels(deviceId);
    Mspti::Common::ContextManager::GetInstance()->InitDevTimeInfo(deviceId);
    std::lock_guard<std::mutex> lk(task_map_mtx_);
    auto iter = task_map_.find({deviceId, kind});
    if (iter == task_map_.end()) {
        std::unique_ptr<DevProfTask> profTask = Mspti::Ascend::DevProfTaskFactory::CreateTask(deviceId, kind);
        if (!profTask) {
            MSPTI_LOGE("Failed to init DevTask for device: %u.", deviceId);
            return MSPTI_ERROR_INNER;
        }
        auto ret = profTask->Start();
        if (ret != MSPTI_SUCCESS) {
            MSPTI_LOGE("The device %u start failed.", deviceId);
            return ret;
        }
        task_map_.insert({{deviceId, kind}, std::move(profTask)});
    } else {
        MSPTI_LOGW("The device: %u, kind: %d is already running.", deviceId, kind);
    }
    return MSPTI_SUCCESS;
}

msptiResult DevTaskManager::StopDevProfTask(uint32_t deviceId, msptiActivityKind kind)
{
    std::lock_guard<std::mutex> lk(task_map_mtx_);
    auto iter = task_map_.find({deviceId, kind});
    if (iter != task_map_.end()) {
        iter->second->Stop();
        iter->second.reset(nullptr);
        task_map_.erase({deviceId, kind});
    }
    return MSPTI_SUCCESS;
}

bool DevTaskManager::CheckDeviceOnline(uint32_t deviceId)
{
    std::call_once(get_device_flag_, [this] () {
        this->InitDeviceList();
    });
    return device_set_.find(deviceId) != device_set_.end() ? true : false;
}

void DevTaskManager::InitDeviceList()
{
    uint32_t deviceNum = 0;
    auto ret = DrvGetDevNum(&deviceNum);
    if (ret != DRV_ERROR_NONE) {
        MSPTI_LOGE("Get device num failed.");
        return;
    }
    constexpr int32_t DEVICE_MAX_NUM = 64;
    uint32_t deviceList[DEVICE_MAX_NUM] = {0};
    ret = DrvGetDevIDs(deviceList, deviceNum);
    if (ret != DRV_ERROR_NONE) {
        MSPTI_LOGE("Get device id list failed.");
        return;
    }
    device_set_.insert(std::begin(deviceList), std::end(deviceList));
}

}  // Ascend
}  // Mspti