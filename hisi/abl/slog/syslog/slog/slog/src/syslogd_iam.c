/**
 * @syslogd_iam.c
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#include "syslogd.h"
#include "syslogd_common.h"
#include "securec.h"
#include "cfg_file_parse.h"
#include "common.h"
#include "dlog_error_code.h"
#include "log_to_file.h"
#include "iam.h"
#define FILTER_OK 1
#define FILTER_NOK 0
#define MAX_BUFNODE_NUM 16384
#define FIFTY_MILLI_SECOND 50
#define MAX_PATH_NAME_LEN (CFG_LOGAGENT_PATH_MAX_LENGTH)
#define MAX_APP_FILEPATH_LEN (CFG_LOGAGENT_PATH_MAX_LENGTH + MAX_FILENAME_LEN)

STATIC ToolUserBlock g_writeFileThread;
static toolMutex g_listMutex = PTHREAD_MUTEX_INITIALIZER;
struct Globals *g_globalsPtr = NULL;
LogLevel g_logLevel;
STATIC unsigned int g_bufListNodeNum = 0;
STATIC void SyncAllToDisk(int appPid);

typedef struct BufList {
    char buf[MSG_LENGTH];
    int type;
    unsigned int len;
    struct BufList *next;
} BufList;

static BufList *g_bufHead = NULL;
static BufList *g_bufTail = NULL;

void FflushLogDataBuf(void);

STATIC int IamReady(void)
{
    int retry = 0;
    int ret = SYS_ERROR;
    while ((ret != SYS_OK) && (retry < IAM_RETRY_TIMES)) {
        ret = IAMResMgrReady();
        retry++;
    }
    return (ret == SYS_OK) ? SYS_OK : SYS_ERROR;
}

int InitGlobals(void)
{
    if (IamReady() != SYS_OK) {
        SELF_LOG_ERROR("iam resource manager ready failed, strerr=%s.", strerror(ToolGetErrorCode()));
        return SYS_ERROR;
    }
    SELF_LOG_INFO("iam resource manager ready");
    void *ptr = malloc(sizeof(*g_globalsPtr));
    if (ptr == NULL) {
        SELF_LOG_WARN("malloc failed, strerr=%s.", strerror(ToolGetErrorCode()));
        return SYS_ERROR;
    }
    (void)memset_s(ptr, sizeof(struct Globals), 0x00, sizeof(struct Globals));
    SET_GLOBALS_PTR(ptr);
    g_globalsPtr->moduleId = -1;
    (void)memset_s(&g_logLevel, sizeof(LogLevel), 0x00, sizeof(LogLevel));
    g_logLevel.globalLevel = MODULE_DEFAULT_LOG_LEVEL;
    g_logLevel.enableEvent = 1;
    return SYS_OK;
}

STATIC int IamRead(char *recvBuf, unsigned int recvBufLen, int *logType)
{
    int readLength;
    if ((recvBuf == NULL) || (recvBufLen == 0) || (logType == NULL)) {
        SELF_LOG_WARN("Input invalid, recvBuf is null or invalid recvBufLen=%d.", recvBufLen);
        return SYS_ERROR;
    }

    BufList *curHeadBuf = NULL;
    LOCK_WARN_LOG(&g_listMutex);
    if (g_bufHead == NULL) {
        UNLOCK_WARN_LOG(&g_listMutex);
        return SYS_INVALID_PARAM;
    }

    curHeadBuf = g_bufHead;
    if (recvBufLen < curHeadBuf->len) {
        SELF_LOG_WARN("list buffer length(%u), input length(%d), canot copy", curHeadBuf->len, recvBufLen);
        UNLOCK_WARN_LOG(&g_listMutex);
        return SYS_ERROR;
    }

    int ret = memcpy_s(recvBuf, recvBufLen, curHeadBuf->buf, curHeadBuf->len);
    if (ret != EOK) {
        SELF_LOG_WARN("memcpy failed, strerr=%s.", strerror(ToolGetErrorCode()));
        UNLOCK_WARN_LOG(&g_listMutex);
        return SYS_ERROR;
    }
    *logType = curHeadBuf->type;
    g_bufHead = g_bufHead->next;
    readLength = curHeadBuf->len;
    XFREE(curHeadBuf);
    if (g_bufListNodeNum > 0) {
        g_bufListNodeNum--;
    }
    UNLOCK_WARN_LOG(&g_listMutex);
    return readLength;
}

// log Iam ops, include read, write, ioctl, open and close
STATIC ssize_t LogIamOpsRead(struct IAMMgrFile *file, char *buf, size_t len, loff_t *pos)
{
    (void)file;
    (void)buf;
    (void)len;
    (void)pos;
    return SYS_OK;
}

/*
* @brief: log write callback
* @return: len: SUCCEED; SYS_ERROR: failed and log control; others: failed
*/
STATIC ssize_t LogIamOpsWrite(struct IAMMgrFile *file, const char *buf, size_t len, loff_t *pos)
{
    (void)file;
    (void)pos;
    unsigned int i;
    const IamLogBuf *iamLogBuf = (const IamLogBuf *)buf;
    if ((iamLogBuf == NULL) || (iamLogBuf->nodeCount > LOG_MSG_NODE_NUM) || (len <= 0)) {
        return SYS_OK;
    }
    LOCK_WARN_LOG(&g_listMutex);
    for (i = 0; i < iamLogBuf->nodeCount; i++) {
        if (g_bufListNodeNum > MAX_BUFNODE_NUM) {
            SELF_LOG_WARN("buf list node num(%d) is more than %d, log loss...", g_bufListNodeNum, MAX_BUFNODE_NUM);
            UNLOCK_WARN_LOG(&g_listMutex);
            return SYS_ERROR;
        }
        const LogMsg *logMsg = (const LogMsg *)&iamLogBuf->logMsgList[i];
        if ((logMsg == NULL) || (logMsg->msgLength >= MSG_LENGTH) || (logMsg->msgLength == 0)) {
            SELF_LOG_WARN("Invalid input, buff is null or illegal len(%u).", len);
            continue;
        }
        BufList *bufList = (BufList *)malloc(sizeof(BufList));
        if (bufList == NULL) {
            SELF_LOG_WARN("malloc failed, strerr=%s.", strerror(ToolGetErrorCode()));
            continue;
        }
        (void)memset_s(bufList, sizeof(BufList), 0, sizeof(BufList));
        bufList->len = logMsg->msgLength;
        bufList->type = logMsg->type;
        bufList->next = NULL;

        int ret = strncpy_s(bufList->buf, MSG_LENGTH, logMsg->msg, logMsg->msgLength);
        if (ret != EOK) {
            SELF_LOG_WARN("strncpy_s failed, strerr=%s.", strerror(ToolGetErrorCode()));
            XFREE(bufList);
            continue;
        }
        if (g_bufHead == NULL) {
            g_bufHead = bufList;
        } else {
            g_bufTail->next = bufList;
        }
        g_bufTail = bufList;
        g_bufListNodeNum++;
    }
    UNLOCK_WARN_LOG(&g_listMutex);
    return len;
}

