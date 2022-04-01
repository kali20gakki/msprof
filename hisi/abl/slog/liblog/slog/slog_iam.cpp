/**
* @slog_iam.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include <sys/ioctl.h>
#include <cstdbool>
#include "securec.h"
#include "dlog_error_code.h"
#include "iam.h"
#include "msg_queue.h"
#include "print_log.h"
#include "slog_common.h"

#ifdef __cplusplus
#ifndef LOG_CPP
extern "C" {
#endif
#endif // __cplusplus

#define IOCTL_W_PRINT_NUM 1000
static int g_isInited = NO_INITED;
static int g_logOutFd = INVALID;
static int g_iamInited = NO_INITED;
static int g_ioctlGlobalPrintNum = 0;
static int g_ioctlEventPrintNum = 0;
static int g_pid = 0;
STATIC int g_levelThreadStatus = 1;
STATIC IamLogBuf *g_iamLogBuf = NULL;
static int g_levelCountUnit[LOG_MSG_NODE_NUM] = { 0 };
STATIC bool g_errorLogAppear = false;
static toolMutex g_slogMutex = PTHREAD_MUTEX_INITIALIZER;

static int g_enableEvent = TRUE;
static bool g_logCtrlSwitch = false;
static unsigned int g_writePrintNum = 0;
static struct timespec g_lastTv = { 0, 0 };
static int g_globalLogLevel = GLOABLE_DEFAULT_LOG_LEVEL; // default loglevel(info)
static int g_logCtrlLevel = GLOABLE_DEFAULT_LOG_LEVEL;
static unsigned int g_levelCount[LOG_COUNT_NUM] = { }; // debug, info, warn, error
static LogAttr g_logAttr = { APPLICATION, 0, 0, { 0 } };
static int g_levelSetted = FALSE;

__attribute__((constructor)) void dlog_init(void);
__attribute__((destructor)) void DlogExitForIam(void);

STATIC int CheckIamInited();

STATIC INT32 SlogLock(void)
{
    return ToolMutexLock(&g_slogMutex);
}

STATIC INT32 SlogUnLock(void)
{
    return ToolMutexUnLock(&g_slogMutex);
}

/**
* @brief GetModuleLogLevelByIam: get module loglevel by IAM
* @return: void
*/
void GetModuleLogLevelByIam(void)
{
    int ret;
    LevelConfInfo levelInfo;
    const ModuleInfo *moduleInfo = GetModuleInfos();
    (void)memset_s(&levelInfo, sizeof(LevelConfInfo), 0, sizeof(LevelConfInfo));
    ret = strncpy_s(levelInfo.configName, CONF_NAME_MAX_LEN + 1,
                    IOCTL_MODULE_NAME, strlen(IOCTL_MODULE_NAME));
    if (ret != EOK) {
        SELF_LOG_WARN("moduleName strcpy failed, ret=%d, strerr=%s, pid=%d.", \
                      ret, strerror(ToolGetErrorCode()), g_pid);
        return;
    }
    // app call write to get info from slogmgr
    struct IAMIoctlArg arg;
    arg.size = sizeof(LevelConfInfo);
    arg.argData = (void *)&levelInfo;
    ret = ioctl(g_logOutFd, CMD_GET_LEVEL, &arg);
    if (ret == SYS_ERROR) {
        return;
    }

    for (; moduleInfo->moduleName != NULL; moduleInfo++) {
        if ((moduleInfo->moduleId < 0) || (moduleInfo->moduleId >= INVLID_MOUDLE_ID) ||
            (levelInfo.configValue[moduleInfo->moduleId] < LOG_MIN_LEVEL) ||
            (levelInfo.configValue[moduleInfo->moduleId] > LOG_MAX_LEVEL)) {
            continue;
        }
        (void)SetLevelByModuleId(moduleInfo->moduleId, levelInfo.configValue[moduleInfo->moduleId]);
    }
    return;
}

/**
* @brief GetGlobalLogLevelByIam: get global loglevel By IAM
* @return: void
*/
void GetGlobalLogLevelByIam(void)
{
    LevelConfInfo levelInfo;
    (void)memset_s(&levelInfo, sizeof(LevelConfInfo), 0, sizeof(LevelConfInfo));
    int ret = strncpy_s(levelInfo.configName, CONF_NAME_MAX_LEN + 1, GLOBALLEVEL_KEY, strlen(GLOBALLEVEL_KEY));
    if (ret != EOK) {
        SELF_LOG_WARN("name(global_level) strcpy failed, result=%d, strerr=%s, pid=%d.", \
                      ret, strerror(ToolGetErrorCode()), g_pid);
        return;
    }
    struct IAMIoctlArg arg;
    arg.size = sizeof(LevelConfInfo);
    arg.argData = (void *)&levelInfo;
    ret = ioctl(g_logOutFd, CMD_GET_LEVEL, &arg);
    if (ret == SYS_OK) {
        if ((levelInfo.configValue[0] >= LOG_MIN_LEVEL) && (levelInfo.configValue[0] <= LOG_MAX_LEVEL)) {
            g_globalLogLevel = levelInfo.configValue[0];
            SELF_LOG_INFO("g_globalLogLevel=%d.", g_globalLogLevel);
        } else {
            SELF_LOG_WARN("global_level=%d is illegal, pid=%d, use value=%d.", \
                          levelInfo.configValue[0], g_pid, g_globalLogLevel);
        }
    } else {
        SELF_LOG_WARN_N(&g_ioctlGlobalPrintNum, IOCTL_W_PRINT_NUM, "get global_level failed, result=%d, "
                        "pid=%d, use value=%d", ret, g_pid, g_globalLogLevel);
    }
}

