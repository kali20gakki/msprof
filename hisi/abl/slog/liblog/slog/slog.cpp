/**
 * @slog.cpp
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "securec.h"
#include "dlog_error_code.h"
#include "slog_common.h"
#if (OS_TYPE_DEF == LINUX)
#include "msg_queue.h"
#else
#include "process_log.h"
#endif

#ifdef HAL_REGISTER_ALOG
#include "process_log.h"
#endif

#ifdef __cplusplus
#ifndef LOG_CPP
extern "C" {
#endif
#endif // __cplusplus

static unsigned int g_connPrintNum = 0;
static unsigned int g_fstatPrintNum = 0;
static unsigned int g_modePrintNum = 0;

time_t g_slogConfigModifyTGime = { 0 };
static toolSockHandle g_logFd = INVALID;
static int g_connectFlag = 0;
static int g_isInited = NO_INITED;
static int g_isMutexInited = 0;
STATIC int g_threadExit = 0;

#ifdef HAL_REGISTER_ALOG
STATIC __attribute__((constructor)) void RegisterHalCallbackInit(void);
STATIC __attribute__((destructor)) void RegisterHalCallbackUninit(void);
#endif

static int g_pid = 0;
#if (OS_TYPE_DEF == LINUX)
__attribute__((constructor)) void dlog_init();
static toolMutex g_slogMutex = PTHREAD_MUTEX_INITIALIZER;
#else
static toolMutex g_slogMutex;
time_t g_currentModifyTime = { 0 };
typedef struct {
    ToolUserBlock *mmThreadArgs;
} ThreadArgs;
#endif

static int g_enableEvent = TRUE;
static int g_logCtrlSwitch = FALSE;
static unsigned int g_writePrintNum = 0;
static struct timespec g_lastTv = { 0, 0 };
static int g_globalLogLevel = GLOABLE_DEFAULT_LOG_LEVEL; // default loglevel(info)
static int g_logCtrlLevel = GLOABLE_DEFAULT_LOG_LEVEL;
static unsigned int g_levelCount[LOG_COUNT_NUM] = { 0, 0, 0, 0 }; // debug, info, warn, error
static LogAttr g_logAttr = { APPLICATION, 0, 0, ""};
static DlogCallback g_dlogCallback = NULL;
static DlogFlushCallback g_dlogFlushCallback = NULL;
static bool g_dlogFlushRegistered = true;
static int g_hasRegistered = FALSE;
static int g_levelSetted = FALSE;

#ifdef HAL_REGISTER_ALOG
/**
 * @brief RegisterHalCallbackInit: register hal callback init
 */
STATIC void RegisterHalCallbackInit(void)
{
    void (*p) (int moduleId, int level, const char *fmt, ...);
    p = DlogInner;
    RegisterHalCallback(p);
}

/**
 * @brief RegisterHalCallbackUninit: register hal callback uninit
 */
STATIC void RegisterHalCallbackUninit(void)
{
    RegisterHalCallback(NULL);
}
#endif

STATIC int IsThreadExit(void)
{
    return g_threadExit;
}

/**
* @brief IsSocketConnected: return socket status
* @return: g_connectFlag
*/
STATIC int IsSocketConnected(void)
{
    return g_connectFlag;
}

/**
* @brief SetSocketConnectedStatus: set g_connectFlag
* @param [in] status: TRUE:socket is connected, FALSE:socket is not connected
* @return: void
*/
STATIC void SetSocketConnectedStatus(int status)
{
    if (status == TRUE || status == FALSE) {
        g_connectFlag = status;
    } else {
        SELF_LOG_WARN("status is invalid. status is %d.", status);
    }
}

/**
* @brief IsSocketFdValid: check g_logFd
* @return: g_logFd<0, return FALSE, others return TRUE.
*/
STATIC int IsSocketFdValid(void)
{
    if (g_logFd < 0) {
        return FALSE;
    } else {
        return TRUE;
    }
}

STATIC int IsLogInited(void)
{
    return g_isInited;
}

STATIC int IsSlogMutexInited(void)
{
    return g_isMutexInited;
}

STATIC void SetSlogMutexInited(int status)
{
    if (status == INITED || status == NO_INITED) {
        g_isMutexInited = status;
    } else {
        SELF_LOG_WARN("status is invalid. status is %d.", status);
    }
}

/**
* @brief SetSocketFd: set g_logFd
* @param [in] fd: socket fd
* @return: void
*/
STATIC void SetSocketFd(int fd)
{
    g_logFd = fd;
}

STATIC INT32 SlogUnlock(void)
{
    int result = ToolMutexUnLock(&g_slogMutex);
    if (result != 0) {
        SELF_LOG_ERROR("unlock slog mutex failed, result=%d, strerr=%s.", result, strerror(ToolGetErrorCode()));
        return FALSE;
    }
    return TRUE;
}

STATIC INT32 SlogLock(void)
{
#if (OS_TYPE_DEF == WIN)
    if (IsSlogMutexInited() == NO_INITED) {
        int result = ToolMutexInit(&g_slogMutex);
        ONE_ACT_ERR_LOG(result != 0, return FALSE, "mutex init fail, result=%d, strerr=%s.",
                        result, strerror(ToolGetErrorCode()));
        SetSlogMutexInited(INITED);
    }
#endif
    LOCK_WARN_LOG(&g_slogMutex);
    return TRUE;
}

/**
* @brief ReadModuleLogLevelFromConfig: read module loglevel from config file
* @return: void
*/
void ReadModuleLogLevelFromConfig(void)
{
    int ret;
    int tmpL;
    int err1, err2, err3;
    int pid = g_pid;
    char val[CONF_VALUE_MAX_LEN + 1] = { 0 };
    char moduleName[MAX_MOD_NAME_LEN + 1] = { 0 };
    const ModuleInfo *moduleInfo = GetModuleInfos();

    for (; moduleInfo->moduleName != NULL; moduleInfo++) {
        err1 = memset_s(val, CONF_VALUE_MAX_LEN + 1, '\0', CONF_VALUE_MAX_LEN + 1);
        err2 = memset_s(moduleName, MAX_MOD_NAME_LEN + 1, '\0', MAX_MOD_NAME_LEN + 1);
        err3 = strcpy_s(moduleName, MAX_MOD_NAME_LEN, moduleInfo->moduleName);
        if (err1 != EOK || err2 != EOK || err3 != EOK) {
            SELF_LOG_WARN("memset_s or strcpy_s failed, errno_1=%d, errno_2=%d, errno_3=%d, pid=%d.",
                          err1, err2, err3, pid);
            continue;
        }

        ret = GetConfValueByList(moduleName, strlen(moduleName), val, CONF_VALUE_MAX_LEN);
        if (ret != SUCCESS || !IsNaturalNumStr(val)) {
            continue;
        }

        tmpL = atoi(val);
        if (tmpL < LOG_MIN_LEVEL || tmpL > LOG_MAX_LEVEL) {
            continue;
        }
        (void)SetLevelByModuleId(moduleInfo->moduleId, tmpL);
    }
    return;
}

