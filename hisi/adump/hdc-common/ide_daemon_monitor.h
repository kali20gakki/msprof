/**
 * @file ide_daemon_monitor.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef IDE_DAEMON_MONITOR_H
#define IDE_DAEMON_MONITOR_H
#include "ide_process_util.h"
int IdeMonitorHostInit(void);
int IdeMonitorHostDestroy(void);
#if defined(__IDE_UT) || defined(__IDE_ST)
STATIC bool IdeMonitorIsRun(void);
STATIC bool IdeMonitorIsHeartBeat(void);
STATIC IdeThreadArg IdeMonitorThread(IdeThreadArg args);
extern STATIC struct IdeMonitorMgr g_ideMonitorMgr;
#endif
#endif // PLATFORM_MONITOR