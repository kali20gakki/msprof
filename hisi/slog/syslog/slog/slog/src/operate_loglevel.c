/**
 * @operate_loglevel.c
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#include "operate_loglevel.h"
#include "securec.h"
#include "common.h"
#include "dlog_error_code.h"
#include "cfg_file_parse.h"
#include "log_common_util.h"
#include "log_sys_package.h"
#include "syslogd.h"
#include "share_mem.h"
#if defined _SLOG_DEVICE_
#include "ascend_hal.h"
#endif

#define EVENT_ENABLE_VALUE 1
#define EVENT_DISABLE_VALUE 0
#define CONFIG_NAME_MAX_LENGTH 1024

static toolMutex g_levelMutex;

STATIC LogRt SetEventLevelValue(LogLevel *logLevel, char *data);
STATIC LogRt SetAllModuleLevel(int *modLevel, int logLevel);

#if defined _SLOG_DEVICE_
STATIC int GetLevelByChnl(int channelType)
{
    int level;
    switch (channelType) {
        case LOG_CHANNEL_TYPE_TS:
            level = g_logLevel.moduleLevel[TS];
            break;
        case LOG_CHANNEL_TYPE_TS_DUMP:
            level = g_logLevel.moduleLevel[TSDUMP];
            break;
        case LOG_CHANNEL_TYPE_AICPU:
            level = g_logLevel.moduleLevel[AICPU];
            break;
        case LOG_CHANNEL_TYPE_LPM3:
            level = g_logLevel.moduleLevel[LP];
            break;
        case LOG_CHANNEL_TYPE_IMU:
            level = g_logLevel.moduleLevel[IMU];
            break;
        case LOG_CHANNEL_TYPE_IMP:
            level = g_logLevel.moduleLevel[IMP];
            break;
        case LOG_CHANNEL_TYPE_ISP:
            level = g_logLevel.moduleLevel[ISP];
            break;
        case LOG_CHANNEL_TYPE_SIS:
            level = g_logLevel.moduleLevel[SIS];
            break;
        case LOG_CHANNEL_TYPE_HSM:
            level = g_logLevel.moduleLevel[HSM];
            break;
        default:
            level = g_logLevel.globalLevel;
            break;
    }
    return level;
}

/**
 * @brief SetDlogLevel: set device log level, eg: TS/LMP3
 * @return: GET_DEVICE_ID_ERR/SET_LEVEL_ERR/SUCCESS
 */
STATIC LogRt SetDlogLevel(void)
{
    int deviceId[LOG_DEVICE_ID_MAX] = { 0 };
    int deviceChnlType[LOG_CHANNEL_NUM_MAX] = { 0 };
    int deviceNum = 0;
    int channelTypeNum = 0;
    int level = 0;

    int ret = log_get_device_id(deviceId, &deviceNum, LOG_DEVICE_ID_MAX);
    if ((ret != SYS_OK) || (deviceNum > LOG_DEVICE_ID_MAX)) {
        SELF_LOG_WARN("get device id failed, result=%d, device_number=%d, max_device_id=%d.",
                      ret, deviceNum, LOG_DEVICE_ID_MAX);
        return GET_DEVICE_ID_ERR;
    }

    LogRt res = SUCCESS;
    int i = 0;
    for (; i < deviceNum; i++) { // iterate all device
        ret = log_get_channel_type(deviceId[i], deviceChnlType, &channelTypeNum, LOG_CHANNEL_NUM_MAX);
        if ((ret != SYS_OK) || (channelTypeNum > LOG_CHANNEL_NUM_MAX)) {
            SELF_LOG_WARN("get device channel type failed, result=%d, device_id=%d, " \
                          "channel_type_number=%d, max_channel_number=%d.",
                          ret, deviceId[i], channelTypeNum, LOG_CHANNEL_NUM_MAX);
            res = SET_LEVEL_ERR;
            continue;
        }
        int j = 0;
        for (; j < channelTypeNum; j++) {
            if (deviceChnlType[j] >= LOG_CHANNEL_TYPE_MAX) {
                continue;
            }
            level = GetLevelByChnl(deviceChnlType[j]);
            // level is unsigned value
            if (level > LOG_MAX_LEVEL) {
                level = g_logLevel.globalLevel;
            }
            ret = log_set_level(deviceId[i], deviceChnlType[j], level);
            if (ret != SYS_OK) {
                SELF_LOG_WARN("set device level failed, result=%d, device_id=%d, channel_type=%d, level=%s.",
                              ret, deviceId[i], deviceChnlType[j], GetBasicLevelNameById(level));
                res = SET_LEVEL_ERR;
            }
        }
    }
    SELF_LOG_INFO("Set device level finished, result=%d", res);
    return res;
}
#else
STATIC LogRt SetDlogLevel(void)
{
    return SUCCESS;
}
#endif

int ThreadInit(void)
{
    if (ToolMutexInit(&g_levelMutex) != SYS_OK) {
        SELF_LOG_WARN("init mutex failed, strerr=%s.", strerror(ToolGetErrorCode()));
        return SYS_ERROR;
    }
    return SYS_OK;
}

STATIC INT32 ThreadLock(void)
{
    INT32 result = ToolMutexLock(&g_levelMutex);
    if (result != SYS_OK) {
        SELF_LOG_WARN("lock failed, strerr=%s.", strerror(ToolGetErrorCode()));
    }
    return result;
}

STATIC INT32 ThreadUnLock(void)
{
    INT32 result = ToolMutexUnLock(&g_levelMutex);
    if (result != SYS_OK) {
        SELF_LOG_WARN("unlock failed, strerr=%s.", strerror(ToolGetErrorCode()));
    }
    return result;
}

/**
 * @brief InitModuleLevel: initial module log level from cfgFile
 * @param [in/out]modLevel: array modLevel which record module log level
 * @param [in]maxId: max num of array modLevel
 * @return: void
 */
void InitModuleLevel(int *modLevel, int maxId)
{
    ONE_ACT_WARN_LOG(modLevel == NULL, return, "[input] module_level_array is null.");
    ONE_ACT_WARN_LOG(maxId > INVLID_MOUDLE_ID, return, "[input] module_level_array_size is illegal, size=%d.", maxId);

    // get value of global_level
    int globalLevel = MODULE_DEFAULT_LOG_LEVEL; // init invalid level
    char val[CONF_VALUE_MAX_LEN + 1] = { 0 };
    LogRt ret = GetConfValueByList(GLOBALLEVEL_KEY, strlen(GLOBALLEVEL_KEY), val, CONF_VALUE_MAX_LEN);
    if ((ret == SUCCESS) && IsNaturalNumStr((const char *)val)) {
        int tempLevel = atoi(val);
        if ((tempLevel >= LOG_MIN_LEVEL) && (tempLevel <= LOG_MAX_LEVEL)) {
            globalLevel = tempLevel;
        }
    }
    int id = 0;
    for (; id  < maxId; id++) {
        modLevel[id] = globalLevel;
    }

    const ModuleInfo *moduleInfo = GetModuleInfos();
    for (; (moduleInfo != NULL) && (moduleInfo->moduleName != NULL); moduleInfo++) {
        (void)memset_s(val, CONF_VALUE_MAX_LEN + 1, 0, CONF_VALUE_MAX_LEN + 1);
        ret = GetConfValueByList(moduleInfo->moduleName, strlen(moduleInfo->moduleName), val, CONF_VALUE_MAX_LEN);
        if (ret != SUCCESS) {
            SELF_LOG_INFO("no config_name=%s in config file, result=%d, use default_config_value=%s.",
                          moduleInfo->moduleName, ret, GetBasicLevelNameById(modLevel[moduleInfo->moduleId]));
            continue;
        }

        if (IsNaturalNumStr((const char *)val)) {
            int moduleLevel = atoi(val);
            if ((moduleLevel >= LOG_MIN_LEVEL) && (moduleLevel <= LOG_MAX_LEVEL)) {
                modLevel[moduleInfo->moduleId] = moduleLevel;
            }
        }
    }
}

