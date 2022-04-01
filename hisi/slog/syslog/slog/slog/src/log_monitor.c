/**
 * @log_monitor.c
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#include "log_monitor.h"

#include <stdbool.h>
#include "log_sys_package.h"

#include "cfg_file_parse.h"
#include "log_common_util.h"
#include "print_log.h"

#ifdef _LOG_MONITOR_
#include "appmon_lib.h"

#define HEART_BEART_RECORD 20
#define MONITOR_REGISTER_WAIT_TIME 1000
#define MONITOR_HEARTBEAT_WAIT_TIME 3000
#define MONITOR_INIT_WAIT_TIME 1000
#define MONITOR_TIMEOUT 9000
#define LOG_DEATH_DESC_LENGTH 128
#define NUM_MONITOR_PROC 3
#define NUM_INIT_APPMOND 3
#define LOG_MONITOR_DEFAULT_THREAD_ATTR {1, 0, 0, 0, 0, 1, 128 * 1024}

#ifdef _SLOG_DEVICE_
#define SLOGD_DAEMON_SCRIPT "/var/slogd_daemon_monitor.sh"
#define SKLOGD_DAEMON_SCRIPT "/var/sklogd_daemon_monitor.sh"
#define LOG_DAEMON_SCRIPT "/var/log_daemon_monitor.sh"
#else
#define SLOGD_DAEMON_SCRIPT "driver/tools/slogd_daemon_monitor.sh"
#define SKLOGD_DAEMON_SCRIPT "driver/tools/sklogd_daemon_monitor.sh"
#define LOG_DAEMON_SCRIPT "driver/tools/log_daemon_monitor.sh"
#endif

enum LOG_MONITOR_STATUS {
    LOG_MONITOR_INIT = 0,
    LOG_MONITOR_RUNNING,
    LOG_MONITOR_HEARTBEAT,
    LOG_MONITOR_EXIT,
};

struct LogMonitorMgr {
    int status;
    toolThread tid;
    client_info_t clnt;
};

struct LogMonitorMgr g_logMonitorMgr;
STATIC unsigned int g_flag = 0;
const char *g_procName[NUM_MONITOR_PROC] = {"slogd_monitor", "log_daemon_monitor", "sklogd_monitor"};

STATIC void LogMonitorSetStatus(int status)
{
    g_logMonitorMgr.status = status;
}

STATIC bool LogMonitorIsRun(void)
{
    return (g_logMonitorMgr.status == LOG_MONITOR_RUNNING ||
            g_logMonitorMgr.status == LOG_MONITOR_HEARTBEAT);
}

STATIC bool LogMonitorIsHeartBeat(void)
{
    return (g_logMonitorMgr.status == LOG_MONITOR_HEARTBEAT);
}

STATIC bool LogMonitorIsInit(void)
{
    return (g_logMonitorMgr.status == LOG_MONITOR_INIT);
}

STATIC bool GetLogDaemonScript(char **logDaemonScript)
{
    if (logDaemonScript == NULL) {
        return false;
    }

    if (g_flag == SLOGD_MONITOR_FLAG) {
        *logDaemonScript = SLOGD_DAEMON_SCRIPT;
    } else if (g_flag == SKLOGD_MONITOR_FLAG) {
        *logDaemonScript = SKLOGD_DAEMON_SCRIPT;
    } else if (g_flag == LOGDAEMON_MONITOR_FLAG) {
        *logDaemonScript = LOG_DAEMON_SCRIPT;
    } else {
        SELF_LOG_ERROR("monitor does not exist, flag=%u, Thread(LogMonitor) quit.", g_flag);
        return false;
    }
    return true;
}

STATIC bool AppMonInit(void)
{
    int i = 0;
    int ret = 0;
    LogMonitorSetStatus(LOG_MONITOR_EXIT);
    while (i < NUM_INIT_APPMOND) {
        if (!LogMonitorIsInit()) {
            ret = appmon_client_init(&g_logMonitorMgr.clnt, APPMON_SERVER_PATH);
            if (ret != 0) {
                SELF_LOG_WARN("appmon client(%u) init failed, result=%d.", g_flag, ret);
                ToolSleep(MONITOR_INIT_WAIT_TIME);
                i += 1;
                continue;
            }
        }
        SELF_LOG_INFO("appmon client(%u) init succeed.", g_flag);
        LogMonitorSetStatus(LOG_MONITOR_INIT);
        break;
    }

    return LogMonitorIsInit();
}

int LogMonitorRegister(unsigned int flagLog)
{
    char *logDaemonScript = NULL;
    g_flag = flagLog;
    if (!GetLogDaemonScript(&logDaemonScript)) {
        SELF_LOG_ERROR("get log daemon script failed");
        return SYS_ERROR;
    }
    if (!AppMonInit()) {
        SELF_LOG_ERROR("appmon client(%u) init failed, Thread(LogMonitor) quit.", g_flag);
        return SYS_ERROR;
    }
    int ret = appmon_client_register(&g_logMonitorMgr.clnt, MONITOR_TIMEOUT, logDaemonScript);
    if (ret != 0) {
        SELF_LOG_WARN("appmon client(%u) register failed, result=%d.", g_flag, ret);
        return SYS_ERROR;
    }
    SELF_LOG_INFO("appmon client(%u) register succeed.", g_flag);
    ret = appmon_client_heartbeat(&g_logMonitorMgr.clnt);
    if (ret != 0) {
        SELF_LOG_WARN("appmon client(%u) send heartbeat failed, result=%d.", g_flag, ret);
        return SYS_ERROR;
    }
    SELF_LOG_INFO("appmon client(%u) send heartbeat to appmon succeed.", g_flag);
    LogMonitorSetStatus(LOG_MONITOR_HEARTBEAT);
    return SYS_OK;
}

/**
 * @brief LogMonitorThread: monitor thread, appmon register and appmon client heartbeat
 * @param [in]args: arg list
 * @return: NULL
 */
