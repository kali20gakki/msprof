/**
 * @msg_queue.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef DLG_QUEUE_H
#define DLG_QUEUE_H
#include "log_sys_package.h"

#define MSG_MAX_LEN   1024
#define MSG_AUTHORITY 0600
#define MSG_RETRY_TIMES 10
#define GLOBAL_ENABLE_MAX_LEN 8
#define SINGLE_MODULE_MAX_LEN 24

enum {
    FORWARD_MSG_TYPE = 1,
    FEEDBACK_MSG_TYPE,
    GET_MSG_REQ_TYPE,
    GET_LEVEL_REP_TYPE,
    GET_LOGPATH_REP_TYPE
};

#ifdef __cplusplus
extern "C" {
#endif

typedef struct LogMsgT {
    long int msgType;
    char msgData[MSG_MAX_LEN];
} LogMsgT;

enum {
    MSG_QUEUE_KEY = 0x474f4c44
};

/*
 * data format:
 * |==== 1 byte ====|==== 4 bytes ====|====
 */
typedef struct DlogMsg {
    long int msgType;
    char msgData[MSG_MAX_LEN];
} DlogMsgT;

int OpenMsgQueue(toolMsgid *queueId);

int DeleteMsgQueue(const toolMsgid queueId);

void RemoveMsgQueue(void);

int SendQueueMsg(const toolMsgid queueId, const Buff *data, const int length, const int isWait);

int RecvQueueMsg(const toolMsgid queueId, Buff *recvData, const int bufLen, const int isWait, const long msgType);

#ifdef __cplusplus
}
#endif

#endif
