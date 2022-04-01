/**
 * @log_to_file.c
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#include "log_to_file.h"
#include "securec.h"
#include "cfg_file_parse.h"
#include "print_log.h"
#include "log_queue.h"
#include "log_common_util.h"
#include "dlog_error_code.h"
#if (OS_TYPE_DEF == LINUX)
#include "log_zip.h"
#else
#define UNLINK_RETRY_TIME 10
#define UNLINK_SLEEP_TIME 150
#endif

static unsigned int g_openPrintNum = 0;
static unsigned int g_writeBPrintNum = 0;
static unsigned int g_rootMkPrintNum = 0;
static unsigned int g_subMkPrintNum = 0;
static unsigned int g_chmodFPrintNum = 0;
char g_logRootPath[MAX_FILEDIR_LEN + 1] = {0};

/**
* @brief LogAgentGetHostFileNumByEnv: get plog or device app log send back to host by env(EP MODE)
* @return: plog or device app log file num
*/
STATIC int LogAgentGetHostFileNumByEnv(void)
{
    const char *env = getenv("ASCEND_HOST_LOG_FILE_NUM");
    if ((env != NULL) && (IsNaturalNumStr(env))) {
        int tmpL = atoi(env);
        if ((tmpL >= HOST_FILE_MIN_NUM) && (tmpL <= HOST_FILE_MAX_NUM)) {
            return tmpL;
        }
    }
    return DEFAULT_MAX_HOST_FILE_NUM;
}

int LogFilter(const ToolDirent *dir)
{
    if (dir == NULL) {
        return 0;
    }
    int cfgPathLen = strlen(dir->d_name);
    if (!IsPathValidbyLog(dir->d_name, cfgPathLen)) {
        return 0;
    } else {
        return 1;
    }
}

int SortFunc(const ToolDirent **a, const ToolDirent **b)
{
    if ((a == NULL) || (b == NULL) || (*a == NULL) || (*b == NULL)) {
        SELF_LOG_WARN("[input] input is null.");
        return SYS_ERROR;
    }
    // sort by char
    return strcmp((*a)->d_name, (*b)->d_name);
}

unsigned int GetLocalTimeHelper(unsigned int bufLen, char *timeBuffer)
{
    struct tm timeInfo = { 0 };
    if (timeBuffer == NULL) {
        SELF_LOG_WARN("[input] time buffer is null.");
        return NOK;
    }
#ifdef GETCLOCK_VIRTUAL
    struct timespec currentTimeval = { 0, 0 };
    if (clock_gettime(CLOCK_VIRTUAL, &currentTimeval) != SYS_OK) {
        SELF_LOG_WARN("get time of day failed, strerr=%s.", strerror(ToolGetErrorCode()));
        return NOK;
    }
    const time_t sec = currentTimeval.tv_sec;
    if (ToolLocalTimeR((&sec), &timeInfo) != SYS_OK) {
        SELF_LOG_WARN("get local time failed, strerr=%s.", strerror(ToolGetErrorCode()));
        return NOK;
    }
    long ms = currentTimeval.tv_nsec / (UNIT_US_TO_MS * UNIT_US_TO_MS);
#else
    ToolTimeval currentTimeval = { 0 };
    if (ToolGetTimeOfDay(&currentTimeval, NULL) != SYS_OK) {
        SELF_LOG_WARN("get time of day failed, strerr=%s.", strerror(ToolGetErrorCode()));
        return NOK;
    }
    const time_t sec = currentTimeval.tv_sec;
    if (ToolLocalTimeR((&sec), &timeInfo) != SYS_OK) {
        SELF_LOG_WARN("get local time failed, strerr=%s.", strerror(ToolGetErrorCode()));
        return NOK;
    }
    long ms = currentTimeval.tv_usec / UNIT_US_TO_MS;
#endif

    int ret = snprintf_s(timeBuffer, bufLen, bufLen - 1, "%04d%02d%02d%02d%02d%02d%03ld",
                         timeInfo.tm_year, timeInfo.tm_mon, timeInfo.tm_mday, timeInfo.tm_hour,
                         timeInfo.tm_min, timeInfo.tm_sec, ms);
    if (ret == -1) {
        SELF_LOG_WARN("snprintf_s time buffer failed, result=%d, strerr=%s.", ret, strerror(ToolGetErrorCode()));
        return NOK;
    }

    return OK;
}

/**
* @brief LogAgentGetFileListForModule: get file list in specified directory
* @param [in/out] pstSubInfo: strut to store file list
* @param [in] dir: directory to be scanned
* @param [in] ulMaxFileNum: max file numbers can stored
* @return: OK: succeed; NOK: failed
*/
unsigned int LogAgentGetFileListForModule(StSubLogFileList *pstSubInfo, const char *dir,
                                          unsigned int ulMaxFileNum)
{
    unsigned int fileNum = 0;
    ToolDirent **namelist = NULL;
    char aucTmp[MAX_NAME_HEAD_LEN + 1] = { 0 };
    ONE_ACT_WARN_LOG(pstSubInfo == NULL, return NOK, "[input] log file list info is null.");
    ONE_ACT_WARN_LOG(dir == NULL, return NOK, "[input] log directory is null.");
    int ret = snprintf_s(aucTmp, MAX_NAME_HEAD_LEN + 1, MAX_NAME_HEAD_LEN, "%s", pstSubInfo->aucFileHead);
    ONE_ACT_WARN_LOG(ret == -1, return NOK, "snprintf fileHead failed, strerr=%s.", strerror(ToolGetErrorCode()));
    unsigned char ucLen = strnlen(aucTmp, MAX_NAME_HEAD_LEN);
    pstSubInfo->fileNum = 0;
    pstSubInfo->currIndex = 0;

    // check if sub-dir for host&device exist, if not then return immediately
    ret = ToolAccess((const char *)pstSubInfo->aucFilePath);
    ONE_ACT_NO_LOG(ret != SYS_OK, return OK);

    // get file lists
#if (OS_TYPE_DEF == LINUX)
    int toatalNum = ToolScandir((const char *)pstSubInfo->aucFilePath, &namelist, LogFilter, alphasort);
#else
    int toatalNum = ToolScandir((const char *)pstSubInfo->aucFilePath, &namelist, LogFilter, SortFunc);
#endif
    ONE_ACT_WARN_LOG(toatalNum < 0, return NOK, "scandir %s failed, result=%d, strerr=%s.",
                     dir, toatalNum, strerror(ToolGetErrorCode()));
    ONE_ACT_NO_LOG(namelist == NULL, return OK);
    int i = 0;
    for (; i < toatalNum; i++) {
        ONE_ACT_WARN_LOG(fileNum >= ulMaxFileNum, break, "illegal filenum=%u, limit=%u.", fileNum, ulMaxFileNum);
        ONE_ACT_NO_LOG((namelist[i] == NULL) || (pstSubInfo->aucFileName[fileNum] == NULL), continue);
        ret = strncmp(namelist[i]->d_name, aucTmp, ucLen);
        ONE_ACT_NO_LOG(ret != 0, continue);
        ret = strcpy_s(pstSubInfo->aucFileName[fileNum], MAX_FILENAME_LEN + 1, namelist[i]->d_name);
        ONE_ACT_WARN_LOG(ret != EOK, break, "strcpy_s failed, res=%d, strerr=%s.", ret, strerror(ToolGetErrorCode()));
        fileNum++;
    }
    ToolScandirFree(namelist, toatalNum);
    if (fileNum > 0) {
        pstSubInfo->fileNum = fileNum;
        pstSubInfo->currIndex = fileNum - 1;
    }
    return OK;
}

/**
* @brief LogAgentGetfileList: get host/devices file list while init
* @param [in/out] logList: strut to store file list
* @return: OK: succeed; NOK: failed
*/
unsigned int LogAgentGetfileList(StLogFileList *logList)
{
    if (logList == NULL) {
        SELF_LOG_WARN("[input] log file info is null.");
        return NOK;
    }

#ifndef IDE_DAEMON_HOST // for slogd
    // get host log file list
    unsigned int ret = LogAgentGetHostfileList(logList);
    if (ret != OK) {
        SELF_LOG_WARN("get host log file list failed, directory=%s, result=%u.", logList->aucFilePath, ret);
        return NOK;
    }
#endif
    return OK;
}

/**
* @brief LogAgentCreateNewFileName: get new log file name with timestamp
* @param [in/out] pstSubInfo: strut to store new file name
* @return: OK: succeed; NOK: failed
*/
unsigned int LogAgentCreateNewFileName(StSubLogFileList *pstSubInfo)
{
    char aucTime[TIME_STR_SIZE + 1] = { 0 };

    ONE_ACT_WARN_LOG(pstSubInfo == NULL, return NOK, "[input] log file list info is null.");

    if (pstSubInfo->currIndex >= pstSubInfo->maxFileNum) {
        SELF_LOG_WARN("current file number is illegal, file_bumber=%d, upper_limit=%d.",
                      pstSubInfo->currIndex, pstSubInfo->maxFileNum);
        return NOK;
    }

    int frontIndex = (pstSubInfo->maxFileNum + pstSubInfo->currIndex - 1) % pstSubInfo->maxFileNum;
    const char *frontName = pstSubInfo->aucFileName[frontIndex];
    do {
        if (GetLocalTimeHelper(TIME_STR_SIZE, aucTime) != OK) {
            return NOK;
        }

        int err = snprintf_s(pstSubInfo->aucFileName[pstSubInfo->currIndex],
                             MAX_FILENAME_LEN + 1, MAX_FILENAME_LEN, "%s%s.log", pstSubInfo->aucFileHead, aucTime);
        if (err == -1) {
            SELF_LOG_WARN("snprintf_s filename failed, result=%d, strerr=%s.", err, strerror(ToolGetErrorCode()));
            break;
        }

        char *currName = pstSubInfo->aucFileName[pstSubInfo->currIndex];
        if ((strncmp(frontName, currName, strlen(currName)) == 0) && (pstSubInfo->maxFileNum > 1)) {
            SELF_LOG_WARN("new log filename is repeat, filename=%s.", currName);
            ToolSleep(1); // sleep 1ms
            continue;
        }
        break;
    } while (1);

    return OK;
}

