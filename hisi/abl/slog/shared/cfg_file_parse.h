/**
 * @cfg_file_parse.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef CFG_FILE_PARSE_H
#define CFG_FILE_PARSE_H
#include <stdbool.h>
#include "dlog_error_code.h"
#include "print_log.h"
#include "log_sys_package.h"
#include "log_level_parse.h"

#define CONF_NAME_MAX_LEN 63
#define CONF_VALUE_MAX_LEN 1023 // ide_daemon.cfg confName(STORE) Max Length(1023 bytes)
#define CONF_FILE_MAX_LINE (CONF_NAME_MAX_LEN + CONF_VALUE_MAX_LEN + 1)
#define CFG_FILE_BUFFIX1 ".cfg"
#define CFG_FILE_BUFFIX2 ".conf"
#define CFG_FILE_BUFFIX3 ".info"
#define ENABLEEVENT_KEY "enableEvent"
#define GLOBALLEVEL_KEY "global_level"
#define LOG_WORKSPACE_STR  "logWorkspace"
#define LOG_AGENT_FILE_DIR_STR "logAgentFileDir"
#define SLOG_CONFIG_FILE_LENGTH 16
#define SLOG_CONF_PATH_MAX_LENGTH (TOOL_MAX_PATH + SLOG_CONFIG_FILE_LENGTH)
#define PERMISSION_FOR_ALL "permission_for_all"
#define SLOG_CONFIG_FILE_SIZE (10 * 1024)

#if (OS_TYPE_DEF == LINUX)
    #define DEFAULT_LOG_WORKSPACE "/usr/slog"
#ifdef LINUX_ETC_CONF
    #define SLOG_CONF_FILE_PATH "/etc/slog.conf"
#else
    #define SLOG_CONF_FILE_PATH "/var/log/npu/conf/slog/slog.conf"
#endif
#else
    #define SLOG_CONF_FILE_PATH ("C:\\Program Files\\Huawei\\Ascend\\conf\\slog\\slog.conf")
    #define DEFAULT_LOG_WORKSPACE "C:\\Program Files\\Huawei\\Ascend\\slog"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tagConfList {
    char confName[CONF_NAME_MAX_LEN + 1];
    char confValue[CONF_VALUE_MAX_LEN + 1];
    struct tagConfList *next;
} ConfList;

ConfList *GetGlobalConfList(void);
void FreeConfList(void);
#if (OS_TYPE_DEF == 0)
void FreeConfigShm(void);
int InitShm(void);
int LogSetConfigPathToShm(const char *configPath);
#endif
int InitCfg(int libSlogFlag);
char *GetConfigPath(void);
int SetThreadName(const char *threadName);
// interface open to other log module users
LogRt InitConfList(const char *file);
LogRt UpdateConfList(const char *file);
LogRt GetConfValueByList(const char *confName, unsigned int nameLen, char *confValue, unsigned int valueLen);
char *RealPathCheck(const char *file, const char *homeDir, unsigned int dirLen);
bool IsNaturalNumStr(const char *str);
bool IsPathValidByConfig(const char *ppath, unsigned int pathLen);
bool IsPathValidbyLog(const char *ppath, unsigned int pathLen);
void InitCfgAndSelfLogPath(void);
void FreeCfgAndSelfLogPath(void);

#ifdef __cplusplus
}
#endif
#endif
