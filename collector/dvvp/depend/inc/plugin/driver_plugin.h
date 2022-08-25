/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: driver interface
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2022-04-15
 */
#ifndef DRIVER_PLUGIN_H
#define DRIVER_PLUGIN_H

#include "singleton/singleton.h"
#include "driver/ascend_hal.h"
#include "plugin_handle.h"

namespace Collector {
namespace Dvvp {
namespace Plugin {
using HalHdcRecvFunc = std::function<hdcError_t(HDC_SESSION, struct drvHdcMsg *, int, UINT64, int *, UINT32)>;
using HalHdcSendFunc = std::function<hdcError_t(HDC_SESSION, struct drvHdcMsg *, UINT64, UINT32)>;
using HalHdcSessionConnectExFunc = std::function<hdcError_t(int, int, int, HDC_CLIENT, HDC_SESSION *)>;
using DrvHdcSetSessionReferenceFunc = std::function<drvError_t(HDC_SESSION)>;
using HalHdcGetSessionAttrFunc = std::function<drvError_t(HDC_SESSION, int, int *)>;
using HalGetChipInfoFunc = std::function<drvError_t(unsigned int, halChipInfo *)>;
using HalGetDeviceInfoFunc = std::function<drvError_t(uint32_t, int32_t, int32_t, int64_t *)>;
using HalProfDataFlushFunc = std::function<int(unsigned int, unsigned int, unsigned int *)>;
using ProfDrvGetChannelsFunc = std::function<int(unsigned int, channel_list_t *)>;
using ProfDrvStartFunc = std::function<int(unsigned int, unsigned int, struct prof_start_para *)>;
using ProfStopFunc = std::function<int(unsigned int, unsigned int)>;
using ProfChannelReadFunc = std::function<int(unsigned int, unsigned int, char *, unsigned int)>;
using ProfChannelPollFunc = std::function<int(struct prof_poll_info *, int, int)>;
using DrvGetDevNumFunc = std::function<drvError_t(uint32_t *)>;
using DrvGetDevIDByLocalDevIDFunc = std::function<drvError_t(uint32_t, uint32_t *)>;
using DrvGetDevIDsFunc = std::function<drvError_t(uint32_t *, uint32_t)>;
using DrvGetPlatformInfoFunc = std::function<drvError_t(uint32_t *)>;
using DrvHdcClientCreateFunc = std::function<drvError_t(HDC_CLIENT *, int, int, int)>;
using DrvHdcClientDestroyFunc = std::function<drvError_t(HDC_CLIENT)>;
using DrvHdcServerCreateFunc = std::function<drvError_t(int, int, HDC_SERVER *)>;
using DrvHdcServerDestroyFunc = std::function<drvError_t(HDC_SERVER)>;
using DrvHdcSessionAcceptFunc = std::function<drvError_t(HDC_SERVER, HDC_SESSION *)>;
using DrvHdcGetMsgBufferFunc = std::function<drvError_t(struct drvHdcMsg *, int, char **, int *)>;
using DrvHdcAllocMsgFunc = std::function<drvError_t(HDC_SESSION, struct drvHdcMsg **, int)>;
using DrvHdcFreeMsgFunc = std::function<drvError_t(struct drvHdcMsg *)>;
using DrvHdcReuseMsgFunc = std::function<drvError_t(struct drvHdcMsg *)>;
using DrvHdcAddMsgBufferFunc = std::function<drvError_t(struct drvHdcMsg *, char *, int)>;
using DrvHdcSessionConnectFunc = std::function<drvError_t(int, int, HDC_CLIENT, HDC_SESSION *)>;
using DrvHdcSessionCloseFunc = std::function<drvError_t(HDC_SESSION)>;
using DrvHdcGetCapacityFunc = std::function<drvError_t(struct drvHdcCapacity *)>;
class DriverPlugin : public analysis::dvvp::common::singleton::Singleton<DriverPlugin> {
public:
    DriverPlugin() : soName_("libascend_hal.so"), pluginHandle_(PluginHandle(soName_)), loadFlag_(0) {}

    bool IsFuncExist(const std::string &funcName) const;

    // halHdcRecv
    hdcError_t MsprofHalHdcRecv(HDC_SESSION session, struct drvHdcMsg *pMsg, int bufLen,
        UINT64 flag, int *recvBufCount, UINT32 timeout);

    // halHdcSend
    hdcError_t MsprofHalHdcSend(HDC_SESSION session, struct drvHdcMsg *pMsg, UINT64 flag, UINT32 timeout);

    // halHdcSessionConnectEx
    hdcError_t MsprofHalHdcSessionConnectEx(int peer_node, int peer_devid, int peer_pid,
        HDC_CLIENT client, HDC_SESSION *pSession);

    // drvHdcSetSessionReference
    drvError_t MsprofDrvHdcSetSessionReference(HDC_SESSION session);

    // halHdcGetSessionAttr
    drvError_t MsprofHalHdcGetSessionAttr(HDC_SESSION session, int attr, int *value);

    // halGetChipInfo
    drvError_t MsprofHalGetChipInfo(unsigned int devId, halChipInfo *chipInfo);

    // halGetDeviceInfo
    drvError_t MsprofHalGetDeviceInfo(uint32_t devId, int32_t moduleType, int32_t infoType, int64_t *value);

    // halProfDataFlush
    int MsprofHalProfDataFlush(unsigned int device_id, unsigned int channel_id, unsigned int *data_len);

    // prof_drv_get_channels
    int MsprofDrvGetChannels(unsigned int device_id, channel_list_t *channels);

    // prof_drv_start
    int MsprofDrvStart(unsigned int device_id, unsigned int channel_id, struct prof_start_para *start_para);

