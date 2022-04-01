/**
 * @msnpureport.c
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#include "msnpureport.h"
#define DEFAULT_DETACH_THREAD_ATTR { 0, 0, 0, 0, 0, 1, 128 * 1024 } // Default ThreadSize(128KB), joinable
#define INVALID_ARG (-1)

#define XFREE(ps) do {  \
    if ((ps) != NULL) { \
        free((ps));     \
        (ps) = NULL;    \
    }                   \
} while (0)

typedef struct {
    mmUserBlock_t block;
    mmThread tid;
} ThreadInfo;

const mmStructOption g_opts[] = {
    {"help",    MMPA_NO_ARGUMENT,       NULL, RPT_ARGS_HELP},
    {"global",  MMPA_REQUIRED_ARGUMENT, NULL, RPT_ARGS_GLOABLE_LEVEL},
    {"module",  MMPA_REQUIRED_ARGUMENT, NULL, RPT_ARGS_MODULE_LEVEL},
    {"event",   MMPA_REQUIRED_ARGUMENT, NULL, RPT_ARGS_EVENT_LEVEL},
    {"type",    MMPA_REQUIRED_ARGUMENT, NULL, RPT_ARGS_LOG_TYPE},
    {"device",  MMPA_REQUIRED_ARGUMENT, NULL, RPT_ARGS_DEVICE_ID},
    {"request", MMPA_NO_ARGUMENT,       NULL, RPT_ARGS_REQUEST_LOG_LEVEL},
    {"all",     MMPA_NO_ARGUMENT,       NULL, RPT_ARGS_EXPORT_BBOX_LOGS_DEVICE_EVENT},
    {"force",   MMPA_NO_ARGUMENT,       NULL, RPT_ARGS_EXPORT_BBOX_LOGS_DEVICE_EVENT_MAINTENANCE_INFORMATION}
};

static int MkdirIfNotExist(const char *path);
STATIC int SetGlobalLevel(ArgInfo *opts);
STATIC int SetModuleLevel(ArgInfo *opts);
STATIC int SetEventLevel(ArgInfo *opts);
STATIC int SetLogType(ArgInfo *opts);
STATIC int SetDeviceId(ArgInfo *opts);
STATIC int RequestLevel(ArgInfo *opts);
STATIC int ExportAllBboxLogs(ArgInfo *opts);
STATIC int ForceExportBboxLogs(ArgInfo *opts);
STATIC int GetHelpInfo(ArgInfo *opts);

static char g_logPath[MMPA_MAX_PATH + 1] = { 0 };
static ThreadInfo g_slogThread[MAX_DEV_NUM];
static ThreadArgInfo g_threadArgInfo[MAX_DEV_NUM];
typedef int (*DataRetriver) (ArgInfo *opts);
DataRetriver dataRetriver[9] = {SetGlobalLevel, SetModuleLevel, SetEventLevel, SetLogType,
                                SetDeviceId, RequestLevel, ExportAllBboxLogs, ForceExportBboxLogs, GetHelpInfo};
static MsnArg g_msnArg[9] = {{'g', 0}, {'m', 1}, {'e', 2}, {'t', 3}, {'d', 4}, {'r', 5}, {'a', 6}, {'f', 7}, {'h', 8}};


/**
 * @brief GetSpecificLogs: get specific logs
 * @param [in]threadArgInfo: thread arg info
 * @param [in]fullPath: splice g_logPath and logPath eg: 2021-12-30-03-19-54/message
 * @param [in]logicId: logicId
 * @param [in]logType: logType
 * @param [in]logPath: logPath
 * @return EN_OK/EN_ERROR
 */
static int GetSpecificLogs(ThreadArgInfo *threadArgInfo, char *fullPath, uint32_t logicId, int logType, char *logPath)
{
    if (threadArgInfo->logType == ALL_LOG || threadArgInfo->logType == logType) {
        (void)memset_s(fullPath, MMPA_MAX_PATH + 1, 0, MMPA_MAX_PATH + 1);
        CHK_EXPR_CTRL(snprintf_s(fullPath, MMPA_MAX_PATH + 1, MMPA_MAX_PATH,
                      "%s/%s/dev-os-%u", g_logPath, logPath, logicId) == -1, return EN_ERROR,
                      "copy path failed, dev_id=%u, strerr=%s.\n", logicId, strerror(mmGetErrorCode()));
        CHK_EXPR_ACT(MkdirIfNotExist((const char *)fullPath) != EN_OK, return EN_ERROR);
        int ret = AdxGetDeviceFile((unsigned short)(threadArgInfo->devOsId), (const char *)fullPath, logPath);
        if (ret != 0) {
            if (ret == BLOCK_RETURN_CODE) {
                printf("non-docker check failed, msnpureport does not support to be used in docker.\n");
            } else {
                printf("get device files failed by adx, dev_id=%ld.\n", threadArgInfo->devOsId);
            }
            return EN_ERROR;
        }
    }
    return EN_OK;
}

