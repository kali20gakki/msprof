/**
 * @log_queuec.c
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include <string.h>
#include "log_common_util.h"
#include "print_log.h"
#include "log_queue.h"


void XFreeLogNode(LogNode **node)
{
    ONE_ACT_WARN_LOG(node == NULL, return, "[input] log node pointer is null.");
    ONE_ACT_WARN_LOG(*node == NULL, return, "[input] log node is null.");

    LogNode *tmp = (LogNode *)*node;
    XFREE(tmp->stNodeData);
    XFREE(tmp);
    *node = NULL;
}

LogRt LogQueueInit(LogQueue *queue, int deviceId)
{
    ONE_ACT_WARN_LOG(queue == NULL, return ARGV_NULL, "[input] queue is null.");
    ONE_ACT_WARN_LOG(deviceId < 0, return ARGV_NULL, "[input] device id is invalid, device_id=%d.", deviceId);

    queue->deviceId = deviceId;
    queue->uiCount = 0;
    queue->uiSize = 0;
    queue->stHead = NULL;
    queue->stRear = NULL;
    return SUCCESS;
}

// not free log_queue
LogRt LogQueueFree(LogQueue *queue, void (*freeNode)(LogNode**))
{
    int i;
    LogNode *tmp = NULL;

    ONE_ACT_WARN_LOG(queue == NULL, return ARGV_NULL, "[input] queue is null.");
    ONE_ACT_WARN_LOG(freeNode == NULL, return ARGV_NULL, "[input] node free function is null");

    int num = queue->uiCount;
    for (i = 0; i < num; i++) {
        LogRt ret = LogQueueDequeue(queue, &tmp);
        if (ret == QUEUE_IS_NULL) {
            break;
        }
        freeNode(&tmp);
    }

    return SUCCESS;
}

LogRt LogQueueEnqueue(LogQueue *queue, LogNode *node)
{
    ONE_ACT_WARN_LOG(queue == NULL, return ARGV_NULL, "[input] queue is null.");
    ONE_ACT_WARN_LOG(node  == NULL, return ARGV_NULL, "[input] log node is null.");
    ONE_ACT_WARN_LOG(node->uiNodeDataLen == 0, return ARGV_NULL, "[input] log node is empty.");

    // queue would be more than max_size, but node max_count
    if (LogQueueFull(queue) != SUCCESS) {
        SELF_LOG_INFO("queue is full.");
        return QUEUE_IS_FULL;
    }

    if (LogQueueNULL(queue) != SUCCESS) {
        queue->stHead = node;
        queue->stRear = node;
        queue->uiCount = 1;
        queue->uiSize = node->uiNodeDataLen;
    } else {
        queue->stRear->next = node; // firstly, join the node to list
        queue->stRear = node; // secondly, mv stRear to listRear
        queue->uiCount++;
        queue->uiSize += node->uiNodeDataLen;
    }

    return SUCCESS;
}

LogRt LogQueueDequeue(LogQueue *queue, LogNode **node)
{
    LogNode *tmp = NULL;

    ONE_ACT_WARN_LOG(queue == NULL, return ARGV_NULL, "[input] queue is null.");
    ONE_ACT_WARN_LOG(node  == NULL, return ARGV_NULL, "[input] log node is null.");

    if (LogQueueNULL(queue) != SUCCESS) {
        return QUEUE_IS_NULL;
    }

    *node = queue->stHead; // get head
    if (queue->uiCount == 1) {
        LogQueueInit(queue, queue->deviceId);
    } else {
        tmp = queue->stHead;
        queue->uiCount--;
        queue->uiSize -= tmp->uiNodeDataLen;
        queue->stHead = tmp->next;
        tmp->next = NULL;
    }

    return SUCCESS;
}

LogRt LogQueueFull(const LogQueue *queue)
{
    ONE_ACT_WARN_LOG(queue == NULL, return ARGV_NULL, "[input] queue is null.");

    if ((queue->uiCount >= MAX_QUEUE_COUNT) ||
        (queue->uiSize >= MAX_QUEUE_SIZE)) {
        return QUEUE_IS_FULL;
    }

    return SUCCESS;
}

LogRt LogQueueNULL(const LogQueue *queue)
{
    ONE_ACT_WARN_LOG(queue == NULL, return ARGV_NULL, "[input] queue is null.");

    if ((queue->uiCount == 0) || (queue->uiSize == 0) ||
        ((queue->stHead == NULL) && (queue->stRear == NULL))) {
        return QUEUE_IS_NULL;
    }

    return SUCCESS;
}