STATIC void ToUpper(char *str)
{
    if (str == NULL) {
        return;
    }

    char *ptr = str;
    while (*ptr != '\0') {
        if ((*ptr <= 'z') && (*ptr >= 'a')) {
            *ptr = 'A' - 'a' + *ptr;
        }
        ptr++;
    }
}

/**
 * @brief ReadFileToBuffer: read config file to buffer, only used in the function 'ReadFileAll'
 * @param [in]fp: config file handle, it mustn't null
 * @param [in]cfgFile: config file path with fileName, it mustn't null
 * @param [out]dataBuf: conif file file data, it mustn't null
 * @return: ARGV_NULL/MALLOC_FAILED/SUCCESS/others
 */
STATIC LogRt ReadFileToBuffer(FILE *fp, const char *cfgFile, FileDataBuf *dataBuf)
{
    // Get cfg file size
    struct stat st;
    if (fstat(fileno(fp), &st) != 0) {
        SELF_LOG_WARN("get file state failed, file=%s, strerr=%s.", cfgFile, strerror(ToolGetErrorCode()));
        return READ_FILE_ERR;
    }

    if ((st.st_size <= 0) || (st.st_size > SLOG_CONFIG_FILE_SIZE)) {
        SELF_LOG_WARN("file size is invalid, file=%s, file_size=%d.", cfgFile, st.st_size);
        return READ_FILE_ERR;
    }

    char *fileBuf = (char *)malloc(st.st_size + 1);
    if (fileBuf == NULL) {
        SELF_LOG_WARN("malloc failed, size=%d, strerr=%s.", st.st_size, strerror(ToolGetErrorCode()));
        return MALLOC_FAILED;
    }
    (void)memset_s(fileBuf, st.st_size + 1, 0x00, st.st_size + 1);

    int ret = fread(fileBuf, 1, st.st_size, fp);
#if (OS_TYPE_DEF == LINUX)
    if (ret != st.st_size) {
        SELF_LOG_WARN("read file failed, file=%s, result=%d, strerr=%s.", cfgFile, ret, strerror(ToolGetErrorCode()));
        XFREE(fileBuf);
        return READ_FILE_ERR;
    }
#else
    int res = ferror(fp);
    if ((res != 0) && (ret != st.st_size)) {
        SELF_LOG_WARN("read file failed, file=%s, result=%d, strerr=%s.", cfgFile, ret, strerror(ToolGetErrorCode()));
        XFREE(fileBuf);
        return READ_FILE_ERR;
    }
#endif
    fileBuf[st.st_size] = '\0';
    dataBuf->len = ret;
    dataBuf->data = fileBuf;
    return SUCCESS;
}

/**
 * @brief ReadFileAll: read config file
 * @param [in]cfgFile: config file path with fileName
 * @param [out]dataBuf: conif file file data
 * @return: ARGV_NULL/MALLOC_FAILED/SUCCESS/others
 */
STATIC LogRt ReadFileAll(const char *cfgFile, FileDataBuf *dataBuf)
{
    ONE_ACT_WARN_LOG(cfgFile == NULL, return ARGV_NULL, "[input] config file is null.");
    ONE_ACT_WARN_LOG(dataBuf == NULL, return ARGV_NULL, "[input] file data buffer is null.");

    char *pPath = (char *)malloc(TOOL_MAX_PATH + 1);
    if (pPath == NULL) {
        SELF_LOG_WARN("malloc failed, strerr=%s.", strerror(ToolGetErrorCode()));
        return MALLOC_FAILED;
    }
    (void)memset_s(pPath, TOOL_MAX_PATH + 1, 0x00, TOOL_MAX_PATH + 1);

    // Get cfg file absolute path
    if (ToolRealPath((CHAR *)cfgFile, pPath, TOOL_MAX_PATH + 1) != SYS_OK) {
        SELF_LOG_WARN("get file realpath failed, file=%s, strerr=%s.", cfgFile, strerror(ToolGetErrorCode()));
        XFREE(pPath);
        return GET_CONF_FILEPATH_FAILED;
    }

    int fileLen = strlen(pPath);
    if (!IsPathValidByConfig(pPath, fileLen)) {
        SELF_LOG_WARN("file realpath is invalid, file=%s, realpath=%s.", cfgFile, pPath);
        XFREE(pPath);
        return CONF_FILEPATH_INVALID;
    }

    FILE *fp = fopen(pPath, "r");
    if (fp == NULL) {
        SELF_LOG_WARN("open file failed, file=%s, strerr=%s.", cfgFile, strerror(ToolGetErrorCode()));
        XFREE(pPath);
        return OPEN_CONF_FAILED;
    }
    XFREE(pPath);

    LogRt res = ReadFileToBuffer(fp, cfgFile, dataBuf);
    LOG_CLOSE_FILE(fp);
    return res;
}

#if (OS_TYPE_DEF == 0)
/**
 * @brief ConstructEvent: constuct global level
 * @param [in]tmp: level char tmp value, > 0
 * @param [in]level: level value
 * @return: level char
 *      debug: 0001x000
 *       info: 0010x000
 *    warning: 0011x000
 *      error: 0100x000
 *       null: 0101x000
 *    invalid: 0110x000
 */
STATIC char ConstructGlobal(char tmp, int *level)
{
    ONE_ACT_NO_LOG(level == NULL, return tmp);
    ONE_ACT_NO_LOG(tmp < 0, return tmp);

    if ((*level > LOG_MAX_LEVEL) || (*level < LOG_MIN_LEVEL)) {
        *level = SLOG_INVALID_LEVEL;
    }
    return ((((unsigned int)(*level + 1)) & 0x7) << 4) | ((unsigned char)tmp); // high 6~4bit
}

/**
 * @brief ConstructEvent: constuct event level
 * @param [in]tmp: level char tmp value, > 0
 * @param [in]level: level value
 * @return: level char
 *     enable: 0xxx1000
 *    disable: 0xxx0100
 */
STATIC char ConstructEvent(char tmp, char *level)
{
    ONE_ACT_NO_LOG(level == NULL, return tmp);
    ONE_ACT_NO_LOG(tmp < 0, return tmp);

    if ((*level != EVENT_ENABLE_VALUE) && (*level != EVENT_DISABLE_VALUE)) {
        *level = EVENT_ENABLE_VALUE;
    }
    return ((((unsigned char)(*level + 1)) & 0x3) << 2) | ((unsigned char)tmp); // low 3~2bit
}

/**
 * @brief ConstructModule: constuct module level
 * @param [in]tmp: level char tmp value, > 0
 * @param [in]levell: module[i] level value
 * @param [in]levelr: module[i+1] level value
 * @return: level char
 *      debug: 00010xxx/0xxx0001
 *       info: 00100xxx/0xxx0010
 *    warning: 00110xxx/0xxx0011
 *      error: 01000xxx/0xxx0100
 *       null: 01010xxx/0xxx0101
 *    invalid: 01100xxx/0xxx0110
 */
STATIC char ConstructModule(char tmp, int *levell, int *levelr)
{
    char retChar = tmp;
    ONE_ACT_NO_LOG(retChar < 0, return retChar);

    if (levell != NULL) {
        if ((*levell > LOG_MAX_LEVEL) || (*levell < LOG_MIN_LEVEL)) {
            *levell = SLOG_INVALID_LEVEL;
        }
        retChar = ((unsigned char)retChar) | ((((unsigned int)(*levell + 1)) & 0x7) << 4); // high 6~4bit
    }

    if (levelr != NULL) {
        if ((*levelr > LOG_MAX_LEVEL) || (*levelr < LOG_MIN_LEVEL)) {
            *levelr = SLOG_INVALID_LEVEL;
        }
        retChar = ((unsigned char)retChar) | (((unsigned int)(*levelr + 1)) & 0x7); // low 2~0bit
    }
    return retChar;
}

/**
 * @brief ConstructLevelStr: constuct all level info to string,
 *  format:      0111 |         11 |         00 |        0111 |       0111 |        0111 |...
 *             global |      event |    reserve |   module[0] |  module[1] |   module[2] |...
 *        high 6~4bit | low 3~2bit | low 1~0bit | high 6~4bit | low 2~0bit | high 6~4bit |...
 * @param [in/out]str: module name string
 * @param [in]strLen: str max length
 * @return: STR_COPY_FAILED: construct module name failed;
 *          ARGV_NULL: str is null;
 *          INPUT_INVALID: strLen is invalid;
 */
