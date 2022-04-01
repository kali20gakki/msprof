/**
 * @file ide_common_util.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2021. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef IDE_DAEMON_COMMON_UTIL_H
#define IDE_DAEMON_COMMON_UTIL_H
#include <fcntl.h>
#include "common/extra_config.h"
#include "hdc_api.h"
#include "securec.h"
#include "mmpa_api.h"
#include "ide_tlv.h"
#include "log/adx_log.h"
#include "common/memory_utils.h"
#include "ide_process_util.h"
#ifdef __cplusplus
extern "C" {
#endif

#if (OS_TYPE == WIN)
#define IDE_FD_ISSET(d, set)   FD_ISSET(d, set)
#define IDE_FD_SET(d, set)     FD_SET(d, set)
#define EXIT_CODE(exit)        (exit)
#else
#define EXIT_CODE(exit)        (((unsigned int)(exit) & 0x0FF00) >> 8)
#define IDE_FD_ISSET(d, set)   FD_ISSET((d), (set))
#define IDE_FD_SET(d, set)     FD_SET((d), (set))
#endif

enum IdeComponentType {
    IDE_COMPONENT_HOOK_REG = 0,
    IDE_COMPONENT_BBOX, // multiple device should init callback first
    IDE_COMPONENT_HDC,
    IDE_COMPONENT_CMD,
    IDE_COMPONENT_SEND_FILE,
    IDE_COMPONENT_DEBUG,
    IDE_COMPONENT_BBOX_HDC,
    IDE_COMPONENT_LOG,
    IDE_COMPONENT_FILE_SYNC,
    IDE_COMPONENT_API,
    IDE_COMPONENT_PROFILING,
    IDE_COMPONENT_DUMP,
    IDE_COMPONENT_HOST_CMD,
    IDE_COMPONENT_DETECT,
    IDE_COMPONENT_FILE_GET,
    IDE_COMPONENT_NV,
    IDE_COMPONENT_FILE_GETD, // get device file (compare in C3x)
    IDE_COMPONENT_MONITOR,
    NR_IDE_COMPONENTS,
};

struct ComponentMap {
    enum IdeComponentType type;
    CmdClassT cmdType;
    IdeString name;
    IdeString operateType;
};

struct IdeSingleComponentFuncs {
    int (*init)(void);
    int (*destroy)(void);
    int (*sockProcess)(IdeSession sockDesc, HDC_CLIENT client, IdeTlvConReq req);
    int (*hdcProcess)(HDC_SESSION session, IdeTlvConReq req);
};

struct IdeComponentsFuncs {
    int (*init[NR_IDE_COMPONENTS])(void);
    int (*destroy[NR_IDE_COMPONENTS])(void);
    int (*sockProcess[NR_IDE_COMPONENTS])(IdeSession sockDesc, HDC_CLIENT client, IdeTlvConReq req);
    int (*hdcProcess[NR_IDE_COMPONENTS])(HDC_SESSION session, IdeTlvConReq req);
};

extern struct IdeComponentsFuncs       g_ideComponentsFuncs;

extern void IdeReqFree(const IdeTlvReq req);
extern void IdeRegisterSig();

extern IdeString IdeGetCompontName(int type);
extern IdeString IdeGetCompontNameByReq(CmdClassT reqType);
enum IdeComponentType IdeGetComponentType(IdeTlvConReq req);
extern int GenrateCfgFile(void);
extern void RemoveCfgFile(void);

extern int IdeComponentsInit();
extern void IdeComponentsDestroy();

extern int IdeDaemonSubInit();
extern int DaemonInit();
extern void DaemonCloseServerSock();
extern void DaemonDestroy();

extern int HdclogDeviceInit();
extern int HdclogDeviceDestroy();

extern int HdclogHostInit();
extern int HdclogHostDestroy();

#define IDE_CTRL_VALUE_RET_NO_LOG(err, action) do {                   \
    if (err) {                                                        \
        action;                                                       \
    }                                                                 \
} while (0)

#define IDE_CTRL_MUTEX_LOCK(mtx, action) do {                          \
    if (mmMutexLock(mtx) != EN_OK) {                                   \
        IDE_LOGE("mutex lock error");                                  \
        action;                                                        \
    }                                                                  \
} while (0)

#define IDE_CTRL_MUTEX_UNLOCK(mtx, action) do {                        \
    if (mmMutexUnLock(mtx) != EN_OK) {                                 \
        IDE_LOGE("mutex lock error");                                  \
        action;                                                        \
    }                                                                  \
} while (0)

#define IDE_MMCLOSE_AND_SET_INVALID(fd) do {                    \
    if ((fd) >= 0) {                                            \
        (void)mmClose(fd);                                      \
        fd = -1;                                                \
    }                                                           \
} while (0)

#define IDE_MMCLOSEFILE_AND_SET_INVALID(fd) do {                \
    (void)mmCloseFile(fd);                                      \
    fd = -1;                                                    \
} while (0)

#define IDE_CHECK_REMOVE(err, path) do {                        \
    if ((err) != 0) {                                           \
        IDE_LOGD("remove %s exit code %d", path, err);          \
    }                                                           \
} while (0)
#if defined(__IDE_UT) || defined(__IDE_ST)
#define STATIC
#else
#define STATIC static
#endif
#ifdef __cplusplus
}
#endif
#endif // IDE_DAEMON_COMMON_UTIL_H
