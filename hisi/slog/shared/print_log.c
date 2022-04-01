/**
 * @print_log.c
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#include "print_log.h"
#if (OS_TYPE_DEF == 0)
#include <sys/file.h>
#endif
#include "securec.h"

#define FILE_MASK_WC 0640
#define UNIT_THOUSAND 1000

typedef struct {
    char selfLogFile[SELF_LOG_FILES_LENGTH + 1];
    char selfLogOldFile[SELF_LOG_FILES_LENGTH + 1];
    char selflogLockFile[SELF_LOG_FILES_LENGTH + 1];
} SelfLogFiles;

typedef struct {
    char *buff;
    unsigned int buffLen;
} Buffer;

STATIC void GetLocalTimeForSelfLog(unsigned int bufLen, char *timeBuffer);
STATIC int GetRingFd(const char *slogdFile, const char *msg);
STATIC void SetRingFile(const char *slogdFile);
STATIC int CatStr(const char *str1, unsigned int len1, const char *str2, unsigned int len2, Buffer *buffer);

SelfLogFiles *g_selfLogFiles = NULL;

/**
 * @brief InitFilePathForSelfLog: init log dir from slog.conf "logAgentdir" to store slef log
 * @return:SYS_OK:succeed;SYS_ERROR:failed;
 */
int InitFilePathForSelfLog(void)
{
    if (LogInitLogAgentPath() != SYS_OK) {
        return SYS_ERROR;
    }
    // g_selfLogFiles has already been initialize."
    if (g_selfLogFiles != NULL) {
        return SYS_OK;
    }

    g_selfLogFiles = (SelfLogFiles *)malloc(sizeof(SelfLogFiles));
    if (g_selfLogFiles == NULL) {
        return SYS_ERROR;
    }
    (void)memset_s(g_selfLogFiles, sizeof(SelfLogFiles), 0, sizeof(SelfLogFiles));

    // init selfLogFiles
    int ret1 = StrcatDir(g_selfLogFiles->selfLogFile, SLOGD_LOG_FILE, GetLogAgentPath(), SELF_LOG_FILES_LENGTH);
    int ret2 = StrcatDir(g_selfLogFiles->selfLogOldFile, SLOGD_LOG_OLD_FILE, GetLogAgentPath(), SELF_LOG_FILES_LENGTH);
    int ret3 = StrcatDir(g_selfLogFiles->selflogLockFile, SLOGD_LOG_LOCK, GetLogAgentPath(), SELF_LOG_FILES_LENGTH);
    if ((ret1 != SYS_OK) || (ret2 != SYS_OK) || (ret3 != SYS_OK)) {
        XFREE(g_selfLogFiles);
        return SYS_ERROR;
    }
    return SYS_OK;
}

char *GetSelfLogPath(void)
{
    if (g_selfLogFiles != NULL) {
        return g_selfLogFiles->selfLogFile;
    }
    return DEFAULT_SLOGD_LOG_FILE;
}

char *GetSelfLogOldPath(void)
{
    if (g_selfLogFiles != NULL) {
        return g_selfLogFiles->selfLogOldFile;
    }
    return DEFAULT_SLOGD_LOG_OLD_FILE;
}

STATIC char *GetSelfLogLockPath(void)
{
    if (g_selfLogFiles != NULL) {
        return g_selfLogFiles->selflogLockFile;
    }
    return DEFAULT_SLOGD_LOG_LOCK;
}

void FreeSelfLogFiles(void)
{
    XFREE(g_selfLogFiles);
}

/**
 * @brief GetLocalTimeForSelfLog: get local timestamp in string
 * @param [in]bufLen: length of timeBuffer
 * @param [in]timeBuffer: buffer to store timestamp
 */
