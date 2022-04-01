/**
 * @app_log_dump.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef APP_LOG_WATCH_H
#define APP_LOG_WATCH_H
#include "log_sys_package.h"
#ifdef __cplusplus
extern "C" {
#endif

#define MAX_FILE_NAME_LEN 256

void CreateAppLogWatchThread(void);
void ParseDeviceAppDirNumsCfg(void);

typedef struct {
    ToolStat aStatbuff;
    ToolStat bStatbuff;
    ToolStat aDirStatbuff;
    ToolStat bDirStatbuff;
    ToolDirent **listA;
    ToolDirent **listB;
    char aDirName[MAX_FILE_NAME_LEN];
    char bDirName[MAX_FILE_NAME_LEN];
    char aFileName[MAX_FILE_NAME_LEN];
    char bFileName[MAX_FILE_NAME_LEN];
} SortArg;
#ifdef __cplusplus
}
#endif
#endif /* APP_LOG_WATCH_H */