/**
 * @log_recv.c
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#include "log_recv.h"
#if (OS_TYPE_DEF == LINUX)
#include <sys/time.h>
#include <unistd.h>
#endif
#include "cfg_file_parse.h"
#include "log_daemon.h"
#include "log_common_util.h"

#define MAX_READY_RETYR_TIMES 10
STATIC unsigned char g_initError = 0;
STATIC unsigned char g_allThreadExit = 0;
STATIC unsigned int g_writeThreadState[MAX_DEV_NUM] = {0};
STATIC unsigned int g_recvThreadState[MAX_DEV_NUM] = {0};
STATIC unsigned int g_idThreadFlags[MAX_DEV_NUM] = {0};
STATIC toolMutex g_deviceThreadLock[MAX_DEV_NUM];
STATIC DeviceRes g_deviceResourcs[MAX_DEV_NUM];
STATIC unsigned int g_writeDPrintNum = 0;

unsigned char IsInitError(void)
{
    return g_initError;
}

unsigned char IsAllThreadExit(void)
{
    return g_allThreadExit;
}

unsigned int IsDeviceThreadAlive(const int deviceId)
{
    if ((deviceId >= MAX_DEV_NUM) || (deviceId < 0)) {
        return 0;
    }

    return g_idThreadFlags[deviceId];
}

/**
 * @brief EnQueSavedBuff: push buffer to queue
 * @param [in]queue: log queue instance
 * @param [in]savedBuff: log data which need to be enqueued. will be free in SetupDeviceWriteThread
 * @param [in]nodeCnt: log data node count
 * @return: LogRt, SUCCESS/ARGV_NULL/MALLOC_FAILED/others
 */
STATIC LogRt EnQueSavedBuff(LogQueue *queue, LogBuff *savedBuff, unsigned int *nodeCnt)
{
    ONE_ACT_WARN_LOG(queue == NULL, return ARGV_NULL, "[input] queue is null.");
    ONE_ACT_WARN_LOG(savedBuff == NULL, return ARGV_NULL, "[input] saved buffer is null.");
    ONE_ACT_WARN_LOG(nodeCnt == NULL, return ARGV_NULL, "[input] node number is null.");

    LogNode *logNode = (LogNode *)malloc(sizeof(LogNode));
    if (logNode == NULL) {
        SELF_LOG_WARN("malloc failed, strerr=%s.", strerror(ToolGetErrorCode()));
        return MALLOC_FAILED;
    }
    (void)memset_s(logNode, sizeof(LogNode), 0, sizeof(LogNode));

    logNode->uiNodeDataLen = savedBuff->buffLen;
    logNode->uiNodeNum = *nodeCnt;
    logNode->stNodeData = savedBuff->buff;
    logNode->next = NULL;
    logNode->slogFlag = savedBuff->slogFlag;
    logNode->smpFlag = savedBuff->smpFlag;

    LogRt ret = LogQueueEnqueue(queue, logNode);
    if (ret != SUCCESS) {
        XFREE(logNode);
        SELF_LOG_WARN("enqueue failed, result=%d.", ret);
        return ret;
    }

    savedBuff->buffLen = 0;
    savedBuff->slogFlag = 0;
    savedBuff->smpFlag = 0;
    *nodeCnt = 0;
    return SUCCESS;
}

/**
* @brief BuffEnqueueAndAllocNew: push buffer to queue and malloc new buffer
* @param [in]queue: log queue instance
* @param [in]savedBuff: buffer will be enqueued and re-malloced
* @param [in]nodeCnt: log data node count
* @return: LogRt, SUCCESS/ARGV_NULL/MALLOC_FAILED/others
*/
STATIC LogRt BuffEnqueueAndAllocNew(LogQueue *queue, LogBuff *savedBuff, unsigned int *nodeCnt)
{
    ONE_ACT_WARN_LOG(queue == NULL, return ARGV_NULL, "[input] queue is null.");
    ONE_ACT_WARN_LOG(savedBuff == NULL, return ARGV_NULL, "[input] saved buffer is null.");
    ONE_ACT_WARN_LOG(nodeCnt == NULL, return ARGV_NULL, "[input] node number is null.");

    if (savedBuff->buffLen > 0) {
        LogRt ret = EnQueSavedBuff(queue, savedBuff, nodeCnt);
        ONE_ACT_NO_LOG(ret != SUCCESS, return ret);
    } else {
        return SUCCESS;
    }

    savedBuff->buff = (char *)malloc(MAX_NODE_BUFF);
    if (savedBuff->buff == NULL) {
        SELF_LOG_WARN("malloc failed, strerr=%s.", strerror(ToolGetErrorCode()));
        return MALLOC_FAILED;
    }

    (void)memset_s(savedBuff->buff, MAX_NODE_BUFF, 0, MAX_NODE_BUFF);
    return SUCCESS;
}

