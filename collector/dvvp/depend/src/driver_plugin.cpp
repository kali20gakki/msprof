#include "driver_plugin.h"

namespace Analysis {
namespace Dvvp {
namespace Plugin {

DriverPlugin::~DriverPlugin()
{
    pluginManager_.CloseHandle();
}

bool DriverPlugin::IsFuncExist(const std::string &funcName) const
{
    return pluginManager_.IsFuncExist(std::string funcName);
}

// halHdcRecv
hdcError_t DriverPlugin::MsprofHalHdcRecv(HDC_SESSION session, struct drvHdcMsg *pMsg, int bufLen,
    UINT64 flag, int *recvBufCount, UINT32 timeout)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginManager_.HasLoad()) {
        ret = pluginManager_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return DRV_ERROR_INVALID_HANDLE;
        }
    }
    MSPROF_HALHDCRECV_T func;
    ret = pluginManager_.GetFunction<hdcError_t, HDC_SESSION, struct drvHdcMsg *, int, UINT64, int *, UINT32>("halHdcRecv", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return DRV_ERROR_INVALID_HANDLE;
    }
    return func(session, pMsg, bufLen, flag, recvBufCount, timeout);
}

// halHdcSend
hdcError_t DriverPlugin::MsprofHalHdcSend(HDC_SESSION session, struct drvHdcMsg *pMsg, UINT64 flag, UINT32 timeout)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginManager_.HasLoad()) {
        ret = pluginManager_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return DRV_ERROR_INVALID_HANDLE;
        }
    }
    MSPROF_HALHDCSEND_T func;
    ret = pluginManager_.GetFunction<hdcError_t, HDC_SESSION, struct drvHdcMsg *, UINT64, UINT32>("halHdcSend", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return DRV_ERROR_INVALID_HANDLE;
    }
    return func(session, pMsg, flag, timeout);
}

// halHdcSessionConnectEx
hdcError_t DriverPlugin::MsprofHalHdcSessionConnectEx(int peer_node, int peer_devid, int peer_pid, HDC_CLIENT client,
    HDC_SESSION *pSession)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginManager_.HasLoad()) {
        ret = pluginManager_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return DRV_ERROR_INVALID_HANDLE;
        }
    }
    MSPROF_HALHDCSESSIONCONNECTEX_T func;
    ret = pluginManager_.GetFunction<hdcError_t, int, int, int, HDC_CLIENT, HDC_SESSION *>("halHdcSessionConnectEx", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return DRV_ERROR_INVALID_HANDLE;
    }
    return func(peer_node, peer_devid, peer_pid, client, pSession);
}

// drvHdcSetSessionReference
drvError_t DriverPlugin::MsprofDrvHdcSetSessionReference(HDC_SESSION session)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginManager_.HasLoad()) {
        ret = pluginManager_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return DRV_ERROR_INVALID_HANDLE;
        }
    }
    MSPROF_DRVHDCSETSESSIONREFERENCE_T func;
    ret = pluginManager_.GetFunction<drvError_t, HDC_SESSION>("drvHdcSetSessionReference", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return DRV_ERROR_INVALID_HANDLE;
    }
    return func(session);
}

// halHdcGetSessionAttr
drvError_t DriverPlugin::MsprofHalHdcGetSessionAttr(HDC_SESSION session, int attr, int *value)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginManager_.HasLoad()) {
        ret = pluginManager_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return DRV_ERROR_INVALID_HANDLE;
        }
    }
    MSPROF_HALHDCGETSESSIONATTR_T func;
    ret = pluginManager_.GetFunction<drvError_t, HDC_SESSION, int, int *>("halHdcGetSessionAttr", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return DRV_ERROR_INVALID_HANDLE;
    }
    return func(session, attr, value);
}

// halGetChipInfo
drvError_t DriverPlugin::MsprofHalGetChipInfo(unsigned int devId, halChipInfo *chipInfo)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginManager_.HasLoad()) {
        ret = pluginManager_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return DRV_ERROR_INVALID_HANDLE;
        }
    }
    MSPROF_HALGETCHIPINFO_T func;
    ret = pluginManager_.GetFunction<drvError_t, unsigned int, halChipInfo *>("halGetChipInfo", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return DRV_ERROR_INVALID_HANDLE;
    }
    return func(devId, chipInfo);
}

