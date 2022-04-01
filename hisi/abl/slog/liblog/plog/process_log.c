/**
 * @process_log.c
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "process_log.h"
#include "plog.h"
#include "ide_tlv.h"
#include "log_drv.h"
#include "log_runtime.h"

#include "log_to_file.h"
#include "log_common_util.h"
#include "log_sys_package.h"
#include "print_log.h"
#include "slog_common.h"

#ifdef HAL_REGISTER_ALOG
#include "api/driver_api.h"
#include "ascend_hal.h"
static HalCallback g_halCallback = NULL;
#endif

typedef struct {
    ToolUserBlock block;
    toolThread tid;
    int devId;
    uintptr_t session;
} ThreadInfo;

typedef struct {
    char *msg;
    unsigned int offset;
} PLogBuf;

#define DEVICE_OPEN 1
#define DEVICE_CLOSE 0
#define MAX_TIMEOUT 3000 // timeout 3s
#define MAX_DEVICE_LOG_FLUSH_TIMEOUT (3 * 60 * 1000) // max timeout 180s
#define DEFALUT_DEVICE_LOG_FLUSH_TIMEOUT 2000 // default timeout 2s
#define MAX_RETRY_TIME 3
#define MAX_LOGBUF_SIZE 1048576
#define DEFAULT_DETACH_THREAD_ATTR { 1, 0, 0, 0, 0, 1, 128 * 1024 } // Default ThreadSize(128KB), detached

static ThreadInfo g_plogThread[MAX_DEV_NUM];
static uintptr_t g_plogClient = 0;
static StLogFileList g_plogFileList = { 0 };
STATIC bool g_isInited = false;
STATIC bool g_isExit = false;
static int g_devState[MAX_DEV_NUM] = { 0 };
static int g_flushTimeout = 0;
static bool g_isWorkerMachine = false;
PLogBuf g_pLogBuf = { NULL, 0 };

// receive device log
STATIC void DevPlogRecvStart(int devId);
STATIC void DevPlogRecvStop(int devId);
// register function callback
STATIC void DevStateCallback(unsigned int devId, bool isOpenDev);
STATIC int LogReportCallback(const char *buf, unsigned int bufLen);
STATIC int LogFlushCallback(void);

#if (OS_TYPE_DEF == 0)
__attribute__((constructor)) int ProcessLogInit(void);
__attribute__((destructor)) int ProcessLogFree(void);
#endif

#ifdef HAL_REGISTER_ALOG
/**
 * @brief RegisterHalCallback: register hal callback
 * @param [in]callback: function pointer
 */
void RegisterHalCallback(void (*callback) (int moduleId, int level, const char *fmt, ...))
{
    g_halCallback = callback;
}

STATIC unsigned int GetLogLevelByEnv(void)
{
    char *env = getenv("ASCEND_GLOBAL_LOG_LEVEL");
    if ((env != NULL) && (IsNaturalNumStr(env))) {
        unsigned int tmpL = atoi(env);
        if (tmpL <= LOG_MAX_LEVEL) {
            return tmpL;
        }
    }
    SELF_LOG_INFO("get global love level from env failed, maybe is's null. Use default: %d",
                  GLOABLE_DEFAULT_LOG_LEVEL);
    return GLOABLE_DEFAULT_LOG_LEVEL;
}
#endif

/**
 * @brief ProcessLogInit: init log file list, register callback, create hdc client
 * @return 0: succeed; -1: failed
 *    if return failed, the function will release init resource before return
 */