/**
* @brief: Copy log data to new buffer and enqueue it
*         Enqueued buffer will be freed in SetupDeviceWriteThread
* @param [in]queue: log queue instance
* @param [in]recvMsg: buffer for log data. this will be cleaned up after quit this func
* @return: LogRt, SUCCESS/ARGV_NULL/MALLOC_FAILED/STR_COPY_FAILED/others
*/
STATIC LogRt AddRecMsgToQueue(LogQueue *queue, LogBuff *recvMsg)
{
    ONE_ACT_WARN_LOG(queue == NULL, return ARGV_NULL, "[input] queue is null.");
    ONE_ACT_WARN_LOG(recvMsg == NULL, return ARGV_NULL, "[input] received message is null.");
    ONE_ACT_NO_LOG(recvMsg->buffLen <= 0, return SUCCESS); // ignore empty message buff

    LogBuff *buffTmp = (LogBuff *)malloc(sizeof(LogBuff));
    if (buffTmp == NULL) {
        SELF_LOG_WARN("malloc failed, strerr=%s.", strerror(ToolGetErrorCode()));
        return MALLOC_FAILED;
    }

    buffTmp->buffLen = recvMsg->buffLen;
    buffTmp->slogFlag = recvMsg->slogFlag;
    buffTmp->smpFlag = recvMsg->smpFlag;
    buffTmp->buff = (char *)malloc(buffTmp->buffLen);
    if (buffTmp->buff == NULL) {
        SELF_LOG_WARN("malloc failed, strerr=%s.", strerror(ToolGetErrorCode()));
        XFREE(buffTmp);
        return MALLOC_FAILED;
    }
    (void)memset_s(buffTmp->buff, buffTmp->buffLen, 0, buffTmp->buffLen);
    int retValue = memcpy_s(buffTmp->buff, recvMsg->buffLen, recvMsg->buff, recvMsg->buffLen);
    if (retValue != EOK) {
        SELF_LOG_WARN("memcpy_s failed, result=%d, strerr=%s.", retValue, strerror(ToolGetErrorCode()));
        XFREE(buffTmp->buff);
        XFREE(buffTmp);
        return STR_COPY_FAILED;
    }
    recvMsg->buffLen = 0;
    (void)memset_s(recvMsg->buff, RECV_FULLY_DATA_LEN + 1, 0, RECV_FULLY_DATA_LEN + 1);

    unsigned int nodeCnt = 1;
    LogRt ret = EnQueSavedBuff(queue, buffTmp, &nodeCnt);
    if (ret != SUCCESS) {
        XFREE(buffTmp->buff);
        XFREE(buffTmp);
        return ret;
    }
    // buffTmp->buff was enqueued, buffTmp->buff should not be freed
    XFREE(buffTmp);
    return SUCCESS;
}

/**
* @brief FullAddBuffQueue: push buffer to queue if buffer is full. if not full copy new data to savedBuff
* @param [in]queue: log queue instance
* @param [in]recvMsg: buffer for log data. this will be cleaned up after enqueued
* @param [in]nodeCnt: log data node count
* @param [in]savedBuff: buffer to store log data which will be enqueued
* @return: LogRt, SUCCESS/ARGV_NULL/STR_COPY_FAILED/others
*/
STATIC LogRt FullAddBuffQueue(LogQueue *queue, LogBuff *recvMsg, unsigned int *nodeCnt, LogBuff *savedBuff)
{
    LogRt ret;

    ONE_ACT_WARN_LOG(queue == NULL, return ARGV_NULL, "[input] queue is null.");
    ONE_ACT_WARN_LOG(recvMsg == NULL, return ARGV_NULL, "[input] received message is null.");
    ONE_ACT_WARN_LOG(savedBuff == NULL, return ARGV_NULL, "[input] saved buffer is null.");
    ONE_ACT_WARN_LOG(nodeCnt == NULL, return ARGV_NULL, "[input] node number is null.");

    if (recvMsg->buffLen > (unsigned int)MAX_NODE_BUFF) {
        if (savedBuff->buffLen > 0) {
            // add previous buff and latest recv to queue;
            ret = BuffEnqueueAndAllocNew(queue, savedBuff, nodeCnt);
            ONE_ACT_WARN_LOG(ret != SUCCESS, return ret, "enqueue and allocate new buffer failed, result=%d.", ret);
        }

        ret = AddRecMsgToQueue(queue, recvMsg);
        ONE_ACT_WARN_LOG(ret != SUCCESS, return ret, "add message to queue failed, result=%d.", ret);

        return SUCCESS;
    }
    // if total lengh exceed limit or current log type not equal to stored log type, enqueue stored log
    if (((savedBuff->buffLen + recvMsg->buffLen) > (unsigned int)MAX_NODE_BUFF) ||
        (savedBuff->slogFlag != recvMsg->slogFlag)) {
        ret = BuffEnqueueAndAllocNew(queue, savedBuff, nodeCnt);
        ONE_ACT_WARN_LOG(ret != SUCCESS, return ret, "enqueue and allocate new buffer failed, result=%d.", ret);
    }

    int retValue = memcpy_s(savedBuff->buff + savedBuff->buffLen, recvMsg->buffLen, recvMsg->buff, recvMsg->buffLen);
    ONE_ACT_WARN_LOG(retValue != EOK, return STR_COPY_FAILED,
                     "memcpy_s failed, result=%d, strerr=%s.", retValue, strerror(ToolGetErrorCode()));

    savedBuff->buffLen = savedBuff->buffLen + recvMsg->buffLen;
    savedBuff->slogFlag = recvMsg->slogFlag;
    savedBuff->smpFlag = recvMsg->smpFlag;
    *nodeCnt = (*nodeCnt) + 1;
    recvMsg->buffLen = 0;
    (void)memset_s(recvMsg->buff, RECV_FULLY_DATA_LEN + 1, 0, RECV_FULLY_DATA_LEN + 1);
    return SUCCESS;
}

STATIC LogRt FullAddBuffQueueS(LogQueue *queue, LogBuff *recvMsg,
                               unsigned int *nodeCnt, LogBuff *buff, toolMutex *threadLock)
{
    LogRt ret = SUCCESS;
    do {
        // save buffer to queue when buffer is full safely
        LOCK_WARN_LOG(threadLock);
        ret = FullAddBuffQueue(queue, recvMsg, nodeCnt, buff);
        if (ret == QUEUE_IS_FULL) {
            UNLOCK_WARN_LOG(threadLock);
            SELF_LOG_INFO("queue is full while enqueue.");
            (void)ToolSleep(ONE_HUNDRED_MILLISECOND);
        }
    } while (ret == QUEUE_IS_FULL);

    UNLOCK_WARN_LOG(threadLock);
    return ret;
}

