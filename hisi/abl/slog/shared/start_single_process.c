/**
 * @start_single_process.c
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "start_single_process.h"
#include <stdbool.h>
#include "dlog_error_code.h"
#include "log_common_util.h"
#include "print_log.h"

// File lock, Just start a process
int g_lockFd = -1;
#define FILE_MASK_WC 0640
#define HUNDRED_MILLI_SECOND 100
#define RETRY_TIMES 5

STATIC int LockReg(const LockRegParams *params)
{
#if (OS_TYPE_DEF == LINUX)
    if (params->fd <= 0) {
        return 0;
    }
    struct flock lock = { 0 };
    lock.l_type = params->type;
    lock.l_start = params->offset;
    lock.l_whence = params->whence;
    lock.l_len = params->len;
    return (fcntl(params->fd, params->cmd, &lock));
#else
    return 0;
#endif
}

int JustStartAProcess(const char *file)
{
    ONE_ACT_WARN_LOG(file == NULL, return ERR_NULL_DATA, "[input] file is null.");

    char buf[ARRAY_LENGTH] = "";
    g_lockFd = ToolOpenWithMode((char *)file, O_WRONLY | O_CREAT, FILE_MASK_WC);
    ONE_ACT_WARN_LOG(g_lockFd < 0, return ERR_NULL_DATA, "open file=%s failed, strerr=%s",
                     file, strerror(ToolGetErrorCode()));

#if (OS_TYPE_DEF == LINUX)
    LockRegParams params = { g_lockFd, F_SETLK, F_WRLCK, 0, SEEK_SET, 0 };
    int retry = 0;
    // perhaps slogd which is killed by cmd is still running, wait until it quit
    while (LockReg(&params) == -1) {
        if ((ToolGetErrorCode() == EAGAIN) && (retry < RETRY_TIMES)) {
            ToolSleep(HUNDRED_MILLI_SECOND);
            retry++;
        } else {
            SELF_LOG_WARN("fcntl file failed, file=%s, strerr=%s.", file, strerror(ToolGetErrorCode()));
            return ERR_PROCESS_STARED;
        }
    }
#endif

    do {
        int ret = ToolFtruncate((toolProcess)g_lockFd, 0);
        ONE_ACT_ERR_LOG(ret == -1, break, "reset file size to zero failed, file=%s, strerr=%s.",
                        file, strerror(ToolGetErrorCode()));

        ret = sprintf_s(buf, sizeof(buf), "%d\n", ToolGetPid());
        ONE_ACT_ERR_LOG(ret == -1, break, "sprintf_s process id failed, result=%d, strerr=%s.",
                        ret, strerror(ToolGetErrorCode()));

        ret = ToolWrite(g_lockFd, buf, strlen(buf));
        ONE_ACT_ERR_LOG((ret < 0) || ((unsigned)ret != strlen(buf)), break, "write buffer to file failed, file=%s,"
                        " strerr=%s.", file, strerror(ToolGetErrorCode()));

#if (OS_TYPE_DEF == LINUX)
        ret = fcntl(g_lockFd, F_GETFD, 0);
        ONE_ACT_ERR_LOG(ret == -1, break, "fcntl file failed, file=%s, strerr=%s.",
                        file, strerror(ToolGetErrorCode()));
        unsigned int res = (unsigned int)ret;
        res |= FD_CLOEXEC;
        ret = fcntl(g_lockFd, F_SETFD, res);
        ONE_ACT_ERR_LOG(ret == -1, break, "fcntl file failed, file=%s, strerr=%s.",
                        file, strerror(ToolGetErrorCode()));
#endif
        return ERR_OK;
    } while (0);

    SingleResourceCleanup(file);
    return ERR_NULL_DATA;
}

// File lock, end
void SingleResourceCleanup(const char *file)
{
    ONE_ACT_NO_LOG((file == NULL) || (strlen(file) == 0), return);

    if (ToolUnlink(file) != 0) {
        SELF_LOG_WARN("unlink file failed, file=%s, strerr=%s.", file, strerror(ToolGetErrorCode()));
    }
    LOG_MMCLOSE_AND_SET_INVALID(g_lockFd);
}
