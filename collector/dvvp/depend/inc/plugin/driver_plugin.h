/* -------------------------------------------------------------------------
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/
#ifndef DRIVER_PLUGIN_H
#define DRIVER_PLUGIN_H

#include "singleton/singleton.h"
#include "driver/ascend_hal.h"
#include "plugin_handle.h"
#include "utils.h"

namespace Collector {
namespace Dvvp {
namespace Plugin {
using HalHdcRecvFunc = std::function<hdcError_t(HDC_SESSION, struct drvHdcMsg *, int, uint64_t, int *, uint32_t)>;
using HalHdcSendFunc = std::function<hdcError_t(HDC_SESSION, struct drvHdcMsg *, uint64_t, uint32_t)>;
using HalHdcSessionConnectExFunc = std::function<hdcError_t(int, int, int, HDC_CLIENT, HDC_SESSION *)>;
using DrvHdcSetSessionReferenceFunc = std::function<drvError_t(HDC_SESSION)>;
using HalHdcGetSessionAttrFunc = std::function<drvError_t(HDC_SESSION, int, int *)>;
using HalGetChipInfoFunc = std::function<drvError_t(unsigned int, halChipInfo *)>;
using HalGetDeviceInfoFunc = std::function<drvError_t(uint32_t, int32_t, int32_t, int64_t *)>;
using HalGetApiVersionFunc = std::function<drvError_t(int32_t *)>;
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
using DrvDeviceGetPhyIdByIndexFunc = std::function<drvError_t(uint32_t, uint32_t *)>;
using HalEschedCreateGrpExFunc = std::function<drvError_t(uint32_t, struct esched_grp_para *, unsigned int *)>;
using HalEschedAttachDeviceFunc = std::function<drvError_t(unsigned int)>;
using HalEschedDettachDeviceFunc = std::function<drvError_t(unsigned int)>;
using HalEschedSubscribeEventFunc =
    std::function<drvError_t(unsigned int, unsigned int, unsigned int, unsigned long long)>;
using HalEschedWaitEventFunc =
    std::function<drvError_t(unsigned int, unsigned int, unsigned int, int, struct event_info *)>;
using DrvGetDeviceSplitModeFunc = std::function<drvError_t(unsigned int, unsigned int*)>;
using HalGetDeviceInfoByBuffFunc = std::function<drvError_t(uint32_t, int32_t, int32_t, void*, int32_t*)>;
using HalEschedQueryInfoFunc =
    std::function<drvError_t(unsigned int, ESCHED_QUERY_TYPE, struct esched_input_info *, struct esched_output_info *)>;
using HalQueryDevpidFunc = std::function<drvError_t(struct halQueryDevpidInfo, pid_t *)>;

class DriverPlugin : public analysis::dvvp::common::singleton::Singleton<DriverPlugin> {
public:
    DriverPlugin() : soName_("libascend_hal.so"), loadFlag_(0) {}

    bool IsFuncExist(const std::string &funcName) const;

    // halHdcRecv
    hdcError_t MsprofHalHdcRecv(HDC_SESSION session, struct drvHdcMsg *pMsg, int bufLen,
        uint64_t flag, int *recvBufCount, uint32_t timeout);

    // halHdcSend
    hdcError_t MsprofHalHdcSend(HDC_SESSION session, struct drvHdcMsg *pMsg, uint64_t flag, uint32_t timeout);

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

    drvError_t MsprofHalGetApiVersion(int32_t *value);

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

    // drvDeviceGetPhyIdByIndex
    drvError_t MsprofDrvDeviceGetPhyIdByIndex(uint32_t devIndex, uint32_t *phyId);

    // halEschedCreateGrpEx
    drvError_t MsprofHalEschedCreateGrpEx(uint32_t devId, struct esched_grp_para *grpPara, unsigned int *grpId);

    // halEschedAttachDevice
    drvError_t MsprofHalEschedAttachDevice(unsigned int devId);

    // halEschedDettachDevice
    drvError_t MsprofHalEschedDettachDevice(unsigned int devId);

    // halEschedSubscribeEvent
    drvError_t MsprofHalEschedSubscribeEvent(unsigned int devId, unsigned int grpId,
                                             unsigned int threadId, unsigned long long eventBitmap);

    // halEschedWaitEvent
    drvError_t MsprofHalEschedWaitEvent(unsigned int devId, unsigned int grpId,
                                        unsigned int threadId, int timeout, struct event_info *event);

    // drvGetDeviceSplitMode
    drvError_t MsprofDrvGetDeviceSplitMode(unsigned int devId, unsigned int* mode);

    // halGetDeviceInfoByBuff
    drvError_t MsprofHalGetDeviceInfoByBuff(uint32_t devId, int32_t moduleType,
                                            int32_t infoType, void *buf, int32_t *size);
    
    // halEschedQueryInfo
    drvError_t MsprofHalEschedQueryInfo(unsigned int devId, ESCHED_QUERY_TYPE type,
                                        struct esched_input_info *inPut, struct esched_output_info *outPut);
    
    // halQueryDevpid
    drvError_t MsprofHalQueryDevpid(struct halQueryDevpidInfo pidInfo, pid_t *pid);

    // get all function addresses at a time
    void GetAllFunction();

private:
    std::string soName_;
    static SHARED_PTR_ALIA<PluginHandle> pluginHandle_;
    PTHREAD_ONCE_T loadFlag_;
    HalHdcRecvFunc halHdcRecv_ = nullptr;
    HalHdcSendFunc halHdcSend_ = nullptr;
    HalHdcSessionConnectExFunc halHdcSessionConnectEx_ = nullptr;
    DrvHdcSetSessionReferenceFunc drvHdcSetSessionReference_ = nullptr;
    HalHdcGetSessionAttrFunc halHdcGetSessionAttr_ = nullptr;
    HalGetChipInfoFunc halGetChipInfo_ = nullptr;
    HalGetDeviceInfoFunc halGetDeviceInfo_ = nullptr;
    HalGetApiVersionFunc halGetApiVersion_ = nullptr;
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
    DrvDeviceGetPhyIdByIndexFunc drvDeviceGetPhyIdByIndex_ = nullptr;
    HalEschedCreateGrpExFunc halEschedCreateGrpEx_ = nullptr;
    HalEschedAttachDeviceFunc halEschedAttachDevice_ = nullptr;
    HalEschedDettachDeviceFunc halEschedDettachDevice_ = nullptr;
    HalEschedSubscribeEventFunc halEschedSubscribeEvent_ = nullptr;
    HalEschedWaitEventFunc halEschedWaitEvent_ = nullptr;
    DrvGetDeviceSplitModeFunc drvGetDeviceSplitMode_ = nullptr;
    HalGetDeviceInfoByBuffFunc halGetDeviceInfoByBuff_ = nullptr;
    HalEschedQueryInfoFunc halEschedQueryInfo_ = nullptr;
    HalQueryDevpidFunc halQueryDevpid_ = nullptr;

private:
    void LoadDriverSo();
    void GetHalFunction();
};

} // Plugin
} // Dvvp
} // Collector

#endif