/**
* @brief LogAgentRemoveFile: remove log file with given filename
* @param [in] filename: file name which will be deleted
* @return: OK: succeed; NOK: failed
*/
unsigned int LogAgentRemoveFile(const char *filename)
{
    if (filename == NULL) {
        SELF_LOG_WARN("[input] filename is null.");
        return NOK;
    }

    int ret = ToolChmod(filename, LOG_FILE_RDWR_MODE);
    // logFile may be deleted by user, then selflog will be ignored
    if ((ret != 0) && (ToolGetErrorCode() != ENOENT)) {
        SELF_LOG_WARN("chmod file failed, file=%s, strerr=%s.", filename, strerror(ToolGetErrorCode()));
    }

    ret = ToolUnlink(filename);
    if (ret == 0) {
        return OK;
    } else {
        // logFile may be deleted by user, then selflog will be ignored
        if (ToolGetErrorCode() != ENOENT) {
            SELF_LOG_WARN("unlink file failed, file=%s, strerr=%s.", filename, strerror(ToolGetErrorCode()));
        }
    }

#if (OS_TYPE_DEF == WIN)
    int retryTimes = 0;
    while (TRUE) {
        retryTimes++;
        if (ToolUnlink(filename) != 0) {
            // bugfix: fp is being using(Ox20)
            if (ToolGetErrorCode() != 0x20) {
                SELF_LOG_WARN("unlink file failed for %d time(s), file=%s, strerr=%s.",
                              retryTimes, filename, strerror(ToolGetErrorCode()));
                break;
            }
            if (retryTimes == UNLINK_RETRY_TIME) {
                SELF_LOG_WARN("unlink file failed, out of time limit, file=%s, strerr=%s, limit_time=%d.",
                              filename, strerror(ToolGetErrorCode()), retryTimes);
                break;
            }
            ToolSleep(UNLINK_SLEEP_TIME);
        } else {
            SELF_LOG_INFO("unlink file succeed, file=%s.", filename);
            break;
        }
    }
#endif
    return OK;
}

/**
* @brief LogAgentDeleteCurrentFile: remove current pointed log file
* @param [in] pstSubInfo: log file list
* @return: OK: succeed; NOK: failed
*/
unsigned int LogAgentDeleteCurrentFile(const StSubLogFileList *pstSubInfo)
{
    char aucFileName[MAX_FILEPATH_LEN + MAX_FILENAME_LEN + 1] = { 0 };

    if (pstSubInfo == NULL) {
        SELF_LOG_WARN("[input] log file list info is null.");
        return NOK;
    }

    if (pstSubInfo->currIndex >= pstSubInfo->maxFileNum) {
        SELF_LOG_WARN("current file number is illegal, file_number=%d, upper_limit=%d.",
                      pstSubInfo->currIndex, pstSubInfo->maxFileNum);
        return NOK;
    }

#if (OS_TYPE_DEF == LINUX)
    int ret = snprintf_s(aucFileName, (MAX_FILEPATH_LEN + MAX_FILENAME_LEN + 1),
                         (MAX_FILEPATH_LEN + MAX_FILENAME_LEN),
                         "%s/%s", pstSubInfo->aucFilePath, pstSubInfo->aucFileName[pstSubInfo->currIndex]);
#else
    int ret = snprintf_s(aucFileName, (MAX_FILEPATH_LEN + MAX_FILENAME_LEN + 1),
                         (MAX_FILEPATH_LEN + MAX_FILENAME_LEN),
                         "%s\\%s", pstSubInfo->aucFilePath, pstSubInfo->aucFileName[pstSubInfo->currIndex]);
#endif
    if (ret == -1) {
        SELF_LOG_WARN("snprintf_s filename failed, result=%d, strerr=%s.", ret, strerror(ToolGetErrorCode()));
        return NOK;
    }

    unsigned int res = LogAgentRemoveFile(aucFileName);
    if (res != OK) {
        SELF_LOG_WARN("remove file failed, file=%s, result=%u.", aucFileName, res);
        return NOK;
    }

    return OK;
}

/**
* @brief LogAgentGetFileName: get current writing log file's full path(contains filename)
* @param [in] pstSubInfo: log file list
* @param [in] pstLogData: log data
* @param [in/out] pFileName: file name to be gotten. contains full file path
* @param [in] ucMaxLen: max length of full log file path(contains filename)
* @return: OK: succeed; NOK: failed
*/
unsigned int LogAgentGetFileName(StSubLogFileList *pstSubInfo, const StLogDataBlock *pstLogData,
                                 char *pFileName, unsigned int ucMaxLen)
{
    off_t filesize = 0;
    char tmpFileName[MAX_FILEPATH_LEN + MAX_FILENAME_LEN + 1] = { 0 };

    ONE_ACT_WARN_LOG(pstSubInfo == NULL, return NOK, "[input] log file list is null.");
    ONE_ACT_WARN_LOG(pstLogData == NULL, return NOK, "[input] log data is null.");
    ONE_ACT_WARN_LOG(pFileName == NULL, return NOK, "[input] log filename pointer is null.");

    // create new one if not exists
    if (pstSubInfo->fileNum == 0) {
        pstSubInfo->currIndex = 0;
        pstSubInfo->fileNum = 1;
        // modify for compress switch, restart with new logfile.
        // When no log file exists,need to deal with "devWriteFileFlag", the same as follows.
        pstSubInfo->devWriteFileFlag = 1;
        (void)LogAgentCreateNewFileName(pstSubInfo);
        return FilePathSplice(pstSubInfo, pFileName, ucMaxLen);
    }

    unsigned int ret = FilePathSplice(pstSubInfo, pFileName, ucMaxLen);
    ONE_ACT_NO_LOG(ret != OK, return NOK);
    int err = GetFileOfSize(pstSubInfo, pstLogData, pFileName, &filesize);
    ONE_ACT_WARN_LOG(err != OK, return NOK, "get file size failed, file=%s, result=%d.", pFileName, err);

    // modify for compress switch, restart with new logfile
    if (((filesize + pstLogData->ulDataLen) > pstSubInfo->ulMaxFileSize) || (pstSubInfo->devWriteFileFlag == 0)) {
        pstSubInfo->devWriteFileFlag = 1;
        pstSubInfo->currIndex++;
        if (pstSubInfo->currIndex != 0) {
            pstSubInfo->currIndex = pstSubInfo->currIndex % pstSubInfo->maxFileNum;
        }

        // if file num max, delete file current position
        if (pstSubInfo->fileNum == pstSubInfo->maxFileNum) {
            (void)LogAgentDeleteCurrentFile(pstSubInfo);
        }
        int i;
        for (i = 0; i < pstSubInfo->fileNum; i++) {
            (void)memset_s(tmpFileName, sizeof(tmpFileName), 0x00, sizeof(tmpFileName));
            err = snprintf_s(tmpFileName, MAX_FILEPATH_LEN + MAX_FILENAME_LEN + 1, ucMaxLen, "%s/%s",
                             pstSubInfo->aucFilePath, pstSubInfo->aucFileName[i]);
            NO_ACT_WARN_LOG(err == -1, "snprintf_s filename failed, result=%d, strerr=%s.",
                            err, strerror(ToolGetErrorCode()));

            // logFile may be deleted by user, then selflog will be ignored
            err = ToolChmod((const char*)tmpFileName, LOG_FILE_ARCHIVE_MODE);
            NO_ACT_WARN_LOG((err != 0) && (ToolGetErrorCode() != ENOENT), "chmod file failed, file=%s, strerr=%s.",
                            tmpFileName, strerror(ToolGetErrorCode()));
        }
        pstSubInfo->fileNum = NUM_MIN((pstSubInfo->fileNum + 1), pstSubInfo->maxFileNum);
        (void)LogAgentCreateNewFileName(pstSubInfo);
    }

    return FilePathSplice(pstSubInfo, pFileName, ucMaxLen);
}

#if (defined PROCESS_LOG && OS_TYPE_DEF != WIN)
/**
 * @brief LogAgentMkdirWithMultiLayer: mkdir log root path
 * @param [in]fullPath: log root path
 * @return SUCCESS: mkdir succeed; Others: failed
 */
STATIC LogRt LogAgentMkdirWithMultiLayer(const char *fullPath)
{
    int ret;
    LogRt err = SUCCESS;
    char *path = (char *)calloc(1, (MAX_FILEDIR_LEN + 1) * sizeof(char));
    if (path == NULL) {
        SELF_LOG_WARN("calloc failed, strerr=%s.", strerror(ToolGetErrorCode()));
        return MALLOC_FAILED;
    }
    char *newDir = strdup(fullPath);
    if (newDir == NULL) {
        SELF_LOG_WARN("strdup failed, strerr=%s.", strerror(ToolGetErrorCode()));
        XFREE(path);
        return STR_COPY_FAILED;
    }
    char *tmpNewDir = newDir;
    char *tmpPath   = path;
    char *token = strsep(&newDir, FILE_SEPERATOR);
    while (token != NULL) {
        if (strcmp(token, "") == 0) {
            token = strsep(&newDir, FILE_SEPERATOR);
            continue;
        }
        if (strlen(path) == 0) {
            ret = snprintf_s(path, MAX_FILEDIR_LEN + 1, MAX_FILEDIR_LEN, "/%s", token);
        } else {
            ret = snprintf_s(path, MAX_FILEDIR_LEN + 1, MAX_FILEDIR_LEN, "%s/%s", path, token);
        }
        if (ret == -1) {
            SELF_LOG_WARN("copy data failed, strerr=%s.", strerror(ToolGetErrorCode()));
            err = STR_COPY_FAILED;
            break;
        }
        err = MkdirIfNotExist((const char *)path);
        if (err != SUCCESS) {
            break;
        }
        token = strsep(&newDir, FILE_SEPERATOR);
    }
    XFREE(tmpPath);
    XFREE(tmpNewDir);
    return err;
}
#endif