int ProcessLogInit(void)
{
    int ret;
    unsigned int platform = INVALID_VALUE;
    if (DrvFunctionsInit() != 0) {
        SELF_LOG_WARN("load driver library failed, treat as worker machine");
        g_isWorkerMachine = true;
    } else {
        ONE_ACT_ERR_LOG(DrvGetPlatformInfo(&platform) != 0, return -1, "get platform info failed.");
        if (platform != INVALID_VALUE) {
            ONE_ACT_NO_LOG(platform != HOST_SIDE, return 0); // only support host or docker
        }

        // register DlogInner function to Hal
#ifdef HAL_REGISTER_ALOG
        struct log_out_handle handle;
        handle.DlogInner = g_halCallback;
        handle.logLevel = GetLogLevelByEnv();
        if (handle.DlogInner == NULL) {
            SELF_LOG_ERROR("handle.DlogInner is null, RegisterHalCallback execute failed");
        }
        ret = LogdrvCtl(HAL_CTL_REGISTER_LOG_OUT_HANDLE, &handle, sizeof(handle), NULL, NULL);
        NO_ACT_WARN_LOG(ret != 0, "register DlogInner function to Hal failed");
#endif
    }

    // init process log file list
    ret = LogAgentInitProc(&g_plogFileList);
    ONE_ACT_ERR_LOG(ret != 0, goto INIT_ERROR, "init plog file list failed, ret=%d.", ret);
    if (g_pLogBuf.msg == NULL) {
        g_pLogBuf.msg = (char *)malloc(MSG_LENGTH * PLOG_MSG_NODE_NUM);
        if (g_pLogBuf.msg == NULL) {
            return -1;
        }
    }
    (void)memset_s(g_pLogBuf.msg, MSG_LENGTH * PLOG_MSG_NODE_NUM, 0x00, MSG_LENGTH * PLOG_MSG_NODE_NUM);

    // register callback to libalog.so
    ret = RegisterCallback(LogReportCallback, LOG_REPORT);
    ONE_ACT_ERR_LOG(ret != 0, goto INIT_ERROR, "register process log callback failed.");
    ret = RegisterCallback(LogFlushCallback, LOG_FLUSH);
    ONE_ACT_ERR_LOG(ret != 0, goto INIT_ERROR, "register process log flush callback failed.");

    // init device log file list
    ret = LogAgentInitDevice(&g_plogFileList, MAX_DEV_NUM);
    ONE_ACT_ERR_LOG(ret != 0, goto INIT_ERROR, "init dev file list failed, ret=%d.", ret);

    (void)memset_s(g_plogThread, MAX_DEV_NUM * sizeof(ThreadInfo), 0, MAX_DEV_NUM * sizeof(ThreadInfo));
    SELF_LOG_INFO("Log init finished for process.");
    return 0;

INIT_ERROR:
    SELF_LOG_ERROR("Log init failed for process.");
    XFREE(g_pLogBuf.msg);
    return -1;
}

STATIC void InitLogFlushTimeoutByEnv(void)
{
    char *env = getenv("ASCEND_LOG_DEVICE_FLUSH_TIMEOUT");
    if ((env != NULL) && (IsNaturalNumStr(env))) {
        int tmpL = atoi(env); // Unit: millisecond
        if (tmpL >= 0 && tmpL <= MAX_DEVICE_LOG_FLUSH_TIMEOUT) {
            g_flushTimeout = tmpL;
            SELF_LOG_INFO("get right env ASCEND_LOG_DEVICE_FLUSH_TIMEOUT(%d).", tmpL);
            return;
        } else {
            SELF_LOG_WARN("env ASCEND_LOG_DEVICE_FLUSH_TIMEOUT(%d) invalid.", tmpL);
        }
    }
    g_flushTimeout = DEFALUT_DEVICE_LOG_FLUSH_TIMEOUT;
    SELF_LOG_INFO("set default device log flush timeout(%d).", g_flushTimeout);
}

int DlogReportInitialize(void)
{
    int ret;
    unsigned int platform = 0;

    ONE_ACT_NO_LOG(g_isInited == true, return 0);
    g_isInited = true;

    ONE_ACT_ERR_LOG(DrvGetPlatformInfo(&platform) != 0, return -1, "get platform info failed.");
    ONE_ACT_NO_LOG(platform != HOST_SIDE, return 0); // only support host or docker

    // create hdc client
    ret = DrvClientCreate((HDC_CLIENT)&g_plogClient, HDC_SERVICE_TYPE_LOG);
    ONE_ACT_ERR_LOG(ret != 0, goto INIT_ERROR, "create hdc client failed.");

    ret = RuntimeFunctionsInit();
    ONE_ACT_NO_LOG(ret != 0, goto INIT_ERROR);
    // register callback to libruntime.so
    ret = LogRegDeviceStateCallback(DevStateCallback);
    ONE_ACT_ERR_LOG(ret != 0, goto INIT_ERROR, "register device state callback failed, ret=%d.", ret);

    g_isExit = false;
    InitLogFlushTimeoutByEnv();
    SELF_LOG_INFO("Dlog initialize finished for process.");
    return 0;

INIT_ERROR:
    (void)DrvClientRelease((HDC_CLIENT)g_plogClient);
    SELF_LOG_ERROR("Dlog initialize failed for process.");
    return -1;
}

static void ThreadRelease(ThreadInfo *threadInfo, int threadNum)
{
    int i = 0;

    for (i = 0; i < threadNum; i++) {
        if (threadInfo[i].tid != 0) {
            DevPlogRecvStop(i);
        }
    }

    for (i = 0; i < threadNum; i++) {
        if (threadInfo[i].tid == 0) {
            continue;
        }
        int ret = ToolJoinTask(&threadInfo[i].tid);
        NO_ACT_WARN_LOG(ret != 0, "pthread join failed, dev_id=%d, strerr=%s.",
                        i, strerror(ToolGetErrorCode()));
        threadInfo[i].tid = 0;
    }
}

