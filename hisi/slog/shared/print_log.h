/**
 * @print_log.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef PRINT_LOG_H
#define PRINT_LOG_H
#include "log_sys_package.h"
#include "log_common_util.h"

#define SLOGD_LOG_MAX_SIZE (1 * 1024 * 1024)
#define SLOGD_LOG_BUFFIX_LEN 7
#define GENERAL_PRINT_NUM 200
#define LOG_PRINT_LOG_BUF_SZ 1024
#define LOG_PRINT_LOG_TIME_STR_SIZE 128
#define SELF_MAX_NAME_LENGTH  16
#define SELF_LOG_FILES_LENGTH (LOG_DIR_FOR_SELF_LENGTH + SELF_MAX_NAME_LENGTH)

#if (OS_TYPE_DEF == LINUX)
    #define SLOGD_LOG_FILE              "/slogdlog"
    #define SLOGD_LOG_OLD_FILE          "/slogdlog.old"
    #define SLOGD_LOG_LOCK              "/tmp.lock"
#ifdef IAM
    #define DEFAULT_SLOGD_LOG_FILE      "/home/mdc/var/log/slogd/slogdlog"
    #define DEFAULT_SLOGD_LOG_OLD_FILE  "/home/mdc/var/log/slogd/slogdlog.old"
    #define DEFAULT_SLOGD_LOG_LOCK      "/home/mdc/var/log/slogd/tmp.lock"
#else
    #define DEFAULT_SLOGD_LOG_FILE      "/var/log/npu/slog/slogd/slogdlog"
    #define DEFAULT_SLOGD_LOG_OLD_FILE  "var/log/npu/slog/slogd/slogdlog.old"
    #define DEFAULT_SLOGD_LOG_LOCK      "/var/log/npu/slog/slogd/tmp.lock"
#endif
#elif (OS_TYPE_DEF == WIN)
    #define S_IFLNK 1
    #define SLOGD_LOG_FILE              "\\slogdlog"
    #define SLOGD_LOG_OLD_FILE          "\\slogdlog.old"
    #define SLOGD_LOG_LOCK              "\\tmp.lock"
    #define DEFAULT_SLOGD_LOG_FILE      "C:\\Program Files\\Huawei\\Ascend\\slog\\slogd\\slogdlog"
    #define DEFAULT_SLOGD_LOG_OLD_FILE  "C:\\Program Files\\Huawei\\Ascend\\slog\\slogd\\slogdlog.old"
    #define DEFAULT_SLOGD_LOG_LOCK      "C:\\Program Files\\Huawei\\Ascend\\slog\\slogd\\tmp.lock"
#endif

#ifdef __cplusplus
extern "C" {
#endif

int InitFilePathForSelfLog(void);
int InitOldFilePathForSelfLog(void);
int InitLockFilePathForSelfLog(void);
char* GetSelfLogPath(void);
char* GetSelfLogOldPath(void);
void FreeSelfLogFiles(void);
void PrintSelfLog(const char *format, ...);
void LogSyslog(int priority, const char *format, ...);

#ifdef SORTED_LOG
#define SYSLOG_WARN(format, ...)                                                                           \
    do {                                                                                                   \
            LogSyslog(LOG_WARNING, "%s:%d: " format "\n", __FILE__, __LINE__, ##__VA_ARGS__);              \
    } while (0)
#else
#define SYSLOG_WARN(format, ...)                                                                           \
    do {                                                                                                   \
            printf("[WARNING] " format "\n", ##__VA_ARGS__);                                               \
    } while (0)
#endif

#ifdef WRITE_TO_SYSLOG
#define SELF_LOG_INFO(format, ...)                                                                         \
    do {                                                                                                   \
            LogSyslog(LOG_INFO, "%s:%d: " format "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
    } while (0)

#define SELF_LOG_WARN(format, ...)                                                                            \
    do {                                                                                                      \
            LogSyslog(LOG_WARNING, "%s:%d: " format "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
    } while (0)

#define SELF_LOG_ERROR(format, ...)                                                                         \
    do {                                                                                                    \
            LogSyslog(LOG_ERR, "%s:%d: " format "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
    } while (0)

// Output warn selfLog every N times, ptrX point to x time to print
#define SELF_LOG_WARN_N(ptrX, N, format, ...)                                                                  \
    do {                                                                                                       \
            if (((*ptrX) == 0) || ((*ptrX) >= N)) {                                                                  \
                (*ptrX) = ((*ptrX) != 0) ? 0 : ((*ptrX) + 1);                                                        \
                LogSyslog(LOG_WARNING, "%s:%d: " format "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
            } else {                                                                                           \
                (*ptrX) = (*ptrX) + 1;                                                                             \
            }                                                                                                  \
    } while (0)

// Output error selfLog every N times, ptrX point to x time to print
#define SELF_LOG_ERROR_N(ptrX, N, format, ...)                                                                 \
    do {                                                                                                       \
            if (((*ptrX) == 0) || ((*ptrX) >= N)) {                                                                  \
                (*ptrX) = ((*ptrX) != 0) ? 0 : ((*ptrX) + 1);                                                        \
                LogSyslog(LOG_ERR, "%s:%d: " format "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
            } else {                                                                                           \
                (*ptrX) = (*ptrX) + 1;                                                                             \
            }                                                                                                  \
    } while (0)
#else
#define SELF_LOG_INFO(format, ...)                                                                         \
    do {                                                                                                   \
            PrintSelfLog("[INFO] %s:%d: " format "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
    } while (0)

#define SELF_LOG_WARN(format, ...)                                                                              \
    do {                                                                                                        \
            PrintSelfLog("[WARNING] %s:%d: " format "\n", __FILE__, __LINE__, ##__VA_ARGS__);     \
    } while (0)

#define SELF_LOG_ERROR(format, ...)                                                                             \
    do {                                                                                                        \
            PrintSelfLog("[ERROR] %s:%d: " format "\n", __FILE__, __LINE__, ##__VA_ARGS__);       \
    } while (0)

// Output warn selfLog every N times, ptrX point to x time to print
#define SELF_LOG_WARN_N(ptrX, N, format, ...)                                                                   \
    do {                                                                                                        \
            if (((*ptrX) == 0) || ((*ptrX) >= N)) {                                                                 \
                (*ptrX) = ((*ptrX) != 0) ? 0 : ((*ptrX) + 1);                                                         \
                PrintSelfLog("[WARNING] %s:%d: " format "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
            } else {                                                                                           \
                (*ptrX) = (*ptrX) + 1;                                                                             \
            }                                                                                                  \
    } while (0)

// Output error selfLog every N times, ptrX point to x time to print
#define SELF_LOG_ERROR_N(ptrX, N, format, ...)                                                                  \
    do {                                                                                                        \
            if (((*ptrX) == 0) || ((*ptrX) >= N)) {                                                                 \
                (*ptrX) = ((*ptrX) != 0) ? 0 : ((*ptrX) + 1);                                                         \
                PrintSelfLog("[ERROR] %s:%d: " format "\n", __FILE__, __LINE__, ##__VA_ARGS__);   \
            } else {                                                                                            \
                (*ptrX) = (*ptrX) + 1;                                                                              \
            }                                                                                                   \
    } while (0)
#endif

#define NO_ACT_WARN_LOG(expr, fmt, ...)                         \
    if (expr) {                                                 \
        SELF_LOG_WARN(fmt, ##__VA_ARGS__);        \
    }                                                           \

#define NO_ACT_ERR_LOG(expr, fmt, ...)                          \
    if (expr) {                                                 \
        SELF_LOG_ERROR(fmt, ##__VA_ARGS__);       \
    }                                                           \

// no do-while(0) because action maybe continue or break
#define ONE_ACT_NO_LOG(expr, action)                            \
    if (expr) {                                                 \
        action;                                                 \
    }                                                           \

#define ONE_ACT_INFO_LOG(expr, action, fmt, ...)                \
    if (expr) {                                                 \
        SELF_LOG_INFO(fmt, ##__VA_ARGS__);        \
        action;                                                 \
    }                                                           \

#define ONE_ACT_WARN_LOG(expr, action, fmt, ...)                \
    if (expr) {                                                 \
        SELF_LOG_WARN(fmt, ##__VA_ARGS__);        \
        action;                                                 \
    }                                                           \

#define ONE_ACT_ERR_LOG(expr, action, fmt, ...)                 \
    if (expr) {                                                 \
        SELF_LOG_ERROR(fmt, ##__VA_ARGS__);       \
        action;                                                 \
    }                                                           \

#define TWO_ACT_NO_LOG(expr, action1, action2)                  \
    if (expr) {                                                 \
        action1;                                                \
        action2;                                                \
    }                                                           \

#define TWO_ACT_INFO_LOG(expr, action1, action2, fmt, ...)      \
    if (expr) {                                                 \
        SELF_LOG_INFO(fmt, ##__VA_ARGS__);        \
        action1;                                                \
        action2;                                                \
    }                                                           \

#define TWO_ACT_WARN_LOG(expr, action1, action2, fmt, ...)      \
    if (expr) {                                                 \
        SELF_LOG_WARN(fmt, ##__VA_ARGS__);        \
        action1;                                                \
        action2;                                                \
    }                                                           \

#define TWO_ACT_ERR_LOG(expr, action1, action2, fmt, ...)       \
    if (expr) {                                                 \
        SELF_LOG_ERROR(fmt, ##__VA_ARGS__);       \
        action1;                                                \
        action2;                                                \
    }                                                           \

// prevent self log print if mutex is null by judge return value
#define LOCK_WARN_LOG(mutex) do {                                                   \
    if (ToolMutexLock((mutex)) == SYS_ERROR) {                                      \
        SELF_LOG_WARN("lock fail, strerr=%s", strerror(ToolGetErrorCode()));     \
    }                                                                               \
} while (0)                                                                         \

#define UNLOCK_WARN_LOG(mutex) do {                                                 \
    if (ToolMutexUnLock((mutex)) == SYS_ERROR) {                                    \
        SELF_LOG_WARN("unlock fail, strerr=%s", strerror(ToolGetErrorCode()));   \
    }                                                                               \
} while (0)                                                                         \

#ifdef __cplusplus
}
#endif
#endif  // PRINT_LOG_H
