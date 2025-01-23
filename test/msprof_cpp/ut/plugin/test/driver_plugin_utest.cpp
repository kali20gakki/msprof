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

drvError_t HalEschedCreateGrpEx(uint32_t devId,
                                struct esched_grp_para *grpPara, unsigned int *grpId)
{
    return DRV_ERROR_NONE;
}

drvError_t HalEschedAttachDevice(unsigned int devId)
{
    return DRV_ERROR_NONE;
}

drvError_t HalEschedDettachDevice(unsigned int devId)
{
    return DRV_ERROR_NONE;
}

drvError_t HalEschedSubscribeEvent(unsigned int devId, unsigned int grpId,
                                   unsigned int threadId, unsigned long long eventBitmap)
{
    return DRV_ERROR_NONE;
}

drvError_t HalEschedWaitEvent(unsigned int devId, unsigned int grpId,
                              unsigned int threadId, int timeout, struct event_info *event)
{
    return DRV_ERROR_NONE;
}

drvError_t DrvGetDeviceSplitMode(unsigned int devId, unsigned int* mode)
{
    return DRV_ERROR_NONE;
}

drvError_t HalEschedQueryInfo(unsigned int devId, ESCHED_QUERY_TYPE type,
                              struct esched_input_info *inPut, struct esched_output_info *outPut)
{
    return DRV_ERROR_NONE;
}

drvError_t HalQueryDevpid(struct halQueryDevpidInfo pidInfo, pid_t *pid)
{
    return DRV_ERROR_NONE;
}

drvError_t DrvGetPlatformInfo(uint32_t *info)
{
    *info = 1;
    return DRV_ERROR_NONE;
}

drvError_t DrvHdcClientCreate(HDC_CLIENT *client, int maxSessionNum, int serviceType, int flag)
{
    return DRV_ERROR_NONE;
}

drvError_t DrvHdcClientDestroy(HDC_CLIENT client)
{
    return DRV_ERROR_NONE;
}

drvError_t DrvHdcServerCreate(int devid, int serviceType, HDC_SERVER *pServer)
{
    return DRV_ERROR_NONE;
}

drvError_t DrvHdcServerDestroy(HDC_SERVER pServer)
{
    return DRV_ERROR_NONE;
}

drvError_t DrvHdcSessionAccept(HDC_SERVER server, HDC_SESSION *session)
{
    return DRV_ERROR_NONE;
}

drvError_t DrvHdcGetMsgBuffer(struct drvHdcMsg *msg, int index, CHAR_PTR_PTR pBuf, int *pLen)
{
    return DRV_ERROR_NONE;
}

drvError_t DrvHdcAllocMsg(HDC_SESSION session, struct drvHdcMsg **ppMsg, int count)
{
    return DRV_ERROR_NONE;
}

drvError_t DrvHdcFreeMsg(struct drvHdcMsg *msg)
{
    return DRV_ERROR_NONE;
}

drvError_t DrvHdcReuseMsg(struct drvHdcMsg *msg)
{
    return DRV_ERROR_NONE;
}

drvError_t DrvHdcAddMsgBuffer(struct drvHdcMsg *msg, CHAR_PTR pBuf, int len)
{
    return DRV_ERROR_NONE;
}

drvError_t DrvHdcSessionConnect(int peer_node, int peer_devid, HDC_CLIENT client, HDC_SESSION *session)
{
    return DRV_ERROR_NONE;
}

drvError_t DrvHdcSessionClose(HDC_SESSION session)
{
    return DRV_ERROR_NONE;
}

drvError_t DrvHdcGetCapacity(struct drvHdcCapacity *capacity)
{
    return DRV_ERROR_NONE;
}

drvError_t HalGetDeviceInfoByBuff(uint32_t devId, int32_t moduleType, int32_t infoType, VOID_PTR buf, int32_t *size)
{
    return DRV_ERROR_NONE;
}

class DriverPluginUtest : public testing::Test {
protected:
    virtual void SetUp() {
    }
    virtual void TearDown() {
    }
};

