#include "driver_plugin.h"
 
namespace Collector {
namespace Dvvp {
namespace Plugin {
bool DriverPlugin::IsFuncExist(const std::string &funcName) const
{
    return true;
}
 
hdcError_t DriverPlugin::MsprofHalHdcRecv(HDC_SESSION session, struct drvHdcMsg *pMsg, int bufLen,
    UINT64 flag, int *recvBufCount, UINT32 timeout)
{
    *recvBufCount = 1;
    return DRV_ERROR_NONE;
}
 
hdcError_t DriverPlugin::MsprofHalHdcSend(HDC_SESSION session, struct drvHdcMsg *pMsg, UINT64 flag, UINT32 timeout)
{
    return DRV_ERROR_NONE;
}
 
hdcError_t DriverPlugin::MsprofHalHdcSessionConnectEx(int peer_node, int peer_devid, int peer_pid, HDC_CLIENT client,
    HDC_SESSION *pSession)
{
    *pSession = (HDC_SESSION)0x123245678;
    return DRV_ERROR_NONE;
}
 
drvError_t DriverPlugin::MsprofDrvHdcSetSessionReference(HDC_SESSION session)
{
    return DRV_ERROR_NONE;
}
 
drvError_t DriverPlugin::MsprofHalHdcGetSessionAttr(HDC_SESSION session, int attr, int *value)
{
    if (attr == HDC_SESSION_ATTR_DEV_ID) {
        *value = 0;
    } else if (attr == HDC_SESSION_ATTR_RUN_ENV) {
        *value = RUN_ENV_PHYSICAL;
    } else {
        *value = 0;
    }
    return DRV_ERROR_NONE;
}
 
drvError_t DriverPlugin::MsprofHalGetChipInfo(unsigned int devId, halChipInfo *chipInfo)
{
    return DRV_ERROR_NONE;
}
 
drvError_t DriverPlugin::MsprofHalGetDeviceInfo(uint32_t devId, int32_t moduleType, int32_t infoType, int64_t *value)
{
    return DRV_ERROR_NONE;
}
 
int DriverPlugin::MsprofHalProfDataFlush(unsigned int device_id, unsigned int channel_id, unsigned int *data_len)
{
    return 0;
}
 
int DriverPlugin::MsprofDrvGetChannels(unsigned int device_id, channel_list_t *channels)
{
    channels->chip_type = 1910;
    channels->channel_num = PROF_CHANNEL_NUM_MAX;
    for (int i = 0; i < PROF_CHANNEL_NUM_MAX; i++) {
        channel_info info = {0};
        info.channel_id = i;
        channels->channel[i] = info;
    }
    return PROF_OK;
}
 
int DriverPlugin::MsprofDrvStart(unsigned int device_id, unsigned int channel_id, struct prof_start_para *start_para)
{
    return 0;
}
 
int DriverPlugin::MsprofDrvStop(unsigned int device_id, unsigned int channel_id)
{
    return 0;
}
 
int DriverPlugin::MsprofChannelRead(unsigned int device_id, unsigned int channel_id, char *out_buf, unsigned int buf_size)
{
    return PROF_OK;
}
 
int DriverPlugin::MsprofChannelPoll(struct prof_poll_info *out_buf, int num, int timeout)
{
    return 0;
}
 
drvError_t DriverPlugin::MsprofDrvGetDevNum(uint32_t *num_dev)
{
    *num_dev = 1;
    return DRV_ERROR_NONE;
}
 
drvError_t DriverPlugin::MsprofDrvGetDevIDByLocalDevID(uint32_t localDevId, uint32_t *devId)
{
    *devId = localDevId;
    return DRV_ERROR_NONE;
}
 
drvError_t DriverPlugin::MsprofDrvGetDevIDs(uint32_t *devices, uint32_t len)
{
    devices[0] = 0;
    return DRV_ERROR_NONE;
}
 
drvError_t DriverPlugin::MsprofDrvGetPlatformInfo(uint32_t *info)
{
    if (info) {
        *info = 1;
    }
    return DRV_ERROR_NONE;
}
 
drvError_t DriverPlugin::MsprofDrvHdcClientCreate(HDC_CLIENT *client, int maxSessionNum, int serviceType, int flag)
{
    *client = (HDC_CLIENT)0x78563421;
    return DRV_ERROR_NONE;
}
 
drvError_t DriverPlugin::MsprofDrvHdcClientDestroy(HDC_CLIENT client)
{
    return DRV_ERROR_NONE;
}
 
drvError_t DriverPlugin::MsprofDrvHdcServerCreate(int devid, int serviceType, HDC_SERVER *pServer)
{
    *pServer = (HDC_SERVER)0x123245678;
    return DRV_ERROR_NONE;
}
 
drvError_t DriverPlugin::MsprofDrvHdcServerDestroy(HDC_SERVER server)
{
    return DRV_ERROR_NONE;
}
 
drvError_t DriverPlugin::MsprofDrvHdcSessionAccept(HDC_SERVER server, HDC_SESSION *session)
{
    *session = (HDC_SESSION)0x123456789;
    return DRV_ERROR_NONE;
}
 
drvError_t DriverPlugin::MsprofDrvHdcGetMsgBuffer(struct drvHdcMsg *msg, int index, char **pBuf, int *pLen)
{
    return DRV_ERROR_NONE;
}
 
drvError_t DriverPlugin::MsprofDrvHdcAllocMsg(HDC_SESSION session, struct drvHdcMsg **ppMsg, int count)
{
    return DRV_ERROR_NONE;
}
 
drvError_t DriverPlugin::MsprofDrvHdcFreeMsg(struct drvHdcMsg *msg)
{
    return DRV_ERROR_NONE;
}
 
drvError_t DriverPlugin::MsprofDrvHdcReuseMsg(struct drvHdcMsg *msg)
{
    return DRV_ERROR_NONE;
}
 
drvError_t DriverPlugin::MsprofDrvHdcAddMsgBuffer(struct drvHdcMsg *msg, char *pBuf, int len)
{
    return DRV_ERROR_NONE;
}
 
drvError_t DriverPlugin::MsprofDrvHdcSessionConnect(int peer_node, int peer_devid, HDC_CLIENT client, HDC_SESSION *session)
{
    *session = (HDC_SESSION)0x123245678;
    return DRV_ERROR_NONE;
}
 
drvError_t DriverPlugin::MsprofDrvHdcSessionClose(HDC_SESSION session)
{
    return DRV_ERROR_NONE;
}
 
drvError_t DriverPlugin::MsprofDrvHdcGetCapacity(struct drvHdcCapacity *capacity)
{
    capacity->maxSegment = 32;
    return DRV_ERROR_NONE;
}
} // Plugin
} // Dvvp
} // Analysis