/**
 * @brief AddBuffQueue: save log msg <buffTmp> to buffer <buffAllData>
 *                      if buffer <buffAllData> is full, save it to queue <queue>
 *                      or if log msg <buffTmp> is end, save buffer <buffAllData> to queue <queue> too
 * @param [in]queue: log queue instance
 * @param [in]recvBuf: buffer read by hdc, drv or other, contains log state
 * @param [in]buffAllData: buffer that contains previous logs
 * @param [in]bufferTemp: log data copied by hdc ,drv or other read buffer
 * @param [in]deviceInfo: struct that contains device resources
 * @param [in]arg: pointer to DeviceRes
 * @return: LogRt, SUCCESS/ARGV_NULL/STR_COPY_FAILED/others
 */
STATIC LogRt AddBuffQueue(LogQueue *queue, const LogMsgHead *recvBuf, LogBuff *buffAllData,
                          const LogBuff *bufferTemp, DeviceRes *deviceInfo)
{
    LogRt ret;

    ONE_ACT_WARN_LOG(queue == NULL, return ARGV_NULL, "[input] queue is null.");
    ONE_ACT_WARN_LOG(recvBuf == NULL, return ARGV_NULL, "[input] received buffer is null.");
    ONE_ACT_WARN_LOG(buffAllData == NULL, return ARGV_NULL, "[input] all data buffer is null.");
    ONE_ACT_WARN_LOG(bufferTemp == NULL, return ARGV_NULL, "[input] temp buffer is null.");
    ONE_ACT_WARN_LOG(deviceInfo == NULL, return ARGV_NULL, "[input] device info is null.");

    if (((long long)(buffAllData->buffLen + bufferTemp->buffLen) > RECV_FULLY_DATA_LEN) ||
        ((buffAllData->buffLen > 0) && (buffAllData->slogFlag != bufferTemp->slogFlag))) {
        // enqueue first because of buff <buffAllData> full
        ret = FullAddBuffQueueS(queue, buffAllData, &(deviceInfo->nodeCnt), deviceInfo->logBuff, deviceInfo->lock);
        ONE_ACT_WARN_LOG(ret != SUCCESS, return ret, "enqueue failed, ret=%d.", ret);
    }

    int retValue = memcpy_s(buffAllData->buff + buffAllData->buffLen, RECV_FULLY_DATA_LEN,
                            bufferTemp->buff, bufferTemp->buffLen);
    ONE_ACT_WARN_LOG(retValue != EOK, return STR_COPY_FAILED,
                     "memcpy_s failed, result=%d, strerr=%s.", retValue, strerror(ToolGetErrorCode()));

    buffAllData->buffLen += bufferTemp->buffLen;
    buffAllData->slogFlag = bufferTemp->slogFlag;
    buffAllData->smpFlag = bufferTemp->smpFlag;

    unsigned int frameEnd = (unsigned)(int)(recvBuf->frame_end) & 0x1;
    if (frameEnd != PACKAGE_END) {
        return SUCCESS;
    }

    // enqueue because of log msg <buffTmp> end
    ret = FullAddBuffQueueS(queue, buffAllData, &(deviceInfo->nodeCnt), deviceInfo->logBuff, deviceInfo->lock);
    ONE_ACT_WARN_LOG(ret != SUCCESS, return ret, "enqueue failed, result=%d.", ret);

    return SUCCESS;
}

STATIC LogRt PrepareRecvRes(DeviceRes *deviceInfo, LogBuff *buffAll, LogBuff *bufferTemp)
{
    ONE_ACT_WARN_LOG(deviceInfo == NULL, return ARGV_NULL, "[input] device info is null.");
    ONE_ACT_WARN_LOG(buffAll == NULL, return ARGV_NULL, "[input] all buffer is null.");
    ONE_ACT_WARN_LOG(bufferTemp == NULL, return ARGV_NULL, "[input] temp buffer is null.");

    LOCK_WARN_LOG(deviceInfo->lock);
    deviceInfo->logBuff->buff = (char *)malloc(MAX_NODE_BUFF);
    if (deviceInfo->logBuff->buff == NULL) {
        SELF_LOG_ERROR("malloc failed, strerr=%s. Thread(setupDeviceRecv) quit.", strerror(ToolGetErrorCode()));
        UNLOCK_WARN_LOG(deviceInfo->lock);
        return MALLOC_FAILED;
    }
    (void)memset_s(deviceInfo->logBuff->buff, MAX_NODE_BUFF, 0, MAX_NODE_BUFF);
    UNLOCK_WARN_LOG(deviceInfo->lock);
    buffAll->buffLen = 0;
    buffAll->slogFlag = 1;
    buffAll->smpFlag = 0;
    buffAll->buff = (char *)malloc(RECV_FULLY_DATA_LEN + 1);
    if (buffAll->buff == NULL) {
        XFREE(deviceInfo->logBuff->buff);
        SELF_LOG_ERROR("malloc failed, strerr=%s. Thread(setupDeviceRecv) quit.", strerror(ToolGetErrorCode()));
        return MALLOC_FAILED;
    }
    (void)memset_s(buffAll->buff, RECV_FULLY_DATA_LEN + 1, 0, RECV_FULLY_DATA_LEN + 1);

    bufferTemp->buff = NULL;
    bufferTemp->buffLen = 0;
    bufferTemp->slogFlag = 0;
    buffAll->smpFlag = 0;

    return SUCCESS;
}