STATIC LogRt ConstructLevelStr(char *str, int strLen, LogLevel *levelInfo)
{
    ONE_ACT_NO_LOG(str == NULL, return ARGV_NULL);
    ONE_ACT_NO_LOG((strLen <= 0) || (strLen > LEVEL_ARR_LEN), return INPUT_INVALID);
    ONE_ACT_NO_LOG(levelInfo == NULL, return ARGV_NULL);

    int ret;
    int i = 0;
    char level = 0;

    level = ConstructEvent(level, &(levelInfo->enableEvent));
    level = ConstructGlobal(level, &(levelInfo->globalLevel));
    ret = strncpy_s(str, strLen - 1, (const char *)&level, 1);
    ONE_ACT_WARN_LOG(ret != EOK, return STR_COPY_FAILED,
                     "copy failed, ret=%d, strerr=%s.", ret, strerror(ToolGetErrorCode()));

    // module
    for (; i < INVLID_MOUDLE_ID; i += LEVEL_PARSE_STEP_SIZE) {
        level = 0;
        if ((i + 1) == INVLID_MOUDLE_ID) {
            level = ConstructModule(level, &(levelInfo->moduleLevel[i]), NULL);
        } else {
            int levell = levelInfo->moduleLevel[i];
            int nextIndex = i + 1;
            int levelr = levelInfo->moduleLevel[nextIndex];
            level = ConstructModule(level, &levell, &levelr);
        }
        ret = strncat_s(str, strLen - 1, (const char *)&level, 1);
        ONE_ACT_WARN_LOG(ret != EOK, return STR_COPY_FAILED,
                         "copy failed, ret=%d, strerr=%s.", ret, strerror(ToolGetErrorCode()));
    }
    return SUCCESS;
}

/**
 * @brief ConstructModuleStr: constuct all module name to string,
 *                              format: SLOG;IDEDD;IDEDH;...;FV;
 * @param [in/out]str: module name string
 * @param [in]strLen: str max length
 * @return: STR_COPY_FAILED: construct module name failed;
 *          ARGV_NULL: str is null;
 *          INPUT_INVALID: strLen is invalid;
 */
STATIC LogRt ConstructModuleStr(char *str, int strLen)
{
    ONE_ACT_NO_LOG(str == NULL, return ARGV_NULL);
    ONE_ACT_NO_LOG((strLen <= 0) || (strLen > MODULE_ARR_LEN), return INPUT_INVALID);

    int i = 0;
    int ret;
    int len = 0;

    const ModuleInfo *moduleInfos = GetModuleInfos();
    ONE_ACT_NO_LOG(moduleInfos == NULL, return ARGV_NULL);
    for (; i < INVLID_MOUDLE_ID; i++) {
        ret = sprintf_s(str + len, strLen - 1, "%s;", moduleInfos[i].moduleName);
        if ((ret == -1) || ((unsigned)ret != (strlen(moduleInfos[i].moduleName) + 1))) {
            SELF_LOG_WARN("copy failed, ret=%d, strerr=%s.", ret, strerror(ToolGetErrorCode()));
            return STR_COPY_FAILED;
        }
        len += strlen(moduleInfos[i].moduleName) + 1;
    }

    return SUCCESS;
}

#ifndef IAM
/**
 * @brief UpdateNotifyFile: write level notify file
 * @return: SYS_OK/SYS_ERROR
 */
STATIC int UpdateNotifyFile(void)
{
    int res;

    char notifyFile[WORKSPACE_PATH_MAX_LENGTH + 1] = { 0 };
    int len = strlen(GetWorkspacePath()) + strlen(LEVEL_NOTIFY_FILE) + 1;
    res = sprintf_s(notifyFile, WORKSPACE_PATH_MAX_LENGTH, "%s/%s", GetWorkspacePath(), LEVEL_NOTIFY_FILE);
    if (res != len) {
        SELF_LOG_WARN("copy failed, res=%d, strerr=%s.", res, strerror(ToolGetErrorCode()));
        return SYS_ERROR;
    }

    int fd = ToolOpen((const char *)notifyFile, O_CREAT | O_WRONLY);
    if (fd < 0) {
        SELF_LOG_WARN("open notify file failed, strerr=%s, file=%s.", strerror(ToolGetErrorCode()), notifyFile);
        return SYS_ERROR;
    }

    res = ToolChmod((const char *)notifyFile, SyncGroupToOther(S_IRUSR | S_IWUSR | S_IRGRP)); // 0640
    if (res != 0) {
        SELF_LOG_WARN("chmod %s failed , strerr=%s.", notifyFile, strerror(ToolGetErrorCode()));
        LOG_MMCLOSE_AND_SET_INVALID(fd);
        return SYS_ERROR;
    }

    res = ToolWrite(fd, LEVEL_NOTIFY_FILE, strlen(LEVEL_NOTIFY_FILE));
    if ((res < 0) || (res != strlen(LEVEL_NOTIFY_FILE))) {
        SELF_LOG_WARN("write notify file failed, res=%d, strerr=%s, file=%s.",
                      res, strerror(ToolGetErrorCode()), notifyFile);
        LOG_MMCLOSE_AND_SET_INVALID(fd);
        return SYS_ERROR;
    }
    LOG_MMCLOSE_AND_SET_INVALID(fd);
    return SYS_OK;
}
#endif
#endif

/**
 * @brief UpdateLevelToShMem: write level info to shmem, and write file /usr/slog/level_notify
 *                             to notify all app to update level change.
 * @param [in]levelInfo: all level info
 * @return: SYS_OK: write to shmem succeed and update notify file succeed
 */
STATIC int UpdateLevelToShMem(LogLevel *levelInfo)
{
#if (OS_TYPE_DEF == 0)
    ONE_ACT_WARN_LOG(levelInfo == NULL, return SYS_ERROR, "input param null.");

    int shmId = -1;
    ShmErr res;

    char *levelStr = (char *)malloc(LEVEL_ARR_LEN);
    if (levelStr == NULL) {
        SELF_LOG_WARN("malloc failed, strerr=%s.", strerror(ToolGetErrorCode()));
        return SYS_ERROR;
    }
    (void)memset_s(levelStr, LEVEL_ARR_LEN, 0, LEVEL_ARR_LEN);

    // construct level info
    LogRt err = ConstructLevelStr(levelStr, LEVEL_ARR_LEN, levelInfo);
    if (err != SUCCESS) {
        SELF_LOG_WARN("construct level str failed, log_err=%d.", err);
        XFREE(levelStr);
        return SYS_ERROR;
    }
    // write level info to shmem
    res = OpenShMem(&shmId);
    if (res == SHM_ERROR) {
        SELF_LOG_WARN("Open shmem failed.");
        XFREE(levelStr);
        return SYS_ERROR;
    }
    // write shmem
    res = WriteToShMem(shmId, levelStr, LEVEL_ARR_LEN, PATH_LEN + MODULE_ARR_LEN);
    if (res == SHM_ERROR) {
        SELF_LOG_WARN("Write level to shmem failed.");
        XFREE(levelStr);
        return SYS_ERROR;
    }
    XFREE(levelStr);

    // update ${workpath}/level_notify
#ifndef IAM
    int ret = UpdateNotifyFile();
    if (ret != SYS_OK) {
        return SYS_ERROR;
    }
#endif
#endif

    return SYS_OK;
}

/**
 * @brief InitModuleArrToShMem: write module name to shmem
 * @return: SYS_OK: write to shmem succeed
 */
