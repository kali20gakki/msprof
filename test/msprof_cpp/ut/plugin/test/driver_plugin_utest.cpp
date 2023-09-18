/**
* @file driver_plugin_utest.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
*
*/
#include <memory>
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "plugin_handle.h"
#include "utils/utils.h"
#include "driver_plugin.h"
#include "ascend_hal.h"

using namespace Collector::Dvvp::Plugin;
using namespace analysis::dvvp::common::utils;
using namespace Collector::Dvvp::Mmpa;

hdcError_t HalHdcSend(HDC_SESSION session, struct drvHdcMsg *pMsg, uint64_t flag, uint32_t timeout)
{
    return DRV_ERROR_NONE;
}

hdcError_t HalHdcSessionConnectEx(int peerNode, int peerDevid, int peerPid, HDC_CLIENT client, HDC_SESSION *pSession)
{
    return DRV_ERROR_NONE;
}

drvError_t DrvHdcSetSessionReference(HDC_SESSION session)
{
    return DRV_ERROR_NONE;
}

drvError_t HalHdcGetSessionAttr(HDC_SESSION session, int attr, int *value)
{
    return DRV_ERROR_NONE;
}

drvError_t HalGetChipInfo(unsigned int devId, halChipInfo *chipInfo)
{
    return DRV_ERROR_NONE;
}

drvError_t HalGetDeviceInfo(uint32_t devId, int32_t moduleType, int32_t infoType, int64_t *value)
{
    return DRV_ERROR_NONE;
}

drvError_t HalGetApiVersion(int32_t *value)
{
    *value = 0;
    return DRV_ERROR_NONE;
}

drvError_t HalProfDataFlush(unsigned int deviceId, unsigned int channelId, unsigned int *dataLen)
{
    *dataLen = 0;
    return DRV_ERROR_NONE;
}

drvError_t DrvGetChannels(unsigned int deviceId, channel_list_t *channels)
{
    channels->chip_type = 0;
    channels->channel_num = 1;
    channels->channel[0].channel_id = 0;
    return DRV_ERROR_NONE;
}

drvError_t DrvStart(unsigned int deviceId, unsigned int channelId, struct prof_start_para *startPara)
{
    return DRV_ERROR_NONE;
}

drvError_t DrvStop(unsigned int deviceId, unsigned int channelId)
{
    return DRV_ERROR_NONE;
}

drvError_t ChannelRead(unsigned int deviceId, unsigned int channelId, CHAR_PTR outBuf, unsigned int bufSize)
{
    return DRV_ERROR_NONE;
}

drvError_t ChannelPoll(struct prof_poll_info *out_buf, int num, int timeout)
{
    return DRV_ERROR_NONE;
}

drvError_t DrvGetDevNum(uint32_t *numDev)
{
    *numDev = 1;
    return DRV_ERROR_NONE;
}

drvError_t DrvGetDevIDByLocalDevID(uint32_t localDevId, uint32_t *devId)
{
    *devId = 1;
    return DRV_ERROR_NONE;
}

drvError_t DrvGetDevIDs(uint32_t *devices, uint32_t len)
{
    *devices = 1;
    return DRV_ERROR_NONE;
}

class DriverPluginUtest : public testing::Test {
protected:
    virtual void SetUp() {
    }
    virtual void TearDown() {
    }
};

TEST_F(DriverPluginUtest, LoadDriverSo)
{
    GlobalMockObject::verify();
    auto driverPlugin = DriverPlugin::instance();
    MOCKER_CPP(&PluginHandle::OpenPluginFromEnv)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&PluginHandle::OpenPluginFromLdcfg)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    driverPlugin->LoadDriverSo();
    EXPECT_EQ(nullptr, driverPlugin->halHdcRecv_);
    driverPlugin->LoadDriverSo();
    EXPECT_EQ(nullptr, driverPlugin->halHdcRecv_);
    driverPlugin->LoadDriverSo();
}

TEST_F(DriverPluginUtest, IsFuncExist)
{
    GlobalMockObject::verify();
    auto driverPlugin = DriverPlugin::instance();
    MOCKER_CPP(&PluginHandle::IsFuncExist)
        .stubs()
        .will(returnValue(true));
    std::string funcName = "halHdcRecv";
    EXPECT_EQ(true, driverPlugin->IsFuncExist(funcName));
}

TEST_F(DriverPluginUtest, MsprofHalHdcSend)
{
    GlobalMockObject::verify();
    int session = 0;
    drvHdcMsg msg;
    uint64_t flag = 0;
    uint32_t timeout = 0;
    auto driverPlugin = DriverPlugin::instance();
    driverPlugin->halHdcSend_ = nullptr;
    EXPECT_EQ(DRV_ERROR_INVALID_HANDLE, driverPlugin->MsprofHalHdcSend((HDC_SESSION)&session, &msg, flag, timeout));
    driverPlugin->halHdcSend_ = HalHdcSend;
    EXPECT_EQ(DRV_ERROR_NONE, driverPlugin->MsprofHalHdcSend((HDC_SESSION)&session, &msg, flag, timeout));
}

