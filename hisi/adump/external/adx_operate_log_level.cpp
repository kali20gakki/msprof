/**
* @file adx_operate_log_level.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "adx_operate_log_level.h"
namespace Adx {
#if (OS_TYPE == LINUX)
/*
 * @brief: sleep
 * @param [in]milliSecond: sleep time, unit: ms, linux support max 4294967 ms
 * @return SYS_OK: succeed; SYS_ERROR: failed; SYS_INVALID_PARAM: invalid param;
 */
int ToolSleep(UINT32 milliSecond)
{
    if (milliSecond == 0) {
        return IDE_DAEMON_ERROR;
    }

    UINT32 microSecond;
    if (milliSecond <= TOOL_MAX_SLEEP_MILLSECOND) {
        microSecond = milliSecond * TOOL_ONE_THOUSAND;
    } else {
        microSecond = 0xffffffff;
    }

    INT32 ret = usleep(microSecond);
    if (ret != IDE_DAEMON_OK) {
        return IDE_DAEMON_ERROR;
    }
    return IDE_DAEMON_OK;
}
#else
int ToolSleep(UINT32 milliSecond)
{
    return IDE_DAEMON_OK;
}
#endif /* OS_TYPE_DEF */

int ReadSettingResult(HDC_SESSION session, IdeStringBuffer const resultBuf)
{
    if ((session == NULL) || (resultBuf == NULL)) {
        return IDE_DAEMON_ERROR;
    }

    IdeStringBuffer buf = NULL;
    unsigned int recvLen = 0;
    int err = IDE_DAEMON_ERROR;
    do {
        // read level setting result from device
        err = HdcRead(session, (IdeRecvBuffT)&buf, (int *)&recvLen);
        if (err == IDE_DAEMON_OK) {
            if (buf == NULL) {
                (void)ToolSleep(1); // sleep 1ms
                continue;
            }

            if ((recvLen == strlen(HDC_END_MSG)) && (strncmp(buf, HDC_END_MSG, recvLen) == 0)) {
                break;
            }

            int strcpyRes = strncpy_s(resultBuf, MSG_MAX_LEN, buf, recvLen);
            if (strcpyRes != EOK) {
                char errBuf[MAX_ERRSTR_LEN + 1] = {0};
                IDE_LOGE("strncpy_s device level setting result failed, result=%d, strerr=%s.",
                    strcpyRes, mmGetErrorFormatMessage(mmGetErrorCode(), errBuf, MAX_ERRSTR_LEN));
                err = IDE_DAEMON_ERROR;
                break;
            }
        }
    } while (err == IDE_DAEMON_OK);

    XFREE(buf);
    return err;
}

/*
 * @brief: set or get device level
 * @param [in]client: hdc client
 * @param [in]req: request info from client
 * @param [in]logLevelResult: log level result
 * @param [in]logOperatonType: log operaton type
 * @return: IDE_DAEMON_OK: succeed; others: failed
 */