STATIC unsigned int LogAgentMkdir(const char *logPath)
{
    ONE_ACT_NO_LOG(logPath == NULL, return NOK);
    int ret;
    LogRt err;

    ret = ToolAccess((const char *)logPath);
    ONE_ACT_NO_LOG(ret == SYS_OK, return OK);

#if (defined PROCESS_LOG && OS_TYPE_DEF != WIN)
    err = LogAgentMkdirWithMultiLayer((const char *)g_logRootPath);
#else
    err = MkdirIfNotExist((const char *)g_logRootPath);
#endif // PROCESS_LOG
    if (err != SUCCESS) {
        SELF_LOG_ERROR_N(&g_rootMkPrintNum, GENERAL_PRINT_NUM,
                         "mkdir %s failed, strerr=%s, log_err=%d, print once every %d times.",
                         g_logRootPath, strerror(ToolGetErrorCode()), err, GENERAL_PRINT_NUM);
        return NOK;
    }

    // create log path
#ifdef SORTED_LOG
    // judge if sub-logdir is exist(for debug/run/security/operation), if not, create one
    char sortPath[MAX_FILEDIR_LEN + 1] = { 0 };
    const char *sortDirName[LOG_TYPE_NUM] = { DEBUG_DIR_NAME, SECURITY_DIR_NAME, RUN_DIR_NAME, OPERATION_DIR_NAME };
    int idx = DEBUG_LOG;
    for (; idx < (int)LOG_TYPE_NUM; idx++) {
        (void)memset_s(sortPath, MAX_FILEDIR_LEN + 1, 0, MAX_FILEDIR_LEN + 1);
        ret = snprintf_s(sortPath, MAX_FILEDIR_LEN + 1, MAX_FILEDIR_LEN, "%s/%s", g_logRootPath, sortDirName[idx]);
        ONE_ACT_WARN_LOG(ret == -1, return NOK, "snprintf_s failed, err=%s.", strerror(ToolGetErrorCode()));
        err = MkdirIfNotExist((const char *)sortPath);
        if (err != SUCCESS) {
            SELF_LOG_ERROR_N(&g_subMkPrintNum, GENERAL_PRINT_NUM,
                             "mkdir %s failed, strerr=%s, log_err=%d, print once every %d times.",
                             sortPath, strerror(ToolGetErrorCode()), err, GENERAL_PRINT_NUM);
            return NOK;
        }
    }
#endif
    err = MkdirIfNotExist(logPath);
    if (err != SUCCESS) {
        SELF_LOG_ERROR_N(&g_subMkPrintNum, GENERAL_PRINT_NUM,
                         "mkdir %s failed, strerr=%s, log_err=%d, print once every %d times.",
                         logPath, strerror(ToolGetErrorCode()), err, GENERAL_PRINT_NUM);
        return NOK;
    }
    return OK;
}

/**
* @brief LogAgentWriteFile: write log to file in zip or not zipped format
* @param [in] subList: log file list
* @param [in] logData: log data
* @return: OK: succeed; NOK: failed
*/
unsigned int LogAgentWriteFile(StSubLogFileList *subList, StLogDataBlock *logData)
{
    char logFileName[MAX_FILEPATH_LEN + MAX_FILENAME_LEN + 1] = "";
    ONE_ACT_NO_LOG(subList == NULL, return NOK);
    ONE_ACT_NO_LOG((logData == NULL) || (logData->paucData == NULL) || (logData->ulDataLen == 0), return OK);
    // judge if sub-logdir is exist(for debug), if not, create one
    if (LogAgentMkdir(subList->aucFilePath) == NOK) {
        return NOK;
    }
    unsigned int ret;
#ifdef HARDWARE_ZIP
    if (IsZip() != 0) {
        ret = CompressWrite(subList, logData, logFileName, sizeof(logFileName));
    } else {
        ret = NoCompressWrite(subList, logData, logFileName, sizeof(logFileName));
    }
#else
    ret = NoCompressWrite(subList, logData, logFileName, sizeof(logFileName));
#endif
    // change file mode if it was masked by umask
    if (ToolChmod((const char *)logFileName, (toolMode)LOG_FILE_RDWR_MODE) != OK) {
        SELF_LOG_WARN_N(&g_chmodFPrintNum, GENERAL_PRINT_NUM,
                        "change log file mode failed, file=%s, print once every %d times.",
                        logFileName, GENERAL_PRINT_NUM);
    }
    return ret;
}

STATIC void LogAgentFreeDeviceAppMaxFileNum(StSubLogFileList *subFileList)
{
    ONE_ACT_NO_LOG(subFileList == NULL, return);
    LogAgentFreeMaxFileNumHelper(subFileList);
}

/**
* @brief LogAgentInitMaxFileNumHelper: initialize struct for log file path and log name list
* @param [in] pstSubInfo: log file list
* @param [in] logPath: log sub directory path
* @param [in] length: logPath length
* @return: OK: succeed; NOK: failed
*/
unsigned int LogAgentInitMaxFileNumHelper(StSubLogFileList *pstSubInfo, const char *logPath, int length)
{
    ONE_ACT_WARN_LOG(pstSubInfo == NULL, return NOK, "[input] log file list info is null.");
    ONE_ACT_WARN_LOG(logPath == NULL, return NOK, "[input] log filepath is null.");
    ONE_ACT_WARN_LOG(length <= 0, return NOK, "[input] log filepath length is invalid, length=%d.", length);
    (void)memset_s(pstSubInfo->aucFilePath, (MAX_FILEPATH_LEN + 1), 0, (MAX_FILEPATH_LEN + 1));
    errno_t err = snprintf_s(pstSubInfo->aucFilePath, MAX_FILEPATH_LEN + 1, MAX_FILEPATH_LEN, "%s", logPath);
    if (err == -1) {
        SELF_LOG_WARN("snprintf_s file path failed, result=%d, strerr=%s.", err, strerror(ToolGetErrorCode()));
    }

    size_t len = pstSubInfo->maxFileNum * sizeof(char *);
    if ((len == 0) || (len > (unsigned int)(UINT_MAX))) {
        SELF_LOG_WARN("current file number is invalid, file_number=%zu.", len);
        return NOK;
    }
    pstSubInfo->aucFileName = (char **)malloc(len);
    if (pstSubInfo->aucFileName == NULL) {
        SELF_LOG_WARN("malloc filename array failed, strerr=%s.", strerror(ToolGetErrorCode()));
        return NOK;
    }
    (void)memset_s((void *)pstSubInfo->aucFileName, len, 0, len);
    int idx = 0;
    for (; idx < pstSubInfo->maxFileNum; idx++) {
        pstSubInfo->aucFileName[idx] = (char *)malloc(MAX_FILENAME_LEN + 1);
        if (pstSubInfo->aucFileName[idx] == NULL) {
            SELF_LOG_WARN("malloc filename failed, strerr=%s.", strerror(ToolGetErrorCode()));
            // don't need to free pstSubInfo->aucFileName
            LogAgentFreeDeviceAppMaxFileNum(pstSubInfo);
            return NOK;
        }
        (void)memset_s((void *)pstSubInfo->aucFileName[idx], MAX_FILENAME_LEN + 1, 0, MAX_FILENAME_LEN + 1);
    }
    return OK;
}

/**
* @brief LogAgentInitHostMaxFileNumHelper: initialize struct for host sorted log files,
*                                          file num, file length and file list
* @param [out] subLogList: log file list
* @param [in] logList: root path info
* @param [in] subPath: sorted file sub path name
* @return: OK: succeed; NOK: failed
*/
STATIC unsigned int LogAgentInitHostMaxFileNumHelper(StSubLogFileList *subLogList, const StLogFileList *logList,
                                                     const char *subPath)
{
    if ((subLogList == NULL) || (logList == NULL) || (subPath == NULL) || (strlen(subPath) == 0)) {
        SELF_LOG_WARN("[input] subLogList, logList or subPath is null.");
        return NOK;
    }

    (void)memset_s(subLogList->aucFileHead, (MAX_NAME_HEAD_LEN + 1), 0, (MAX_NAME_HEAD_LEN + 1));
    errno_t err = snprintf_s(subLogList->aucFileHead, MAX_NAME_HEAD_LEN + 1, MAX_NAME_HEAD_LEN, "%s0_", HOST_HEAD);
    if (err == -1) {
        SELF_LOG_WARN("copy host header failed, res=%d, strerr=%s.", err, strerror(ToolGetErrorCode()));
        return NOK;
    }

    char logPath[MAX_FILEPATH_LEN + 1] = { 0 };
    err = snprintf_s(logPath, MAX_FILEPATH_LEN + 1, MAX_FILEPATH_LEN, "%s/%s", logList->aucFilePath, subPath);
    if (err == -1) {
        SELF_LOG_WARN("get key log dir path failed, strerr=%s.", strerror(ToolGetErrorCode()));
        return NOK;
    }
    unsigned int ret = LogAgentInitMaxFileNumHelper(subLogList, logPath, MAX_FILEPATH_LEN);
    return ret;
}

unsigned int LogAgentInitMaxFileNum(StLogFileList *logList)
{
    if (logList == NULL) {
        SELF_LOG_WARN("[input] log file list info is null.");
        return NOK;
    }

#ifndef IDE_DAEMON_HOST // for slogd
    // init for host info
    unsigned int ret = LogAgentInitHostMaxFileNum(logList);
    ONE_ACT_WARN_LOG(ret != OK, return NOK, "init host file info failed, result=%u.", ret);
#endif
    return OK;
}

/**
* @brief LogAgentFreeMaxFileNumHelper: free log file resources
* @param [in] logList: log file list
* @return: OK: succeed; NOK: failed
*/
void LogAgentFreeMaxFileNumHelper(StSubLogFileList *pstSubInfo)
{
    if ((pstSubInfo == NULL) || (pstSubInfo->aucFileName == NULL)) {
        return;
    }
    int idx = 0;
    for (; idx < pstSubInfo->maxFileNum; idx++) {
        XFREE(pstSubInfo->aucFileName[idx]);
    }
    XFREE(pstSubInfo->aucFileName);
}

/**
* @brief LogAgentFreeMaxFileNumHelper: clear all log info
* @param [in] logList: log file list
* @return: OK: succeed; NOK: failed
*/
void LogAgentFreeMaxFileNum(StLogFileList *logList)
{
    if (logList == NULL) {
        return;
    }

    LogAgentFreeHostMaxFileNum(logList);
    (void)memset_s(logList, sizeof(StLogFileList), 0x00, sizeof(StLogFileList));
}

/**
* @brief LogAgentGetFileNum: get max number of device log file from config list
* @param [in] logList: log file list
* @return: OK: succeed; NOK: failed
*/
void LogAgentGetFileNum(StLogFileList *logList)
{
    if (logList == NULL) {
        SELF_LOG_ERROR("[input] log file list info is null.");
        return;
    }
    logList->maxFileNum = DEFAULT_MAX_FILE_NUM;
    char val[CONF_VALUE_MAX_LEN + 1] = "\0";
    LogRt ret = GetConfValueByList(DEVICE_MAX_FILE_NUM_STR, strlen(DEVICE_MAX_FILE_NUM_STR),
                                 val, CONF_VALUE_MAX_LEN);
    if ((ret == SUCCESS) && IsNaturalNumStr((const char *)val)) {
        int tmpL = atoi(val);
        if (tmpL < HOST_FILE_MIN_NUM) {
            SELF_LOG_WARN("max file number less than lower limit, max_file_number=%d, use lower_limit=%d.",
                          tmpL, HOST_FILE_MIN_NUM);
            logList->maxFileNum = HOST_FILE_MIN_NUM;
        } else if (tmpL > HOST_FILE_MAX_NUM) {
            SELF_LOG_WARN("max file number more than upper limit, max_file_number=%d, use upper_limit=%d.",
                          tmpL, HOST_FILE_MAX_NUM);
            logList->maxFileNum = HOST_FILE_MAX_NUM;
        } else {
            logList->maxFileNum = tmpL;
        }
    } else {
        SELF_LOG_WARN("Use default device max file number=%d.", logList->maxFileNum);
    }
}

