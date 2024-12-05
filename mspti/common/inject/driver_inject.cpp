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
#include "common/utils.h"
#include "common/function_loader.h"

namespace {
enum DriverFunctionIndex {
    FUNC_PROF_DRV_GET_CHANNELS,
    FUNC_DRV_GET_DEV_IDS,
    FUNC_DRV_GET_DEV_NUM,
    FUNC_PROF_DRV_START,
    FUNC_PROF_STOP,
    FUNC_PROF_CHANNEL_READ,
    FUNC_PROF_CHANNEL_POLL,
    FUNC_HAL_GET_DEVICE_INFO,
    FUNC_HAL_GET_API_VERSION,
    FUNC_DRIVER_COUNT
};

pthread_once_t g_once;
void* g_driverFuncArray[FUNC_DRIVER_COUNT];

void LoadDriverFunction()
{
    g_driverFuncArray[FUNC_PROF_DRV_GET_CHANNELS] =
        Mspti::Common::RegisterFunction("libascend_hal", "prof_drv_get_channels");
    g_driverFuncArray[FUNC_DRV_GET_DEV_IDS] = Mspti::Common::RegisterFunction("libascend_hal", "drvGetDevIDs");
    g_driverFuncArray[FUNC_DRV_GET_DEV_NUM] = Mspti::Common::RegisterFunction("libascend_hal", "drvGetDevNum");
    g_driverFuncArray[FUNC_PROF_DRV_START] = Mspti::Common::RegisterFunction("libascend_hal", "prof_drv_start");
    g_driverFuncArray[FUNC_PROF_STOP] = Mspti::Common::RegisterFunction("libascend_hal", "prof_stop");
    g_driverFuncArray[FUNC_PROF_CHANNEL_READ] = Mspti::Common::RegisterFunction("libascend_hal", "prof_channel_read");
    g_driverFuncArray[FUNC_PROF_CHANNEL_POLL] = Mspti::Common::RegisterFunction("libascend_hal", "prof_channel_poll");
    g_driverFuncArray[FUNC_HAL_GET_DEVICE_INFO] = Mspti::Common::RegisterFunction("libascend_hal", "halGetDeviceInfo");
    g_driverFuncArray[FUNC_HAL_GET_API_VERSION] = Mspti::Common::RegisterFunction("libascend_hal", "halGetAPIVersion");
}
}

int ProfDrvGetChannels(unsigned int deviceId, ChannelListT* channelList)
{
    pthread_once(&g_once, LoadDriverFunction);
    void* voidFunc = g_driverFuncArray[FUNC_PROF_DRV_GET_CHANNELS];
    using ProfDrvGetChannelsFunc = std::function<int(unsigned int, ChannelListT*)>;
    ProfDrvGetChannelsFunc func = Mspti::Common::ReinterpretConvert<int (*)(unsigned int, ChannelListT*)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<int, unsigned int, ChannelListT*>("libascend_hal", "prof_drv_get_channels", func);
    }
    THROW_FUNC_NOTFOUND(func, "prof_drv_get_channels", "libascend_hal.so");
    return func(deviceId, channelList);
}

DrvError DrvGetDevIDs(uint32_t* devices, uint32_t len)
{
    pthread_once(&g_once, LoadDriverFunction);
    void* voidFunc = g_driverFuncArray[FUNC_DRV_GET_DEV_IDS];
    using DrvGetDevIDsFunc = std::function<DrvError(uint32_t*, uint32_t)>;
    DrvGetDevIDsFunc func = Mspti::Common::ReinterpretConvert<DrvError (*)(uint32_t*, uint32_t)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<DrvError, uint32_t*, uint32_t>("libascend_hal", "drvGetDevIDs", func);
    }
    THROW_FUNC_NOTFOUND(func, "drvGetDevIDs", "libascend_hal.so");
    return func(devices, len);
}

DrvError DrvGetDevNum(uint32_t* count)
{
    pthread_once(&g_once, LoadDriverFunction);
    void* voidFunc = g_driverFuncArray[FUNC_DRV_GET_DEV_NUM];
    using DrvGetDevNumFunc = std::function<DrvError(uint32_t*)>;
    DrvGetDevNumFunc func = Mspti::Common::ReinterpretConvert<DrvError (*)(uint32_t*)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<DrvError, uint32_t*>("libascend_hal", "drvGetDevNum", func);
    }
    THROW_FUNC_NOTFOUND(func, "drvGetDevNum", "libascend_hal.so");
    return func(count);
}

