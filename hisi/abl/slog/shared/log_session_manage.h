/**
 * @log_session_manage.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef LOG_SESSION_MANAGE_H
#define LOG_SESSION_MANAGE_H

#include "dlog_error_code.h"
#include "log_sys_package.h"
#include "print_log.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SessionPidDevIdNode {
    struct SessionPidDevIdNode *next;
    uintptr_t session;
    int pid;
    int devId;
    int timeout;
} SessionNode;

LogRt InitSessionList(void);
void FreeSessionList(void);
LogRt DeleteSessionNode(uintptr_t session, int pid, int devId);
LogRt InsertSessionNode(uintptr_t session, int pid, int devId);
SessionNode* GetSessionNode(int pid, int devId);
void PushDeletedSessionNode(SessionNode *node);
SessionNode* PopDeletedSessionNode(void);
SessionNode* GetDeletedSessionNode(int pid, int devId);
#ifdef APP_LOG_REPORT
void HandleInvalidSessionNode(void);
#endif
#ifdef __cplusplus
}
#endif
#endif