/**
* @brief GetEventEnableByIam: get event enable By IAM
* @return: void
*/
void GetEventEnableByIam(void)
{
    LevelConfInfo levelInfo;
    (void)memset_s(&levelInfo, sizeof(LevelConfInfo), 0, sizeof(LevelConfInfo));
    int ret = strncpy_s(levelInfo.configName, CONF_NAME_MAX_LEN + 1, ENABLEEVENT_KEY, strlen(ENABLEEVENT_KEY));
    if (ret != EOK) {
        SELF_LOG_WARN("name(enableEvent) strcpy failed, result=%d, strerr=%s, pid=%d.", \
                      ret, strerror(ToolGetErrorCode()), g_pid);
        return;
    }
    struct IAMIoctlArg arg;
    arg.size = sizeof(LevelConfInfo);
    arg.argData = (void *)&levelInfo;
    ret = ioctl(g_logOutFd, CMD_GET_LEVEL, &arg);
    if (ret == SYS_OK) {
        if ((levelInfo.configValue[0] == EVENT_DISABLE_VALUE) || (levelInfo.configValue[0] == EVENT_ENABLE_VALUE)) {
            g_enableEvent = (levelInfo.configValue[0] == EVENT_ENABLE_VALUE) ? TRUE : FALSE;
            SELF_LOG_INFO("g_enableEvent=%d.", g_enableEvent);
        } else {
            SELF_LOG_WARN("enableEvent=%d is illegal, pid=%d, use value=%d.", \
                          levelInfo.configValue[0], g_pid, g_enableEvent);
        }
    } else {
        SELF_LOG_WARN_N(&g_ioctlEventPrintNum, IOCTL_W_PRINT_NUM, "get enableEvent failed, result=%d, "
                        "pid=%d, use value=%d", ret, g_pid, g_enableEvent);
    }
}

extern char *__progname;

STATIC int LogToIamLogBuf(const LogMsg *logMsg)
{
    // if fd init failed, reserve log msg in buff(max log msg num refer to LOG_MSG_NODE_NUM)
    if (g_iamLogBuf->nodeCount >= LOG_MSG_NODE_NUM) {
        return TRUE;
    }
    int level = logMsg->level;
    if (level == DLOG_ERROR) {
        g_errorLogAppear = true;
    }
    g_levelCountUnit[g_iamLogBuf->nodeCount] = level;
    int ret = memcpy_s(&g_iamLogBuf->logMsgList[g_iamLogBuf->nodeCount], sizeof(LogMsg), logMsg, sizeof(LogMsg));
    if (ret != EOK) {
        return FALSE;
    }
    g_iamLogBuf->nodeCount++;
    return (g_iamLogBuf->nodeCount >= LOG_MSG_NODE_NUM) ? TRUE : FALSE;
}

STATIC void LogLevelUnitCal(void)
{
    unsigned int i;
    unsigned int nodeCount = g_iamLogBuf->nodeCount;
    if (nodeCount > LOG_MSG_NODE_NUM) {
        return;
    }
    for (i = 0; i < nodeCount; i++) {
        if ((g_levelCountUnit[i] < 0) || (g_levelCountUnit[i] >= LOG_COUNT_NUM)) {
            continue;
        }
        g_levelCount[g_levelCountUnit[i]]++;
    }
}

STATIC void SafeWritesResetByIam(void)
{
    (void)memset_s(g_levelCountUnit, sizeof(g_levelCountUnit), 0, sizeof(g_levelCountUnit));
    g_iamLogBuf->nodeCount = 0;
    (void)memset_s(g_iamLogBuf->logMsgList, LOG_MSG_NODE_NUM * sizeof(LogMsg), 0, LOG_MSG_NODE_NUM * sizeof(LogMsg));
    g_errorLogAppear = false;
}

STATIC void LogCtrlDecLogic(void)
{
    const long timeValue = TimeDiff(&g_lastTv);
    if (timeValue >= LOG_WARN_INTERVAL) {
        const char *pidName = (__progname != NULL) ? __progname : (const char *)"None";
        if (timeValue < LOG_INFO_INTERVAL) {
            if (g_logCtrlLevel != DLOG_WARN) {
                g_logCtrlLevel = DLOG_WARN;
                SELF_LOG_WARN("log control down to level=WARNING, pid=%d, pid_name=%s, log loss condition: " \
                              "error_num=%u, warn_num=%u, info_num=%u, debug_num=%u.",
                              g_pid, pidName, g_levelCount[DLOG_ERROR], g_levelCount[DLOG_WARN],
                              g_levelCount[DLOG_INFO], g_levelCount[DLOG_DEBUG]);
            }
        } else if (timeValue < LOG_CTRL_TOTAL_INTERVAL) {
            if (g_logCtrlLevel != DLOG_INFO) {
                g_logCtrlLevel = DLOG_INFO;
                SELF_LOG_WARN("log control down to level=INFO, pid=%d, pid_name=%s, log loss condition: " \
                              "error_num=%u, warn_num=%u, info_num=%u, debug_num=%u.",
                              g_pid, pidName, g_levelCount[DLOG_ERROR], g_levelCount[DLOG_WARN],
                              g_levelCount[DLOG_INFO], g_levelCount[DLOG_DEBUG]);
            }
        } else {
            g_logCtrlSwitch = true;
            g_logCtrlLevel = g_globalLogLevel;
            g_lastTv.tv_sec = 0;
            g_lastTv.tv_nsec = 0;
            SELF_LOG_WARN("clear log control switch, pid=%d, pid_name=%s, log loss condition: " \
                          "error_num=%u, warn_num=%u, info_num=%u, debug_num=%u.",
                          g_pid, pidName, g_levelCount[DLOG_ERROR], g_levelCount[DLOG_WARN],
                          g_levelCount[DLOG_INFO], g_levelCount[DLOG_DEBUG]);
        }
    }
}

