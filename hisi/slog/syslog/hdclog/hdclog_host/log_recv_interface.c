/**
* @log_recv_interface.c
*
* Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#include "log_recv_interface.h"
#include "log_common_util.h"
#include "log_recv.h"
#include "print_log.h"


#if defined(RECV_BY_DRV)
#include "ascend_hal.h"
void LogRecvReleaseMsgBuf(TotalMsgBuf *totalMsgBuf, LogMsgHead *recvBuf)
{
    (void)totalMsgBuf;
    XFREE(recvBuf);
    return;
}

LogRt LogRecvInitParentSource(ParentSource *parentSource)
{
    ONE_ACT_WARN_LOG(parentSource == NULL, return ARGV_NULL, "[input] input is null.");
    *parentSource = (ParentSource)PARENTSOURCE_INSTANCE;
    return SUCCESS;
}

LogRt LogRecvReleaseParentSource(ParentSource parentSource)
{
    (void)parentSource;
    return SUCCESS;
}

LogRt LogRecvInitChildSource(const LogRecvChildSourceDes *childSourceDes)
{
    (void)childSourceDes;
    return SUCCESS;
}

LogRt LogRecvSafeInitChildSource(const LogRecvChildSourceDes *childSourceDes)
{
    (void)childSourceDes;
    return SUCCESS;
}

LogRt LogRecvReleaseChildSource(ChildSource childSource)
{
    (void)childSource;
    return SUCCESS;
}

void LogRecvReleaseAllSource(const LogRecvAllSourceDes *allSourceDes)
{
    (void)allSourceDes;
    return;
}

LogRt LogRecvRead(const ChildSource childSource, TotalMsgBuf **totalMsgBuf, int *bufCount)
{
    (void)childSource;
    (void)totalMsgBuf;
    (void)bufCount;
    return SUCCESS;
}

LogRt LogRecvSafeRead(const LogRecvChildSourceDes *childDes, TotalMsgBuf **totalMsgBuf, LogMsgHead **recvBuf)
{
    (void)totalMsgBuf;
    ONE_ACT_WARN_LOG(recvBuf == NULL, return ARGV_NULL, "[input] input recvBuf is null.");

    unsigned int bufLen = (unsigned int)(sizeof(LogMsgHead) + DRV_RECV_MAX_LEN + 1);
    const int deviceId = childDes->peerDevId;
    LogMsgHead *tmpNode = (LogMsgHead *)malloc(bufLen);
    if (tmpNode == NULL) {
        SELF_LOG_WARN("malloc failed, strerr=%s.", strerror(ToolGetErrorCode()));
        return MALLOC_FAILED;
    }

    (void)memset_s(tmpNode, bufLen, 0x00, bufLen);
    int ret = log_read(deviceId, (char *)tmpNode, &bufLen, -1); // -1 means 'infinite timeout'
    if (ret != SYS_OK) {
        SELF_LOG_WARN("receive log failed, device_id=%d, result=%d, strerr=%s.",
                      deviceId, ret, strerror(ToolGetErrorCode()));
        XFREE(tmpNode);
        return LOG_RECV_FAILED;
    }

    *recvBuf = tmpNode;
    return SUCCESS;
}
#else
static unsigned int g_connPrintNum = 0;
void LogRecvReleaseMsgBuf(TotalMsgBuf *totalMsgBuf, LogMsgHead *recvBuff)
{
    (void)recvBuff;
    if (totalMsgBuf != NULL) {
        drvHdcFreeMsg((struct drvHdcMsg *)totalMsgBuf);
        totalMsgBuf = NULL;
    }
}

LogRt LogRecvInitParentSource(ParentSource *parentSource)
{
    ONE_ACT_WARN_LOG(parentSource == NULL, return ARGV_NULL, "[input] hdc client is null.");

    // Note: here log type is HDCDRV_SERVICE_TYPE_LOG, difference with IDE and other client
    hdcError_t error = drvHdcClientCreate(parentSource, MAX_HDC_SESSION_NUM, HDCDRV_SERVICE_TYPE_LOG, 0);
    if (error != DRV_ERROR_NONE) {
        SELF_LOG_WARN("create hdc client failed, result=%d, strerr=%s.", error, strerror(ToolGetErrorCode()));
        return INIT_PARENTSOURCE_ERR;
    }
    return SUCCESS;
}

LogRt LogRecvReleaseParentSource(ParentSource parentSource)
{
    if (parentSource != NULL) {
        hdcError_t error = drvHdcClientDestroy(parentSource);
        if (error != DRV_ERROR_NONE) {
            SELF_LOG_WARN("destroy hdc client failed, result=%d, strerr=%s.", error, strerror(ToolGetErrorCode()));
            return RELEASE_PARENTSOURCE_ERR;
        }
    }
    return SUCCESS;
}

LogRt LogRecvInitChildSource(const LogRecvChildSourceDes *childSourceDes)
{
    ONE_ACT_WARN_LOG(childSourceDes == NULL, return ARGV_NULL, "[input] input child source des is null.");
    int peerNode = childSourceDes->peerNode;
    int peerDevId = childSourceDes->peerDevId;
    ParentSource client = childSourceDes->parentSource;
    ChildSource *session = childSourceDes->childSource;

    ONE_ACT_WARN_LOG(client == NULL, return ARGV_NULL, "[input] hdc client is null.");
    ONE_ACT_WARN_LOG(session == NULL, return ARGV_NULL, "[input] hdc session is null.");

    hdcError_t error = drvHdcSessionConnect(peerNode, peerDevId, client, session);
    if (error != DRV_ERROR_NONE) {
        SELF_LOG_ERROR_N(&g_connPrintNum, CONN_E_PRINT_NUM,
                         "create hdc connection failed, result=%d, strerr=%s, print once every %d times.",
                         error, strerror(ToolGetErrorCode()), CONN_E_PRINT_NUM);
        return INIT_CHILDSOURCE_ERR;
    }

    (void)drvHdcSetSessionReference(*session);
    return SUCCESS;
}

LogRt LogRecvReleaseChildSource(ChildSource childSource)
{
    ONE_ACT_WARN_LOG(childSource == NULL, return ARGV_NULL, "[input] session is null.");

    hdcError_t error = drvHdcSessionClose(childSource);
    if (error != DRV_ERROR_NONE) {
        SELF_LOG_WARN("close hdc connection failed, result=%d, strerr=%s.", error, strerror(ToolGetErrorCode()));
        return RELEASE_CHILDSOURCE_ERR;
    }
    return SUCCESS;
}

LogRt LogRecvRead(const ChildSource childSource, TotalMsgBuf **totalMsgBuf, int *bufCount)
{
    int count = 1;
    int flag = 0;  // 0: wait; 1: no wait; 2: wait timeout(default timeout 3s)
    int timeout = 1; // because flag=0, timeout no use

    ONE_ACT_WARN_LOG(childSource == NULL, return ARGV_NULL, "[input] hdc session is null.");
    ONE_ACT_WARN_LOG(totalMsgBuf == NULL, return ARGV_NULL, "[input] hdc message is null.");
    ONE_ACT_WARN_LOG(bufCount == NULL, return ARGV_NULL, "[input] received buffer count is null.");

    hdcError_t error = drvHdcAllocMsg(childSource, (struct drvHdcMsg **)totalMsgBuf, count);
    if (error != DRV_ERROR_NONE) {
        SELF_LOG_WARN("alloc buffer failed by hdc, result=%d, strerr=%s.", error, strerror(ToolGetErrorCode()));
        return MALLOC_FAILED;
    }
    error = halHdcRecv(childSource, (struct drvHdcMsg *)(*totalMsgBuf), HDC_RECV_MAX_LEN, flag, bufCount, timeout);
    if (error == DRV_ERROR_NON_BLOCK) {
        SELF_LOG_WARN("receive message failed by hdc, result=%d, strerr=%s.", error, strerror(ToolGetErrorCode()));
        LogRecvReleaseMsgBuf(*totalMsgBuf, NULL);
        *totalMsgBuf = NULL;
        return LOG_RECV_FAILED;
    } else if (error == DRV_ERROR_SOCKET_CLOSE) {
        SELF_LOG_WARN("receive message failed by hdc, result=%d, strerr=%s.", error, strerror(ToolGetErrorCode()));
        LogRecvReleaseMsgBuf(*totalMsgBuf, NULL);
        *totalMsgBuf = NULL;
        return HDC_CHANNEL_CLOSE;
    } else if (error != DRV_ERROR_NONE) {
        SELF_LOG_WARN("receive message failed by hdc, result=%d, strerr=%s.", error, strerror(ToolGetErrorCode()));
        LogRecvReleaseMsgBuf(*totalMsgBuf, NULL);
        *totalMsgBuf = NULL;
        return LOG_RECV_FAILED;
    } else {
        return SUCCESS;
    }
}

LogRt LogRecvSafeInitChildSource(const LogRecvChildSourceDes *childSourceDes)
{
    LogRt error = SUCCESS;
    unsigned char sessionState = 0;
    ONE_ACT_WARN_LOG(childSourceDes == NULL, return sessionState, "[input] input des is null.");
    const int deviceId = childSourceDes->peerDevId;
    ONE_ACT_WARN_LOG((deviceId >= MAX_DEV_NUM || deviceId < 0), return INIT_CHILDSOURCE_ERR,
                     "[input] device_id=%u is invalid.", deviceId);
    while (!sessionState && IsDeviceThreadAlive(deviceId)) {
        error = LogRecvInitChildSource(childSourceDes);
        if (error != SUCCESS) {
            SELF_LOG_WARN_N(&g_connPrintNum, CONN_W_PRINT_NUM,
                            "create session failed, device_id=%d, result=%d, strerr=%s, print once every %d times.",
                            deviceId, error, strerror(ToolGetErrorCode()), CONN_W_PRINT_NUM);
            (void)ToolSleep(TWO_SECOND); // wait 2s for next connect
            sessionState = 0;
            continue;
        }
        sessionState = 1;
    }
    SELF_LOG_INFO("create log session succeed, device_id=%d.", deviceId);
    return ((sessionState == 1) ? SUCCESS : INIT_CHILDSOURCE_ERR);
}

LogRt LogRecvSafeRead(const LogRecvChildSourceDes *childDes, TotalMsgBuf **totalMsgBuf, LogMsgHead **recvBuf)
{
    ONE_ACT_NO_LOG(childDes == NULL, return ARGV_NULL);
    ONE_ACT_NO_LOG(totalMsgBuf == NULL, return ARGV_NULL);
    ONE_ACT_NO_LOG(recvBuf == NULL, return ARGV_NULL);

    int recvLen = 0;
    const int deviceId = childDes->peerDevId;
    LogRt error = LOG_RECV_FAILED;
    while (error != SUCCESS && IsDeviceThreadAlive(deviceId)) {
        // read by HDC
        error = LogRecvRead(*(childDes->childSource), totalMsgBuf, &recvLen);
        if (error == HDC_CHANNEL_CLOSE && !IsDeviceThreadAlive(deviceId)) {
            SELF_LOG_WARN("read log interrupted by hdc, device is offline, device_id=%d", deviceId);
            break;
        } else if (error != SUCCESS) {
            SELF_LOG_WARN("read log failed by hdc, try to create session again, device_id=%d.", deviceId);
            LogRt ret = LogRecvReleaseChildSource(*(childDes->childSource));
            ONE_ACT_WARN_LOG(ret != SUCCESS, ToolSleep(TWO_SECOND),
                             "destroy hdc session failed, device_id=%d, result=%d, strerr=%s.",
                             deviceId, ret, strerror(ToolGetErrorCode()));
            *(childDes->childSource) = 0;
            (void)LogRecvSafeInitChildSource(childDes);
            continue;
        }

        if ((*totalMsgBuf == NULL) || (recvLen == 0)) {
            (void)ToolSleep(ONE_SECOND);
            error = LOG_RECV_FAILED;
            XFREE(*totalMsgBuf);
            continue;
        }

        // parse hdc recv pmsg
        int buffLen = 0;
        hdcError_t drvErr = drvHdcGetMsgBuffer((struct drvHdcMsg *)*totalMsgBuf, 0, (char **)recvBuf, &buffLen);
        if (drvErr != DRV_ERROR_NONE) {
            LogRecvReleaseMsgBuf(*totalMsgBuf, NULL);
            *totalMsgBuf = NULL;
            SELF_LOG_ERROR("[FATAL] get log failed by hdc, result=%d, strerr=%s.",
                           drvErr, strerror(ToolGetErrorCode()));
            return LOG_RECV_FAILED;
        }
    }
    return error;
}

void LogRecvReleaseAllSource(const LogRecvAllSourceDes *allSourceDes)
{
    ONE_ACT_WARN_LOG(allSourceDes == NULL, return, "[input] input des is null.");
    const ParentSource parentSource = allSourceDes->parentSource;
    LogRt ret = LogRecvReleaseParentSource(parentSource);
    ONE_ACT_WARN_LOG(ret != SUCCESS, return, "release parent source failed.");
}
#endif