STATIC void GetLocalTimeForSelfLog(unsigned int bufLen, char *timeBuffer)
{
    ToolTimeval currentTimeval = { 0 };
    struct tm timeInfo = { 0 };

    if (timeBuffer == NULL) {
        return;
    }

    if ((ToolGetTimeOfDay(&currentTimeval, NULL)) != SYS_OK) {
        return;
    }
    const time_t sec = currentTimeval.tv_sec;
    if (ToolLocalTimeR(&sec, &timeInfo) != SYS_OK) {
        return;
    }

    int errT = snprintf_s(timeBuffer, bufLen, bufLen - 1, "%04d-%02d-%02d %02d:%02d:%02d.%03ld",
                          timeInfo.tm_year, timeInfo.tm_mon, timeInfo.tm_mday,
                          timeInfo.tm_hour, timeInfo.tm_min,
                          timeInfo.tm_sec, (currentTimeval.tv_usec / UNIT_THOUSAND));
    if (errT == -1) {
        return;
    }
}

STATIC int CatStr(const char *str1, unsigned int len1, const char *str2, unsigned int len2, Buffer *buffer)
{
    if ((str1 == NULL) || (str2 == NULL) || (buffer->buff == NULL) || ((len1 + len2) >= buffer->buffLen)) {
        return INVALID;
    }

    int ret = strcpy_s(buffer->buff, buffer->buffLen, str1);
    if (ret != EOK) {
        return INVALID;
    }
    // join two string
    ret = strcpy_s(buffer->buff + strlen(str1), buffer->buffLen - strlen(str1), str2);
    if (ret != EOK) {
        return INVALID;
    }
    return EOK;
}

/**
 * @brief CheckLogPath: mkdir log root path and slogd path
 * @return: SYS_OK/SYS_ERROR
 */
STATIC int CheckSelfLogPath(void)
{
    const char *logPath = GetRootLogPath();
    if ((logPath == NULL) || (strlen(logPath) == 0)) {
        return SYS_ERROR;
    }
    LogRt ret = MkdirIfNotExist(logPath);
    if (ret != SUCCESS) {
        return SYS_ERROR;
    }

    const char *slogdPath = GetLogAgentPath();
    if ((slogdPath == NULL) || (strlen(slogdPath) == 0)) {
        return SYS_ERROR;
    }
    ret = MkdirIfNotExist(slogdPath);
    if (ret != SUCCESS) {
        return SYS_ERROR;
    }
    return SYS_OK;
}

/**
 * @brief GetRingFd: get the logfile fd which to be written
 * @param [in]slogdFile: slogd log file path
 * @param [in]msg: log msg to be written
 * @return: success:fd, failed:INVALID(-1)
 */
STATIC int GetRingFd(const char *slogdFile, const char *msg)
{
    int getRenameFileFlag = 0;
    int fd = 0;
    int retryTime = 0;
    struct stat buf = { 0 };

    if ((slogdFile == NULL) || (msg == NULL)) {
        return INVALID;
    }

    do {
        fd = ToolOpenWithMode(slogdFile, O_CREAT | O_WRONLY | O_APPEND, FILE_MASK_WC);
        if (fd < 0) {
            if ((retryTime == 0) && (CheckSelfLogPath() == SYS_OK)) {
                retryTime++;
                getRenameFileFlag = 1;
                continue;
            }
            break;
        }

        // get file slog.log Size
        if (fstat(fd, &buf) != 0) {
            SYSLOG_WARN("get file size failed , strerr=%s.\n", strerror(ToolGetErrorCode()));
            getRenameFileFlag = 0;
            LOG_MMCLOSE_AND_SET_INVALID(fd);
            continue;
        }
        unsigned int sum = (unsigned int)(buf.st_size) + (unsigned int)(strlen(msg));
        if ((sum > (unsigned int)buf.st_size) && (sum > (unsigned int)strlen(msg)) &&
            (sum >= (unsigned int)SLOGD_LOG_MAX_SIZE)) {
            LOG_MMCLOSE_AND_SET_INVALID(fd);
            SetRingFile(slogdFile);
            getRenameFileFlag = 1;
        } else {
            getRenameFileFlag = 0;
        }
    } while (getRenameFileFlag);

    return fd;
}

void LogSyslog(int priority, const char *format, ...)
{
#if (OS_TYPE_DEF == 0)
    va_list args;

    va_start(args, format);
    vsyslog(priority, format, args);
    va_end(args);
#endif
}