static void* SlogSyncThread(void *arg)
{
    CHK_EXPR_CTRL(arg == NULL, return NULL, "arg of SlogSyncThread is null.\n");
    ThreadArgInfo *threadArgInfo = (ThreadArgInfo *)arg;

    // user only known logic Id
    uint32_t logicId = 0;
    CHK_EXPR_CTRL(drvDeviceGetIndexByPhyId((uint32_t)threadArgInfo->devOsId, &logicId) != DRV_ERROR_NONE, return NULL,
                  "get logic id failed, dev_id=%ld.\n", threadArgInfo->devOsId);
    char *fullPath = (char *)calloc(1, MMPA_MAX_PATH + 1);
    CHK_EXPR_CTRL(fullPath == NULL, return NULL, "calloc failed, dev_id=%ld, strerr=%s.\n",
                  threadArgInfo->devOsId, strerror(mmGetErrorCode()));
    do {
        // create slog path and receive slog files
        if (GetSpecificLogs(threadArgInfo, fullPath, logicId, SLOG_LOG, SLOG_PATH) != EN_OK) {
            break;
        }
        // create stackcore path and receive stackcore files
        if (GetSpecificLogs(threadArgInfo, fullPath, logicId, STACKCORE_LOG, STACKCORE_PATH) != EN_OK) {
            break;
        }
        // create message path and receive message files
        if (GetSpecificLogs(threadArgInfo, fullPath, logicId, SLOG_LOG, MESSAGE_PATH) != EN_OK) {
            break;
        }
    } while (0);

    XFREE(fullPath);
    return NULL;
}

/**
 * @brief GetDevIDs: get device num and device ids
 * @param [in/out]devNum: device number
 * @param [in/out]idArray: device id array
 * @return EN_OK/EN_ERROR
 */
static int GetDevIDs(unsigned int *devNum, unsigned int *idArray)
{
    int err = drvGetDevNum(devNum);
    if (err != DRV_ERROR_NONE || *devNum > MAX_DEV_NUM) {
        printf("get device num failed, ret=%d.\n", err);
        return EN_ERROR;
    }
    unsigned int i;
    for (i = 0; i < *devNum; i++) {
        idArray[i] = i;
    }
    return EN_OK;
}

/**
 * @brief SlogStartSyncFile: start thread to sync device log files
 * @param [in]logType: log type
 * @return EN_OK/EN_ERROR
 */
static int SlogStartSyncFile(int logType)
{
    mmThreadAttr threadAttr = DEFAULT_DETACH_THREAD_ATTR;
    (void)memset_s(g_slogThread, MAX_DEV_NUM * sizeof(ThreadInfo), 0, MAX_DEV_NUM * sizeof(ThreadInfo));

    unsigned int devNum = 0;
    unsigned int devIds[MAX_DEV_NUM] = { 0 };
    int ret = GetDevIDs(&devNum, devIds);
    if (ret != EN_OK) {
        return EN_ERROR;
    }

    bool devFlag[MAX_DEV_NUM] = { false };
    unsigned int phyId = 0;
    long devOsId = 0;
    unsigned int i;
    for (i = 0; i < devNum; i++) {
        if (devIds[i] >= MAX_DEV_NUM) {
            continue;
        }
        ret = drvDeviceGetPhyIdByIndex(devIds[i], &phyId);
        if (ret != DRV_ERROR_NONE || phyId >= MAX_DEV_NUM) {
            printf("get pyhsical id failed, dev_id=%u, ret=%d.\n", devIds[i], ret);
            continue;
        }
        ret = halGetDeviceInfo(phyId, MODULE_TYPE_SYSTEM, INFO_TYPE_MASTERID, &devOsId);
        if (ret != DRV_ERROR_NONE || devOsId < 0 || devOsId >= MAX_DEV_NUM) {
            printf("get device os id failed, dev_id=%u, phy_id=%u, ret=%d.\n", devIds[i], phyId, ret);
            continue;
        }

        if (devFlag[devOsId] == true) {
            continue;
        }

        g_slogThread[devOsId].block.procFunc = SlogSyncThread;
        g_threadArgInfo[devOsId].devOsId = devOsId;
        g_threadArgInfo[devOsId].logType = logType;
        g_slogThread[devOsId].block.pulArg = (void *)&g_threadArgInfo[devOsId];
        ret = mmCreateTaskWithThreadAttr(&g_slogThread[devOsId].tid, &g_slogThread[devOsId].block, &threadAttr);
        if (ret != EN_OK) {
            printf("create task(SlogSyncThread) failed, dev_id=%u, strerr=%s.\n",
                   devIds[i], strerror(mmGetErrorCode()));
            continue;
        }
        devFlag[devOsId] = true;
    }

    return EN_OK;
}

