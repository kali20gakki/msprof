/**
 * @operate_loglevel.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef OPERATE_LOGLEVEL_H
#define OPERATE_LOGLEVEL_H
#include <stdbool.h>

#include "constant.h"
#include "msg_queue.h"
#include "slog.h"

#define LEVEL_ERR (-1) // level setting failed
#define SET_LOG_LEVEL_STR "SetLogLevel"
#define GET_LOG_LEVEL_STR "GetLogLevel"
#define EVENT_ENABLE "ENABLE"
#define EVENT_DISABLE "DISABLE"
#define MAX_LEVEL_STR_LEN 2048
#define LOG_EVENT_WRAP_NUM 4
#define LEVEL_PARSE_STEP_SIZE 2

#ifdef __cplusplus
extern "C" {
#endif
typedef struct tagFileDataBuf {
    int len;
    char *data;
} FileDataBuf;

typedef struct tagLogLevel {
    int globalLevel;
    int moduleLevel[INVLID_MOUDLE_ID];
    char enableEvent;
} LogLevel;

typedef struct tagLogLevelInfo {
    LogLevel *logLevel;
    int logLevelType;
    char logLevelResult[MSG_MAX_LEN + 1];
} LogLevelInfo;

int ThreadInit(void);
void *OperateLogLevel(ArgPtr arg);
void InitModuleLevel(int *modLevel, int maxId);
void HandleLogLevelChange(void);
int InitModuleArrToShMem(void);

#ifdef __cplusplus
}
#endif
#endif /* OPERATE_LOGLEVEL_H */