int InitModuleArrToShMem(void)
{
#if (OS_TYPE_DEF == 0)
    ShmErr res;
    int shmId = -1;

    char *moduleStr = (char *)malloc(MODULE_ARR_LEN);
    if (moduleStr == NULL) {
        SELF_LOG_WARN("malloc failed, strerr=%s.", strerror(ToolGetErrorCode()));
        return SYS_ERROR;
    }
    (void)memset_s(moduleStr, MODULE_ARR_LEN, 0, MODULE_ARR_LEN);

    // construct module
    LogRt err = ConstructModuleStr(moduleStr, MODULE_ARR_LEN);
    if (err != SUCCESS) {
        SELF_LOG_WARN("construct module str failed, log_err=%d.", err);
        XFREE(moduleStr);
        return SYS_ERROR;
    }
    // write module info to shmem
    res = OpenShMem(&shmId);
    if (res == SHM_ERROR) {
        SELF_LOG_WARN("Open shmem failed.");
        XFREE(moduleStr);
        return SYS_ERROR;
    }
    // write shmem
    res = WriteToShMem(shmId, moduleStr, MODULE_ARR_LEN, PATH_LEN);
    if (res == SHM_ERROR) {
        SELF_LOG_WARN("Write module arr to shmem failed, res=%d.", res);
        XFREE(moduleStr);
        return SYS_ERROR;
    }
    XFREE(moduleStr);
#endif

    return SYS_OK;
}

/**
 * @brief WriteToSlogCfg: write buf to override config file
 * @param [in]cfgFile: config file info
 * @param [in]fileBuf: file content after modified
 * @return SUCCESS: succeed; others: failed;
 */
STATIC LogRt WriteToSlogCfg(const char *cfgFile, const FileDataBuf fileBuf)
{
    char *pPath =  NULL;
    LogRt res = SUCCESS;

    do {
        pPath = (char *)malloc(TOOL_MAX_PATH + 1);
        if (pPath == NULL) {
            SELF_LOG_WARN("malloc failed, strerr=%s.", strerror(ToolGetErrorCode()));
            res = MALLOC_FAILED;
            break;
        }
        (void)memset_s(pPath, TOOL_MAX_PATH + 1, 0x00, TOOL_MAX_PATH + 1);
        if (ToolRealPath(cfgFile, pPath, TOOL_MAX_PATH + 1) != SYS_OK) {
            SELF_LOG_WARN("get file realpath failed, file=%s.", cfgFile);
            res = GET_CONF_FILEPATH_FAILED;
            break;
        }

        int fileLen = strlen(pPath);
        if (!IsPathValidByConfig(pPath, fileLen)) {
            SELF_LOG_WARN("file realpath is invalid, file=%s, realpath=%s.", cfgFile, pPath);
            res = CONF_FILEPATH_INVALID;
            break;
        }

        int fd = ToolOpenWithMode((const char *)pPath, M_WRONLY, M_IRUSR | M_IWUSR);
        if (fd < 0) {
            SELF_LOG_WARN("open file failed, file=%s.", cfgFile);
            res = OPEN_CONF_FAILED;
            break;
        }

        int ret = ToolWrite(fd, fileBuf.data, fileBuf.len);
        if (ret != fileBuf.len) {
            SELF_LOG_WARN("write to file failed, file=%s, result=%d, file_buffer_size=%d.", cfgFile, ret, fileBuf.len);
            res = SET_LEVEL_ERR;
        }
        LOG_MMCLOSE_AND_SET_INVALID(fd);
    } while (0);

    XFREE(pPath);
    return res;
}

/**
 * @brief SetSlogCfgLevel: set level to config file
 * @param [in]cfgFile: config file info
 * @param [in]cfgName: config file name
 * @param [in]level: level id
 * @return SUCCESS: succeed; others: failed;
 */
STATIC LogRt SetSlogCfgLevel(const char *cfgFile, const char *cfgName, int level)
{
    FileDataBuf fileBuf = { 0, NULL };
    char *fullCfgName = NULL;
    const int levelConfigLen = 2;

    ONE_ACT_WARN_LOG(cfgFile == NULL, return ARGV_NULL, "[input] config file is null.");
    ONE_ACT_WARN_LOG(cfgName == NULL, return ARGV_NULL, "[input] config name is null.");

    LogRt res = ReadFileAll(cfgFile, &fileBuf);
    ONE_ACT_WARN_LOG(res != SUCCESS, return res, "read file all to buffer failed, file=%s.", cfgFile);

    do {
        int cfgNameLen = strlen(cfgName);
        if ((cfgNameLen <= 0) || (cfgNameLen > CONFIG_NAME_MAX_LENGTH)) {
            SELF_LOG_WARN("config name is empty or to long. length=%d.", cfgNameLen);
            res = INPUT_INVALID;
            break;
        }
        fullCfgName = (char *)malloc(cfgNameLen + levelConfigLen + 1);
        if (fullCfgName == NULL) {
            SELF_LOG_WARN("malloc failed, strerr=%s.", strerror(ToolGetErrorCode()));
            res = MALLOC_FAILED;
            break;
        }
        (void)memset_s(fullCfgName, cfgNameLen + levelConfigLen + 1, 0x00, cfgNameLen + levelConfigLen + 1);
        fullCfgName[0] = '\n';
        int i = 1;
        for (; i < (cfgNameLen + 1); i++) {
            fullCfgName[i] = cfgName[i - 1];
        }
        fullCfgName[i] = '=';

        // find cfgName pos in cfgFile
        char *ptr = strstr(fileBuf.data, fullCfgName);
        if (ptr == NULL) {
            break;
        }

        // ptr point to 'cfgName=' after
        ptr = ptr + 1 + strlen(cfgName) + 1; // +1 represents '\n' and '='

        // clear all string after 'cfgName=' except comment
        char *endCfgPtr = ptr;
        while ((*endCfgPtr != '\r') && (*endCfgPtr != '\n') && (*endCfgPtr != '#') && (*endCfgPtr != '\0')) {
            *endCfgPtr = ' ';
            endCfgPtr++;
        }
        // change cfgName val
        *ptr = '0' + level;

        res = WriteToSlogCfg(cfgFile, fileBuf);
    } while (0);

    XFREE(fullCfgName);
    XFREE(fileBuf.data);

    // update key-value config list, especially loglevel
    LogRt ret = UpdateConfList(cfgFile);
    ONE_ACT_WARN_LOG(ret != SUCCESS, return res, "update config list failed, result=%d.", ret);
    return res;
}

/**
 * @brief SetGlobalLogLevel: set global log level. format: "[info]"
 * @param [in]ptr: temp level info
 * @param [in]data: level setting info
 * @return SUCCESS: succeed; others: failed;
 */
STATIC LogRt SetGlobalLogLevel(LogLevel *ptr, char *data)
{
    ONE_ACT_WARN_LOG(data == NULL, return ARGV_NULL, "[input] global level string is null.");
    ONE_ACT_WARN_LOG(ptr == NULL, return ARGV_NULL, "[input] global level struct is null.");

    char *levelStr = data;
    ONE_ACT_WARN_LOG(*levelStr != '[', return LEVEL_INFO_ILLEGAL, \
                     "global level info has no '[', level_string=%s.", levelStr);

    size_t len = strlen(levelStr);
    ONE_ACT_WARN_LOG(levelStr[len - 1] != ']', return LEVEL_INFO_ILLEGAL, \
                     "global level info has no '[', level_string=%s.", levelStr);

    levelStr[len - 1] = '\0';
    levelStr++;
    ToUpper(levelStr);
    const ModuleInfo *levelInfo = GetLevelInfoByName((const char *)levelStr);
    ONE_ACT_WARN_LOG((levelInfo == NULL) || (levelInfo->moduleLevel < LOG_MIN_LEVEL) ||
                     (levelInfo->moduleLevel > LOG_MAX_LEVEL), return LEVEL_INFO_ILLEGAL,
                     "global level is invalid, str=%s.", levelStr);

    int result = ThreadLock();
    ONE_ACT_NO_LOG(result != 0, return FAILED);
    ptr->globalLevel = levelInfo->moduleLevel;

    // set global level to slog.conf
    LogRt res = SetSlogCfgLevel(GetConfigPath(), GLOBALLEVEL_KEY, levelInfo->moduleLevel);
    NO_ACT_WARN_LOG(res != SUCCESS, "set global level to file(%s) failed, result=%d.", GetConfigPath(), res);

    // set all module level to slog.conf
    LogRt moduleSettingRes = SetAllModuleLevel(ptr->moduleLevel, levelInfo->moduleLevel);
    if (moduleSettingRes != SUCCESS) {
        SELF_LOG_WARN("set all module level failed, result=%d.", moduleSettingRes);
        res = (res == SUCCESS) ? moduleSettingRes : res;
    }
    (void)ThreadUnLock();

    LogRt deviceSettingRes = SetDlogLevel();
    if (deviceSettingRes != SUCCESS) {
        SELF_LOG_WARN("set device level failed, result=%d.", deviceSettingRes);
        res = (res == SUCCESS) ? deviceSettingRes : res;
    }

    // write level info to shmem
    int ret = UpdateLevelToShMem(ptr);
    if (ret != SYS_OK) {
        SELF_LOG_WARN("update level info to shmem failed.");
        res = (res == SUCCESS) ? LEVEL_NOTIFY_FAILED : res;
    }
    return res;
}

