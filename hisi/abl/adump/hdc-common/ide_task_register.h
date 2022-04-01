/**
 * @file ide_task_register.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef IDE_TASK_REGISTER_H
#define IDE_TASK_REGISTER_H
#include "ide_common_util.h"
#include "ide_daemon_hdc.h"
#ifdef IDE_DAEMON_HOST
#include "ide_daemon_host.h"
#endif
namespace Adx {
#ifdef IDE_DAEMON_DEVICE
const char * const IDE_DAEMON_NAME = "adda";
#endif
extern void IdeDaemonRegisterModules();
#if defined(__IDE_UT) || defined(__IDE_ST)
STATIC void IdeRegisterModule(enum IdeComponentType type, const struct IdeSingleComponentFuncs &ideFuncs);
#endif
}
#ifdef __cplusplus
extern "C" {
#endif
extern int32_t IdeHostProfileProcess(IdeSession sockDesc, HDC_CLIENT client, IdeTlvConReq req);
extern int32_t IdeHostProfileInit();
extern int32_t IdeHostProfileCleanup();
extern int32_t IdeDeviceProfileProcess(HDC_SESSION session, IdeTlvConReq req);
extern int32_t IdeDeviceProfileInit();
extern int32_t IdeDeviceProfileCleanup();
extern int32_t IdeHostLogProcess(IdeSession sockDesc, HDC_CLIENT client, IdeTlvConReq req);
extern int32_t IdeDeviceLogProcess(HDC_SESSION session, IdeTlvConReq req);
extern int32_t AdxGetFileServerInit();
#ifdef __cplusplus
}
#endif
#endif