TEST_F(DriverPluginUtest, MsprofDrvHdcSetSessionReference)
{
    GlobalMockObject::verify();
    int session = 0;
    auto driverPlugin = DriverPlugin::instance();
    driverPlugin->drvHdcSetSessionReference_ = nullptr;
    EXPECT_EQ(DRV_ERROR_INVALID_HANDLE, driverPlugin->MsprofDrvHdcSetSessionReference((HDC_SESSION)&session));
    driverPlugin->drvHdcSetSessionReference_ = DrvHdcSetSessionReference;
    EXPECT_EQ(DRV_ERROR_NONE, driverPlugin->MsprofDrvHdcSetSessionReference((HDC_SESSION)&session));
}

TEST_F(DriverPluginUtest, MsprofHalHdcGetSessionAttr)
{
    GlobalMockObject::verify();
    int session = 0;
    int attr = 0;
    int value = 0;
    auto driverPlugin = DriverPlugin::instance();
    driverPlugin->halHdcGetSessionAttr_ = nullptr;
    EXPECT_EQ(DRV_ERROR_INVALID_HANDLE, driverPlugin->MsprofHalHdcGetSessionAttr((HDC_SESSION)&session, attr, &value));
    driverPlugin->halHdcGetSessionAttr_ = HalHdcGetSessionAttr;
    EXPECT_EQ(DRV_ERROR_NONE, driverPlugin->MsprofHalHdcGetSessionAttr((HDC_SESSION)&session, attr, &value));
}

TEST_F(DriverPluginUtest, MsprofHalGetChipInfo)
{
    GlobalMockObject::verify();
    unsigned int devId = 0;
    halChipInfo chipInfo;
    auto driverPlugin = DriverPlugin::instance();
    driverPlugin->halGetChipInfo_ = nullptr;
    EXPECT_EQ(DRV_ERROR_INVALID_HANDLE, driverPlugin->MsprofHalGetChipInfo(devId, &chipInfo));
    driverPlugin->halGetChipInfo_ = HalGetChipInfo;
    EXPECT_EQ(DRV_ERROR_NONE, driverPlugin->MsprofHalGetChipInfo(devId, &chipInfo));
}

TEST_F(DriverPluginUtest, MsprofHalGetDeviceInfo)
{
    GlobalMockObject::verify();
    uint32_t devId = 0;
    int32_t moduleType = 0;
    int32_t infoType = 0;
    int64_t value;
    auto driverPlugin = DriverPlugin::instance();
    driverPlugin->halGetDeviceInfo_ = nullptr;
    EXPECT_EQ(DRV_ERROR_INVALID_HANDLE, driverPlugin->MsprofHalGetDeviceInfo(devId, moduleType, infoType, &value));
    driverPlugin->halGetDeviceInfo_ = HalGetDeviceInfo;
    EXPECT_EQ(DRV_ERROR_NONE, driverPlugin->MsprofHalGetDeviceInfo(devId, moduleType, infoType, &value));
}

TEST_F(DriverPluginUtest, MsprofHalGetApiVersion)
{
    GlobalMockObject::verify();
    int32_t value;
    auto driverPlugin = DriverPlugin::instance();
    driverPlugin->halGetApiVersion_ = nullptr;
    EXPECT_EQ(DRV_ERROR_INVALID_HANDLE, driverPlugin->MsprofHalGetApiVersion(&value));
    driverPlugin->halGetApiVersion_ = HalGetApiVersion;
    EXPECT_EQ(DRV_ERROR_NONE, driverPlugin->MsprofHalGetApiVersion(&value));
    EXPECT_EQ(0, value);
}

TEST_F(DriverPluginUtest, MsprofHalProfDataFlush)
{
    GlobalMockObject::verify();
    unsigned int deviceId = 0;
    unsigned int channelId = 0;
    unsigned int dataLen;
    auto driverPlugin = DriverPlugin::instance();
    driverPlugin->halProfDataFlush_ = nullptr;
    EXPECT_EQ(DRV_ERROR_INVALID_HANDLE, driverPlugin->MsprofHalProfDataFlush(deviceId, channelId, &dataLen));
    driverPlugin->halProfDataFlush_ = HalProfDataFlush;
    EXPECT_EQ(DRV_ERROR_NONE, driverPlugin->MsprofHalProfDataFlush(deviceId, channelId, &dataLen));
    EXPECT_EQ(0, dataLen);
}

TEST_F(DriverPluginUtest, MsprofDrvGetChannels)
{
    GlobalMockObject::verify();
    unsigned int deviceId = 0;
    channel_list_t channels;
    auto driverPlugin = DriverPlugin::instance();
    driverPlugin->profDrvGetChannels_ = nullptr;
    EXPECT_EQ(DRV_ERROR_INVALID_HANDLE, driverPlugin->MsprofDrvGetChannels(deviceId, &channels));
    driverPlugin->profDrvGetChannels_ = DrvGetChannels;
    EXPECT_EQ(DRV_ERROR_NONE, driverPlugin->MsprofDrvGetChannels(deviceId, &channels));
    EXPECT_EQ(0, channels.chip_type);
    EXPECT_EQ(1, channels.channel_num);
    EXPECT_EQ(0, channels.channel[0].channel_id);
}