void PrintSelfLog(const char *format, ...)
{
    if (format == NULL) {
        return;
    }
    va_list arg;
    va_start(arg, format);
    char msg[LOG_PRINT_LOG_BUF_SZ + 1] = { 0 };
    char timer[LOG_PRINT_LOG_TIME_STR_SIZE + 1] = { 0 };

    GetLocalTimeForSelfLog(LOG_PRINT_LOG_TIME_STR_SIZE, timer);
    int ret = strncpy_s(msg, LOG_PRINT_LOG_BUF_SZ, timer, strlen(timer));
    if (ret != EOK) {
        SYSLOG_WARN("strncpy_s time failed, result=%d, strerr=%s.\n", ret, strerror(ToolGetErrorCode()));
        va_end(arg);
        return;
    }

    unsigned int len = strlen(msg);
    int used = vsnprintf_truncated_s(msg + len, LOG_PRINT_LOG_BUF_SZ - len - 1, format, arg);
    if (used == -1) {
        va_end(arg);
        return;
    }
    // to check it is a soft connection or not
    const char *file = GetSelfLogPath();
    struct stat buf = { 0 };
#if (OS_TYPE_DEF == 0)
    if ((lstat(file, &buf) == 0) && ((S_IFMT & buf.st_mode) == S_IFLNK)) {
        va_end(arg);
        return;
    }
#endif
    int fd = GetRingFd(file, msg);
    if (fd < 0) {
        va_end(arg);
        return;
    }
    (void)ToolWrite(fd, msg, strlen(msg));
    LOG_MMCLOSE_AND_SET_INVALID(fd);
    va_end(arg);
}

#if (OS_TYPE_DEF == 0)
/**
 * @brief SetRingFile: set self dfx log file ring. lock log file during log rotation
 * @param [in]slogdFile: slogd log file path
 */
STATIC void SetRingFile(const char *slogdFile)
{
    const char logFileBufFix[SLOGD_LOG_BUFFIX_LEN + 1] = ".old";

    if (slogdFile == NULL) {
        return;
    }

    unsigned int len = strlen(slogdFile) + 1;
    if (len <= 1) {
        return;
    }
    char *newFileName = (char *)malloc(len + SLOGD_LOG_BUFFIX_LEN);
    if (newFileName == NULL) {
        return;
    }

    (void)memset_s(newFileName, len + SLOGD_LOG_BUFFIX_LEN, 0x00, len + SLOGD_LOG_BUFFIX_LEN);
    Buffer buffer = { newFileName, len + SLOGD_LOG_BUFFIX_LEN };
    int ret = CatStr(slogdFile, strlen(slogdFile), logFileBufFix, strlen(logFileBufFix), &buffer);
    if (ret != EOK) {
        XFREE(newFileName);
        return;
    }

    int lockFd = ToolOpenWithMode(GetSelfLogLockPath(), O_CREAT | O_RDWR, FILE_MASK_WC);
    if (lockFd < 0) {
        XFREE(newFileName);
        return;
    }
    // non-blocking lock. if failed to fetch lock, immediately exit func
    if (flock(lockFd, LOCK_EX | LOCK_NB) < 0) {
        LOG_MMCLOSE_AND_SET_INVALID(lockFd);
        XFREE(newFileName);
        return;
    }
    do {
        struct stat logStat = {0};
        ret = stat(GetSelfLogPath(), &logStat);
        if (ret < 0) {
            break;
        }
        // check log size to judge whether log file already rotated
        if (logStat.st_size < (SLOGD_LOG_MAX_SIZE - LOG_PRINT_LOG_BUF_SZ)) {
            break;
        }
        (void)remove(newFileName);
        (void)ToolChmod(GetSelfLogOldPath(), S_IRUSR | S_IRGRP); // readonly, 440
        (void)rename(slogdFile, newFileName);
    } while (0);

    XFREE(newFileName);
    (void)flock(lockFd, LOCK_UN);
    LOG_MMCLOSE_AND_SET_INVALID(lockFd);
    (void)remove(GetSelfLogLockPath());
    return;
}

#else
/**
 * @brief SetRingFile: rm old logfile and rename logfile with fix of .old
 * @param [in]slogdFile: slogd log file path
 */
STATIC void SetRingFile(const char *slogdFile)
{
    // rm slogdlog.old
    ToolUnlink(GetSelfLogOldPath());
    (void)rename(slogdFile, GetSelfLogOldPath());
    return;
}
#endif
