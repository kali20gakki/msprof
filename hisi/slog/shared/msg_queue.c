/**
 * @msg_queue.c
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "msg_queue.h"
#include "dlog_error_code.h"
#include "print_log.h"

/**
 * @brief OpenMsgQueue: open message queue to communicate with another process
 * @param [in/out]queueId: message queue id
 * @return: ERR_OK: ok, others: error
 */
int OpenMsgQueue(toolMsgid *queueId)
{
    ONE_ACT_NO_LOG(queueId == NULL, return ERR_NULL_PTR);
    toolMsgid msgId = ToolMsgOpen(MSG_QUEUE_KEY, MSG_AUTHORITY | M_MSG_CREAT);
    if (msgId == SYS_ERROR) {
        return ERR_CREATE_MSG_QUEUE_FAIL;
    }
    *queueId = msgId;
    return ERR_OK;
}

/**
 * @brief DeleteMsgQueue: delete message queue
 * @param [in]queueId: queue id to delete
 * @return: ERR_OK:ok, others: error
 */
int DeleteMsgQueue(const toolMsgid queueId)
{
    ONE_ACT_NO_LOG(queueId < 0, return ERR_INVALID_QUEUE_ID);

    ONE_ACT_NO_LOG(ToolMsgClose(queueId) != SYS_OK, return ERR_DELETE_MSG_QUEUE_FAIL);

    return ERR_OK;
}

/**
 * @brief RemoveMsgQueue: remove message queue
 * @return: void
 */
void RemoveMsgQueue(void)
{
    toolMsgid msgId = ToolMsgOpen(MSG_QUEUE_KEY, 0);
    if (msgId < 0) {
        SELF_LOG_WARN("open msg_queue failed, maybe not exist.");
        return;
    }

    int ret = DeleteMsgQueue(msgId);
    if (ret != ERR_OK) {
        SELF_LOG_WARN("remove msg queue failed.");
    }
}

/**
 * @brief SendQueueMsg: write data to message queue
 * @param [in]queueId: message queue id
 * @param [in]data: DlogMsgT type data to send
 * @param [in]length: data length(byte), exclude long int part
 * @param [in]isWait: whether wait when queue is full
 * @return: ERR_OK:ok, others:error
 */
int SendQueueMsg(const toolMsgid queueId, const Buff *data, const int length, const int isWait)
{
    ONE_ACT_NO_LOG(queueId < 0, return ERR_INVALID_QUEUE_ID);
    ONE_ACT_NO_LOG(data == NULL, return ERR_NULL_DATA);
    ONE_ACT_NO_LOG(length <= 0, return ERR_NULL_DATA);
    ONE_ACT_NO_LOG(isWait < 0, return ERR_INVALID_WAIT_FLAG);

    int msgFlag = (isWait == 0) ? 0 : M_MSG_NOWAIT;
    if (ToolMsgSnd(queueId, (void *)data, length, msgFlag) != 0) {
        return ERR_SEND_MSG_FAIL;
    }
    return ERR_OK;
}

/**
 * @brief RecvQueueMsg: receive message data from a queue
 * @param [in]queueId: message queue id
 * @param [in]recvData: a buffer to store data received from queue
 * @param [in]bufLen: length of data part(byte), exclude long int msgType
 * @param [in]isWait: whether suspend when call recv api here
 * @param [in]msgType: message type
 * return: ERR_OK: ok, others: error
 */
int RecvQueueMsg(const toolMsgid queueId, Buff *recvData, const int bufLen, const int isWait, const long msgType)
{
    ONE_ACT_NO_LOG(queueId < 0, return ERR_INVALID_QUEUE_ID);
    ONE_ACT_NO_LOG(recvData == NULL, return ERR_NULL_DATA);
    ONE_ACT_NO_LOG(bufLen <= 0, return ERR_NULL_DATA);
    ONE_ACT_NO_LOG(isWait < 0, return ERR_INVALID_WAIT_FLAG);

    int msgFlag = (isWait == 0) ? 0 : M_MSG_NOWAIT;
    if (ToolMsgRcv(queueId, (void *)recvData, bufLen, msgFlag, msgType) == SYS_ERROR) {
        return ERR_RECV_MSG_FAIL;
    }
    return ERR_OK;
}
