/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: driver interface
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2022-04-15
 */
#include "driver_plugin.h"

namespace Collector {
namespace Dvvp {
namespace Plugin {
void DriverPlugin::LoadDriverSo()
{
    int32_t ret = PROFILING_SUCCESS;
    if (!pluginHandle_.HasLoad()) {
        ret = pluginHandle_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PROFILING_SUCCESS) {
            return;
        }
    }
}

bool DriverPlugin::IsFuncExist(const std::string &funcName) const
{
    return pluginHandle_.IsFuncExist(funcName);
}

// halHdcRecv
hdcError_t DriverPlugin::MsprofHalHdcRecv(HDC_SESSION session, struct drvHdcMsg *pMsg, int bufLen,
    uint64_t flag, int *recvBufCount, uint32_t timeout)
{
    PthreadOnce(&loadFlag_, []()->void {DriverPlugin::instance()->LoadDriverSo();});
    if (halHdcRecv_ == nullptr) {
        int32_t ret = pluginHandle_.GetFunction<hdcError_t, HDC_SESSION, struct drvHdcMsg *, int, uint64_t,
                                                     int *, uint32_t>("halHdcRecv", halHdcRecv_);
        if (ret != PROFILING_SUCCESS) {
            return DRV_ERROR_INVALID_HANDLE;
        }
    }
    return halHdcRecv_(session, pMsg, bufLen, flag, recvBufCount, timeout);
}

// halHdcSend
hdcError_t DriverPlugin::MsprofHalHdcSend(HDC_SESSION session, struct drvHdcMsg *pMsg, uint64_t flag, uint32_t timeout)
{
    PthreadOnce(&loadFlag_, []()->void {DriverPlugin::instance()->LoadDriverSo();});
    if (halHdcSend_ == nullptr) {
        int32_t ret = pluginHandle_.GetFunction<hdcError_t, HDC_SESSION, struct drvHdcMsg *,
            uint64_t, uint32_t>("halHdcSend", halHdcSend_);
        if (ret != PROFILING_SUCCESS) {
            return DRV_ERROR_INVALID_HANDLE;
        }
    }
    return halHdcSend_(session, pMsg, flag, timeout);
}

// halHdcSessionConnectEx
hdcError_t DriverPlugin::MsprofHalHdcSessionConnectEx(int peer_node, int peer_devid, int peer_pid, HDC_CLIENT client,
    HDC_SESSION *pSession)
{
    PthreadOnce(&loadFlag_, []()->void {DriverPlugin::instance()->LoadDriverSo();});
    if (halHdcSessionConnectEx_ == nullptr) {
        int32_t ret = pluginHandle_.GetFunction<hdcError_t, int, int, int, HDC_CLIENT,
                              HDC_SESSION *>("halHdcSessionConnectEx", halHdcSessionConnectEx_);
        if (ret != PROFILING_SUCCESS) {
            return DRV_ERROR_INVALID_HANDLE;
        }
    }
    return halHdcSessionConnectEx_(peer_node, peer_devid, peer_pid, client, pSession);
}

// drvHdcSetSessionReference
drvError_t DriverPlugin::MsprofDrvHdcSetSessionReference(HDC_SESSION session)
{
    PthreadOnce(&loadFlag_, []()->void {DriverPlugin::instance()->LoadDriverSo();});
    if (drvHdcSetSessionReference_ == nullptr) {
        int32_t ret = pluginHandle_.GetFunction<drvError_t, HDC_SESSION>("drvHdcSetSessionReference",
            drvHdcSetSessionReference_);
        if (ret != PROFILING_SUCCESS) {
            return DRV_ERROR_INVALID_HANDLE;
        }
    }
    return drvHdcSetSessionReference_(session);
}

// halHdcGetSessionAttr
drvError_t DriverPlugin::MsprofHalHdcGetSessionAttr(HDC_SESSION session, int attr, int *value)
{
    PthreadOnce(&loadFlag_, []()->void {DriverPlugin::instance()->LoadDriverSo();});
    if (halHdcGetSessionAttr_ == nullptr) {
        int32_t ret = pluginHandle_.GetFunction<drvError_t, HDC_SESSION, int, int *>("halHdcGetSessionAttr",
            halHdcGetSessionAttr_);
        if (ret != PROFILING_SUCCESS) {
            return DRV_ERROR_INVALID_HANDLE;
        }
    }
    return halHdcGetSessionAttr_(session, attr, value);
}

// halGetChipInfo
drvError_t DriverPlugin::MsprofHalGetChipInfo(unsigned int devId, halChipInfo *chipInfo)
{
    PthreadOnce(&loadFlag_, []()->void {DriverPlugin::instance()->LoadDriverSo();});
    if (halGetChipInfo_ == nullptr) {
        int32_t ret = pluginHandle_.GetFunction<drvError_t, unsigned int, halChipInfo *>("halGetChipInfo",
            halGetChipInfo_);
        if (ret != PROFILING_SUCCESS) {
            return DRV_ERROR_INVALID_HANDLE;
        }
    }
    return halGetChipInfo_(devId, chipInfo);
}

// halGetDeviceInfo
drvError_t DriverPlugin::MsprofHalGetDeviceInfo(uint32_t devId, int32_t moduleType, int32_t infoType, int64_t *value)
{
    PthreadOnce(&loadFlag_, []()->void {DriverPlugin::instance()->LoadDriverSo();});
    if (halGetDeviceInfo_ == nullptr) {
        int32_t ret = pluginHandle_.GetFunction<drvError_t, uint32_t, int32_t, int32_t,
            int64_t *>("halGetDeviceInfo", halGetDeviceInfo_);
        if (ret != PROFILING_SUCCESS) {
            return DRV_ERROR_INVALID_HANDLE;
        }
    }
    return halGetDeviceInfo_(devId, moduleType, infoType, value);
}

// halProfDataFlush
int DriverPlugin::MsprofHalProfDataFlush(unsigned int device_id, unsigned int channel_id, unsigned int *data_len)
{
    PthreadOnce(&loadFlag_, []()->void {DriverPlugin::instance()->LoadDriverSo();});
    if (halProfDataFlush_ == nullptr) {
        int32_t ret = pluginHandle_.GetFunction<int, unsigned int, unsigned int,
            unsigned int *>("halProfDataFlush", halProfDataFlush_);
        if (ret != PROFILING_SUCCESS) {
            return -1;
        }
    }
    return halProfDataFlush_(device_id, channel_id, data_len);
}

// prof_drv_get_channels
int DriverPlugin::MsprofDrvGetChannels(unsigned int device_id, channel_list_t *channels)
{
    PthreadOnce(&loadFlag_, []()->void {DriverPlugin::instance()->LoadDriverSo();});
    if (profDrvGetChannels_ == nullptr) {
        int32_t ret = pluginHandle_.GetFunction<int, unsigned int, channel_list_t *>("prof_drv_get_channels",
            profDrvGetChannels_);
        if (ret != PROFILING_SUCCESS) {
            return -1;
        }
    }
    return profDrvGetChannels_(device_id, channels);
}

// prof_drv_start
int DriverPlugin::MsprofDrvStart(unsigned int device_id, unsigned int channel_id, struct prof_start_para *start_para)
{
    PthreadOnce(&loadFlag_, []()->void {DriverPlugin::instance()->LoadDriverSo();});
    if (profDrvStart_ == nullptr) {
        int32_t ret = pluginHandle_.GetFunction<int, unsigned int, unsigned int,
                              struct prof_start_para *>("prof_drv_start", profDrvStart_);
        if (ret != PROFILING_SUCCESS) {
            return -1;
        }
    }
    return profDrvStart_(device_id, channel_id, start_para);
}

// prof_stop
int DriverPlugin::MsprofDrvStop(unsigned int device_id, unsigned int channel_id)
{
    PthreadOnce(&loadFlag_, []()->void {DriverPlugin::instance()->LoadDriverSo();});
    if (profStop_ == nullptr) {
        int32_t ret = pluginHandle_.GetFunction<int, unsigned int, unsigned int>("prof_stop", profStop_);
        if (ret != PROFILING_SUCCESS) {
            return -1;
        }
    }
    return profStop_(device_id, channel_id);
}

// prof_channel_read
int DriverPlugin::MsprofChannelRead(unsigned int device_id, unsigned int channel_id,
                                    char *out_buf, unsigned int buf_size)
{
    PthreadOnce(&loadFlag_, []()->void {DriverPlugin::instance()->LoadDriverSo();});
    if (profChannelRead_ == nullptr) {
        int32_t ret = pluginHandle_.GetFunction<int, unsigned int, unsigned int,
            char *, unsigned int>("prof_channel_read", profChannelRead_);
        if (ret != PROFILING_SUCCESS) {
            return -1;
        }
    }
    return profChannelRead_(device_id, channel_id, out_buf, buf_size);
}

// prof_channel_poll
int DriverPlugin::MsprofChannelPoll(struct prof_poll_info *out_buf, int num, int timeout)
{
    PthreadOnce(&loadFlag_, []()->void {DriverPlugin::instance()->LoadDriverSo();});
    if (profChannelPoll_ == nullptr) {
        int32_t ret = pluginHandle_.GetFunction<int, struct prof_poll_info *, int, int>("prof_channel_poll",
            profChannelPoll_);
        if (ret != PROFILING_SUCCESS) {
            return -1;
        }
    }
    return profChannelPoll_(out_buf, num, timeout);
}

// drvGetDevNum
drvError_t DriverPlugin::MsprofDrvGetDevNum(uint32_t *num_dev)
{
    PthreadOnce(&loadFlag_, []()->void {DriverPlugin::instance()->LoadDriverSo();});
    if (drvGetDevNum_ == nullptr) {
        int32_t ret = pluginHandle_.GetFunction<drvError_t, uint32_t *>("drvGetDevNum", drvGetDevNum_);
        if (ret != PROFILING_SUCCESS) {
            return DRV_ERROR_INVALID_HANDLE;
        }
    }
    return drvGetDevNum_(num_dev);
}

// drvGetDevIDByLocalDevID
drvError_t DriverPlugin::MsprofDrvGetDevIDByLocalDevID(uint32_t localDevId, uint32_t *devId)
{
    PthreadOnce(&loadFlag_, []()->void {DriverPlugin::instance()->LoadDriverSo();});
    if (drvGetDevIDByLocalDevID_ == nullptr) {
        int32_t ret = pluginHandle_.GetFunction<drvError_t, uint32_t, uint32_t*>("drvGetDevIDByLocalDevID",
            drvGetDevIDByLocalDevID_);
        if (ret != PROFILING_SUCCESS) {
            return DRV_ERROR_INVALID_HANDLE;
        }
    }
    return drvGetDevIDByLocalDevID_(localDevId, devId);
}

// drvGetDevIDs
drvError_t DriverPlugin::MsprofDrvGetDevIDs(uint32_t *devices, uint32_t len)
{
    PthreadOnce(&loadFlag_, []()->void {DriverPlugin::instance()->LoadDriverSo();});
    if (drvGetDevIDs_ == nullptr) {
        int32_t ret = pluginHandle_.GetFunction<drvError_t, uint32_t *, uint32_t>("drvGetDevIDs", drvGetDevIDs_);
        if (ret != PROFILING_SUCCESS) {
            return DRV_ERROR_INVALID_HANDLE;
        }
    }
    return drvGetDevIDs_(devices, len);
}

// drvGetPlatformInfo
drvError_t DriverPlugin::MsprofDrvGetPlatformInfo(uint32_t *info)
{
    PthreadOnce(&loadFlag_, []()->void {DriverPlugin::instance()->LoadDriverSo();});
    if (drvGetPlatformInfo_ == nullptr) {
        int32_t ret = pluginHandle_.GetFunction<drvError_t, uint32_t *>("drvGetPlatformInfo",
            drvGetPlatformInfo_);
        if (ret != PROFILING_SUCCESS) {
            return DRV_ERROR_INVALID_HANDLE;
        }
    }
    return drvGetPlatformInfo_(info);
}

// drvHdcClientCreate
drvError_t DriverPlugin::MsprofDrvHdcClientCreate(HDC_CLIENT *client, int maxSessionNum, int serviceType, int flag)
{
    PthreadOnce(&loadFlag_, []()->void {DriverPlugin::instance()->LoadDriverSo();});
    if (drvHdcClientCreate_ == nullptr) {
        int32_t ret = pluginHandle_.GetFunction<drvError_t, HDC_CLIENT *, int, int, int>("drvHdcClientCreate",
            drvHdcClientCreate_);
        if (ret != PROFILING_SUCCESS) {
            return DRV_ERROR_INVALID_HANDLE;
        }
    }
    return drvHdcClientCreate_(client, maxSessionNum, serviceType, flag);
}

// drvHdcClientDestroy
drvError_t DriverPlugin::MsprofDrvHdcClientDestroy(HDC_CLIENT client)
{
    PthreadOnce(&loadFlag_, []()->void {DriverPlugin::instance()->LoadDriverSo();});
    if (drvHdcClientDestroy_ == nullptr) {
        int32_t ret = pluginHandle_.GetFunction<drvError_t, HDC_CLIENT>("drvHdcClientDestroy",
            drvHdcClientDestroy_);
        if (ret != PROFILING_SUCCESS) {
            return DRV_ERROR_INVALID_HANDLE;
        }
    }
    return drvHdcClientDestroy_(client);
}

// drvHdcServerCreate
drvError_t DriverPlugin::MsprofDrvHdcServerCreate(int devid, int serviceType, HDC_SERVER *pServer)
{
    PthreadOnce(&loadFlag_, []()->void {DriverPlugin::instance()->LoadDriverSo();});
    if (drvHdcServerCreate_ == nullptr) {
        int32_t ret = pluginHandle_.GetFunction<drvError_t, int, int, HDC_SERVER *>("drvHdcServerCreate",
            drvHdcServerCreate_);
        if (ret != PROFILING_SUCCESS) {
            return DRV_ERROR_INVALID_HANDLE;
        }
    }
    return drvHdcServerCreate_(devid, serviceType, pServer);
}

// drvHdcServerDestroy
drvError_t DriverPlugin::MsprofDrvHdcServerDestroy(HDC_SERVER server)
{
    PthreadOnce(&loadFlag_, []()->void {DriverPlugin::instance()->LoadDriverSo();});
    if (drvHdcServerDestroy_ == nullptr) {
        int32_t ret = pluginHandle_.GetFunction<drvError_t, HDC_SERVER>("drvHdcServerDestroy",
            drvHdcServerDestroy_);
        if (ret != PROFILING_SUCCESS) {
            return DRV_ERROR_INVALID_HANDLE;
        }
    }
    return drvHdcServerDestroy_(server);
}

// drvHdcSessionAccept
drvError_t DriverPlugin::MsprofDrvHdcSessionAccept(HDC_SERVER server, HDC_SESSION *session)
{
    PthreadOnce(&loadFlag_, []()->void {DriverPlugin::instance()->LoadDriverSo();});
    if (drvHdcSessionAccept_ == nullptr) {
        int32_t ret = pluginHandle_.GetFunction<drvError_t, HDC_SERVER, HDC_SESSION *>("drvHdcSessionAccept",
            drvHdcSessionAccept_);
        if (ret != PROFILING_SUCCESS) {
            return DRV_ERROR_INVALID_HANDLE;
        }
    }
    return drvHdcSessionAccept_(server, session);
}

// drvHdcGetMsgBuffer
drvError_t DriverPlugin::MsprofDrvHdcGetMsgBuffer(struct drvHdcMsg *msg, int index, char **pBuf, int *pLen)
{
    PthreadOnce(&loadFlag_, []()->void {DriverPlugin::instance()->LoadDriverSo();});
    if (drvHdcGetMsgBuffer_ == nullptr) {
        int32_t ret = pluginHandle_.GetFunction<drvError_t, struct drvHdcMsg *,
            int, char **, int *>("drvHdcGetMsgBuffer", drvHdcGetMsgBuffer_);
        if (ret != PROFILING_SUCCESS) {
            return DRV_ERROR_INVALID_HANDLE;
        }
    }
    return drvHdcGetMsgBuffer_(msg, index, pBuf, pLen);
}

// drvHdcAllocMsg
drvError_t DriverPlugin::MsprofDrvHdcAllocMsg(HDC_SESSION session, struct drvHdcMsg **ppMsg, int count)
{
    PthreadOnce(&loadFlag_, []()->void {DriverPlugin::instance()->LoadDriverSo();});
    if (drvHdcAllocMsg_ == nullptr) {
        int32_t ret = pluginHandle_.GetFunction<drvError_t, HDC_SESSION,
            struct drvHdcMsg **, int>("drvHdcAllocMsg", drvHdcAllocMsg_);
        if (ret != PROFILING_SUCCESS) {
            return DRV_ERROR_INVALID_HANDLE;
        }
    }
    return drvHdcAllocMsg_(session, ppMsg, count);
}

// drvHdcFreeMsg
drvError_t DriverPlugin::MsprofDrvHdcFreeMsg(struct drvHdcMsg *msg)
{
    PthreadOnce(&loadFlag_, []()->void {DriverPlugin::instance()->LoadDriverSo();});
    if (drvHdcFreeMsg_ == nullptr) {
        int32_t ret = pluginHandle_.GetFunction<drvError_t, struct drvHdcMsg *>("drvHdcFreeMsg", drvHdcFreeMsg_);
        if (ret != PROFILING_SUCCESS) {
            return DRV_ERROR_INVALID_HANDLE;
        }
    }
    return drvHdcFreeMsg_(msg);
}

// drvHdcReuseMsg
drvError_t DriverPlugin::MsprofDrvHdcReuseMsg(struct drvHdcMsg *msg)
{
    PthreadOnce(&loadFlag_, []()->void {DriverPlugin::instance()->LoadDriverSo();});
    if (drvHdcReuseMsg_ == nullptr) {
        int32_t ret = pluginHandle_.GetFunction<drvError_t, struct drvHdcMsg *>("drvHdcReuseMsg",
            drvHdcReuseMsg_);
        if (ret != PROFILING_SUCCESS) {
            return DRV_ERROR_INVALID_HANDLE;
        }
    }
    return drvHdcReuseMsg_(msg);
}

// drvHdcAddMsgBuffer
drvError_t DriverPlugin::MsprofDrvHdcAddMsgBuffer(struct drvHdcMsg *msg, char *pBuf, int len)
{
    PthreadOnce(&loadFlag_, []()->void {DriverPlugin::instance()->LoadDriverSo();});
    if (drvHdcAddMsgBuffer_ == nullptr) {
        int32_t ret = pluginHandle_.GetFunction<drvError_t, struct drvHdcMsg *, char *,
            int>("drvHdcAddMsgBuffer", drvHdcAddMsgBuffer_);
        if (ret != PROFILING_SUCCESS) {
            return DRV_ERROR_INVALID_HANDLE;
        }
    }
    return drvHdcAddMsgBuffer_(msg, pBuf, len);
}

// drvHdcSessionConnect
drvError_t DriverPlugin::MsprofDrvHdcSessionConnect(int peer_node, int peer_devid,
                                                    HDC_CLIENT client, HDC_SESSION *session)
{
    PthreadOnce(&loadFlag_, []()->void {DriverPlugin::instance()->LoadDriverSo();});
    if (drvHdcSessionConnect_ == nullptr) {
        int32_t ret = pluginHandle_.GetFunction<drvError_t, int, int, HDC_CLIENT,
            HDC_SESSION *>("drvHdcSessionConnect", drvHdcSessionConnect_);
        if (ret != PROFILING_SUCCESS) {
            return DRV_ERROR_INVALID_HANDLE;
        }
    }
    return drvHdcSessionConnect_(peer_node, peer_devid, client, session);
}

// drvHdcSessionClose
drvError_t DriverPlugin::MsprofDrvHdcSessionClose(HDC_SESSION session)
{
    PthreadOnce(&loadFlag_, []()->void {DriverPlugin::instance()->LoadDriverSo();});
    if (drvHdcSessionClose_ == nullptr) {
        int32_t ret = pluginHandle_.GetFunction<drvError_t, HDC_SESSION>("drvHdcSessionClose",
            drvHdcSessionClose_);
        if (ret != PROFILING_SUCCESS) {
            return DRV_ERROR_INVALID_HANDLE;
        }
    }
    return drvHdcSessionClose_(session);
}

// drvHdcGetCapacity
drvError_t DriverPlugin::MsprofDrvHdcGetCapacity(struct drvHdcCapacity *capacity)
{
    PthreadOnce(&loadFlag_, []()->void {DriverPlugin::instance()->LoadDriverSo();});
    if (drvHdcGetCapacity_ == nullptr) {
        int32_t ret = pluginHandle_.GetFunction<drvError_t, struct drvHdcCapacity *>("drvHdcGetCapacity",
            drvHdcGetCapacity_);
        if (ret != PROFILING_SUCCESS) {
            return DRV_ERROR_INVALID_HANDLE;
        }
    }
    return drvHdcGetCapacity_(capacity);
}
} // Plugin
} // Dvvp
} // Collector