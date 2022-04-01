/**
 * @log_dump.c
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#include "app_log_watch.h"
#include "common.h"
#include "constant.h"
#include "cfg_file_parse.h"
#include "dlog_error_code.h"
#include "log_common_util.h"
#include "log_to_file.h"
#include "syslogd_common.h"
#define FILTER_OK 1
#define FILTER_NOK 0
#define OPERATE_MODE 0750
#define MAX_RETRY_COUNT 3
#define SCAN_INTERVAL 15000
#define WATCH_DEFAULT_THREAD_ATTR {1, 0, 0, 0, 0, 1, 128 * 1024} // Default ThreadSize(128KB)
#define RESERVE_DEVICE_APP_DIR_NUMS "DeviceAppDirNums"
#define MIN_RESERVE_DEVICE_APP_DIR_NUMS 1
#define MAX_RESERVE_DEVICE_APP_DIR_NUMS 96

#ifdef APP_LOG_REPORT
#define DEAFULE_RESERVE_DEVICE_APP_DIR_NUMS 24
#else
#define DEAFULE_RESERVE_DEVICE_APP_DIR_NUMS 48
#endif

typedef struct {
    ToolUserBlock block;
    toolThread tid;
} ThreadInfo;

typedef struct {
    int logType;
    char appLogPath[CFG_LOGAGENT_PATH_MAX_LENGTH + 1];
} ThreadArg;

STATIC ThreadArg g_args[LOG_TYPE_NUM];
static int g_devicfeAppDirNums = DEAFULE_RESERVE_DEVICE_APP_DIR_NUMS;

void ParseDeviceAppDirNumsCfg(void)
{
    g_devicfeAppDirNums = GetItemValueFromCfg(RESERVE_DEVICE_APP_DIR_NUMS, MIN_RESERVE_DEVICE_APP_DIR_NUMS,
                                              MAX_RESERVE_DEVICE_APP_DIR_NUMS, DEAFULE_RESERVE_DEVICE_APP_DIR_NUMS);
}

STATIC LogRt ScanAppLog(const char *path, int logType);
/**
 * @brief CheckAppDirIfExist: check app log path if existed or not, if not, make dir
 * @param [in]appLogPath: app log path
 * @return: LogRt, SUCCESS/MKDIR_FAILED/ARGV_NULL
 */
STATIC LogRt CheckAppDirIfExist(const char *appLogPath)
{
    ONE_ACT_NO_LOG(appLogPath == NULL, return ARGV_NULL);
    if (access(appLogPath, F_OK) != 0) {
        int ret = ToolMkdir(appLogPath, (toolMode)(OPERATE_MODE));
        if (ret != SYS_OK) {
            SELF_LOG_ERROR("mkdir %s failed, result=%d, strerr=%s.", appLogPath, ret, strerror(ToolGetErrorCode()));
            return MKDIR_FAILED;
        }
        SELF_LOG_INFO("mkdir %s succeed.", appLogPath);
    }
    return SUCCESS;
}

STATIC void *AppLogWatcher(ArgPtr arg)
{
    const ThreadArg *input = (ThreadArg *)arg;

    if (SetThreadName("AppLogWatcher") != 0) {
        SELF_LOG_WARN("set thread_name(AppLogWatcher) failed but continue.");
    }

    while (g_gotSignal == 0) {
        int i;
        for (i = 0; i < LOG_TYPE_NUM; i++) {
            if (strlen((input + i)->appLogPath) != 0) {
                const char *path = (input + i)->appLogPath;
                int logType = (input + i)->logType;
                ONE_ACT_ERR_LOG(CheckAppDirIfExist(path) != SUCCESS, continue, "path %s is not exist.", path);
                LogRt ret = ScanAppLog(path, logType);
                NO_ACT_WARN_LOG(ret != SUCCESS, "scan directory failed but continue, result=%d.", ret);
            }
        }
        (void)ToolSleep(SCAN_INTERVAL);
    }

    SELF_LOG_ERROR("Thread(AppLogWatcher) quit, signal=%d.", g_gotSignal);
    return NULL;
}

/**
* @brief AppLogDirFilter: scandir filter func
* @param [in]dir: file struct which include the path and filename
* @return: FILTER_NOK/FILTER_OK
*/
STATIC int AppLogDirFilter(const ToolDirent *dir)
{
    ONE_ACT_NO_LOG(dir == NULL, return FILTER_NOK);
    if ((dir->d_type == DT_DIR) && (StartsWith(dir->d_name, DEVICE_APP_HEAD) != 0)) {
        return FILTER_OK;
    }
    return FILTER_NOK;
}