/**
 * @brief BboxStartSyncFile: start to sync bbox files
 * @param [in]opt: bbox dump opt
 * @return EN_OK/EN_ERROR
 */
static int BboxStartSyncFile(struct BboxDumpOpt *opt)
{
    // start to sync bbox log
    int ret = BboxStartDump(g_logPath, strlen(g_logPath), opt);
    if (ret != 0) {
        printf("start bbox dump failed.\n");
        return EN_ERROR;
    }
    return EN_OK;
}

/**
 * @brief StartSyncDeviceLog: start thread to receive files from device
 * @param [in]opt: bbox dump opt
 * @param [in]logType: log type
 * @return void
 */
static void StartSyncDeviceLog(struct BboxDumpOpt *opt, int logType)
{
    if (SlogStartSyncFile(logType) != EN_OK) {
        printf("start to sync slog failed.\n");
    }
    if (logType == ALL_LOG || logType == HISILOGS_LOG) {
        if (BboxStartSyncFile(opt) != EN_OK) {
            printf("start to sync bbox dump failed.\n");
        }
    }
}

/**
 * @brief StopSyncDeviceLog: wait all thread stop
 * @param [in]logType: log type
 * @return void
 */
static void StopSyncDeviceLog(int logType)
{
    int ret;
    // wait slog sync thread stop
    unsigned int i;
    for (i = 0; i < MAX_DEV_NUM; i++) {
        if (g_slogThread[i].tid == 0) {
            continue;
        }
        ret = mmJoinTask(&g_slogThread[i].tid);
        if (ret != EN_OK) {
            printf("pthread join failed, dev_id=%u, strerr=%s.\n", i, strerror(mmGetErrorCode()));
        }
        g_slogThread[i].tid = 0;
    }
    // wait bbox sync thread stop
    if (logType == ALL_LOG || logType == HISILOGS_LOG) {
        BboxStopDump();
    }
}

/**
 * @brief MkdirIfNotExist: mkdir path
 * @param [in]path: log path
 * @return EN_OK/EN_ERROR
 */
static int MkdirIfNotExist(const char *path)
{
    if (mmAccess(path) != EN_OK) {
        if (mmMkdir(path, (mmMode_t)DIR_MODE) != EN_OK) {
            printf("mkdir %s failed, strerr=%s.\n", path, strerror(mmGetErrorCode()));
            return EN_ERROR;
        }
    }
    return EN_OK;
}

/**
 * @brief GetLocalTimeForPath: get current timestamp
 * @param [in]bufLen: time buffer size
 * @param [in/out]timeBuffer: timestamp buffer
 * @return EN_OK/EN_ERROR
 */
static int GetLocalTimeForPath(unsigned int bufLen, char *timeBuffer)
{
    mmTimeval currentTimeval = { 0 };
    struct tm timeInfo = { 0 };

    if (mmGetTimeOfDay(&currentTimeval, NULL) != EN_OK) {
        printf("get times failed.\n");
        return EN_ERROR;
    }
    const time_t sec = currentTimeval.tv_sec;
    if (mmLocalTimeR(&sec, &timeInfo) != EN_OK) {
        printf("get timestamp failed.\n");
        return EN_ERROR;
    }

    int err = snprintf_s(timeBuffer, bufLen, bufLen - 1, "%04d-%02d-%02d-%02d-%02d-%02d",
        timeInfo.tm_year, timeInfo.tm_mon, timeInfo.tm_mday,
        timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec);
    if (err == -1) {
        printf("copy time buffer failed, strerr=%s.\n", strerror(mmGetErrorCode()));
        return EN_ERROR;
    }
    return EN_OK;
}

/**
 * @brief CreateSubPath: mkdir sub path
 * @param [in]ppath: parent path
 * @param [in]subPath: ppath's sub path
 * @return EN_OK/EN_ERROR
 */
static int CreateSubPath(const char *ppath, const char *subPath)
{
    char *fullPath = (char *)calloc(1, MMPA_MAX_PATH + 1);
    if (fullPath == NULL) {
        printf("calloc failed, strerr=%s.\n", strerror(mmGetErrorCode()));
        return EN_ERROR;
    }

    bool flag = false;
    do {
        int ret = snprintf_s(fullPath, MMPA_MAX_PATH + 1, MMPA_MAX_PATH, "%s/%s", ppath, subPath);
        if (ret == -1) {
            printf("copy path failed, strerr=%s.\n", strerror(mmGetErrorCode()));
            break;
        }
        if (MkdirIfNotExist(fullPath) != EN_OK) {
            break;
        }
        flag = true;
    } while (0);

    XFREE(fullPath);
    return flag == true ? EN_OK : EN_ERROR;
}