// halGetDeviceInfo
drvError_t DriverPlugin::MsprofHalGetDeviceInfo(uint32_t devId, int32_t moduleType, int32_t infoType, int64_t *value)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginManager_.HasLoad()) {
        ret = pluginManager_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return DRV_ERROR_INVALID_HANDLE;
        }
    }
    MSPROF_HALGETDEVICEINFO_T func;
    ret = pluginManager_.GetFunction<drvError_t, uint32_t, int32_t, int32_t, int64_t *>("halGetDeviceInfo", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return DRV_ERROR_INVALID_HANDLE;
    }
    return func(devId, moduleType, infoType, value);
}

// halProfDataFlush
int DriverPlugin::MsprofHalProfDataFlush(unsigned int device_id, unsigned int channel_id, unsigned int *data_len)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginManager_.HasLoad()) {
        ret = pluginManager_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_HALPROFDATAFLUSH_T func;
    ret = pluginManager_.GetFunction<int, unsigned int, unsigned int, unsigned int *>("halProfDataFlush", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(device_id, channel_id, data_len);
}

// prof_drv_get_channels
int DriverPlugin::MsprofDrvGetChannels(unsigned int device_id, channel_list_t *channels)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginManager_.HasLoad()) {
        ret = pluginManager_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_PROFDRVGETCHANNELS_T func;
    ret = pluginManager_.GetFunction<int, unsigned int, channel_list_t *>("prof_drv_get_channels", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(device_id, channels);
}

// prof_drv_start
int DriverPlugin::MsprofDrvStart(unsigned int device_id, unsigned int channel_id, struct prof_start_para *start_para)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginManager_.HasLoad()) {
        ret = pluginManager_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_PROFDRVSRART_T func;
    ret = pluginManager_.GetFunction<int, unsigned int, unsigned int, struct prof_start_para *>("prof_drv_start", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(device_id, channel_id, start_para);
}

// prof_stop
int DriverPlugin::MsprofDrvStop(unsigned int device_id, unsigned int channel_id)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginManager_.HasLoad()) {
        ret = pluginManager_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_PROFSTOP_T func;
    ret = pluginManager_.GetFunction<int, unsigned int, unsigned int>("prof_stop", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(device_id, channel_id);
}

// prof_channel_read
int DriverPlugin::MsprofChannelRead(unsigned int device_id, unsigned int channel_id, char *out_buf, unsigned int buf_size)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginManager_.HasLoad()) {
        ret = pluginManager_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_CHANNELREAD_T func;
    ret = pluginManager_.GetFunction<int, unsigned int, unsigned int, char *, unsigned int>("prof_channel_read", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(device_id, channel_id, out_buf, buf_size);
}

// prof_channel_poll
int DriverPlugin::MsprofChannelPoll(struct prof_poll_info *out_buf, int num, int timeout)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginManager_.HasLoad()) {
        ret = pluginManager_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_CHANNELPOLL_T func;
    ret = pluginManager_.GetFunction<int, struct prof_poll_info *, int, int>("prof_channel_poll", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(out_buf, num, timeout);
}

// drvGetDevNum
drvError_t DriverPlugin::MsprofDrvGetDevNum(uint32_t *num_dev)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginManager_.HasLoad()) {
        ret = pluginManager_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return DRV_ERROR_INVALID_HANDLE;
        }
    }
    MSPROF_DRVGETDEVNUM_T func;
    ret = pluginManager_.GetFunction<drvError_t, uint32_t *>("drvGetDevNum", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return DRV_ERROR_INVALID_HANDLE;
    }
    return func(num_dev);
}

// drvGetDevIDByLocalDevID
drvError_t DriverPlugin::MsprofDrvGetDevIDByLocalDevID(uint32_t localDevId, uint32_t *devId)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginManager_.HasLoad()) {
        ret = pluginManager_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return DRV_ERROR_INVALID_HANDLE;
        }
    }
    MSPROF_DRVGETDEVIDBYLOCALID_T func;
    ret = pluginManager_.GetFunction<drvError_t, uint32_t, uint32_t*>("drvGetDevIDByLocalDevID", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return DRV_ERROR_INVALID_HANDLE;
    }
    return func(localDevId, devId);
}