/**
* @brief PackageData: package data in LogMsgHead struct to LogBuff
* @param [in]logMsg: log data read by hdc, drv or other
* @param [in]buff: pointer to LogBuff.buff
* @param [in]len: pointer to LogBuff.buffLen
* @return: LogRt, SUCCESS/ARGV_NULL/INPUT_INVALID/MALLOC_FAILED/STR_COPY_FAILED
*/
STATIC LogRt PackageData(const LogMsgHead *logMsg, char **buff, unsigned int *len)
{
    ONE_ACT_WARN_LOG(logMsg == NULL, return ARGV_NULL, "[input] message head is null.");
    ONE_ACT_WARN_LOG(buff == NULL, return ARGV_NULL, "[input] log buffer is null.");

    if ((logMsg->data_len <= 0) || (logMsg->data_len > RECV_FULLY_DATA_LEN)) {
        SELF_LOG_WARN("[input] received data length is invalid, data_length=%d.", logMsg->data_len);
        return INPUT_INVALID;
    }

    *buff = (char*)malloc((unsigned int)(logMsg->data_len));
    if (*buff == NULL) {
        SELF_LOG_WARN("malloc failed, strerr=%s.", strerror(ToolGetErrorCode()));
        return MALLOC_FAILED;
    }
    (void)memset_s(*buff, (unsigned int)(logMsg->data_len), 0, (unsigned int)(logMsg->data_len));

    int err = memcpy_s(*buff, (unsigned int)(logMsg->data_len), logMsg->data, (unsigned int)(logMsg->data_len));
    if (err != EOK) {
        SELF_LOG_WARN("memcpy_s failed, result=%d, strerr=%s.", err, strerror(ToolGetErrorCode()));
        XFREE(*buff);
        return STR_COPY_FAILED;
    }
    *len = (unsigned int)(logMsg->data_len);

    return SUCCESS;
}

/**
 * @brief CreateDeviceRecv: Thread for receiving log data from device
 *                               Data buffer will be pushed into log queue
 * @param [in]arg: pointer to DeviceRes
 */
STATIC void CreateDeviceRecv(DeviceRes *deviceInfo)
{
    unsigned int deviceId = deviceInfo->devLogQueue->deviceId;
    NO_ACT_WARN_LOG(SetThreadName("setupDeviceRecv") != 0,
                    "set thread_name(setupDeviceRecv) failed, device_id=%u.", deviceId);
    TotalMsgBuf *totalMsgBuf = NULL;
    LogMsgHead *recvBuf = NULL;

    LogBuff buffAll = { NULL, 0, 0, 0 };
    LogBuff bufferTemp = { NULL, 0, 0, 0 };
    LogRt ret = PrepareRecvRes(deviceInfo, &buffAll, &bufferTemp);
    ONE_ACT_NO_LOG(ret != SUCCESS, return);

    LogRecvChildSourceDes childDes;
    childDes.parentSource = deviceInfo->parentSource;
    childDes.childSource = deviceInfo->childSource;
    childDes.peerDevId = (int)deviceId;
    childDes.peerNode = 0;

    (void)LogRecvSafeInitChildSource(&childDes);
    SELF_LOG_INFO("Thread(setupDeviceRecv) enter, device_id=%u.", deviceId);
    while (g_idThreadFlags[deviceId] != 0) {
        // read by hdc, drv or other
        ret = LogRecvSafeRead(&childDes, &totalMsgBuf, &recvBuf);
        // for device state change, eg: low power state
        TWO_ACT_ERR_LOG(ret == HDC_CHANNEL_CLOSE, LogRecvReleaseMsgBuf(totalMsgBuf, recvBuf), goto EXIT,
                        "[FATAL] device is offline, device_id=%u.", deviceId);
        // happens if deviceInfo equal NULL or parse recvmsg failed
        TWO_ACT_ERR_LOG(ret != SUCCESS, LogRecvReleaseMsgBuf(totalMsgBuf, recvBuf), goto EXIT,
                        "[FATAL] get log or parse recvmsg failed, result=%d, strerr=%s",
                        ret, strerror(ToolGetErrorCode()));

        bufferTemp.slogFlag = recvBuf->slog_flag & 0x1;
        bufferTemp.smpFlag = recvBuf->smp_flag & 0x1;
        ret = PackageData(recvBuf, &(bufferTemp.buff), &(bufferTemp.buffLen));
        TWO_ACT_ERR_LOG(ret != SUCCESS, LogRecvReleaseMsgBuf(totalMsgBuf, recvBuf), goto EXIT,
                        "[FATAL] package log data failed, result=%d.", ret);

        ret = AddBuffQueue(deviceInfo->devLogQueue, recvBuf, &buffAll, &bufferTemp, deviceInfo);
        TWO_ACT_ERR_LOG(ret != SUCCESS, LogRecvReleaseMsgBuf(totalMsgBuf, recvBuf), goto EXIT,
                        "[FATAL] enqueue failed, result=%d.", ret);

        XFREE(bufferTemp.buff);
        // release recvmsg
        LogRecvReleaseMsgBuf(totalMsgBuf, recvBuf);
    }
EXIT:
    XFREE(bufferTemp.buff);
    XFREE(buffAll.buff);
    (void)LogRecvReleaseChildSource(*(childDes.childSource));
}

/**
 * @brief SetupDeviceRecvThread: Thread for receiving log data from device
 *                               Data buffer will be pushed into log queue
 * @param [in]arg: pointer to DeviceRes
 */
STATIC void *SetupDeviceRecvThread(ArgPtr arg)
{
    ONE_ACT_NO_LOG(arg == NULL, return NULL);

    // Get device argument
    DeviceRes *deviceInfo = (DeviceRes*)arg;
    unsigned int deviceId = deviceInfo->devLogQueue->deviceId;
    // free arguments pointer
    XFREE(deviceInfo->mmRecvThreadArg);

    NO_ACT_WARN_LOG(SetThreadName("setupDeviceRecv") != 0,
                    "set thread_name(setupDeviceRecv) failed, device_id=%u.", deviceId);

    CreateDeviceRecv(deviceInfo);

    LOCK_WARN_LOG(deviceInfo->lock);
    XFREE(deviceInfo->logBuff->buff);
    UNLOCK_WARN_LOG(deviceInfo->lock);
    g_recvThreadState[deviceId] = 0;
    SELF_LOG_ERROR("Thread(setupDeviceRecv) quit, device_id=%u, thread_flag=%u.", deviceId, g_idThreadFlags[deviceId]);
    return NULL;
}