/**
 * @brief CreateLogPath: mkdir slog path for device log syncing
 * @param [in]logType: log type
 * @return EN_OK/EN_ERROR
 */
static int CreateLogPath(int logType)
{
    char timer[TIME_SIZE + 1] = { 0 };
    if (GetLocalTimeForPath(TIME_SIZE, timer) != EN_OK) {
        return EN_ERROR;
    }
    char *currentPath = (char *)calloc(1, MMPA_MAX_PATH + 1);
    if (currentPath == NULL) {
        printf("calloc failed, strerr=%s.\n", strerror(mmGetErrorCode()));
        return EN_ERROR;
    }

    bool flag = false;
    do {
        if (mmGetCwd(currentPath, MMPA_MAX_PATH) != EN_OK) {
            printf("get current path failed, strerr=%s.\n", strerror(mmGetErrorCode()));
            break;
        }

        // mkdir timestamp path
        if (CreateSubPath((const char *)currentPath, (const char *)timer) != EN_OK) {
            break;
        }

        int ret = snprintf_s(g_logPath, MMPA_MAX_PATH + 1, MMPA_MAX_PATH, "%s/%s", currentPath, timer);
        if (ret == -1) {
            printf("copy path failed, strerr=%s.\n", strerror(mmGetErrorCode()));
            break;
        }
        if (logType == ALL_LOG || logType == STACKCORE_LOG) {
            if (CreateSubPath((const char *)g_logPath, STACKCORE_PATH) != EN_OK) {
                break;
            }
        }
        if (logType == ALL_LOG || logType == SLOG_LOG) {
            if (CreateSubPath((const char *)g_logPath, SLOG_PATH) != EN_OK ||
                CreateSubPath((const char *)g_logPath, MESSAGE_PATH) != EN_OK) {
                break;
            }
        }
        flag = true;
    } while (0);

    XFREE(currentPath);
    return flag == true ? EN_OK : EN_ERROR;
}

STATIC void OptionsUsage(void)
{
    fprintf(stdout,
        "Usage:\tmsnpureport\n"
         "  or:\tmsnpureport[OPTIONS]\n"
        "Example:\n"
        "\tmsnpureport                     Export device log files, bbox log files.\n"
        "\tmsnpureport -a                  Export device log files, bbox log files, device event information.\n"
        "\tmsnpureport -f                  Export device log files, bbox log files, device event information, \
historical maintenance and measurement information in the storage space.\n"
        "\tmsnpureport -g info             The device (which device id is 0) global log level options to info.\n"
        "\tmsnpureport -m FMK:debug -d 1   The device (which device id is 1) FMK module log level options to debug.\n"
        "\tmsnpureport -t 1                Export slog and message logs.\n"
        "\tmsnpureport -r                  Get log level.\n"
        "Options:\n"
        "  -g, --global            Global log level: error, info, warning, debug, null\n"
        "  -m, --module            Module log level: error, info, warning, debug, null\n"
        "  -e, --event             Event log level: enable, disable\n"
        "  -t, --type              Log type: 0(all log, default is 0), 1(slog message), 2(hisi_logs), 3(stackcore)\n"
        "  -d, --device            Device ID, default is 0, can use with -g or -m or -e or -r\n"
        "  -r, --request           Get log level\n"
        "  -a, --all               Export device log files, bbox log files, device event information.\n"
        "  -f, --force             Export device log files, bbox log files, device event information, \
historical maintenance and measurement information in the storage space.\n"
        "  -h, --help              Help.\n");
}

/**
 * @brief check whether the string only has numbers
 * @param [in]val: number string
 * @return  EN_OK is bumber, EN_ERROR is not a number string
 */
STATIC int IsNumStr(const char *val)
{
    CHK_NULL_PTR(val, return EN_ERROR);
    while (*val != '\0') {
        if (*val > '9' || *val < '0') {
            return EN_ERROR;
        }
        val++;
    }
    return EN_OK;
}
/**
 * @brief init opts->level by option values
 * @param [in] args: option values from cmdline input
 * @param [out] opts: structure to store option values
 * @return EN_OK/EN_ERROR
 */
STATIC int InitArgInfoLevel(ArgInfo *opts, const char *args)
{
    CHK_NULL_PTR(args, return EN_ERROR);
    CHK_NULL_PTR(opts, return EN_ERROR);
    CHK_EXPR_CTRL(strlen(opts->level) > 0, return EN_ERROR,
        "Only support one option: -g or -m or -e or -r or -a or -f, you can execute '--help' for example value.\n");
    int ret = strcpy_s(opts->level, MAX_LEVEL_STR_LEN, args);
    CHK_EXPR_CTRL(ret != 0, return EN_ERROR, "The opts level(%s) init failed!\n", args);
    return EN_OK;
}

