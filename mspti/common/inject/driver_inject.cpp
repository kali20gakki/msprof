/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : driver_inject.cpp
 * Description        : Injection of libascend_hal.
 * Author             : msprof team
 * Creation Date      : 2024/05/07
 * *****************************************************************************
*/
#include "common/inject/driver_inject.h"

#include <functional>

#include "common/function_loader.h"

class DriverInject {
public:
    DriverInject() noexcept
    {
        Mspti::Common::RegisterFunction("libascend_hal", "prof_drv_get_channels");
        Mspti::Common::RegisterFunction("libascend_hal", "drvGetDevIDs");
        Mspti::Common::RegisterFunction("libascend_hal", "drvGetDevNum");
        Mspti::Common::RegisterFunction("libascend_hal", "prof_drv_start");
        Mspti::Common::RegisterFunction("libascend_hal", "prof_stop");
        Mspti::Common::RegisterFunction("libascend_hal", "prof_channel_read");
        Mspti::Common::RegisterFunction("libascend_hal", "prof_channel_poll");
        Mspti::Common::RegisterFunction("libascend_hal", "halGetDeviceInfo");
    }
    ~DriverInject() = default;
};

DriverInject g_drvInject;

int ProfDrvGetChannels(unsigned int deviceId, ChannelListT* channelList)
{
    using ProfDrvGetChannelsFunc = std::function<int(unsigned int, ChannelListT*)>;
    static ProfDrvGetChannelsFunc func = nullptr;
    if (func == nullptr) {
        Mspti::Common::GetFunction<int, unsigned int, ChannelListT*>("libascend_hal", "prof_drv_get_channels", func);
    }
    THROW_FUNC_NOTFOUND(func, "prof_drv_get_channels", "libascend_hal.so");
    return func(deviceId, channelList);
}

DrvError DrvGetDevIDs(uint32_t* devices, uint32_t len)
{
    using DrvGetDevIDsFunc = std::function<DrvError(uint32_t*, uint32_t)>;
    static DrvGetDevIDsFunc func = nullptr;
    if (func == nullptr) {
        Mspti::Common::GetFunction<DrvError, uint32_t*, uint32_t>("libascend_hal", "drvGetDevIDs", func);
    }
    THROW_FUNC_NOTFOUND(func, "drvGetDevIDs", "libascend_hal.so");
    return func(devices, len);
}

DrvError DrvGetDevNum(uint32_t* count)
{
    using DrvGetDevNumFunc = std::function<DrvError(uint32_t*)>;
    static DrvGetDevNumFunc func = nullptr;
    if (func == nullptr) {
        Mspti::Common::GetFunction<DrvError, uint32_t*>("libascend_hal", "drvGetDevNum", func);
    }
    THROW_FUNC_NOTFOUND(func, "drvGetDevNum", "libascend_hal.so");
    return func(count);
}

int ProfDrvStart(unsigned int deviceId, unsigned int channelId, struct ProfStartPara* startPara)
{
    using ProfDrvStartFunc = std::function<int(unsigned int, unsigned int, struct ProfStartPara*)>;
    static ProfDrvStartFunc func = nullptr;
    if (func == nullptr) {
        Mspti::Common::GetFunction<int, unsigned int, unsigned int, struct ProfStartPara*>("libascend_hal",
            "prof_drv_start", func);
    }
    THROW_FUNC_NOTFOUND(func, "prof_drv_start", "libascend_hal.so");
    return func(deviceId, channelId, startPara);
}

int ProfStop(unsigned int deviceId, unsigned int channelId)
{
    using ProfStopFunc = std::function<int(unsigned int, unsigned int)>;
    static ProfStopFunc func = nullptr;
    if (func == nullptr) {
        Mspti::Common::GetFunction<int, unsigned int, unsigned int>("libascend_hal", "prof_stop", func);
    }
    THROW_FUNC_NOTFOUND(func, "prof_stop", "libascend_hal.so");
    return func(deviceId, channelId);
}

int ProfChannelRead(unsigned int deviceId, unsigned int channelId, char *outBuf, unsigned int bufSize)
{
    using ProfChannelReadFunc = std::function<int(unsigned int, unsigned int, char*, unsigned int)>;
    static ProfChannelReadFunc func = nullptr;
    if (func == nullptr) {
        Mspti::Common::GetFunction<int, unsigned int, unsigned int, char*, unsigned int>("libascend_hal",
            "prof_channel_read", func);
    }
    THROW_FUNC_NOTFOUND(func, "prof_channel_read", "libascend_hal.so");
    return func(deviceId, channelId, outBuf, bufSize);
}

int ProfChannelPoll(struct ProfPollInfo* outBuf, int num, int timeout)
{
    using ProfChannelPollFunc = std::function<int(struct ProfPollInfo*, int, int)>;
    static ProfChannelPollFunc func = nullptr;
    if (func == nullptr) {
        Mspti::Common::GetFunction<int, struct ProfPollInfo*, int, int>("libascend_hal", "prof_channel_poll", func);
    }
    THROW_FUNC_NOTFOUND(func, "prof_channel_poll", "libascend_hal.so");
    return func(outBuf, num, timeout);
}

DrvError HalGetDeviceInfo(uint32_t deviceId, int32_t moduleType, int32_t infoType, int64_t* value)
{
    using HalGetDeviceInfoFunc = std::function<DrvError(uint32_t, int32_t, int32_t, int64_t*)>;
    static HalGetDeviceInfoFunc func = nullptr;
    if (func == nullptr) {
        Mspti::Common::GetFunction<DrvError, uint32_t, int32_t, int32_t, int64_t*>("libascend_hal",
            "halGetDeviceInfo", func);
    }
    THROW_FUNC_NOTFOUND(func, "halGetDeviceInfo", "libascend_hal.so");
    return func(deviceId, moduleType, infoType, value);
}