/**
* @brief LogAgentGetFileSize: get max size of log file from config list
* @param [in] logList: log file list
* @return: OK: succeed; NOK: failed
*/
void LogAgentGetFileSize(StLogFileList *logList)
{
    if (logList == NULL) {
        return;
    }
    logList->ulMaxFileSize = DEFAULT_MAX_FILE_SIZE;
    char val[CONF_VALUE_MAX_LEN + 1] = "\0";
    LogRt ret = GetConfValueByList(DEVICE_MAX_FILE_SIZE_STR, strlen(DEVICE_MAX_FILE_SIZE_STR),
                                 val, CONF_VALUE_MAX_LEN);
    if ((ret == SUCCESS) && IsNaturalNumStr((const char *)val)) {
        int tmpL = atoi(val);
        if (tmpL < HOST_FILE_MIN_SIZE) {
            SELF_LOG_WARN("max file size less than lower limit, max_file_size=%d, use lower_limit=%d.",
                          tmpL, HOST_FILE_MIN_SIZE);
            logList->ulMaxFileSize = HOST_FILE_MIN_SIZE;
        } else if (tmpL > HOST_FILE_MAX_SIZE) {
            SELF_LOG_WARN("max file size more than upper limit, max_file_size=%d, use upper_limit=%d.",
                          tmpL, HOST_FILE_MAX_SIZE);
            logList->ulMaxFileSize = HOST_FILE_MAX_SIZE;
        } else {
            logList->ulMaxFileSize = tmpL;
        }
    } else {
        SELF_LOG_WARN("Use default device max file size=%u.", logList->ulMaxFileSize);
    }
}

/**
* @brief LogAgentGetOsFileNum: get device os max number of device os log file from config list
* @param [in] logList: log file list
* @return: OK: succeed; NOK: failed
*/
void LogAgentGetOsFileNum(StLogFileList *logList)
{
    if (logList == NULL) {
        SELF_LOG_ERROR("[input] log file list info is null.");
        return;
    }
    logList->maxOsFileNum = DEFAULT_MAX_OS_FILE_NUM;
    char val[CONF_VALUE_MAX_LEN + 1] = "\0";
    LogRt ret = GetConfValueByList(DEVICE_OS_MAX_FILE_NUM_STR, strlen(DEVICE_OS_MAX_FILE_NUM_STR),
        val, CONF_VALUE_MAX_LEN);
    if ((ret == SUCCESS) && IsNaturalNumStr((const char *)val)) {
        int tmpL = atoi(val);
        if (tmpL < HOST_FILE_MIN_NUM) {
            SELF_LOG_WARN("device os max file number less than lower limit, max_file_number=%d, use lower_limit=%d.",
                          tmpL, HOST_FILE_MIN_NUM);
            logList->maxOsFileNum = HOST_FILE_MIN_NUM;
        } else if (tmpL > HOST_FILE_MAX_NUM) {
            SELF_LOG_WARN("device os max file number more than upper limit, max_file_number=%d, use upper_limit=%d.",
                          tmpL, HOST_FILE_MAX_NUM);
            logList->maxOsFileNum = HOST_FILE_MAX_NUM;
        } else {
            logList->maxOsFileNum = tmpL;
        }
    } else {
        SELF_LOG_WARN("Use default device os max file number=%d.", logList->maxOsFileNum);
    }
}

/**
* @brief LogAgentGetOsFileSize: get device os max size of log file from config list
* @param [in] logList: log file list
* @return: OK: succeed; NOK: failed
*/
void LogAgentGetOsFileSize(StLogFileList *logList)
{
    if (logList == NULL) {
        return;
    }
    logList->ulMaxOsFileSize = DEFAULT_MAX_OS_FILE_SIZE;
    char val[CONF_VALUE_MAX_LEN + 1] = "\0";
    LogRt ret = GetConfValueByList(DEVICE_OS_MAX_FILE_SIZE_STR, strlen(DEVICE_OS_MAX_FILE_SIZE_STR),
        val, CONF_VALUE_MAX_LEN);
    if ((ret == SUCCESS) && IsNaturalNumStr((const char *)val)) {
        int tmpL = atoi(val);
        if (tmpL < HOST_FILE_MIN_SIZE) {
            SELF_LOG_WARN("device os max file size less than lower limit, max_file_size=%d, use lower_limit=%d.",
                          tmpL, HOST_FILE_MIN_SIZE);
            logList->ulMaxOsFileSize = HOST_FILE_MIN_SIZE;
        } else if (tmpL > HOST_FILE_MAX_SIZE) {
            SELF_LOG_WARN("device os max file size more than upper limit, max_file_size=%d, use upper_limit=%d.",
                          tmpL, HOST_FILE_MAX_SIZE);
            logList->ulMaxOsFileSize = HOST_FILE_MAX_SIZE;
        } else {
            logList->ulMaxOsFileSize = tmpL;
        }
    } else {
        SELF_LOG_WARN("Use default device os max file size=%u.", logList->ulMaxOsFileSize);
    }
}

STATIC void LogAgentGetAppFileNum(StLogFileList *logList)
{
    if (logList == NULL) {
        return;
    }
    logList->maxAppFileNum = DEFAULT_MAX_APP_FILE_NUM;
    char val[CONF_VALUE_MAX_LEN + 1] = "\0";
    LogRt ret = GetConfValueByList(DEVICE_APP_MAX_FILE_NUM_STR, strlen(DEVICE_APP_MAX_FILE_NUM_STR),
                                   val, CONF_VALUE_MAX_LEN);
    if ((ret == SUCCESS) && IsNaturalNumStr((const char *)val)) {
        int tmpL = atoi(val);
        if (tmpL < HOST_FILE_MIN_NUM) {
            SELF_LOG_WARN("max app file number less than lower limit, max_file_number=%d, use lower_limit=%d.",
                          tmpL, HOST_FILE_MIN_NUM);
            logList->maxAppFileNum = HOST_FILE_MIN_NUM;
        } else if (tmpL > HOST_FILE_MAX_NUM) {
            SELF_LOG_WARN("max app file number more than upper limit, max_file_number=%d, use upper_limit=%d.",
                          tmpL, HOST_FILE_MAX_NUM);
            logList->maxAppFileNum = HOST_FILE_MAX_NUM;
        } else {
            logList->maxAppFileNum = tmpL;
        }
    } else {
        SELF_LOG_WARN("Use default max app file num=%d.", logList->maxAppFileNum);
    }
}

STATIC void LogAgentGetAppFileSize(StLogFileList *logList)
{
    if (logList == NULL) {
        return;
    }

    logList->ulMaxAppFileSize = DEFAULT_MAX_APP_FILE_SIZE;
    char val[CONF_VALUE_MAX_LEN + 1] = "\0";
    LogRt ret = GetConfValueByList(DEVICE_APP_MAX_FILE_SIZE_STR, strlen(DEVICE_APP_MAX_FILE_SIZE_STR),
                                   val, CONF_VALUE_MAX_LEN);
    if ((ret == SUCCESS) && IsNaturalNumStr((const char *)val)) {
        int tmpL = atoi(val);
        if (tmpL < HOST_APP_FILE_MIN_SIZE) {
            SELF_LOG_WARN("max app file size less than lower limit, max_file_size=%d, use lower_limit=%d.",
                          tmpL, HOST_APP_FILE_MIN_SIZE);
            logList->ulMaxAppFileSize = HOST_APP_FILE_MIN_SIZE;
        } else if (tmpL > HOST_APP_FILE_MAX_SIZE) {
            SELF_LOG_WARN("max app file size more than upper limit, max_file_size=%d, use upper_limit=%d.",
                          tmpL, HOST_APP_FILE_MAX_SIZE);
            logList->ulMaxAppFileSize = HOST_APP_FILE_MAX_SIZE;
        } else {
            logList->ulMaxAppFileSize = tmpL;
        }
    } else {
        SELF_LOG_WARN("Use default max app file size=%u.", logList->ulMaxAppFileSize);
    }
}

/**
* @brief LogAgentGetNdebugFileNum: get max number of no debug log file from config list
* @param [in] logList: log file list
* @return: OK: succeed; NOK: failed
*/
#ifdef SORTED_LOG
void LogAgentGetNdebugFileNum(StLogFileList *logList)
{
    if (logList == NULL) {
        return;
    }
    logList->maxNdebugFileNum = DEFAULT_MAX_NDEBUG_FILE_NUM;
    char val[CONF_VALUE_MAX_LEN + 1] = "\0";
    LogRt ret = GetConfValueByList(DEVICE_NDEBUG_MAX_FILE_NUM_STR, strlen(DEVICE_NDEBUG_MAX_FILE_NUM_STR),
                                   val, CONF_VALUE_MAX_LEN);
    if ((ret == SUCCESS) && IsNaturalNumStr((const char *)val)) {
        int tmpL = atoi(val);
        if (tmpL < HOST_FILE_MIN_NUM) {
            SELF_LOG_WARN("max ndebug file number less than lower limit, max_file_number=%d, use lower_limit=%d.",
                          tmpL, HOST_FILE_MIN_NUM);
            logList->maxNdebugFileNum = HOST_FILE_MIN_NUM;
        } else if (tmpL > HOST_FILE_MAX_NUM) {
            SELF_LOG_WARN("max ndebug file number more than upper limit, max_file_number=%d, use upper_limit=%d.",
                          tmpL, HOST_FILE_MAX_NUM);
            logList->maxNdebugFileNum = HOST_FILE_MAX_NUM;
        } else {
            logList->maxNdebugFileNum = tmpL;
        }
    } else {
        SELF_LOG_WARN("Use default max ndebug file num=%d.", logList->maxNdebugFileNum);
    }
}

