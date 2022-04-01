/**
 * @log_dump.c
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#include "log_dump.h"

#ifdef LOG_COREDUMP
#include "common.h"
#include "cfg_file_parse.h"
#include "log_common_util.h"

#define FILTER_OK 1
#define FILTER_NOK 0
#define OPERATE_MODE 0750
#define MAX_FILENAME_LEN 256
#define MAX_RETRY_COUNT 3
#define HALF_MINUTE 30000
#define DUMPFILE_NAME_PREFIX "stackcore."
#define DUMPFILE_DEVICE_PATH "/var/log/npu/coredump/"
#define MAX_RESERVE_FILE_NUMS 100
#define DUMP_DEFAULT_THREAD_ATTR {1, 0, 0, 0, 0, 1, 128 * 1024} // Default ThreadSize(128KB)

STATIC LogRt Scan(void);
STATIC void *CoreDumpWatcher(ArgPtr ptr);

/**
 * @brief CheckCoreDirIfExist: check coredump dir if existed or not, if not, make dir
 * @return: LogRt, SUCCESS/MKDIR_FAILED
 */
STATIC LogRt CheckCoreDirIfExist(void)
{
    if (access(DUMPFILE_DEVICE_PATH, F_OK) != 0) {
        int ret = ToolMkdir((const char *)DUMPFILE_DEVICE_PATH, (toolMode)(OPERATE_MODE));
        if (ret != SYS_OK) {
            SELF_LOG_ERROR("mkdir %s failed, result=%d, strerr=%s.",
                           DUMPFILE_DEVICE_PATH, ret, strerror(ToolGetErrorCode()));
            return MKDIR_FAILED;
        }
        SELF_LOG_INFO("mkdir %s succeed.", DUMPFILE_DEVICE_PATH);
    }
    return SUCCESS;
}

void CreateCoreDumpThread(void)
{
    toolThread watcherThrdID = 0;
    ToolUserBlock stCoreDumpWatcher;
    ToolThreadAttr threadAttr = DUMP_DEFAULT_THREAD_ATTR;
    stCoreDumpWatcher.procFunc = CoreDumpWatcher;
    stCoreDumpWatcher.pulArg = NULL;
    if (ToolCreateTaskWithThreadAttr(&watcherThrdID, &stCoreDumpWatcher, &threadAttr) != SYS_OK) {
        SELF_LOG_ERROR("create thread(CoreDumpWatcher) failed, strerr=%s.", strerror(ToolGetErrorCode()));
        return;
    }
    SELF_LOG_INFO("create thread(CoreDumpWatcher) succeed.");
    return;
}

STATIC void *CoreDumpWatcher(ArgPtr ptr)
{
    (void)ptr;
    if (SetThreadName("CoreDumpWatcher") != 0) {
        SELF_LOG_WARN("set thread_name(CoreDumpWatcher) failed but continue.");
    }

    while (!g_gotSignal) {
        if (CheckCoreDirIfExist() != SUCCESS) {
            ToolSleep(HALF_MINUTE);
            continue;
        }
        (void)Scan();
        ToolSleep(HALF_MINUTE);
    }

    SELF_LOG_ERROR("Thread(CoreDumpWatcher) quit, signal=%d.", g_gotSignal);
    return NULL;
}

/**
* @brief StackCoreFilter: scandir filter func
* @param [in]dir: file struct which include the path and filename
* @return: FILTER_NOK/FILTER_OK
*/
STATIC int StackCoreFilter(const ToolDirent *dir)
{
    ONE_ACT_NO_LOG(dir == NULL, return FILTER_NOK);
    if ((StartsWith(dir->d_name, DUMPFILE_NAME_PREFIX))) {
        char srcFileName[MAX_FILENAME_LEN] = { 0 };
        int ret = snprintf_s(srcFileName, MAX_FILENAME_LEN, MAX_FILENAME_LEN - 1, "%s%s",
                             DUMPFILE_DEVICE_PATH, dir->d_name);
        ONE_ACT_ERR_LOG(ret == -1, return FILTER_NOK, "snprintf_s failed, strerr=%s", strerror(ToolGetErrorCode()));

        ToolStat statbuff = { 0 };
        if (ToolStatGet(srcFileName, &statbuff) == 0) {
            // filesize must be over 0
            if (statbuff.st_size == 0) {
                SELF_LOG_ERROR("%s size is 0 and remove it", dir->d_name);
                (void)remove(srcFileName);
                return FILTER_NOK;
            }

            // only 440 can transfer
            if ((statbuff.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO)) == (S_IRUSR | S_IRGRP)) {
                return FILTER_OK;
            }
        }
    }
    return FILTER_NOK;
}