STATIC void LogCtrlIncLogic(void)
{
    const char *pidName = (__progname != NULL) ? __progname : (const char *)"None";
    if (!g_logCtrlSwitch) {
        g_logCtrlSwitch = true;
        g_logCtrlLevel = DLOG_ERROR;
        SELF_LOG_WARN("set log control switch to level=ERROR, pid=%d, pid_name=%s, log loss condition: " \
                      "error_num=%u, warn_num=%u, info_num=%u, debug_num=%u.",
                      g_pid, pidName, g_levelCount[DLOG_ERROR], g_levelCount[DLOG_WARN],
                      g_levelCount[DLOG_INFO], g_levelCount[DLOG_DEBUG]);
    } else {
        if (g_logCtrlLevel < DLOG_ERROR) {
            g_logCtrlLevel++;
            SELF_LOG_WARN("log control up to level=%s, pid=%d, pid_name=%s, log loss condition: " \
                          "error_num=%u, warn_num=%u, info_num=%u, debug_num=%u.",
                          GetBasicLevelNameById(g_logCtrlLevel), g_pid, pidName, g_levelCount[DLOG_ERROR],
                          g_levelCount[DLOG_WARN], g_levelCount[DLOG_INFO], g_levelCount[DLOG_DEBUG]);
        }
    }
    (void)clock_gettime(CLOCK_INPUT, &g_lastTv);
}

STATIC void SafeWritesByIam(const int fd)
{
    int n;
    int err;
    int retryTimes = 0;
    ONE_ACT_NO_LOG(fd == INVALID, return);

    do {
        n = write(fd, (void *)g_iamLogBuf, sizeof(IamLogBuf));
        err = ToolGetErrorCode();
        if (n < 0) {
            if (err == EINTR) {
                continue;
            } else if (g_errorLogAppear) {
                retryTimes++;
                LogCtrlIncLogic();
                continue;
            }
            break;
        }
    } while ((n < 0) && (retryTimes != WRITE_MAX_RETRY_TIMES));

    if ((n > 0) && g_logCtrlSwitch) {
        LogCtrlDecLogic();
    } else if (n < 0) {
        LogLevelUnitCal();
        const char *pidName = (__progname != NULL) ? __progname : (const char *)"None";
        SELF_LOG_WARN_N(&g_writePrintNum, WRITE_E_PRINT_NUM, "write failed, print every %d times, result=%d, "
                        "strerr=%s, pid=%d, pid_name=%s, log loss condition: error_num=%u, warn_num=%u, "
                        "info_num=%u, debug_num=%u.", WRITE_E_PRINT_NUM, n, strerror(err), g_pid, pidName,
                        g_levelCount[DLOG_ERROR], g_levelCount[DLOG_WARN], g_levelCount[DLOG_INFO],
                        g_levelCount[DLOG_DEBUG]);
    }
    SafeWritesResetByIam();
    return;
}

STATIC void GetTimeForIam(char *timeStr, const unsigned int size)
{
    struct timespec currentTimeval = { 0, 0 };
    struct tm timInfo;
    (void)memset_s(&timInfo, sizeof(timInfo), 0, sizeof(timInfo));
    ONE_ACT_ERR_LOG(timeStr == NULL, return, "[input] time is null.");

    if (clock_gettime(CLOCK_VIRTUAL, &currentTimeval) != SYS_OK) {
        SELF_LOG_WARN("get time failed, strerr=%s, pid=%d.", strerror(ToolGetErrorCode()), g_pid);
        return;
    }

    if (GetLocaltimeR(&timInfo, currentTimeval.tv_sec) != SYS_OK) {
        SELF_LOG_WARN("get local time failed, strerr=%s, pid=%d.", strerror(ToolGetErrorCode()), g_pid);
        return;
    }

    const int ret = snprintf_s(timeStr, size, size - 1, "%04d-%02d-%02d-%02d:%02d:%02d.%03ld.%03ld",
                               timInfo.tm_year, timInfo.tm_mon, timInfo.tm_mday, timInfo.tm_hour, timInfo.tm_min,
                               timInfo.tm_sec, (currentTimeval.tv_nsec / TIME_ONE_THOUSAND_MS) / TIME_ONE_THOUSAND_MS,
                               (currentTimeval.tv_nsec / TIME_ONE_THOUSAND_MS) % TIME_ONE_THOUSAND_MS);
    if (ret == -1) {
        SELF_LOG_WARN("snprintf_s time failed, result=%d, strerr=%s, pid=%d.", \
                      ret, strerror(ToolGetErrorCode()), g_pid);
    }
    return;
}

STATIC void InitEventLevelByEnv(void)
{
    const char *eventEnv = getenv("ASCEND_GLOBAL_EVENT_ENABLE");
    if ((eventEnv != NULL) && (IsNaturalNumStr(eventEnv))) {
        const int tmpL = atoi(eventEnv);
        if ((tmpL == TRUE) || (tmpL == FALSE)) {
            g_enableEvent = tmpL;
            SELF_LOG_INFO("get right env ASCEND_GLOBAL_EVENT_ENABLE(%d).", tmpL);
            return;
        }
    }
    g_enableEvent = TRUE;
    SELF_LOG_INFO("set default global event level (%d).", TRUE);
}