/**
* @brief: check whether specified time interval was passed by
* @param [in/out]lastTv: previous time value
* @return: pointer to result. 1 for time enough, 0 for not
*/
STATIC unsigned int TimerEnough(struct timeval *lastTv)
{
    ONE_ACT_NO_LOG(lastTv == NULL, return false);

    ToolTimeval realTv = {0, 0};
    if (ToolGetTimeOfDay(&realTv, NULL) != SYS_OK) {
        SELF_LOG_WARN("get time of day failed, strerr=%s.", strerror(ToolGetErrorCode()));
    }

    unsigned int flag = ((realTv.tv_sec - lastTv->tv_sec) >= WRITE_INTERVAL) ? true : false;

    lastTv->tv_sec = realTv.tv_sec;
    return flag;
}

STATIC INLINE void WriteThreadFailedHandle(const unsigned int deviceId)
{
    // set state flags to 0, tell recv thread to end
    g_writeThreadState[deviceId] = 0;
    g_idThreadFlags[deviceId] = 0;
}

/**
 * @brief SetupDeviceWriteThread: Thread for writing data in log queue to files
 *                                data in log queue will be freed after written
 * @param [in]arg: pointer to DeviceRes
 */
STATIC void *SetupDeviceWriteThread(ArgPtr arg)
{
    ONE_ACT_NO_LOG(arg == NULL, return NULL);

    int error;
    LogRt ret = SUCCESS;
    DeviceRes *deviceInfo = (DeviceRes*)arg;
    ONE_ACT_NO_LOG(deviceInfo->devLogQueue == NULL, return NULL);
    unsigned int deviceId = deviceInfo->devLogQueue->deviceId;
    g_writeThreadState[deviceId] = 1;
    // free arguments pointer
    XFREE(deviceInfo->mmWriteThreadArg);

    LogNode *logNode = NULL;
    NodesInfo nodesInfo = {0};
    struct timeval lastTv = {0, 0};

    NO_ACT_WARN_LOG(SetThreadName("setupDevWrite") != 0, "set thread_name(setupDevWrite) failed, devid=%u.", deviceId);
    // exit while loop if device receive thread exit
    SELF_LOG_INFO("Thread(setupDevWrite) enter, device_id=%u.", deviceId);
    while (g_recvThreadState[deviceId] != 0) {
        // Note: lock should be here, can not be putted in func BuffEnqueueAndAllocNew
        LOCK_WARN_LOG(deviceInfo->lock);
        unsigned int writeFlag = TimerEnough(&lastTv);
        if (writeFlag != 0) { // enqueue because of time interval reached
            ret = BuffEnqueueAndAllocNew(deviceInfo->devLogQueue, deviceInfo->logBuff, &(deviceInfo->nodeCnt));
            if ((ret != SUCCESS) && (ret != QUEUE_IS_FULL)) {
                UNLOCK_WARN_LOG(deviceInfo->lock);
                SELF_LOG_ERROR("[FATAL] queue is not full, but buffer enqueue failed, result=%d.", ret);
                break;
            }
        }
        // get data pointer and release lock
        ret = LogQueueDequeue(deviceInfo->devLogQueue, &logNode);
        UNLOCK_WARN_LOG(deviceInfo->lock);
        TWO_ACT_NO_LOG(ret == QUEUE_IS_NULL, (void)ToolSleep(TWO_HUNDRED_MILLISECOND), continue);
        ONE_ACT_WARN_LOG(ret != SUCCESS, break, "[FATAL] log queue dequeue failed, result=%d.", ret);

        nodesInfo.uiNodeNum = logNode->uiNodeNum;
        nodesInfo.stNodeData = logNode->stNodeData;

        DeviceWriteLogInfo deviceWriteLogInfo;
        deviceWriteLogInfo.deviceId = deviceId;
        deviceWriteLogInfo.smpFlag = logNode->smpFlag;
        deviceWriteLogInfo.slogFlag = logNode->slogFlag;
        deviceWriteLogInfo.len = logNode->uiNodeDataLen;

        error = LogAgentWriteDeviceLog(deviceInfo->pstLogFileList, (char*)&nodesInfo, &deviceWriteLogInfo);
        XFreeLogNode(&logNode);
        if (error != OK) {
            SELF_LOG_ERROR_N(&g_writeDPrintNum, GENERAL_PRINT_NUM, "write device log to file failed, device_id=%u,"
                             "result=%d, print once every %d times.", deviceId, error, GENERAL_PRINT_NUM);
        }
    }

    WriteThreadFailedHandle(deviceId);
    SELF_LOG_ERROR("Thread(setupDevWrite) quit, device_id=%u.", deviceId);
    return NULL;
}

STATIC INLINE void ClearThreadFlag(void)
{
    unsigned int i = 0;
    for (; i < MAX_DEV_NUM; i++) {
        g_idThreadFlags[i] = 0;
    }
}

STATIC LogRt GetDevStatus(unsigned int deviceId)
{
#if (OS_TYPE_DEF == LINUX)
    drvStatus_t status = DRV_STATUS_INITING;
    if (deviceId >= MAX_DEV_NUM) {
        SELF_LOG_WARN("wrong deviceId=%u.", deviceId);
        return FLAG_FALSE;
    }
    drvError_t ret = drvDeviceStatus(deviceId, &status);
    if (ret != DRV_ERROR_NONE) {
        SELF_LOG_WARN("get deviceId=%u status failed, result=%d.", deviceId, ret);
        return FLAG_FALSE;
    }

    return (status == DRV_STATUS_WORK) ? SUCCESS : FLAG_FALSE;
#else
    return SUCCESS;
#endif
}

