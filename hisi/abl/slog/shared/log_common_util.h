/**
 * @log_common_util.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#ifndef LOG_COMMON_UTIL_H
#define LOG_COMMON_UTIL_H
#include "slog.h"
#include "dlog_error_code.h"
#include "log_sys_package.h"
#include "log_level_parse.h"
#ifdef __cplusplus
extern "C" {
#endif

#if (OS_TYPE_DEF == WIN)
#define EOK 0 /* no error */
#define R_OK 0
#define F_OK 0
#define W_OK 0
#define X_OK 0
#endif

#ifndef CLOCK_VIRTUAL
#define CLOCK_VIRTUAL 100
#endif

#ifndef CONF_NAME_MAX_LEN
#define CONF_NAME_MAX_LEN 63
#endif

#ifndef __IDE_UT
#define INLINE inline
#else
#define INLINE
#endif

#ifdef _CLOUD_DOCKER
#define OPERATE_MODE 0770
#else
#define OPERATE_MODE 0750
#endif

#define MAX_DEV_NUM 64
#define INVALID (-1)

#define MAX_INOTIFY_BUFF 1024
#define LOG_MSG_NODE_NUM 1000
#define PLOG_MSG_NODE_NUM 1000
#define CFG_LOGAGENT_PATH_MAX_LENGTH 255
#define LOG_FOR_SELF_MAX_FILE_LENGTH 8
#define LOG_DIR_FOR_SELF_LENGTH (CFG_LOGAGENT_PATH_MAX_LENGTH + LOG_FOR_SELF_MAX_FILE_LENGTH)
#define CFG_WORKSPACE_PATH_MAX_LENGTH 64
#define WORKSPACE_MAX_FILE_LENGTH 16
#define WORKSPACE_PATH_MAX_LENGTH (CFG_WORKSPACE_PATH_MAX_LENGTH + WORKSPACE_MAX_FILE_LENGTH)
#define LOG_CONFIG_FILE "slog.conf"
#define IOCTL_MODULE_NAME "module"

#if (OS_TYPE_DEF == LINUX)
    #define OS_SPLIT '/'
#ifdef IAM
    #define GLOABLE_DEFAULT_LOG_LEVEL (DLOG_ERROR)
    #define MODULE_DEFAULT_LOG_LEVEL (DLOG_ERROR)
    #define LOG_FILE_PATH "/home/mdc/var/log"
#else
    #define GLOABLE_DEFAULT_LOG_LEVEL (DLOG_INFO)
    #define MODULE_DEFAULT_LOG_LEVEL (DLOG_INFO)
    #define LOG_FILE_PATH "/var/log/npu/slog"
#endif
    #define LOG_DIR_FOR_SELF_LOG "/slogd"
    #define SOCKET_FILE "/slog"
    #define SLOGD_PID_FILE "/slogd.pid"
    #define SKLOGD_PID_FILE "/sklogd.pid"
    #define LOGDAEMON_PID_FILE "/log-daemon.pid"
    #define PROCESS_SUB_LOG_PATH "ascend/log"
#else
    #define GLOABLE_DEFAULT_LOG_LEVEL (DLOG_INFO)
    #define MODULE_DEFAULT_LOG_LEVEL (DLOG_INFO)
    #define PROCESS_SUB_LOG_PATH "alog"
    #define LOG_HOME_DIR ("C:\\Program Files\\Huawei\\Ascend")
    #define LOG_FILE_PATH "C:\\Program Files\\Huawei\\Ascend\\slog"
    #define LOG_DIR_FOR_SELF_LOG "\\slogd"
    #define SOCKET_FILE "\\slog"
    #define OS_SPLIT '\\'
#endif

struct Options {
    int n;
    int l;
};

typedef enum {
    DEBUG_LOG = 0,
    SECURITY_LOG,
    RUN_LOG,
    OPERATION_LOG,
    LOG_TYPE_NUM
} LogType;

enum {
    USER_OK = 0,
    USER_NOT_FOUND,
    USER_ERROR,
    USER_SUCCESS
};

typedef struct {
    LogType type;
    int level;
    int moduleId;
    unsigned int msgLength;
    char msg[MSG_LENGTH];
} LogMsg;

typedef struct {
    unsigned int nodeCount;
    LogMsg logMsgList[LOG_MSG_NODE_NUM];
} IamLogBuf;

typedef struct {
    char configName[CONF_NAME_MAX_LEN + 1];
    int configValue[INVLID_MOUDLE_ID];
} LevelConfInfo;

typedef struct {
    int appPid;
} FlushInfo;

#define XFREE(ps)           \
    do {                    \
        if ((ps) != NULL) { \
            free((ps));     \
            (ps) = NULL;    \
        }                   \
    } while (0)

#define LOG_CLOSE_SOCK_S(sock) \
    do {                       \
        (void)ToolCloseSocket((sock)); \
        (sock) = -1;           \
    } while (0)

#define LOG_MMCLOSE_AND_SET_INVALID(fd)   \
    do {                                  \
            (void)ToolClose((fd));              \
            (fd) = -1;                    \
    } while (0)

#define LOG_CLOSE_FILE(fp) \
    do {                   \
        if ((fp) != NULL) {  \
            (void)fclose((fp));    \
            (fp) = NULL;     \
        }                  \
    } while (0)

#if (OS_TYPE_DEF == LINUX)
void ChangePathOwner(const char *path);
#endif

int GetUserGroupId(unsigned int *uid, unsigned int *gid);
int StartsWith(const char *str, const char *pattern);
int LogGetProcessPath(char *processDir, unsigned int len);
int LogGetProcessConfigPath(char *configPath, unsigned int len);
int LogInitWorkspacePath(void);
int LogInitLogAgentPath(void);
int SetWorkspacePath(const char *path);
char *GetWorkspacePath(void);
char *GetRootLogPath(void);
char *GetLogAgentPath(void);
char *GetSocketPath(void);
toolMode SyncGroupToOther(toolMode perm);
INT32 GetLocaltimeR(struct tm *timeInfo, time_t sec);
LogRt MkdirIfNotExist(const char *dirPath);
LogRt LogAgentChangeLogPathMode(const char *path);
LogRt LogAgentChangeLogFdMode(int fd);
int StrcatDir(char *path, const char *filename, const char *dir, unsigned int maxlen);
int LogGetHomeDir(char *const homedir, unsigned int len);
int LogReplaceDefaultByDir(const char *path, char *homeDir, unsigned int len);
unsigned int GetHostDeviceID(unsigned int deviceId);
LogRt GetDevNumIDs(unsigned int *deviceNum, unsigned int *deviceIdArray);
#ifdef __cplusplus
}
#endif
#endif /* LOG_COMMON_UTIL_H */
