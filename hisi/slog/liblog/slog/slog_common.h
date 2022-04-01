/**
* @slog_common.h
*
* Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#ifndef SLOG_COMMON_H
#define SLOG_COMMON_H
#include "cfg_file_parse.h"
#include "constant.h"
#include "log_sys_package.h"
#include "log_common_util.h"
#include "share_mem.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef GETCLOCK_VIRTUAL
#define CLOCK_INPUT CLOCK_VIRTUAL
#else
#define CLOCK_INPUT CLOCK_MONOTONIC_RAW
#endif

#define WRITE_E_PRINT_NUM            10000

#define TRUE                         1
#define FALSE                        0
#define INITED                       1
#define NO_INITED                    0
#define FSTAT_E_PRINT_NUM            2000
#define CONN_W_PRINT_NUM             50000
#define EVENT_ENABLE_VALUE           1
#define EVENT_DISABLE_VALUE          0
#define TIME_LOOP_READ_CONFIG        1000
#define TIMESTAMP_LEN                128
#define TIME_ONE_THOUSAND_MS         1000
#define WRITE_TO_CONSOLES            "1"
#define LOG_CTRL_TOTAL_INTERVAL      (12 * 1000) // 12s
#define LOG_WARN_INTERVAL            (4 * 1000) // 4s
#define LOG_INFO_INTERVAL            (8 * 1000) // 8s
#define WRITE_MAX_RETRY_TIMES        3
#define SIZE_TWO_MB                  (2 * 1024 * 1024)  // 2MB
#define LOG_COUNT_NUM                32

typedef struct {
    KeyValue *pstKVArray;
    int kvNum;
} KeyValueArg;

typedef struct {
    int moduleId;
    int level;
    char *timestamp;
    KeyValueArg kvArg;
} LogMsgArg;

typedef enum {
    LOG_REPORT,
    LOG_FLUSH
} CallbackType;

typedef void (*ThreadAtFork)(void);
typedef int (*DlogCallback) (const char *, unsigned int);
typedef int (*DlogFlushCallback) (void);

long TimeDiff(const struct timespec *lastTv);
int GetGlobalLevel(char levelChar);
int GetEventLevel(char levelChar);
int ReadStrFromShm(int shmId, char **str, unsigned int strLen, unsigned int offset);
LogRt AddNewWatch(int *pNotifyFd, int *pWatchFd, const char *notifyFile);
int RegisterCallback(const ArgPtr callback, const CallbackType funcType);

#ifdef __cplusplus
}
#endif
#endif