STATIC int AppLogFileFilter(const ToolDirent *dir)
{
    ONE_ACT_NO_LOG(dir == NULL, return FILTER_NOK);
    if (StartsWith(dir->d_name, DEVICE_APP_HEAD) != 0) {
        return FILTER_OK;
    }
    return FILTER_NOK;
}

/**
* @brief ScanAndGetDirFile: scan and get dir and file info
* @param [in]sortArg: sort args
* @param [in]path: log path
* @param [in]a: log file name, log filename
* @param [in]b: log file name, log filename
* @return: 0:success 1/2:failed
*/
STATIC INT32 ScanAndGetDirFile(SortArg *sortArg, const char *path, const ToolDirent **a, const ToolDirent **b)
{
    int ret = snprintf_s(sortArg->aDirName, MAX_FILE_NAME_LEN, MAX_FILE_NAME_LEN - 1, "%s/%s", path, (*a)->d_name);
    ONE_ACT_ERR_LOG(ret == -1, return SYS_ERROR, "snprintf_s A dir failed, strerr=%s", strerror(ToolGetErrorCode()));
    ret = snprintf_s(sortArg->bDirName, MAX_FILE_NAME_LEN, MAX_FILE_NAME_LEN - 1, "%s/%s", path, (*b)->d_name);
    ONE_ACT_ERR_LOG(ret == -1, return SYS_ERROR, "snprintf_s B dir failed, strerr=%s", strerror(ToolGetErrorCode()));
    ret = ToolStatGet(sortArg->aDirName, &sortArg->aDirStatbuff);
    ONE_ACT_ERR_LOG(ret != SYS_OK,  return SYS_ERROR, "get status %s failed, strerr=%s",
                    sortArg->aDirName, strerror(ToolGetErrorCode()));
    ret = ToolStatGet(sortArg->bDirName, &sortArg->bDirStatbuff);
    ONE_ACT_ERR_LOG(ret != SYS_OK, return SYS_ERROR, "get status %s failed, strerr=%s",
                    sortArg->aDirName, strerror(ToolGetErrorCode()));
    return SYS_OK;
}

/**
* @brief FreeDir: free dir
* @param [in]listA: file lists of dir a
* @param [in]numA: num of files in dir a
* @param [in]listB: file lists of dir b
* @param [in]numB: num of files in dir b
* @return: void
*/
STATIC void FreeDir(ToolDirent **listA, int numA, ToolDirent **listB, int numB)
{
    ToolScandirFree(listA, numA);
    ToolScandirFree(listB, numB);
}

/**
* @brief GetSortResult: get sort result accord to time
* @param [in]sortArg: sort args
* @param [in]type: 0, numA <= 0 and numB > 0; 1, numA > 0 and numB <= 0
* @param [in]numA: num of files in dir a
* @param [in]numB: num of files in dir b
* @return: TRUE(1) a's timestamp newer than b
*          FALSE(0) a's timestamp older than b
*/
STATIC INT32 GetSortResult(SortArg *sortArg, int type, int numA, int numB)
{
    if (type == 0) {
        int ret = snprintf_s(sortArg->bFileName, MAX_FILE_NAME_LEN, MAX_FILE_NAME_LEN - 1, "%s/%s",
                             sortArg->bDirName, sortArg->listB[numB - 1]->d_name);
        if (ret == -1) {
            SELF_LOG_ERROR("snprintf_s B dir failed, strerr=%s", strerror(ToolGetErrorCode()));
            return SYS_ERROR;
        }
        ONE_ACT_ERR_LOG(ToolStatGet(sortArg->bFileName, &sortArg->bStatbuff) != SYS_OK, return SYS_ERROR,
                        "get status %s failed, strerr=%s", sortArg->bFileName, strerror(ToolGetErrorCode()));
        FreeDir(sortArg->listA, numA, sortArg->listB, numB);
        return (sortArg->aDirStatbuff.st_mtime > sortArg->bStatbuff.st_mtime) ? 1 : 0;
    } else {
        int ret = snprintf_s(sortArg->aFileName, MAX_FILE_NAME_LEN, MAX_FILE_NAME_LEN - 1, "%s/%s",
                             sortArg->aDirName, sortArg->listA[numA - 1]->d_name);
        if (ret == -1) {
            SELF_LOG_ERROR("snprintf_s A dir failed, strerr=%s", strerror(ToolGetErrorCode()));
            return SYS_ERROR;
        }
        ONE_ACT_ERR_LOG(ToolStatGet(sortArg->aFileName, &sortArg->aStatbuff) != SYS_OK, return SYS_ERROR,
                        "get status %s failed, strerr=%s", sortArg->aFileName, strerror(ToolGetErrorCode()));
        FreeDir(sortArg->listA, numA, sortArg->listB, numB);
        return (sortArg->aStatbuff.st_mtime > sortArg->bDirStatbuff.st_mtime) ? 1 : 0;
    }
}