int OperateDeviceLevel(const HDC_CLIENT client, const struct tlv_req *req, \
                       IdeStringBuffer logLevelResult, int logLevelResultLength, int logOperatonType)
{
    HDC_SESSION session = NULL;
    // connect to device
    int err = HdcSessionConnect(0, req->dev_id, client, &session);
    if (err != IDE_DAEMON_OK) {
        IDE_LOGW("connect to device failed, result=%d.", err);
        return err;
    }

    // send level setting info to device
    err = HdcWrite((HDC_SESSION)session, req, sizeof(struct tlv_req) + req->len);
    if (err != IDE_DAEMON_OK) {
        IDE_LOGW("write level info to device failed by hdc, result=%d.", err);
        (void)HdcSessionDestroy(session);
        return err;
    }

    IdeStringBuffer resultBuf = (IdeStringBuffer)malloc(MSG_MAX_LEN);
    if (resultBuf == NULL) {
        char errBuf[MAX_ERRSTR_LEN + 1] = {0};
        IDE_LOGE("malloc failed, strerr=%s.", mmGetErrorFormatMessage(mmGetErrorCode(), errBuf, MAX_ERRSTR_LEN));
        (void)HdcSessionDestroy(session);
        return IDE_DAEMON_ERROR;
    }
    (void)memset_s(resultBuf, MSG_MAX_LEN, 0, MSG_MAX_LEN);
    err = ReadSettingResult(session, (IdeStringBuffer const)resultBuf);
    if (err == IDE_DAEMON_ERROR) {
        IDE_LOGW("receive device level setting result failed.");
    } else if (strcmp(COMPUTE_POWER_GROUP, resultBuf) == 0) {
        err = ADX_COMPUTE_POWER_GROUP;
    } else if (logOperatonType == GETLOGLEVEL) {
        int strcpyRes = strncpy_s(logLevelResult, logLevelResultLength, resultBuf, MSG_MAX_LEN);
        if (strcpyRes != EOK) {
            err = IDE_DAEMON_ERROR;
            char errBuf[MAX_ERRSTR_LEN + 1] = {0};
            IDE_LOGE("strncpy_s device level result failed, result=%d, strerr=%s.",
                     strcpyRes, mmGetErrorFormatMessage(mmGetErrorCode(), errBuf, MAX_ERRSTR_LEN));
        } else {
            err = IDE_DAEMON_OK;
        }
    } else {
        int strcmpRes = strcmp(LEVEL_SETTING_SUCCESS, resultBuf);
        if (strcmpRes == 0) {
            err = IDE_DAEMON_OK;
        } else {
            err = IDE_DAEMON_ERROR;
            IDE_LOGE("receive device level setting result_info=%s.", resultBuf);
        }
    }

    (void)HdcSessionDestroy(session);
    XFREE(resultBuf);
    return err;
}

/**
 * @brief Create a data frame to send tlv
 * @param dev_id: device id
 * @param value:the value to send
 * @param value_len: the length of value
 * @param buf: the data frame to send tlv
 * @param buf_len: the data length of frame
 *
 * @return
 *        IDE_DAEMON_OK: succ
 *        IDE_DAEMON_ERROR: failed
 */
int CreatePacket(int devId, IdeString value, uint32_t valueLen, IdeRecvBuffT buf, IdeI32Pt bufLen)
{
    if (value == nullptr || buf == nullptr || bufLen == nullptr) {
        IDE_LOGE("input invalid parameter");
        return IDE_DAEMON_ERROR;
    }

    if ((uint64_t)valueLen + sizeof(struct tlv_req) + 1 > UINT32_MAX) {
        IDE_LOGE("bigger than UINT32_MAX, value_len: %u, tlv_len: %lu", valueLen, sizeof(struct tlv_req));
        return IDE_DAEMON_ERROR;
    }

    uint32_t mallocValueLen = valueLen + 1;
    uint32_t sendLen = sizeof(struct tlv_req) + mallocValueLen;
    IdeStringBuffer sendBuf = static_cast<IdeStringBuffer>(IdeXmalloc(sendLen));
    IDE_CTRL_VALUE_FAILED(sendBuf != nullptr, return IDE_DAEMON_ERROR, "malloc memory failed");
    IdeTlvReq req = (IdeTlvReq)sendBuf;
    req->type = IDE_LOG_REQ;
    req->dev_id = devId;
    req->len = valueLen;
    errno_t err = memcpy_s(req->value, mallocValueLen, value, valueLen);
    if (err != EOK) {
        IDE_LOGE("memory copy failed, err: %d", err);
        IDE_XFREE_AND_SET_NULL(req);
        return IDE_DAEMON_ERROR;
    }

    *buf = static_cast<IdeMemHandle>(sendBuf);
    *bufLen = sizeof(struct tlv_req) + valueLen;

    return IDE_DAEMON_OK;
}
}