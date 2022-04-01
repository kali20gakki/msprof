/**
* @slog_common.h
*
* Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "slog_common.h"
#include "print_log.h"

extern char *__progname;

#if (OS_TYPE_DEF == 0)
long TimeDiff(const struct timespec *lastTv)
{
    struct timespec currentTv = { 0, 0 };
    if (lastTv == NULL) {
        return 0;
    }

    int result = clock_gettime(CLOCK_INPUT, &currentTv);
    ONE_ACT_WARN_LOG(result != 0, return SYS_ERROR, "[input] buffer is NULL.");
    long timeValue = (currentTv.tv_nsec - lastTv->tv_nsec) / 1000000; // 1 millisecond = 1000000 nanosecond
    timeValue += (currentTv.tv_sec - lastTv->tv_sec) * 1000; // 1000 millisecond = 1 second
    return (timeValue > 0) ? timeValue : 0;
}

int GetGlobalLevel(char levelChar)
{
    ONE_ACT_NO_LOG(levelChar < 0, return GLOABLE_DEFAULT_LOG_LEVEL);

    int level = (int)((((unsigned char)levelChar) >> 4) & 0x7) - 1; // high 6~4bit, & 0x7
    if ((level < LOG_MIN_LEVEL) || (level > LOG_MAX_LEVEL)) {
        level = GLOABLE_DEFAULT_LOG_LEVEL;
    }
    return level;
}

int GetEventLevel(char levelChar)
{
    ONE_ACT_NO_LOG(levelChar < 0, return EVENT_ENABLE_VALUE);

    int level = ((((unsigned char)levelChar) >> 2) & 0x3) - 1; // low 3~2bit, & 0x3
    if ((level != EVENT_ENABLE_VALUE) && (level != EVENT_DISABLE_VALUE)) {
        level = EVENT_ENABLE_VALUE;
    }
    return level;
}

/**
 * @brief ReadStrFromShm: read string from shmem
 * @param [in]shmId: shmem identifier ID
 * @param [in/out]str: string which read from shmem.
 *      The arg '*str' only malloc in the method, no need to malloc in caller.
 * @param [in]strLen: string max length
 * @param [in]offset: read offset in shmem
 * @return: SYS_ERROR/SYS_OK
 */
int ReadStrFromShm(int shmId, char **str, unsigned int strLen, unsigned int offset)
{
    ONE_ACT_NO_LOG(shmId < 0, return SYS_ERROR);
    ONE_ACT_NO_LOG(str == NULL, return SYS_ERROR);
    ONE_ACT_NO_LOG(((strLen == 0) || (strLen > SHM_SIZE)), return SYS_ERROR);
    ONE_ACT_NO_LOG(((offset > SHM_SIZE) || ((offset + strLen) > SHM_SIZE)), return SYS_ERROR);

    char *tmpBuf = (char *)malloc(strLen + 1);
    if (tmpBuf == NULL) {
        SELF_LOG_WARN("malloc failed, pid=%d, strerr=%s.", ToolGetPid(), strerror(ToolGetErrorCode()));
        return SYS_ERROR;
    }
    (void)memset_s(tmpBuf, strLen + 1, 0, strLen + 1);

    ShmErr res;

    // read module arr
    res = ReadFromShMem(shmId, tmpBuf, strLen, offset);
    if (res == SHM_ERROR) {
        SELF_LOG_WARN("read from shmem failed, pid=%d, strerr=%s.", ToolGetPid(), strerror(ToolGetErrorCode()));
        XFREE(tmpBuf);
        return SYS_ERROR;
    }
    *str = tmpBuf;
    return SYS_OK;
}

/**
 * @brief AddNewWatch: check file access and add new watch
 * @param [in/out]pNotifyFd: pointer to nofity fd
 * @param [in/out]pWatchFd: pointer to watched fd
 * @param [in]notifyFile: notify file, default is "/usr/slog/level_notify"
 * @return: LogRt, SUCCESS/NOTIFY_WATCH_FAILED/ARGV_NULL/NOTIFY_INIT_FAILED
 */
LogRt AddNewWatch(int *pNotifyFd, int *pWatchFd, const char *notifyFile)
{
    ONE_ACT_NO_LOG(pNotifyFd == NULL, return ARGV_NULL);
    ONE_ACT_NO_LOG(pWatchFd == NULL, return ARGV_NULL);
    ONE_ACT_NO_LOG(notifyFile == NULL, return ARGV_NULL);

    int printNum = 0;
    while (ToolAccess(notifyFile) != SYS_OK) {
        SELF_LOG_WARN_N(&printNum, CONN_W_PRINT_NUM,
                        "access notify file failed, file=%s, pid=%d, print once every %d times.",
                        notifyFile, ToolGetPid(), CONN_W_PRINT_NUM);
        (void)ToolSleep(1000); // wait file created, sleep 1000ms
    }

    if ((*pNotifyFd != INVALID) && (*pWatchFd != INVALID)) {
        int res = inotify_rm_watch(*pNotifyFd, *pWatchFd);
        NO_ACT_WARN_LOG(res != 0, "remove inotify failed but continue, res=%d, strerr=%s, pid=%d.",
                        res, strerror(ToolGetErrorCode()), ToolGetPid());
        LOG_MMCLOSE_AND_SET_INVALID(*pNotifyFd);
    }
    *pNotifyFd = INVALID;
    *pWatchFd = INVALID;

    // init notify
    int notifyFd = inotify_init();
    ONE_ACT_ERR_LOG(notifyFd == INVALID, return NOTIFY_INIT_FAILED,
                    "init inotify failed, res=%d, strerr=%s, pid=%d.",
                    notifyFd, strerror(ToolGetErrorCode()), ToolGetPid());
    // add new watcher
    int watchFd = inotify_add_watch(notifyFd, notifyFile, IN_DELETE_SELF | IN_CLOSE_WRITE);
    TWO_ACT_ERR_LOG(watchFd < 0, LOG_MMCLOSE_AND_SET_INVALID(notifyFd), return NOTIFY_WATCH_FAILED,
                    "add file watcher failed, res=%d, strerr=%s, pid=%d.",
                    watchFd, strerror(ToolGetErrorCode()), ToolGetPid());
    *pNotifyFd = notifyFd;
    *pWatchFd = watchFd;
    return SUCCESS;
}
#endif