STATIC bool IsNULLDirOrFile(const char *path, const ToolDirent **a, const ToolDirent **b)
{
    return (path == NULL) || (a == NULL) || ((*a) == NULL) || (b == NULL) || ((*b) == NULL);
}

/**
* @brief SortFileFunc: sort log files by timestamp in log filename
* @param [in]path: log dir
* @param [in]a: log file name, log filename
* @param [in]b: log file name, log filename
* @return: TRUE(1) a's timestamp newer than b
*          FALSE(0) a's timestamp older than b
*/
STATIC int SortFileFunc(const char *path, const ToolDirent **a, const ToolDirent **b)
{
    SortArg sortArg;
    (void)memset_s(&sortArg, sizeof(sortArg), 0, sizeof(sortArg));
    if (IsNULLDirOrFile(path, a, b)) {
        return 1;
    }

    int ret = ScanAndGetDirFile(&sortArg, path, a, b);
    ONE_ACT_NO_LOG(ret == SYS_ERROR, return 1);
    int numA = ToolScandir(sortArg.aDirName, &sortArg.listA, AppLogFileFilter, alphasort);
    int numB = ToolScandir(sortArg.bDirName, &sortArg.listB, AppLogFileFilter, alphasort);
    if ((numA <= 0) && (numB <= 0)) {
        FreeDir(sortArg.listA, numA, sortArg.listB, numB);
        return (sortArg.aDirStatbuff.st_mtime > sortArg.bDirStatbuff.st_mtime) ? 1 : 0;
    }

    if ((numA <= 0) && (numB > 0)) {
        ret = GetSortResult(&sortArg, 0, numA, numB);
        ONE_ACT_NO_LOG(ret == SYS_ERROR, goto SORTEXIT);
        return ret;
    }

    if ((numA > 0) && (numB <= 0)) {
        ret = GetSortResult(&sortArg, 1, numA, numB);
        ONE_ACT_NO_LOG(ret == SYS_ERROR, goto SORTEXIT);
        return ret;
    }

    // find newest log file in dir to judge dir new or old
    ret = snprintf_s(sortArg.aFileName, MAX_FILE_NAME_LEN, MAX_FILE_NAME_LEN - 1, "%s/%s",
                     sortArg.aDirName, sortArg.listA[numA - 1]->d_name);
    ONE_ACT_ERR_LOG(ret == -1, goto SORTEXIT, "snprintf_s A dir failed, strerr=%s", strerror(ToolGetErrorCode()));
    ret = snprintf_s(sortArg.bFileName, MAX_FILE_NAME_LEN, MAX_FILE_NAME_LEN - 1, "%s/%s",
                     sortArg.bDirName, sortArg.listB[numB - 1]->d_name);
    ONE_ACT_ERR_LOG(ret == -1, goto SORTEXIT, "snprintf_s B dir failed, strerr=%s", strerror(ToolGetErrorCode()));

    ONE_ACT_ERR_LOG(ToolStatGet(sortArg.aFileName, &sortArg.aStatbuff) != SYS_OK,
                    goto SORTEXIT, "get status %s failed, strerr=%s", sortArg.aFileName, strerror(ToolGetErrorCode()));
    ONE_ACT_ERR_LOG(ToolStatGet(sortArg.bFileName, &sortArg.bStatbuff) != SYS_OK,
                    goto SORTEXIT, "get status %s failed, strerr=%s", sortArg.bFileName, strerror(ToolGetErrorCode()));
    FreeDir(sortArg.listA, numA, sortArg.listB, numB);
    return (sortArg.aStatbuff.st_mtime > sortArg.bStatbuff.st_mtime) ? 1 : 0;
SORTEXIT:
    FreeDir(sortArg.listA, numA, sortArg.listB, numB);
    return 1;
}

