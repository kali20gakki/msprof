/**
 * @log_daemon.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef LOG_DAEMON_H
#define LOG_DAEMON_H

#include <signal.h>
#define FIVE_SECOND       5000

#ifdef __cplusplus
extern "C" {
#endif
int GetSigFlag(void);

#if (OS_TYPE_DEF == LINUX)
void BboxStartMainThread(void);
void BboxStopMainThread(void);
#endif

extern int AdxCoreDumpServerInit(void);
#ifdef __cplusplus
}
#endif
#endif
