/**
 * @file ide_common_util.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#ifndef IDE_DAEMON_PROCESS_UTIL_H
#define IDE_DAEMON_PROCESS_UTIL_H
#include <stdio.h>
#include "common/extra_config.h"
#include "mmpa_api.h"

#define IDE_CTRL_WAIT_FAILED(value, action, logText, ...) do {         \
    if (mmSemWait(value) != EN_OK && mmGetErrorCode() != EINTR) {      \
        IDE_LOGE(logText, ##__VA_ARGS__);                              \
        action;                                                        \
    }                                                                  \
} while (mmGetErrorCode() == EINTR)

#if defined(__IDE_UT) || defined(__IDE_ST)
#define IDE_EXIT(value)
#define IDE_CONSTRUCTOR
#define IDE_DESTRUCTOR
#define STATIC
#else
#define IDE_EXIT(value)      (exit(value))
#define IDE_CONSTRUCTOR __attribute__((constructor))
#define IDE_DESTRUCTOR __attribute__((destructor))
#define STATIC static
#endif

#define IDE_FREE_REQ_AND_SET_NULL(req) do {                     \
    IdeReqFree(req);                                            \
    req = nullptr;                                              \
} while (0)

namespace Adx {
int IdeCheckPopenArgs(IdeString type, size_t len);
FILE *Popen(IdeString command, IdeString type);
int Pclose(FILE* fp);
int InitChildPid();
FILE *PopenCommon(IdeString command, IdeStringBuffer env[], IdeString type, IdePidPtr processId);
#if (OS_TYPE != WIN)
void PopenClean();
uint32_t OpenMax();
#endif
}
#endif