    // prof_stop
    int MsprofDrvStop(unsigned int device_id, unsigned int channel_id);

    // prof_channel_read
    int MsprofChannelRead(unsigned int device_id, unsigned int channel_id, char *out_buf, unsigned int buf_size);

    // prof_channel_poll
    int MsprofChannelPoll(struct prof_poll_info *out_buf, int num, int timeout);
    
    // drvGetDevNum
    drvError_t MsprofDrvGetDevNum(uint32_t *num_dev);
    
    // drvGetDevIDByLocalDevID
    drvError_t MsprofDrvGetDevIDByLocalDevID(uint32_t localDevId, uint32_t *devId);
    
    // drvGetDevIDs
    drvError_t MsprofDrvGetDevIDs(uint32_t *devices, uint32_t len);
    
    // drvGetPlatformInfo
    drvError_t MsprofDrvGetPlatformInfo(uint32_t *info);
    
    // drvHdcClientCreate
    drvError_t MsprofDrvHdcClientCreate(HDC_CLIENT *client, int maxSessionNum, int serviceType, int flag);
    
    // drvHdcClientDestroy
    drvError_t MsprofDrvHdcClientDestroy(HDC_CLIENT client);
    
    // drvHdcServerCreate
    drvError_t MsprofDrvHdcServerCreate(int devid, int serviceType, HDC_SERVER *pServer);
    
    // drvHdcServerDestroy
    drvError_t MsprofDrvHdcServerDestroy(HDC_SERVER server);
    
    // drvHdcSessionAccept
    drvError_t MsprofDrvHdcSessionAccept(HDC_SERVER server, HDC_SESSION *session);
    
    // drvHdcGetMsgBuffer
    drvError_t MsprofDrvHdcGetMsgBuffer(struct drvHdcMsg *msg, int index, char **pBuf, int *pLen);
    
    // drvHdcAllocMsg
    drvError_t MsprofDrvHdcAllocMsg(HDC_SESSION session, struct drvHdcMsg **ppMsg, int count);
    
    // drvHdcFreeMsg
    drvError_t MsprofDrvHdcFreeMsg(struct drvHdcMsg *msg);
    
    // drvHdcReuseMsg
    drvError_t MsprofDrvHdcReuseMsg(struct drvHdcMsg *msg);
    
    // drvHdcAddMsgBuffer
    drvError_t MsprofDrvHdcAddMsgBuffer(struct drvHdcMsg *msg, char *pBuf, int len);
    
    // drvHdcSessionConnect
    drvError_t MsprofDrvHdcSessionConnect(int peer_node, int peer_devid, HDC_CLIENT client, HDC_SESSION *session);
    
    // drvHdcSessionClose
    drvError_t MsprofDrvHdcSessionClose(HDC_SESSION session);
    
    // drvHdcGetCapacity
    drvError_t MsprofDrvHdcGetCapacity(struct drvHdcCapacity *capacity);

private:
    std::string soName_;
    PluginHandle pluginHandle_;
    PTHREAD_ONCE_T loadFlag_;
    HalHdcRecvFunc halHdcRecv_ = nullptr;
    HalHdcSendFunc halHdcSend_ = nullptr;
    HalHdcSessionConnectExFunc halHdcSessionConnectEx_ = nullptr;
    DrvHdcSetSessionReferenceFunc drvHdcSetSessionReference_ = nullptr;
    HalHdcGetSessionAttrFunc halHdcGetSessionAttr_ = nullptr;
    HalGetChipInfoFunc halGetChipInfo_ = nullptr;
    HalGetDeviceInfoFunc halGetDeviceInfo_ = nullptr;
    HalProfDataFlushFunc halProfDataFlush_ = nullptr;
    ProfDrvGetChannelsFunc profDrvGetChannels_ = nullptr;
    ProfDrvStartFunc profDrvStart_ = nullptr;
    ProfStopFunc profStop_ = nullptr;
    ProfChannelReadFunc profChannelRead_ = nullptr;
    ProfChannelPollFunc profChannelPoll_ = nullptr;
    DrvGetDevNumFunc drvGetDevNum_ = nullptr;
    DrvGetDevIDByLocalDevIDFunc drvGetDevIDByLocalDevID_ = nullptr;
    DrvGetDevIDsFunc drvGetDevIDs_ = nullptr;
    DrvGetPlatformInfoFunc drvGetPlatformInfo_ = nullptr;
    DrvHdcClientCreateFunc drvHdcClientCreate_ = nullptr;
    DrvHdcClientDestroyFunc drvHdcClientDestroy_ = nullptr;
    DrvHdcServerCreateFunc drvHdcServerCreate_ = nullptr;
    DrvHdcServerDestroyFunc drvHdcServerDestroy_ = nullptr;
    DrvHdcSessionAcceptFunc drvHdcSessionAccept_ = nullptr;
    DrvHdcGetMsgBufferFunc drvHdcGetMsgBuffer_ = nullptr;
    DrvHdcAllocMsgFunc drvHdcAllocMsg_ = nullptr;
    DrvHdcFreeMsgFunc drvHdcFreeMsg_ = nullptr;
    DrvHdcReuseMsgFunc drvHdcReuseMsg_ = nullptr;
    DrvHdcAddMsgBufferFunc drvHdcAddMsgBuffer_ = nullptr;
    DrvHdcSessionConnectFunc drvHdcSessionConnect_ = nullptr;
    DrvHdcSessionCloseFunc drvHdcSessionClose_ = nullptr;
    DrvHdcGetCapacityFunc drvHdcGetCapacity_ = nullptr;
 
private:
    void LoadDriverSo();
};

} // Plugin
} // Dvvp
} // Collector

#endif