/**
 * @brief init opts->level by no parameter option
 * @param [out] opts: structure to store option values
 * @return EN_OK/EN_ERROR
 */
int InitArgInfoLevelByNoParameterOption(ArgInfo *opts)
{
    CHK_NULL_PTR(opts, return EN_ERROR);
    CHK_EXPR_CTRL(strlen(opts->level) > 0, return EN_ERROR,
                  "Only support one option: -g or -m or -e or -t or -r or -a or -f, \
                  you can execute '--help' for example value.\n");
    int ret = strcpy_s(opts->level, MAX_LEVEL_STR_LEN, "raftFlag");
    CHK_EXPR_CTRL(ret != 0, return EN_ERROR, "The opts init failed!\n");
    return EN_OK;
}

/**
 * @brief convert the input string
 * @param [in/out] str: string
 * @return void
 */
STATIC int ToUpper(char *str)
{
    CHK_NULL_PTR(str, return EN_ERROR);
    char *ptr = str;
    while (*ptr != '\0') {
        if ((*ptr <= 'z') && (*ptr >= 'a')) {
            *ptr = 'A' - 'a' + *ptr;
        }
        ptr++;
    }
    return EN_OK;
}

/**
 * @brief check whether the global level of cmdline input valid
 * @param [in/out]levelStr: cmdline input level
 * @return EN_ERROR: invalid, EN_OK: valid
 */
STATIC int CheckGlobalLevel(char *levelStr, size_t len)
{
    CHK_NULL_PTR(levelStr, return EN_ERROR);
    CHK_EXPR_ACT(strlen(levelStr) >= len, return EN_ERROR);
    CHK_EXPR_ACT(ToUpper(levelStr) != EN_OK, return EN_ERROR);
    const ModuleInfo *levelInfo = GetLevelInfoByName((const char *)levelStr);
    CHK_EXPR_CTRL(levelInfo == NULL || levelInfo->moduleLevel < LOG_MIN_LEVEL ||
        levelInfo->moduleLevel > LOG_MAX_LEVEL, return EN_ERROR,
        "Global level is invalid, str=%s. You can execute '--help' for example value.\n", levelStr);
    return EN_OK;
}

/**
 * @brief check whether the module level and module name of cmdline input valid
 * @param [in/out]levelStr: cmdline input level
 * @return EN_ERROR: invalid, EN_OK: valid
 */
STATIC int CheckModuleLevel(char *levelStr, size_t len)
{
    CHK_NULL_PTR(levelStr, return EN_ERROR);
    CHK_EXPR_ACT(strlen(levelStr) >= len, return EN_ERROR);
    char modName[MAX_MOD_NAME_LEN] = { 0 };
    char *pend = strchr(levelStr, ':');
    CHK_EXPR_CTRL(pend == NULL, return EN_ERROR, \
        "module level info has no ':', level_string=%s.\n", levelStr);
    int retValue = memcpy_s(modName, MAX_MOD_NAME_LEN, levelStr, pend - levelStr);
    CHK_EXPR_CTRL(retValue != EOK, return EN_ERROR, \
        "memcpy_s failed, result=%d, strerr=%s.\n", retValue, strerror(mmGetErrorCode()));
    CHK_EXPR_ACT(ToUpper(modName) != EN_OK, return EN_ERROR);
    const ModuleInfo *moduleInfo = GetModuleInfoByName((const char *)modName);
    CHK_EXPR_CTRL(moduleInfo == NULL, return EN_ERROR, \
        "module name does not exist, module=%s.\n", modName);

    levelStr = pend + 1;
    CHK_EXPR_ACT(ToUpper(levelStr) != EN_OK, return EN_ERROR);
    const ModuleInfo *levelInfo = GetLevelInfoByName((const char *)levelStr);
    CHK_EXPR_CTRL(levelInfo == NULL || levelInfo->moduleLevel < LOG_MIN_LEVEL ||
        levelInfo->moduleLevel > LOG_MAX_LEVEL, return EN_ERROR,
        "module level is invalid, level_string=%s.\n", levelStr);
    return EN_OK;
}

/**
 * @brief check whether the event level of cmdline input valid
 * @param [in/out]levelStr: cmdline input level
 * @return EN_ERROR: invalid, EN_OK: valid
 */