/**
* @brief: each device has two thread, write thread is eixt after read
*         so use write thread state to check device threads state
* @return: SUCCESS/FLAG_FALSE
*/
STATIC bool IsDeviceThreadCleanedUp(void)
{
    unsigned int i = 0;
    for (; i < MAX_DEV_NUM; i++) {
        if (g_writeThreadState[i] != 0) {
            return false;
        }
    }
    return true;
}

/**
* @brief: used to clean up resources shared between recv and write thread
* @param [in]deviceInfo: struct which contains device resources
*/
STATIC void CleanUpSharedRes(DeviceRes *deviceInfo)
{
    ONE_ACT_NO_LOG(deviceInfo == NULL, return);
    LOCK_WARN_LOG(deviceInfo->lock);
    (void)LogQueueFree(deviceInfo->devLogQueue, XFreeLogNode);
    XFREE(deviceInfo->devLogQueue);
    if (deviceInfo->logBuff != NULL) {
        XFREE(deviceInfo->logBuff->buff);
        XFREE(deviceInfo->logBuff);
    }
    UNLOCK_WARN_LOG(deviceInfo->lock);
    if (deviceInfo->lock != NULL) {
        (void)ToolMutexDestroy(deviceInfo->lock);
    }
}

/**
 * @brief CleanUpAllResources: clean up all device resources
 * @param [in]pstLogFileList: all device log files
 */
STATIC void CleanUpAllResources(StLogFileList *pstLogFileList)
{
    unsigned int i = 0;
    // nonexistent device will be ignored since there DeviceRes has been initialised to NULL
    for (i = 0; i < MAX_DEV_NUM; i++) {
#ifndef __IDE_UT
        CleanUpSharedRes(&(g_deviceResourcs[i]));
#endif
    }
    // free opened files
    LogAgentCleanUpDevice(pstLogFileList);
}

STATIC LogRt SetupThread(DeviceRes *deviceArg, const unsigned int deviceId, ThreadFunc func)
{
    ONE_ACT_WARN_LOG(deviceArg == NULL, return ARGV_NULL, "[input] device info is null.");
    ONE_ACT_WARN_LOG(func == NULL, return ARGV_NULL, "[input] thread function is null.");

    // create recv thread
    ToolUserBlock *threadInfo = (ToolUserBlock *)malloc(sizeof(ToolUserBlock));
    if (threadInfo == NULL) {
        SELF_LOG_WARN("malloc failed, device_id=%u, strerr=%s.", deviceId, strerror(ToolGetErrorCode()));
        return MALLOC_FAILED;
    }
    (void)memset_s(threadInfo, sizeof(ToolUserBlock), 0, sizeof(ToolUserBlock));

    threadInfo->procFunc = func;
    threadInfo->pulArg = (void *)deviceArg;
    // recv & write thread shared same 'DeviceRes', so save threadInfo in different member
    if (func == SetupDeviceRecvThread) {
        deviceArg->mmRecvThreadArg = threadInfo;
    } else {
        deviceArg->mmWriteThreadArg = threadInfo;
    }

    toolThread thread = 0;
    int err = ToolCreateTaskWithDetach(&thread, threadInfo);
    if (err != SYS_OK) {
        SELF_LOG_WARN("create thread failed, device_id=%u, result=%d, strerr=%s.",
                      deviceId, err, strerror(ToolGetErrorCode()));
        XFREE(threadInfo);
        return CREAT_THREAD_ERR;
    }
    return SUCCESS;
}

STATIC LogRt PrepareThreadRes(const unsigned int deviceId)
{
    int err = ToolMutexInit(&(g_deviceThreadLock[deviceId]));
    ONE_ACT_WARN_LOG(err != SYS_OK, return QUEUE_LOCK_INIT_ERR, "init mutex failed, " \
                     "device_id=%u, result=%d, strerr=%s.", deviceId, err, strerror(ToolGetErrorCode()));
    // avoid memory leak when thread restart
    DeviceRes *deviceInfo = &(g_deviceResourcs[deviceId]);
    if (deviceInfo != NULL) {
        LOCK_WARN_LOG(&(g_deviceThreadLock[deviceId]));
        (void)LogQueueFree(deviceInfo->devLogQueue, XFreeLogNode);
        XFREE(deviceInfo->devLogQueue);
        if (deviceInfo->logBuff != NULL) {
            XFREE(deviceInfo->logBuff->buff);
            XFREE(deviceInfo->logBuff);
        }
        UNLOCK_WARN_LOG(&(g_deviceThreadLock[deviceId]));
    }

    // set and init different device buffer queue
    LogQueue *devLogQueue = (LogQueue *)malloc(sizeof(LogQueue));
    if (devLogQueue == NULL) {
        SELF_LOG_WARN("malloc failed, device_id=%u, strerr=%s.", deviceId, strerror(ToolGetErrorCode()));
        ToolMutexDestroy(&(g_deviceThreadLock[deviceId]));
        return MALLOC_FAILED;
    }
    (void)memset_s(devLogQueue, sizeof(LogQueue), 0, sizeof(LogQueue));

    // init log buffer queue for every device,one buffer queue for one device
    LogRt ret = LogQueueInit(devLogQueue, (int)deviceId);
    if (ret != SUCCESS) {
        XFREE(devLogQueue);
        ToolMutexDestroy(&(g_deviceThreadLock[deviceId]));
        SELF_LOG_WARN("init log queue failed, device_id=%u, result=%d.", deviceId, ret);
        return QUEUE_INIT_ERR;
    }

    LogBuff *logBuff = (LogBuff *)malloc(sizeof(LogBuff));
    if (logBuff == NULL) {
        SELF_LOG_WARN("malloc failed, device_id=%u, strerr=%s.", deviceId, strerror(ToolGetErrorCode()));
        ToolMutexDestroy(&(g_deviceThreadLock[deviceId]));
        XFREE(devLogQueue);
        return MALLOC_FAILED;
    }
    logBuff->buff = NULL; // device buffer
    logBuff->buffLen = 0; // device buffer has been used length
    logBuff->slogFlag = 1; // to distinguish device and os
    logBuff->smpFlag = 0; // device is smp or not
    g_deviceResourcs[deviceId].logBuff = logBuff;
    g_deviceResourcs[deviceId].lock = &(g_deviceThreadLock[deviceId]); // device thread operate lock
    g_deviceResourcs[deviceId].devLogQueue = devLogQueue;

    return SUCCESS;
}