STATIC int LogIamOpsIoctlGetLevel(LevelConfInfo *levelConfInfo)
{
    LogRt ret;
    char configValue[CONF_VALUE_MAX_LEN + 1] = { 0 };
    ONE_ACT_WARN_LOG(levelConfInfo == NULL, return SYS_ERROR, "levelConfInfo invalid.");
    // global level or event value
    if ((strcmp(levelConfInfo->configName, GLOBALLEVEL_KEY) == 0) ||
        (strcmp(levelConfInfo->configName, ENABLEEVENT_KEY) == 0)) {
        ret = GetConfValueByList(levelConfInfo->configName, strlen(levelConfInfo->configName),
                                 configValue, CONF_VALUE_MAX_LEN);
        if (ret != SUCCESS) {
            levelConfInfo->configValue[0] = LOG_MAX_LEVEL + 1;
            return SYS_OK;
        }
        if (IsNaturalNumStr(configValue) == false) {
            return SYS_ERROR;
        }
        levelConfInfo->configValue[0] = atoi(configValue);
    }
    if (strcmp(levelConfInfo->configName, IOCTL_MODULE_NAME) == 0) {
        const ModuleInfo *moduleInfo = GetModuleInfos();
        for (; moduleInfo->moduleName != NULL; moduleInfo++) {
            if ((moduleInfo->moduleId < 0) || (moduleInfo->moduleId >= INVLID_MOUDLE_ID)) {
                continue;
            }
            (void)memset_s(configValue, sizeof(configValue), 0, sizeof(configValue));
            ret = GetConfValueByList(moduleInfo->moduleName, strlen(moduleInfo->moduleName),
                                     configValue, CONF_VALUE_MAX_LEN);
            if ((ret != SUCCESS) || (IsNaturalNumStr(configValue) == false)) {
                levelConfInfo->configValue[moduleInfo->moduleId] = LOG_MAX_LEVEL + 1;
                continue;
            }
            levelConfInfo->configValue[moduleInfo->moduleId] = atoi(configValue);
        }
    }
    return SYS_OK;
}