STATIC void InitLogLevelByEnv(void)
{
    // global level env is prior to config file
    const char *globalLevelEnv = getenv("ASCEND_GLOBAL_LOG_LEVEL");
    if ((globalLevelEnv != NULL) && (IsNaturalNumStr(globalLevelEnv))) {
        const int globalLevel = atoi(globalLevelEnv);
        if ((globalLevel >= LOG_MIN_LEVEL) && (globalLevel <= LOG_MAX_LEVEL)) {
            g_globalLogLevel = globalLevel;
            SetLevelToAllModule(globalLevel);
            SELF_LOG_INFO("get env ASCEND_GLOBAL_LOG_LEVEL(%d) and it is prior to conf file.", globalLevel);
            return;
        }
    }
    g_globalLogLevel = GLOABLE_DEFAULT_LOG_LEVEL;
    SetLevelToAllModule(GLOABLE_DEFAULT_LOG_LEVEL);
    SELF_LOG_INFO("set default global log level (%d).", GLOABLE_DEFAULT_LOG_LEVEL);
}

/**
* @brief CheckLogLevel: check log allow output or not
* @param [in]moduleId: module Id
* @param [in]level: log level
* @return: TRUE/FALSE
*/
int CheckLogLevel(int moduleId, int logLevel)
{
    if (CheckIamInited() == FALSE) {
        return FALSE;
    }

    if ((logLevel == DLOG_TRACE) || (logLevel == DLOG_OPLOG)) {
        return TRUE;
    }

    // event log check
    if (logLevel == DLOG_EVENT) {
        return (g_enableEvent == TRUE) ? TRUE : FALSE;
    }

    // get module loglevel by moduleId
    int realModuleId = moduleId;
    if (moduleId >= 0) {
        realModuleId = (unsigned int)moduleId & MODULE_ID_MASK;
    }
    int moduleLevel = g_globalLogLevel;
    if ((realModuleId >= 0) && (realModuleId < INVLID_MOUDLE_ID)) {
        moduleLevel = GetLevelByModuleId(realModuleId);
        if ((moduleLevel < LOG_MIN_LEVEL) || (moduleLevel > LOG_MAX_LEVEL)) {
            moduleLevel = g_globalLogLevel;
        }
    }

    // check module loglevel and log control, check time diff to make switch back to false
    if ((logLevel >= moduleLevel) && (logLevel < LOG_MAX_LEVEL)) {
        if (g_logCtrlSwitch && (TimeDiff(&g_lastTv) <= LOG_CTRL_TOTAL_INTERVAL)) {
            TWO_ACT_NO_LOG(logLevel < g_logCtrlLevel, g_levelCount[logLevel]++, return FALSE);
        }
        return TRUE;
    }
    return FALSE;
}

/**
* @brief WriteLogToFileByIam: write log message to file or stdout
capture SIGPIPE, prepare for a broken connection
* @param [in]fd: log message
* @param [in]logMsg: log message
*/
STATIC void WriteLogToFileByIam(const int fd, const LogMsg *logMsg)
{
    if ((logMsg == NULL) || (logMsg->msgLength == 0) || (g_iamLogBuf == NULL)) {
        return;
    }
    if (LogToIamLogBuf(logMsg)) {
        SafeWritesByIam(fd); // do not check, because print a lot
        return;
    }
}

/**
* @brief ParseLogMsg: parse module Id
* @param [in]moduleId: module id (in lower 16 bits) with log type(in higher 16 bits)
* @param [out]logMsg: log Msg data struct
*/
STATIC void ParseLogMsg(const int moduleId, const int level, LogMsg *logMsg)
{
    logMsg->level = level;
    int realModuleId = moduleId;
    int logTypeMask = 0;
    if (moduleId >= 0) {
        realModuleId = (unsigned int)moduleId & MODULE_ID_MASK;
        logTypeMask = (unsigned int)moduleId & LOG_TYPE_MASK;
    }
    logMsg->moduleId = realModuleId;
    if (logTypeMask == DEBUG_LOG_MASK) {
        logMsg->type = DEBUG_LOG;
    } else if (logTypeMask == SECURITY_LOG_MASK) {
        logMsg->type = SECURITY_LOG;
    } else if (logTypeMask == RUN_LOG_MASK) {
        logMsg->type = RUN_LOG;
    } else if (logTypeMask == OPERATION_LOG_MASK) {
        logMsg->type = OPERATION_LOG;
    } else {
        logMsg->type = DEBUG_LOG;
    }
}

STATIC void *WriteLogByIamPeriodic(void *)
{
    NO_ACT_WARN_LOG(SetThreadName("WriteLogByIamPeriodic") != 0,
                    "set thread_name(WriteLogByIamPeriodic) failed, pid=%d.", g_pid);
    while (g_levelThreadStatus != 0) {
        (void)SlogLock();
        if ((g_iamLogBuf != NULL) && (g_iamLogBuf->nodeCount > 0)) {
            SafeWritesByIam(g_logOutFd);
        }
        (void)SlogUnLock();
        ToolSleep(ONE_SECOND);
    }
    SELF_LOG_INFO("Thread(WriteLogByIamPeriodic) quit, pid=%d.", g_pid);
    return NULL;
}

STATIC void StartThreadForLevelChange(void)
{
    // start thread
    ToolUserBlock thread;
    thread.procFunc = WriteLogByIamPeriodic;
    thread.pulArg = NULL;

    toolThread tid = 0;
    ONE_ACT_ERR_LOG(ToolCreateTaskWithDetach(&tid, &thread) != SYS_OK, return,
                    "create task failed, strerr=%s, pid=%d.", strerror(ToolGetErrorCode()), g_pid);
}

