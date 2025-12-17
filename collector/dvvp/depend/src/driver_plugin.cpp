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
#include "driver_plugin.h"
#include "msprof_dlog.h"

namespace Collector {
namespace Dvvp {
namespace Plugin {
SHARED_PTR_ALIA<PluginHandle> DriverPlugin::pluginHandle_ = nullptr;

void DriverPlugin::GetAllFunction()
{
    pluginHandle_->GetFunction<drvError_t, HDC_SESSION>("drvHdcSetSessionReference", drvHdcSetSessionReference_);
    pluginHandle_->GetFunction<int, unsigned int, channel_list_t *>("prof_drv_get_channels", profDrvGetChannels_);
    pluginHandle_->GetFunction<int, unsigned int, unsigned int, struct prof_start_para *>(
        "prof_drv_start", profDrvStart_);
    pluginHandle_->GetFunction<int, unsigned int, unsigned int>("prof_stop", profStop_);
    pluginHandle_->GetFunction<int, unsigned int, unsigned int, char *, unsigned int>(
        "prof_channel_read", profChannelRead_);
    pluginHandle_->GetFunction<int, struct prof_poll_info *, int, int>("prof_channel_poll", profChannelPoll_);
    pluginHandle_->GetFunction<drvError_t, uint32_t *>("drvGetDevNum", drvGetDevNum_);
    pluginHandle_->GetFunction<drvError_t, uint32_t, uint32_t*>("drvGetDevIDByLocalDevID", drvGetDevIDByLocalDevID_);
    pluginHandle_->GetFunction<drvError_t, uint32_t *, uint32_t>("drvGetDevIDs", drvGetDevIDs_);
    pluginHandle_->GetFunction<drvError_t, uint32_t *>("drvGetPlatformInfo", drvGetPlatformInfo_);
    pluginHandle_->GetFunction<drvError_t, HDC_CLIENT *, int, int, int>("drvHdcClientCreate", drvHdcClientCreate_);
    pluginHandle_->GetFunction<drvError_t, HDC_CLIENT>("drvHdcClientDestroy", drvHdcClientDestroy_);
    pluginHandle_->GetFunction<drvError_t, int, int, HDC_SERVER *>("drvHdcServerCreate", drvHdcServerCreate_);
    pluginHandle_->GetFunction<drvError_t, HDC_SERVER>("drvHdcServerDestroy", drvHdcServerDestroy_);
    pluginHandle_->GetFunction<drvError_t, HDC_SERVER, HDC_SESSION *>("drvHdcSessionAccept", drvHdcSessionAccept_);
    pluginHandle_->GetFunction<drvError_t, struct drvHdcMsg *, int, char **, int *>(
        "drvHdcGetMsgBuffer", drvHdcGetMsgBuffer_);
    pluginHandle_->GetFunction<drvError_t, HDC_SESSION, struct drvHdcMsg **, int>("drvHdcAllocMsg", drvHdcAllocMsg_);
    pluginHandle_->GetFunction<drvError_t, struct drvHdcMsg *>("drvHdcFreeMsg", drvHdcFreeMsg_);
    pluginHandle_->GetFunction<drvError_t, struct drvHdcMsg *>("drvHdcReuseMsg", drvHdcReuseMsg_);
    pluginHandle_->GetFunction<drvError_t, struct drvHdcMsg *, char *, int>("drvHdcAddMsgBuffer", drvHdcAddMsgBuffer_);
    pluginHandle_->GetFunction<drvError_t, int, int, HDC_CLIENT, HDC_SESSION *>(
        "drvHdcSessionConnect", drvHdcSessionConnect_);
    pluginHandle_->GetFunction<drvError_t, HDC_SESSION>("drvHdcSessionClose", drvHdcSessionClose_);
    pluginHandle_->GetFunction<drvError_t, struct drvHdcCapacity *>("drvHdcGetCapacity", drvHdcGetCapacity_);
    pluginHandle_->GetFunction<drvError_t, unsigned int, unsigned int *>(
        "drvGetDeviceSplitMode", drvGetDeviceSplitMode_);
    pluginHandle_->GetFunction<drvError_t, uint32_t, uint32_t *>("drvDeviceGetPhyIdByIndex", drvDeviceGetPhyIdByIndex_);
    GetHalFunction();
}

void DriverPlugin::GetHalFunction()
{
    pluginHandle_->GetFunction<hdcError_t, HDC_SESSION, struct drvHdcMsg *, int, uint64_t, int *, uint32_t>(
        "halHdcRecv", halHdcRecv_);
    pluginHandle_->GetFunction<hdcError_t, HDC_SESSION, struct drvHdcMsg *, uint64_t, uint32_t>(
        "halHdcSend", halHdcSend_);
    pluginHandle_->GetFunction<hdcError_t, int, int, int, HDC_CLIENT, HDC_SESSION *>(
        "halHdcSessionConnectEx", halHdcSessionConnectEx_);
    pluginHandle_->GetFunction<drvError_t, HDC_SESSION, int, int *>(
        "halHdcGetSessionAttr", halHdcGetSessionAttr_);
    pluginHandle_->GetFunction<drvError_t, unsigned int, halChipInfo *>("halGetChipInfo", halGetChipInfo_);
    pluginHandle_->GetFunction<drvError_t, uint32_t, int32_t, int32_t, int64_t *>(
        "halGetDeviceInfo", halGetDeviceInfo_);
    pluginHandle_->GetFunction<drvError_t, int32_t *>("halGetAPIVersion", halGetApiVersion_);
    pluginHandle_->GetFunction<int, unsigned int, unsigned int, unsigned int *>("halProfDataFlush", halProfDataFlush_);
    pluginHandle_->GetFunction<drvError_t, uint32_t, struct esched_grp_para *, unsigned int *>(
        "halEschedCreateGrpEx", halEschedCreateGrpEx_);
    pluginHandle_->GetFunction<drvError_t, unsigned int>("halEschedAttachDevice", halEschedAttachDevice_);
    pluginHandle_->GetFunction<drvError_t, unsigned int>("halEschedDettachDevice", halEschedDettachDevice_);
    pluginHandle_->GetFunction<drvError_t, unsigned int, unsigned int, unsigned int, unsigned long long>(
        "halEschedSubscribeEvent", halEschedSubscribeEvent_);
    pluginHandle_->GetFunction<drvError_t, unsigned int, unsigned int, unsigned int, int, struct event_info *>(
        "halEschedWaitEvent", halEschedWaitEvent_);
    pluginHandle_->GetFunction<drvError_t, uint32_t, int32_t, int32_t, void *, int32_t *>("halGetDeviceInfoByBuff",
        halGetDeviceInfoByBuff_);
    pluginHandle_->GetFunction<drvError_t, unsigned int, ESCHED_QUERY_TYPE,
        struct esched_input_info *, struct esched_output_info *>("halEschedQueryInfo", halEschedQueryInfo_);
    pluginHandle_->GetFunction<drvError_t, struct halQueryDevpidInfo, pid_t *>("halQueryDevpid", halQueryDevpid_);
}

void DriverPlugin::LoadDriverSo()
{
    if (pluginHandle_ == nullptr) {
        MSVP_MAKE_SHARED1_VOID(pluginHandle_, PluginHandle, soName_);
    }
    if (!pluginHandle_->HasLoad()) {
        bool isSoValid = true;
        if (pluginHandle_->OpenPluginFromEnv("LD_LIBRARY_PATH", isSoValid) != PROFILING_SUCCESS &&
            pluginHandle_->OpenPluginFromLdcfg(isSoValid) != PROFILING_SUCCESS) {
            MSPROF_LOGE("DriverPlugin failed to load driver so");
            return;
        }
        if (!isSoValid) {
            MSPROF_LOGW("driver so it may not be secure because there are incorrect permissions");
        }
    }
    GetAllFunction();
}

bool DriverPlugin::IsFuncExist(const std::string &funcName) const
{
    return pluginHandle_->IsFuncExist(funcName);
}

// halHdcRecv
hdcError_t DriverPlugin::MsprofHalHdcRecv(HDC_SESSION session, struct drvHdcMsg *pMsg, int bufLen,
    uint64_t flag, int *recvBufCount, uint32_t timeout)
{
    PthreadOnce(&loadFlag_, []()->void {DriverPlugin::instance()->LoadDriverSo();});
    if (halHdcRecv_ == nullptr) {
        MSPROF_LOGE("DriverPlugin halHdcRecv function is null.");
        return DRV_ERROR_INVALID_HANDLE;
    }
    return halHdcRecv_(session, pMsg, bufLen, flag, recvBufCount, timeout);
}

// halHdcSend
hdcError_t DriverPlugin::MsprofHalHdcSend(HDC_SESSION session, struct drvHdcMsg *pMsg, uint64_t flag, uint32_t timeout)
{
    PthreadOnce(&loadFlag_, []()->void {DriverPlugin::instance()->LoadDriverSo();});
    if (halHdcSend_ == nullptr) {
        MSPROF_LOGE("DriverPlugin halHdcSend function is null.");
        return DRV_ERROR_INVALID_HANDLE;
    }
    return halHdcSend_(session, pMsg, flag, timeout);
}

// halHdcSessionConnectEx
hdcError_t DriverPlugin::MsprofHalHdcSessionConnectEx(int peer_node, int peer_devid, int peer_pid, HDC_CLIENT client,
    HDC_SESSION *pSession)
{
    PthreadOnce(&loadFlag_, []()->void {DriverPlugin::instance()->LoadDriverSo();});
    if (halHdcSessionConnectEx_ == nullptr) {
        MSPROF_LOGE("DriverPlugin halHdcSessionConnectEx function is null.");
        return DRV_ERROR_INVALID_HANDLE;
    }
    return halHdcSessionConnectEx_(peer_node, peer_devid, peer_pid, client, pSession);
}

// drvHdcSetSessionReference
drvError_t DriverPlugin::MsprofDrvHdcSetSessionReference(HDC_SESSION session)
{
    PthreadOnce(&loadFlag_, []()->void {DriverPlugin::instance()->LoadDriverSo();});
    if (drvHdcSetSessionReference_ == nullptr) {
        MSPROF_LOGE("DriverPlugin drvHdcSetSessionReference function is null.");
        return DRV_ERROR_INVALID_HANDLE;
    }
    return drvHdcSetSessionReference_(session);
}

// halHdcGetSessionAttr
drvError_t DriverPlugin::MsprofHalHdcGetSessionAttr(HDC_SESSION session, int attr, int *value)
{
    PthreadOnce(&loadFlag_, []()->void {DriverPlugin::instance()->LoadDriverSo();});
    if (halHdcGetSessionAttr_ == nullptr) {
        MSPROF_LOGE("DriverPlugin halHdcGetSessionAttr function is null.");
        return DRV_ERROR_INVALID_HANDLE;
    }
    return halHdcGetSessionAttr_(session, attr, value);
}

// halGetChipInfo
drvError_t DriverPlugin::MsprofHalGetChipInfo(unsigned int devId, halChipInfo *chipInfo)
{
    PthreadOnce(&loadFlag_, []()->void {DriverPlugin::instance()->LoadDriverSo();});
    if (halGetChipInfo_ == nullptr) {
        MSPROF_LOGE("DriverPlugin halGetChipInfo function is null.");
        return DRV_ERROR_INVALID_HANDLE;
    }
    return halGetChipInfo_(devId, chipInfo);
}

// halGetDeviceInfo
drvError_t DriverPlugin::MsprofHalGetDeviceInfo(uint32_t devId, int32_t moduleType, int32_t infoType, int64_t *value)
{
    PthreadOnce(&loadFlag_, []()->void {DriverPlugin::instance()->LoadDriverSo();});
    if (halGetDeviceInfo_ == nullptr) {
        MSPROF_LOGE("DriverPlugin halGetDeviceInfo function is null.");
        return DRV_ERROR_INVALID_HANDLE;
    }
    return halGetDeviceInfo_(devId, moduleType, infoType, value);
}

drvError_t DriverPlugin::MsprofHalGetApiVersion(int32_t *value)
{
    PthreadOnce(&loadFlag_, []()->void {DriverPlugin::instance()->LoadDriverSo();});
    if (halGetApiVersion_ == nullptr) {
        MSPROF_LOGE("DriverPlugin halGetApiVersion function is null.");
        return DRV_ERROR_INVALID_HANDLE;
    }
    return halGetApiVersion_(value);
}

// halProfDataFlush
int DriverPlugin::MsprofHalProfDataFlush(unsigned int device_id, unsigned int channel_id, unsigned int *data_len)
{
    PthreadOnce(&loadFlag_, []()->void {DriverPlugin::instance()->LoadDriverSo();});
    if (halProfDataFlush_ == nullptr) {
        MSPROF_LOGE("DriverPlugin halProfDataFlush function is null.");
        return DRV_ERROR_INVALID_HANDLE;
    }
    return halProfDataFlush_(device_id, channel_id, data_len);
}

// prof_drv_get_channels
int DriverPlugin::MsprofDrvGetChannels(unsigned int device_id, channel_list_t *channels)
{
    PthreadOnce(&loadFlag_, []()->void {DriverPlugin::instance()->LoadDriverSo();});
    if (profDrvGetChannels_ == nullptr) {
        MSPROF_LOGE("DriverPlugin profDrvGetChannels function is null.");
        return DRV_ERROR_INVALID_HANDLE;
    }
    return profDrvGetChannels_(device_id, channels);
}

// prof_drv_start
int DriverPlugin::MsprofDrvStart(unsigned int device_id, unsigned int channel_id, struct prof_start_para *start_para)
{
    PthreadOnce(&loadFlag_, []()->void {DriverPlugin::instance()->LoadDriverSo();});
    if (profDrvStart_ == nullptr) {
        MSPROF_LOGE("DriverPlugin profDrvStart function is null.");
        return DRV_ERROR_INVALID_HANDLE;
    }
    return profDrvStart_(device_id, channel_id, start_para);
}

// prof_stop
int DriverPlugin::MsprofDrvStop(unsigned int device_id, unsigned int channel_id)
{
    PthreadOnce(&loadFlag_, []()->void {DriverPlugin::instance()->LoadDriverSo();});
    if (profStop_ == nullptr) {
        MSPROF_LOGE("DriverPlugin profStop function is null.");
        return DRV_ERROR_INVALID_HANDLE;
    }
    return profStop_(device_id, channel_id);
}

// prof_channel_read
int DriverPlugin::MsprofChannelRead(unsigned int device_id, unsigned int channel_id,
                                    char *out_buf, unsigned int buf_size)
{
    PthreadOnce(&loadFlag_, []()->void {DriverPlugin::instance()->LoadDriverSo();});
    if (profChannelRead_ == nullptr) {
        MSPROF_LOGE("DriverPlugin profChannelRead function is null.");
        return DRV_ERROR_INVALID_HANDLE;
    }
    return profChannelRead_(device_id, channel_id, out_buf, buf_size);
}

// prof_channel_poll
int DriverPlugin::MsprofChannelPoll(struct prof_poll_info *out_buf, int num, int timeout)
{
    PthreadOnce(&loadFlag_, []()->void {DriverPlugin::instance()->LoadDriverSo();});
    if (profChannelPoll_ == nullptr) {
        MSPROF_LOGE("DriverPlugin profChannelPoll function is null.");
        return DRV_ERROR_INVALID_HANDLE;
    }
    return profChannelPoll_(out_buf, num, timeout);
}

// drvGetDevNum
drvError_t DriverPlugin::MsprofDrvGetDevNum(uint32_t *num_dev)
{
    PthreadOnce(&loadFlag_, []()->void {DriverPlugin::instance()->LoadDriverSo();});
    if (drvGetDevNum_ == nullptr) {
        MSPROF_LOGE("DriverPlugin drvGetDevNum function is null.");
        return DRV_ERROR_INVALID_HANDLE;
    }
    return drvGetDevNum_(num_dev);
}

// drvGetDevIDByLocalDevID
drvError_t DriverPlugin::MsprofDrvGetDevIDByLocalDevID(uint32_t localDevId, uint32_t *devId)
{
    PthreadOnce(&loadFlag_, []()->void {DriverPlugin::instance()->LoadDriverSo();});
    if (drvGetDevIDByLocalDevID_ == nullptr) {
        MSPROF_LOGE("DriverPlugin drvGetDevIDByLocalDevID function is null.");
        return DRV_ERROR_INVALID_HANDLE;
    }
    return drvGetDevIDByLocalDevID_(localDevId, devId);
}

// drvGetDevIDs
drvError_t DriverPlugin::MsprofDrvGetDevIDs(uint32_t *devices, uint32_t len)
{
    PthreadOnce(&loadFlag_, []()->void {DriverPlugin::instance()->LoadDriverSo();});
    if (drvGetDevIDs_ == nullptr) {
        MSPROF_LOGE("DriverPlugin drvGetDevIDs function is null.");
        return DRV_ERROR_INVALID_HANDLE;
    }
    return drvGetDevIDs_(devices, len);
}

// drvGetPlatformInfo
drvError_t DriverPlugin::MsprofDrvGetPlatformInfo(uint32_t *info)
{
    PthreadOnce(&loadFlag_, []()->void {DriverPlugin::instance()->LoadDriverSo();});
    if (drvGetPlatformInfo_ == nullptr) {
        MSPROF_LOGE("DriverPlugin drvGetPlatformInfo function is null.");
        return DRV_ERROR_INVALID_HANDLE;
    }
    return drvGetPlatformInfo_(info);
}

// drvHdcClientCreate
drvError_t DriverPlugin::MsprofDrvHdcClientCreate(HDC_CLIENT *client, int maxSessionNum, int serviceType, int flag)
{
    PthreadOnce(&loadFlag_, []()->void {DriverPlugin::instance()->LoadDriverSo();});
    if (drvHdcClientCreate_ == nullptr) {
        MSPROF_LOGE("DriverPlugin drvHdcClientCreate function is null.");
        return DRV_ERROR_INVALID_HANDLE;
    }
    return drvHdcClientCreate_(client, maxSessionNum, serviceType, flag);
}

// drvHdcClientDestroy
drvError_t DriverPlugin::MsprofDrvHdcClientDestroy(HDC_CLIENT client)
{
    PthreadOnce(&loadFlag_, []()->void {DriverPlugin::instance()->LoadDriverSo();});
    if (drvHdcClientDestroy_ == nullptr) {
        MSPROF_LOGE("DriverPlugin drvHdcClientDestroy function is null.");
        return DRV_ERROR_INVALID_HANDLE;
    }
    return drvHdcClientDestroy_(client);
}

// drvHdcServerCreate
drvError_t DriverPlugin::MsprofDrvHdcServerCreate(int devid, int serviceType, HDC_SERVER *pServer)
{
    PthreadOnce(&loadFlag_, []()->void {DriverPlugin::instance()->LoadDriverSo();});
    if (drvHdcServerCreate_ == nullptr) {
        MSPROF_LOGE("DriverPlugin drvHdcServerCreate function is null.");
        return DRV_ERROR_INVALID_HANDLE;
    }
    return drvHdcServerCreate_(devid, serviceType, pServer);
}

// drvHdcServerDestroy
drvError_t DriverPlugin::MsprofDrvHdcServerDestroy(HDC_SERVER server)
{
    PthreadOnce(&loadFlag_, []()->void {DriverPlugin::instance()->LoadDriverSo();});
    if (drvHdcServerDestroy_ == nullptr) {
        MSPROF_LOGE("DriverPlugin drvHdcServerDestroy function is null.");
        return DRV_ERROR_INVALID_HANDLE;
    }
    return drvHdcServerDestroy_(server);
}

// drvHdcSessionAccept
drvError_t DriverPlugin::MsprofDrvHdcSessionAccept(HDC_SERVER server, HDC_SESSION *session)
{
    PthreadOnce(&loadFlag_, []()->void {DriverPlugin::instance()->LoadDriverSo();});
    if (drvHdcSessionAccept_ == nullptr) {
        MSPROF_LOGE("DriverPlugin drvHdcSessionAccept function is null.");
        return DRV_ERROR_INVALID_HANDLE;
    }
    return drvHdcSessionAccept_(server, session);
}

// drvHdcGetMsgBuffer
drvError_t DriverPlugin::MsprofDrvHdcGetMsgBuffer(struct drvHdcMsg *msg, int index, char **pBuf, int *pLen)
{
    PthreadOnce(&loadFlag_, []()->void {DriverPlugin::instance()->LoadDriverSo();});
    if (drvHdcGetMsgBuffer_ == nullptr) {
        MSPROF_LOGE("DriverPlugin drvHdcGetMsgBuffer function is null.");
        return DRV_ERROR_INVALID_HANDLE;
    }
    return drvHdcGetMsgBuffer_(msg, index, pBuf, pLen);
}

// drvHdcAllocMsg
drvError_t DriverPlugin::MsprofDrvHdcAllocMsg(HDC_SESSION session, struct drvHdcMsg **ppMsg, int count)
{
    PthreadOnce(&loadFlag_, []()->void {DriverPlugin::instance()->LoadDriverSo();});
    if (drvHdcAllocMsg_ == nullptr) {
        MSPROF_LOGE("DriverPlugin drvHdcAllocMsg function is null.");
        return DRV_ERROR_INVALID_HANDLE;
    }
    return drvHdcAllocMsg_(session, ppMsg, count);
}

// drvHdcFreeMsg
drvError_t DriverPlugin::MsprofDrvHdcFreeMsg(struct drvHdcMsg *msg)
{
    PthreadOnce(&loadFlag_, []()->void {DriverPlugin::instance()->LoadDriverSo();});
    if (drvHdcFreeMsg_ == nullptr) {
        MSPROF_LOGE("DriverPlugin drvHdcFreeMsg function is null.");
        return DRV_ERROR_INVALID_HANDLE;
    }
    return drvHdcFreeMsg_(msg);
}

// drvHdcReuseMsg
drvError_t DriverPlugin::MsprofDrvHdcReuseMsg(struct drvHdcMsg *msg)
{
    PthreadOnce(&loadFlag_, []()->void {DriverPlugin::instance()->LoadDriverSo();});
    if (drvHdcReuseMsg_ == nullptr) {
        MSPROF_LOGE("DriverPlugin drvHdcReuseMsg function is null.");
        return DRV_ERROR_INVALID_HANDLE;
    }
    return drvHdcReuseMsg_(msg);
}

// drvHdcAddMsgBuffer
drvError_t DriverPlugin::MsprofDrvHdcAddMsgBuffer(struct drvHdcMsg *msg, char *pBuf, int len)
{
    PthreadOnce(&loadFlag_, []()->void {DriverPlugin::instance()->LoadDriverSo();});
    if (drvHdcAddMsgBuffer_ == nullptr) {
        MSPROF_LOGE("DriverPlugin drvHdcAddMsgBuffer function is null.");
        return DRV_ERROR_INVALID_HANDLE;
    }
    return drvHdcAddMsgBuffer_(msg, pBuf, len);
}

// drvHdcSessionConnect
drvError_t DriverPlugin::MsprofDrvHdcSessionConnect(int peer_node, int peer_devid,
                                                    HDC_CLIENT client, HDC_SESSION *session)
{
    PthreadOnce(&loadFlag_, []()->void {DriverPlugin::instance()->LoadDriverSo();});
    if (drvHdcSessionConnect_ == nullptr) {
        MSPROF_LOGE("DriverPlugin drvHdcSessionConnect function is null.");
        return DRV_ERROR_INVALID_HANDLE;
    }
    return drvHdcSessionConnect_(peer_node, peer_devid, client, session);
}

// drvHdcSessionClose
drvError_t DriverPlugin::MsprofDrvHdcSessionClose(HDC_SESSION session)
{
    PthreadOnce(&loadFlag_, []()->void {DriverPlugin::instance()->LoadDriverSo();});
    if (drvHdcSessionClose_ == nullptr) {
        MSPROF_LOGE("DriverPlugin drvHdcSessionClose function is null.");
        return DRV_ERROR_INVALID_HANDLE;
    }
    return drvHdcSessionClose_(session);
}

// drvHdcGetCapacity
drvError_t DriverPlugin::MsprofDrvHdcGetCapacity(struct drvHdcCapacity *capacity)
{
    PthreadOnce(&loadFlag_, []()->void {DriverPlugin::instance()->LoadDriverSo();});
    if (drvHdcGetCapacity_ == nullptr) {
        MSPROF_LOGE("DriverPlugin drvHdcGetCapacity function is null.");
        return DRV_ERROR_INVALID_HANDLE;
    }
    return drvHdcGetCapacity_(capacity);
}

// drvDeviceGetPhyIdByIndex
drvError_t DriverPlugin::MsprofDrvDeviceGetPhyIdByIndex(uint32_t devIndex, uint32_t *phyId)
{
    PthreadOnce(&loadFlag_, []()->void {DriverPlugin::instance()->LoadDriverSo();});
    if (drvDeviceGetPhyIdByIndex_ == nullptr) {
        MSPROF_LOGE("DriverPlugin drvDeviceGetPhyIdByIndex function is null.");
        return DRV_ERROR_INVALID_HANDLE;
    }
    return drvDeviceGetPhyIdByIndex_(devIndex, phyId);
}

// halEschedCreateGrpEx
drvError_t DriverPlugin::MsprofHalEschedCreateGrpEx(uint32_t devId,
                                                    struct esched_grp_para *grpPara, unsigned int *grpId)
{
    PthreadOnce(&loadFlag_, []()->void {DriverPlugin::instance()->LoadDriverSo();});
    if (halEschedCreateGrpEx_ == nullptr) {
        MSPROF_LOGE("DriverPlugin halEschedCreateGrpEx function is null.");
        return DRV_ERROR_INVALID_HANDLE;
    }
    return halEschedCreateGrpEx_(devId, grpPara, grpId);
}

// halEschedAttachDevice
drvError_t DriverPlugin::MsprofHalEschedAttachDevice(unsigned int devId)
{
    PthreadOnce(&loadFlag_, []()->void {DriverPlugin::instance()->LoadDriverSo();});
    if (halEschedAttachDevice_ == nullptr) {
        MSPROF_LOGE("DriverPlugin halEschedAttachDevice function is null.");
        return DRV_ERROR_INVALID_HANDLE;
    }
    return halEschedAttachDevice_(devId);
}

// halEschedDettachDevice
drvError_t DriverPlugin::MsprofHalEschedDettachDevice(unsigned int devId)
{
    PthreadOnce(&loadFlag_, []()->void {DriverPlugin::instance()->LoadDriverSo();});
    if (halEschedDettachDevice_ == nullptr) {
        MSPROF_LOGE("DriverPlugin halEschedDettachDevice function is null.");
        return DRV_ERROR_INVALID_HANDLE;
    }
    return halEschedDettachDevice_(devId);
}

// halEschedSubscribeEvent
drvError_t DriverPlugin::MsprofHalEschedSubscribeEvent(unsigned int devId, unsigned int grpId,
                                                       unsigned int threadId, unsigned long long eventBitmap)
{
    PthreadOnce(&loadFlag_, []()->void {DriverPlugin::instance()->LoadDriverSo();});
    if (halEschedSubscribeEvent_ == nullptr) {
        MSPROF_LOGE("DriverPlugin halEschedSubscribeEvent function is null.");
        return DRV_ERROR_INVALID_HANDLE;
    }
    return halEschedSubscribeEvent_(devId, grpId, threadId, eventBitmap);
}

// halEschedWaitEvent
drvError_t DriverPlugin::MsprofHalEschedWaitEvent(unsigned int devId, unsigned int grpId,
                                                  unsigned int threadId, int timeout, struct event_info *event)
{
    PthreadOnce(&loadFlag_, []()->void {DriverPlugin::instance()->LoadDriverSo();});
    if (halEschedWaitEvent_ == nullptr) {
        MSPROF_LOGE("DriverPlugin halEschedWaitEvent function is null.");
        return DRV_ERROR_INVALID_HANDLE;
    }
    return halEschedWaitEvent_(devId, grpId, threadId, timeout, event);
}

// drvGetDeviceSplitMode
drvError_t DriverPlugin::MsprofDrvGetDeviceSplitMode(unsigned int devId, unsigned int* mode)
{
    PthreadOnce(&loadFlag_, []()->void {DriverPlugin::instance()->LoadDriverSo();});
    if (drvGetDeviceSplitMode_ == nullptr) {
        MSPROF_LOGE("DriverPlugin drvGetDeviceSplitMode function is null.");
        return DRV_ERROR_INVALID_HANDLE;
    }
    return drvGetDeviceSplitMode_(devId, mode);
}

// halGetDeviceInfoByBuff
drvError_t DriverPlugin::MsprofHalGetDeviceInfoByBuff(uint32_t devId, int32_t moduleType,
                                                      int32_t infoType, void *buf, int32_t *size)
{
    PthreadOnce(&loadFlag_, []() -> void { DriverPlugin::instance()->LoadDriverSo(); });
    if (halGetDeviceInfoByBuff_ != nullptr) {
        return halGetDeviceInfoByBuff_(devId, moduleType, infoType, buf, size);
    }
    return DRV_ERROR_NOT_SUPPORT;
}

// halEschedQueryInfo
drvError_t DriverPlugin::MsprofHalEschedQueryInfo(unsigned int devId, ESCHED_QUERY_TYPE type,
                                                  struct esched_input_info *inPut, struct esched_output_info *outPut)
{
    PthreadOnce(&loadFlag_, []()->void {DriverPlugin::instance()->LoadDriverSo();});
    if (halEschedQueryInfo_ == nullptr) {
        MSPROF_LOGE("DriverPlugin halEschedQueryInfo function is null.");
        return DRV_ERROR_INVALID_HANDLE;
    }
    return halEschedQueryInfo_(devId, type, inPut, outPut);
}

// halQueryDevpid
drvError_t DriverPlugin::MsprofHalQueryDevpid(struct halQueryDevpidInfo pidInfo, pid_t *pid)
{
    PthreadOnce(&loadFlag_, []()->void {DriverPlugin::instance()->LoadDriverSo();});
    if (halQueryDevpid_ == nullptr) {
        MSPROF_LOGE("DriverPlugin halQueryDevpid function is null.");
        return DRV_ERROR_INVALID_HANDLE;
    }
    return halQueryDevpid_(pidInfo, pid);
}
} // Plugin
} // Dvvp
} // Collector