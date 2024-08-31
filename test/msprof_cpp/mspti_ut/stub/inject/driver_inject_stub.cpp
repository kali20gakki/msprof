/**
* @file callback_utest.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
*
*/

#include "common/inject/driver_inject.h"

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
    return 0;
}

DrvError DrvGetDevIDs(uint32_t* devices, uint32_t len)
{
    return DRV_ERROR_NONE;
}