STATIC int RemoveDir(const char *dir)
{
    int res = SYS_OK;
    ToolDirent **namelist = NULL;
    int totalNum = ToolScandir(dir, &namelist, AppLogFileFilter, NULL);
    int i;
    for (i = 0; i < totalNum; i++) {
        char fileName[MAX_FILE_NAME_LEN] = { 0 };
        int ret = snprintf_s(fileName, MAX_FILE_NAME_LEN, MAX_FILE_NAME_LEN - 1, "%s/%s", dir, namelist[i]->d_name);
        ONE_ACT_ERR_LOG(ret == -1, continue, "snprintf_s failed, strerr=%s", strerror(ToolGetErrorCode()));
        if (remove(fileName) != SYS_OK) {
            SELF_LOG_INFO("remove app log file, fileName=%s.", fileName);
            res = SYS_ERROR;
        }
    }
    (void)rmdir(dir);
    ToolScandirFree(namelist, totalNum);
    return res;
}

STATIC void RemoveAppLogDir(int logType, const char *dir)
{
    int ret;
    ONE_ACT_NO_LOG(dir == NULL, return);
    char dstDir[MAX_FILE_NAME_LEN] = { 0 };
    switch (logType) {
#ifdef SORTED_LOG
        case DEBUG_LOG:
            ret = snprintf_s(dstDir, MAX_FILE_NAME_LEN, MAX_FILE_NAME_LEN - 1, "%s/%s",
                             g_args[DEBUG_LOG].appLogPath, dir);
            break;
        case SECURITY_LOG:
            ret = snprintf_s(dstDir, MAX_FILE_NAME_LEN, MAX_FILE_NAME_LEN - 1, "%s/%s",
                             g_args[SECURITY_LOG].appLogPath, dir);
            break;
        case RUN_LOG:
            ret = snprintf_s(dstDir, MAX_FILE_NAME_LEN, MAX_FILE_NAME_LEN - 1, "%s/%s",
                             g_args[RUN_LOG].appLogPath, dir);
            break;
        case OPERATION_LOG:
            ret = snprintf_s(dstDir, MAX_FILE_NAME_LEN, MAX_FILE_NAME_LEN - 1, "%s/%s",
                             g_args[OPERATION_LOG].appLogPath, dir);
            break;
#endif
        default:
            ret = snprintf_s(dstDir, MAX_FILE_NAME_LEN, MAX_FILE_NAME_LEN - 1, "%s/%s",
                             g_args[DEBUG_LOG].appLogPath, dir);
            break;
    }
    ONE_ACT_ERR_LOG(ret == -1, return, "snprintf_s failed, strerr=%s", strerror(ToolGetErrorCode()));
    if (access(dstDir, F_OK) != 0) {
        return;
    }
    int retryCnt = 0;
    do {
        retryCnt++;
        ret = RemoveDir(dstDir);
        if (ret == SYS_OK) {
            break;
        }
    } while (retryCnt != MAX_RETRY_COUNT);

    if (ret != SYS_OK) {
        SELF_LOG_ERROR("remove app log dir failed, dir=%s, result=%d, strerr=%s.",
                       dstDir, ret, strerror(ToolGetErrorCode()));
    } else {
        SELF_LOG_INFO("remove app log dir succeed, dir=%s.", dstDir);
    }
}

/**
* @brief RemoveOldAppDirs: remove older files and keep max file number
* @param [in]logType: log type, include debug, security, run, operation
* @param [in]namelist: all device-app directory
* @param [in]totalNum: app directory number
* @return: void
*/
STATIC void RemoveOldAppDirs(int logType, ToolDirent **namelist, int totalNum)
{
    ONE_ACT_NO_LOG(namelist == NULL, return);
    int i, point;
    int start = 0;

    const char *path = g_args[logType].appLogPath;
    int deleteNum = totalNum - g_devicfeAppDirNums;
    while (start < deleteNum) {
        // find the oldest directory
        for (i = start, point = start; i < totalNum; i++) {
            const ToolDirent **cur = (const ToolDirent **)&namelist[point];
            const ToolDirent **new = (const ToolDirent **)&namelist[i];
            point = (SortFileFunc(path, cur, new) == 0) ? point : i;
        }
        if (point != start) {
            // switch the oldest item to [start]
            ToolDirent *temp = namelist[point];
            namelist[point] = namelist[start];
            namelist[start] = temp;
        }
        RemoveAppLogDir(logType, namelist[start]->d_name);
        start++;
    }
}

