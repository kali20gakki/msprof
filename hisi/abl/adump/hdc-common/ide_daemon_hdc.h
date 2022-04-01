/**
 * @file ide_daemon_hdc.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef IDE_DAEMON_HDC_H
#define IDE_DAEMON_HDC_H
#include <map>
#include "ide_common_util.h"
#include "ide_platform_util.h"
#include "ascend_hal.h"

struct IdeDevStateInfo {
    uint32_t idx;
#if(OS_TYPE == WIN)
    struct drvDevStateInfo *stateInfo;
#else
    devdrv_state_info_t* stateInfo;
#endif
};

/* hdc server state and param */
struct IdeDevInfo {
    mmThread tid;
    uint32_t phyDevId;
    uint32_t logDevId;
    bool createHdc;
    enum drvHdcServiceType serviceType;
    HDC_SERVER server;
    bool devDisable;
};

struct DeviceUpCallBack {
    std::vector <drvDeviceStartupNotify> upCallbacks;
};

struct IdeGlobalCtrlInfo {
    HDC_CLIENT hdcClient;                        // HDC client
    DeviceUpCallBack deviceNotifyCallbacks;      // Device Notify Callback functions
    std::map<int, struct IdeDevInfo> mapDevInfo; // Device Info
    mmMutex_t mtx;                               // Mutex for struct
    mmSem_t devNotifySem;                        // Number of devices change semaphore
    bool devMapInfoInitFlag;                      // Mutex init flag
    bool hdcServerProcFlag;                       // HDC server process running flag
    bool hdcHandleEventFlag;                      // HDC handle process running flag
    bool initFlag;                                // Init flag
};

struct DevSession {
    uint32_t phyDevId;
    uint32_t logDevId;
    HDC_SESSION session;
    void *mmUserBlockAddr;
};

extern IdeThreadArg HdcCreateHdcServerProc(IdeThreadArg args);
extern int HdcDaemonInit();
extern int HdcDaemonDestroy();
extern int HdcDaemonServerRegister(uint32_t num, const IdeU32Pt dev);
extern IdeThreadArg IdeDaemonHdcProcessEvent(IdeThreadArg arg);
extern IdeThreadArg IdeDaemonHdcHandleEvent(IdeThreadArg args);
#endif /* IDE_COMMON_UTIL_H */