/**
 * @brief SetModuleLogLevel: set module level. format: "[module:level]"
 * @param [in]ptr: temp level info
 * @param [in]data: level setting info
 * @return SUCCESS: succeed; others: failed;
 */
STATIC LogRt SetModuleLogLevel(LogLevel *ptr, char *data)
{
    char *levelStr = data;
    char modName[MAX_MOD_NAME_LEN] = { 0 };

    ONE_ACT_WARN_LOG(data == NULL, return ARGV_NULL, "[input] module level string is null.");
    ONE_ACT_WARN_LOG(ptr == NULL, return ARGV_NULL, "[input] module level struct is null.");
    ONE_ACT_WARN_LOG(*levelStr != '[', return LEVEL_INFO_ILLEGAL, \
                     "module level info has no '[', level_string=%s.", levelStr);

    size_t len = strlen(levelStr);
    ONE_ACT_WARN_LOG(levelStr[len - 1] != ']', return LEVEL_INFO_ILLEGAL, \
                     "module level info has no ']', level_string=%s.", levelStr);

    levelStr[len - 1] = '\0';
    levelStr++;
    char *pend = strchr(levelStr, ':');
    ONE_ACT_WARN_LOG(pend == NULL, return LEVEL_INFO_ILLEGAL, \
                      "module level info has no ':', level_string=%s.", levelStr);

    int retValue = memcpy_s(modName, MAX_MOD_NAME_LEN, levelStr, pend - levelStr);
    ONE_ACT_WARN_LOG(retValue != EOK, return STR_COPY_FAILED, \
                     "memcpy_s failed, result=%d, strerr=%s.", retValue, strerror(ToolGetErrorCode()));

    ToUpper(modName);
    const ModuleInfo *moduleInfo = GetModuleInfoByName((const char *)modName);
    ONE_ACT_WARN_LOG(moduleInfo == NULL, return LEVEL_INFO_ILLEGAL, \
                     "module name does not exist, module=%s.", modName);

    levelStr = pend + 1;
    ToUpper(levelStr);
    const ModuleInfo *levelInfo = GetLevelInfoByName((const char *)levelStr);
    ONE_ACT_WARN_LOG((levelInfo == NULL) || (levelInfo->moduleLevel < LOG_MIN_LEVEL) ||
                     (levelInfo->moduleLevel > LOG_MAX_LEVEL), return LEVEL_INFO_ILLEGAL,
                     "module level is invalid, level_string=%s.", levelStr);

    int result = ThreadLock();
    ONE_ACT_NO_LOG(result != 0, return FAILED);
    ptr->moduleLevel[moduleInfo->moduleId] = levelInfo->moduleLevel;

    // set module level to config file
    LogRt res = SetSlogCfgLevel(GetConfigPath(), modName, levelInfo->moduleLevel);
    NO_ACT_WARN_LOG(res != SUCCESS, "set module level to file failed, file=%s, module=%s, result=%d.", \
                    GetConfigPath(), modName, res);
    (void)ThreadUnLock();

    LogRt deviceSettingRes = SetDlogLevel();
    if (deviceSettingRes != SUCCESS) {
        SELF_LOG_WARN("set device level failed, module=%s, result=%d.", modName, deviceSettingRes);
        res = (res == SUCCESS) ? deviceSettingRes : res;
    }

    // write level info to shmem
    int ret = UpdateLevelToShMem(ptr);
    if (ret != SYS_OK) {
        SELF_LOG_WARN("update level info to shmem failed.");
        res = (res == SUCCESS) ? LEVEL_NOTIFY_FAILED : res;
    }
    return res;
}

/**
 * @brief IsModule: judge whether it belongs to modules
 * @param [in]confName: conf name
 * @return true: true; others: false;
 */
STATIC bool IsModule(const char *confName)
{
    const ModuleInfo *moduleInfo = GetModuleInfoByName(confName);
    return moduleInfo != NULL;
}

/**
 * @brief ConstructModuleLevel: construct module level
 * @param [in]moduleLevel: module level
 * @param [in]moduleLevelLength: module level length
 * @param [in]moduleArray: module array
 * @param [in]moduleNum: num of modules
 * @return SUCCESS: succeed; others: failed;
 */
STATIC LogRt ConstructModuleLevel(char *moduleLevel, int moduleLevelLength,
                                  const char (*moduleArray)[SINGLE_MODULE_MAX_LEN], int moduleNum)
{
    int i;
    for (i = moduleNum - 1; i >= 0; i--) {
        int ret = strcat_s(moduleLevel, moduleLevelLength, moduleArray[i]);
        ONE_ACT_WARN_LOG(ret != EOK, return STR_COPY_FAILED,
                         "strcat_s single module to moduleLevel failed, module info=%s, strerr=%s.",
                         moduleArray[i], strerror(ToolGetErrorCode()));
        if (((moduleNum - i) % LOG_EVENT_WRAP_NUM) == 0) {
            ret = strcat_s(moduleLevel, moduleLevelLength, "\n");
            ONE_ACT_WARN_LOG(ret != EOK, return STR_COPY_FAILED,
                             "strcat_s wrap to result failed, strerr=%s.", strerror(ToolGetErrorCode()));
        }
    }
    return SUCCESS;
}

/**
 * @brief DoCheckAndGetLevel: execute check level param and get level
 * @param [in]level: level
 * @param [in]levelLength: level length
 * @param [in]confListValue: conf list value
 * @param [in]defaultLogLevel: default log level value
 * @param [in]maxLevel: max value of log level
 * @return SUCCESS: succeed; others: failed;
 */
STATIC LogRt DoCheckAndGetLevel(char *level, int levelLength, const char *confListValue,
                                int defaultLogLevel, int maxLevel)
{
    if (!IsNaturalNumStr(confListValue) || !((atoi(confListValue) >= LOG_MIN_LEVEL) &&
        (atoi(confListValue) <= maxLevel))) {
        int ret = snprintf_s(level, levelLength, levelLength - 1, "%d", defaultLogLevel);
        ONE_ACT_WARN_LOG(ret == -1, return STR_COPY_FAILED,
                         "snprintf_s default log level failed, strerr=%s.", strerror(ToolGetErrorCode()));
    } else {
        int ret = memcpy_s(level, levelLength, confListValue, levelLength);
        ONE_ACT_WARN_LOG(ret != EOK, return STR_COPY_FAILED,
                         "memcpy_s log level failed, strerr=%s.", strerror(ToolGetErrorCode()));
    }
    return SUCCESS;
}

/**
 * @brief CheckAndGetLevel: check level param and get level
 * @param [in]level: level
 * @param [in]levelLength: level length
 * @param [in]confListValue: conf list value
 * @param [in]logType: log type, global event module
 * @return SUCCESS: succeed; others: failed;
 */