int ProfDrvStart(unsigned int deviceId, unsigned int channelId, struct ProfStartPara* startPara)
{
    pthread_once(&g_once, LoadDriverFunction);
    void* voidFunc = g_driverFuncArray[FUNC_PROF_DRV_START];
    using ProfDrvStartFunc = std::function<int(unsigned int, unsigned int, struct ProfStartPara*)>;
    ProfDrvStartFunc func = Mspti::Common::ReinterpretConvert<int (*)(unsigned int, unsigned int,
        struct ProfStartPara*)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<int, unsigned int, unsigned int, struct ProfStartPara*>("libascend_hal",
            "prof_drv_start", func);
    }
    THROW_FUNC_NOTFOUND(func, "prof_drv_start", "libascend_hal.so");
    return func(deviceId, channelId, startPara);
}

int ProfStop(unsigned int deviceId, unsigned int channelId)
{
    pthread_once(&g_once, LoadDriverFunction);
    void* voidFunc = g_driverFuncArray[FUNC_PROF_STOP];
    using ProfStopFunc = std::function<int(unsigned int, unsigned int)>;
    ProfStopFunc func = Mspti::Common::ReinterpretConvert<int (*)(unsigned int, unsigned int)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<int, unsigned int, unsigned int>("libascend_hal", "prof_stop", func);
    }
    THROW_FUNC_NOTFOUND(func, "prof_stop", "libascend_hal.so");
    return func(deviceId, channelId);
}

int ProfChannelRead(unsigned int deviceId, unsigned int channelId, char *outBuf, unsigned int bufSize)
{
    pthread_once(&g_once, LoadDriverFunction);
    void* voidFunc = g_driverFuncArray[FUNC_PROF_CHANNEL_READ];
    using ProfChannelReadFunc = std::function<int(unsigned int, unsigned int, char*, unsigned int)>;
    ProfChannelReadFunc func = Mspti::Common::ReinterpretConvert<int (*)(unsigned int, unsigned int, char*,
        unsigned int)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<int, unsigned int, unsigned int, char*, unsigned int>("libascend_hal",
            "prof_channel_read", func);
    }
    THROW_FUNC_NOTFOUND(func, "prof_channel_read", "libascend_hal.so");
    return func(deviceId, channelId, outBuf, bufSize);
}

int ProfChannelPoll(struct ProfPollInfo* outBuf, int num, int timeout)
{
    pthread_once(&g_once, LoadDriverFunction);
    void* voidFunc = g_driverFuncArray[FUNC_PROF_CHANNEL_POLL];
    using ProfChannelPollFunc = std::function<int(struct ProfPollInfo*, int, int)>;
    ProfChannelPollFunc func = Mspti::Common::ReinterpretConvert<int (*)(struct ProfPollInfo*, int, int)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<int, struct ProfPollInfo*, int, int>("libascend_hal", "prof_channel_poll", func);
    }
    THROW_FUNC_NOTFOUND(func, "prof_channel_poll", "libascend_hal.so");
    return func(outBuf, num, timeout);
}

DrvError HalGetDeviceInfo(uint32_t deviceId, int32_t moduleType, int32_t infoType, int64_t* value)
{
    pthread_once(&g_once, LoadDriverFunction);
    void* voidFunc = g_driverFuncArray[FUNC_HAL_GET_DEVICE_INFO];
    using HalGetDeviceInfoFunc = std::function<DrvError(uint32_t, int32_t, int32_t, int64_t*)>;
    HalGetDeviceInfoFunc func = Mspti::Common::ReinterpretConvert<DrvError (*)(uint32_t, int32_t, int32_t,
        int64_t*)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<DrvError, uint32_t, int32_t, int32_t, int64_t*>("libascend_hal",
            "halGetDeviceInfo", func);
    }
    THROW_FUNC_NOTFOUND(func, "halGetDeviceInfo", "libascend_hal.so");
    return func(deviceId, moduleType, infoType, value);
}

DrvError halGetAPIVersion(int32_t* apiVersion)
{
    pthread_once(&g_once, LoadDriverFunction);
    void* voidFunc = g_driverFuncArray[FUNC_HAL_GET_API_VERSION];
    using halGetAPIVersionFunc = std::function<DrvError(int32_t*)>;
    halGetAPIVersionFunc func = Mspti::Common::ReinterpretConvert<DrvError (*)(int32_t*)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<DrvError, int32_t*>("libascend_hal", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libascend_hal.so");
    return func(apiVersion);
}