/**
* @brief ReadLogLevelFromConfig: read global loglevel and eventEnable from config file
* @return: void
*/
void ReadLogLevelFromConfig(void)
{
    int tmpL = 0;
    int pid = g_pid;
    char val[CONF_VALUE_MAX_LEN + 1] = { 0 };

    // get value of global_level
    int ret = GetConfValueByList(GLOBALLEVEL_KEY, strlen(GLOBALLEVEL_KEY), val, CONF_VALUE_MAX_LEN);
    if (ret == SUCCESS && IsNaturalNumStr(val)) {
        tmpL = atoi(val);
        if (tmpL >= LOG_MIN_LEVEL && tmpL <= LOG_MAX_LEVEL) {
            g_globalLogLevel = tmpL;
        } else {
            SELF_LOG_WARN("global_level=%d is illegal, pid=%d, use value=%d.", tmpL, pid, g_globalLogLevel);
        }
    } else {
        SELF_LOG_WARN("get global_level failed, result=%d, pid=%d, use value=%d.", ret, pid, g_globalLogLevel);
    }

    ret = memset_s(val, CONF_VALUE_MAX_LEN + 1, '\0', CONF_VALUE_MAX_LEN + 1);
    ONE_ACT_WARN_LOG(ret != EOK, return, "memset_s failed, result=%d, strerr=%s, pid=%d.",
                     ret, strerror(ToolGetErrorCode()), pid);

    // get value of enableEvent
    ret = GetConfValueByList(ENABLEEVENT_KEY, strlen(ENABLEEVENT_KEY), val, CONF_VALUE_MAX_LEN);
    if (ret == SUCCESS && IsNaturalNumStr(val)) {
        tmpL = atoi(val);
        if (tmpL == EVENT_DISABLE_VALUE || tmpL == EVENT_ENABLE_VALUE) {
            g_enableEvent = (tmpL == EVENT_ENABLE_VALUE) ? TRUE : FALSE;
        } else {
            SELF_LOG_WARN("enableEvent=%d is illegal, pid=%d, use value=%d.", tmpL, pid, g_enableEvent);
        }
    } else {
        SELF_LOG_WARN("get event switch failed, result=%d, pid=%d, use value=%d.", ret, pid, g_enableEvent);
    }
    return;
}

#if (OS_TYPE_DEF == LINUX)
extern char *__progname;