STATIC int LogIamOpsIoctl(struct IAMMgrFile *file, unsigned cmd, struct IAMIoctlArg *arg)
{
    (void)file;
    const FlushInfo *flushInfo = NULL;
    LevelConfInfo *levelConfInfo = NULL;
    int ret = SYS_OK;
    switch (cmd) {
        case CMD_FLUSH_LOG:
            flushInfo = (FlushInfo *)(arg->argData);
            ONE_ACT_WARN_LOG(flushInfo == NULL, return SYS_ERROR, "flushInfo invalid.");
            FflushLogDataBuf();
            SyncAllToDisk(flushInfo->appPid);
            break;
        case CMD_GET_LEVEL:
            levelConfInfo = (LevelConfInfo *)(arg->argData);
            ret = LogIamOpsIoctlGetLevel(levelConfInfo);
            break;
        default:
            break;
    }
    return ret;
}

STATIC int LogIamOpsOpen(struct IAMMgrFile *file)
{
    (void)file;
    return SYS_OK;
}

STATIC int LogIamOpsClose(struct IAMMgrFile *file)
{
    (void)file;
    return SYS_OK;
}

static struct IAMFileOps g_logIamOps = {
    .read = LogIamOpsRead,
    .write = LogIamOpsWrite,
    .ioctl = LogIamOpsIoctl,
    .open = LogIamOpsOpen,
    .close = LogIamOpsClose,
};

STATIC int RegisterIamService(const char *serviceName, const int serviceNameLen, const struct IAMFileOps *ops)
{
    int retry = 0;
    int ret = SYS_ERROR;
    if ((serviceName == NULL) || (ops == NULL) || (serviceNameLen < 0) ||
        (serviceNameLen > IAM_SERVICE_NAME_MAX_LENGTH)) {
        SELF_LOG_WARN("Input invalid, serviceNameLen=%d", serviceNameLen);
        return SYS_ERROR;
    }
    struct IAMFileConfig iamFileConfig;
    iamFileConfig.serviceName = serviceName;
    iamFileConfig.ops = ops;
    iamFileConfig.timeOut = 50; // timeout 50ms for read/write/ioctl/close except open(use iam default timeout)
    while ((ret != SYS_OK) && (retry < IAM_RETRY_TIMES)) {
        ret = IAMRegisterService(&iamFileConfig);
        retry++;
    }
    return (ret == SYS_OK) ? SYS_OK : SYS_ERROR;
}

STATIC int RegisterIamTotalService(void)
{
    int ret = RegisterIamService(LOGOUT_IAM_SERVICE_PATH, strlen(LOGOUT_IAM_SERVICE_PATH), &g_logIamOps);
    if (ret != SYS_OK) {
        SELF_LOG_WARN("Iam register %s failed, strerr=%s.", LOGOUT_IAM_SERVICE_PATH, strerror(ToolGetErrorCode()));
        return SYS_ERROR;
    }

    return SYS_OK;
}