/**
* @brief LogAgentGetNdebugFileSize: get max size of no debug log file from config list
* @param [in] logList: log file list
* @return: OK: succeed; NOK: failed
*/
void LogAgentGetNdebugFileSize(StLogFileList *logList)
{
    if (logList == NULL) {
        return;
    }
    logList->ulMaxNdebugFileSize = DEFAULT_MAX_NDEBUG_FILE_SIZE;
    char val[CONF_VALUE_MAX_LEN + 1] = "\0";
    LogRt ret = GetConfValueByList(DEVICE_NDEBUG_MAX_FILE_SIZE_STR, strlen(DEVICE_NDEBUG_MAX_FILE_SIZE_STR),
                                   val, CONF_VALUE_MAX_LEN);
    if ((ret == SUCCESS) && IsNaturalNumStr((const char *)val)) {
        int tmpL = atoi(val);
        if (tmpL < HOST_FILE_MIN_SIZE) {
            SELF_LOG_WARN("max ndebug file size less than lower limit, max_file_size=%d, use lower_limit=%d.",
                tmpL, HOST_FILE_MIN_SIZE);
            logList->ulMaxNdebugFileSize = HOST_FILE_MIN_SIZE;
        } else if (tmpL > HOST_FILE_MAX_SIZE) {
            SELF_LOG_WARN("max ndebug file size more than upper limit, max_file_size=%d, use upper_limit=%d.",
                tmpL, HOST_FILE_MAX_SIZE);
            logList->ulMaxNdebugFileSize = HOST_FILE_MAX_SIZE;
        } else {
            logList->ulMaxNdebugFileSize = tmpL;
        }
    } else {
        SELF_LOG_WARN("Use default max ndebug file size=%u.", logList->ulMaxNdebugFileSize);
    }
}
#endif

/**
* @brief GetRealPath: get absolute path
* @param [in] ppath: path
* @return: SYS_OK: succeed; SYS_ERROR: failed
*/
int GetRealPath(char *ppath, int length)
{
    const char *env = getenv("ASCEND_PROCESS_LOG_PATH");
    (void)memset_s(ppath, length, 0, length);

    if ((ToolRealPath(env, ppath, length) != SYS_OK) && (ToolGetErrorCode() != ENOENT)) {
        SELF_LOG_WARN("get realpath failed, file=%s, strerr=%s.", env, strerror(ToolGetErrorCode()));
        return SYS_ERROR;
    }
    return SYS_OK;
}


/**
* @brief IsUserEnvPath: judege if user env path
* @param [in] ppath: env path
* @return: true: succeed; false: failed
*/
bool IsUseEnvPath(const char *ppath)
{
    // dir exists and has write permission, user env path; dir not exists and mkdir succeed, use env path
    return ((((ToolAccessWithMode(ppath, F_OK) == SYS_OK) && (ToolAccessWithMode(ppath, W_OK) == SYS_OK)) ||
        ((ToolAccessWithMode(ppath, F_OK) != SYS_OK) && (ToolMkdir(ppath, (toolMode)OPERATE_MODE) == SYS_OK))));
}

/**
* @brief LogAgentGetFileDir: get root log directory from config list
* @param [in] logList: log file list
* @return: OK: succeed; NOK: failed
*/
unsigned int LogAgentGetFileDir(StLogFileList *logList)
{
    ONE_ACT_NO_LOG(logList == NULL, return NOK);

    int ret;
#ifdef PROCESS_LOG
    // get home dir
    char *homeDir = (char *)calloc(1, (TOOL_MAX_PATH + 1) * sizeof(char));
    ONE_ACT_WARN_LOG(homeDir == NULL, return NOK, "calloc failed, strerr=%s", strerror(ToolGetErrorCode()));

    ret = LogGetHomeDir(homeDir, TOOL_MAX_PATH);
    TWO_ACT_WARN_LOG(ret != 0, XFREE(homeDir), return NOK, "get home directory failed, ret=%d.", ret);

    // get process log path
    char *ppath = (char *)malloc(TOOL_MAX_PATH + 1);
    if (ppath != NULL) {
        if (GetRealPath(ppath, TOOL_MAX_PATH + 1) == SYS_OK && IsUseEnvPath(ppath)) {
            snprintf_truncated_s(logList->aucFilePath, MAX_FILEDIR_LEN + 1, "%s", ppath);
        } else {
            snprintf_truncated_s(logList->aucFilePath, MAX_FILEDIR_LEN + 1, "%s%s%s",
                                 homeDir, FILE_SEPERATOR, PROCESS_SUB_LOG_PATH);
        }
    } else {
        SELF_LOG_WARN("malloc failed, use default host log file path");
        snprintf_truncated_s(logList->aucFilePath, MAX_FILEDIR_LEN + 1, "%s%s%s",
                             homeDir, FILE_SEPERATOR, PROCESS_SUB_LOG_PATH);
    }
    XFREE(ppath);
    XFREE(homeDir);
#else
    char val[CONF_VALUE_MAX_LEN + 1] = "";
    LogRt logRt = GetConfValueByList(LOG_AGENT_FILE_DIR_STR, strlen(LOG_AGENT_FILE_DIR_STR), val, CONF_VALUE_MAX_LEN);
    if (logRt == SUCCESS) {
        (void)memset_s(logList->aucFilePath, (MAX_FILEDIR_LEN + 1), 0, (MAX_FILEDIR_LEN + 1));
        (void)snprintf_truncated_s(logList->aucFilePath, MAX_FILEDIR_LEN + 1, "%s", val);
    } else {
        (void)snprintf_truncated_s(logList->aucFilePath, MAX_FILEDIR_LEN + 1, "%s", LOG_FILE_PATH);
        SELF_LOG_WARN("get log root path failed, use default_path=%s, result=%d.", LOG_FILE_PATH, logRt);
    }
#endif

    ret = strcpy_s(g_logRootPath, MAX_FILEDIR_LEN + 1, logList->aucFilePath);
    if (ret != OK) {
        SELF_LOG_WARN("strcpy_s log directory path failed, result=%d, strerr=%s.", ret, strerror(ToolGetErrorCode()));
    }
    return OK;
}

unsigned int LogAgentGetCfg(StLogFileList *logList)
{
    if (logList == NULL) {
        SELF_LOG_WARN("[input] log file list info is null.");
        return ARGV_NULL;
    }
    LogAgentGetOsFileNum(logList);
    LogAgentGetOsFileSize(logList);
    LogAgentGetAppFileNum(logList);
    LogAgentGetAppFileSize(logList);

#ifndef RC_HISI_MODE
    LogAgentGetFileNum(logList);
    LogAgentGetFileSize(logList);
#endif

#ifdef SORTED_LOG
    LogAgentGetNdebugFileNum(logList);
    LogAgentGetNdebugFileSize(logList);
#endif
    return LogAgentGetFileDir(logList);
}

void LogAgentCleanUp(StLogFileList *logList)
{
    if (logList == NULL) {
        SELF_LOG_WARN("[input] log file list info is null.");
        return;
    }

    LogAgentFreeMaxFileNum(logList);
}

/**
 * @brief : sync buffer to disk
 * @param [in]logPath: log absolute path
 * @return : NA
 */
void FsyncLogToDisk(const char *logPath)
{
    if (logPath == NULL) {
        return;
    }

    int fd = ToolOpenWithMode(logPath, O_WRONLY | O_APPEND, LOG_FILE_RDWR_MODE);
    if (fd < 0) {
        SELF_LOG_WARN("open file failed, file=%s.", logPath);
        return;
    }

    int ret = ToolFsync(fd);
    if (ret != SYS_OK) {
        SELF_LOG_WARN("fsync fail, ret=%d, strerr=%s.", ret, strerror(ToolGetErrorCode()));
    }

    ToolClose(fd);
}

/**
* @brief GetFileOfSize: get current writing files' size.
*                       if file size exceed limitation, change file mode to read only
*                       if specified file is removed, create new log file
* @param [in] logList: log file list
* @param [in] pstLogData: log data
* @param [in] pFileName: filename with full path
* @param [in] filesize: current log file size
* @return: OK: succeed; NOK: failed
*/
int GetFileOfSize(StSubLogFileList *pstSubInfo, const StLogDataBlock *pstLogData,
                  const char *pFileName, off_t *filesize)
{
    ToolStat statbuff = { 0 };
    FILE *fp = NULL;

    char *ppath = (char *)malloc(TOOL_MAX_PATH + 1);
    if (ppath == NULL) {
        SELF_LOG_WARN("malloc failed, strerr=%s.", strerror(ToolGetErrorCode()));
        return MALLOC_FAILED;
    }
    (void)memset_s(ppath, TOOL_MAX_PATH + 1, 0, TOOL_MAX_PATH + 1);

    // get file length
    if (ToolRealPath(pFileName, ppath, TOOL_MAX_PATH + 1) == SYS_OK) {
        unsigned int cfgPathLen = strlen(ppath);
        if (IsPathValidbyLog(ppath, cfgPathLen) == false) {
            SELF_LOG_WARN("file realpath is invalid, file=%s, realpath=%s.", pFileName, ppath);
            XFREE(ppath);
            return CFG_FILE_INVALID;
        }

        fp = fopen(ppath, "r");
        if (fp != NULL) {
            if (ToolStatGet(ppath, &statbuff) == SYS_OK) {
                *filesize = statbuff.st_size;
            }
        }
    } else {
        *filesize = 0;
        (void)LogAgentCreateNewFileName(pstSubInfo);
    }
    if (*filesize > ((UINT_MAX) - pstLogData->ulDataLen)) {
        LOG_CLOSE_FILE(fp);
        XFREE(ppath);
        return NOK;
    }
    if (((unsigned int)(*filesize) + pstLogData->ulDataLen) > pstSubInfo->ulMaxFileSize) {
        if (fp != NULL) {
            FsyncLogToDisk((const char *)ppath);
            (void)ToolChmod((const char *)ppath, LOG_FILE_ARCHIVE_MODE);
        }
    }
    LOG_CLOSE_FILE(fp);
    XFREE(ppath);
    return OK;
}

/**
* @brief FilePathSplice: concat log directory path with log filename
* @param [in] pstSubInfo: log file list
* @param [in] pFileName: filename with full path
* @param [in] ucMaxLen: max file path length
* @return: OK: succeed; NOK: failed
*/
unsigned int FilePathSplice(const StSubLogFileList *pstSubInfo, char *pFileName, unsigned int ucMaxLen)
{
    int ret = snprintf_s(pFileName, ucMaxLen + 1, ucMaxLen, "%s%s%s",
                         pstSubInfo->aucFilePath, FILE_SEPERATOR, pstSubInfo->aucFileName[pstSubInfo->currIndex]);
    if (ret == -1) {
        SELF_LOG_WARN("snprintf_s filename failed, strerr=%s.", strerror(ToolGetErrorCode()));
        return NOK;
    }
    return OK;
}