STATIC void LogCtrlDecLogic(void)
{
    char *pidName = (__progname != NULL) ? __progname : (char *)"None";
    long timeValue = TimeDiff(&g_lastTv);
    if (timeValue >= LOG_WARN_INTERVAL) {
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
            g_logCtrlSwitch = FALSE;
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
    char *pidName = (__progname != NULL) ? __progname : (char *)"None";
    if (g_logCtrlSwitch == FALSE) {
        g_logCtrlSwitch = TRUE;
        g_logCtrlLevel = DLOG_ERROR;
        SELF_LOG_WARN("set log control switch to level=ERROR, pid=%d, pid_name=%s, log loss condition: " \
                      "error_num=%u, warn_num=%u, info_num=%u, debug_num=%u.",
                      g_pid, pidName, g_levelCount[DLOG_ERROR], g_levelCount[DLOG_WARN],
                      g_levelCount[DLOG_INFO], g_levelCount[DLOG_DEBUG]);
    } else if (g_logCtrlLevel < DLOG_ERROR) {
        g_logCtrlLevel++;
        SELF_LOG_WARN("log control up to level=%s, pid=%d, pid_name=%s, log loss condition: " \
                      "error_num=%u, warn_num=%u, info_num=%u, debug_num=%u.",
                      GetBasicLevelNameById(g_logCtrlLevel), g_pid, pidName, g_levelCount[DLOG_ERROR],
                      g_levelCount[DLOG_WARN], g_levelCount[DLOG_INFO], g_levelCount[DLOG_DEBUG]);
    }
    (void)clock_gettime(CLOCK_INPUT, &g_lastTv);
}

static toolSockHandle CreatSocket(void)
{
    struct sockaddr_un sunx;
    int pid = g_pid;
    toolSockHandle sockFd = ToolSocket(AF_UNIX, SOCK_DGRAM | SOCK_NONBLOCK, 0);
    if (sockFd == SYS_ERROR) {
        SELF_LOG_ERROR("create socket failed, strerr=%s, pid=%d.", strerror(ToolGetErrorCode()), pid);
        return SYS_ERROR;
    }

    int nSendBuf = SIZE_TWO_MB;
    int ret = setsockopt(sockFd, SOL_SOCKET, SO_SNDBUF, (const char *)&nSendBuf, sizeof(int));
    if (ret < 0) {
        SELF_LOG_WARN("set socket option failed, strerr=%s, pid=%d.", strerror(ToolGetErrorCode()), pid);
    }

    ret = memset_s(&sunx, sizeof(sunx), 0, sizeof(sunx));
    if (ret != EOK) {
        SELF_LOG_WARN("memset_s failed, result=%d, strerr=%s, pid=%d.", ret, strerror(ToolGetErrorCode()), pid);
    }

    sunx.sun_family = AF_UNIX;
    ret = strcpy_s(sunx.sun_path, sizeof(sunx.sun_path), GetSocketPath());
    if (ret != EOK) {
        SELF_LOG_WARN("strcpy failed, ret=%d, pid=%d.", ret, pid);
    }

    if (ToolConnect(sockFd, (ToolSockAddr *)&sunx, sizeof(sunx)) != SYS_OK) {
        SELF_LOG_WARN_N(&g_connPrintNum, CONN_W_PRINT_NUM,
                        "socket connect failed, perhaps slogd is not ready, " \
                        "strerr=%s, pid=%d, print once every %d times.",
                        strerror(ToolGetErrorCode()), pid, CONN_W_PRINT_NUM);
        ret = ToolClose(sockFd);
        if (ret != SYS_OK) {
            SELF_LOG_WARN("close socket failed, strerr=%s, pid=%d.", strerror(ToolGetErrorCode()), pid);
        }
        sockFd = 0;
        return SYS_ERROR;
    }

    return sockFd;
}

static int SafeWrites(int fd, const void *buf, size_t count, int moduleId, int level)
{
    int n, err;
    int retryTimes = 0;
    char *pidName = (__progname != NULL) ? __progname : (char *)"None";
    do {
        n = ToolWrite(fd, (void *)buf, count);
        err = ToolGetErrorCode();
        if (n < 0) {
            if (err == EINTR) {
                continue;
            } else if ((err == EAGAIN) && (level == DLOG_ERROR)) {
                retryTimes++;
                LogCtrlIncLogic();
                continue;
            }
            break;
        }
    } while ((n < 0) && (retryTimes != WRITE_MAX_RETRY_TIMES));

    if (n > 0 && g_logCtrlSwitch) {
        LogCtrlDecLogic();
    } else if (n < 0) {
        g_levelCount[level]++;
        g_writePrintNum++;
        if ((g_writePrintNum % WRITE_E_PRINT_NUM) == 0) {
            SELF_LOG_WARN("write failed, print every %d times, result=%d, strerr=%s, pid=%d, pid_name=%s, module=%d, " \
                          "log loss condition: error_num=%u, warn_num=%u, info_num=%u, debug_num=%u.",
                          WRITE_E_PRINT_NUM, n, strerror(err), g_pid, pidName, moduleId,
                          g_levelCount[DLOG_ERROR], g_levelCount[DLOG_WARN], g_levelCount[DLOG_INFO],
                          g_levelCount[DLOG_DEBUG]);
            g_writePrintNum = 0;
        }
    }
    return n;
}
#else
// loglevel read threadproc for windows
ArgPtr ReadLogLevelProc(ArgPtr arg)
{
    ToolStat stat = { 0 };
    time_t lastModifyTime = { 0 };
    ThreadArgs* args = (ThreadArgs *)arg;

    if (args == NULL) {
        SELF_LOG_WARN("quit thread(ReadLogLevelProc), args null");
        return NULL;
    }
    while (!IsThreadExit()) {
        if (ToolStatGet(SLOG_CONF_FILE_PATH, &stat) != SYS_OK) {
            ToolSleep(TIME_LOOP_READ_CONFIG);
            continue;
        }

        lastModifyTime = g_currentModifyTime;
        g_currentModifyTime = stat.st_mtime;

        // if last modify time is equal to current modify time, no need to read again
        if (g_currentModifyTime == lastModifyTime) {
            ToolSleep(TIME_LOOP_READ_CONFIG);
            continue;
        }

        // read new log level, include global\module\event
        if (InitConfList(SLOG_CONF_FILE_PATH) != SUCCESS) {
            SELF_LOG_WARN("Init conf list failed");
            FreeConfList();
            continue;
        }
        ReadLogLevelFromConfig();
        ReadModuleLogLevelFromConfig();
        FreeConfList();
        ToolSleep(TIME_LOOP_READ_CONFIG);
    }
    SELF_LOG_WARN("quit thread(ReadLogLevelProc), g_threadExit=%d", g_threadExit);
    FreeSelfLogFiles();
    XFREE(args->mmThreadArgs);
    XFREE(args);
    return NULL;
}

#define LIBSLOG_SOCKET_PORT 8008
#define LIBSLOG_SOCKET_ADDR "127.0.0.1"
static toolSockHandle CreatSocket(void)
{
    SetSocketConnectedStatus(FALSE);
    if (ToolSAStartup() != SYS_OK) {
        ToolSACleanup();
        return SYS_ERROR;
    }
    toolSockHandle sockFd = ToolSocket(AF_INET, SOCK_DGRAM, 0);
    if (sockFd == SYS_ERROR) {
        SELF_LOG_ERROR("create socket failed, strerr=%s, pid=%d.", strerror(ToolGetErrorCode()), g_pid);
        ToolSACleanup();
        return SYS_ERROR;
    }

    struct sockaddr_in clientAddr;
    (void)memset_s(&clientAddr, sizeof(clientAddr), 0, sizeof(clientAddr));
    clientAddr.sin_family = AF_INET;
    clientAddr.sin_port = htons(0); // Random alloc port if set to zero by kernal
    if ((inet_pton(AF_INET, LIBSLOG_SOCKET_ADDR, &clientAddr.sin_addr)) <= 0) {
        SELF_LOG_ERROR("convert address failed, strerr=%s, pid=%d.", strerror(ToolGetErrorCode()), g_pid);
        ToolCloseSocket(sockFd);
        ToolSACleanup();
        return SYS_ERROR;
    }

    if (ToolBind(sockFd, (ToolSockAddr *)&clientAddr, sizeof(clientAddr)) != SYS_OK) {
        LOG_CLOSE_SOCK_S(sockFd);
        SELF_LOG_ERROR("bind socket failed, strerr=%s, pid=%d.", strerror(ToolGetErrorCode()), g_pid);
        ToolCloseSocket(sockFd);
        ToolSACleanup();
        return SYS_ERROR;
    }
    SetSocketConnectedStatus(TRUE);
    return sockFd;
}

static int SafeWrites(int fd, const void *buf, size_t count, int moduleId, int level)
{
    ONE_ACT_ERR_LOG(buf == NULL, return SYS_ERROR, "[input] buffer is NULL.");
    if (fd == ToolFileno(stdout)) {
        return ToolWrite(fd, (void *)buf, count);
    } else {
        struct sockaddr_in serverAddr;
        (void)memset_s(&serverAddr, sizeof(serverAddr), 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(LIBSLOG_SOCKET_PORT); // host's 8008 port convert to network byte
        if ((inet_pton(AF_INET, LIBSLOG_SOCKET_ADDR, &serverAddr.sin_addr)) <= 0) {
            SELF_LOG_ERROR("convert address failed, strerr=%s, pid=%d, module=%d.",
                           strerror(ToolGetErrorCode()), g_pid, moduleId);
            return SYS_ERROR;
        }
        return sendto(fd, (const char *)buf, count, 0, (const struct sockaddr *)&serverAddr, sizeof(serverAddr));
    }
}
#endif

STATIC int FullWrites(int fd, const void *buf, size_t len, int moduleId, int level)
{
    int cc;
    int total = 0;

    while (len) {
        cc = SafeWrites(fd, buf, len, moduleId, level);
        if (cc < 0) {
            if (total) {
                return total;
            }
            return cc;
        }

        buf = ((const char *)buf) + cc;
        if ((unsigned int)len >= (unsigned int)cc) {
            total += cc;
            len -= cc;
        } else {
            break;
        }
    }
    return total;
}

STATIC void GetTime(char *timeStr, unsigned int size)
{
    ToolTimeval currentTimeval = { 0, 0 };
    int pid = g_pid;
    struct tm timInfo;
    (void)memset_s(&timInfo, sizeof(timInfo), 0, sizeof(timInfo));
    ONE_ACT_ERR_LOG(timeStr == NULL, return, "[input] time is null.");

#if (OS_TYPE_DEF == 0)
    if (ToolGetTimeOfDay(&currentTimeval, NULL) != SYS_OK) {
        SELF_LOG_WARN("get time of day failed, strerr=%s, pid=%d.", strerror(ToolGetErrorCode()), pid);
        return;
    }
    if (GetLocaltimeR(&timInfo, currentTimeval.tv_sec) != SYS_OK) {
        SELF_LOG_WARN("get local time failed, strerr=%s, pid=%d.", strerror(ToolGetErrorCode()), pid);
        return;
    }
#else
    // sync time zone
    tzset();
    if (ToolGetTimeOfDay(&currentTimeval, NULL) != SYS_OK) {
        SELF_LOG_WARN("get time of day failed, errno=%s, pid=%d.", strerror(ToolGetErrorCode()), pid);
        return;
    }
    const time_t sec = currentTimeval.tv_sec;
    if (ToolLocalTimeR(&sec, &timInfo) != SYS_OK) {
        SELF_LOG_WARN("get local time failed, errno=%s, pid=%d.", strerror(ToolGetErrorCode()), pid);
        return;
    }
#endif

    int err = snprintf_s(timeStr, size, size - 1, "%04d-%02d-%02d-%02d:%02d:%02d.%03ld.%03ld",
                         (timInfo.tm_year), timInfo.tm_mon, timInfo.tm_mday, timInfo.tm_hour, timInfo.tm_min,
                         timInfo.tm_sec, currentTimeval.tv_usec / TIME_ONE_THOUSAND_MS,
                         currentTimeval.tv_usec % TIME_ONE_THOUSAND_MS);
    if (err == -1) {
        SELF_LOG_WARN("snprintf_s time failed, result=%d, strerr=%s, pid=%d.", err, strerror(ToolGetErrorCode()), pid);
    }
    return;
}

STATIC int IsWriteConsole(void)
{
    char *envres = {0};
    envres = getenv("ASCEND_SLOG_PRINT_TO_STDOUT");
    if (envres != NULL) {
        if (strcmp(envres, WRITE_TO_CONSOLES) == 0) {
            return TRUE;
        }
    }
    return FALSE;
}

STATIC void InitEventLevelByEnv(void)
{
    char *eventEnv = getenv("ASCEND_GLOBAL_EVENT_ENABLE");
    if ((eventEnv != NULL) && (IsNaturalNumStr(eventEnv))) {
        int tmpL = atoi(eventEnv);
        if (tmpL == TRUE || tmpL == FALSE) {
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
    char *env = getenv("ASCEND_GLOBAL_LOG_LEVEL");
    // global level env is prior to config file
    if ((env != NULL) && (IsNaturalNumStr(env))) {
        int tmpL = atoi(env);
        if (tmpL >= LOG_MIN_LEVEL && tmpL <= LOG_MAX_LEVEL) {
            g_globalLogLevel = tmpL;
            SetLevelToAllModule(tmpL);
            SELF_LOG_INFO("get right env ASCEND_GLOBAL_LOG_LEVEL(%d).", tmpL);
            return;
        }
    }
    g_globalLogLevel = GLOABLE_DEFAULT_LOG_LEVEL;
    SetLevelToAllModule(GLOABLE_DEFAULT_LOG_LEVEL);
    SELF_LOG_INFO("set default global log level (%d).", GLOABLE_DEFAULT_LOG_LEVEL);
}

#if (OS_TYPE_DEF == 0)
STATIC int GetModuleLevel(char levelChar, unsigned int off)
{
    ONE_ACT_NO_LOG(levelChar < 0, return g_globalLogLevel);

    int level = (int)((((unsigned char)levelChar) >> off) & 0x7) - 1; // (0111 0000) or (0000 0111)
    if (level < LOG_MIN_LEVEL || level > LOG_MAX_LEVEL) {
        level = g_globalLogLevel;
    }
    return level;
}

/**
 * @brief ParseGlobalLevel: parse global and event level
 * @param [in]levelStr: level string from shmem
 * @return: void
 */
STATIC void ParseGlobalLevel(const char *levelStr)
{
    ONE_ACT_NO_LOG(levelStr == NULL, return);
    // global
    g_globalLogLevel = GetGlobalLevel(levelStr[0]);
    // event
    g_enableEvent = GetEventLevel(levelStr[0]);
}

/**
 * @brief ParseModuleLevel: parse module level to global struct value
 * @param [in]levelStr: level string from shmem
 * @param [in]num: module num in global array
 * @return: module level value;
 */
STATIC int ParseModuleLevel(const char *levelStr, int num)
{
    ONE_ACT_NO_LOG(levelStr == NULL, return g_globalLogLevel);

    int off = ((num + 1) % 2) * 4; // high 6~4bit, low 2~0bit
    int sub = num / 2 + 1;
    if (sub < 0 || (unsigned int)sub >= (unsigned int)strlen(levelStr)) {
        return g_globalLogLevel; // use global level
    }

    return GetModuleLevel(levelStr[sub], (unsigned int)off); // off > 0
}

/**
 * @brief ParseLogLevel: parse log level to global struct value
 *  levelStr format:      0111 |         11 |         00 |        0111 |       0111 |        0111 |...
 *                      global |      event |    reserve |   module[0] |  module[1] |   module[2] |...
 *                 high 6~4bit | low 3~2bit | low 1~0bit | high 6~4bit | low 2~0bit | high 6~4bit |...
 * @param [in]levelStr: level string from shmem
 * @param [in]moduleStr: module string from shmem
 * @return: SUCCESS: parse success; ARGV_NULL: param is null;
 */
STATIC LogRt ParseLogLevel(const char *levelStr, const char *moduleStr)
{
    ONE_ACT_NO_LOG(levelStr == NULL, return ARGV_NULL);
    ONE_ACT_NO_LOG(moduleStr == NULL, return ARGV_NULL);

    // update global and event level
    ParseGlobalLevel(levelStr);

    // update module level
    int i = 0;
    int n = 0;
    int off = 0;
    int offPre = 0;
    int len = strlen(moduleStr);
    const ModuleInfo *moduleInfos = GetModuleInfos();
    while (off < len) {
        if (moduleStr[off] != ';') {
            off++;
            continue;
        }
        int pre = i;
        for (; i < INVLID_MOUDLE_ID; i++) {
            const char *name  = moduleInfos[i].moduleName;
            if (strncmp(name, moduleStr + offPre, strlen(name)) == 0) {
                int level = ParseModuleLevel(levelStr, n); // parse n module level
                SetLevelByModuleId(moduleInfos[i].moduleId, level);
                break;
            }
        }
        if (i == INVLID_MOUDLE_ID) {
            i = pre;
        }
        n++; // next module in moduleStr
        off++; // ';' next
        offPre = off;
    }
    return SUCCESS;
}

/**
 * @brief: watch the file '/usr/slog/level_notify', when the file changed, then update log level.
 * @param: NULL
 * @return: (void*)NULL
 */
STATIC int UpdateLogLevel()
{
    int res;
    int shmId = -1;
    char *moduleStr = NULL;
    char *levelStr = NULL;

    ShmErr ret = OpenShMem(&shmId);
    if (ret == SHM_ERROR) {
        SELF_LOG_WARN("open shmem failed, pid=%d, strerr=%s.", g_pid, strerror(ToolGetErrorCode()));
        return SYS_ERROR;
    }
    // read module info
    res = ReadStrFromShm(shmId, &moduleStr, MODULE_ARR_LEN, PATH_LEN);
    if (res != SYS_OK) {
        SELF_LOG_WARN("read module from shmem failed, pid=%d.", g_pid);
        return SYS_ERROR;
    }
    // read level info
    res = ReadStrFromShm(shmId, &levelStr, LEVEL_ARR_LEN, PATH_LEN + MODULE_ARR_LEN);
    if (res != SYS_OK) {
        SELF_LOG_WARN("read level from shmem failed, pid=%d.", g_pid);
        XFREE(moduleStr);
        return SYS_ERROR;
    }

    // parse level and update to global value
    LogRt err = ParseLogLevel(levelStr, moduleStr);
    XFREE(levelStr);
    XFREE(moduleStr);
    if (err != SUCCESS) {
        SELF_LOG_WARN("parse level failed, log_err=%d, pid=%d.", err, g_pid);
        return SYS_ERROR;
    }
    return SYS_OK;
}

STATIC void LogLevelInit(void)
{
#ifdef LOG_CPP
    InitLogLevelByEnv();
    InitEventLevelByEnv();
#else
    int res = UpdateLogLevel();
    if (res != SYS_OK) {
        ReadLogLevelFromConfig();
        ReadModuleLogLevelFromConfig();
    }
#endif
}
#endif

static int FullWriteFile(char *msg, int fd, int moduleId, int level)
{
#ifndef __IDE_UT
    struct stat st;
    (void)memset_s(&st, sizeof(st), 0, sizeof(st));
    int pid = g_pid;
    size_t len = strlen(msg);

    ONE_ACT_ERR_LOG(msg == NULL, return -1, "[input] message_buffer is null.");
    ONE_ACT_ERR_LOG(fd <= 0, return -1, "[input] file_handle is invalid, file_handle=%d.", fd);
    int err = 0;
    if (IsWriteConsole()) {
        goto WRITE_CONSOLE;
    }
    err = fstat(fd, &st);
    if (err == -1) {
        SELF_LOG_ERROR_N(&g_fstatPrintNum, FSTAT_E_PRINT_NUM,
                         "get file state failed, strerr=%s, pid=%d, print once every %d times.",
                         strerror(ToolGetErrorCode()), pid, FSTAT_E_PRINT_NUM);
        return -1;
    }

    if ((S_IFMT & st.st_mode) != S_IFCHR) {
        SELF_LOG_ERROR_N(&g_modePrintNum, FSTAT_E_PRINT_NUM,
                         "file mode is invalid, strerr=%s, pid=%d, print once every %d times.",
                         strerror(ToolGetErrorCode()), pid, FSTAT_E_PRINT_NUM);
        return -1;
    }

WRITE_CONSOLE:
     if (len > 1 && msg[len - 1] != '\n') {
        msg[MSG_LENGTH - 2] = '\n'; // 2 for \n. 1 for \0.
        err = snprintf_truncated_s(msg, MSG_LENGTH, "%s\n", msg);
        ONE_ACT_WARN_LOG(err == -1, return -1, "copy failed, strerr=%s.", strerror(ToolGetErrorCode()));
    }
    (void)FullWrites(fd, msg, strlen(msg), moduleId, level); // do not check, because print a lot
    return 0;
#else
    return -1;
#endif
}

STATIC void CloseLogInternal(void)
{
    int pid = g_pid;
    if (!IsSocketConnected()) {
        return;
    }

    if (ToolClose(g_logFd) != SYS_OK) {
        SELF_LOG_WARN("close socket failed, strerr=%s, pid=%d.", strerror(ToolGetErrorCode()), pid);
    }
    SetSocketFd(INVALID);
    SetSocketConnectedStatus(FALSE);
}

STATIC void SigPipeHandler(int signo)
{
    (void)signo;
    CloseLogInternal();
}

/**
* @brief CheckLogLevel: check log allow output or not
* @param [in]moduleId: module Id
* @param [in]logLevel: log level
* @return: TRUE/FALSE
*/
int CheckLogLevel(int moduleId, int logLevel)
{
    if (logLevel == DLOG_TRACE || logLevel == DLOG_OPLOG) {
        return TRUE;
    }

    // event log contrl
    if (logLevel == DLOG_EVENT) {
        return (g_enableEvent == TRUE) ? TRUE : FALSE;
    }

    const ModuleInfo *moduleInfos = GetModuleInfos();
    // get module loglevel by moduleId
    int moduleLevel = g_globalLogLevel;
    if (moduleId >= 0 && moduleId < INVLID_MOUDLE_ID) {
        moduleLevel = moduleInfos[moduleId].moduleLevel;
        if (moduleLevel < LOG_MIN_LEVEL || moduleLevel > LOG_MAX_LEVEL) {
            moduleLevel = g_globalLogLevel;
        }
    }

    // check module loglevel and log control, check time diff to make switch back to false
    if (logLevel >= moduleLevel && logLevel < LOG_MAX_LEVEL) {
#if (OS_TYPE_DEF == 0)
        if (g_logCtrlSwitch && (TimeDiff(&g_lastTv) <= LOG_CTRL_TOTAL_INTERVAL)) {
            TWO_ACT_NO_LOG(logLevel < g_logCtrlLevel, g_levelCount[logLevel]++, return FALSE);
        }
#endif
        return TRUE;
    }
    return FALSE;
}


/**
* @brief WriteLogToSocket: write log message to socket fd
                           capture SIGPIPE, prepare for a broken connection
* @param [in]msg: log message
* @param [in]len: length of log message
* @param [in]moduleId: module Id
*/
#if (OS_TYPE_DEF == 0)
STATIC void WriteLogToSocket(const char *msg, int len, int moduleId, int level)
{
    int pid = g_pid;
    if (msg == NULL || len == 0) {
        SELF_LOG_WARN("[input] input is null or length is invalid, length=%d, pid=%d, module=%d.",
                      len, pid, moduleId);
        return;
    }

    struct sigaction action, oldaction;
    (void)memset_s(&oldaction, sizeof(oldaction), 0, sizeof(oldaction));
    (void)memset_s(&action, sizeof(action), 0, sizeof(action));

    action.sa_handler = SigPipeHandler;
    int result = sigemptyset(&action.sa_mask);
    ONE_ACT_WARN_LOG(result < 0, return, "call sigemptyset failed, result=%d, strerr=%s.",
                     result, strerror(ToolGetErrorCode()));
    int sigpipe = sigaction(SIGPIPE, &action, &oldaction);
    if (FullWrites(g_logFd, msg, len, moduleId, level) < 0) {
        CloseLogInternal();
    }
    if (sigpipe == 0) {
        if (sigaction(SIGPIPE, &oldaction, (struct sigaction *)NULL) < 0) {
            SELF_LOG_WARN("examine and change a signal action failed, strerr=%s, pid=%d, module=%d.",
                          strerror(ToolGetErrorCode()), pid, moduleId);
        }
    }
}
#else
STATIC void WriteLogToSocket(const char *msg, int len, int moduleId, int level)
{
    if (msg == NULL || len == 0) {
        SELF_LOG_WARN("[input] input is null or length=%d is invalid, pid=%d, module=%d.",
                      len, g_pid, moduleId);
        return;
    }

    if (FullWrites(g_logFd, msg, len, moduleId, level) < 0) {
        CloseLogInternal();
    }
}
#endif

STATIC int InitLogAndCheckLogLevel(int moduleId, int level)
{
    if (!IsLogInited()) {
        dlog_init();
        if (!CheckLogLevel(moduleId, level)) {
            return FALSE;
        }
    }
    return TRUE;
}

#if (OS_TYPE_DEF == 0)
/**
 * @brief: watch the file '/usr/slog/level_notify', when the file changed, then update log level.
 * @param: NULL
 * @return: (void*)NULL
 */
STATIC void DoUpdateLogLevel()
{
    if (g_logAttr.type == SYSTEM) {
        (void)UpdateLogLevel();
    }
}

void *LevelNotifyWatcher(void *)
{
    NO_ACT_WARN_LOG(SetThreadName("LevelNotifyWatcher") != 0,
                    "set thread_name(LevelNotifyWatcher) failed, pid=%d.", g_pid);

    int res;
    int notifyFd = INVALID;
    int watchFd = INVALID;
    char notifyFile[CFG_WORKSPACE_PATH_MAX_LENGTH] = { 0 };

    int len = strlen(GetWorkspacePath()) + strlen(LEVEL_NOTIFY_FILE) + 1;
    res = sprintf_s(notifyFile, CFG_WORKSPACE_PATH_MAX_LENGTH - 1, "%s/%s", GetWorkspacePath(), LEVEL_NOTIFY_FILE);
    ONE_ACT_ERR_LOG(res != len, return NULL,
                    "copy path failed, res=%d, strerr=%s, pid=%d, Thread(LevelNotifyWatcher) quit.",
                    res, strerror(ToolGetErrorCode()), g_pid);

    LogRt err = AddNewWatch(&notifyFd, &watchFd, notifyFile);
    ONE_ACT_ERR_LOG(err != SUCCESS, return NULL,
                    "add watcher failed, err=%d, pid=%d, Thread(LevelNotifyWatcher) quit.", err, g_pid);

    char eventBuf[MAX_INOTIFY_BUFF] = { 0 };
    struct inotify_event *event = NULL;
    while (1) {
        ssize_t numRead = read(notifyFd, eventBuf, MAX_INOTIFY_BUFF);
        ONE_ACT_WARN_LOG(numRead <= 0, continue, "read watcher event failed, res=%d, strerr=%s, pid=%d, but continue.",
                         numRead, strerror(ToolGetErrorCode()), g_pid);

        char *tmpBuf = eventBuf;
        for (; tmpBuf < (eventBuf + numRead);) {
            event = (struct inotify_event *)tmpBuf;
            if (event->mask & IN_CLOSE_WRITE) {
                DoUpdateLogLevel();
            } else if (event->mask & IN_DELETE_SELF) {
                err = AddNewWatch(&notifyFd, &watchFd, (const char *)notifyFile);
                ONE_ACT_NO_LOG(err != SUCCESS, break);
                DoUpdateLogLevel();
            }
            tmpBuf += sizeof(struct inotify_event) + event->len;
        }
    }

    SELF_LOG_ERROR("Thread(LevelNotifyWatcher) quit, pid=%d.", g_pid);
    res = inotify_rm_watch(notifyFd, watchFd);
    NO_ACT_WARN_LOG(res != 0, "remove inotify failed, res=%d, strerr=%s, pid=%d",
                    res, strerror(ToolGetErrorCode()), g_pid);
    LOG_MMCLOSE_AND_SET_INVALID(notifyFd);
    return NULL;
}

/**
 * @brief: start one thread to watch the file '/usr/slog/level_notify' and read level from shmem.
 */
STATIC void StartThreadForLevelChangeWatcher()
{
#ifndef LOG_CPP
    // start thread
    ToolUserBlock thread;
    thread.procFunc = LevelNotifyWatcher;
    thread.pulArg = NULL;

    toolThread tid = 0;
    ONE_ACT_ERR_LOG(ToolCreateTaskWithDetach(&tid, &thread) != SYS_OK, return,
                    "create task LevelWatcher failed, strerr=%s, pid=%d.",
                    strerror(ToolGetErrorCode()), g_pid);
#endif
}
#endif

#if (OS_TYPE_DEF == 0)
ArgPtr FlushAppLog(ArgPtr args)
{
    (void)args;
    NO_ACT_WARN_LOG(SetThreadName("alogFlush") != 0, "set thread_name(alogFlush) failed, pid=%d.", g_pid);

    const long maxTimeval = 200; // max 200ms
    long timeval = 0;
    struct timespec itv = { 0, 0 };
    (void)clock_gettime(CLOCK_INPUT, &itv);
    while (!IsThreadExit()) {
        timeval = TimeDiff(&itv);
        if (timeval < maxTimeval) {
            ToolSleep(50); // sleep 50 ms
            continue;
        }
        if (g_dlogFlushCallback != NULL) {
            // flush alog to disk
            SlogLock();
            if (g_dlogFlushRegistered && g_dlogFlushCallback != NULL) {
                g_dlogFlushCallback();
            }
            SlogUnlock();
        }
        (void)clock_gettime(CLOCK_INPUT, &itv);
    }

    SELF_LOG_ERROR("Thread(alogFlush) quit, pid=%d.", g_pid);
    return NULL;
}

/**
 * @brief: start one thread to flush log to disk with timer
 */
STATIC void StartThreadForLogFlush()
{
#ifdef LOG_CPP
    // start thread
    ToolUserBlock thread;
    thread.procFunc = FlushAppLog;
    thread.pulArg = NULL;

    toolThread tid = 0;
    ONE_ACT_ERR_LOG(ToolCreateTaskWithDetach(&tid, &thread) != SYS_OK, return,
                    "create task FlushAppLog failed, strerr=%s, pid=%d.",
                    strerror(ToolGetErrorCode()), g_pid);
#endif // LOG_CPP
}
#else // OS_TYPE_DEF
STATIC ArgPtr FlushAppLog(ArgPtr args)
{
    NO_ACT_WARN_LOG(SetThreadName("alogFlush") != 0, "set thread_name(alogFlush) failed, pid=%d.", g_pid);

    ThreadArgs *threadArgs = (ThreadArgs *)args;
    ONE_ACT_ERR_LOG(threadArgs == NULL, return NULL, "Thread(alogFlush) quit, args invalid, pid=%d.", g_pid);

    const int maxTimeval = 200; // max 200ms
    const int sleepTime = 50; // sleep 50ms every time
    int timeval = 0;
    while (!IsThreadExit()) {
        if (timeval < maxTimeval) {
            ToolSleep(sleepTime); // sleep 50 ms
            timeval += sleepTime;
            continue;
        }
        if (g_dlogFlushCallback != NULL) {
            // flush alog to disk
            SlogLock();
            if (g_dlogFlushCallback != NULL) {
                g_dlogFlushCallback();
            }
            SlogUnlock();
        }
        timeval = 0;
    }

    XFREE(threadArgs->mmThreadArgs);
    XFREE(threadArgs);
    SELF_LOG_ERROR("Thread(alogFlush) quit, pid=%d.", g_pid);
    return NULL;
}

/**
 * @brief: start one thread to flush log to disk with timer
 */
STATIC void StartThreadForLogFlush()
{
#ifdef LOG_CPP
    ToolUserBlock* userBlock = (ToolUserBlock *)malloc(sizeof(ToolUserBlock));
    ThreadArgs* args = (ThreadArgs *)malloc(sizeof(ThreadArgs));
    if (userBlock == NULL || args == NULL) {
        XFREE(userBlock);
        XFREE(args);
        SELF_LOG_WARN("malloc failed, strerr=%s.", strerror(ToolGetErrorCode()));
        return;
    }

    (void)memset_s(userBlock, sizeof(ToolUserBlock), 0, sizeof(ToolUserBlock));
    (void)memset_s(args, sizeof(ThreadArgs), 0, sizeof(ThreadArgs));
    args->mmThreadArgs = userBlock;
    userBlock->procFunc = FlushAppLog;
    userBlock->pulArg = (void *)args;

    toolThread thread;
    if (ToolCreateTaskWithDetach(&thread, userBlock) != SYS_OK) {
        SELF_LOG_ERROR("create thread(FlushAppLog) failed, strerr=%s, pid=%d.",
                       strerror(ToolGetErrorCode()), g_pid);
        XFREE(userBlock);
        XFREE(args);
    }
#endif // LOG_CPP
}
#endif // OS_TYPE_DEF

/**
 * @brief: check sub process, restart thread
 * @return: void
 */
STATIC void CheckPid(void)
{
#if (OS_TYPE_DEF == 0)
    if (g_pid != ToolGetPid()) {
        StartThreadForLevelChangeWatcher();
        StartThreadForLogFlush();
        g_pid = ToolGetPid();
    }
#endif
}

static int ConstructBaseLogForValidModuleId(char *msg, unsigned int msgLen, LogMsgArg msgArg)
{
    int err;
#if (OS_TYPE_DEF == 0)
    const char *pidName = (__progname != NULL) ? __progname : "None";
#else
    const char *pidName = "None";
#endif
    if ((msgArg.level >= DLOG_DEBUG && msgArg.level <= DLOG_OPLOG) || msgArg.level == DLOG_EVENT) {
        if (g_dlogCallback != NULL || IsWriteConsole()) {
            err = snprintf_s(msg, msgLen, msgLen - 1, "[%s] %s(%d,%s):%s ",
                             GetLevelNameById(msgArg.level), GetModuleNameById(msgArg.moduleId),
                             ToolGetPid(), pidName, msgArg.timestamp);
        } else {
            err = snprintf_s(msg, msgLen, msgLen - 1, "[%d,%u,%u][%s] %s(%d,%s):%s ",
                             g_logAttr.type, g_logAttr.pid, g_logAttr.deviceId,
                             GetLevelNameById(msgArg.level), GetModuleNameById(msgArg.moduleId),
                             ToolGetPid(), pidName, msgArg.timestamp);
        }
    } else {
        if (g_dlogCallback != NULL || IsWriteConsole()) {
            err = snprintf_s(msg, msgLen, msgLen - 1, "[%d] %s(%d,%s):%s ",
                             msgArg.level, GetModuleNameById(msgArg.moduleId),
                             ToolGetPid(), pidName, msgArg.timestamp);
        } else {
            err = snprintf_s(msg, msgLen, msgLen - 1, "[%d,%u,%u][%d] %s(%d,%s):%s ",
                             g_logAttr.type, g_logAttr.pid, g_logAttr.deviceId,
                             msgArg.level, GetModuleNameById(msgArg.moduleId),
                             ToolGetPid(), pidName, msgArg.timestamp);
        }
    }
    return err;
}

static int ConstructBaseLogForInvalidModuleId(char *msg, unsigned int msgLen, LogMsgArg msgArg)
{
    int err;
#if (OS_TYPE_DEF == 0)
    const char *pidName = (__progname != NULL) ? __progname : "None";
#else
    const char *pidName = "None";
#endif
    if ((msgArg.level >= DLOG_DEBUG && msgArg.level <= DLOG_OPLOG) || msgArg.level == DLOG_EVENT) {
        if (g_dlogCallback != NULL || IsWriteConsole()) {
            err = snprintf_s(msg, msgLen, msgLen - 1, "[%s] %d(%d,%s):%s ",
                             GetLevelNameById(msgArg.level), msgArg.moduleId,
                             ToolGetPid(), pidName, msgArg.timestamp);
        } else {
            err = snprintf_s(msg, msgLen, msgLen - 1, "[%d,%u,%u][%s] %d(%d,%s):%s ",
                             g_logAttr.type, g_logAttr.pid, g_logAttr.deviceId,
                             GetLevelNameById(msgArg.level), msgArg.moduleId,
                             ToolGetPid(), pidName, msgArg.timestamp);
        }
    } else {
        if (g_dlogCallback != NULL || IsWriteConsole()) {
            err = snprintf_s(msg, msgLen, msgLen - 1, "[%d] %d(%d,%s):%s ",
                             msgArg.level, msgArg.moduleId, ToolGetPid(), pidName, msgArg.timestamp);
        } else {
            err = snprintf_s(msg, msgLen, msgLen - 1, "[%d,%u,%u][%d] %d(%d,%s):%s ",
                             g_logAttr.type, g_logAttr.pid, g_logAttr.deviceId,
                             msgArg.level, msgArg.moduleId, ToolGetPid(), pidName, msgArg.timestamp);
        }
    }
    return err;
}

/**
* @brief ConstructBaseLogMsg: construct base log message
* @param [in]msg: log message
* @param [in]msgLen: log message max length
* @param [in]msgArg: log info, include level, module, and key-value
* @return SYS_OK/SYS_ERROR
*/
static int ConstructBaseLogMsg(char *msg, unsigned int msgLen, LogMsgArg msgArg)
{
    int err;
#if (OS_TYPE_DEF == 0)
    const char *pidName = (__progname != NULL) ? __progname : "None";
#else
    const char *pidName = "None";
#endif
    if (msgArg.moduleId >= 0 && msgArg.moduleId < INVLID_MOUDLE_ID) {
        err = ConstructBaseLogForValidModuleId(msg, msgLen, msgArg);
    } else {
        err = ConstructBaseLogForInvalidModuleId(msg, msgLen, msgArg);
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
STATIC int ConstructLogMsg(char *msg, unsigned int msgLen, LogMsgArg msgArg, const char *fmt, va_list v)
{
    ONE_ACT_NO_LOG(msg == NULL, return SYS_ERROR);
    ONE_ACT_NO_LOG(msgLen == 0 || msgLen > MSG_LENGTH, return SYS_ERROR);
    ONE_ACT_NO_LOG(fmt == NULL, return SYS_ERROR);

    int i;
    int len;
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
        err = snprintf_s(msg + len, msgLen - len, msgLen - len - 1, "[%s:%s] ",
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

STATIC void RemoveMessageTag(char **input)
{
    char *p = *input;
    if (*p == '[') {
        p++;
        while (*p != ']') {
            p++;
        }
    }
    if (*p == ']') {
        p++;
    }
    *input = p;
}

STATIC int WritePLog(char *msg, unsigned int len)
{
    int ret;
    if (len > 1 && msg[len - 1] != '\n') {
        msg[MSG_LENGTH - 2] = '\n'; // 2 for \n. 1 for \0.
        ret = snprintf_truncated_s(msg, MSG_LENGTH, "%s\n", msg);
        ONE_ACT_WARN_LOG(ret == -1, return FALSE, "copy failed, strerr=%s.", strerror(ToolGetErrorCode()));
    }
    ret = g_dlogCallback(msg, strlen(msg));
    if (ret != 0) {
        return FALSE;
    }
    return TRUE;
}

/**
* @brief WriteDlog: write log to log socket or stdout
* @param [in]moduleId: module id number
* @param [in]level: log leve number
* @param [in]kvArg: pointer to struct that contains key-value arrary and size of this array
* @param [in]fmt: pointer to first value in va_list
* @param [in]v: variable list
*/
static void WriteDlog(int moduleId, int level, const KeyValueArg *kvArg, const char *fmt, va_list v)
{
    int result;
    int fd = ToolFileno(stdout);
    char msg[MSG_LENGTH] = { 0 };
    char copymsg[MSG_LENGTH] = { 0 };
    char timestamp[TIMESTAMP_LEN] = { 0 };
    char *newMsg = msg;

    // construct log content
    GetTime(timestamp, TIMESTAMP_LEN);
    LogMsgArg msgArg = { moduleId, level, (char *)timestamp, *kvArg };
    result = ConstructLogMsg(msg, sizeof(msg), msgArg, fmt, v);
    ONE_ACT_WARN_LOG(result != SYS_OK, return, "construct log content failed.");
    msg[MSG_LENGTH - 1] = '\0';

    // lock, To prevent the leaked of file handle.(socket)
    result = SlogLock();
    ONE_ACT_NO_LOG(result == FALSE, return);
    // if callback from not null to null, discarding log
    TWO_ACT_NO_LOG(((g_hasRegistered == TRUE) && (g_dlogCallback == NULL)), (void)SlogUnlock(), return);
    CheckPid();

    // check log level and log inited status
    result = InitLogAndCheckLogLevel(moduleId, level);
    TWO_ACT_NO_LOG(result == FALSE, (VOID)SlogUnlock(), return);
    ONE_ACT_NO_LOG(IsWriteConsole(), goto WRITE_CONSOLE);

    if (g_dlogCallback != NULL) {
        if (!WritePLog(msg, strlen(msg))) {
            goto WRITE_CONSOLE;
        }
        (void)SlogUnlock();
        return;
    }
    if (!IsSocketConnected()) {
        SetSocketFd(CreatSocket());
        SetSocketConnectedStatus(TRUE);
    }
    if (!IsSocketFdValid()) {
        SetSocketConnectedStatus(FALSE);
        RemoveMessageTag(&newMsg);
        goto WRITE_CONSOLE;
    }

    WriteLogToSocket((const char*)msg, strlen(msg), moduleId, level);
    (void)SlogUnlock();
    return;

WRITE_CONSOLE:
    result = strncpy_s(copymsg, MSG_LENGTH, (const char *)newMsg, strlen(newMsg));
    TWO_ACT_WARN_LOG(result != EOK,  (void)SlogUnlock(), return, "strcpy failed");
    FullWriteFile(copymsg, fd, moduleId, level);
    (void)SlogUnlock();
    return;
}

/**
* @brief dlog_init: libslog.so initialize
* @return: void
*/
#if (OS_TYPE_DEF == 0)
void dlog_init(void)
{
    g_pid = ToolGetPid();
    ONE_ACT_INFO_LOG(IsLogInited(), return, "Log has bean inited.");

    if (IsSlogMutexInited() == NO_INITED) {
        // fix deadlock because of fork
        int result = pthread_atfork((ThreadAtFork)SlogLock, (ThreadAtFork)SlogUnlock, (ThreadAtFork)SlogUnlock);
        ONE_ACT_ERR_LOG(result != 0, return, "register fork fail, result=%d, strerr=%s.",
                        result, strerror(ToolGetErrorCode()));
        SetSlogMutexInited(INITED);
    }

    // sync time zone
    tzset();
#ifdef LOG_CPP
    LogInitWorkspacePath();
    LogLevelInit();
    g_logAttr.pid = (unsigned int)getpid();
    StartThreadForLogFlush();
#else
    int isLibSlog = 1;
    (void)InitCfg(isLibSlog);
    if (LogInitWorkspacePath() != SYS_OK) {
        SELF_LOG_ERROR("Workspace=%s not exist and return.\n", GetWorkspacePath());
        FreeConfList();
        return;
    }
    LogLevelInit();
    FreeConfList();
    g_logAttr.type = SYSTEM;
    StartThreadForLevelChangeWatcher();
#endif
    g_isInited = INITED;
}
#else
void dlog_init(void)
{
    g_pid = ToolGetPid();
    ONE_ACT_INFO_LOG(IsLogInited(), return, "Log has bean inited.");

#ifndef LOG_CPP
    // get log level conf in initial stage
    if (InitConfList(SLOG_CONF_FILE_PATH) != SUCCESS) {
        SELF_LOG_WARN("Init conf list failed");
    }
    InitFilePathForSelfLog();
    ReadLogLevelFromConfig();
    ReadModuleLogLevelFromConfig();
    FreeConfList();
#else
    InitFilePathForSelfLog();
    InitLogLevelByEnv();
    InitEventLevelByEnv();
    StartThreadForLogFlush();
#endif

    ToolUserBlock* userBlock = (ToolUserBlock *)malloc(sizeof(ToolUserBlock));
    ThreadArgs* args = (ThreadArgs *)malloc(sizeof(ThreadArgs));
    if (userBlock == NULL || args == NULL) {
        XFREE(userBlock);
        XFREE(args);
        SELF_LOG_WARN("malloc failed, strerr=%s.", strerror(ToolGetErrorCode()));
        return;
    }

    (void)memset_s(userBlock, sizeof(ToolUserBlock), 0, sizeof(ToolUserBlock));
    (void)memset_s(args, sizeof(ThreadArgs), 0, sizeof(ThreadArgs));
    args->mmThreadArgs = userBlock;
    userBlock->procFunc = ReadLogLevelProc;
    userBlock->pulArg = (void *)args;

    toolThread thread;
    if (SYS_OK != ToolCreateTaskWithDetach(&thread, userBlock)) {
        SELF_LOG_ERROR("create thread(ReadLogLevel) failed, strerr=%s, pid=%d.",
                       strerror(ToolGetErrorCode()), g_pid);
        XFREE(userBlock);
        XFREE(args);
        g_isInited = NO_INITED;
    }
    g_isInited = INITED;
#ifdef LOG_CPP
    g_logAttr.pid = (unsigned int)ToolGetPid();
#else
    g_logAttr.type = SYSTEM;
#endif
}
#endif

STATIC int CheckLogLevelAfterInited(int moduleId, int level)
{
    if (IsLogInited()) {
        int result = CheckLogLevel(moduleId, level);
        ONE_ACT_NO_LOG(result == FALSE, return FALSE);
    }
    return TRUE;
}

void DlogErrorInner(int moduleId, const char *fmt, ...)
{
    if (fmt != NULL) {
        int result = CheckLogLevelAfterInited(moduleId, DLOG_ERROR);
        ONE_ACT_NO_LOG(result == FALSE, return);
        va_list list;
        va_start(list, fmt);
        KeyValueArg kvArg = { NULL, 0 };
        WriteDlog(moduleId, DLOG_ERROR, &kvArg, fmt, list);
        va_end(list);
    }
}

void DlogWarnInner(int moduleId, const char *fmt, ...)
{
    if (fmt != NULL) {
        int result = CheckLogLevelAfterInited(moduleId, DLOG_WARN);
        ONE_ACT_NO_LOG(result == FALSE, return);
        va_list list;
        va_start(list, fmt);
        KeyValueArg kvArg = { NULL, 0 };
        WriteDlog(moduleId, DLOG_WARN, &kvArg, fmt, list);
        va_end(list);
    }
}

void DlogInfoInner(int moduleId, const char *fmt, ...)
{
    if (fmt != NULL) {
        int result = CheckLogLevelAfterInited(moduleId, DLOG_INFO);
        ONE_ACT_NO_LOG(result == FALSE, return);
        va_list list;
        va_start(list, fmt);
        KeyValueArg kvArg = { NULL, 0 };
        WriteDlog(moduleId, DLOG_INFO, &kvArg, fmt, list);
        va_end(list);
    }
}

void DlogDebugInner(int moduleId, const char *fmt, ...)
{
    if (fmt != NULL) {
        int result = CheckLogLevelAfterInited(moduleId, DLOG_DEBUG);
        ONE_ACT_NO_LOG(result == FALSE, return);
        va_list list;
        va_start(list, fmt);
        KeyValueArg kvArg = { NULL, 0 };
        WriteDlog(moduleId, DLOG_DEBUG, &kvArg, fmt, list);
        va_end(list);
    }
}

void DlogEventInner(int moduleId, const char *fmt, ...)
{
    if (fmt != NULL) {
        int result = CheckLogLevelAfterInited(moduleId, DLOG_EVENT);
        ONE_ACT_NO_LOG(result == FALSE, return);
        va_list list;
        va_start(list, fmt);
        KeyValueArg kvArg = { NULL, 0 };
        WriteDlog(moduleId, DLOG_EVENT, &kvArg, fmt, list);
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
        int result = CheckLogLevelAfterInited(moduleId, level);
        ONE_ACT_NO_LOG(result == FALSE, return);
        va_list list;
        va_start(list, fmt);
        KeyValueArg kvArg = { NULL, 0 };
        WriteDlog(moduleId, level, &kvArg, fmt, list);
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
    ONE_ACT_ERR_LOG(kvNum <= 0, return, "[input] key-value number is invalid, key_value_number=%d.", kvNum);
    if (fmt != NULL) {
        int result = CheckLogLevelAfterInited(moduleId, level);
        ONE_ACT_NO_LOG(result == FALSE, return);
        va_list list;
        va_start(list, fmt);
        KeyValueArg kvArg = { pstKVArray, kvNum };
        WriteDlog(moduleId, level, &kvArg, fmt, list);
        va_end(list);
    }
}

/**
* @brief DlogFlush: flush log buffer to file
* @return: void
*/
void DlogFlush(void)
{
    return;
}

#if (OS_TYPE_DEF == WIN)
BOOL APIENTRY DllMain(HANDLE hModule, DWORD ulReasonForCall, LPVOID lpReserved)
{
    switch (ulReasonForCall) {
        case DLL_PROCESS_ATTACH:
            if (IsSlogMutexInited() == NO_INITED) {
                if (ToolMutexInit(&g_slogMutex) != 0) {
                    SELF_LOG_INFO("mutex init failed, strerr=%s", strerror(ToolGetErrorCode()));
                } else {
                    SetSlogMutexInited(INITED);
                }
            }
            ProcessLogInit();
            dlog_init();
            SELF_LOG_INFO("process attach.");
            break;

        case DLL_PROCESS_DETACH:
            g_threadExit = 1;
            ProcessLogFree();
            SELF_LOG_INFO("process detach.");
            break;

        default:
            SELF_LOG_INFO("default type.");
            break;
    }

    return TRUE;
}
#endif

/**
 * @brief DlogSetAttr: set log attr
 * @param [in]logAttr: attr info, include pid, process type and device id
 * @return: 0: SUCCEED, others: FAILED
 */
int DlogSetAttr(LogAttr logAttr)
{
    if (logAttr.type == APPLICATION && logAttr.pid <= 0) {
        SELF_LOG_ERROR("set log attr failed, pid=%u is invalid.", logAttr.pid);
        return SYS_ERROR;
    }
    if (logAttr.type == APPLICATION && logAttr.deviceId >= MAX_DEV_NUM) {
        SELF_LOG_ERROR("set log attr failed, deviceId=%u is invalid.", logAttr.deviceId);
        return SYS_ERROR;
    }
    g_logAttr.deviceId = logAttr.deviceId;
    g_logAttr.type = logAttr.type;
    g_logAttr.pid = logAttr.pid;
    SELF_LOG_INFO("set log attr, pid=%u, type=%d, devId=%u.", logAttr.pid, logAttr.type, logAttr.deviceId);
    if (logAttr.type == APPLICATION && !g_levelSetted) {
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
    (void)SlogLock();
    switch (funcType) {
        case LOG_REPORT:
            g_dlogCallback = (DlogCallback)callback;
            if (g_dlogCallback != NULL) {
                g_hasRegistered = TRUE;
            }
            break;
        case LOG_FLUSH:
            if (callback == NULL) {
                g_dlogFlushRegistered = false;
            }
            ToolMemBarrier();
            g_dlogFlushCallback = (DlogFlushCallback)callback;
            break;
        default:
            break;
    }
    (void)SlogUnlock();
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
    if (moduleLevel < LOG_MIN_LEVEL || moduleLevel > LOG_MAX_LEVEL) {
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
        int result = CheckLogLevelAfterInited(moduleId, level);
        ONE_ACT_NO_LOG(result == FALSE, return);
        va_list list;
        va_start(list, fmt);
        KeyValueArg kvArg = { NULL, 0 };
        WriteDlog(moduleId, level, &kvArg, fmt, list);
        va_end(list);
    }
}

void DlogWithKVInnerForC(int moduleId, int level, KeyValue *pstKVArray, int kvNum, const char *fmt, ...)
{
    ONE_ACT_ERR_LOG(pstKVArray == NULL, return, "[input] key-value array is null.");
    ONE_ACT_ERR_LOG(kvNum <= 0, return, "[input] key-value number is invalid, key_value_number=%d.", kvNum);
    if (fmt != NULL) {
        int result = CheckLogLevelAfterInited(moduleId, level);
        ONE_ACT_NO_LOG(result == FALSE, return);
        va_list list;
        va_start(list, fmt);
        KeyValueArg kvArg = { pstKVArray, kvNum };
        WriteDlog(moduleId, level, &kvArg, fmt, list);
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