void InitModuleDefault(const struct Options *opt)
{
    if ((opt != NULL) && (opt->l != 0)) {
        g_logLevel.globalLevel = opt->l;
    }
}

STATIC void SyslogCleanUp(void)
{
    FreeBufAndFileList();
}

STATIC int SyslogInit(void)
{
    if (InitBufAndFileList() != SUCCESS) {
        SELF_LOG_ERROR("init LOG buffer failed, strerr=%s.", strerror(ToolGetErrorCode()));
        return SYS_ERROR;
    }
    if (RegisterIamTotalService() != SYS_OK) {
        SELF_LOG_ERROR("Register Iam service failed");
        SyslogCleanUp();
        return SYS_ERROR;
    }
    SELF_LOG_INFO("Iam register total service succeed");
    return SYS_OK;
}

STATIC void SyncOsLogToDisk(const StSubLogFileList *pstSubInfo)
{
    ONE_ACT_NO_LOG(pstSubInfo == NULL, return);
    if (pstSubInfo->fileNum == 0) {
        return;
    }
    char fileName[MAX_FILEPATH_LEN + MAX_FILENAME_LEN + 1] = "";
    unsigned int ret = FilePathSplice(pstSubInfo, fileName, MAX_FILEPATH_LEN + MAX_FILENAME_LEN);
    ONE_ACT_NO_LOG(ret != OK, return);
    FsyncLogToDisk(fileName);
    SELF_LOG_INFO("Fsync OS Log %s To Disk.", fileName);
}

STATIC int AppLogFlushFilter(const ToolDirent *dir)
{
    ONE_ACT_NO_LOG(dir == NULL, return FILTER_NOK);
    if (StartsWith(dir->d_name, DEVICE_APP_HEAD) != 0) {
        return FILTER_OK;
    }
    return FILTER_NOK;
}

STATIC void SyncAppLogToDisk(const char* path, int appPid)
{
    ONE_ACT_NO_LOG(path == NULL, return);
    char appLogPath[MAX_PATH_NAME_LEN + 1] = { 0 };
    char appLogFileName[MAX_APP_FILEPATH_LEN + 1] = { 0 };
    const char* sortDirName[LOG_TYPE_NUM] = { DEBUG_DIR_NAME, SECURITY_DIR_NAME, RUN_DIR_NAME, OPERATION_DIR_NAME };
    int logType = DEBUG_LOG;
    for (; logType < (int)LOG_TYPE_NUM; logType++) {
        (void)memset_s(appLogPath, MAX_PATH_NAME_LEN + 1, 0, MAX_PATH_NAME_LEN + 1);
        (void)memset_s(appLogFileName, MAX_APP_FILEPATH_LEN + 1, 0, MAX_APP_FILEPATH_LEN + 1);
        int ret = snprintf_s(appLogPath, MAX_PATH_NAME_LEN + 1, MAX_PATH_NAME_LEN, "%s%s%s%s%s%u", path,
                             FILE_SEPERATOR, sortDirName[logType], FILE_SEPERATOR, DEVICE_APP_HEAD, appPid);
        ONE_ACT_WARN_LOG(ret == -1, continue, "get applog path failed, pid=%d, result=%d, strerr=%s.",
                         appPid, ret, strerror(ToolGetErrorCode()));
        if (ToolAccess(appLogPath) != SYS_OK) {
            continue;
        }
        ToolDirent **namelist = NULL;
        int toatalNum = ToolScandir((const char *)appLogPath, &namelist, AppLogFlushFilter, alphasort);
        if ((toatalNum <= 0) || (namelist == NULL)) {
            continue;
        }
        ret = snprintf_s(appLogFileName, MAX_APP_FILEPATH_LEN + 1, MAX_APP_FILEPATH_LEN, "%s/%s",
                         appLogPath, namelist[toatalNum - 1]->d_name);
        if (ret == -1) {
            ToolScandirFree(namelist, toatalNum);
            SELF_LOG_INFO("snprintf failed, strerr=%s.", strerror(ToolGetErrorCode()));
            continue;
        }
        FsyncLogToDisk(appLogFileName);
        ToolScandirFree(namelist, toatalNum);
        SELF_LOG_INFO("Fsync App Log %s To Disk.", appLogFileName);
    }
}