/**
 * @brief: check sub process, restart thread
 * @return: void
 */
STATIC void CheckPid(void)
{
    if (g_pid != ToolGetPid()) {
        (void)SlogLock();
        if (g_pid != ToolGetPid()) {
            g_pid = ToolGetPid();
            StartThreadForLevelChange();
        }
        (void)SlogUnLock();
    }
}

/**
* @brief ConstructBaseLogMsg: construct base log message
* @param [in]msg: log message
* @param [in]msgLen: log message max length
* @param [in]msgArg: log info, include level, module, and key-value
* @return SYS_OK/SYS_ERROR
*/
static int ConstructBaseLogMsg(char *msg, unsigned const int msgLen, const LogMsgArg msgArg)
{
    int err;
#if (OS_TYPE_DEF == 0)
    const char *pidName = (__progname != NULL) ? __progname : "None";
#else
    const char *pidName = "None";
#endif
    if ((msgArg.moduleId >= 0) && (msgArg.moduleId < INVLID_MOUDLE_ID)) {
        if (((msgArg.level >= DLOG_DEBUG) && (msgArg.level <= DLOG_OPLOG)) || (msgArg.level == DLOG_EVENT)) {
            err = snprintf_s(msg, msgLen, msgLen - 1, "[%d,%u,%u][%s] %s(%d,%s):%s ",
                             g_logAttr.type, g_logAttr.pid, g_logAttr.deviceId,
                             GetLevelNameById(msgArg.level), GetModuleNameById(msgArg.moduleId),
                             g_pid, pidName, msgArg.timestamp);
        } else {
            err = snprintf_s(msg, msgLen, msgLen - 1, "[%d,%u,%u][%d] %s(%d,%s):%s ",
                             g_logAttr.type, g_logAttr.pid, g_logAttr.deviceId,
                             msgArg.level, GetModuleNameById(msgArg.moduleId),
                             g_pid, pidName, msgArg.timestamp);
        }
    } else {
        if (((msgArg.level >= DLOG_DEBUG) && (msgArg.level <= DLOG_OPLOG)) || (msgArg.level == DLOG_EVENT)) {
            err = snprintf_s(msg, msgLen, msgLen - 1, "[%d,%u,%u][%s] %d(%d,%s):%s ",
                             g_logAttr.type, g_logAttr.pid, g_logAttr.deviceId,
                             GetLevelNameById(msgArg.level), msgArg.moduleId,
                             g_pid, pidName, msgArg.timestamp);
        } else {
            err = snprintf_s(msg, msgLen, msgLen - 1, "[%d,%u,%u][%d] %d(%d,%s):%s ",
                             g_logAttr.type, g_logAttr.pid, g_logAttr.deviceId,
                             msgArg.level, msgArg.moduleId, g_pid, pidName, msgArg.timestamp);
        }
    }
    if (err == -1) {
        SELF_LOG_WARN("snprintf_s failed, strerr=%s, pid=%d, pid_name=%s, module=%d.",
                      strerror(ToolGetErrorCode()), g_pid, pidName, msgArg.moduleId);
        return SYS_ERROR;
    }
    return SYS_OK;
}

/**
* @brief ConstructLogMsg: construct full log content
* @param [in]msg: log message
* @param [in]msgLen: log message max length
* @param [in]msgArg: log info, include level, module, and key-value
* @param [in]fmt: log content format
* @param [in]v: merge to log message variable
* @return SYS_OK/SYS_ERROR
*/
STATIC int ConstructLogMsg(char *msg, unsigned const int msgLen, const LogMsgArg msgArg, const char *fmt, va_list v)
{
    ONE_ACT_NO_LOG(msg == NULL, return SYS_ERROR);
    ONE_ACT_NO_LOG((msgLen == 0) || (msgLen > MSG_LENGTH), return SYS_ERROR);
    ONE_ACT_NO_LOG(fmt == NULL, return SYS_ERROR);

    int i;
    unsigned int len;
#if (OS_TYPE_DEF == 0)
    const char *pidName = (__progname != NULL) ? __progname : "None";
#else
    const char *pidName = "None";
#endif

    int err = ConstructBaseLogMsg(msg, msgLen, msgArg);
    if (err != SYS_OK) {
        SELF_LOG_WARN("construct base log msg failed.");
        return SYS_ERROR;
    }

    // splice key and value
    KeyValue *pstKVArray = msgArg.kvArg.pstKVArray;
    for (i = 0; i < msgArg.kvArg.kvNum; i++, pstKVArray++) {
        len = strlen(msg);
        err = snprintf_s(msg + len, msgLen - len, msgLen - len - 1U, "[%s:%s] ",
                         pstKVArray->kname, pstKVArray->value);
        if (err == -1) {
            SELF_LOG_WARN("snprintf_s failed, strerr=%s, pid=%d, pid_name=%s, module=%d.",
                          strerror(ToolGetErrorCode()), g_pid, pidName, msgArg.moduleId);
            return SYS_ERROR;
        }
    }

    // construct log content
    len = strlen(msg);
    err = vsnprintf_truncated_s(msg + len, msgLen - len, fmt, v);
    if (err == -1) {
        SELF_LOG_WARN("vsnprintf_truncated_s failed, strerr=%s, pid=%d, pid_name=%s, module=%d.",
                      strerror(ToolGetErrorCode()), g_pid, pidName, msgArg.moduleId);
        return SYS_ERROR;
    }
    return SYS_OK;
}

