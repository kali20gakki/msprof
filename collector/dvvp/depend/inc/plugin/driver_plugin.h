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
#include "plugin_manager.h"

namespace Analysis {
namespace Dvvp {
namespace Plugin {

class DriverPlugin : public analysis::dvvp::common::singleton::Singleton<DriverPlugin> {
public:
    DriverPlugin()
     :soName_("libascend_hal.so"),
      pluginManager_(PluginManager(soName_))
    {}
    ~DriverPlugin();

    bool IsFuncExist(const std::string &funcName) const;

    // halHdcRecv
    using MSPROF_HALHDCRECV_T = std::function<hdcError_t(HDC_SESSION, struct drvHdcMsg *, int, UINT64, int *, UINT32)>;
    hdcError_t MsprofHalHdcRecv(HDC_SESSION session, struct drvHdcMsg *pMsg, int bufLen,
        UINT64 flag, int *recvBufCount, UINT32 timeout);

    // halHdcSend
    using MSPROF_HALHDCSEND_T = std::function<hdcError_t(HDC_SESSION, struct drvHdcMsg *, UINT64, UINT32)>;
    hdcError_t MsprofHalHdcSend(HDC_SESSION session, struct drvHdcMsg *pMsg, UINT64 flag, UINT32 timeout);

    // halHdcSessionConnectEx
    using MSPROF_HALHDCSESSIONCONNECTEX_T = std::function<hdcError_t(int, int, int, HDC_CLIENT, HDC_SESSION *)>;
    hdcError_t MsprofHalHdcSessionConnectEx(int peer_node, int peer_devid, int peer_pid, HDC_CLIENT client, HDC_SESSION *pSession);

    // drvHdcSetSessionReference
    using MSPROF_DRVHDCSETSESSIONREFERENCE_T = std::function<drvError_t(HDC_SESSION)>; 
    drvError_t MsprofDrvHdcSetSessionReference(HDC_SESSION session);

    // halHdcGetSessionAttr
    using MSPROF_HALHDCGETSESSIONATTR_T = std::function<drvError_t(HDC_SESSION, int, int *)>;
    drvError_t MsprofHalHdcGetSessionAttr(HDC_SESSION session, int attr, int *value);

    // halGetChipInfo
    using MSPROF_HALGETCHIPINFO_T = std::function<drvError_t(unsigned int, halChipInfo *)>;
    drvError_t MsprofHalGetChipInfo(unsigned int devId, halChipInfo *chipInfo);

    // halGetDeviceInfo
    using MSPROF_HALGETDEVICEINFO_T = std::function<drvError_t(uint32_t, int32_t, int32_t, int64_t *)>;
    drvError_t MsprofHalGetDeviceInfo(uint32_t devId, int32_t moduleType, int32_t infoType, int64_t *value);

    // halProfDataFlush
    using MSPROF_HALPROFDATAFLUSH_T = std::function<int(unsigned int, unsigned int, unsigned int *)>;
    int MsprofHalProfDataFlush(unsigned int device_id, unsigned int channel_id, unsigned int *data_len);

    // prof_drv_get_channels
    using MSPROF_PROFDRVGETCHANNELS_T = std::function<int(unsigned int, channel_list_t *)>;
    int MsprofDrvGetChannels(unsigned int device_id, channel_list_t *channels);

    // prof_drv_start
    using MSPROF_PROFDRVSRART_T = std::function<int(unsigned int, unsigned int, struct prof_start_para *)>; 
    int MsprofDrvStart(unsigned int device_id, unsigned int channel_id, struct prof_start_para *start_para);

    // prof_stop
    using MSPROF_PROFSTOP_T = std::function<int(unsigned int, unsigned int)>;
    int MsprofDrvStop(unsigned int device_id, unsigned int channel_id);

    // prof_channel_read
    using MSPROF_CHANNELREAD_T = std::function<int(unsigned int, unsigned int, char *, unsigned int)>;
    int MsprofChannelRead(unsigned int device_id, unsigned int channel_id, char *out_buf, unsigned int buf_size);

    // prof_channel_poll
    using MSPROF_CHANNELPOLL_T = std::function<int(struct prof_poll_info *, int, int)>; 
    int MsprofChannelPoll(struct prof_poll_info *out_buf, int num, int timeout);
    
    // drvGetDevNum
    using MSPROF_DRVGETDEVNUM_T = std::function<drvError_t(uint32_t *)>;
    drvError_t MsprofDrvGetDevNum(uint32_t *num_dev);
    