/**
 * @brief ProcessLogFree: free log file list, close hdc client
 * @return 0: succeed; -1: failed
 */
int ProcessLogFree(void)
{
    g_isExit = true;
    unsigned int platform = 0; // 0: device side, 1: host side

    if (!g_isWorkerMachine) {
        // unregister DlogInner function to Hal
#ifdef HAL_REGISTER_ALOG
        int ret = LogdrvCtl(HAL_CTL_UNREGISTER_LOG_OUT_HANDLE, NULL, 0, NULL, NULL);
        NO_ACT_WARN_LOG(ret != DRV_ERROR_NONE, "unregister DlogInner function to Hal failed");
#endif

        TWO_ACT_ERR_LOG(DrvGetPlatformInfo(&platform) != 0,
                        DrvFunctionsUninit(), return -1, "get platform info failed.");
        TWO_ACT_NO_LOG(platform != HOST_SIDE, DrvFunctionsUninit(), return 0); // only support host or docker
    }

    // release all recv thread repeatedly
    ThreadRelease(g_plogThread, MAX_DEV_NUM);

    // flush all log and set callback null
    (void)RegisterCallback(NULL, LOG_REPORT);
    (void)RegisterCallback(NULL, LOG_FLUSH);
    if (g_pLogBuf.offset > 0) {
        unsigned int ret = LogAgentWriteProcLog(&g_plogFileList, g_pLogBuf.msg, g_pLogBuf.offset);
        SELF_LOG_INFO("LogAgentWriteProcLog ret=%u, offset=%u.", ret, g_pLogBuf.offset);
    }
    XFREE(g_pLogBuf.msg);
    // free log file list
    LogAgentCleanUpProc(&g_plogFileList);
    LogAgentCleanUpDevice(&g_plogFileList);
    if (!g_isWorkerMachine) {
        (void)DrvFunctionsUninit();
    }
    SELF_LOG_INFO("Log uninit finished.");
    return 0;
}

int DlogReportFinalize(void)
{
    int ret;
    unsigned int platform = 0; // 0: device side, 1: host side

    ONE_ACT_ERR_LOG(DrvGetPlatformInfo(&platform) != 0, return -1, "get platform info failed.");
    ONE_ACT_NO_LOG(platform != HOST_SIDE, return 0); // only support host or docker

    ONE_ACT_NO_LOG(g_isExit, return 0);
    g_isExit = true;

    // release all recv thread
    ThreadRelease(g_plogThread, MAX_DEV_NUM);
    int retryTimes = 0;
    uintptr_t flag = (uintptr_t)0;
    // use the product of DevPlogRecvThread's max timeout and retry times as time limit
    int maxTimeout = g_flushTimeout > MAX_TIMEOUT ? g_flushTimeout : MAX_TIMEOUT;
    int maxTimes = (maxTimeout + TEN_MILLISECOND) * MAX_RETRY_TIME; // cycle intercal of DevPlogRecvThread retry is 10ms
    SELF_LOG_INFO("maxTimes is %d", maxTimes);
    int i = 0;
    for (; i < MAX_DEV_NUM; i++) {
        retryTimes = 0;
        while (g_plogThread[i].session != flag && retryTimes <= maxTimes) {
            ToolSleep(1);
            retryTimes += 1;
        }
        NO_ACT_ERR_LOG(retryTimes > maxTimes, "session g_plogThread[%d] not exit exceed %d s", maxTimes / ONE_SECOND)
    }
    // release hdc client
    ret = DrvClientRelease((HDC_CLIENT)g_plogClient);
    NO_ACT_ERR_LOG(ret != 0, "free hdc client failed.");

    g_isInited = false;
    (void)RuntimeFunctionsUninit();
    SELF_LOG_INFO("Dlog finalize finished.");
    return 0;
}

/**
* @brief DevStateCallback: handle device state changed
* @param [in]devId: device id
* @param [in]isOpenDev: device run state
* @return: void
*/
STATIC void DevStateCallback(unsigned int devId, bool isOpenDev)
{
    ONE_ACT_ERR_LOG(devId >= MAX_DEV_NUM, return, "device id is invalid, devId=%u.", devId);

    SELF_LOG_INFO("dev state changed, devId=%d, state=%d.", devId, isOpenDev);
    if (isOpenDev == true) {
        DevPlogRecvStart((int)devId);
    } else {
        DevPlogRecvStop((int)devId);
    }
    SELF_LOG_INFO("dev state change handle end, devId=%d, state=%d.", devId, isOpenDev);
}

