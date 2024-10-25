/**
* @file driver_inject_stub.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
*
*/

#include "common/inject/driver_inject.h"
#include "activity/ascend/channel/channel_data.h"
#include "securec.h"

DrvError HalGetDeviceInfo(uint32_t deviceId, int32_t moduleType, int32_t infoType, int64_t* value)
{
    constexpr int64_t VALUE = 1283L;
    *value = VALUE;
    return DRV_ERROR_NONE;
}

DrvError halGetAPIVersion(int32_t* apiVersion)
{
    constexpr int32_t SUPPORT_OSC_FREQ_API_VERSION = 0x071905;
    *apiVersion = SUPPORT_OSC_FREQ_API_VERSION;
    return DRV_ERROR_NONE;
}

int ProfDrvStart(unsigned int deviceId, unsigned int channelId, struct ProfStartPara* startPara)
{
    return 0;
}

int ProfStop(unsigned int deviceId, unsigned int channelId)
{
    return 0;
}

DrvError DrvGetDevNum(uint32_t* count)
{
    *count = 1;
    return DRV_ERROR_NONE;
}

int ProfDrvGetChannels(unsigned int deviceId, ChannelListT* channelList)
{
    return 0;
}

int ProfChannelPoll(struct ProfPollInfo* outBuf, int num, int timeout)
{
    return 0;
}

int ProfChannelRead(unsigned int deviceId, unsigned int channelId, char *outBuf, unsigned int bufSize)
{
    constexpr unsigned int maxBufferSize = 1024 * 1024 * 2;
    constexpr unsigned int maxChannelId = 160;
    if (channelId == maxChannelId) {
        return maxBufferSize;
    }
    if (channelId == PROF_CHANNEL_TS_FW) {
        static bool reportFlag = true;
        if (reportFlag) {
            StepTrace stepTraceData;
            stepTraceData.tsTraceHead.rptType = RPT_TYPE_STEP_TRACE;
            stepTraceData.tagId = STEP_TRACE_TAG_MARKEX;
            auto ret = memcpy_s(outBuf, bufSize, (void*)&stepTraceData, sizeof(stepTraceData));
            if (ret != EOK) {
                return 0;
            }
            TaskFlipInfo taskFlipData;
            taskFlipData.tsTraceHead.rptType = RTP_TYPE_FLIP_INFO;
            ret = memcpy_s(outBuf + sizeof(stepTraceData), bufSize - sizeof(stepTraceData),
                (void*)&taskFlipData, sizeof(taskFlipData));
            if (ret != EOK) {
                return 0;
            }
            reportFlag = false;
            return sizeof(stepTraceData) + sizeof(taskFlipData);
        }
        return 0;
    }
    if (channelId == PROF_CHANNEL_STARS_SOC_LOG) {
        static bool reportFlag = true;
        if (reportFlag) {
            StarsSocLog starsLogData;
            auto ret = memcpy_s(outBuf, bufSize, (void*)&starsLogData, sizeof(starsLogData));
            if (ret != EOK) {
                return 0;
            }
            reportFlag = false;
            return sizeof(starsLogData);
        }
        return 0;
    }
    return 0;
}

DrvError DrvGetDevIDs(uint32_t* devices, uint32_t len)
{
    if (devices) {
        devices[0] = 0;
    }
    return DRV_ERROR_NONE;
}