STATIC LogRt CheckAndGetLevel(char *level, int levelLength, const char *confListValue, int logType)
{
    LogRt ret = SUCCESS;
    switch (logType) {
        case GLOBALLEVEL:
            if (DoCheckAndGetLevel(level, levelLength, confListValue,
                                   GLOABLE_DEFAULT_LOG_LEVEL, LOG_MAX_LEVEL) != SUCCESS) {
                ret = STR_COPY_FAILED;
            }
            break;
        case EVENTLEVEL:
            if (DoCheckAndGetLevel(level, levelLength, confListValue,
                                   EVENT_ENABLE_VALUE, EVENT_ENABLE_VALUE) != SUCCESS) {
                ret = STR_COPY_FAILED;
            }
            break;
        case MODULELEVEL:
            if (DoCheckAndGetLevel(level, levelLength, confListValue,
                                   MODULE_DEFAULT_LOG_LEVEL, LOG_MAX_LEVEL) != SUCCESS) {
                ret = STR_COPY_FAILED;
            }
            break;
        default:
            SELF_LOG_WARN("log type is invalid.");
            ret = INVALID_LOG_TYPE;
            break;
    }
    return ret;
}

/**
 * @brief GetAllLevel: get globalLevel, eventLevel, moduleLevel
 * @param [in]globalLevel: global level
 * @param [in]eventLevel: event level
 * @param [in]moduleLevel: module level
 * @param [in]moduleLevelLength: module level length
 * @param [in]confListTmp: log level list
 * @return SUCCESS: succeed; others: failed;
 */
STATIC LogRt GetAllLevel(char *globalLevel, char *eventLevel, char *moduleLevel, int moduleLevelLength,
                         const ConfList *confListTmp)
{
    char moduleArray[INVLID_MOUDLE_ID][SINGLE_MODULE_MAX_LEN] = { { 0 } };
    char level[SINGLE_MODULE_MAX_LEN] = { 0 };
    LogRt logRt = SUCCESS;
    int ret = -1;
    int num = 0;
    for (; confListTmp != NULL; confListTmp = confListTmp->next) {
        if (strcmp(GLOBALLEVEL_KEY, confListTmp->confName) == 0) {
            logRt = CheckAndGetLevel(level, SINGLE_MODULE_MAX_LEN, confListTmp->confValue, GLOBALLEVEL);
            ONE_ACT_WARN_LOG(logRt != SUCCESS, return logRt,
                             "get global level failed, strerr=%s.", strerror(ToolGetErrorCode()));
            const char *levelName = GetLevelNameById(atoi(level));
            ONE_ACT_WARN_LOG(levelName == NULL, return LEVEL_INFO_ILLEGAL, "get level name failed, level=%s.", level);
            ret = memcpy_s(globalLevel, GLOBAL_ENABLE_MAX_LEN, levelName, GLOBAL_ENABLE_MAX_LEN);
            ONE_ACT_WARN_LOG(ret != EOK, return STR_COPY_FAILED,
                             "memcpy_s global level failed, strerr=%s.", strerror(ToolGetErrorCode()));
        }
        if (strcmp(ENABLEEVENT_KEY, confListTmp->confName) == 0) {
            logRt = CheckAndGetLevel(level, SINGLE_MODULE_MAX_LEN, confListTmp->confValue, EVENTLEVEL);
            ONE_ACT_WARN_LOG(logRt != SUCCESS, return logRt,
                             "get event level failed, strerr=%s.", strerror(ToolGetErrorCode()));
            if (atoi(level) == EVENT_ENABLE_VALUE) {
                ret = strncpy_s(eventLevel, GLOBAL_ENABLE_MAX_LEN, EVENT_ENABLE, strlen(EVENT_ENABLE));
            } else {
                ret = strncpy_s(eventLevel, GLOBAL_ENABLE_MAX_LEN, EVENT_DISABLE, strlen(EVENT_DISABLE));
            }
            ONE_ACT_WARN_LOG(ret != EOK, return STR_COPY_FAILED,
                             "strncpy_s event level failed, strerr=%s.", strerror(ToolGetErrorCode()));
        }
        if (IsModule(confListTmp->confName)) {
            logRt = CheckAndGetLevel(level, SINGLE_MODULE_MAX_LEN, confListTmp->confValue, MODULELEVEL);
            ONE_ACT_WARN_LOG(logRt != SUCCESS, return logRt,
                             "get module level failed, strerr=%s.", strerror(ToolGetErrorCode()));
            const char *levelName = GetLevelNameById(atoi(level));
            ONE_ACT_WARN_LOG(levelName == NULL, return LEVEL_INFO_ILLEGAL, "get level name failed, level=%s.", level);
            ret = snprintf_s(moduleArray[num], SINGLE_MODULE_MAX_LEN, SINGLE_MODULE_MAX_LEN - 1,
                             "%s:%s ", confListTmp->confName, levelName);
            ONE_ACT_WARN_LOG(ret == -1, return STR_COPY_FAILED,
                             "snprintf_s level info to single_module failed, strerr=%s.", strerror(ToolGetErrorCode()));
            num++;
        }
        (void)memset_s(level, SINGLE_MODULE_MAX_LEN, 0, SINGLE_MODULE_MAX_LEN);
    }
    logRt = ConstructModuleLevel(moduleLevel, moduleLevelLength, moduleArray, num);
    ONE_ACT_WARN_LOG(logRt != SUCCESS, return STR_COPY_FAILED,
                     "construct module level failed, strerr=%s.", strerror(ToolGetErrorCode()));
    return SUCCESS;
}

/**
 * @brief GetLogLevelValue: get log level
 * @param [in]logLevelResult: log level result of global, event, module
 * @return SUCCESS: succeed; others: failed;
 */
STATIC LogRt GetLogLevelValue(char *logLevelResult)
{
    const char *global = "[global]\n";
    const char *event = "[event]\n";
    const char *module = "[module]\n";
    char globalLevel[GLOBAL_ENABLE_MAX_LEN] = { 0 };
    char eventLevel[GLOBAL_ENABLE_MAX_LEN] = { 0 };
    char *moduleLevel = (char *)calloc(1, INVLID_MOUDLE_ID * SINGLE_MODULE_MAX_LEN);
    if (moduleLevel == NULL) {
        XFREE(moduleLevel);
        SELF_LOG_WARN("calloc moduleLevel failed, strerr=%s.", strerror(ToolGetErrorCode()));
        return CALLOC_FAILED;
    }
    const ConfList *confListTmp = GetGlobalConfList();
    if (GetAllLevel(globalLevel, eventLevel, moduleLevel,
        INVLID_MOUDLE_ID * SINGLE_MODULE_MAX_LEN, confListTmp) != SUCCESS) {
        XFREE(moduleLevel);
        SELF_LOG_WARN("get all level failed, strerr=%s.", strerror(ToolGetErrorCode()));
        return STR_COPY_FAILED;
    }
    int ret = snprintf_s(logLevelResult, MSG_MAX_LEN + 1, MSG_MAX_LEN, "%s%s%s%s%s%s%s%s", global, globalLevel, "\n",
                         event, eventLevel, "\n", module, moduleLevel);
    if (ret == -1) {
        XFREE(moduleLevel);
        SELF_LOG_WARN("construct result and copy to logLevelResult failed.", strerror(ToolGetErrorCode()));
        return STR_COPY_FAILED;
    }
    XFREE(moduleLevel);
    return SUCCESS;
}

/**
 * @brief SetLogLevelValue: set log level
 * @param [in]ptr: level info, inclue global level, module level and event level
 * @param [in]data: level setting info, format: 'SetLogLevel(scope)[levelInfo]'.
 * @return SUCCESS: succeed; others: failed;
 */
