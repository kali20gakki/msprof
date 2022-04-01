/**
 * @constant.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef CONSTANT_H
#define CONSTANT_H

#ifdef __cplusplus
extern "C" {
#endif

// log level
enum { // use as constant
    SLOG_LEVEL_DEBUG,
    SLOG_LEVEL_INFO,
    SLOG_LEVEL_WARN,
    SLOG_LEVEL_ERROR,
    SLOG_LEVEL_NULL,
    SLOG_INVALID_LEVEL
};

// level value
enum {
    LOG_MIN_LEVEL = SLOG_LEVEL_DEBUG,
    LOG_MAX_LEVEL = SLOG_LEVEL_NULL
};

// log level type
typedef enum {
    LOGLEVEL_GLOBAL = 0,
    LOGLEVEL_MODULE,
    LOGLEVEL_EVENT
} LogLevelType;

// level type
enum {
    GLOBALLEVEL = 0,
    EVENTLEVEL,
    MODULELEVEL
};

#define LEVEL_NOTIFY_FILE "level_notify"
#define MAX_MOD_NAME_LEN 32 // the max length of module name

// log iam
#define IAM_RETRY_TIMES 3
#define IAM_SERVICE_NAME_MAX_LENGTH 32
#define LOGOUT_IAM_SERVICE_PATH "dp:/res/logmgr/logout"
#define CMD_FLUSH_LOG 0x01
#define CMD_GET_LEVEL 0x03
#define MODULE_ID_MASK 0x0000FFFF
#define LOG_TYPE_MASK 0xFFFF0000

// common value
#define COMMON_BUFSIZE 1024
#define ONE_SECOND 1000 // 1 second
#define TEN_MILLISECOND 10 // 10 milliseconds
#define BASE_NUM 10

#ifdef __cplusplus
}
#endif
#endif