STATIC int CheckEventLevel(char *levelStr, size_t len)
{
    CHK_NULL_PTR(levelStr, return EN_ERROR);
    CHK_EXPR_ACT(strlen(levelStr) >= len, return EN_ERROR);
    CHK_EXPR_ACT(ToUpper(levelStr) != EN_OK, return EN_ERROR);
    int levelValue = (strcmp(EVENT_ENABLE, levelStr) == 0) ? 1 : (strcmp(EVENT_DISABLE, levelStr) == 0 ? 0 : -1);
    if (levelValue == -1) {
        printf("Event level is invalid, level=%s.\n", levelStr);
        return EN_ERROR;
    }
    return EN_OK;
}

/**
 * @brief check whether the log type is valid
 * @param [in/out]logType: log type
 * @return
 */
STATIC int CheckLogType(const char *args, size_t len)
{
    CHK_EXPR_CTRL((strlen(args) > len || (IsNumStr(args) != EN_OK)),
                  return EN_ERROR, "Invalid logType(%s), please execute '--help'.\n", args);
    unsigned int logType = atoi(args);
    if (logType > STACKCORE_LOG) {
        printf("invalid log type.\n");
        return EN_ERROR;
    }
    return EN_OK;
}

/**
 * @brief SetGlobalLevel
 * @param [in/out][in]opts: arg info
 * @return EN_ERROR: failed EN_OK: success
 */
STATIC int SetGlobalLevel(ArgInfo *opts)
{
    CHK_EXPR_ACT(InitArgInfoLevel(opts, mmGetOptArg()) != EN_OK, return EN_ERROR);
    CHK_EXPR_CTRL(CheckGlobalLevel(opts->level, MAX_LEVEL_STR_LEN) != EN_OK, return EN_ERROR,
                  "The level information is illegal, you can execute '--help' for example value.\n");
    opts->logLevelType = LOGLEVEL_GLOBAL;
    return EN_OK;
}

/**
 * @brief SetModuleLevel
 * @param [in/out][in]opts: arg info
 * @return EN_ERROR: failed EN_OK: success
 */
STATIC int SetModuleLevel(ArgInfo *opts)
{
    CHK_EXPR_ACT(InitArgInfoLevel(opts, mmGetOptArg()) != EN_OK, return EN_ERROR);
    CHK_EXPR_CTRL(CheckModuleLevel(opts->level, MAX_LEVEL_STR_LEN) != EN_OK, return EN_ERROR,
                  "The level information is illegal, you can execute '--help' for example value.\n");
    opts->logLevelType = LOGLEVEL_MODULE;
    return EN_OK;
}

/**
 * @brief SetEventLevel
 * @param [in/out][in]opts: arg info
 * @return EN_ERROR: failed EN_OK: success
 */
STATIC int SetEventLevel(ArgInfo *opts)
{
    CHK_EXPR_ACT(InitArgInfoLevel(opts, mmGetOptArg()) != EN_OK, return EN_ERROR);
    CHK_EXPR_CTRL(CheckEventLevel(opts->level, MAX_LEVEL_STR_LEN) != EN_OK, return EN_ERROR,
                  "The level information is illegal, you can execute '--help' for example value.\n");
    opts->logLevelType = LOGLEVEL_EVENT;
    return EN_OK;
}

/**
 * @brief SetLogType
 * @param [in/out][in]opts: arg info
 * @return EN_ERROR: failed EN_OK: success
 */
STATIC int SetLogType(ArgInfo *opts)
{
    CHK_NULL_PTR(mmGetOptArg(), return EN_ERROR);
    CHK_EXPR_ACT(InitArgInfoLevelByNoParameterOption(opts) != EN_OK, return EN_ERROR);
    CHK_EXPR_CTRL(CheckLogType(mmGetOptArg(), LOG_TYPE_LEN) != EN_OK, return EN_ERROR,
                  "The log type information is illegal, you can execute '--help' for example value.\n");
    opts->logType = atoi(mmGetOptArg());
    opts->logOperationType = EXPORT_SPECIFIC_LOG;
    return EN_OK;
}

/**
 * @brief SetDeviceId
 * @param [in/out][in]opts: arg info
 * @return EN_ERROR: failed EN_OK: success
 */
STATIC int SetDeviceId(ArgInfo *opts)
{
    CHK_NULL_PTR(mmGetOptArg(), return EN_ERROR);
    CHK_EXPR_CTRL((strlen(mmGetOptArg()) > DEVICE_LEN || (IsNumStr(mmGetOptArg()) != EN_OK)),
                  return EN_ERROR, "Invalid device(%s), please execute '--help'.\n.", mmGetOptArg());
    opts->deviceId = atoi(mmGetOptArg());
    opts->deviceIdFlag = 1;
    return EN_OK;
}

/**
 * @brief RequestLevel
 * @param [in/out][in]opts: arg info
 * @return EN_ERROR: failed EN_OK: success
 */
