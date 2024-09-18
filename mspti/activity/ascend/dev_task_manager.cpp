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
#include "common/inject/profapi_inject.h"
#include "common/utils.h"
#include "securec.h"

namespace Mspti {
namespace Ascend {

std::map<msptiActivityKind, uint64_t> DevTaskManager::datatype_config_map_ = {
    {MSPTI_ACTIVITY_KIND_KERNEL, MSPTI_CONFIG_KERNEL},
    {MSPTI_ACTIVITY_KIND_API, MSPTI_CONFIG_API},
};

DevTaskManager *DevTaskManager::GetInstance()
{
    static DevTaskManager instance;
    return &instance;
}

DevTaskManager::~DevTaskManager()
{
    for (auto iter = task_map_.begin(); iter != task_map_.end(); iter++) {
        StopAllDevKindProfTask(iter->second);
    }
    task_map_.clear();
    Mspti::Ascend::Channel::ChannelPoolManager::GetInstance()->UnInit();
}

DevTaskManager::DevTaskManager()
{
    Mspti::Ascend::Channel::ChannelPoolManager::GetInstance()->Init();
    Mspti::Common::ContextManager::GetInstance()->InitHostTimeInfo();
    RegisterReportCallback();
}

msptiResult DevTaskManager::StartAllDevKindProfTask(std::vector<std::unique_ptr<DevProfTask>>& profTasks)
{
    msptiResult ret = MSPTI_SUCCESS;
    for (auto& profTask : profTasks) {
        if (profTask->Start() != MSPTI_SUCCESS) {
            ret = MSPTI_ERROR_INNER;
        }
    }
    return ret;
}

msptiResult DevTaskManager::StopAllDevKindProfTask(std::vector<std::unique_ptr<DevProfTask>>& profTasks)
{
    msptiResult ret = MSPTI_SUCCESS;
    for (auto& profTask : profTasks) {
        if (profTask->Stop() != MSPTI_SUCCESS) {
            ret = MSPTI_ERROR_INNER;
        }
        profTask.reset(nullptr);
    }
    return ret;
}

msptiResult DevTaskManager::StartDevProfTask(uint32_t deviceId, msptiActivityKind kind)
{
    if (!CheckDeviceOnline(deviceId)) {
        MSPTI_LOGE("Device: %u is offline.", deviceId);
        return MSPTI_ERROR_INNER;
    }
    MSPTI_LOGI("Start DevProfTask, deviceId: %u, kind: %d", deviceId, kind);
    if (Mspti::Ascend::Channel::ChannelPoolManager::GetInstance()->GetAllChannels(deviceId) != MSPTI_SUCCESS) {
        MSPTI_LOGE("Get device: %u channels failed.", deviceId);
        return MSPTI_ERROR_INNER;
    }
    Mspti::Common::ContextManager::GetInstance()->InitDevTimeInfo(deviceId);
    // 根据DeviceId配置项，开启CANN软件栈的Profiling任务
    if (StartCannProfTask(deviceId, kind) != MSPTI_SUCCESS) {
        MSPTI_LOGE("Start CannProfTask failed. deviceId: %u, kind: %d", deviceId, static_cast<int>(kind));
        return MSPTI_ERROR_INNER;
    }
    std::lock_guard<std::mutex> lk(task_map_mtx_);
    auto iter = task_map_.find({deviceId, kind});
    if (iter == task_map_.end()) {
        auto profTasks = Mspti::Ascend::DevProfTaskFactory::CreateTasks(deviceId, kind);
        auto ret = StartAllDevKindProfTask(profTasks);
        if (ret != MSPTI_SUCCESS) {
            MSPTI_LOGE("The device %u start DevProfTask failed.", deviceId);
            return ret;
        }
        task_map_.insert({{deviceId, kind}, std::move(profTasks)});
    } else {
        MSPTI_LOGW("The device: %u, kind: %d is already running.", deviceId, kind);
    }
    return MSPTI_SUCCESS;
}

msptiResult DevTaskManager::StopDevProfTask(uint32_t deviceId, msptiActivityKind kind)
{
    if (!CheckDeviceOnline(deviceId)) {
        MSPTI_LOGE("Device: %u is offline.", deviceId);
        return MSPTI_ERROR_INNER;
    }
    MSPTI_LOGI("Stop DevProfTask, deviceId: %u, kind: %d", deviceId, kind);
    if (StopCannProfTask(deviceId, kind) != MSPTI_SUCCESS) {
        MSPTI_LOGE("Stop CannProfTask failed. deviceId: %u, kind: %d", deviceId, static_cast<int>(kind));
        return MSPTI_ERROR_INNER;
    }
    msptiResult ret = MSPTI_SUCCESS;
    std::lock_guard<std::mutex> lk(task_map_mtx_);
    auto iter = task_map_.find({deviceId, kind});
    if (iter != task_map_.end()) {
        ret = StopAllDevKindProfTask(iter->second);
        task_map_.erase(iter);
    }
    return ret;
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
    constexpr int32_t DEVICE_MAX_NUM = 64;
    if (ret != DRV_ERROR_NONE || deviceNum > DEVICE_MAX_NUM) {
        MSPTI_LOGE("Get device num failed.");
        return;
    }
    uint32_t deviceList[DEVICE_MAX_NUM] = {0};
    ret = DrvGetDevIDs(deviceList, deviceNum);
    if (ret != DRV_ERROR_NONE) {
        MSPTI_LOGE("Get device id list failed.");
        return;
    }
    device_set_.insert(std::begin(deviceList), std::end(deviceList));
}

void DevTaskManager::RegisterReportCallback()
{
    if (Mspti::Inject::profRegReporterCallback(Mspti::Inject::MsprofReporterCallbackImpl) != MSPTI_SUCCESS) {
        MSPTI_LOGE("Failed to register origin reporter callback");
    }
    static const std::vector<std::pair<int, VOID_PTR>> CALLBACK_FUNC_LIST = {
        {PROFILE_REPORT_GET_HASH_ID_C_CALLBACK,
            reinterpret_cast<VOID_PTR>(Mspti::Inject::MsptiGetHashIdImpl)},
        {PROFILE_HOST_FREQ_IS_ENABLE_C_CALLBACK,
            reinterpret_cast<VOID_PTR>(Mspti::Inject::MsptiHostFreqIsEnableImpl)},
        {PROFILE_REPORT_API_C_CALLBACK,
            reinterpret_cast<VOID_PTR>(Mspti::Inject::MsptiApiReporterCallbackImpl)},
        {PROFILE_REPORT_EVENT_C_CALLBACK,
            reinterpret_cast<VOID_PTR>(Mspti::Inject::MsptiEventReporterCallbackImpl)},
        {PROFILE_REPORT_COMPACT_CALLBACK,
            reinterpret_cast<VOID_PTR>(Mspti::Inject::MsptiCompactInfoReporterCallbackImpl)},
        {PROFILE_REPORT_ADDITIONAL_CALLBACK,
            reinterpret_cast<VOID_PTR>(Mspti::Inject::MsptiAddiInfoReporterCallbackImpl)},
        {PROFILE_REPORT_REG_TYPE_INFO_C_CALLBACK,
            reinterpret_cast<VOID_PTR>(Mspti::Inject::MsptiRegReportTypeInfoImpl)},
    };
    for (auto iter : CALLBACK_FUNC_LIST) {
        auto ret = Mspti::Inject::MsprofRegisterProfileCallback(iter.first, iter.second, sizeof(VOID_PTR));
        if (ret != MSPTI_SUCCESS) {
            MSPTI_LOGE("Failed to register reporter callback: %d", static_cast<int32_t>(iter.first));
        }
    }
}

msptiResult DevTaskManager::StartCannProfTask(uint32_t deviceId, msptiActivityKind kind)
{
    auto cfg_iter = datatype_config_map_.find(kind);
    if (cfg_iter == datatype_config_map_.end()) {
        MSPTI_LOGW("Device: %u, kind: %d don't need to start cann profiling task.", deviceId, static_cast<int>(kind));
        return MSPTI_SUCCESS;
    }
    CommandHandle command;
    if (memset_s(&command, sizeof(command), 0, sizeof(command)) != EOK) {
        MSPTI_LOGE("memset CommandHandle failed.");
        return MSPTI_ERROR_INNER;
    }
    command.profSwitch = cfg_iter->second;
    command.profSwitchHi = 0;
    command.devNums = 1;
    command.devIdList[0] = deviceId;
    command.modelId = PROF_INVALID_MODE_ID;
    command.type = PROF_COMMANDHANDLE_TYPE_START;
    auto ret = Mspti::Inject::profSetProfCommand(static_cast<VOID_PTR>(&command), sizeof(CommandHandle));
    if (ret != MSPTI_SUCCESS) {
        MSPTI_LOGE("Start Profiling Commond failed.");
        return MSPTI_ERROR_INNER;
    }
    return MSPTI_SUCCESS;
}

msptiResult DevTaskManager::StopCannProfTask(uint32_t deviceId, msptiActivityKind kind)
{
    auto cfg_iter = datatype_config_map_.find(kind);
    if (cfg_iter == datatype_config_map_.end()) {
        MSPTI_LOGW("Device: %u, kind: %d don't need to stop cann profiling task.", deviceId, static_cast<int>(kind));
        return MSPTI_SUCCESS;
    }
    CommandHandle command;
    if (memset_s(&command, sizeof(command), 0, sizeof(command)) != EOK) {
        MSPTI_LOGE("memset CommandHandle failed.");
        return MSPTI_ERROR_INNER;
    }
    command.profSwitch = cfg_iter->second;
    command.profSwitchHi = 0;
    command.devNums = 1;
    command.devIdList[0] = deviceId;
    command.modelId = PROF_INVALID_MODE_ID;
    command.type = PROF_COMMANDHANDLE_TYPE_STOP;
    auto ret = Mspti::Inject::profSetProfCommand(static_cast<VOID_PTR>(&command), sizeof(CommandHandle));
    if (ret != MSPTI_SUCCESS) {
        MSPTI_LOGE("Stop Profiling Commond failed.");
        return MSPTI_ERROR_INNER;
    }
    return MSPTI_SUCCESS;
}
}  // Ascend
}  // Mspti