// drvGetDevIDs
drvError_t DriverPlugin::MsprofDrvGetDevIDs(uint32_t *devices, uint32_t len)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginManager_.HasLoad()) {
        ret = pluginManager_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return DRV_ERROR_INVALID_HANDLE;
        }
    }
    MSPROF_DRVGETDEVIDS_T func;
    ret = pluginManager_.GetFunction<drvError_t, uint32_t *, uint32_t>("drvGetDevIDs", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return DRV_ERROR_INVALID_HANDLE;
    }
    return func(devices, len);
}

// drvGetPlatformInfo
drvError_t DriverPlugin::MsprofDrvGetPlatformInfo(uint32_t *info)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginManager_.HasLoad()) {
        ret = pluginManager_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return DRV_ERROR_INVALID_HANDLE;
        }
    }
    MSPROF_DRVGETPLATFORMINFO_T func;
    ret = pluginManager_.GetFunction<drvError_t, uint32_t *>("drvGetPlatformInfo", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return DRV_ERROR_INVALID_HANDLE;
    }
    return func(info);
}

// drvHdcClientCreate
drvError_t DriverPlugin::MsprofDrvHdcClientCreate(HDC_CLIENT *client, int maxSessionNum, int serviceType, int flag)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginManager_.HasLoad()) {
        ret = pluginManager_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return DRV_ERROR_INVALID_HANDLE;
        }
    }
    MSPROF_DRVHDCCLIENTCREATE_T func;
    ret = pluginManager_.GetFunction<drvError_t, HDC_CLIENT *, int, int, int>("drvHdcClientCreate", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return DRV_ERROR_INVALID_HANDLE;
    }
    return func(client, maxSessionNum, serviceType, flag);
}

// drvHdcClientDestroy
drvError_t DriverPlugin::MsprofDrvHdcClientDestroy(HDC_CLIENT client)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginManager_.HasLoad()) {
        ret = pluginManager_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return DRV_ERROR_INVALID_HANDLE;
        }
    }
    MSPROF_DRVHDCCLIENTDESTROY_T func;
    ret = pluginManager_.GetFunction<drvError_t, HDC_CLIENT>("drvHdcClientDestroy", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return DRV_ERROR_INVALID_HANDLE;
    }
    return func(client);
}

// drvHdcServerCreate
drvError_t DriverPlugin::MsprofDrvHdcServerCreate(int devid, int serviceType, HDC_SERVER *pServer)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginManager_.HasLoad()) {
        ret = pluginManager_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return DRV_ERROR_INVALID_HANDLE;
        }
    }
    MSPROF_DRVHDCSERVERCREATE_T func;
    ret = pluginManager_.GetFunction<drvError_t, int, int, HDC_SERVER *>("drvHdcServerCreate", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return DRV_ERROR_INVALID_HANDLE;
    }
    return func(devid, serviceType, pServer);
}

// drvHdcServerDestroy
drvError_t DriverPlugin::MsprofDrvHdcServerDestroy(HDC_SERVER server)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginManager_.HasLoad()) {
        ret = pluginManager_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return DRV_ERROR_INVALID_HANDLE;
        }
    }
    MSPROF_DRVHDCSERVERDESTROY_T func;
    ret = pluginManager_.GetFunction<drvError_t, HDC_SERVER>("drvHdcServerDestroy", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return DRV_ERROR_INVALID_HANDLE;
    }
    return func(server);
}

// drvHdcSessionAccept
drvError_t DriverPlugin::MsprofDrvHdcSessionAccept(HDC_SERVER server, HDC_SESSION *session)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginManager_.HasLoad()) {
        ret = pluginManager_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return DRV_ERROR_INVALID_HANDLE;
        }
    }
    MSPROF_DRVHDCSESSIONACCEPT_T func;
    ret = pluginManager_.GetFunction<drvError_t, HDC_SERVER, HDC_SESSION *>("drvHdcSessionAccept", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return DRV_ERROR_INVALID_HANDLE;
    }
    return func(server, session);
}

// drvHdcGetMsgBuffer
drvError_t DriverPlugin::MsprofDrvHdcGetMsgBuffer(struct drvHdcMsg *msg, int index, char **pBuf, int *pLen)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginManager_.HasLoad()) {
        ret = pluginManager_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return DRV_ERROR_INVALID_HANDLE;
        }
    }
    MSPROF_DRVHDCGETMSGBUFFER_T func;
    ret = pluginManager_.GetFunction<drvError_t, struct drvHdcMsg *, int, char **, int *>("drvHdcGetMsgBuffer", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return DRV_ERROR_INVALID_HANDLE;
    }
    return func(msg, index, pBuf, pLen);
}

