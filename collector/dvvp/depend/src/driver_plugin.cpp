#include "driver_plugin.h"

namespace Analysis {
namespace Dvvp {
namespace Plugin {

// halHdcRecv
hdcError_t DriverPlugin::MsprofHalHdcRecv(HDC_SESSION session, struct drvHdcMsg *pMsg, int bufLen,
    UINT64 flag, int *recvBufCount, UINT32 timeout)
{
    MSPROF_HALHDCRECV_T func;
    Status ret = pluginManager_.GetFunction<HDC_SESSION, struct drvHdcMsg *, int, UINT64
                                            int *, UINT32>("halHdcRecv", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(session, pMsg, bufLen, flag, recvBufCount, timeout);
}

// halHdcSend
hdcError_t MsprofHalHdcSend(HDC_SESSION session, struct drvHdcMsg *pMsg, UINT64 flag, UINT32 timeout)
{
    MSPROF_HALHDCSEND_T func;
    Status ret = pluginManager_.GetFunction<HDC_SESSION, struct drvHdcMsg *, UINT64, UINT32>("halHdcSend", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(session, pMsg, flag, timeout);
}

// halHdcSessionConnectEx
hdcError_t MsprofHalHdcSessionConnectEx(int peer_node, int peer_devid, int peer_pid, HDC_CLIENT client,
    HDC_SESSION *pSession)
{
    MSPROF_HALHDCSESSIONCONNECTEX_T func;
    Status ret = pluginManager_.GetFunction<int, int, int, HDC_CLIENT, HDC_SESSION *>("halHdcSessionConnectEx", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(peer_node, peer_devid, peer_pid, client);
}

// drvHdcSetSessionReference
drvError_t MsprofDrvHdcSetSessionReference(HDC_SESSION session)
{
    MSPROF_DRVHDCSETSESSIONREFERENCE_T func;
    Status ret = pluginManager_.GetFunction<HDC_SESSION>("drvHdcSetSessionReference", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(session);
}

// halHdcGetSessionAttr
drvError_t MsprofHalHdcGetSessionAttr(HDC_SESSION session, int attr, int *value)
{
    MSPROF_HALHDCGETSESSIONATTR_T func;
    Status ret = pluginManager_.GetFunction<HDC_SESSION, int, int *>("halHdcGetSessionAttr", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(session, attr, value);
}

// halGetChipInfo
drvError_t MsprofHalGetChipInfo(unsigned int devId, halChipInfo *chipInfo)
{
    MSPROF_HALGETCHIPINFO_T func;
    Status ret = pluginManager_.GetFunction<unsigned int, halChipInfo *>("halGetChipInfo", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(devId, chipInfo);
}

// halGetDeviceInfo
drvError_t MsprofHalGetDeviceInfo(uint32_t devId, int32_t moduleType, int32_t infoType, int64_t *value)
{
    MSPROF_HALGETDEVICEINFO_T func;
    Status ret = pluginManager_.GetFunction<uint32_t, int32_t, int32_t, int64_t *>("halGetDeviceInfo", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(devId, moduleType, infoType, value);
}

// halProfDataFlush
int MsprofHalProfDataFlush(unsigned int device_id, unsigned int channel_id, unsigned int *data_len)
{
    MSPROF_HALPROFDATAFLUSH_T func;
    Status ret = pluginManager_.GetFunction<unsigned int, unsigned int, unsigned int *>("halProfDataFlush", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(device_id, channel_id, data_len);
}

// prof_drv_get_channels
int MsprofDrvGetChannels(unsigned int device_id, channel_list_t *channels)
{
    MSPROF_PROFDRVGETCHANNELS_T func;
    Status ret = pluginManager_.GetFunction<unsigned int, channel_list_t *>("prof_drv_get_channels", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(device_id, channels);
}

// prof_drv_start
int MsprofDrvStart(unsigned int device_id, unsigned int channel_id, struct prof_start_para *start_para)
{
    MSPROF_PROFDRVSRART_T func;
    Status ret = pluginManager_.GetFunction<unsigned int, unsigned int, struct prof_start_para *>("prof_drv_start", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(device_id, channel_id, start_para);
}

// prof_stop
int MsprofDrvStop(unsigned int device_id, unsigned int channel_id)
{
    MSPROF_PROFSTOP_T func;
    Status ret = pluginManager_.GetFunction<unsigned int, unsigned int>("prof_stop", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(device_id, channel_id);
}

// prof_channel_read
int MsprofChannelRead(unsigned int device_id, unsigned int channel_id, char *out_buf, unsigned int buf_size)
{
    MSPROF_CHANNELREAD_T func;
    Status ret = pluginManager_.GetFunction<unsigned int, unsigned int, char *, unsigned int>("prof_channel_read", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(device_id, channel_id, out_buf, buf_size);
}

// prof_channel_poll
int MsprofChannelPoll(struct prof_poll_info *out_buf, int num, int timeout)
{
    MSPROF_CHANNELPOLL_T func;
    Status ret = pluginManager_.GetFunction<struct prof_poll_info *, int, int>("prof_channel_poll", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(out_buf, num, timeout);
}



} // Plugin
} // Dvvp
} // Analysis