/**
* @brief WriteDlogByIam: write log to log socket or stdout
* @param [in]moduleId: module id number
* @param [in]level: log leve number
* @param [in]kvArg: pointer to struct that contains key-value arrary and size of this array
* @param [in]fmt: pointer to first value in va_list
* @param [in]v: variable list
*/
static void WriteDlogByIam(const int moduleId, const int level, const KeyValueArg *kvArg, const char *fmt, va_list v)
{
    int result;
    LogMsg logMsg;
    char timestamp[TIMESTAMP_LEN] = { 0 };

    CheckPid();
    (void)memset_s(&logMsg, sizeof(LogMsg), 0, sizeof(LogMsg));
    ParseLogMsg(moduleId, level, &logMsg);

    // construct log content
    GetTimeForIam(timestamp, TIMESTAMP_LEN);
    const LogMsgArg msgArg = { logMsg.moduleId, logMsg.level, (char *)timestamp, *kvArg };
    result = ConstructLogMsg(logMsg.msg, sizeof(logMsg.msg), msgArg, fmt, v);
    ONE_ACT_WARN_LOG(result != SYS_OK, return, "construct log content failed.");
    logMsg.msg[MSG_LENGTH - 1] = '\0';
    logMsg.msgLength = strlen(logMsg.msg);

    (void)SlogLock();
    WriteLogToFileByIam(g_logOutFd, &logMsg);
    (void)SlogUnLock();
    return;
}

int OpenIamServiceFd(const char *serviceName, const int serviceNameLen, int *fd)
{
    int retry = 0;
    if ((serviceName == NULL) || (fd == NULL) || (serviceNameLen < 0) ||
        (serviceNameLen > IAM_SERVICE_NAME_MAX_LENGTH)) {
        SYSLOG_WARN("Input invalid, serviceNameLen=%d\n", serviceNameLen);
        return SYS_ERROR;
    }

    while ((*fd == INVALID) && (retry < IAM_RETRY_TIMES)) {
        *fd = open(serviceName, O_RDWR);
        retry++;
    }
    return (*fd == INVALID) ? SYS_ERROR : SYS_OK;
}

STATIC int CheckIamInited()
{
    if (g_iamInited == INITED) {
        return TRUE;
    }
    (void)SlogLock();
    if (g_iamInited == INITED) {
        (void)SlogUnLock();
        return TRUE;
    }

    if (g_iamLogBuf == NULL) {
        g_iamLogBuf = (IamLogBuf *)malloc(sizeof(IamLogBuf));
        if (g_iamLogBuf == NULL) {
            (void)SlogUnLock();
            return FALSE;
        }
        (void)memset_s(g_iamLogBuf, sizeof(IamLogBuf), 0x00, sizeof(IamLogBuf));
    }

    // if fd init failed, return true to reserve logmsg in buff rather than clear buff
    if (OpenIamServiceFd(LOGOUT_IAM_SERVICE_PATH, strlen(LOGOUT_IAM_SERVICE_PATH), &g_logOutFd) != SYS_OK) {
        (void)SlogUnLock();
        return TRUE;
    }

    SELF_LOG_INFO("Iam open all service fd succeed, pid=%d.", g_pid);
#ifdef LOG_CPP
    InitLogLevelByEnv();
    InitEventLevelByEnv();
#else
    // get global_level
    GetGlobalLogLevelByIam();
    // get module loglevel
    GetModuleLogLevelByIam();
    // get enableEvent
    GetEventEnableByIam();
#endif
    StartThreadForLevelChange();
    g_iamInited = INITED;
    (void)SlogUnLock();
    return TRUE;
}

/**
* @brief dlog_init: libslog.so initialize
* @return: void
*/
void dlog_init(void)
{
    g_pid = ToolGetPid();
    if (g_isInited == INITED) {
        return;
    }

    const int ret = pthread_atfork((ThreadAtFork)SlogLock, (ThreadAtFork)SlogUnLock, (ThreadAtFork)SlogUnLock);
    if (ret != 0) {
        SYSLOG_WARN("register fork failed, ret=%d, strerr=%s.\n", ret, strerror(ToolGetErrorCode()));
    }

    // sync time zone
    tzset();

    g_isInited = INITED;
#ifdef LOG_CPP
    g_logAttr.pid = (unsigned int)getpid();
#else
    g_logAttr.type = SYSTEM;
#endif
}

void DlogErrorInner(int moduleId, const char *fmt, ...)
{
    if (fmt != NULL) {
        const int result = CheckLogLevel(moduleId, DLOG_ERROR);
        ONE_ACT_NO_LOG(result == FALSE, return);
        va_list list;
        va_start(list, fmt);
        const KeyValueArg kvArg = { NULL, 0 };
        WriteDlogByIam(moduleId, DLOG_ERROR, &kvArg, fmt, list);
        va_end(list);
    }
}

void DlogWarnInner(int moduleId, const char *fmt, ...)
{
    if (fmt != NULL) {
        const int result = CheckLogLevel(moduleId, DLOG_WARN);
        ONE_ACT_NO_LOG(result == FALSE, return);
        va_list list;
        va_start(list, fmt);
        const KeyValueArg kvArg = { NULL, 0 };
        WriteDlogByIam(moduleId, DLOG_WARN, &kvArg, fmt, list);
        va_end(list);
    }
}

void DlogInfoInner(int moduleId, const char *fmt, ...)
{
    if (fmt != NULL) {
        const int result = CheckLogLevel(moduleId, DLOG_INFO);
        ONE_ACT_NO_LOG(result == FALSE, return);
        va_list list;
        va_start(list, fmt);
        const KeyValueArg kvArg = { NULL, 0 };
        WriteDlogByIam(moduleId, DLOG_INFO, &kvArg, fmt, list);
        va_end(list);
    }
}

