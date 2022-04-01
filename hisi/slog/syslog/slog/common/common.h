/**
 * @common.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef COMMON_H
#define COMMON_H

#include "constant.h"
#include "log_sys_package.h"
#include "print_log.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int g_gotSignal;

struct Globals;

#define SET_GLOBALS_PTR(x)                                          \
    do {                                                               \
        (*(struct Globals **)&g_globalsPtr) = (struct Globals *)(x); \
        ToolMemBarrier();                                                        \
    } while (0)

#define FREE_GLOBALS_PTR()  \
    do {                       \
        free(g_globalsPtr);  \
        g_globalsPtr = NULL; \
    } while (0)

int DaemonizeProc(void);
void SignalNoSaRestartEmptyMask(int sig, void (*handler)(int));
void RecordSigNo(int sigNo);
int GetSigNo(void);

#ifdef __cplusplus
}
#endif
#endif
