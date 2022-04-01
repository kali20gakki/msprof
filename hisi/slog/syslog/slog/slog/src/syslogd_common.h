/**
 * @syslogd_common.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef SYSLOGD_COMMON_H
#define SYSLOGD_COMMON_H

#include <stdbool.h>
#include "constant.h"
#include "log_common_util.h"
#include "operate_loglevel.h"
#include "log_to_file.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_READ (1024 + 1)
#define SIZE_SIXTEEN_MB (16 * 1024 * 1024) // 16MB
#define LOG_BUF_SIZE "LogBufSize"
#define DEFAULT_LOG_BUF_SIZE (256 * 1024) // 256KB
#define MIN_LOG_BUF_SIZE (64 * 1024) // 64KB
#define MAX_LOG_BUF_SIZE (1024 * 1024) // 1024KB
#define NO_APP_DATA_MAX_COUNT 180   // log buf node will be released when count come to it
#define MAX_WRITE_WAIT_TIME 3 // log report to host wait max 3 time

struct Globals {
    int moduleId;
    int systemInit;
    char recvBuf[MAX_READ];
    // parseBuf content can be doubled, because escaping control chars
    char parseBuf[MAX_READ + MAX_READ];
};
extern struct Globals *g_globalsPtr;
extern LogLevel g_logLevel;

typedef struct tagLogBufList {
    LogType type;
    unsigned int pid;
    unsigned int deviceId;
    unsigned int noAppDataCount;
    unsigned int writeWaitTime;
    unsigned int len;
    char *data;
    struct tagLogBufList *next;
} LogBufList;

void ParseSyslogdCfg(void);
void ParseLogBufCfg(void);
void ProcEscapeThenLog(const char *tmpbuf, int len, LogType type);
int ProcSyslogBuf(const char* recvBuf, int* size);
UINT8 IsWriteFileThreadExit(void);
void SetWriteFileThreadExit(void);

LogRt InitBufAndFileList(void);
void FreeBufAndFileList(void);
LogRt DeleteAppLogBufNode(LogInfo *info);
void FflushLogDataBuf(void);
void DataBufLock(void);
void DataBufUnLock(void);
StLogFileList* GetGlobalLogFileList(void);
int GetItemValueFromCfg(const char *item, int minItemValue, int maxItemValue, int defaultItemValue);

#ifdef __cplusplus
}
#endif
#endif