void DlogDebugInner(int moduleId, const char *fmt, ...)
{
    if (fmt != NULL) {
        const int result = CheckLogLevel(moduleId, DLOG_DEBUG);
        ONE_ACT_NO_LOG(result == FALSE, return);
        va_list list;
        va_start(list, fmt);
        const KeyValueArg kvArg = { NULL, 0 };
        WriteDlogByIam(moduleId, DLOG_DEBUG, &kvArg, fmt, list);
        va_end(list);
    }
}

void DlogEventInner(int moduleId, const char *fmt, ...)
{
    if (fmt != NULL) {
        const int result = CheckLogLevel(moduleId, DLOG_EVENT);
        ONE_ACT_NO_LOG(result == FALSE, return);
        va_list list;
        va_start(list, fmt);
        const KeyValueArg kvArg = { NULL, 0 };
        WriteDlogByIam(moduleId, DLOG_EVENT, &kvArg, fmt, list);
        va_end(list);
    }
}

/**
* @brief DlogInner: log interface with level
* @param [in]moduleId: moudule Id eg: CCE
* @param [in]level: log level((0: debug, 1: info, 2: warning, 3: error)
* @param [in]fmt: log msg string
* @return: void
*/
void DlogInner(int moduleId, int level, const char *fmt, ...)
{
    if (fmt != NULL) {
        const int result = CheckLogLevel(moduleId, level);
        ONE_ACT_NO_LOG(result == FALSE, return);
        va_list list;
        va_start(list, fmt);
        const KeyValueArg kvArg = { NULL, 0 };
        WriteDlogByIam(moduleId, level, &kvArg, fmt, list);
        va_end(list);
    }
}

/**
* @brief DlogWithKVInner: log interface with level
* @param [in]moduleId: moudule Id eg: CCE
* @param [in]level: log level(0: debug, 1: info, 2: warning, 3: error)
* @param [in]pstKVArray: key-value arrary
* @param [in]kvNum: num of key-value arrary
* @param [in]fmt: log msg string
* @return: void
*/
void DlogWithKVInner(int moduleId, int level, KeyValue *pstKVArray, int kvNum, const char *fmt, ...)
{
    ONE_ACT_ERR_LOG(pstKVArray == NULL, return, "[input] key-value array is null.");
    ONE_ACT_ERR_LOG(kvNum <= 0, return, "[input] key-value number=%d is invalid.", kvNum);
    if (fmt != NULL) {
        const int result = CheckLogLevel(moduleId, level);
        ONE_ACT_NO_LOG(result == FALSE, return);
        va_list list;
        va_start(list, fmt);
        const KeyValueArg kvArg = { pstKVArray, kvNum };
        WriteDlogByIam(moduleId, level, &kvArg, fmt, list);
        va_end(list);
    }
}

/**
* @brief DlogExitForIam: destructor function of libslog.so
* @return: void
*/
void DlogExitForIam(void)
{
    g_levelThreadStatus = 0;
    const int pid = g_pid;
    // if call this in thread exit, it may not be called
    (void)SlogLock();
    if (g_iamLogBuf && (g_iamLogBuf->nodeCount > 0)) {
        SafeWritesByIam(g_logOutFd);
    }
    XFREE(g_iamLogBuf);
    (void)SlogUnLock();

    // log loss selflog print
    if ((g_levelCount[DLOG_ERROR] != 0) || (g_levelCount[DLOG_WARN] != 0) ||
        (g_levelCount[DLOG_INFO] != 0) || (g_levelCount[DLOG_DEBUG] != 0)) {
        const char *pidName = (__progname != NULL) ? __progname : (const char *)"None";
        SELF_LOG_WARN("pid=%d, pid_name=%s quit, total log loss condition: error_num=%u, warn_num=%u, info_num=%u,"
                      "debug_num=%u.", pid, pidName, g_levelCount[DLOG_ERROR], g_levelCount[DLOG_WARN],
                      g_levelCount[DLOG_INFO], g_levelCount[DLOG_DEBUG]);
    }
}

/**
* @brief DlogFlush: flush log buffer to file
* @return: void
*/
void DlogFlush(void)
{
    FlushInfo info;
    struct IAMIoctlArg arg;

    (void)SlogLock();
    if (g_iamLogBuf && (g_iamLogBuf->nodeCount > 0)) {
        SafeWritesByIam(g_logOutFd);
    }
    (void)SlogUnLock();

#ifdef LOG_CPP
    info.appPid = g_pid;
#else
    info.appPid = INVALID;
#endif
    arg.size = sizeof(FlushInfo);
    arg.argData = (void *)&info;
    const int ret = ioctl(g_logOutFd, CMD_FLUSH_LOG, &arg);
    if (ret != SYS_OK) {
        SELF_LOG_WARN("log flush failed, ret=%d, pid=%d.", ret, g_pid);
        return;
    }
}

/**
 * @brief DlogSetAttr: set log attr
 * @param [in]logAttr: attr info, include pid, process type and device id
 * @return: 0: SUCCEED, others: FAILED
 */