/**
 * @brief SetupDeviceThread: set up receive & write thread for specified device
 *                           this will be recalled if create failed
 * @param [in]deviceId: device id
 * @param [in]pstLogFileList: log file lists for all devices
 * @param [in]parentSource: parent source
 * @param [in]childSource: pointer to array of all child sources
 * @return: LogRt, SUCCESS/ARGV_NULL/QUEUE_LOCK_INIT_ERR/MALLOC_FAILED/
 *                 QUEUE_INIT_ERR/CREAT_THREAD_ERR
 */
STATIC LogRt SetupDeviceThread(const unsigned int deviceId, StLogFileList *pstLogFileList,
                               const ParentSource parentSource, ChildSource **childSources)
{
    ONE_ACT_WARN_LOG(pstLogFileList == NULL, return ARGV_NULL, "[input] log file list is null.");
    ONE_ACT_WARN_LOG(parentSource == NULL, return ARGV_NULL, "[input] parent source is null.");
    ONE_ACT_WARN_LOG(childSources == NULL, return ARGV_NULL, "[input] child source is null.");

    g_deviceResourcs[deviceId].parentSource = parentSource;
    g_deviceResourcs[deviceId].childSource = &((*childSources)[deviceId]);
    g_deviceResourcs[deviceId].pstLogFileList = pstLogFileList; // log file lists for all devices
    g_deviceResourcs[deviceId].nodeCnt = 0; // recv times in device buffer

    LogRt ret = PrepareThreadRes(deviceId);
    ONE_ACT_NO_LOG(ret != SUCCESS, return ret);
    // setup recv thread
    ret = SetupThread(&(g_deviceResourcs[deviceId]), deviceId, SetupDeviceRecvThread);
    if (ret != SUCCESS) {
        SELF_LOG_WARN("set and create thread(setupDeviceRecv) failed, device_id=%u.", deviceId);
        CleanUpSharedRes(&(g_deviceResourcs[deviceId]));
        return CREAT_THREAD_ERR;
    }

    // set up write files thread
    ret = SetupThread(&(g_deviceResourcs[deviceId]), deviceId, SetupDeviceWriteThread);
    if (ret != SUCCESS) {
        SELF_LOG_WARN("set and create thread(setupDevWrite) failed, device_id=%u.", deviceId);
        g_idThreadFlags[deviceId] = 0;
        // when g_idThreadFlags becomes zero, g_recvThreadState will be automatically set to zero
        while (g_recvThreadState[deviceId] != 0) {
            (void)ToolSleep(ONE_HUNDRED_MILLISECOND);
        }
        CleanUpSharedRes(&(g_deviceResourcs[deviceId]));
        return CREAT_THREAD_ERR;
    }

    return SUCCESS;
}

STATIC void DeviceThreadCheck(unsigned int *preDevNum, StLogFileList *pstLogFileList,
                              const ParentSource parentSource, ChildSource **childSources)
{
    ONE_ACT_NO_LOG((preDevNum == NULL) || (pstLogFileList == NULL) || (childSources == NULL), return);
    unsigned int deviceIdArray[MAX_DEV_NUM] = { 0 };    // device-side device id array
    unsigned int devNum = 0;
    LogRt ret = GetDevNumIDs(&devNum, deviceIdArray);
    if (ret != SUCCESS) {
        (void)ToolSleep(ONE_SECOND);
        return;
    }
    devNum = (devNum > MAX_DEV_NUM) ? MAX_DEV_NUM : devNum;

    // monitor device num change
    if (*preDevNum != devNum) {
        SELF_LOG_WARN("device number changed, origin=%u, current=%u.", *preDevNum, devNum);
        *preDevNum = devNum;
    }
    unsigned int i = 0;
    for (; i < devNum; i++) {
        unsigned int deviceId = deviceIdArray[i];
        if (deviceId >= MAX_DEV_NUM) {
            SELF_LOG_WARN("device id is invalid, device_id=%u, max_device_id=%u.", deviceId, MAX_DEV_NUM - 1);
            continue;
        }

        // if device thread exists, continue
        if ((g_idThreadFlags[deviceId] == 1) || (g_recvThreadState[deviceId] == 1) ||
            (GetDevStatus(deviceId) != SUCCESS)) {
            continue;
        }

        // set up 2 thread for every device, 1 for recv and 1 for write files, recv is blocked
        g_idThreadFlags[deviceId] = 1;
        g_recvThreadState[deviceId] = 1; // reset recv state as initial value
        ret = SetupDeviceThread(deviceId, pstLogFileList, parentSource, childSources);
        if (ret != SUCCESS) {
            g_idThreadFlags[deviceId] = 0;
            g_recvThreadState[deviceId] = 0;
            SELF_LOG_ERROR("set and create thread failed, device_id=%u, result=%d.", deviceId, ret);
            continue;
        }
    }
    // flush device state, every two seconds
    (void)ToolSleep(TWO_SECOND);
}

STATIC INLINE void ThreadExitHandle(void)
{
    g_initError = 1;
    g_allThreadExit = 1;
}