STATIC LogRt SetLogLevelValue(LogLevel *ptr, char *data)
{
    LogRt ret = LEVEL_INFO_ILLEGAL;
    char *levelStr = NULL;

    ONE_ACT_WARN_LOG(data == NULL, return ARGV_NULL, "[input] level command is null.");
    ONE_ACT_WARN_LOG(ptr == NULL, return ARGV_NULL, "[input] level struct is null.");
    if (StartsWith(data, SET_LOG_LEVEL_STR) == 0) {
        SELF_LOG_WARN("level data is invalid, default prefix is %s.", SET_LOG_LEVEL_STR);
        return ret;
    }

    levelStr = data + strlen(SET_LOG_LEVEL_STR);
    if (*levelStr != '(') {
        SELF_LOG_WARN("level command is invalid, level_command=%s.", data);
        return ret;
    }
    levelStr++;

    int globalMode = *levelStr - '0';
    levelStr++;
    if (*levelStr != ')') {
        SELF_LOG_WARN("level command is invalid, level_command=%s.", data);
        return ret;
    }
    levelStr++;

    switch (globalMode) {
        case LOGLEVEL_GLOBAL:
            ret = SetGlobalLogLevel(ptr, levelStr);
            break;
        case LOGLEVEL_MODULE:
            ret = SetModuleLogLevel(ptr, levelStr);
            break;
        case LOGLEVEL_EVENT:
            ret = SetEventLevelValue(ptr, levelStr);
            break;
        default:
            SELF_LOG_WARN("level command type is invalid, level_command=%s.", data);
            break;
    }
    return ret;
}

/**
 * @brief RespSettingResult: response level setting result by msg queue
 * @param [in]res: level setting error code
 * @param [in]queueId: message queue id
 * @param [in]logLevelResult: log level result
 * @param [in]logLevelType: log level type
 * @return: NA
 */
STATIC void RespSettingResult(LogRt res, toolMsgid queueId, const char *logLevelResult, int logLevelType)
{
    DlogMsgT resMsg = { 0, "" };

    int ret;
    resMsg.msgType = FEEDBACK_MSG_TYPE;
    switch (res) {
        case SUCCESS:
            if (logLevelType == GET_LOGLEVEL) {
                ret = strcpy_s(resMsg.msgData, MSG_MAX_LEN, logLevelResult);
            } else {
                ret = strcpy_s(resMsg.msgData, MSG_MAX_LEN, LEVEL_SETTING_SUCCESS);
            }
            break;
        case CONF_FILEPATH_INVALID:
        case GET_CONF_FILEPATH_FAILED:
        case OPEN_CONF_FAILED:
            ret = strcpy_s(resMsg.msgData, MSG_MAX_LEN, SLOG_CONF_ERROR_MSG);
            break;
        case LEVEL_INFO_ILLEGAL:
            ret = strcpy_s(resMsg.msgData, MSG_MAX_LEN, LEVEL_INFO_ERROR_MSG);
            break;
        case MALLOC_FAILED:
            ret = strcpy_s(resMsg.msgData, MSG_MAX_LEN, MALLOC_ERROR_MSG);
            break;
        case INITIAL_CHAR_POINTER_FAILED:
            ret = strcpy_s(resMsg.msgData, MSG_MAX_LEN, INITIAL_CHAR_POINTER_ERROR_MSG);
            break;
        case STR_COPY_FAILED:
            ret = strcpy_s(resMsg.msgData, MSG_MAX_LEN, STR_COPY_ERROR_MSG);
            break;
        default:
            ret = strcpy_s(resMsg.msgData, MSG_MAX_LEN, UNKNOWN_ERROR_MSG);
            break;
    }
    if (ret != EOK) {
        SELF_LOG_WARN("strcpy_s failed, result=%d, strerr=%s.", ret, strerror(ToolGetErrorCode()));
    }

    ret = SendQueueMsg(queueId, (void *)(&resMsg), MSG_MAX_LEN, 0);
    if (ret != ERR_OK) {
        SELF_LOG_WARN("response message to daemon client error, strerr=%s, response=%s.",
                      strerror(ToolGetErrorCode()), resMsg.msgData);
    }
}

/**
 * @brief ReceiveAndProcessLogLevel: receive and process log level
 * @param [in]inputQueueId: message queue id
 * @param [in]data: reveice data
 * @param [in]logLevelInfo: log level info
 * @return: NA
 */
STATIC void ReceiveAndProcessLogLevel(toolMsgid inputQueueId, DlogMsgT data, LogLevelInfo logLevelInfo)
{
    int ret;
    toolMsgid queueId = inputQueueId;
    LogRt res = LEVEL_INFO_ILLEGAL;
    while (GetSigNo() == 0) {
        if (queueId == INVALID) {
            ret = OpenMsgQueue(&queueId);
            TWO_ACT_WARN_LOG(ret != SYS_OK, (void)ToolSleep(ONE_SECOND), continue,
                             "create message queue failed, then try to create again, strerr=%s.",
                             strerror(ToolGetErrorCode()));
        }

        ret = RecvQueueMsg(queueId, (void *)(&data), MSG_MAX_LEN, 0, FORWARD_MSG_TYPE);
        if (ret != SYS_OK) {
            // Interrupted system call
            ONE_ACT_NO_LOG(ToolGetErrorCode() == EINTR, continue);
            SELF_LOG_WARN("receive message failed, result=%d, strerr=%s.", ret, strerror(ToolGetErrorCode()));
            queueId = INVALID;
            ToolSleep(10); // sleep 10ms
            continue;
        }

        if (strcmp(GET_LOG_LEVEL_STR, data.msgData) == 0) {
            logLevelInfo.logLevelType = GET_LOGLEVEL;
            res = GetLogLevelValue(logLevelInfo.logLevelResult);
            SELF_LOG_INFO("receive_data=%s, level_getting_result=%d, logLevelResult=%s",
                          data.msgData, res, logLevelInfo.logLevelResult);
        } else {
            logLevelInfo.logLevelType = SET_LOGLEVEL;
            res = SetLogLevelValue(logLevelInfo.logLevel, data.msgData);
            SELF_LOG_INFO("receive_data=%s, level_setting_result=%d.", data.msgData, res);
        }
        RespSettingResult(res, queueId, logLevelInfo.logLevelResult, logLevelInfo.logLevelType);
    }
}

/**
 * @brief OperateLogLevel: set or get log level by msg queue
 * @param [in]arg: LogLevel ptr passed by pthread func
 * @return: NA
 */
void *OperateLogLevel(ArgPtr arg)
{
    ONE_ACT_ERR_LOG(arg == NULL, return NULL, "invalid input, Thread(operateLogLevel) quit.");
    NO_ACT_WARN_LOG(SetThreadName("operateLogLevel") != 0, "set thread_name(operateLogLevel) failed.");

    LogLevel *ptr = (LogLevel *)arg;
    LogLevelInfo logLevelInfo = { ptr,  SET_LOGLEVEL, { 0 } };

    logLevelInfo.logLevel->globalLevel = LEVEL_ERR;
    (void)memset_s(logLevelInfo.logLevel->moduleLevel, sizeof(int) * INVLID_MOUDLE_ID,
                   LEVEL_ERR, sizeof(int) * INVLID_MOUDLE_ID);
    (void)memset_s(logLevelInfo.logLevel->moduleLevel, INVLID_MOUDLE_ID, LEVEL_ERR, INVLID_MOUDLE_ID);
    logLevelInfo.logLevel->enableEvent = EVENT_ENABLE_VALUE;

    ONE_ACT_ERR_LOG(InitModuleArrToShMem() != SYS_OK, return NULL,
                    "init module arr to shmem failed, Thread(operateLogLevel) quit.");
    HandleLogLevelChange();

    toolMsgid queueId = INVALID;
    DlogMsgT data = { 0, "" };
    ReceiveAndProcessLogLevel(queueId, data, logLevelInfo);
    SELF_LOG_ERROR("Thread(setLogLevel) quit, signal=%d.", GetSigNo());
    return NULL;
}

/**
 * @brief SetAllModuleLevel: set all module level to config file
 * @param [in]modLevel: module and level list
 * @param [in]logLevel: level id
 * @return SUCCESS: succeed; others: failed;
 */
STATIC LogRt SetAllModuleLevel(int *modLevel, int logLevel)
{
    if (modLevel == NULL) {
        SELF_LOG_WARN("[input] module level list is null.");
        return ARGV_NULL;
    }

    const ModuleInfo *moduleInfo = GetModuleInfos();
    ONE_ACT_NO_LOG(moduleInfo == NULL, return ARGV_NULL);
    // modify all module loglevel in slog.conf
    LogRt ret = SUCCESS;
    for (; moduleInfo->moduleId != INVALID; moduleInfo++) {
        modLevel[moduleInfo->moduleId] = logLevel;
        LogRt setRes = SetSlogCfgLevel(GetConfigPath(), moduleInfo->moduleName, logLevel);
        if (setRes != SUCCESS) {
            ret = (ret == SUCCESS) ? setRes : ret;
            SELF_LOG_WARN("set module level to file failed, module_name=%s, file=%s.",
                          moduleInfo->moduleName, GetConfigPath());
        }
    }

    return ret;
}

