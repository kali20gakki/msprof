/**
 * @hdclog_device_init.c
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#include "hdclog_device_init.h"

#include <malloc.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <unistd.h>
#include <stdbool.h>

#include "securec.h"
#include "log_sys_package.h"

#include "cfg_file_parse.h"
#include "dlog_error_code.h"
#include "hdclog_com.h"
#include "msg_queue.h"
#include "print_log.h"

const long KEY_VALUE_ONE = 0x474f4c44;
toolMutex g_setLogLevelLock;

int HdclogDeviceInit(void)
{
    InitCfgAndSelfLogPath();
    int ret = ToolMutexInit(&g_setLogLevelLock);
    ONE_ACT_WARN_LOG(ret != SYS_OK, return HDCLOG_MUTEX_INIT_FAILED,
                     "init log level setting mutex failed, result=%d, strerr=%s.", ret, strerror(ToolGetErrorCode()));
    return HDCLOG_SUCCESSED;
}

int HdclogDeviceDestroy(void)
{
    FreeCfgAndSelfLogPath();
    ToolMutexDestroy(&g_setLogLevelLock);
    return HDCLOG_SUCCESSED;
}

/*
 * @brief: response level setting result to host
 * @param [in]session: hdc session
 * @param [in]errMsg: level setting error message
 * @param [in]errLen: level setting error length
 * @return: HDCLOG_SUCCESSED: succeed; others: failed
 */
STATIC HdclogErr RespSettingResult(HDC_SESSION session, const char *errMsg, unsigned int errLen)
{
    int ret = AdxHdcWrite(session, errMsg, errLen);
    if (ret < 0) {
        SELF_LOG_WARN("write level setting result to host failed, result=%d.", ret);
        return HDCLOG_WRITE_FAILED;
    }

    return HDCLOG_SUCCESSED;
}

/*
 * @brief: receive level setting result from slogd
 * @param [in/out]rcvMsg: received message
 * @param [in]session: hdc session
 * @param [in]queueId: message queue id
 * @return: HDCLOG_SUCCESSED: succeed; others: failed
 */
HdclogErr RecvSettingResult(DlogMsgT *rcvMsg, HDC_SESSION session, int queueId)
{
    int rcvRes = 0;
    int retryTimes = 0;
    do {
        rcvRes = msgrcv(queueId, (void *)rcvMsg, MSG_MAX_LEN, FEEDBACK_MSG_TYPE, IPC_NOWAIT);
        if (!(rcvRes < 0)) {
            break;
        }
        if (ToolGetErrorCode() == ENOMSG) {
            retryTimes++;
            if (retryTimes == MSG_RETRY_TIMES) {
                break;
            }
            ToolSleep(100); // sleep 100ms
            continue;
        }
        if (ToolGetErrorCode() == EINTR) { // interrupted system call
            continue;
        }
        break;
    } while (1);
    if (rcvRes < 0) {
        int errCode = ToolGetErrorCode();
        char *errMsg = ((errCode == EINVAL) || (errCode == EIDRM)) || (retryTimes == MSG_RETRY_TIMES) ?
                       SLOGD_RCV_ERROR_MSG : UNKNOWN_ERROR_MSG;
        SELF_LOG_WARN("receive level setting result failed from slogd, result=%d, strerr=%s, retry_time=%d.",
                      rcvRes, strerror(errCode), retryTimes);
        (void)RespSettingResult(session, errMsg, strlen(errMsg));
        return HDCLOG_MSGQUEUE_RECV_FAILED;
    }
    return HDCLOG_SUCCESSED;
}

/**
 * @brief ClearReceiveMessage: Clear receive message
 * @param [in]queueid: message queue id
 */
void ClearRecvMessage(toolMsgid queueid)
{
    DlogMsgT rcvMsg = { 0, "" };
    do {
        int rcvRes = msgrcv(queueid, (void *)(&rcvMsg), MSG_MAX_LEN, FEEDBACK_MSG_TYPE, IPC_NOWAIT);
        if (rcvRes < 0 && ToolGetErrorCode() != EINTR) {
            break;
        }
    } while (1);
}

/*
 * @brief: send level info to slogd, and wait to receive result
 * @param [in]session: hdc session
 * @param [in]stMsg: message info include level info
 * @return: HDCLOG_SUCCESSED: succeed; others: failed
 */
