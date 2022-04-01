/**
 * @start_single_process.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef START_SINGLE_PROCESS_H
#define START_SINGLE_PROCESS_H 1
#include "log_sys_package.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ARRAY_LENGTH 10

typedef struct {
    int fd;
    int cmd;
    int type;
    off_t offset;
    int whence;
    off_t len;
} LockRegParams;
// File lock, Just start a process
int JustStartAProcess(const char *file);
// File lock, end
void SingleResourceCleanup(const char *file);

#ifdef __cplusplus
}
#endif

#endif