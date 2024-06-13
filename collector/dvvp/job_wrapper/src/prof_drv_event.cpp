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
#include "ascend_hal.h"

namespace Analysis {
namespace Dvvp {
namespace JobWrapper {
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::driver;
using namespace Collector::Dvvp::Plugin;
using namespace Collector::Dvvp::Mmpa;

const int32_t DRV_EVENT_TIMEOUT = 100;

ProfDrvEvent::ProfDrvEvent() {}

ProfDrvEvent::~ProfDrvEvent() {}

int ProfDrvEvent::SubscribeEventThreadInit(struct TaskEventAttr *eventAttr) const
{
    MmUserBlockT userBlock;
    MmThreadAttr threadAttr = {0, 0, 0, 0, 0, 0, 0};
    userBlock.procFunc = ProfDrvEvent::EventThreadHandle;
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

void *ProfDrvEvent::EventThreadHandle(void *attr)
{
    if (attr == nullptr) {
        MSPROF_LOGE("attr is nullptr");
        return nullptr;
    }
    struct TaskEventAttr *eventAttr = static_cast<struct TaskEventAttr *>(attr);
    MSPROF_EVENT("Start drv event thread, device id:%u, channel id:%d", eventAttr->deviceId, eventAttr->channelId);
 
    if (QueryDevPid(eventAttr) != PROFILING_SUCCESS) {    // make sure aicpu bind pid before attach
        MSPROF_LOGE("Query devPid failed, devId:%u.", eventAttr->deviceId);
        return nullptr;
    }
    drvError_t ret = DriverPlugin::instance()->MsprofHalEschedAttachDevice(eventAttr->deviceId);
    if (ret != DRV_ERROR_NONE) {
        MSPROF_LOGE("Call halEschedAttachDevice failed, devId=%u, ret=%d", eventAttr->deviceId, ret);
        return nullptr;
    }
    eventAttr->isAttachDevice = true;
 
    uint32_t grpId = 0;
    if (QueryGroupId(eventAttr->deviceId, grpId, eventAttr->grpName) != PROFILING_SUCCESS) {
        /* multiple start and stop profiling, create grpId and subscribe event only need once */
        struct esched_grp_para grpPara = {GRP_TYPE_BIND_CP_CPU, 1, {0}, {0}};
        if (strcpy_s(grpPara.grp_name, EVENT_MAX_GRP_NAME_LEN, eventAttr->grpName.c_str()) != EOK) {
            MSPROF_LOGE("Call strcpy for grp name: %s failed", eventAttr->grpName.c_str());
            return nullptr;
        }
        ret = DriverPlugin::instance()->MsprofHalEschedCreateGrpEx(eventAttr->deviceId, &grpPara, &grpId);
        if (ret != DRV_ERROR_NONE) {
            (void)DriverPlugin::instance()->MsprofHalEschedDettachDevice(eventAttr->deviceId);
            MSPROF_LOGE("Call halEschedCreateGrpEx failed. (devId=%u, ret=%d)\n", eventAttr->deviceId, ret);
            return nullptr;
        }
        MSPROF_LOGI("create grp id:%u by name '%s'", grpId, eventAttr->grpName);
 
        uint64_t eventBitmap = (1ULL << static_cast<uint64_t>(EVENT_USR_START));
        ret = DriverPlugin::instance()->MsprofHalEschedSubscribeEvent(eventAttr->deviceId, grpId, 0, eventBitmap);
        if (ret != DRV_ERROR_NONE) {
            (void)DriverPlugin::instance()->MsprofHalEschedDettachDevice(eventAttr->deviceId);
            MSPROF_LOGE("Call halEschedSubscribeEvent failed, devId=%u, ret=%d", eventAttr->deviceId, ret);
            return nullptr;
        }
    }
 
    WaitEvent(eventAttr, grpId);
 
    MSPROF_LOGI("Event thread exit, device id:%u, channel id:%d", eventAttr->deviceId, eventAttr->channelId);
    return nullptr;
}

int ProfDrvEvent::QueryGroupId(uint32_t devId, uint32_t &grpId, std::string grpName)
{
    struct esched_query_gid_output gidOut = {0};
    struct esched_query_gid_input gidIn = {0, {0}};
    struct esched_output_info outPut = {&gidOut, sizeof(struct esched_query_gid_output)};
    struct esched_input_info inPut = {&gidIn, sizeof(struct esched_query_gid_input)};
 
    gidIn.pid = MmGetPid();
    if (strcpy_s(gidIn.grp_name, EVENT_MAX_GRP_NAME_LEN, grpName.c_str()) != EOK) {
        MSPROF_LOGE("strcpy failed for drv event grp name");
        return PROFILING_FAILED;
    }
    
    drvError_t ret = DriverPlugin::instance()->MsprofHalEschedQueryInfo(devId, QUERY_TYPE_LOCAL_GRP_ID,
                                                                        &inPut, &outPut);
    if (ret == DRV_ERROR_NONE) {
        grpId = gidOut.grp_id;
        MSPROF_LOGI("Query group id %u by name '%s'", grpId, grpName.c_str());
        return PROFILING_SUCCESS;
    }

    MSPROF_LOGD("Query group id failed by name: '%s'", grpName.c_str());
    return PROFILING_FAILED;
}

int ProfDrvEvent::QueryDevPid(struct TaskEventAttr *eventAttr)
{
    enum devdrv_process_type procType = DEVDRV_PROCESS_CPTYPE_MAX;
    if (eventAttr->channelId == PROF_CHANNEL_AICPU || eventAttr->channelId == PROF_CHANNEL_CUS_AICPU) {
        procType = DEVDRV_PROCESS_CP1;
    } else {
        return PROFILING_SUCCESS;
    }
    const uint32_t WAIT_TIME = 20;
    const int32_t WAIT_COUNT = 80;
    pid_t devPid = 0;
    int32_t hostPid = MmGetPid();
    struct halQueryDevpidInfo hostpidinfo = {static_cast<pid_t>(hostPid), eventAttr->deviceId, 0, procType, {0}};
    drvError_t ret = DRV_ERROR_NOT_SUPPORT;
    for (int32_t i = 0; (i < WAIT_COUNT) && (!eventAttr->isExit); i++) {
        ret = DriverPlugin::instance()->MsprofHalQueryDevpid(hostpidinfo, &devPid);
        if (ret == DRV_ERROR_NONE) {
            MSPROF_LOGI("Query devPid succ, devId:%u, hostPid:%d, devPid:%d", eventAttr->deviceId, hostPid, devPid);
            return PROFILING_SUCCESS;
        }
        MSPROF_LOGD("Query devPid failed, devId:%u, hostPid:%d, ret:%d.", eventAttr->deviceId, hostPid, ret);
        MmSleep(WAIT_TIME);
    }
    return PROFILING_FAILED;
}

void ProfDrvEvent::WaitEvent(struct TaskEventAttr *eventAttr, uint32_t grpId)
{
    MSPROF_LOGI("Start wait drv event, device id:%u, channel id:%d", eventAttr->deviceId, eventAttr->channelId);
    struct event_info event;
    bool onceFlag = true;
    int32_t timeout = 1;  // first timeout need to check channel is valid
    drvError_t err;
    while (!eventAttr->isExit) {
        err = DriverPlugin::instance()->MsprofHalEschedWaitEvent(eventAttr->deviceId, grpId, 0, timeout, &event);
        timeout = DRV_EVENT_TIMEOUT;
        if (err == DRV_ERROR_NONE) {
            MSPROF_LOGI("Receive event, devId=%u, event_id=%u", eventAttr->deviceId, event.comm.event_id);
            if (event.comm.event_id == EVENT_USR_START) {
                eventAttr->isChannelValid = true;
                (void)CollectionRegisterMgr::instance()->CollectionJobRun(eventAttr->deviceId, eventAttr->jobTag);
            }
            return;
        } else if ((err == DRV_ERROR_SCHED_WAIT_TIMEOUT) || (err == DRV_ERROR_NO_EVENT)) {
            MSPROF_LOGD("Wait event time out or no event, devId=%u, event_id=%u, onceFlag=%d",
                eventAttr->deviceId, event.comm.event_id, onceFlag);
            if (!onceFlag) {
                continue;
            }
            onceFlag = false;
 
            if (DrvChannelsMgr::instance()->GetAllChannels(eventAttr->deviceId) == PROFILING_SUCCESS &&
                DrvChannelsMgr::instance()->ChannelIsValid(eventAttr->deviceId, eventAttr->channelId)) {
                MSPROF_LOGI("Channel is valid, channelId=%d", static_cast<int32_t>(eventAttr->channelId));
                eventAttr->isChannelValid = true;
                (void)CollectionRegisterMgr::instance()->CollectionJobRun(eventAttr->deviceId, eventAttr->jobTag);
                return;
            }
        } else {
            MSPROF_LOGE("Wait event failed, device id:%u, channel id:%d, event id:%u, return:%d",
                eventAttr->deviceId, eventAttr->channelId, event.comm.event_id, err);
            return;
        }
    }
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