TEST_F(DriverPluginUtest, MsprofDrvStart)
{
    GlobalMockObject::verify();
    unsigned int deviceId = 0;
    unsigned int channelId = 0;
    prof_start_para start_para;
    auto driverPlugin = DriverPlugin::instance();
    driverPlugin->profDrvStart_ = nullptr;
    EXPECT_EQ(DRV_ERROR_INVALID_HANDLE, driverPlugin->MsprofDrvStart(deviceId, channelId, &start_para));
    driverPlugin->profDrvStart_ = DrvStart;
    EXPECT_EQ(DRV_ERROR_NONE, driverPlugin->MsprofDrvStart(deviceId, channelId, &start_para));
}

TEST_F(DriverPluginUtest, MsprofDrvStop)
{
    GlobalMockObject::verify();
    unsigned int deviceId = 0;
    unsigned int channelId = 0;
    auto driverPlugin = DriverPlugin::instance();
    driverPlugin->profStop_ = nullptr;
    EXPECT_EQ(DRV_ERROR_INVALID_HANDLE, driverPlugin->MsprofDrvStop(deviceId, channelId));
    driverPlugin->profStop_ = DrvStop;
    EXPECT_EQ(DRV_ERROR_NONE, driverPlugin->MsprofDrvStop(deviceId, channelId));
}

TEST_F(DriverPluginUtest, MsprofChannelRead)
{
    GlobalMockObject::verify();
    unsigned int deviceId = 0;
    unsigned int channelId = 0;
    char outBuf[1] = {0};
    unsigned int bufSize = 0;
    auto driverPlugin = DriverPlugin::instance();
    driverPlugin->profChannelRead_ = nullptr;
    EXPECT_EQ(DRV_ERROR_INVALID_HANDLE, driverPlugin->MsprofChannelRead(deviceId, channelId, outBuf, bufSize));
    driverPlugin->profChannelRead_ = ChannelRead;
    EXPECT_EQ(DRV_ERROR_NONE, driverPlugin->MsprofChannelRead(deviceId, channelId, outBuf, bufSize));
}

TEST_F(DriverPluginUtest, MsprofChannelPoll)
{
    GlobalMockObject::verify();
    struct prof_poll_info info;
    int num = 0;
    int timeout = 0;
    auto driverPlugin = DriverPlugin::instance();
    driverPlugin->profChannelPoll_ = nullptr;
    EXPECT_EQ(DRV_ERROR_INVALID_HANDLE, driverPlugin->MsprofChannelPoll(&info, num, timeout));
    driverPlugin->profChannelPoll_ = ChannelPoll;
    EXPECT_EQ(DRV_ERROR_NONE, driverPlugin->MsprofChannelPoll(&info, num, timeout));
}

TEST_F(DriverPluginUtest, MsprofDrvGetDevNum)
{
    GlobalMockObject::verify();
    uint32_t numDev = 0;
    auto driverPlugin = DriverPlugin::instance();
    driverPlugin->drvGetDevNum_ = nullptr;
    EXPECT_EQ(DRV_ERROR_INVALID_HANDLE, driverPlugin->MsprofDrvGetDevNum(&numDev));
    driverPlugin->drvGetDevNum_ = DrvGetDevNum;
    EXPECT_EQ(DRV_ERROR_NONE, driverPlugin->MsprofDrvGetDevNum(&numDev));
    EXPECT_EQ(1, numDev);
}

TEST_F(DriverPluginUtest, MsprofDrvGetDevIDByLocalDevID)
{
    GlobalMockObject::verify();
    uint32_t localDevId = 0;
    uint32_t devId = 0;
    auto driverPlugin = DriverPlugin::instance();
    driverPlugin->drvGetDevIDByLocalDevID_ = nullptr;
    EXPECT_EQ(DRV_ERROR_INVALID_HANDLE, driverPlugin->MsprofDrvGetDevIDByLocalDevID(localDevId, &devId));
    driverPlugin->drvGetDevIDByLocalDevID_ = DrvGetDevIDByLocalDevID;
    EXPECT_EQ(DRV_ERROR_NONE, driverPlugin->MsprofDrvGetDevIDByLocalDevID(localDevId, &devId));
    EXPECT_EQ(1, devId);
}

TEST_F(DriverPluginUtest, MsprofDrvGetDevIDs)
{
    GlobalMockObject::verify();
    uint32_t len = 1;
    uint32_t devices[len];
    auto driverPlugin = DriverPlugin::instance();
    driverPlugin->drvGetDevIDs_ = nullptr;
    EXPECT_EQ(DRV_ERROR_INVALID_HANDLE, driverPlugin->MsprofDrvGetDevIDs(devices, len));
    driverPlugin->drvGetDevIDs_ = DrvGetDevIDs;
    EXPECT_EQ(DRV_ERROR_NONE, driverPlugin->MsprofDrvGetDevIDs(devices, len));
    EXPECT_EQ(1, devices[0]);
}