STATIC int RequestLevel(ArgInfo *opts)
{
    CHK_EXPR_ACT(InitArgInfoLevelByNoParameterOption(opts) != EN_OK, return EN_ERROR);
    opts->logOperationType = GET_LOGLEVEL;
    return EN_OK;
}

/**
 * @brief ExportAllBboxLogs
 * @param [in/out][in]opts: arg info
 * @return EN_ERROR: failed EN_OK: success
 */
STATIC int ExportAllBboxLogs(ArgInfo *opts)
{
    CHK_EXPR_ACT(InitArgInfoLevelByNoParameterOption(opts) != EN_OK, return EN_ERROR);
    opts->logOperationType = EXPORT_BBOX_LOGS_DEVICE_EVENT;
    return EN_OK;
}

/**
 * @brief ForceExportBboxLogs
 * @param [in/out][in]opts: arg info
 * @return EN_ERROR: failed EN_OK: success
 */
STATIC int ForceExportBboxLogs(ArgInfo *opts)
{
    CHK_EXPR_ACT(InitArgInfoLevelByNoParameterOption(opts) != EN_OK, return EN_ERROR);
    opts->logOperationType = EXPORT_BBOX_LOGS_DEVICE_EVENT_MAINTENANCE_INFORMATION;
    return EN_OK;
}

/**
 * @brief GetHelpInfo
 * @param [in/out][in]opts: arg info
 * @return EN_ERROR: failed EN_OK: success
 */
STATIC int GetHelpInfo(ArgInfo *opts)
{
    (void)opts;
    OptionsUsage();
    return EN_INVALID_PARAM;
}

/**
 * @brief  GetInputArg
 * @param [in/out][in]opts: return value of mmGetOptLong
 * @return arg
 */
static int GetInputArg(int opt)
{
    int len = sizeof(g_msnArg) / sizeof(g_msnArg[0]);
    int i = 0;
    for (; i < len; ++i) {
        if (opt == g_msnArg[i].arg) {
            return g_msnArg[i].index;
        }
    }
    return INVALID_ARG;
}

/**
 * @brief InitArgInfo
 * @param [in/out][in]opt: return value of mmGetOptLong
 * @param [in/out][in]opts: arg info
 * @param [in/out][in]argc: argc
 * @param [in/out][in]argv: argv
 * @return EN_ERROR: failed EN_OK: success
 */
STATIC int InitArgInfo(int opt, ArgInfo *opts, int argc, char **argv)
{
    int ret;
    while (opt != -1) {
        int arg = GetInputArg(opt);
        if (arg == INVALID_ARG) {
            OptionsUsage();
            return EN_INVALID_PARAM;
        }
        DataRetriver pRetriever = dataRetriver[arg];
        if (pRetriever != NULL) {
            ret = pRetriever(opts);
            if (ret != EN_OK) {
                return ret;
            }
        } else {
            printf("pRetriever is null");
            return EN_ERROR;
        }

        opt = mmGetOptLong(argc, argv, DEFAULT_ARG_TYPES, g_opts, NULL);
    }
    return EN_OK;
}

/**
 * @brief read option values from cmdline input
 * @param [in]argc: argument count
 * @param [in]argv: argument valuse
 * @param [out]opts:structure to store option values
 * @return EN_ERROR: failed EN_OK: success
 */
STATIC int GetOptions(int argc, char **argv, ArgInfo *opts)
{
    CHK_NULL_PTR(opts, return EN_ERROR);
    opts->logOperationType = SET_LOGLEVEL;
    CHK_NULL_PTR(argv, return EN_ERROR);
    int opt = mmGetOptLong(argc, argv, DEFAULT_ARG_TYPES, g_opts, NULL);
    if (opt == -1) {
        OptionsUsage();
        return EN_ERROR;
    }
    return InitArgInfo(opt, opts, argc, argv);
}

/**
 * @brief process command
 * @param [in]logOperationType: command info
 * @return SET_LOGLEVEL/GET_LOGLEVEL
 */
STATIC char *ToLogOperatonTypeString(LogOperatonType logOperationType)
{
        switch (logOperationType) {
            case SET_LOGLEVEL: return "SET_LOGLEVEL";
            case GET_LOGLEVEL: return "GET_LOGLEVEL";
            default: return "SET_LOGLEVEL";
        }
}

/**
 * @brief export device log
 * @param [in]opt: bboxdump info
 * @param [in]logType: log type
 * @return EN_OK: succ EN_ERROR: failed
 */
int SyncDeviceLog(struct BboxDumpOpt *opt, int logType)
{
    // create slog/bbox/stackcore path
    if (CreateLogPath(logType) != EN_OK) {
        return EN_ERROR;
    }

    // run thread to connect device for log
    StartSyncDeviceLog(opt, logType);
    StopSyncDeviceLog(logType);

    printf("Finished.\n");
    return EN_OK;
}