/**
* @brief ScanAndTrans: scan dir and remove older files
* @return: SUCCESS/MKDIR_FAILED/CHANGE_OWNER_FAILED/SCANDIR_DIR_FAILED
*/
STATIC LogRt ScanAppLog(const char *path, int logType)
{
    int totalNum;
    ToolDirent **namelist = NULL;
    // get file lists
    totalNum = ToolScandir(path, &namelist, AppLogDirFilter, NULL);
    if ((totalNum < 0) || ((totalNum > 0) && (namelist == NULL))) {
        SELF_LOG_WARN("scan directory failed, result=%d, strerr=%s.", totalNum, strerror(ToolGetErrorCode()));
        return SCANDIR_DIR_FAILED;
    }
    if (totalNum <= g_devicfeAppDirNums) {
        ToolScandirFree(namelist, totalNum);
        return SUCCESS;
    }
    RemoveOldAppDirs(logType, namelist, totalNum);
    ToolScandirFree(namelist, totalNum);
    return SUCCESS;
}

STATIC void CreateThread(void)
{
    toolThread appLogWatchThreadID = 0;
    ToolUserBlock appLogWatchThreadInfo;
    appLogWatchThreadInfo.procFunc = AppLogWatcher;
    appLogWatchThreadInfo.pulArg = g_args;
    ToolThreadAttr threadAttr = WATCH_DEFAULT_THREAD_ATTR;
    if (ToolCreateTaskWithThreadAttr(&appLogWatchThreadID, &appLogWatchThreadInfo, &threadAttr) != SYS_OK) {
        SELF_LOG_ERROR("create thread(AppLogWatcher) failed, strerr=%s.", strerror(ToolGetErrorCode()));
        return;
    }
    SELF_LOG_INFO("create thread(AppLogWatcher) succeed.");
    return;
}

void CreateAppLogWatchThread(void)
{
    // get config
    char *rootPath = GetRootLogPath();
    ONE_ACT_ERR_LOG(rootPath == NULL, return, "Root path is null and Thread(AppLogWatcher) quit.");
#ifdef  SORTED_LOG
    g_args[DEBUG_LOG].logType = DEBUG_LOG;
    int ret = snprintf_s(g_args[DEBUG_LOG].appLogPath, CFG_LOGAGENT_PATH_MAX_LENGTH + 1, CFG_LOGAGENT_PATH_MAX_LENGTH,
                         "%s/%s", rootPath, DEBUG_DIR_NAME);
    ONE_ACT_ERR_LOG(ret == -1, return, "get debug app path failed and Thread(AppLogWatcher) quit.");

    g_args[SECURITY_LOG].logType = SECURITY_LOG;
    ret = snprintf_s(g_args[SECURITY_LOG].appLogPath, CFG_LOGAGENT_PATH_MAX_LENGTH + 1, CFG_LOGAGENT_PATH_MAX_LENGTH,
                     "%s/%s", rootPath, SECURITY_DIR_NAME);
    ONE_ACT_ERR_LOG(ret == -1, return, "get security app path failed and Thread(AppLogWatcher) quit.");

    g_args[RUN_LOG].logType = RUN_LOG;
    ret = snprintf_s(g_args[RUN_LOG].appLogPath, CFG_LOGAGENT_PATH_MAX_LENGTH + 1, CFG_LOGAGENT_PATH_MAX_LENGTH,
                     "%s/%s", rootPath, RUN_DIR_NAME);
    ONE_ACT_ERR_LOG(ret == -1, return, "get run app path failed and Thread(AppLogWatcher) quit.");

    g_args[OPERATION_LOG].logType = OPERATION_LOG;
    ret = snprintf_s(g_args[OPERATION_LOG].appLogPath, CFG_LOGAGENT_PATH_MAX_LENGTH + 1,
                     CFG_LOGAGENT_PATH_MAX_LENGTH, "%s/%s", rootPath, OPERATION_DIR_NAME);
    ONE_ACT_ERR_LOG(ret == -1, return, "get operation app path failed and Thread(AppLogWatcher) quit.");
    CreateThread();
#else
    g_args[DEBUG_LOG].logType = DEBUG_LOG;
    int ret = snprintf_s(g_args[DEBUG_LOG].appLogPath, CFG_LOGAGENT_PATH_MAX_LENGTH + 1, CFG_LOGAGENT_PATH_MAX_LENGTH,
                         "%s", rootPath);
    ONE_ACT_ERR_LOG(ret == -1, return, "get app path failed and Thread(AppLogWatcher) quit.");
    CreateThread();
#endif
}