/**
* @brief LogReportCallback: write process log from libalog.so
* @param [in]buf: log msg
* @param [in]bufLen: log msg length
* @return: 0: success; -1: failed
*/
STATIC int LogReportCallback(const char *buf, unsigned int bufLen)
{
    ONE_ACT_NO_LOG(buf == NULL || bufLen == 0, return -1);
    ONE_ACT_NO_LOG(g_pLogBuf.msg == NULL, return -1);
    if (g_pLogBuf.offset >= (MSG_LENGTH * (PLOG_MSG_NODE_NUM - 1))) {
        unsigned int ret = LogAgentWriteProcLog(&g_plogFileList, g_pLogBuf.msg, g_pLogBuf.offset);
        if (ret == 0) {
            g_pLogBuf.offset = 0;
            (void)memset_s(g_pLogBuf.msg, MSG_LENGTH * PLOG_MSG_NODE_NUM, 0x00, MSG_LENGTH * PLOG_MSG_NODE_NUM);
        } else {
            SELF_LOG_WARN("write host log wrong, ret=%u, offset=%u.", ret, g_pLogBuf.offset);
            return -1;
        }
    }
    int bufResLength = MSG_LENGTH * PLOG_MSG_NODE_NUM - g_pLogBuf.offset - 1;
    int res = memcpy_s(g_pLogBuf.msg + g_pLogBuf.offset, bufResLength, buf, bufLen);
    if (res != EOK) {
        return -1;
    }
    g_pLogBuf.offset += bufLen;
    return 0;
}

/**
* @brief LogFlushCallback: flush host log to disk
*/
STATIC int LogFlushCallback(void)
{
    ONE_ACT_NO_LOG(g_pLogBuf.msg == NULL, return -1);
    ONE_ACT_NO_LOG(g_pLogBuf.offset == 0, return 0);
    unsigned int ret = LogAgentWriteProcLog(&g_plogFileList, g_pLogBuf.msg, g_pLogBuf.offset);
    if (ret == 0) {
        g_pLogBuf.offset = 0;
        (void)memset_s(g_pLogBuf.msg, MSG_LENGTH * PLOG_MSG_NODE_NUM, 0x00, MSG_LENGTH * PLOG_MSG_NODE_NUM);
    } else {
        SELF_LOG_WARN("flush host log wrong, ret=%u, offset=%u.", ret, g_pLogBuf.offset);
        return -1;
    }
    return 0;
}

static int SendRequestMsg(uintptr_t session, const char *data, unsigned int dataLen)
{
    int ret;
    LogNotifyMsg *msg = NULL;
    LogDataMsg *packet = NULL;
    int msgLen = sizeof(LogNotifyMsg) + dataLen;

    msg = (LogNotifyMsg *)calloc(1, msgLen + 1);
    ONE_ACT_WARN_LOG(msg == NULL, return -1, "calloc failed, strerr=%s.", strerror(ToolGetErrorCode()));

    msg->timeout = g_flushTimeout;
    ret = memcpy_s(msg->data, dataLen + 1, data, dataLen);
    ONE_ACT_WARN_LOG(ret != EOK, goto FUNC_END, "copy data failed, strerr=%s.", strerror(ToolGetErrorCode()));

    ret = -1;
    packet = (LogDataMsg *)calloc(1, sizeof(LogDataMsg) + msgLen + 1);
    ONE_ACT_WARN_LOG(packet == NULL, goto FUNC_END, "calloc failed, strerr=%s.", strerror(ToolGetErrorCode()));

    packet->totalLen = msgLen;
    packet->sliceLen = msgLen;
    packet->offset = 0;
    packet->msgType = LOG_MSG_DATA;
    packet->reqType = IDE_LOG_REQ;
    ret = memcpy_s(packet->data, msgLen + 1, msg, msgLen);
    ONE_ACT_WARN_LOG(ret != 0, goto FUNC_END, "copy failed, ret=%d, strerr=%s.", ret, strerror(ToolGetErrorCode()));

    ret = DrvBufWrite((HDC_SESSION)session, (const char *)packet, sizeof(LogDataMsg) + msgLen);
    ONE_ACT_WARN_LOG(ret != 0, goto FUNC_END, "write data to hdc failed, ret=%d.", ret);

FUNC_END:
    XFREE(msg);
    XFREE(packet);
    return ret;
}