/**
 * @brief SetEventLevelValue: set event level
 * @param [in]logLevel: global level info
 * @param [in]data: level setting info, format: "[enable]"/"[disable]"
 * @return SUCCESS/LEVEL_INFO_ILLEGAL/others
 */
STATIC LogRt SetEventLevelValue(LogLevel* logLevel, char *data)
{
    if ((logLevel == NULL) || (data == NULL)) {
        SELF_LOG_WARN("[input] event level info is null.");
        return ARGV_NULL;
    }

    char *levelStr = data;
    if (*levelStr != '[') {
        SELF_LOG_WARN("event level info has no '[', level=%s.", levelStr);
        return LEVEL_INFO_ILLEGAL;
    }

    size_t len = strlen(levelStr);
    if (levelStr[len - 1] != ']') {
        SELF_LOG_WARN("event level info has no ']', level=%s.", levelStr);
        return LEVEL_INFO_ILLEGAL;
    }
    levelStr[len - 1] = '\0';
    levelStr++;

    ToUpper(levelStr);

    int levelValue = (strcmp(EVENT_ENABLE, levelStr) == 0) ? 1 : ((strcmp(EVENT_DISABLE, levelStr) == 0) ? 0 : -1);
    if (levelValue == -1) {
        SELF_LOG_WARN("event level is invalid, level=%s.", levelStr);
        return LEVEL_INFO_ILLEGAL;
    }

    LogRt res = SetSlogCfgLevel(GetConfigPath(), ENABLEEVENT_KEY, levelValue);
    if (res != SUCCESS) {
        SELF_LOG_WARN("write event level to file failed, file=%s, result=%d.", GetConfigPath(), res);
    }

    logLevel->enableEvent = levelValue;

    // write level info to shmem
    int ret = UpdateLevelToShMem(logLevel);
    if (ret != SYS_OK) {
        SELF_LOG_WARN("update level info to shmem failed.");
        res = (res == SUCCESS) ? LEVEL_NOTIFY_FAILED : res;
    }
    return res;
}

/**
 * @brief GetAllLevelConf: get all the level related config items from config file;
 * @param [out] modLevel: a array denotes the individual level for each module
 * @param [in] len: the number of modules plus 1
 * @param [out] gLevel: global log level
 * @param [out] eventLevel: event level(enable or not)
 * @return: LogRt SUCCESS/others
 */
STATIC LogRt GetAllLevelConf(int *modLevel, int len, int *gLevel, int *eventLevel)
{
    char val[CONF_VALUE_MAX_LEN + 1] = { 0 };
    if ((modLevel == NULL) || (gLevel == NULL) || (eventLevel == NULL) || (len != INVLID_MOUDLE_ID)) {
        return ARGV_NULL;
    }

    // get value of global_level
    *gLevel = MODULE_DEFAULT_LOG_LEVEL;
    LogRt ret = GetConfValueByList(GLOBALLEVEL_KEY, strlen(GLOBALLEVEL_KEY), val, CONF_VALUE_MAX_LEN);
    if ((ret == SUCCESS) && IsNaturalNumStr((const char *)val)) {
        int gValue = atoi(val);
        if ((gValue >= LOG_MIN_LEVEL) && (gValue <= LOG_MAX_LEVEL)) {
            *gLevel = gValue;
        }
    } else {
        SELF_LOG_WARN("get config item failed, config_name=%s, res=%d, use default=%s.",
                      GLOBALLEVEL_KEY, ret, GetBasicLevelNameById(*gLevel));
    }

    // get value of enableEvent
    *eventLevel = EVENT_ENABLE_VALUE;
    (void)memset_s(val, CONF_VALUE_MAX_LEN + 1, 0, CONF_VALUE_MAX_LEN + 1);
    ret = GetConfValueByList(ENABLEEVENT_KEY, strlen(ENABLEEVENT_KEY), val, CONF_VALUE_MAX_LEN);
    if ((ret == SUCCESS) && IsNaturalNumStr((const char *)val)) {
        int eventValue = atoi(val);
        if ((eventValue == EVENT_ENABLE_VALUE) || (eventValue == EVENT_DISABLE_VALUE)) {
            *eventLevel = eventValue;
        }
    } else {
        SELF_LOG_WARN("get config item failed, config_name=%s, res=%d, use default=%d, 0:disable, 1:enable.",
                      ENABLEEVENT_KEY, ret, *eventLevel);
    }

    InitModuleLevel(modLevel, len);
    return SUCCESS;
}

/**
* @brief HandleLogLevelChange: refresh log level when detect modifications in config file.
*                              read all the related confg items from cofig file to tmp variables,
*                              compare them with memo variables(global vars for host&dev, and eventLevel).
*                              if differences are detected, update global vars and sent signal to corresponding
*                              processes in process list generated by SetLogLevel thread.
*/
void HandleLogLevelChange(void)
{
    int globalLevelTmp = g_logLevel.globalLevel;
    int eventLevelTmp = g_logLevel.enableEvent;
    int ret;
    int modLevelTmp[INVLID_MOUDLE_ID]; // this array will be assigned in GetAllLevelConf before use
    LogRt err;

#ifdef _CLOUD_DOCKER
    err = UpdateConfList(GetConfigPath()); // update key-value config list, especially loglevel
    NO_ACT_WARN_LOG(err != SUCCESS, "update config list failed, result=%d.", err);
#endif

    ret = memcpy_s(modLevelTmp, sizeof(modLevelTmp), g_logLevel.moduleLevel, sizeof(modLevelTmp));
    ONE_ACT_WARN_LOG(ret != EOK, return, "memcpy failed, result=%d, strerr=%s.", ret, strerror(ToolGetErrorCode()));
    err = GetAllLevelConf(modLevelTmp, INVLID_MOUDLE_ID, &globalLevelTmp, &eventLevelTmp);
    ONE_ACT_WARN_LOG(err != SUCCESS, return, "get all level config failed, result=%d.", err);

    ONE_ACT_NO_LOG(ThreadLock() != 0, return);
    if (globalLevelTmp != g_logLevel.globalLevel) {
        SELF_LOG_INFO("global level changed, origin=%s, current=%s.",
                      GetBasicLevelNameById(g_logLevel.globalLevel),
                      GetBasicLevelNameById(globalLevelTmp));
        g_logLevel.globalLevel = globalLevelTmp;
        int moduleId = 0;
        for (; moduleId < INVLID_MOUDLE_ID; ++moduleId) {
            g_logLevel.moduleLevel[moduleId] = globalLevelTmp;
        }
    }
    if (eventLevelTmp != g_logLevel.enableEvent) {
        SELF_LOG_INFO("event level changed, origin=%d, current=%d, 0:disable, 1:enable.",
                      g_logLevel.enableEvent, eventLevelTmp);
        g_logLevel.enableEvent = eventLevelTmp;
    }
    // set single module log level
    int id = 0;
    for (; id < INVLID_MOUDLE_ID; ++id) {
        if (modLevelTmp[id] != g_logLevel.moduleLevel[id]) {
            SELF_LOG_INFO("module level changed, module=%s, origin=%s, current=%s.", \
                          GetModuleNameById(id), GetBasicLevelNameById(g_logLevel.moduleLevel[id]),
                          GetBasicLevelNameById(modLevelTmp[id]));
            g_logLevel.moduleLevel[id] = modLevelTmp[id];
        }
    }
    (void)ThreadUnLock();

    ret = UpdateLevelToShMem(&g_logLevel);
    NO_ACT_WARN_LOG(ret != SYS_OK, "update level info to shmm failed, ret=%d.", ret);
    // only update dlog level at end
    NO_ACT_WARN_LOG(SetDlogLevel() != SUCCESS, "set device level failed, result=%d.", err);
}
