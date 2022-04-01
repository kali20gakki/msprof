/**
 * @process_log.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef PROCESS_LOG_H
#define PROCESS_LOG_H

typedef struct {
    int timeout;
    char data[0];
} LogNotifyMsg;

#ifdef __cplusplus
extern "C" {
#endif

#if (OS_TYPE_DEF == 1)
int ProcessLogInit(void);
int ProcessLogFree(void);
#endif

#ifdef HAL_REGISTER_ALOG
typedef void (*HalCallback) (int moduleId, int level, const char *fmt, ...);
void RegisterHalCallback(void (*callback)(int moduleId, int level, const char *fmt, ...));
#endif

#ifdef __cplusplus
}
#endif
#endif