/**
* @brief CompressWrite: write compressed log by hardware or software compress
* @param [in] pstSubInfo: log file list
* @param [in] pstLogData: log data to be written
* @param [in] aucFileName: filename with full path
* @param [in] aucFileNameLen: max file path length
* @return: OK: succeed; NOK: failed
*/
#if defined(HARDWARE_ZIP)
unsigned int CompressWrite(StSubLogFileList *pstSubInfo, StLogDataBlock *const pstLogData,
                           char *const aucFileName, unsigned int aucFileNameLen)
{
    ONE_ACT_WARN_LOG(((pstSubInfo == NULL) || (pstLogData == NULL) || (aucFileName == NULL) ||
                     (pstLogData->paucData == NULL)), return NOK, "null input");
    ONE_ACT_WARN_LOG((aucFileNameLen == 0) || (aucFileNameLen > (MAX_FILEPATH_LEN + MAX_FILENAME_LEN + 1)),
                     return NOK, "[input] log filename length is invalid, length=%d", aucFileNameLen);
    char *zippedBuf = NULL;
    int zippedBufLen = 0;
    unsigned int result = NOK;

#ifdef _LOG_DAEMON_T_
    const NodesInfo *nodesInfo = (NodesInfo *)(pstLogData->paucData);
    const char *dataBuf = nodesInfo->stNodeData;
#else
    const char *dataBuf = pstLogData->paucData;
#endif

    int unzippedBufLen = pstLogData->ulDataLen;
    LogRt err = HardwareZip(dataBuf, unzippedBufLen, &zippedBuf, &zippedBufLen);
    ONE_ACT_ERR_LOG(err != SUCCESS, return err, "compress buffer failed, result=%d.", err);

    pstLogData->ulDataLen = zippedBufLen;
    unsigned int ret = LogAgentGetFileName(pstSubInfo, pstLogData, aucFileName, (MAX_FILEPATH_LEN + MAX_FILENAME_LEN));
    TWO_ACT_ERR_LOG(ret != OK, XFREE(zippedBuf), return NOK, "get filename failed, result=%u.", ret);

    int fd = ToolOpenWithMode(aucFileName, O_CREAT | O_RDWR, LOG_FILE_RDWR_MODE);
    if (fd < 0) {
        SELF_LOG_ERROR_N(&g_openPrintNum, GENERAL_PRINT_NUM, "open file failed with mode, file=%s, strerr=%s,"
                         " print once every %d times.", aucFileName, strerror(ToolGetErrorCode()), GENERAL_PRINT_NUM);
        goto CMPEND;
    }

    ONE_ACT_ERR_LOG((lseek(fd, 0, SEEK_END) == INVALID), goto CMPEND, "lseek file failed, file=%s", aucFileName);
    LogMsgHeadInfo head = { FILE_VERSION, 0, 0, zippedBufLen };
    err = WriteLogBlock(fd, (const LogMsgHeadInfo *)&head, (const char *)zippedBuf);
    if (err != SUCCESS) {
        SELF_LOG_ERROR_N(&g_writeBPrintNum, GENERAL_PRINT_NUM,
                         "write log block to file=%s failed, result=%d, strerr=%s print once every %d times.",
                         aucFileName, err, strerror(ToolGetErrorCode()), GENERAL_PRINT_NUM);
        goto CMPEND;
    }

    err = LogAgentChangeLogFdMode(fd);
    NO_ACT_WARN_LOG(err != SUCCESS, "change file owner failed, file=%s, log_err=%d, strerr=%s.",
                    aucFileName, err, strerror(ToolGetErrorCode()));
    result = OK;

CMPEND:
    XFREE(zippedBuf);
    LOG_MMCLOSE_AND_SET_INVALID(fd);
    return result;
}
#endif

/**
* @brief NoCompressWrite: write log without compression
* @param [in] pstSubInfo: log file list
* @param [in] pstLogData: log data to be written
* @param [in] aucFileName: filename with full path
* @param [in] aucFileNameLen: max file path length
* @return: OK: succeed; NOK: failed
*/
unsigned int NoCompressWrite(StSubLogFileList *pstSubInfo, const StLogDataBlock *pstLogData,
                             char *const aucFileName, unsigned int aucFileNameLen)
{
    unsigned int ulRet;
    ONE_ACT_WARN_LOG(pstSubInfo == NULL, return NOK, "[input] log file list is null.");
    ONE_ACT_WARN_LOG(pstLogData == NULL, return NOK, "[input] log data is null.");
    ONE_ACT_WARN_LOG(aucFileName == NULL, return NOK, "[input] log filename is null.");
    ONE_ACT_WARN_LOG((aucFileNameLen == 0) || (aucFileNameLen > (MAX_FILEPATH_LEN + MAX_FILENAME_LEN + 1)),
                     return NOK, "[input] log filename length is invalid, length=%d", aucFileNameLen);

#ifdef _LOG_DAEMON_T_
    const NodesInfo *nodesInfo = (NodesInfo *)(pstLogData->paucData);
    const VOID *dataBuf = nodesInfo->stNodeData;
#else
    const VOID *dataBuf = pstLogData->paucData;
#endif
    ulRet = LogAgentGetFileName(pstSubInfo, pstLogData, aucFileName, (MAX_FILEPATH_LEN + MAX_FILENAME_LEN));
    if (ulRet != OK) {
        SELF_LOG_ERROR("get filename failed, result=%u.", ulRet);
        return NOK;
    }

    int fd = ToolOpenWithMode(aucFileName, O_CREAT | O_WRONLY | O_APPEND, LOG_FILE_RDWR_MODE);
    if (fd < 0) {
        SELF_LOG_ERROR_N(&g_openPrintNum, GENERAL_PRINT_NUM,
                         "open file failed with mode, file=%s, strerr=%s, print once every %d times.",
                         aucFileName, strerror(ToolGetErrorCode()), GENERAL_PRINT_NUM);
        return NOK;
    }

    int ret = ToolWrite(fd, dataBuf, pstLogData->ulDataLen);
    if ((ret < 0) || ((unsigned)ret != pstLogData->ulDataLen)) {
        LOG_MMCLOSE_AND_SET_INVALID(fd);
        SELF_LOG_ERROR_N(&g_writeBPrintNum, GENERAL_PRINT_NUM,
                         "write to file failed, file=%s, data_length=%u, write_length=%d, strerr=%s," \
                         " print once every %d time.",
                         aucFileName, pstLogData->ulDataLen, ret, strerror(ToolGetErrorCode()), GENERAL_PRINT_NUM);
        return NOK;
    }
    LogRt err = LogAgentChangeLogFdMode(fd);
    if (err != SUCCESS) {
        SELF_LOG_WARN("change file owner failed, file=%s, log_err=%u, strerr=%s.",
                      aucFileName, err, strerror(ToolGetErrorCode()));
    }
    LOG_MMCLOSE_AND_SET_INVALID(fd);
    return OK;
}

/**
* @brief LogAgentInitHostMaxFileNum: initialize struct for host log files,
*                                    file num, file length and file list
* @param [in] logList: log file list
* @return: OK: succeed; NOK: failed
*/
unsigned int LogAgentInitHostMaxFileNum(StLogFileList *logList)
{
    ONE_ACT_WARN_LOG(logList == NULL, return NOK, "[input] log file list info is null.");

    // init for security log file info
    unsigned int ret = LogAgentInitHostMaxFileNumHelper(&(logList->hostLogList), logList, HOST_DIR_NAME);
    ONE_ACT_WARN_LOG(ret != OK, return NOK, "init host file info failed, res=%u.", ret);
    return OK;
}

void LogAgentFreeHostMaxFileNum(StLogFileList *logList)
{
    if (logList == NULL) {
        return;
    }
    LogAgentFreeMaxFileNumHelper(&(logList->hostLogList));
}

unsigned int LogAgentGetHostfileList(StLogFileList *logList)
{
    ONE_ACT_NO_LOG(logList == NULL, return NOK);
    unsigned int ret = LogAgentGetFileListForModule(&(logList->hostLogList), logList->aucFilePath, logList->maxFileNum);
    ONE_ACT_WARN_LOG(ret != OK, return NOK, "get host file list failed, res=%u.", ret);
    return OK;
}

unsigned int LogAgentWriteHostLog(LogType logType, StLogFileList *logList, const char *msg, unsigned int len)
{
    ONE_ACT_WARN_LOG(msg == NULL, return NOK, "[input] host log buff is null.");
    ONE_ACT_WARN_LOG(logList == NULL, return NOK, "[input] log file list info is null.");
    ONE_ACT_WARN_LOG(len == 0, return NOK, "[input] host log buff size is 0.");
    ONE_ACT_WARN_LOG(logType != DEBUG_LOG, return NOK, "[input] sorted type=%d is not debug.", logType);

    StLogDataBlock stLogData = { 0 };
    stLogData.ucDeviceID = 0;
    stLogData.ulDataLen = len;
    stLogData.paucData = (char *)msg;
    unsigned int ret = LogAgentWriteFile(&(logList->hostLogList), &stLogData);
    return ret;
}

void LogAgentCleanUpDeviceOs(StLogFileList *logList)
{
    ONE_ACT_NO_LOG(logList == NULL, return);
#ifdef SORTED_LOG
    int idx = DEBUG_LOG;
    for (; idx < (int)LOG_TYPE_NUM; idx++) {
        LogAgentFreeMaxFileNumHelper(&(logList->sortDeviceOsLogList[idx]));
    }
#else
    LogAgentFreeMaxFileNumHelper(&(logList->deviceOsLogList));
#endif
}

STATIC unsigned int LogAgentGetDeviceOsFileList(StLogFileList *logList)
{
    StSubLogFileList *list = NULL;
    if (logList == NULL) {
        SELF_LOG_WARN("[input] log file info is null.");
        return NOK;
    }
#ifdef SORTED_LOG
    int i = DEBUG_LOG;
    for (; i < (int)LOG_TYPE_NUM; i++) {
        list = &(logList->sortDeviceOsLogList[i]);
        unsigned int ret = LogAgentGetFileListForModule(list, list->aucFilePath, list->maxFileNum);
        if (ret != OK) {
            SELF_LOG_WARN("get device os log file list failed, directory=%s, result=%u", logList->aucFilePath, ret);
            return NOK;
        }
    }
#else
    list = &(logList->deviceOsLogList);
    unsigned int ret = LogAgentGetFileListForModule(list, list->aucFilePath, list->maxFileNum);
    if (ret != OK) {
        SELF_LOG_WARN("get device os log file list failed, directory=%s, result=%u", logList->aucFilePath, ret);
        return NOK;
    }
#endif
    return OK;
}