/**
* @brief SortByCreatetime: sort log files by timestamp in log filename
* @param [in]a: log file name, log filename
* @param [in]b: log file name, log filename
* @return: TRUE(1) a's timestamp newer than b
*          FALSE(0) a's timestamp older than b
*/
STATIC int SortByCreatetime(const ToolDirent **a, const ToolDirent **b)
{
    int ret;
    ToolStat aStatbuff = { 0 };
    ToolStat bStatbuff = { 0 };
    char aSrcFileName[MAX_FILENAME_LEN] = { 0 };
    char bSrcFileName[MAX_FILENAME_LEN] = { 0 };

    if ((a == NULL) || ((*a) == NULL) || (b == NULL) || ((*b) == NULL)) {
        return 1;
    }

    ret = snprintf_s(aSrcFileName, MAX_FILENAME_LEN, MAX_FILENAME_LEN - 1, "%s%s", DUMPFILE_DEVICE_PATH, (*a)->d_name);
    ONE_ACT_ERR_LOG(ret == -1, return 1, "snprintf_s A file failed, strerr=%s", strerror(ToolGetErrorCode()));

    ret = snprintf_s(bSrcFileName, MAX_FILENAME_LEN, MAX_FILENAME_LEN - 1, "%s%s", DUMPFILE_DEVICE_PATH, (*b)->d_name);
    ONE_ACT_ERR_LOG(ret == -1, return 1, "snprintf_s B file failed, strerr=%s", strerror(ToolGetErrorCode()));

    ret = ToolStatGet(aSrcFileName, &aStatbuff);
    ONE_ACT_ERR_LOG(ret != SYS_OK, return 1, "get status of A file failed, strerr=%s", strerror(ToolGetErrorCode()));

    ret = ToolStatGet(bSrcFileName, &bStatbuff);
    ONE_ACT_ERR_LOG(ret != SYS_OK, return 1, "get status of B file failed, strerr=%s", strerror(ToolGetErrorCode()));

    // file create time compare
    if (aStatbuff.st_ctime > bStatbuff.st_ctime) {
        return 0;
    } else {
        return 1;
    }
}

STATIC void RemoveStackCoreFile(const char* srcFileName)
{
    int ret;
    ONE_ACT_NO_LOG(srcFileName == NULL, return);
    int retryCnt = 0;
    do {
        retryCnt++;
        ret = remove(srcFileName);
        if (ret == SYS_OK) {
            break;
        }
    } while (retryCnt != MAX_RETRY_COUNT);

    if (ret != SYS_OK) {
        SELF_LOG_ERROR("remove source file failed, source_file=%s, result=%d, strerr=%s.",
                       srcFileName, ret, strerror(ToolGetErrorCode()));
    } else {
        SELF_LOG_INFO("remove source file succeed, source_file=%s.", srcFileName);
    }
}

/**
* @brief ScanAndTrans: scan dir and remove older files
* @return: SUCCESS/MKDIR_FAILED/CHANGE_OWNER_FAILED/SCANDIR_DIR_FAILED
*/
STATIC LogRt Scan(void)
{
    int i;
    ToolDirent **namelist = NULL;
    char srcFileName[MAX_FILENAME_LEN] = { 0 }; // include filepath and filename

    // get file lists
    int totalNum = ToolScandir((const char *)DUMPFILE_DEVICE_PATH, &namelist, StackCoreFilter, SortByCreatetime);
    if ((totalNum < 0) || (totalNum > 0 && namelist == NULL)) {
        SELF_LOG_WARN("scan directory failed, directory=%s, result=%d, strerr=%s.",
                      DUMPFILE_DEVICE_PATH, totalNum, strerror(ToolGetErrorCode()));
        return SCANDIR_DIR_FAILED;
    }

    if (totalNum <= MAX_RESERVE_FILE_NUMS) {
        ToolScandirFree(namelist, totalNum);
        return SUCCESS;
    }

    for (i = 0; i < totalNum - MAX_RESERVE_FILE_NUMS; i++) {
        ONE_ACT_WARN_LOG(namelist[i] == NULL, continue, "namelist[%d] is invalid.", i);
        (void)memset_s(srcFileName, MAX_FILENAME_LEN, 0, MAX_FILENAME_LEN);
        int ret = snprintf_s(srcFileName, MAX_FILENAME_LEN, MAX_FILENAME_LEN - 1, "%s%s", DUMPFILE_DEVICE_PATH,
                             namelist[i]->d_name);
        ONE_ACT_WARN_LOG(ret == -1, continue, "snprintf_s filename failed, result=%d, strerr=%s.", ret,
                         strerror(ToolGetErrorCode()));

        // check file can access or not
        if (access(srcFileName, R_OK) != 0) {
            SELF_LOG_WARN("file cannot be accessed, file=%s, strerr=%s.", srcFileName, strerror(ToolGetErrorCode()));
            continue;
        }
        RemoveStackCoreFile(srcFileName);
    }
    ToolScandirFree(namelist, totalNum);
    return SUCCESS;
}
#endif
