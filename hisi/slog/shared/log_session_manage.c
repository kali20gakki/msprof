/**
 * @log_session_manage.c
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "log_session_manage.h"
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include "securec.h"
#include "log_common_util.h"
#ifdef APP_LOG_REPORT
#include "log_drv.h"
#endif

// global zone
SessionNode *g_sessionPidDevIdList = NULL;
SessionNode *g_sessionPidDevIdDeletedList = NULL;
STATIC toolMutex g_sessionMutex;
STATIC int g_sessionMutexInit = 0;

LogRt InitSessionList(void)
{
    if (g_sessionMutexInit == 0) {
        if (ToolMutexInit(&g_sessionMutex) == SYS_OK) {
            g_sessionMutexInit = 1;
        } else {
            SELF_LOG_WARN("init mutex failed, strerr=%s.", strerror(ToolGetErrorCode()));
            return MUTEX_INIT_ERR;
        }
    }
    return SUCCESS;
}

void FreeSessionList(void)
{
    LOCK_WARN_LOG(&g_sessionMutex);
    SessionNode *tmp = g_sessionPidDevIdList;
    SessionNode *node = NULL;
    while (tmp != NULL) {
        node = tmp;
        tmp = tmp->next;
        XFREE(node);
    }
    tmp = g_sessionPidDevIdDeletedList;
    while (tmp != NULL) {
        node = tmp;
        tmp = tmp->next;
        XFREE(node);
    }
    g_sessionPidDevIdList = NULL;
    g_sessionPidDevIdDeletedList = NULL;
    UNLOCK_WARN_LOG(&g_sessionMutex);
    (void)ToolMutexDestroy(&g_sessionMutex);
    g_sessionMutexInit = 0;
    return;
}

void PushDeletedSessionNode(SessionNode *node)
{
    ONE_ACT_NO_LOG(node == NULL, return);

    LOCK_WARN_LOG(&g_sessionMutex);
    if (g_sessionPidDevIdDeletedList == NULL) {
        g_sessionPidDevIdDeletedList = node;
        UNLOCK_WARN_LOG(&g_sessionMutex);
        return;
    }
    if (node->next != NULL) {
        // if node is list, insert to global list tail
        SessionNode *tmp = g_sessionPidDevIdDeletedList;
        while (tmp->next != NULL) {
            tmp = tmp->next;
        }
        tmp->next = node;
    } else {
        // if node is single, insert to global list head
        node->next = g_sessionPidDevIdDeletedList;
        g_sessionPidDevIdDeletedList = node;
    }
    UNLOCK_WARN_LOG(&g_sessionMutex);
}

SessionNode* PopDeletedSessionNode(void)
{
    LOCK_WARN_LOG(&g_sessionMutex);
    if (g_sessionPidDevIdDeletedList == NULL) {
        UNLOCK_WARN_LOG(&g_sessionMutex);
        return NULL;
    }
    SessionNode *tmp = g_sessionPidDevIdDeletedList;
    if (tmp != NULL) {
        g_sessionPidDevIdDeletedList = NULL;
    }
    UNLOCK_WARN_LOG(&g_sessionMutex);
    return tmp;
}

LogRt DeleteSessionNode(uintptr_t session, int pid, int devId)
{
    ONE_ACT_ERR_LOG(devId > MAX_DEV_NUM, return ARGV_NULL, "invalid device id for deletion: %d", devId);
    LOCK_WARN_LOG(&g_sessionMutex);
    if (g_sessionPidDevIdList == NULL) {
        UNLOCK_WARN_LOG(&g_sessionMutex);
        return ARGV_NULL;
    }
    SessionNode *deletedNode = NULL;
    SessionNode *tmp = g_sessionPidDevIdList;
    if ((tmp->pid == pid) && (tmp->devId == devId) && (tmp->session == session)) {
        g_sessionPidDevIdList = tmp->next;
        tmp->next = NULL;
        deletedNode = tmp;
    } else {
        while ((tmp->next != NULL) && ((tmp->next->pid != pid) ||
            (tmp->next->devId != devId) || (tmp->next->session != session))) {
            tmp = tmp->next;
        }
        if ((tmp->next != NULL) && (tmp->next->pid == pid) &&
            (tmp->next->devId == devId) && (tmp->next->session == session)) {
            deletedNode = tmp->next;
            tmp->next = deletedNode->next;
            deletedNode->next = NULL;
        }
    }
    UNLOCK_WARN_LOG(&g_sessionMutex);
    PushDeletedSessionNode(deletedNode);
    return SUCCESS;
}

#ifdef APP_LOG_REPORT
void HandleInvalidSessionNode(void)
{
    int ret;
    int value;
    SessionNode *tmp = NULL;
    LOCK_WARN_LOG(&g_sessionMutex);
    SessionNode *node = g_sessionPidDevIdList;
    // get the first valid session node
    while (node != NULL) {
        if (DrvDevIdGetBySession((HDC_SESSION)node->session, HDC_SESSION_ATTR_VFID, &value) != 0) {
            g_sessionPidDevIdList = node->next;
            ret = DrvSessionRelease((HDC_SESSION)node->session);
            SELF_LOG_INFO("release session finished, ret=%d, pid=%d", ret, node->pid);
            tmp = node;
            node = node->next;
            XFREE(tmp);
        } else {
            break;
        }
    }

    // delete invalid session nodes
    while ((node != NULL) && (node->next != NULL)) {
        if (DrvDevIdGetBySession((HDC_SESSION)node->next->session, HDC_SESSION_ATTR_VFID, &value) != 0) {
            ret = DrvSessionRelease((HDC_SESSION)node->next->session);
            SELF_LOG_INFO("release session finished, ret=%d, pid=%d", ret, node->next->pid);
            tmp = node->next;
            node->next = node->next->next;
            XFREE(tmp);
        } else {
            node = node->next;
        }
    }
    UNLOCK_WARN_LOG(&g_sessionMutex);
    return;
}
#endif

LogRt InsertSessionNode(uintptr_t session, int pid, int devId)
{
    ONE_ACT_ERR_LOG(devId > MAX_DEV_NUM, return ARGV_NULL, "invalid device id for insertion: %d", devId);
    SessionNode *sessionNode = (SessionNode *)malloc(sizeof(SessionNode));
    if (sessionNode == NULL) {
        SELF_LOG_WARN("malloc failed, strerr=%s.", strerror(ToolGetErrorCode()));
        return MALLOC_FAILED;
    }
    (void)memset_s(sessionNode, sizeof(SessionNode), 0, sizeof(SessionNode));
    sessionNode->pid = pid;
    sessionNode->devId = devId;
    sessionNode->session = session;
    LOCK_WARN_LOG(&g_sessionMutex);
    sessionNode->next = g_sessionPidDevIdList;
    g_sessionPidDevIdList = sessionNode;
    UNLOCK_WARN_LOG(&g_sessionMutex);
    return SUCCESS;
}

static SessionNode* GetSessionNodeByList(int pid, int devId, SessionNode *list)
{
    ONE_ACT_NO_LOG(list == NULL, return NULL);

    SessionNode *tmp = list;
    while (tmp != NULL) {
        if ((tmp->pid == pid) && (tmp->devId == devId)) {
            return tmp;
        }
        tmp = tmp->next;
    }
    return NULL;
}

SessionNode* GetSessionNode(int pid, int devId)
{
    ONE_ACT_ERR_LOG(devId > MAX_DEV_NUM, return NULL, "invalid device id for node searching: %d", devId);

    LOCK_WARN_LOG(&g_sessionMutex);
    SessionNode *tmp = GetSessionNodeByList(pid, devId, g_sessionPidDevIdList);
    UNLOCK_WARN_LOG(&g_sessionMutex);
    return tmp;
}

SessionNode* GetDeletedSessionNode(int pid, int devId)
{
    ONE_ACT_ERR_LOG(devId > MAX_DEV_NUM, return NULL, "invalid device id for node searching: %d", devId);

    LOCK_WARN_LOG(&g_sessionMutex);
    SessionNode *tmp = GetSessionNodeByList(pid, devId, g_sessionPidDevIdDeletedList);
    UNLOCK_WARN_LOG(&g_sessionMutex);
    return tmp;
}

