/**
 * @log_queue.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef LOG_QUEUE_H
#define LOG_QUEUE_H

#include "dlog_error_code.h"

#define MAX_QUEUE_COUNT 8
#define MAX_QUEUE_SIZE (8 * 1024 * 1024)

#ifdef __cplusplus
extern "C" {
#endif

typedef struct node {
    unsigned int uiNodeDataLen;
    unsigned int uiNodeNum;
    void *stNodeData;
    struct node *next;
    short slogFlag;
    short smpFlag;
} LogNode;

typedef struct {
    unsigned int uiNodeNum;
    void *stNodeData;
} NodesInfo;

typedef struct {
    unsigned int deviceId;
    unsigned int uiCount;
    unsigned int uiSize;
    LogNode *stHead; // void struct should have uiSize
    LogNode *stRear;
} LogQueue;

// queue init
LogRt LogQueueInit(LogQueue *queue, int deviceId);

// queue free
LogRt LogQueueFree(LogQueue *queue, void (*freeNode)(LogNode **));

// node enqueue
LogRt LogQueueEnqueue(LogQueue *queue, LogNode *node);

// node dequeue
LogRt LogQueueDequeue(LogQueue *queue, LogNode **node);

// check whether queue is full
LogRt LogQueueFull(const LogQueue *queue);

// check whether queue is NULL
LogRt LogQueueNULL(const LogQueue *queue);

void XFreeLogNode(LogNode **node);

#ifdef __cplusplus
}
#endif

#endif  // __LOG_BUF_Q_H__
