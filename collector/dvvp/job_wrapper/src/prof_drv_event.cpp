/**
* @file prof_drv_event.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "prof_drv_event.h"

#include "errno/error_code.h"
#include "driver_plugin.h"

namespace Analysis {
namespace Dvvp {
namespace JobWrapper {
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::driver;
using namespace Collector::Dvvp::Plugin;
using namespace Collector::Dvvp::Mmpa;

ProfDrvEvent::ProfDrvEvent() {}

ProfDrvEvent::~ProfDrvEvent() {}

int ProfDrvEvent::SubscribeEventThreadInit(uint32_t devId, TaskEventAttr *eventAttr, std::string grpName) const
{
    drvError_t ret = DriverPlugin::instance()->MsprofHalEschedAttachDevice(devId);  // attach process to device
    if (ret != DRV_ERROR_NONE) {
        MSPROF_LOGE("Call MsprofHalEschedAttachDevice failed, devId=%u, ret=%d", devId, ret);
        return PROFILING_FAILED;
    }

    /* process attach device only need once */
    struct esched_grp_para grpPara = {GRP_TYPE_BIND_CP_CPU, 1, {0}, {0}};
    if (strcpy_s(grpPara.grp_name, EVENT_MAX_GRP_NAME_LEN, grpName.c_str()) != EOK) {
        MSPROF_LOGE("Call strcpy for grp name: %s failed", grpName.c_str());
        return PROFILING_FAILED;
    }
    uint32_t grpId = 0;
    ret = DriverPlugin::instance()->MsprofHalEschedCreateGrpEx(devId, &grpPara, &grpId);
    if (ret != DRV_ERROR_NONE) {
        (void)DriverPlugin::instance()->MsprofHalEschedDettachDevice(devId);  // dettach process from device
        MSPROF_LOGE("Call MsprofHalEschedCreateGrpEx failed. (devId=%u, ret=%d)\n", devId, ret);
        return PROFILING_FAILED;
    }
    MSPROF_LOGI("create grp id:%u by name '%s'", grpId, grpName.c_str());

    uint64_t eventBitmap = (1ULL << static_cast<uint64_t>(EVENT_USR_START));
    ret = DriverPlugin::instance()->MsprofHalEschedSubscribeEvent(devId, grpId, 0, eventBitmap);
    if (ret != DRV_ERROR_NONE) {
        (void)DriverPlugin::instance()->MsprofHalEschedDettachDevice(devId);
        MSPROF_LOGE("Call MsprofHalEschedSubscribeEvent failed, devId=%u, ret=%d", devId, ret);
        return PROFILING_FAILED;
    }
    eventAttr->deviceId = devId;
    eventAttr->groupId = grpId;
    int status = CreateWaitEventThread(eventAttr);
    if (status != PROFILING_SUCCESS) {
        (void)DriverPlugin::instance()->MsprofHalEschedDettachDevice(devId);
        MSPROF_LOGE("Call CreateWaitEventThread failed, devId=%u, ret=%d", devId, status);
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

int ProfDrvEvent::CreateWaitEventThread(TaskEventAttr *eventAttr) const
{
    MmUserBlockT userBlock;
    MmThreadAttr threadAttr = {0, 0, 0, 0, 0, 0, 0};
    userBlock.procFunc = ProfDrvEvent::WaitEventThread;
    userBlock.pulArg = eventAttr;
    int ret = MmCreateTaskWithThreadAttr(&eventAttr->handle, &userBlock, &threadAttr);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("Start task wait event thread for device %u failed, strerr : %s",
                    eventAttr->deviceId, strerror(MmGetErrorCode()));
        return PROFILING_FAILED;
    }
    eventAttr->isThreadStart = true;

    return PROFILING_SUCCESS;
}

int ProfDrvEvent::HandleEvent(drvError_t err, struct event_info &event, TaskEventAttr *eventAttr, bool &onceFlag)
{
    if (err == DRV_ERROR_NONE) {
        MSPROF_LOGI("Receive event, devId=%u, event_id=%u", eventAttr->deviceId, event.comm.event_id);
        if (event.comm.event_id == EVENT_USR_START) {
            eventAttr->isChannelValid = true;
            int ret = CollectionRegisterMgr::instance()->CollectionJobRun(eventAttr->deviceId, eventAttr->jobTag);
            return ret;
        }
        return PROFILING_SUCCESS;
    } else if ((err == DRV_ERROR_WAIT_TIMEOUT) || (err == DRV_ERROR_NO_EVENT)) {
        MSPROF_LOGD("Wait event time out or no event, devId=%u, event_id=%u, onceFlag=%d",
                    eventAttr->deviceId, event.comm.event_id, onceFlag);
        if (!onceFlag) {
            return PROFILING_FAILED;
        }
        onceFlag = false;
        if (DrvChannelsMgr::instance()->GetAllChannels(eventAttr->deviceId) == PROFILING_SUCCESS &&
            DrvChannelsMgr::instance()->ChannelIsValid(eventAttr->deviceId, eventAttr->channelId)) {
            MSPROF_LOGI("Channel is valid, channelId=%d", static_cast<int32_t>(eventAttr->channelId));
            eventAttr->isChannelValid = true;
            int ret = CollectionRegisterMgr::instance()->CollectionJobRun(eventAttr->deviceId, eventAttr->jobTag);
            return ret;
        }
    } else {
        MSPROF_LOGW("Wait event failed, devId=%u, event_id=%u, ret=%d", eventAttr->deviceId, event.comm.event_id, err);
    }
    return PROFILING_FAILED;
}

void *ProfDrvEvent::WaitEventThread(void *attr)
{
    if (attr == nullptr) {
        return nullptr;
    }
    TaskEventAttr *eventAttr = static_cast<TaskEventAttr *>(attr);
    MSPROF_EVENT("Start wait drv event, device id:%u, channel id:%d", eventAttr->deviceId, eventAttr->channelId);
    struct event_info event;
    bool onceFlag = true;
    int32_t timeout = 1;  // first 1ms timeout to check channel is valid
    while (!eventAttr->isExit) {
        drvError_t err = DriverPlugin::instance()->MsprofHalEschedWaitEvent(eventAttr->deviceId,
                                                                            eventAttr->groupId, 0, timeout, &event);
        timeout = 100;  // 100ms timeout
        if (ProfDrvEvent::HandleEvent(err, event, eventAttr, onceFlag) == PROFILING_SUCCESS) {
            break;
        }
    }

    MSPROF_EVENT("Event thread exit, device id:%u, channel id:%d", eventAttr->deviceId, eventAttr->channelId);
    return nullptr;
}

void ProfDrvEvent::SubscribeEventThreadUninit(uint32_t devId) const
{
    // dettach process from device
    drvError_t ret = DriverPlugin::instance()->MsprofHalEschedDettachDevice(devId);
    if (ret != DRV_ERROR_NONE) {
        MSPROF_LOGW("call MsprofHalEschedDettachDevice ret: %d", ret);
    }
}
}  // namespace JobWrapper
}  // namespace Dvvp
}  // namespace Analysis