// drvHdcAllocMsg
drvError_t DriverPlugin::MsprofDrvHdcAllocMsg(HDC_SESSION session, struct drvHdcMsg **ppMsg, int count)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginManager_.HasLoad()) {
        ret = pluginManager_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return DRV_ERROR_INVALID_HANDLE;
        }
    }
    MSPROF_DRVHDCALLOCMSG_T func;
    ret = pluginManager_.GetFunction<drvError_t, HDC_SESSION, struct drvHdcMsg **, int>("drvHdcAllocMsg", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return DRV_ERROR_INVALID_HANDLE;
    }
    return func(session, ppMsg, count);
}

// drvHdcFreeMsg
drvError_t DriverPlugin::MsprofDrvHdcFreeMsg(struct drvHdcMsg *msg)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginManager_.HasLoad()) {
        ret = pluginManager_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return DRV_ERROR_INVALID_HANDLE;
        }
    }
    MSPROF_DRVHDCFREEMSG_T func;
    ret = pluginManager_.GetFunction<drvError_t, struct drvHdcMsg *>("drvHdcFreeMsg", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return DRV_ERROR_INVALID_HANDLE;
    }
    return func(msg);
}

// drvHdcReuseMsg
drvError_t DriverPlugin::MsprofDrvHdcReuseMsg(struct drvHdcMsg *msg)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginManager_.HasLoad()) {
        ret = pluginManager_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return DRV_ERROR_INVALID_HANDLE;
        }
    }
    MSPROF_DRVHDCREUSEMSG_T func;
    ret = pluginManager_.GetFunction<drvError_t, struct drvHdcMsg *>("drvHdcReuseMsg", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return DRV_ERROR_INVALID_HANDLE;
    }
    return func(msg);
}

// drvHdcAddMsgBuffer
drvError_t DriverPlugin::MsprofDrvHdcAddMsgBuffer(struct drvHdcMsg *msg, char *pBuf, int len)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginManager_.HasLoad()) {
        ret = pluginManager_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return DRV_ERROR_INVALID_HANDLE;
        }
    }
    MSPROF_DRVHDCADDMSGBUFFER_T func;
    ret = pluginManager_.GetFunction<drvError_t, struct drvHdcMsg *, char *, int>("drvHdcAddMsgBuffer", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return DRV_ERROR_INVALID_HANDLE;
    }
    return func(msg, pBuf, len);
}

// drvHdcSessionConnect
drvError_t DriverPlugin::MsprofDrvHdcSessionConnect(int peer_node, int peer_devid, HDC_CLIENT client, HDC_SESSION *session)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginManager_.HasLoad()) {
        ret = pluginManager_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return DRV_ERROR_INVALID_HANDLE;
        }
    }
    MSPROF_DRVHDCSESSIONCONNECT_T func;
    ret = pluginManager_.GetFunction<drvError_t, int, int, HDC_CLIENT, HDC_SESSION *>("drvHdcSessionConnect", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return DRV_ERROR_INVALID_HANDLE;
    }
    return func(peer_node, peer_devid, client, session);
}

// drvHdcSessionClose
drvError_t DriverPlugin::MsprofDrvHdcSessionClose(HDC_SESSION session)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginManager_.HasLoad()) {
        ret = pluginManager_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return DRV_ERROR_INVALID_HANDLE;
        }
    }
    MSPROF_DRVHDCSESSIONCLOSE_T func;
    ret = pluginManager_.GetFunction<drvError_t, HDC_SESSION>("drvHdcSessionClose", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return DRV_ERROR_INVALID_HANDLE;
    }
    return func(session);
}

// drvHdcGetCapacity
drvError_t DriverPlugin::MsprofDrvHdcGetCapacity(struct drvHdcCapacity *capacity)
{
    Status ret = PLUGIN_LOAD_SUCCESS;
    if (!pluginManager_.HasLoad()) {
        ret = pluginManager_.OpenPlugin("LD_LIBRARY_PATH");
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return DRV_ERROR_INVALID_HANDLE;
        }
    }
    MSPROF_DRVHDCGETCAPACITY_T func;
    ret = pluginManager_.GetFunction<drvError_t, struct drvHdcCapacity *>("drvHdcGetCapacity", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return DRV_ERROR_INVALID_HANDLE;
    }
    return func(capacity);
}

} // Plugin
} // Dvvp
} // Analysis