STATIC bool IsHostDrvReady(void)
{
#ifdef EP_DEVICE_MODE
    unsigned int hostDeviceId = 0;
    int ret = drvGetDevIDByLocalDevID(0, &hostDeviceId);
    if (ret != SYS_OK || hostDeviceId == MAX_DEV_NUM) {
        ToolSleep(ONE_SECOND);
        return false;
    }
#endif
    return true;
}
/**
 * @brief HostRTGetDeviceIDThread: Thread to get device ID and monitor every device thread state
 * @param [in]arg: pointer to GetIDArg, contians parent & child source
 */
STATIC void *HostRTGetDeviceIDThread(ArgPtr arg)
{
    ONE_ACT_NO_LOG(arg == NULL, return NULL);

    GetIDArg *idArgs = (GetIDArg *)arg;
    ParentSource parentSource = idArgs->parentSource;
    ChildSource **childSources = idArgs->childSources;
    XFREE(idArgs->stMmThreadArg);
    XFREE(idArgs);

    StLogFileList pstLogFileList = { 0 };
    unsigned int preDevNum = 0;
    NO_ACT_WARN_LOG(SetThreadName("hostGetDevID") != 0, "set thread_name(hostGetDevID) failed.");
    // wait until host side ready
    bool readyFlag = false;
    unsigned int retryTime = 0;
    do {
        readyFlag = IsHostDrvReady();
        retryTime++;
    } while ((readyFlag == false) && (retryTime <= MAX_READY_RETYR_TIMES));
    SELF_LOG_INFO("drvGetDevIDByLocalDevID flag=%d, retryTime=%u.", readyFlag, retryTime);

    // Note: pstLogFileList should be free if thread exit while process alive
    // read configure file
    if (LogAgentGetCfg(&pstLogFileList) != OK) {
        SELF_LOG_WARN("read and init config failed.");
        return NULL;
    }

    unsigned int errT = LogAgentInitDevice(&pstLogFileList, MAX_DEV_NUM);
    if (errT != (unsigned int)OK) {
        SELF_LOG_ERROR("[FATAL] init log file list failed, result=%u, Thread(hostGetDevID) quit.", errT);
        ThreadExitHandle();
        return NULL;
    }
    // Real time check device num, exit when SIGTERM received
    while (GetSigFlag() != 0) {
        DeviceThreadCheck(&preDevNum, &pstLogFileList, parentSource, childSources);
    }
    // notify all device recv thread to exit, write thread will exit after recv thread exited
    ClearThreadFlag();
    // wait write thread exit
    bool cleanFlag = false;
    do {
        cleanFlag = IsDeviceThreadCleanedUp();
    } while (!cleanFlag);

    CleanUpAllResources(&pstLogFileList);
    SELF_LOG_ERROR("Thread(hostGetDevID) quit.");
    g_allThreadExit = 1;
    return NULL;
}

/**
 * @brief LogRecvInit: create thread HostRTGetDeviceIDThread to monitor device and deviceThread
 * @param [in]parentSource: parent source
 * @param [in]childSources: pointer to array of all child source
 * @param [in]childSourceNum: number of child source
 * @return: LogRt, SUCCESS/ARGV_NULL/MALLOC_FAILED/CREAT_THREAD_ERR
 */
LogRt LogRecvInit(const ParentSource parentSource, ChildSource **childSources, unsigned int childSourceNum)
{
    toolThread hostLogThread = 0;
    ONE_ACT_WARN_LOG(parentSource == NULL, return ARGV_NULL, "[input] parent source is null.");
    ONE_ACT_WARN_LOG(childSources == NULL, return ARGV_NULL, "[input] child sources is null.");
    ONE_ACT_WARN_LOG(childSourceNum > MAX_DEV_NUM, return ARGV_NULL,
                     "[input] child source number is illegal, number=%u.", childSourceNum);

    // init all devices' resources pointers which will be freed on exit as NULL
    unsigned int i;
    for (i = 0; i < childSourceNum; i++) {
        g_deviceResourcs[i].devLogQueue = NULL;
        g_deviceResourcs[i].lock = NULL;
        g_deviceResourcs[i].logBuff = NULL;
        // recv thread states should be initialised as 0
        g_recvThreadState[i] = 0;
    }

    GetIDArg *idArgs = (GetIDArg *)malloc(sizeof(GetIDArg));
    if (idArgs == NULL) {
        SELF_LOG_WARN("malloc failed, strerr=%s.", strerror(ToolGetErrorCode()));
        return MALLOC_FAILED;
    }
    (void)memset_s(idArgs, sizeof(GetIDArg), 0, sizeof(GetIDArg));

    idArgs->parentSource = parentSource;
    idArgs->childSources = childSources;
    ToolUserBlock *threadGetIDs = (ToolUserBlock *)malloc(sizeof(ToolUserBlock));
    if (threadGetIDs == NULL) {
        SELF_LOG_WARN("malloc failed, strerr=%s.", strerror(ToolGetErrorCode()));
        XFREE(idArgs);
        return MALLOC_FAILED;
    }
    (void)memset_s(threadGetIDs, sizeof(ToolUserBlock), 0, sizeof(ToolUserBlock));
    idArgs->stMmThreadArg = threadGetIDs; // Note:threadGetIDs should be free within thread

    threadGetIDs->procFunc = HostRTGetDeviceIDThread;
    threadGetIDs->pulArg = idArgs;
    int errT = ToolCreateTaskWithDetach(&hostLogThread, threadGetIDs);
    if (errT != SYS_OK) {
        SELF_LOG_WARN("create thread failed with detach, result=%d, strerr=%s.", errT, strerror(ToolGetErrorCode()));
        XFREE(threadGetIDs);
        XFREE(idArgs);
        return CREAT_THREAD_ERR;
    }
    SELF_LOG_INFO("create thread(HostRTGetDeviceIDThread) succeed.");
    return SUCCESS;
}