STATIC unsigned int LogAgentInitDeviceOsMaxFileNum(StLogFileList *logList)
{
    char deviceOsLogPath[MAX_FILEPATH_LEN + 1] = { 0 };
    ONE_ACT_WARN_LOG(logList == NULL, return NOK, "[input] log file list is null.");
#ifdef SORTED_LOG
    int i = DEBUG_LOG;
    const char* sortDirName[LOG_TYPE_NUM] = { DEBUG_DIR_NAME, SECURITY_DIR_NAME, RUN_DIR_NAME, OPERATION_DIR_NAME };
    for (; i < (int)LOG_TYPE_NUM; i++) {
        StSubLogFileList* list = &(logList->sortDeviceOsLogList[i]);
        ONE_ACT_WARN_LOG(list == NULL, return NOK, "[input] list is null.");
        if (i == (int)DEBUG_LOG) {
            list->maxFileNum = logList->maxOsFileNum;
            list->ulMaxFileSize = logList->ulMaxOsFileSize;
        } else {
            list->maxFileNum = logList->maxNdebugFileNum;
            list->ulMaxFileSize = logList->ulMaxNdebugFileSize;
        }
        int err = snprintf_s(list->aucFileHead, MAX_NAME_HEAD_LEN + 1, MAX_NAME_HEAD_LEN, "%s_", DEVICE_OS_HEAD);
        ONE_ACT_WARN_LOG(err == -1, return NOK, "get device os header failed, result=%d, strerr=%s.", \
                         err, strerror(ToolGetErrorCode()));
        (void)memset_s(deviceOsLogPath, MAX_FILEPATH_LEN + 1, 0, MAX_FILEPATH_LEN + 1);
        err = snprintf_s(deviceOsLogPath, MAX_FILEPATH_LEN + 1, MAX_FILEPATH_LEN, "%s%s%s%s%s",
                         logList->aucFilePath, FILE_SEPERATOR, sortDirName[i], FILE_SEPERATOR, DEVICE_OS_HEAD);
        ONE_ACT_WARN_LOG(err == -1, return NOK, "get device os log dir path failed, result=%d, strerr=%s.",
                         err, strerror(ToolGetErrorCode()));
        unsigned int ret = LogAgentInitMaxFileNumHelper(list, deviceOsLogPath, MAX_FILEPATH_LEN);
        ONE_ACT_WARN_LOG(ret != OK, return NOK, "init max device os filename list failed, result=%d.", ret);
    }
#else
    StSubLogFileList* list = &(logList->deviceOsLogList);
    ONE_ACT_WARN_LOG(list == NULL, return NOK, "[input] list is null.");
    list->maxFileNum = logList->maxOsFileNum;
    list->ulMaxFileSize = logList->ulMaxOsFileSize;
    (void)memset_s(list->aucFileHead, MAX_NAME_HEAD_LEN + 1, 0, MAX_NAME_HEAD_LEN + 1);

    int err = snprintf_s(list->aucFileHead, MAX_NAME_HEAD_LEN + 1, MAX_NAME_HEAD_LEN, "%s_", DEVICE_OS_HEAD);
    ONE_ACT_WARN_LOG(err == -1, return NOK, "get device os header failed, result=%d, strerr=%s.", \
                     err, strerror(ToolGetErrorCode()));
    err = snprintf_s(deviceOsLogPath, MAX_FILEPATH_LEN + 1, MAX_FILEPATH_LEN, "%s%s%s",
                     logList->aucFilePath, FILE_SEPERATOR, DEVICE_OS_HEAD);
    ONE_ACT_WARN_LOG(err == -1, return NOK, "get device os log dir path failed, result=%d, strerr=%s.",
                     err, strerror(ToolGetErrorCode()));
    unsigned int ret = LogAgentInitMaxFileNumHelper(list, deviceOsLogPath, MAX_FILEPATH_LEN);
    ONE_ACT_WARN_LOG(ret != OK, return NOK, "init max device os filename list failed, result=%d.", ret);
#endif
    return OK;
}

unsigned int LogAgentInitDeviceOs(StLogFileList *logList)
{
    ONE_ACT_WARN_LOG(logList == NULL, return NOK, "[input] log file list info is null.");
    (void)memset_s(logList, sizeof(StLogFileList), 0x00, sizeof(StLogFileList));
    snprintf_truncated_s(logList->aucFilePath, MAX_FILEDIR_LEN + 1, "%s", LOG_FILE_PATH);

    if (LogAgentGetCfg(logList) != OK) {
        SELF_LOG_WARN("init device os config failed.");
        return NOK;
    }
    if (LogAgentInitDeviceOsMaxFileNum(logList) != OK) {
        LogAgentCleanUpDeviceOs(logList);
        SELF_LOG_WARN("init device os file list failed.");
        return NOK;
    }
    if (LogAgentGetDeviceOsFileList(logList) != OK) {
        LogAgentCleanUpDeviceOs(logList);
        SELF_LOG_WARN("get current device os file list failed.");
        return NOK;
    }
    return OK;
}

unsigned int LogAgentWriteDeviceOsLog(LogType logType, StLogFileList *logList, const char *msg, unsigned int len)
{
    ONE_ACT_WARN_LOG(msg == NULL, return NOK, "[input] device log buff is null.");
    ONE_ACT_WARN_LOG(len == 0, return NOK, "[input] device log buff size is 0.");
    ONE_ACT_WARN_LOG(logList == NULL, return NOK, "[input] log file list is null.");
    StLogDataBlock stLogData = { 0 };
    stLogData.ucDeviceID = 0;
    stLogData.ulDataLen = len;
    stLogData.paucData = (char *)msg;

#ifdef SORTED_LOG
    (void)logType;
    (void)len;
    ONE_ACT_WARN_LOG((logType < DEBUG_LOG) || (logType >= LOG_TYPE_NUM),
                     return NOK, "[input] wrong log type %d", logType);
    unsigned int ret = LogAgentWriteFile(&(logList->sortDeviceOsLogList[logType]), &stLogData);
#else
    (void)logType;
    unsigned int ret = LogAgentWriteFile(&(logList->deviceOsLogList), &stLogData);
#endif
    return ret;
}

void LogAgentCleanUpDevice(StLogFileList *logList)
{
    if ((logList == NULL) || (logList->deviceLogList == NULL)) {
        return;
    }
    unsigned char idx = 0;
    for (; idx < logList->ucDeviceNum; idx++) {
        LogAgentFreeMaxFileNumHelper(&(logList->deviceLogList[idx]));
    }
    XFREE(logList->deviceLogList);
}

STATIC unsigned int LogAgentGetDeviceFileList(StLogFileList *logList)
{
    if (logList == NULL) {
        SELF_LOG_WARN("[input] log file list is null.");
        return NOK;
    }

    StSubLogFileList *list = NULL;
    unsigned char idx = 0;
    for (; idx < logList->ucDeviceNum; idx++) {
        list = &(logList->deviceLogList[idx]);
#ifndef PROCESS_LOG
        unsigned int ret = LogAgentGetFileListForModule(list, list->aucFilePath, list->maxFileNum);
        if (ret != OK) {
            SELF_LOG_WARN("get device log file list failed, directory=%s, device_id=%d, result=%u.",
                          list->aucFilePath, idx, ret);
            return NOK;
        }
#else
        list->fileNum = 0;
        list->currIndex = 0;
#endif
    }
    return OK;
}

unsigned int LogAgentInitDeviceMaxFileNum(StLogFileList *logList)
{
    ONE_ACT_WARN_LOG(logList == NULL, return NOK, "[input] log file list info is null.");
    unsigned int idx = 0;
    for (; idx < logList->ucDeviceNum; idx++) {
        StSubLogFileList* list = &(logList->deviceLogList[idx]);
        ONE_ACT_WARN_LOG(list == NULL, return NOK, "[input] list is null.");
        (void)memset_s(list->aucFileHead, MAX_NAME_HEAD_LEN + 1, 0, MAX_NAME_HEAD_LEN + 1);
#ifdef PROCESS_LOG
        list->maxFileNum = LogAgentGetHostFileNumByEnv();
        list->ulMaxFileSize = DEFAULT_MAX_HOST_FILE_SIZE;
        int err = snprintf_s(list->aucFileHead, MAX_NAME_HEAD_LEN + 1, MAX_NAME_HEAD_LEN,
                             "%s%u_", DEVICE_HEAD, ToolGetPid());
#else
        list->maxFileNum = logList->maxFileNum;
        list->ulMaxFileSize = logList->ulMaxFileSize;
        int err = snprintf_s(list->aucFileHead, MAX_NAME_HEAD_LEN + 1, MAX_NAME_HEAD_LEN,
                             "%s%u_", DEVICE_HEAD, GetHostDeviceID(idx));
#endif
        ONE_ACT_WARN_LOG(err == -1, return NOK, "get device header failed, result=%d, strerr=%s.", \
                         err, strerror(ToolGetErrorCode()));

        char deviceLogPath[MAX_FILEPATH_LEN + 1] = { 0 };
#ifdef  SORTED_LOG
        // device log must be debug type
        err = snprintf_s(deviceLogPath, MAX_FILEPATH_LEN + 1, MAX_FILEPATH_LEN, "%s%s%s%s%s%u", logList->aucFilePath,
                         FILE_SEPERATOR, DEBUG_DIR_NAME, FILE_SEPERATOR, DEVICE_HEAD, GetHostDeviceID(idx));
#else
        err = snprintf_s(deviceLogPath, MAX_FILEPATH_LEN + 1, MAX_FILEPATH_LEN, "%s%s%s%u", logList->aucFilePath,
                         FILE_SEPERATOR, DEVICE_HEAD, GetHostDeviceID(idx));
#endif
        ONE_ACT_WARN_LOG(err == -1, return NOK, "get device log dir path failed, device_id=%u, result=%d, strerr=%s.",
                         idx, err, strerror(ToolGetErrorCode()));
        unsigned int ret = LogAgentInitMaxFileNumHelper(list, deviceLogPath, MAX_FILEPATH_LEN);
        ONE_ACT_WARN_LOG(ret != OK, return NOK, "init max device filename list failed, result=%u.", ret);
    }
    return OK;
}