int DlogSetAttr(LogAttr logAttr)
{
    if ((logAttr.type == APPLICATION) && (logAttr.pid <= 0)) {
        SELF_LOG_ERROR("set log attr failed, pid=%u is invalid.", logAttr.pid);
        return SYS_ERROR;
    }
    if ((logAttr.type == APPLICATION) && (logAttr.deviceId >= MAX_DEV_NUM)) {
        SELF_LOG_ERROR("set log attr failed, deviceId=%u is invalid.", logAttr.deviceId);
        return SYS_ERROR;
    }
    g_logAttr.deviceId = logAttr.deviceId;
    g_logAttr.type = logAttr.type;
    g_logAttr.pid = logAttr.pid;
    SELF_LOG_INFO("set log attr, pid=%u, type=%d, devId=%u.", logAttr.pid, logAttr.type, logAttr.deviceId);
    if ((logAttr.type == APPLICATION) && !g_levelSetted) {
        // update level by env after setting app property.
        // g_levelSetted is setted true in the interface 'dlog_setlevel',
        // if g_levelSetted is false, then update level by env.
        InitLogLevelByEnv();
        InitEventLevelByEnv();
    }
    return SUCCESS;
}

/**
 * @brief RegisterCallback: register DlogCallback
 * @param [in]callback: function pointer
 * @return: 0: SUCCEED, others: FAILED
 */
int RegisterCallback(const ArgPtr callback, const CallbackType funcType)
{
    (void)callback;
    (void)funcType;
    return SUCCESS;
}

/**
* @brief dlog_getlevel: get module loglevel
* @param [in]moduleId: moudule Id eg: CCE
* @param [in/out]enableEvent: point to enableEvent to record enableEvent
* @return: module level(0: debug, 1: info, 2: warning, 3: error, 4: null output)
*/
int dlog_getlevel(int moduleId, int *enableEvent)
{
    if (enableEvent != NULL) {
        *enableEvent = g_enableEvent;
    }

    int moduleLevel = GetLevelByModuleId(moduleId);
    if ((moduleLevel < LOG_MIN_LEVEL) || (moduleLevel > LOG_MAX_LEVEL)) {
        moduleLevel = g_globalLogLevel;
    }
    return moduleLevel;
}

/**
* @brief dlog_setlevel: set module loglevel and enableEvent
* @param [in]moduleId: moudule id(see slog.h, eg: CCE), -1: all modules, others: invalid
* @param [in]level: log level(0: debug, 1: info, 2: warning, 3: error, 4: null output)
* @param [in]enableEvent: 1: enable; 0: disable, others:invalid
* @return: 0: SUCCEED, others: FAILED
*/
int dlog_setlevel(int moduleId, int level, int enableEvent)
{
    int ret = SYS_OK;
    if ((enableEvent == TRUE) || (enableEvent == FALSE)) {
        g_enableEvent = enableEvent;
    } else {
        SELF_LOG_WARN("set loglevel input enableEvent=%d is illegal.", enableEvent);
        ret = SYS_ERROR;
    }

    if ((level < LOG_MIN_LEVEL) || (level > LOG_MAX_LEVEL)) {
        SELF_LOG_WARN("set loglevel input level=%d is illegal.", level);
        return SYS_ERROR;
    }

    if (moduleId == INVALID) {
        g_globalLogLevel = level;
        SetLevelToAllModule(level);
        g_levelSetted = TRUE;
    } else {
        (void)SetLevelByModuleId(moduleId, level);
    }
    return ret;
}

#ifdef __cplusplus
#ifndef LOG_CPP
}
#endif // LOG_CPP
#endif // __cplusplus

#ifdef LOG_CPP
#ifdef __cplusplus
extern "C" {
#endif

int DlogGetlevelForC(int moduleId, int *enableEvent)
{
    return dlog_getlevel(moduleId, enableEvent);
}

int DlogSetlevelForC(int moduleId, int level, int enableEvent)
{
    return dlog_setlevel(moduleId, level, enableEvent);
}

int CheckLogLevelForC(int moduleId, int logLevel)
{
    return CheckLogLevel(moduleId, logLevel);
}

int DlogSetAttrForC(LogAttr logAttr)
{
    return DlogSetAttr(logAttr);
}

void DlogFlushForC(void)
{
    return DlogFlush();
}

void DlogInnerForC(int moduleId, int level, const char *fmt, ...)
{
    if (fmt != NULL) {
        const int result = CheckLogLevel(moduleId, level);
        ONE_ACT_NO_LOG(result == FALSE, return);
        va_list list;
        va_start(list, fmt);
        const KeyValueArg kvArg = { NULL, 0 };
        WriteDlogByIam(moduleId, level, &kvArg, fmt, list);
        va_end(list);
    }
}

void DlogWithKVInnerForC(int moduleId, int level, KeyValue *pstKVArray, int kvNum, const char *fmt, ...)
{
    ONE_ACT_ERR_LOG(pstKVArray == NULL, return, "[input] key-value array is null.");
    ONE_ACT_ERR_LOG(kvNum <= 0, return, "[input] key-value number=%d is invalid.", kvNum);
    if (fmt != NULL) {
        const int result = CheckLogLevel(moduleId, level);
        ONE_ACT_NO_LOG(result == FALSE, return);
        va_list list;
        va_start(list, fmt);
        const KeyValueArg kvArg = { pstKVArray, kvNum };
        WriteDlogByIam(moduleId, level, &kvArg, fmt, list);
        va_end(list);
    }
}

#ifdef __cplusplus
}
#endif
#endif // LOG_CPP

// the code for stub
#if (!defined PROCESS_LOG && !defined PLOG_AUTO)
#include "plog.h"

#ifdef __cplusplus
extern "C" {
#endif
int DlogReportInitialize(void)
{
    return 0;
}

int DlogReportFinalize(void)
{
    return 0;
}
#ifdef __cplusplus
}
#endif
#endif