TEST_F(DriverPluginUtest, LoadDriverSoWillCreatePluginHandleWhenFirstLoad)
{
    GlobalMockObject::verify();
    auto driverPlugin = DriverPlugin::instance();
    MOCKER_CPP(&DriverPlugin::GetAllFunction)
        .stubs();
    MOCKER_CPP(&PluginHandle::HasLoad)
        .stubs()
        .will(returnValue(true));
    EXPECT_EQ(nullptr, driverPlugin->pluginHandle_);
    driverPlugin->LoadDriverSo();
    EXPECT_NE(nullptr, driverPlugin->pluginHandle_);
    driverPlugin->pluginHandle_ = nullptr;
}

TEST_F(DriverPluginUtest, LoadDriverSoWillReturnWhenOpenPluginFromEnvAndOpenPluginFromLdcfgFail)
{
    GlobalMockObject::verify();
    auto driverPlugin = DriverPlugin::instance();
    MOCKER_CPP(&PluginHandle::HasLoad)
        .stubs()
        .will(returnValue(false));
    MOCKER_CPP(&PluginHandle::OpenPluginFromEnv)
        .stubs()
        .will(returnValue(PROFILING_FAILED));
    MOCKER_CPP(&PluginHandle::OpenPluginFromLdcfg)
        .stubs()
        .will(returnValue(PROFILING_FAILED));
    driverPlugin->LoadDriverSo();
    EXPECT_EQ(nullptr, driverPlugin->halHdcRecv_);
    driverPlugin->pluginHandle_ = nullptr;
}

TEST_F(DriverPluginUtest, LoadDriverSoWillGetFunctionWhenOpenPluginFromEnvSucc)
{
    GlobalMockObject::verify();
    auto driverPlugin = DriverPlugin::instance();
    MOCKER_CPP(&PluginHandle::HasLoad)
        .stubs()
        .will(returnValue(false));
    MOCKER_CPP(&PluginHandle::OpenPluginFromEnv)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    driverPlugin->LoadDriverSo();
    driverPlugin->pluginHandle_ = nullptr;
}

TEST_F(DriverPluginUtest, LoadDriverSoWillGetFunctionWhenOpenPluginFromLdcfgSucc)
{
    GlobalMockObject::verify();
    auto driverPlugin = DriverPlugin::instance();
    MOCKER_CPP(&PluginHandle::HasLoad)
        .stubs()
        .will(returnValue(false));
    MOCKER_CPP(&PluginHandle::OpenPluginFromEnv)
        .stubs()
        .will(returnValue(PROFILING_FAILED));
    MOCKER_CPP(&PluginHandle::OpenPluginFromLdcfg)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    driverPlugin->LoadDriverSo();
    driverPlugin->pluginHandle_ = nullptr;
}

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