HdclogErr DoParseDeviceLogCmd(HDC_SESSION session, const LogMsgT *stMsg)
{
    key_t key1 = (key_t)KEY_VALUE_ONE;

    int queueId = msgget(key1, 0);
    if (queueId < 0) {
        int errCode = ToolGetErrorCode();
        SELF_LOG_WARN("get message queue failed, strerr=%s.", strerror(errCode));
        char *errMsg = (errCode == ENOENT) ? SLOGD_ERROR_MSG : UNKNOWN_ERROR_MSG;
        (void)RespSettingResult(session, errMsg, strlen(errMsg));
        return HDCLOG_MSGQUEUE_GET_FAILED;
    }

    LOCK_WARN_LOG(&g_setLogLevelLock);
    ClearRecvMessage(queueId);
    // send message to notify the slog
    int sendRes = 0;
    do {
        sendRes = msgsnd(queueId, (void *)(stMsg), MSG_MAX_LEN, IPC_NOWAIT);
    } while ((sendRes < 0) && (ToolGetErrorCode() == EINTR)); // interrupted system call
    if (sendRes < 0) {
        int errCode = ToolGetErrorCode();
        char *errMsg = ((errCode == EINVAL) || (errCode == EIDRM)) ? SLOGD_ERROR_MSG : UNKNOWN_ERROR_MSG;
        SELF_LOG_WARN("send level info to slogd failed, strerr=%s.", strerror(errCode));
        UNLOCK_WARN_LOG(&g_setLogLevelLock);
        (void)RespSettingResult(session, errMsg, strlen(errMsg));
        return HDCLOG_MSGQUEUE_SEND_FAILED;
    }
    SELF_LOG_INFO("send level info to slogd succeed.");

    // receive level setting result from slog
    DlogMsgT rcvMsg = { 0, "" };
    HdclogErr rcvRes = RecvSettingResult(&rcvMsg, session, queueId);
    if (rcvRes != HDCLOG_SUCCESSED) {
        UNLOCK_WARN_LOG(&g_setLogLevelLock);
        return rcvRes;
    }
    UNLOCK_WARN_LOG(&g_setLogLevelLock);
    // response result to host from slogd
    char *resultMsg = (strcmp(rcvMsg.msgData, "") == 0) ? UNKNOWN_ERROR_MSG : rcvMsg.msgData;
    if (strcmp(LEVEL_SETTING_SUCCESS, resultMsg) == 0) {
        SELF_LOG_INFO("set device level succeed.");
    } else {
        SELF_LOG_WARN("set device level failed, result_info=%s.", resultMsg);
    }
    return RespSettingResult(session, resultMsg, strlen(resultMsg));
}

/*
 * @brief: parse device message, and notify the process of dlog and slog
 * @param [in]session: hdc session
 * @param [in]req: request info from client
 * @return: HDCLOG_SUCCESSED: succeed; others: failed
 */
HdclogErr ParseDeviceLogCmd(HDC_SESSION session, const struct tlv_req *req)
{
    LogMsgT stMsg = { 0, "" };

    if ((req->len >= MSG_MAX_LEN) || (req->len <= 0)) {
        SELF_LOG_WARN("request length is illegal, request_length=%d.", req->len);
        (void)RespSettingResult(session, LEVEL_INFO_ERROR_MSG, strlen(LEVEL_INFO_ERROR_MSG));
        return HDCLOG_INIT_FAILED;
    }

    stMsg.msgType = FORWARD_MSG_TYPE;
    int res = memcpy_s(stMsg.msgData, MSG_MAX_LEN - 1, req->value, req->len);
    if (res != EOK) {
        SELF_LOG_WARN("memcpy_s request value failed, result=%d, strerr=%s.", res, strerror(ToolGetErrorCode()));
        (void)RespSettingResult(session, UNKNOWN_ERROR_MSG, strlen(UNKNOWN_ERROR_MSG));
        return HDCLOG_WRITE_FAILED;
    }

    SELF_LOG_INFO("request_value=%s, request_length=%d.", req->value, req->len);
    return DoParseDeviceLogCmd(session, &stMsg);
}

/*
 * @brief:judge if compute power group
 * @param [in]session: hdc session
 * @param [in]runEnv: run env value
 * @param [in]vfId: vfid value
 * @return: HDCLOG_SUCCESSED: true; others: false
 */
bool JudgeIfComputePowerGroup(HDC_SESSION session, int vfId)
{
    if (vfId != 0) {
        SELF_LOG_WARN("compute power group, vfId=%d.", vfId);
        (void)RespSettingResult(session, COMPUTE_POWER_GROUP, strlen(COMPUTE_POWER_GROUP));
        return true;
    }
    return false;
}

/*
 * @brief: process level setting cmd
 * @param [in]session: hdc session
 * @param [in]req: request info from client
 * @return: HDCLOG_SUCCESSED: succeed; others: failed
 */
int IdeDeviceLogProcess(HDC_SESSION session, const struct tlv_req *req)
{
    int vfId = 0;
    int ret = AdxGetVfIdBySession(session, &vfId);
    if (ret != IDE_DAEMON_OK) {
        SELF_LOG_WARN("ide get vfid by session failed, ret=%d.", ret);
        (void)RespSettingResult(session, UNKNOWN_ERROR_MSG, strlen(UNKNOWN_ERROR_MSG));
        return HDCLOG_IDE_GET_EVN_OR_VFID_FAILED;
    }

    if (JudgeIfComputePowerGroup(session, vfId)) {
        // compute power group: multiple hosts use one device, prohibit operating log level
        return HDCLOG_COMPUTE_POWER_GROUP;
    }

    if (session == NULL) {
        SELF_LOG_WARN("[input] session is null.");
        return HDCLOG_EMPTY_QUEUE;
    }

    HdclogErr res = HDCLOG_SUCCESSED;
    if (req == NULL) {
        SELF_LOG_WARN("[input] request is null.");
        (void)RespSettingResult(session, UNKNOWN_ERROR_MSG, strlen(UNKNOWN_ERROR_MSG));
        res = HDCLOG_EMPTY_QUEUE;
    } else {
        res = ParseDeviceLogCmd(session, req);
        SELF_LOG_INFO("set device level finished, result=%d.", res);
    }

    HdclogErr respRes = RespSettingResult(session, HDC_END_MSG, strlen(HDC_END_MSG));
    if (respRes != HDCLOG_SUCCESSED) {
        SELF_LOG_WARN("response end message to host failed, ret=%d.", respRes);
        return respRes;
    }

    return res;
}