unsigned int LogAgentInitDevice(StLogFileList *logList, unsigned char deviceNum)
{
    ONE_ACT_WARN_LOG(logList == NULL, return NOK, "[input] log file list is null.");
    logList->ucDeviceNum = deviceNum;

    unsigned int len = sizeof(StSubLogFileList) * deviceNum;
    ONE_ACT_WARN_LOG(len == 0, return NOK, "device number is invalid, device_number=%u.", deviceNum);
    // multi devices and os init
    logList->deviceLogList = (StSubLogFileList *)malloc(len);
    if (logList->deviceLogList == NULL) {
        SELF_LOG_WARN("malloc failed, strerr=%s.", strerror(ToolGetErrorCode()));
        return NOK;
    }
    (void)memset_s(logList->deviceLogList, len, 0, len); // include init devWriteFlag
    // init log file list
    if (LogAgentInitDeviceMaxFileNum(logList) != OK) {
        LogAgentCleanUpDevice(logList);
        SELF_LOG_WARN("init device file list failed.");
        return NOK;
    }
    // get log dir files
    if (LogAgentGetDeviceFileList(logList) != OK) {
        LogAgentCleanUpDevice(logList);
        SELF_LOG_WARN("get current device file list failed.");
        return NOK;
    }
    return OK;
}

unsigned int LogAgentWriteDeviceLog(const StLogFileList *logList, char *msg, const DeviceWriteLogInfo *info)
{
    StLogDataBlock stLogData = { 0 };
    ONE_ACT_WARN_LOG(logList == NULL, return NOK, "[input] log file list info is null.");
    ONE_ACT_WARN_LOG(msg == NULL, return NOK, "[input] log message is null.");
    ONE_ACT_WARN_LOG(info == NULL, return NOK, "[input] info is null.");

    if ((info->deviceId > logList->ucDeviceNum) || (info->slogFlag == 1)) {
        SELF_LOG_WARN("[input] wrong device_id=%u, device_number=%u or slogFlag=%d.",
                      info->deviceId, logList->ucDeviceNum, info->slogFlag);
        return NOK;
    }

    if (logList->deviceLogList == NULL) {
        SELF_LOG_WARN("[input] device log file list is null.");
        return NOK;
    }

    unsigned int deviceId = info->deviceId;
    stLogData.ucDeviceID = deviceId;
    stLogData.ulDataLen = info->len;
    stLogData.paucData = msg;
    return LogAgentWriteFile(&(logList->deviceLogList[deviceId]), &stLogData);
}

STATIC unsigned int LogAgentGetDeviceAppFileList(StSubLogFileList *subFileList)
{
    if (subFileList == NULL) {
        SELF_LOG_WARN("[input] log file info is null.");
        return NOK;
    }
    unsigned int ret = LogAgentGetFileListForModule(subFileList, subFileList->aucFilePath, subFileList->maxFileNum);
    if (ret != OK) {
        SELF_LOG_WARN("get device app log file list failed, directory=%s, result=%u", subFileList->aucFilePath, ret);
        return NOK;
    }
    return OK;
}

STATIC unsigned int LogAgentInitDeviceApp(const StLogFileList *logList, StSubLogFileList *subFileList,
                                          const LogInfo* logInfo)
{
    ONE_ACT_WARN_LOG(logList == NULL, return NOK, "[input] log file list is null.");
    ONE_ACT_WARN_LOG(subFileList == NULL, return NOK, "[input] sub log file list is null.");
    ONE_ACT_WARN_LOG(logInfo == NULL, return NOK, "[input] log info is null.");

    unsigned int pid = logInfo->pid;
    subFileList->maxFileNum = logList->maxAppFileNum;
    subFileList->ulMaxFileSize = logList->ulMaxAppFileSize;
    int ret = snprintf_s(subFileList->aucFileHead, MAX_NAME_HEAD_LEN + 1, MAX_NAME_HEAD_LEN,
                         "%s%u_", DEVICE_APP_HEAD, pid);
    ONE_ACT_WARN_LOG(ret == -1, return NOK, "get device app header failed, result=%d, strerr=%s.", \
                     ret, strerror(ToolGetErrorCode()));
    char deviceAppLogPath[MAX_FILEPATH_LEN + 1] = { 0 };
#ifdef SORTED_LOG
    const char* sortDirName[LOG_TYPE_NUM] = { DEBUG_DIR_NAME, SECURITY_DIR_NAME, RUN_DIR_NAME, OPERATION_DIR_NAME };
    ret = snprintf_s(deviceAppLogPath, MAX_FILEPATH_LEN + 1, MAX_FILEPATH_LEN, "%s%s%s%s%s%u", logList->aucFilePath,
                     FILE_SEPERATOR, sortDirName[logInfo->type], FILE_SEPERATOR, DEVICE_APP_HEAD, pid);
#else
    ret = snprintf_s(deviceAppLogPath, MAX_FILEPATH_LEN + 1, MAX_FILEPATH_LEN, "%s%s%s%u",
                     logList->aucFilePath, FILE_SEPERATOR, DEVICE_APP_HEAD, pid);
#endif
    ONE_ACT_WARN_LOG(ret == -1, return NOK, "get device app log dir path failed, pid=%u, result=%d, strerr=%s.",
                     pid, ret, strerror(ToolGetErrorCode()));
    unsigned int res = LogAgentInitMaxFileNumHelper(subFileList, deviceAppLogPath, MAX_FILEPATH_LEN);
    ONE_ACT_WARN_LOG(res != OK, return NOK, "init max device app filename list failed, result=%u.", res);
    if (LogAgentGetDeviceAppFileList(subFileList) != OK) {
        LogAgentFreeDeviceAppMaxFileNum(subFileList);
        SELF_LOG_WARN("get current device app file list failed.");
        return NOK;
    }
    return OK;
}

unsigned int LogAgentWriteDeviceAppLog(const char *msg, unsigned int len, const LogInfo* logInfo)
{
    ONE_ACT_WARN_LOG((msg == NULL) || (len == 0), return NOK, "[input] device app log buff is null or len=%u.", len);
    ONE_ACT_WARN_LOG(logInfo == NULL, return NOK, "[input] device app log info is null.");
    ONE_ACT_WARN_LOG(logInfo->processType != APPLICATION, return NOK, "[input] wrong device app log type=%d.",
                     logInfo->processType);
#ifdef SORTED_LOG
    ONE_ACT_WARN_LOG((logInfo->type < DEBUG_LOG) || (logInfo->type >= LOG_TYPE_NUM), return NOK,
                     "[input] wrong device app log type=%d.", logInfo->type);
#endif
    StLogFileList logList = { 0 };
    StLogDataBlock stLogData = { 0 };
    StSubLogFileList subFileList = { 0 };
    stLogData.ucDeviceID = 0;
    stLogData.ulDataLen = len;
    stLogData.paucData = (char *)msg;
    (void)memset_s(&logList, sizeof(StLogFileList), 0, sizeof(StLogFileList));
    (void)memset_s(&subFileList, sizeof(StSubLogFileList), 0, sizeof(StSubLogFileList));
    if (LogAgentGetCfg(&logList) != OK) {
        SELF_LOG_WARN("init device os config failed.");
        return NOK;
    }

    unsigned int ret = LogAgentInitDeviceApp(&logList, &subFileList, logInfo);
    if (ret != OK) {
        SELF_LOG_WARN("get current device app file list failed.");
        return NOK;
    }
    subFileList.devWriteFileFlag = 1;
    ret = LogAgentWriteFile(&subFileList, &stLogData);
    LogAgentFreeDeviceAppMaxFileNum(&subFileList);
    return ret;
}

STATIC unsigned int LogAgentInitProcMaxFileNum(StLogFileList *logList)
{
    ONE_ACT_WARN_LOG(logList == NULL, return NOK, "[input] log file list info is null.");
    StSubLogFileList *list = &(logList->hostLogList);
    list->maxFileNum = LogAgentGetHostFileNumByEnv();
    list->ulMaxFileSize = DEFAULT_MAX_HOST_FILE_SIZE;
    list->fileNum = 0;
    list->currIndex = 0;

    // init log file head
    (void)memset_s(list->aucFileHead, MAX_NAME_HEAD_LEN + 1, 0, MAX_NAME_HEAD_LEN + 1);
    int err = snprintf_s(list->aucFileHead, MAX_NAME_HEAD_LEN + 1, MAX_NAME_HEAD_LEN,
                         "%s-%d_", PROC_HEAD, ToolGetPid());
    ONE_ACT_WARN_LOG(err == -1, return NOK, "get proc header failed, result=%d, strerr=%s.", \
                     err, strerror(ToolGetErrorCode()));

    // init log file root path
    char logPath[MAX_FILEPATH_LEN + 1] = { 0 };
    err = snprintf_s(logPath, MAX_FILEPATH_LEN + 1, MAX_FILEPATH_LEN, "%s%s%s",
                     logList->aucFilePath, FILE_SEPERATOR, PROC_DIR_NAME);
    ONE_ACT_WARN_LOG(err == -1, return NOK, "get proc log dir path failed, result=%d, strerr=%s.",
                     err, strerror(ToolGetErrorCode()));

    // init proc log list, include log path and filename list
    unsigned int ret = LogAgentInitMaxFileNumHelper(list, logPath, MAX_FILEPATH_LEN);
    ONE_ACT_WARN_LOG(ret != OK, return NOK, "init max proc filename list failed, result=%u.", ret);

    return OK;
}

unsigned int LogAgentInitProc(StLogFileList *logList)
{
    ONE_ACT_WARN_LOG(logList == NULL, return NOK, "[input] log file list info is null.");

    // read configure file
    if (LogAgentGetCfg(logList) != OK) {
        SELF_LOG_WARN("read and init config failed.");
        return NOK;
    }
    // init log file list
    if (LogAgentInitProcMaxFileNum(logList) != OK) {
        LogAgentCleanUpProc(logList);
        SELF_LOG_WARN("init max filename list failed.");
        return NOK;
    }
    return OK;
}

unsigned int LogAgentWriteProcLog(StLogFileList *logList, const char *buf, unsigned int bufLen)
{
    return LogAgentWriteHostLog(DEBUG_LOG, logList, buf, bufLen);
}

void LogAgentCleanUpProc(StLogFileList *logList)
{
    ONE_ACT_NO_LOG(logList == NULL, return);

    LogAgentFreeMaxFileNumHelper(&(logList->hostLogList));
}
