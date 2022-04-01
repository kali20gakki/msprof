/**
 * @msnpureport.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#ifndef MSNPUREPORT_H
#define MSNPUREPORT_H

#include "mmpa_api.h"
#include "securec.h"
#include <stdbool.h>
#include "ascend_hal.h"
#include "adx_log_api.h"
#include "adx_api.h"
#include "constant.h"
#include "log_level_parse.h"

#define SLOG_PATH "slog"
#define STACKCORE_PATH "stackcore"
#define MESSAGE_PATH "message"
#define DIR_MODE 0700
#define TIME_SIZE 128
#define MAX_DEV_NUM 64

#define MIN_USER_ARG_LEN 2
#define DEVICE_LEN 3
#define LOG_TYPE_LEN 1
#define MAX_LOG_LEVEL_CMD_LEN 128 // the max length of : SetLogLevel(1)[MDCPERCEPTION:debug]
/*
the max length of LOG_LEVEL_RESULT:
[global]
WARNING
[event]
DISABLE
[module]
SLOG:WARNING IDEDD:WARNING IDEDH:WARNING HCCL:WARNING
FMK:WARNING HIAIENGINE:WARNING DVPP:WARNING RUNTIME:WARNING
...
*/
#define MAX_LOG_LEVEL_RESULT_LEN 1024
#define MAX_MOD_NAME_LEN 32    // the max length of module names
#define MAX_LEVEL_STR_LEN 64  // the max length of module name + level: error, info, warning, debug, null,
#define BLOCK_RETURN_CODE 4

#define SET_LOG_LEVEL_STR "SetLogLevel"
#define GET_LOG_LEVEL_STR "GetLogLevel"
#define EVENT_ENABLE "ENABLE"
#define EVENT_DISABLE "DISABLE"

#ifdef _LOG_UT_
#define MAIN test
#define STATIC
#else
#define MAIN main
#define STATIC static
#endif

#ifndef WIN
#define WIN 1
#endif

#ifndef LINUX
#define LINUX 0
#endif

#ifndef OS_TYPE_DEF
#define OS_TYPE_DEF LINUX
#endif


#define CHK_NULL_PTR(PTR, ACTION) do { \
    if ((PTR) == NULL) { \
        printf("Invalid ptr parameter [" #PTR "](NULL).\n"); \
        ACTION; \
    } \
} while (0)

#define CHK_EXPR_CTRL(EXPR, ACTION, FORMAT, ...) do { \
    if (EXPR) { \
        printf(FORMAT, ##__VA_ARGS__); \
        ACTION; \
    } \
} while (0)

#define CHK_EXPR_ACT(EXPR, ACTION) do { \
    if (EXPR) { \
        ACTION; \
    } \
} while (0)

typedef enum {
    RPT_ARGS_GLOABLE_LEVEL = 'g',
    RPT_ARGS_MODULE_LEVEL = 'm',
    RPT_ARGS_EVENT_LEVEL = 'e',
    RPT_ARGS_LOG_TYPE = 't',
    RPT_ARGS_DEVICE_ID = 'd',
    RPT_ARGS_REQUEST_LOG_LEVEL = 'r',
    RPT_ARGS_EXPORT_BBOX_LOGS_DEVICE_EVENT = 'a',
    RPT_ARGS_EXPORT_BBOX_LOGS_DEVICE_EVENT_MAINTENANCE_INFORMATION = 'f',
    RPT_ARGS_HELP = 'h'
} UserArgsType;

typedef struct {
    char arg;
    int index;
} MsnArg;

typedef struct {
    uint16_t deviceId;
    int deviceIdFlag;
    char level[MAX_LEVEL_STR_LEN];
    int logType;
    LogLevelType logLevelType;
    LogOperatonType logOperationType;
} ArgInfo;

typedef struct {
    int logType;
    long devOsId;
} ThreadArgInfo;

typedef enum {
    ALL_LOG,
    SLOG_LOG, // contain message
    HISILOGS_LOG,
    STACKCORE_LOG
} MsnLogType;

struct BboxDumpOpt {
    bool all;
    bool force;
};

#define DEFAULT_ARG_TYPES "g:m:e:t:d:rafh"

extern int BboxStartDump(const char *path, int pSize, const struct BboxDumpOpt *opt);
extern void BboxStopDump(void);

STATIC int GetOptions(int argc, char **argv, ArgInfo *opts);
STATIC int RequestHandle(const ArgInfo *argInfo, struct BboxDumpOpt *opt);
STATIC int CheckGlobalLevel(char *levelStr, size_t len);
int SyncDeviceLog(struct BboxDumpOpt *opt, int logType);
int InitArgInfoLevelByNoParameterOption(ArgInfo *opts);
#endif