TEST_F(DriverPluginUtest, MsprofHalHdcRecv)
{
    GlobalMockObject::verify();
    int session = 0;
    drvHdcMsg msg;
    uint64_t flag = 0;
    uint32_t timeout = 0;
    int bufLen = 0;
    int recvBufCount = 0;
    auto driverPlugin = DriverPlugin::instance();
    driverPlugin->halHdcRecv_ = nullptr;
    EXPECT_EQ(DRV_ERROR_INVALID_HANDLE, driverPlugin->MsprofHalHdcRecv((HDC_SESSION)&session, &msg,
        bufLen, flag, &recvBufCount, timeout));
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

TEST_F(DriverPluginUtest, MsprofHalHdcSessionConnectEx)
{
    GlobalMockObject::verify();
    int peerNode = 0;
    int peerDeviceId = 0;
    int peerPid = 0;
    int client = 0;
    int session = 0;
    HDC_SESSION psession = static_cast<HDC_SESSION>(&session);
    auto driverPlugin = DriverPlugin::instance();
    driverPlugin->halHdcSessionConnectEx_ = nullptr;
    EXPECT_EQ(DRV_ERROR_INVALID_HANDLE, driverPlugin->MsprofHalHdcSessionConnectEx(peerNode, peerDeviceId,
        peerPid, static_cast<HDC_CLIENT>(&client), &psession));
    driverPlugin->halHdcSessionConnectEx_ = HalHdcSessionConnectEx;
    EXPECT_EQ(DRV_ERROR_NONE, driverPlugin->MsprofHalHdcSessionConnectEx(peerNode, peerDeviceId,
        peerPid, static_cast<HDC_CLIENT>(&client), &psession));
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

TEST_F(DriverPluginUtest, MsprofHalEschedCreateGrpEx)
{
    GlobalMockObject::verify();
    uint32_t deviceId = 0;
    struct esched_grp_para grpPara = {GRP_TYPE_BIND_CP_CPU, 1, {0}, {0}};
    uint32_t grpId = 0;
    auto driverPlugin = DriverPlugin::instance();
    driverPlugin->halEschedCreateGrpEx_ = nullptr;
    EXPECT_EQ(DRV_ERROR_INVALID_HANDLE, driverPlugin->MsprofHalEschedCreateGrpEx(deviceId, &grpPara, &grpId));
    driverPlugin->halEschedCreateGrpEx_ = HalEschedCreateGrpEx;
    EXPECT_EQ(DRV_ERROR_NONE, driverPlugin->MsprofHalEschedCreateGrpEx(deviceId, &grpPara, &grpId));
}

TEST_F(DriverPluginUtest, MsprofHalEschedAttachDevice)
{
    GlobalMockObject::verify();
    uint32_t deviceId = 0;
    auto driverPlugin = DriverPlugin::instance();
    driverPlugin->halEschedAttachDevice_ = nullptr;
    EXPECT_EQ(DRV_ERROR_INVALID_HANDLE, driverPlugin->MsprofHalEschedAttachDevice(deviceId));
    driverPlugin->halEschedAttachDevice_ = HalEschedAttachDevice;
    EXPECT_EQ(DRV_ERROR_NONE, driverPlugin->MsprofHalEschedAttachDevice(deviceId));
}

TEST_F(DriverPluginUtest, MsprofHalEschedDettachDevice)
{
    GlobalMockObject::verify();
    uint32_t deviceId = 0;
    auto driverPlugin = DriverPlugin::instance();
    driverPlugin->halEschedDettachDevice_ = nullptr;
    EXPECT_EQ(DRV_ERROR_INVALID_HANDLE, driverPlugin->MsprofHalEschedDettachDevice(deviceId));
    driverPlugin->halEschedDettachDevice_ = HalEschedDettachDevice;
    EXPECT_EQ(DRV_ERROR_NONE, driverPlugin->MsprofHalEschedDettachDevice(deviceId));
}

TEST_F(DriverPluginUtest, MsprofHalEschedSubscribeEvent)
{
    GlobalMockObject::verify();
    uint32_t deviceId = 0;
    unsigned int grpId = 0;
    unsigned int threadId = 0;
    unsigned long long eventBitmap = 0;
    auto driverPlugin = DriverPlugin::instance();
    driverPlugin->halEschedSubscribeEvent_ = nullptr;
    EXPECT_EQ(DRV_ERROR_INVALID_HANDLE, driverPlugin->MsprofHalEschedSubscribeEvent(deviceId, grpId,
                                                                                    threadId, eventBitmap));
    driverPlugin->halEschedSubscribeEvent_ = HalEschedSubscribeEvent;
    EXPECT_EQ(DRV_ERROR_NONE, driverPlugin->MsprofHalEschedSubscribeEvent(deviceId, grpId,
                                                                          threadId, eventBitmap));
}

TEST_F(DriverPluginUtest, MsprofHalEschedWaitEvent)
{
    GlobalMockObject::verify();
    uint32_t deviceId = 0;
    unsigned int grpId = 0;
    unsigned int threadId = 0;
    int timeout = 10;
    struct event_info event;
    auto driverPlugin = DriverPlugin::instance();
    driverPlugin->halEschedWaitEvent_ = nullptr;
    EXPECT_EQ(DRV_ERROR_INVALID_HANDLE, driverPlugin->MsprofHalEschedWaitEvent(deviceId, grpId, threadId,
                                                                               timeout, &event));
    driverPlugin->halEschedWaitEvent_ = HalEschedWaitEvent;
    EXPECT_EQ(DRV_ERROR_NONE, driverPlugin->MsprofHalEschedWaitEvent(deviceId, grpId, threadId,
                                                                     timeout, &event));
}

TEST_F(DriverPluginUtest, MsprofDrvGetDeviceSplitMode)
{
    GlobalMockObject::verify();
    uint32_t deviceId = 0;
    unsigned int mode = 0;
    auto driverPlugin = DriverPlugin::instance();
    driverPlugin->drvGetDeviceSplitMode_ = nullptr;
    EXPECT_EQ(DRV_ERROR_INVALID_HANDLE, driverPlugin->MsprofDrvGetDeviceSplitMode(deviceId, &mode));
    driverPlugin->drvGetDeviceSplitMode_ = DrvGetDeviceSplitMode;
    EXPECT_EQ(DRV_ERROR_NONE, driverPlugin->MsprofDrvGetDeviceSplitMode(deviceId, &mode));
}

TEST_F(DriverPluginUtest, MsprofHalEschedQueryInfo)
{
    GlobalMockObject::verify();
    uint32_t devId = 0;
    uint32_t grpId = 0;
    struct esched_query_gid_output gidOut = {0};
    struct esched_query_gid_input gidIn = {0, {0}};
    struct esched_output_info outPut = {&gidOut, sizeof(struct esched_query_gid_output)};
    struct esched_input_info inPut = {&gidIn, sizeof(struct esched_query_gid_input)};
    auto driverPlugin = DriverPlugin::instance();

    driverPlugin->halEschedQueryInfo_ = nullptr;
    EXPECT_EQ(DRV_ERROR_INVALID_HANDLE, driverPlugin->MsprofHalEschedQueryInfo(devId, QUERY_TYPE_LOCAL_GRP_ID,
                                                                               &inPut, &outPut));
    driverPlugin->halEschedQueryInfo_ = HalEschedQueryInfo;
    EXPECT_EQ(DRV_ERROR_NONE, driverPlugin->MsprofHalEschedQueryInfo(devId, QUERY_TYPE_LOCAL_GRP_ID, &inPut, &outPut));
}

TEST_F(DriverPluginUtest, MsprofHalQueryDevpid)
{
    GlobalMockObject::verify();
    pid_t devPid = 0;
    int32_t hostPid = 0;
    uint32_t deviceId = 0;
    struct halQueryDevpidInfo hostpidinfo = {static_cast<pid_t>(hostPid), deviceId, 0, DEVDRV_PROCESS_CPTYPE_MAX, {0}};
    auto driverPlugin = DriverPlugin::instance();

    driverPlugin->halQueryDevpid_ = nullptr;
    EXPECT_EQ(DRV_ERROR_INVALID_HANDLE, driverPlugin->MsprofHalQueryDevpid(hostpidinfo, &devPid));
    driverPlugin->halQueryDevpid_ = HalQueryDevpid;
    EXPECT_EQ(DRV_ERROR_NONE, driverPlugin->MsprofHalQueryDevpid(hostpidinfo, &devPid));
}

TEST_F(DriverPluginUtest, MsprofDrvGetPlatformInfo)
{
    GlobalMockObject::verify();
    auto driverPlugin = DriverPlugin::instance();
    uint32_t info = 0;
    driverPlugin->drvGetPlatformInfo_ = nullptr;
    EXPECT_EQ(DRV_ERROR_INVALID_HANDLE, driverPlugin->MsprofDrvGetPlatformInfo(&info));
    driverPlugin->drvGetPlatformInfo_ = DrvGetPlatformInfo;
    EXPECT_EQ(DRV_ERROR_NONE, driverPlugin->MsprofDrvGetPlatformInfo(&info));
    EXPECT_EQ(1, info);
}

TEST_F(DriverPluginUtest, MsprofDrvHdcClientCreate)
{
    GlobalMockObject::verify();
    auto driverPlugin = DriverPlugin::instance();
    int client = 0;
    HDC_CLIENT clientPtr = static_cast<HDC_CLIENT>(&client);
    int maxSessionNum = 1;
    int serviceType = 0;
    int flag = 0;
    driverPlugin->drvHdcClientCreate_ = nullptr;
    EXPECT_EQ(DRV_ERROR_INVALID_HANDLE, driverPlugin->MsprofDrvHdcClientCreate(&clientPtr,
        maxSessionNum, serviceType, flag));
    driverPlugin->drvHdcClientCreate_ = DrvHdcClientCreate;
    EXPECT_EQ(DRV_ERROR_NONE, driverPlugin->MsprofDrvHdcClientCreate(&clientPtr,
        maxSessionNum, serviceType, flag));
}

TEST_F(DriverPluginUtest, MsprofDrvHdcClientDestroy)
{
    GlobalMockObject::verify();
    auto driverPlugin = DriverPlugin::instance();
    int client = 0;
    HDC_CLIENT clientPtr = static_cast<HDC_CLIENT>(&client);
    driverPlugin->drvHdcClientDestroy_ = nullptr;
    EXPECT_EQ(DRV_ERROR_INVALID_HANDLE, driverPlugin->MsprofDrvHdcClientDestroy(clientPtr));
    driverPlugin->drvHdcClientDestroy_ = DrvHdcClientDestroy;
    EXPECT_EQ(DRV_ERROR_NONE, driverPlugin->MsprofDrvHdcClientDestroy(clientPtr));
}

TEST_F(DriverPluginUtest, MsprofDrvHdcServerCreate)
{
    GlobalMockObject::verify();
    auto driverPlugin = DriverPlugin::instance();
    int devid = 0;
    int serviceType = 0;
    int server = 0;
    HDC_SERVER serverPtr = static_cast<HDC_SERVER>(&server);
    driverPlugin->drvHdcServerCreate_ = nullptr;
    EXPECT_EQ(DRV_ERROR_INVALID_HANDLE, driverPlugin->MsprofDrvHdcServerCreate(devid, serviceType, &serverPtr));
    driverPlugin->drvHdcServerCreate_ = DrvHdcServerCreate;
    EXPECT_EQ(DRV_ERROR_NONE, driverPlugin->MsprofDrvHdcServerCreate(devid, serviceType, &serverPtr));
}

TEST_F(DriverPluginUtest, MsprofDrvHdcServerDestroy)
{
    GlobalMockObject::verify();
    auto driverPlugin = DriverPlugin::instance();
    int server = 0;
    HDC_SERVER serverPtr = static_cast<HDC_SERVER>(&server);
    driverPlugin->drvHdcServerDestroy_ = nullptr;
    EXPECT_EQ(DRV_ERROR_INVALID_HANDLE, driverPlugin->MsprofDrvHdcServerDestroy(serverPtr));
    driverPlugin->drvHdcServerDestroy_ = DrvHdcServerDestroy;
    EXPECT_EQ(DRV_ERROR_NONE, driverPlugin->MsprofDrvHdcServerDestroy(serverPtr));
}

TEST_F(DriverPluginUtest, MsprofDrvHdcSessionAccept)
{
    GlobalMockObject::verify();
    auto driverPlugin = DriverPlugin::instance();
    int server = 0;
    HDC_SERVER serverPtr = static_cast<HDC_SERVER>(&server);
    int session = 0;
    HDC_SESSION sessionPtr = static_cast<HDC_SESSION>(&session);
    driverPlugin->drvHdcSessionAccept_ = nullptr;
    EXPECT_EQ(DRV_ERROR_INVALID_HANDLE, driverPlugin->MsprofDrvHdcSessionAccept(serverPtr, &sessionPtr));
    driverPlugin->drvHdcSessionAccept_ = DrvHdcSessionAccept;
    EXPECT_EQ(DRV_ERROR_NONE, driverPlugin->MsprofDrvHdcSessionAccept(serverPtr, &sessionPtr));
}

TEST_F(DriverPluginUtest, MsprofDrvHdcGetMsgBuffer)
{
    GlobalMockObject::verify();
    auto driverPlugin = DriverPlugin::instance();
    struct drvHdcMsg pmsg;
    int index = 0;
    char buffer;
    analysis::dvvp::common::utils::CHAR_PTR fakeBuf = &buffer;
    int bufLen = 0;
    driverPlugin->drvHdcGetMsgBuffer_ = nullptr;
    EXPECT_EQ(DRV_ERROR_INVALID_HANDLE, driverPlugin->MsprofDrvHdcGetMsgBuffer(&pmsg, index, &fakeBuf, &bufLen));
    driverPlugin->drvHdcGetMsgBuffer_ = DrvHdcGetMsgBuffer;
    EXPECT_EQ(DRV_ERROR_NONE, driverPlugin->MsprofDrvHdcGetMsgBuffer(&pmsg, index, &fakeBuf, &bufLen));
}

TEST_F(DriverPluginUtest, MsprofDrvHdcAllocMsg)
{
    GlobalMockObject::verify();
    auto driverPlugin = DriverPlugin::instance();
    int session = 0;
    HDC_SESSION sessionPtr = static_cast<HDC_SESSION>(&session);
    struct drvHdcMsg pmsg;
    struct drvHdcMsg *pmsgPtr = &pmsg;
    int count = 0;
    driverPlugin->drvHdcAllocMsg_ = nullptr;
    EXPECT_EQ(DRV_ERROR_INVALID_HANDLE, driverPlugin->MsprofDrvHdcAllocMsg(sessionPtr, &pmsgPtr, count));
    driverPlugin->drvHdcAllocMsg_ = DrvHdcAllocMsg;
    EXPECT_EQ(DRV_ERROR_NONE, driverPlugin->MsprofDrvHdcAllocMsg(sessionPtr, &pmsgPtr, count));
}

TEST_F(DriverPluginUtest, MsprofDrvHdcFreeMsg)
{
    GlobalMockObject::verify();
    auto driverPlugin = DriverPlugin::instance();
    struct drvHdcMsg pmsg;
    struct drvHdcMsg *pmsgPtr = &pmsg;
    driverPlugin->drvHdcFreeMsg_ = nullptr;
    EXPECT_EQ(DRV_ERROR_INVALID_HANDLE, driverPlugin->MsprofDrvHdcFreeMsg(pmsgPtr));
    driverPlugin->drvHdcFreeMsg_ = DrvHdcFreeMsg;
    EXPECT_EQ(DRV_ERROR_NONE, driverPlugin->MsprofDrvHdcFreeMsg(pmsgPtr));
}

TEST_F(DriverPluginUtest, MsprofDrvHdcReuseMsg)
{
    GlobalMockObject::verify();
    auto driverPlugin = DriverPlugin::instance();
    struct drvHdcMsg pmsg;
    struct drvHdcMsg *pmsgPtr = &pmsg;
    driverPlugin->drvHdcReuseMsg_ = nullptr;
    EXPECT_EQ(DRV_ERROR_INVALID_HANDLE, driverPlugin->MsprofDrvHdcReuseMsg(pmsgPtr));
    driverPlugin->drvHdcReuseMsg_ = DrvHdcReuseMsg;
    EXPECT_EQ(DRV_ERROR_NONE, driverPlugin->MsprofDrvHdcReuseMsg(pmsgPtr));
}

TEST_F(DriverPluginUtest, MsprofDrvHdcAddMsgBuffer)
{
    GlobalMockObject::verify();
    auto driverPlugin = DriverPlugin::instance();
    struct drvHdcMsg pmsg;
    struct drvHdcMsg *pmsgPtr = &pmsg;
    char pbuf;
    int len;
    driverPlugin->drvHdcAddMsgBuffer_ = nullptr;
    EXPECT_EQ(DRV_ERROR_INVALID_HANDLE, driverPlugin->MsprofDrvHdcAddMsgBuffer(pmsgPtr, &pbuf, len));
    driverPlugin->drvHdcAddMsgBuffer_ = DrvHdcAddMsgBuffer;
    EXPECT_EQ(DRV_ERROR_NONE, driverPlugin->MsprofDrvHdcAddMsgBuffer(pmsgPtr, &pbuf, len));
}

TEST_F(DriverPluginUtest, MsprofDrvHdcSessionConnect)
{
    GlobalMockObject::verify();
    auto driverPlugin = DriverPlugin::instance();
    int peerNode = 0;
    int peerDevid = 0;
    int client = 0;
    HDC_CLIENT clientPtr = static_cast<HDC_CLIENT>(&client);
    int session = 0;
    HDC_SESSION sessionPtr = static_cast<HDC_SESSION>(&session);
    driverPlugin->drvHdcSessionConnect_ = nullptr;
    EXPECT_EQ(DRV_ERROR_INVALID_HANDLE, driverPlugin->MsprofDrvHdcSessionConnect(peerNode, peerDevid,
        clientPtr, &sessionPtr));
    driverPlugin->drvHdcSessionConnect_ = DrvHdcSessionConnect;
    EXPECT_EQ(DRV_ERROR_NONE, driverPlugin->MsprofDrvHdcSessionConnect(peerNode, peerDevid,
        clientPtr, &sessionPtr));
}

TEST_F(DriverPluginUtest, MsprofDrvHdcSessionClose)
{
    GlobalMockObject::verify();
    auto driverPlugin = DriverPlugin::instance();
    int session = 0;
    HDC_SESSION sessionPtr = static_cast<HDC_SESSION>(&session);
    driverPlugin->drvHdcSessionClose_ = nullptr;
    EXPECT_EQ(DRV_ERROR_INVALID_HANDLE, driverPlugin->MsprofDrvHdcSessionClose(sessionPtr));
    driverPlugin->drvHdcSessionClose_ = DrvHdcSessionClose;
    EXPECT_EQ(DRV_ERROR_NONE, driverPlugin->MsprofDrvHdcSessionClose(sessionPtr));
}

TEST_F(DriverPluginUtest, MsprofDrvHdcGetCapacity)
{
    GlobalMockObject::verify();
    auto driverPlugin = DriverPlugin::instance();
    struct drvHdcCapacity capacity;
    driverPlugin->drvHdcGetCapacity_ = nullptr;
    EXPECT_EQ(DRV_ERROR_INVALID_HANDLE, driverPlugin->MsprofDrvHdcGetCapacity(&capacity));
    driverPlugin->drvHdcGetCapacity_ = DrvHdcGetCapacity;
    EXPECT_EQ(DRV_ERROR_NONE, driverPlugin->MsprofDrvHdcGetCapacity(&capacity));
}

TEST_F(DriverPluginUtest, MsprofHalGetDeviceInfoByBuff)
{
    GlobalMockObject::verify();
    auto driverPlugin = DriverPlugin::instance();
    uint32_t devId = 0;
    int32_t moduleType = 0;
    int32_t infoType = 0;
    int32_t size;
    VOID_PTR buf = nullptr;
    driverPlugin->halGetDeviceInfoByBuff_ = nullptr;
    EXPECT_EQ(DRV_ERROR_NOT_SUPPORT, driverPlugin->MsprofHalGetDeviceInfoByBuff(devId, moduleType,
        infoType, buf, &size));
    driverPlugin->halGetDeviceInfoByBuff_ = HalGetDeviceInfoByBuff;
    EXPECT_EQ(DRV_ERROR_NONE, driverPlugin->MsprofHalGetDeviceInfoByBuff(devId, moduleType,
        infoType, buf, &size));
}
