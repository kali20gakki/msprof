#ifndef DRIVER_PLUGIN_H
#define DRIVER_PLUGIN_H

#include "singleton/singleton.h"
#include "ascend_hal.h"
#include "plugin_manager.h"

/* Msprof调用 ascend_hal.so 使用的函数原型  
*  halHdcRecv、halHdcSend、halHdcSessionConnectEx、drvHdcSetSessionReference、halHdcGetSessionAttr
*  halGetChipInfo、halGetDeviceInfo、halProfDataFlush、
*  prof_drv_get_channels、prof_drv_start、prof_stop、prof_channel_read、prof_channel_poll、
*/
namespace Analysis {
namespace Dvvp {
namespace Plugin {

class DriverPlugin : public analysis::dvvp::common::singleton::Singleton<DriverPlugin> {
public:
    DriverPlugin()
     :load_(false),
     :soName_("libascend_hal.so"),
      pluginManager_(PluginManager(soName_))
    {}
    ~DriverPlugin() = default;

    // halHdcRecv
    using MSPROF_HALHDCRECV_T = hdcError_t (*)(HDC_SESSION, struct drvHdcMsg *, int, UINT64, int *, UINT32);
    hdcError_t MsprofHalHdcRecv(HDC_SESSION session, struct drvHdcMsg *pMsg, int bufLen,
        UINT64 flag, int *recvBufCount, UINT32 timeout);

    // halHdcSend
    using MSPROF_HALHDCSEND_T = hdcError_t (*)(HDC_SESSION, struct drvHdcMsg *, UINT64, UINT32);
    hdcError_t MsprofHalHdcSend(HDC_SESSION session, struct drvHdcMsg *pMsg, UINT64 flag, UINT32 timeout);

    // halHdcSessionConnectEx
    using MSPROF_HALHDCSESSIONCONNECTEX_T = hdcError_t (*)(int, int, int, HDC_CLIENT, HDC_SESSION);
    hdcError_t MsprofHalHdcSessionConnectEx(int peer_node, int peer_devid, int peer_pid, HDC_CLIENT client,
        HDC_SESSION *pSession);

    // drvHdcSetSessionReference
    using MSPROF_DRVHDCSETSESSIONREFERENCE_T = drvError_t (*)(HDC_SESSION); 
    drvError_t MsprofDrvHdcSetSessionReference(HDC_SESSION session);

    // halHdcGetSessionAttr
    using MSPROF_HALHDCGETSESSIONATTR_T = drvError_t (*)(HDC_SESSION, int, int *);
    drvError_t MsprofHalHdcGetSessionAttr(HDC_SESSION session, int attr, int *value);

    // halGetChipInfo
    using MSPROF_HALGETCHIPINFO_T = drvError_t (*)(unsigned int, halChipInfo *);
    drvError_t MsprofHalGetChipInfo(unsigned int devId, halChipInfo *chipInfo);

    // halGetDeviceInfo
    using MSPROF_HALGETDEVICEINFO_T = drvError_t (*)(uint32_t, int32_t, int32_t, int64_t *);
    drvError_t MsprofHalGetDeviceInfo(uint32_t devId, int32_t moduleType, int32_t infoType, int64_t *value);

    // halProfDataFlush
    using MSPROF_HALPROFDATAFLUSH_T = int (*)(unsigned int, unsigned int, unsigned int *);
    int MsprofHalProfDataFlush(unsigned int device_id, unsigned int channel_id, unsigned int *data_len);

    // prof_drv_get_channels
    using MSPROF_PROFDRVGETCHANNELS_T = int (*)(unsigned int, channel_list_t *);
    int MsprofDrvGetChannels(unsigned int device_id, channel_list_t *channels);

    // prof_drv_start
    using MSPROF_PROFDRVSRART_T = int (*)(unsigned int, unsigned int, struct prof_start_para *); 
    int MsprofDrvStart(unsigned int device_id, unsigned int channel_id, struct prof_start_para *start_para);

    // prof_stop
    using MSPROF_PROFSTOP_T = int (*)(unsigned int, unsigned int);
    int MsprofDrvStop(unsigned int device_id, unsigned int channel_id);

    // prof_channel_read
    using MSPROF_CHANNELREAD_T = int (*)(unsigned int, unsigned int, char *, unsigned int);
    int MsprofChannelRead(unsigned int device_id, unsigned int channel_id, char *out_buf, unsigned int buf_size);

    // prof_channel_poll
    using MSPROF_CHANNELPOLL_T = int (*)(struct prof_poll_info *, int, int); 
    int MsprofChannelPoll(struct prof_poll_info *out_buf, int num, int timeout);

private:
    bool load_;
    std::string soName_;
    PluginManager pluginManager_;
};

} // Plugin
} // Dvvp
} // Analysis

#endif