    // drvGetDevIDByLocalDevID
    using MSPROF_DRVGETDEVIDBYLOCALID_T = std::function<drvError_t(uint32_t, uint32_t *)>;
    drvError_t MsprofDrvGetDevIDByLocalDevID(uint32_t localDevId, uint32_t *devId);
    
    // drvGetDevIDs
    using MSPROF_DRVGETDEVIDS_T = std::function<drvError_t(uint32_t *, uint32_t)>;
    drvError_t MsprofDrvGetDevIDs(uint32_t *devices, uint32_t len);
    
    // drvGetPlatformInfo
    using MSPROF_DRVGETPLATFORMINFO_T = std::function<drvError_t(uint32_t *)>;
    drvError_t MsprofDrvGetPlatformInfo(uint32_t *info);
    
    // drvHdcClientCreate
    using MSPROF_DRVHDCCLIENTCREATE_T = std::function<drvError_t(HDC_CLIENT *, int, int, int)>;
    drvError_t MsprofDrvHdcClientCreate(HDC_CLIENT *client, int maxSessionNum, int serviceType, int flag);
    
    // drvHdcClientDestroy
    using MSPROF_DRVHDCCLIENTDESTROY_T = std::function<drvError_t(HDC_CLIENT)>;
    drvError_t MsprofDrvHdcClientDestroy(HDC_CLIENT client);
    
    // drvHdcServerCreate
    using MSPROF_DRVHDCSERVERCREATE_T = std::function<drvError_t(int, int, HDC_SERVER *)>;
    drvError_t MsprofDrvHdcServerCreate(int devid, int serviceType, HDC_SERVER *pServer);
    
    // drvHdcServerDestroy
    using MSPROF_DRVHDCSERVERDESTROY_T = std::function<drvError_t(HDC_SERVER)>;
    drvError_t MsprofDrvHdcServerDestroy(HDC_SERVER server);
    
    // drvHdcSessionAccept
    using MSPROF_DRVHDCSESSIONACCEPT_T = std::function<drvError_t(HDC_SERVER, HDC_SESSION *)>;
    drvError_t MsprofDrvHdcSessionAccept(HDC_SERVER server, HDC_SESSION *session);
    
    // drvHdcGetMsgBuffer
    using MSPROF_DRVHDCGETMSGBUFFER_T = std::function<drvError_t(struct drvHdcMsg *, int, char **, int *)>;
    drvError_t MsprofDrvHdcGetMsgBuffer(struct drvHdcMsg *msg, int index, char **pBuf, int *pLen);
    
    // drvHdcAllocMsg
    using MSPROF_DRVHDCALLOCMSG_T = std::function<drvError_t(HDC_SESSION, struct drvHdcMsg **, int)>;
    drvError_t MsprofDrvHdcAllocMsg(HDC_SESSION session, struct drvHdcMsg **ppMsg, int count);
    
    // drvHdcFreeMsg
    using MSPROF_DRVHDCFREEMSG_T = std::function<drvError_t(struct drvHdcMsg *)>;
    drvError_t MsprofDrvHdcFreeMsg(struct drvHdcMsg *msg);
    
    // drvHdcReuseMsg
    using MSPROF_DRVHDCREUSEMSG_T = std::function<drvError_t(struct drvHdcMsg *)>;
    drvError_t MsprofDrvHdcReuseMsg(struct drvHdcMsg *msg);
    
    // drvHdcAddMsgBuffer
    using MSPROF_DRVHDCADDMSGBUFFER_T = std::function<drvError_t(struct drvHdcMsg *, char *, int)>;
    drvError_t MsprofDrvHdcAddMsgBuffer(struct drvHdcMsg *msg, char *pBuf, int len);
    
    // drvHdcSessionConnect
    using MSPROF_DRVHDCSESSIONCONNECT_T = std::function<drvError_t(int, int, HDC_CLIENT, HDC_SESSION *)>;
    drvError_t MsprofDrvHdcSessionConnect(int peer_node, int peer_devid, HDC_CLIENT client, HDC_SESSION *session);
    
    // drvHdcSessionClose
    using MSPROF_DRVHDCSESSIONCLOSE_T = std::function<drvError_t(HDC_SESSION)>;
    drvError_t MsprofDrvHdcSessionClose(HDC_SESSION session);
    
    // drvHdcGetCapacity
    using MSPROF_DRVHDCGETCAPACITY_T = std::function<drvError_t(struct drvHdcCapacity *)>;
    drvError_t MsprofDrvHdcGetCapacity(struct drvHdcCapacity *capacity);

private:
    std::string soName_;
    PluginManager pluginManager_;
};

} // Plugin
} // Dvvp
} // Analysis

#endif