/**
 * @brief export Bbox logs
 * @param [in]argInfo: command info
 * @param [in]logType: log type
 * @param [in]opt: export option
 * @return EN_OK: succ EN_ERROR: failed
 */
STATIC int ExportBboxLogs(const ArgInfo *argInfo, int logType, struct BboxDumpOpt *opt)
{
    if (argInfo->deviceIdFlag == 1) {
        printf("opition -a and -f is used alone, no need to specify option -d.\n");
        return EN_ERROR;
    }
    // Export bbox log files, device event information.\n"
    if (argInfo->logOperationType == EXPORT_BBOX_LOGS_DEVICE_EVENT) {
        opt->all = true;
        return SyncDeviceLog(opt, logType);
    }
    // Export bbox log files, device event information, historical maintenance information.\n"
    opt->force = true;
    return SyncDeviceLog(opt, logType);
}

/**
 * @brief process command
 * @param [in]argInfo: command info
 * @return EN_OK: succ EN_ERROR: failed
 */
STATIC int RequestHandle(const ArgInfo *argInfo, struct BboxDumpOpt *opt)
{
    CHK_NULL_PTR(argInfo, return EN_ERROR);
    if (argInfo->logOperationType == EXPORT_BBOX_LOGS_DEVICE_EVENT ||
        argInfo->logOperationType == EXPORT_BBOX_LOGS_DEVICE_EVENT_MAINTENANCE_INFORMATION) {
        return ExportBboxLogs(argInfo, argInfo->logType, opt);
    }
    if (argInfo->logOperationType == EXPORT_SPECIFIC_LOG) {
        if (argInfo->deviceIdFlag == 1) {
            printf("opition -t is used alone, no need to specify option -d.\n");
            return EN_ERROR;
        }
        return SyncDeviceLog(opt, argInfo->logType);
    }
    // to avid user only config "-d"
    if (argInfo->logOperationType == SET_LOGLEVEL) {
        CHK_EXPR_CTRL(strlen(argInfo->level) == 0, return EN_ERROR,
                      "The log level is empty, you can execute '--help' for example value.\n");
    }
    char logLevelStr[MAX_LOG_LEVEL_CMD_LEN + 1] = { 0 };
    int ret = 0;
    if (argInfo->logOperationType == SET_LOGLEVEL) {
        ret = snprintf_s(logLevelStr, MAX_LOG_LEVEL_CMD_LEN, MAX_LOG_LEVEL_CMD_LEN,
                         "%s(%d)[%s]", SET_LOG_LEVEL_STR, argInfo->logLevelType, argInfo->level);
    } else {
        ret = snprintf_s(logLevelStr, MAX_LOG_LEVEL_CMD_LEN, MAX_LOG_LEVEL_CMD_LEN, "%s", GET_LOG_LEVEL_STR);
    }
    if (ret == -1) {
        printf("Make %s cmd string failed!. strerr=%s.\n",
               ToLogOperatonTypeString(argInfo->logOperationType), strerror(mmGetErrorCode()));
        return EN_ERROR;
    }
    // set or get device log level
    char logLevelResult[MAX_LOG_LEVEL_RESULT_LEN + 1] = { 0 };
    int logLevelResultLength = MAX_LOG_LEVEL_RESULT_LEN + 1;
    ret = AdxOperateDeviceLogLevel(argInfo->deviceId, (const char *)logLevelStr,
                                   logLevelResult, logLevelResultLength, argInfo->logOperationType);
    if (ret != 0) {
        printf("%s failed, log level string=%s, device id=%d.\n",
               ToLogOperatonTypeString(argInfo->logOperationType), logLevelStr, argInfo->deviceId);
        return EN_ERROR;
    }
    if (argInfo->logOperationType == GET_LOGLEVEL) {
        printf("The system log level of device_id:%d is as follows:\n%s\n", argInfo->deviceId, logLevelResult);
    } else {
        printf("Set device log level success! log level string=%s, device id=%d.\n", logLevelStr, argInfo->deviceId);
    }
    return EN_OK;
}

int MAIN(int argc, char **argv)
{
    struct BboxDumpOpt bboxDumpOpt;
    bboxDumpOpt.all = false;
    bboxDumpOpt.force = false;

    if (argc >= MIN_USER_ARG_LEN) {
        ArgInfo argInfo = { 0 };
        CHK_EXPR_ACT(GetOptions(argc, argv, &argInfo) == EN_OK, return RequestHandle(&argInfo, &bboxDumpOpt));
        return EN_ERROR;
    }
    return SyncDeviceLog(&bboxDumpOpt, ALL_LOG);
}