STATIC void *LogMonitorThread(ArgPtr args)
{
    (void)args;
    NO_ACT_WARN_LOG(SetThreadName("LogMonitor") != 0, "set thread_name(LogMonitor) failed.");
    int ret;
    int count = 0;
    int countFailed = 0;

    char *logDaemonScript = NULL;
    if (!GetLogDaemonScript(&logDaemonScript)) {
        return NULL;
    }
    SELF_LOG_INFO("Thread(LogMonitor) start with the appmon client(%u).", g_flag);

    if (!LogMonitorIsHeartBeat()) {
        if (!AppMonInit()) {
            SELF_LOG_ERROR("appmon client(%u) init failed, Thread(LogMonitor) quit.", g_flag);
            return NULL;
        }
        LogMonitorSetStatus(LOG_MONITOR_RUNNING);
    }
    while (LogMonitorIsRun()) {
        if (!LogMonitorIsHeartBeat()) {
            ret = appmon_client_register(&g_logMonitorMgr.clnt, MONITOR_TIMEOUT, logDaemonScript);
            if (ret != 0) {
                ToolSleep(MONITOR_REGISTER_WAIT_TIME);
                SELF_LOG_WARN("appmon client(%u) register failed, result=%d.", g_flag, ret);
                continue;
            }
            SELF_LOG_INFO("appmon client(%u) register succeed.", g_flag);
            LogMonitorSetStatus(LOG_MONITOR_HEARTBEAT);
        }
        ret = appmon_client_heartbeat(&g_logMonitorMgr.clnt);
        if (ret != 0) {
            countFailed += 1;
            SELF_LOG_WARN("appmon client(%u) send heartbeat failed for %d time(s), result=%d.",
                          g_flag, countFailed, ret);
            ToolSleep(MONITOR_HEARTBEAT_WAIT_TIME);
            continue;
        }
        countFailed = 0;
        ToolSleep(MONITOR_HEARTBEAT_WAIT_TIME);
        count += 1;
        if (count == HEART_BEART_RECORD) {
            SELF_LOG_INFO("appmon client(%u) send heartbeat to appmon succeed.", g_flag);
            count = 0;
        }
    }
    SELF_LOG_ERROR("Thread(LogMonitor) quit.");
    return NULL;
}

void LogMonitorInit(unsigned int flagLog)
{
    g_flag = flagLog;
    if (g_flag >= NUM_MONITOR_PROC) {
        SELF_LOG_ERROR("invalid monitor flag=%u.", g_flag);
        return;
    }

    SELF_LOG_INFO("log monitor init with %u.", g_flag);
    ToolUserBlock funcBlock;
    funcBlock.procFunc = LogMonitorThread;
    funcBlock.pulArg = (void *)NULL;

#ifdef _SLOG_DEVICE_
    ToolThreadAttr threadAttr = LOG_MONITOR_DEFAULT_THREAD_ATTR;
    int ret = ToolCreateTaskWithThreadAttr(&g_logMonitorMgr.tid, &funcBlock, &threadAttr);
#else
    int ret = ToolCreateTaskWithDetach(&g_logMonitorMgr.tid, &funcBlock);
#endif
    if (ret != SYS_OK) {
        SELF_LOG_ERROR("create thread(%u) failed, result=%d.", g_flag, ret);
        return;
    }

    SELF_LOG_INFO("log monitor init succeed with %u.", g_flag);
    return;
}

#else
void LogMonitorInit(unsigned int flagLog)
{
    (void)flagLog;
    return;
}

int LogMonitorRegister(unsigned int flagLog)
{
    (void)flagLog;
    return SYS_OK;
}
#endif // _LOG_MONITOR_