STATIC void* DevPlogRecvThread(void *args)
{
    int ret;
    int retryTime = 0;
    uintptr_t session = 0;
    unsigned int recvTimeout = 0;

    int *devId = (int *)args;
    ONE_ACT_WARN_LOG(*devId < 0 || *devId >= MAX_DEV_NUM, goto THREAD_EXIT, "invalid devId %d.", *devId);
    DeviceWriteLogInfo info = { 0, *devId, 0, 0 };

    session = g_plogThread[*devId].session;

    char *buf = NULL;
    unsigned int bufLen = 0;
    while (retryTime < MAX_RETRY_TIME) {
        // compute timeout for every retry time, max retry time is <MAX_RETRY_TIME>
        recvTimeout = g_isExit ? (g_flushTimeout / MAX_RETRY_TIME) : MAX_TIMEOUT;
        ret = DrvBufRead((HDC_SESSION)session, *devId, (char **)&buf, &bufLen, recvTimeout);
        if (ret != 0) {
            SELF_LOG_WARN("recv log buf by hdc failed, ret=%d.", ret);
            ONE_ACT_NO_LOG(g_isExit, retryTime++);
            ToolSleep(TEN_MILLISECOND); // sleep 10ms
            continue;
        }
        if (buf == NULL || bufLen == 0) {
            SELF_LOG_WARN("recv log buf empty, buf=%s, len=%u.", buf, bufLen);
        } else if (strncmp(buf, HDC_END_BUF, sizeof(HDC_END_BUF)) == 0) {
            SELF_LOG_WARN("recv end buf by hdc.");
            retryTime = MAX_RETRY_TIME; // then exit loop
        } else {
            info.len = bufLen;
            ret = LogAgentWriteDeviceLog(&g_plogFileList, buf, &info);
            NO_ACT_WARN_LOG(ret != 0, "write device log failed, ret=%d.", ret);
            retryTime = 0;
        }
        XFREE(buf);
        bufLen = 0;
    }

THREAD_EXIT:
    (void)DrvSessionRelease((HDC_SESSION)session);
    g_plogThread[*devId].tid = 0;
    g_plogThread[*devId].session = (uintptr_t)0;
    SELF_LOG_INFO("Log recv thread exited, devId=%d.", *devId);
    return NULL;
}

STATIC void DevPlogRecvStart(int devId)
{
    ONE_ACT_WARN_LOG(g_plogThread[devId].tid != 0, return, "Log recv thread has bean started, devId=%d.", devId);

    int ret;
    uintptr_t session;
    ToolThreadAttr threadAttr = DEFAULT_DETACH_THREAD_ATTR;

    ret = DrvSessionInit((HDC_CLIENT)g_plogClient, (HDC_SESSION *)&session, devId);
    ONE_ACT_WARN_LOG(ret != 0, return, "create session failed, ret=%d, devId=%d.", ret, devId);

    ret = SendRequestMsg(session, HDC_START_BUF, sizeof(HDC_START_BUF));
    if (ret != 0) {
        SELF_LOG_WARN("send request info failed, devId=%d.", devId);
        (void)DrvSessionRelease((HDC_SESSION)session);
        return;
    }

    g_plogThread[devId].devId = devId;
    g_plogThread[devId].session = session;
    g_plogThread[devId].block.procFunc = DevPlogRecvThread;
    g_plogThread[devId].block.pulArg = (void *)&g_plogThread[devId].devId;
    ret = ToolCreateTaskWithThreadAttr(&g_plogThread[devId].tid, &g_plogThread[devId].block, &threadAttr);
    if (ret != SYS_OK) {
        SELF_LOG_WARN("create task(DevPlogRecvThread) failed, devId=%d, strerr=%s.",
                      devId, strerror(ToolGetErrorCode()));
        (void)DrvSessionRelease((HDC_SESSION)session);
        return;
    }
    g_devState[devId] = DEVICE_OPEN; // device is open
}

STATIC void DevPlogRecvStop(int devId)
{
    int ret;
    uintptr_t session = 0;

    ONE_ACT_NO_LOG(g_devState[devId] == DEVICE_CLOSE, return);
    g_devState[devId] = DEVICE_CLOSE;

    // create session
    ret = DrvSessionInit((HDC_CLIENT)g_plogClient, (HDC_SESSION *)&session, devId);
    ONE_ACT_WARN_LOG(ret != 0, return, "create session failed, ret=%d, devId=%d.", ret, devId);

    ret = SendRequestMsg(session, HDC_END_BUF, sizeof(HDC_END_BUF));
    ONE_ACT_WARN_LOG(ret != 0, return, "send request info failed, devId=%d.", devId);

    ret = DrvSessionRelease((HDC_SESSION)session);
    ONE_ACT_WARN_LOG(ret != 0, return, "release session failed, devId=%d.", devId);
}