STATIC void SyncAllToDisk(int appPid)
{
    DataBufLock();
    const StLogFileList *fileList = GetGlobalLogFileList();
    if (fileList == NULL) {
        DataBufUnLock();
        return;
    }
    if (appPid != INVALID) {
        SyncAppLogToDisk(fileList->aucFilePath, appPid);
    } else {
        SyncOsLogToDisk(&(fileList->sortDeviceOsLogList[DEBUG_LOG]));
        SyncOsLogToDisk(&(fileList->sortDeviceOsLogList[SECURITY_LOG]));
        SyncOsLogToDisk(&(fileList->sortDeviceOsLogList[RUN_LOG]));
        SyncOsLogToDisk(&(fileList->sortDeviceOsLogList[OPERATION_LOG]));
    }
    DataBufUnLock();
}

STATIC void *WriteLogBufToFile(ArgPtr arg)
{
    (void)arg;
    if (SetThreadName("writeLogToFile") != 0) {
        SELF_LOG_WARN("set thread_name(writeLogToFile) failed.");
    }
    while (g_gotSignal == 0) {
        FflushLogDataBuf();
        (void)ToolSleep(ONE_SECOND);
    }
    FflushLogDataBuf();
    SELF_LOG_ERROR("Thread(writeLogToFile) quit, signal=%d.", g_gotSignal);
    SetWriteFileThreadExit();
    return (void *)NULL;
}

void ProcSyslogd(void)
{
    // Set up signal handlers (so that they interrupt read())
    SignalNoSaRestartEmptyMask(SIGTERM, RecordSigNo);
    SignalNoSaRestartEmptyMask(SIGINT, RecordSigNo);
    (void)signal(SIGHUP, SIG_IGN);
    (void)signal(SIGPIPE, SIG_IGN);

    int ret = SyslogInit();
    if (ret == SYS_ERROR) {
        SELF_LOG_ERROR("[FATAL] init slogd failed, quit slogd process...");
        return;
    }

    // threadstack no optimize, use default (thread in device Performance priority)
    toolThread tid = 0;
    g_writeFileThread.pulArg = NULL;
    g_writeFileThread.procFunc = WriteLogBufToFile;
    if (ToolCreateTaskWithDetach(&tid, &g_writeFileThread) != SYS_OK) {
        SELF_LOG_ERROR("[FATAL] create thread failed, strerr=%s, quit slogd process...", strerror(ToolGetErrorCode()));
        SyslogCleanUp();
        return;
    }
    char *recvBuf = g_globalsPtr->recvBuf;
    int logType = DEBUG_LOG;
    while (g_gotSignal == 0) {
        int sz = IamRead(recvBuf, MAX_READ - 1, &logType);
        TWO_ACT_NO_LOG(sz == SYS_INVALID_PARAM, (void)ToolSleep(10), continue); // sleep 10ms
        if (ProcSyslogBuf(recvBuf, &sz) != SYS_OK) {
            continue;
        }
        recvBuf[sz] = '\0';
        if ((logType >= 0) && (logType < LOG_TYPE_NUM)) {
            ProcEscapeThenLog(recvBuf, sz, (LogType)logType);
        }
    }
    // wait until thread exit
    while (IsWriteFileThreadExit() == 0) {
        (void)ToolSleep(FIFTY_MILLI_SECOND);
    }

    // main process exit and resource free
    SyslogCleanUp();
    SELF_LOG_ERROR("signal=%d, quit slogd process...", g_gotSignal);
}