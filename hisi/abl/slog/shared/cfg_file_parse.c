/**
 * @cfg_file_parse.c
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#include "cfg_file_parse.h"

#if (OS_TYPE_DEF == LINUX)
#include <sys/prctl.h>
#include <stdint.h>
#endif
#include "securec.h"
#include "log_common_util.h"
#include "share_mem.h"

#define BASE_NUM 10
#define INVALID_VALUE "INVALID"

// interface inner
STATIC void TrimString(char *str);

ConfList *g_confList = NULL;
char g_configFilePath[SLOG_CONF_PATH_MAX_LENGTH] = "\0";
STATIC toolMutex g_confMutex;
STATIC int g_cfgMutexInit = 0;

ConfList *GetGlobalConfList(void)
{
    return g_confList;
}

#if (OS_TYPE_DEF == 0)
/**
 * @brief LogSetConfigPathToShm: write config file path to share memory
 * @param [out]configPath: buffer to stor config path
 * @return: SYS_OK/SYS_ERROR
 */
int LogSetConfigPathToShm(const char *configPath)
{
    // only call by slogd when init
    if (configPath == NULL) {
        SYSLOG_WARN("[input] config path is null.\n");
        return SYS_ERROR;
    }
    unsigned int len = strlen(configPath);
    if ((len == 0) || (len > (unsigned int)SLOG_CONF_PATH_MAX_LENGTH)) {
        SYSLOG_WARN("[input] config Path length is invalid, length=%u, max_length=%d.\n",
                    len, SLOG_CONF_PATH_MAX_LENGTH);
        return SYS_ERROR;
    }
    int shmId = 0;
    if (OpenShMem(&shmId) == SHM_ERROR) {
        SYSLOG_WARN("[ERROR] OpenShMem failed, slogd maybe is not runing, please check!\n");
        return SYS_ERROR;
    }
    if (WriteToShMem(shmId, configPath, PATH_LEN, 0) != SHM_SUCCEED) {
        // if write failed, should remove the share memory
        SYSLOG_WARN("WriteToShMem failed.\n");
        return SYS_ERROR;
    }
    return SYS_OK;
}

/**
* @brief LogGetConfigPathFromShm: get config file path from shared memory
 * @param [out]configPath: buffer to stor config path
 * @param [in]len: max length of process dir path
 * @return: SYS_OK/SYS_ERROR
*/
STATIC int LogGetConfigPathFromShm(char *configPath, unsigned int len)
{
    if (configPath == NULL) {
        SYSLOG_WARN("[input] config path is null.\n");
        return SYS_ERROR;
    }
    if ((len == 0) || (len > (unsigned int)SLOG_CONF_PATH_MAX_LENGTH)) {
        SYSLOG_WARN("[input] config Path length is invalid, length=%u, max_length=%d.\n", len,
                    SLOG_CONF_PATH_MAX_LENGTH);
        return SYS_ERROR;
    }
    int shmId = 0;
    if (OpenShMem(&shmId) == SHM_ERROR) {
        return SYS_ERROR;
    }
    ShmErr ret = ReadFromShMem(shmId, configPath, len, 0);
    if (ret == SHM_ERROR) {
        return SYS_ERROR;
    }
    return SYS_OK;
}

/**
* @brief InitCfg: get config file path and assign to g_configFilePath
* @param [in] libSlogFlag: 1:call by libslog.so, 0:call by slogd\sklogd\log-daemon
* @return: SYS_OK: succeed; SYS_ERROR: failed
*/
int InitCfg(int libSlogFlag)
{
    // use defualt path
    snprintf_truncated_s(g_configFilePath, SLOG_CONF_PATH_MAX_LENGTH, "%s", SLOG_CONF_FILE_PATH);
    if (ToolAccess(g_configFilePath) != SYS_OK) {
        if (libSlogFlag) {
            // libslog.so try to get config file from shared memory.
            (void)LogGetConfigPathFromShm(g_configFilePath, SLOG_CONF_PATH_MAX_LENGTH);
        } else {
            // slogd try to get config file from the current process directory.
            (void)LogGetProcessConfigPath(g_configFilePath, SLOG_CONF_PATH_MAX_LENGTH);
        }
    }
    if (InitConfList(g_configFilePath) != SUCCESS) {
        return SYS_ERROR;
    }
    return SYS_OK;
}

/**
* @brief GetConfigPath: return config file path
* @return: return config file path
*/
char *GetConfigPath(void)
{
    return g_configFilePath;
}

/**
* @brief FreeConfigShm: free shared memory
 * @return: void
*/
void FreeConfigShm(void)
{
    RemoveShMem();
}

/**
* @brief IsConfigShmExist: judge config shared memory exist or not
* @return: SYS_OK: exist, SYS_ERROR : not exist
*/
STATIC int IsConfigShmExist(void)
{
    int shmId;
    if (OpenShMem(&shmId) == SHM_ERROR) {
        return SYS_ERROR;
    }
    return SYS_OK;
}

/**
 * @brief: if shmem not exist, then create
 * @return: SYS_OK: shmem existed or created success.
 */
int InitShm(void)
{
    int shmId = 0;

    if (IsConfigShmExist() == SYS_OK) {
        // shm exist
        FreeConfigShm();
    }
    if (CreatShMem(&shmId) == SHM_ERROR) {
        return SYS_ERROR;
    }
    return SYS_OK;
}
#else
/**
* @brief InitCfg: get config file path and assign to g_configFilePath
* @param [in] libSlogFlag: 1:call by libslog.so, 0:call by slogd\sklogd\log-daemon
* @return: SYS_OK: succeed; SYS_ERROR: failed
*/
int InitCfg(int libSlogFlag)
{
    // use defualt path
    snprintf_truncated_s(g_configFilePath, SLOG_CONF_PATH_MAX_LENGTH, "%s", SLOG_CONF_FILE_PATH);
    if (ToolAccess(g_configFilePath) != SYS_OK) {
        SYSLOG_WARN("[WARNING] Config path=%s not exist.\n", g_configFilePath);
    }
    if (InitConfList(g_configFilePath) != SUCCESS) {
        return SYS_ERROR;
    }
    return SYS_OK;
}

/**
* @brief GetConfigPath: return config file path
* @return: return config file path
*/
char *GetConfigPath(void)
{
    return g_configFilePath;
}
#endif

/**
* @brief OpenCfgFile: open config file and return file pointer
* @param [out] fp: pointer to file pointer
* @param [in] file: config file realpath include filename, it can be NULL! file length should be less than 256(PATH_MAX)
* @return: SUCCEES: succeed; others: failed
*/
STATIC LogRt OpenCfgFile(FILE **fp, const char *file)
{
    char *ppath = NULL;
    char *homeDir = NULL;

    homeDir = (char *)malloc(TOOL_MAX_PATH + 1);
    if (homeDir == NULL) {
        SELF_LOG_WARN("malloc failed, strerr=%s.", strerror(ToolGetErrorCode()));
        return MALLOC_FAILED;
    }
    (void)memset_s(homeDir, TOOL_MAX_PATH + 1, 0x00, TOOL_MAX_PATH + 1);

    if (file != NULL) {
        if (LogReplaceDefaultByDir(file, homeDir, TOOL_MAX_PATH) != SYS_OK) {
            XFREE(homeDir);
            return CFG_FILE_INVALID;
        }
    }

    // if file is NULL, then use default config file path
    ppath = RealPathCheck(file, homeDir, TOOL_MAX_PATH);
    if (ppath == NULL) {
        SELF_LOG_WARN("get realpath failed or filepath is invalid, file=%s.", file);
        XFREE(homeDir);
        return CFG_FILE_INVALID;
    }
    XFREE(homeDir);

    *fp = fopen(ppath, "r");
    if (*fp == NULL) {
        SELF_LOG_WARN("open file failed, file=%s.", ppath);
        XFREE(ppath);
        return OPEN_FILE_FAILED;
    }
    XFREE(ppath);
    return SUCCESS;
}

/**
* @brief InsertConfList: insert config item to global list
* @param [in] confName: config name string
* @param [in] nameLen: config item string length
* @param [in] confValue: config value string
* @param [in] valueLen: config value string length
* @return: SUCCEES: succeed; others: failed
*/
STATIC LogRt InsertConfList(const char *confName, unsigned int nameLen, const char *confValue, unsigned int valueLen)
{
    ONE_ACT_WARN_LOG(confName == NULL, return ARGV_NULL, "[input] config name is null.");
    ONE_ACT_WARN_LOG(confValue == NULL, return ARGV_NULL, "[input] config value is null.");
    ONE_ACT_WARN_LOG(nameLen > CONF_NAME_MAX_LEN, return ARGV_NULL,
                     "[input] config name length is invalid, length=%u, max_length=%d.",
                     nameLen, CONF_NAME_MAX_LEN);
    ONE_ACT_WARN_LOG(valueLen > CONF_VALUE_MAX_LEN, return ARGV_NULL,
                     "[input] config value length is invalid, length=%u, max_length=%d.",
                     valueLen, CONF_VALUE_MAX_LEN);

    ConfList *confListTemp = g_confList;
    ConfList *confListNode = (ConfList *)malloc(sizeof(ConfList));
    if (confListNode == NULL) {
        SELF_LOG_WARN("malloc failed, strerr=%s.", strerror(ToolGetErrorCode()));
        return MALLOC_FAILED;
    }
    (void)memset_s(confListNode, sizeof(ConfList), 0, sizeof(ConfList));

    int ret1 = strcpy_s(confListNode->confName, CONF_NAME_MAX_LEN, confName);
    int ret2 = strcpy_s(confListNode->confValue, CONF_VALUE_MAX_LEN, confValue);
    if ((ret1 != EOK) || (ret2 != EOK)) {
        SELF_LOG_WARN("strcpy_s failed, errno_1=%d, errno_2=%d.", ret1, ret2);
        XFREE(confListNode);
        return STR_COPY_FAILED;
    }
    confListNode->next = confListTemp;
    g_confList = confListNode;
    return SUCCESS;
}

/**
* @brief ParseConfigBufHelper: get config name from lineBuf
* @param [in] lineBuf: config file one line content
* @param [out] confName: config name string
* @param [in] nameLen: config name string length
* @param [out] pos: '=' position
* @return: SUCCEES: succeed; others: failed
*/
STATIC LogRt ParseConfigBufHelper(const char *lineBuf, char *confName, unsigned int nameLen, char **pos)
{
    ONE_ACT_WARN_LOG(lineBuf == NULL, return ARGV_NULL, "[input] one line is null from config file.");
    ONE_ACT_WARN_LOG(confName == NULL, return ARGV_NULL, "[output] config name is null.");
    ONE_ACT_WARN_LOG(nameLen > CONF_NAME_MAX_LEN, return ARGV_NULL,
                      "[input] config name length is invalid, length=%u, max_length=%d.",
                      nameLen, CONF_NAME_MAX_LEN);
    ONE_ACT_WARN_LOG(pos == NULL, return ARGV_NULL, "[output] file position pointer is null.");

    *pos = strchr((char *)lineBuf, '=');
    if (*pos == NULL) {
        SELF_LOG_WARN("config item has no symbol(=).");
        return LINE_NO_SYMBLE;
    }

    // ignore configName end blank before '='(eg: zip_swtich  = 1)
    // Do not convert len to unsigned, or SIGSEGV error may occurr because len reverse
    int len = *pos - lineBuf;
    while (((len - 1) >= 0) && (lineBuf[len - 1] == ' ')) {
        len = len - 1;
    }
    if (len == 0) {
        SELF_LOG_WARN("config item has no config name.");
        return CONF_VALUE_NULL;
    }

    int ret = strncpy_s(confName, nameLen, lineBuf, len);
    if (ret != EOK) {
        SELF_LOG_WARN("strncpy_s config name failed, result=%d, strerr=%s.", ret, strerror(ToolGetErrorCode()));
        return STR_COPY_FAILED;
    }

    return SUCCESS;
}

/**
* @brief HandleInvalidValue: handle invalid value
* @param [in] confName: config name string
* @param [in] pos: '=' position
* @return: SUCCEES: succeed; others: failed
*/
STATIC LogRt HandleInvalidValue(char *confName, char *pos)
{
    if ((strcmp(confName, GLOBALLEVEL_KEY) == 0) || (strcmp(confName, ENABLEEVENT_KEY) == 0) ||
        (GetModuleInfoByNameForC(confName) != NULL)) {
        // config value is empty, user "INVALID" instead
        int ret = strcpy_s(pos, CONF_VALUE_MAX_LEN, INVALID_VALUE);
        ONE_ACT_WARN_LOG(ret != EOK, return STR_COPY_FAILED, "copy config value failed, config_name=%s, strerr=%s.",
                         confName, strerror(ToolGetErrorCode()));
    } else {
        SELF_LOG_WARN("config value is empty, config_name=%s.", confName);
        return GET_NO_VALUE_ERR;
    }
    return SUCCESS;
}

/**
* @brief ParseConfigBuf: get config name and its value
* @param [in] lineBuf: config file one line content
* @param [out] confName: config name string
* @param [in] nameLen: config name string length
* @param [out] confValue: config vaulue string
* @param [in] valueLen: config vaulue string length
* @return: SUCCEES: succeed; others: failed
*/
STATIC LogRt ParseConfigBuf(const char *lineBuf, char *confName, unsigned int nameLen,
                            char *confValue, unsigned int valueLen)
{
    char *pos = NULL;
    char buff[CONF_VALUE_MAX_LEN + 1] = "\0";
    unsigned int start = 0;

    LogRt res = ParseConfigBufHelper(lineBuf, confName, nameLen, &pos);
    if (res != SUCCESS) {
        return CONF_VALUE_NULL;
    }

    ++pos;
    int ret = strcpy_s(buff, CONF_VALUE_MAX_LEN, pos);
    ONE_ACT_WARN_LOG(ret != EOK, return STR_COPY_FAILED,
                     "strcpy_s config value to buffer failed, result=%d, strerr=%s.",
                     ret, strerror(ToolGetErrorCode()));

    // delete blank
    while ((buff[start] != '\0') && ((buff[start] == ' ') || (buff[start] == '\t'))) {
        start++;
    }

    unsigned int strLen = strlen(buff);
    while ((strLen > 0) && ((buff[strLen - 1] == '\r') || (buff[strLen - 1] == '\n') || (buff[strLen - 1] == '\t'))) {
        buff[strlen(buff) - 1] = 0;
        strLen = strlen(buff);
        if (strLen == 0) {
            if ((strcmp(confName, GLOBALLEVEL_KEY) == 0) || (strcmp(confName, ENABLEEVENT_KEY) == 0) ||
                (GetModuleInfoByNameForC(confName) != NULL)) {
                // no config value after the symbol(=), user "INVALID" instead
                ret = strcpy_s(buff, CONF_VALUE_MAX_LEN, INVALID_VALUE);
                start = 0;
                ONE_ACT_WARN_LOG(ret != EOK, return STR_COPY_FAILED,
                                 "copy config value failed, config_name=%s, strerr=%s.",
                                 confName, strerror(ToolGetErrorCode()));
            } else {
                SELF_LOG_WARN("no config value after the symbol(=), config_name=%s.", confName);
                return CONF_VALUE_NULL;
            }
        }
    }

    // ignore comment and value split by space after config value
    TrimString(&(buff[start]));

    pos = &(buff[start]);
    if (strlen(pos) == 0) {
        res = HandleInvalidValue(confName, pos);
        ONE_ACT_WARN_LOG(res != SUCCESS, return res, "handle invalid value failed, confName=%s.", confName);
    }

    ret = strcpy_s(confValue, valueLen, pos);
    ONE_ACT_WARN_LOG(ret != EOK, return NO_ENOUTH_SPACE, "copy config value failed, result=%d, strerr=%s.",
                     ret, strerror(ToolGetErrorCode()));

    return SUCCESS;
}

STATIC bool IsBlankline(char ch)
{
    return (ch == '#') || (ch == '\n') || (ch == '\r');
}

STATIC bool IsBlank(char ch)
{
    return (ch == ' ') || (ch == '\t');
}

/**
* @brief InitConfList: config item list init from config file
* @param [in] file: config file realpath include filename, it can be NULL
* @return: SUCCEES: succeed; others: failed
*/
LogRt InitConfList(const char *file)
{
    char confName[CONF_NAME_MAX_LEN + 1] = { 0 };
    char confValue[CONF_VALUE_MAX_LEN + 1] = { 0 };
    char buf[CONF_FILE_MAX_LINE + 1] = { 0 };
    char tmpBuf[CONF_FILE_MAX_LINE + 1] = { 0 };
    FILE *fp = NULL;

    if (g_cfgMutexInit == 0) {
        if (ToolMutexInit(&g_confMutex) == SYS_OK) {
            g_cfgMutexInit = 1;
        } else {
            SELF_LOG_WARN("init config mutex failed, strerr=%s.", strerror(ToolGetErrorCode()));
            return MUTEX_INIT_ERR;
        }
    }

    // if file is NULL, then use default config file path
    LogRt res = OpenCfgFile(&fp, file);
    if (res != SUCCESS) {
        SELF_LOG_WARN("open config file failed, file=%s, result=%d, strerr=%s.",
                      file, res, strerror(ToolGetErrorCode()));
        return OPEN_FILE_FAILED;
    }

    int ret = fseek(fp, 0L, SEEK_SET);
    if (ret < 0) {
        SELF_LOG_WARN("fseek config file failed, file=%s, result=%d, strerr=%s.",
                      file, ret, strerror(ToolGetErrorCode()));
        LOG_CLOSE_FILE(fp);
        return MV_FILE_PNTR_ERR;
    }

    while (fgets(buf, CONF_FILE_MAX_LINE, fp) != NULL) {
        unsigned int start = 0;
        if (IsBlankline(*buf)) {
            continue;
        }

        while (IsBlank(buf[start])) {
            start++;
        }

        ret = strcpy_s(tmpBuf, sizeof(tmpBuf) - 1, (buf + start));
        ONE_ACT_WARN_LOG(ret != EOK, continue, "strcpy_s config item failed, result=%d, strerr=%s.",
                         ret, strerror(ToolGetErrorCode()));

        res = ParseConfigBuf(tmpBuf, confName, CONF_NAME_MAX_LEN, confValue, CONF_VALUE_MAX_LEN);
        ONE_ACT_WARN_LOG(res != SUCCESS, continue, "parse one line config item failed, result=%d, strerr=%s.",
                         res, strerror(ToolGetErrorCode()));

        res = InsertConfList(confName, CONF_NAME_MAX_LEN, confValue, CONF_VALUE_MAX_LEN);
        ONE_ACT_WARN_LOG(res != SUCCESS, continue, "init config list failed, result=%d, strerr=%s.",
                         res, strerror(ToolGetErrorCode()));
    }

    LOG_CLOSE_FILE(fp);
    return SUCCESS;
}

/**
* @brief UpdateConfList: config item list update from config file
* @param [in] file: config file realpath include filename, it can be NULL
* @return: SUCCEES: succeed; others: failed
*/
LogRt UpdateConfList(const char *file)
{
    LOCK_WARN_LOG(&g_confMutex);
    ConfList *confListTmp = g_confList;
    ConfList *confListNode = NULL;

    while (confListTmp != NULL) {
        confListNode = confListTmp;
        confListTmp = confListTmp->next;
        XFREE(confListNode);
    }
    g_confList = NULL;
    LogRt result = InitConfList(file);
    UNLOCK_WARN_LOG(&g_confMutex);
    return result;
}

void FreeConfList(void)
{
    LOCK_WARN_LOG(&g_confMutex);
    ConfList *confListTmp = g_confList;
    ConfList *confListNode = NULL;

    while (confListTmp != NULL) {
        confListNode = confListTmp;
        confListTmp = confListTmp->next;
        XFREE(confListNode);
    }
    g_confList = NULL;
    UNLOCK_WARN_LOG(&g_confMutex);
    (void)ToolMutexDestroy(&g_confMutex);
    g_cfgMutexInit = 0;
    return;
}

/**
* @brief GetConfValueByList: get config value by config value
* @param [in] confName: config name string
* @param [in] nameLen: config item string length
* @param [out] confValue: config value string
* @param [in] valueLen: config value string length
* @return: SUCCEES: succeed; others: failed
*/
LogRt GetConfValueByList(const char *confName, unsigned int nameLen, char *confValue, unsigned int valueLen)
{
    ONE_ACT_WARN_LOG(confName == NULL, return ARGV_NULL, "[input] config name is null.");
    ONE_ACT_WARN_LOG(confValue == NULL, return ARGV_NULL, "[output] config value is null.");
    ONE_ACT_WARN_LOG(nameLen > CONF_NAME_MAX_LEN, return ARGV_NULL,
                     "[input] config name length is invalid, length=%u, max_length=%d.",
                      nameLen, CONF_NAME_MAX_LEN);
    ONE_ACT_WARN_LOG(valueLen > CONF_VALUE_MAX_LEN, return ARGV_NULL,
                     "[input] config value length is invalid, length=%u, max_length=%d.",
                     valueLen, CONF_VALUE_MAX_LEN);

    LOCK_WARN_LOG(&g_confMutex);
    const ConfList *confListTmp = g_confList;
    while (confListTmp != NULL) {
        if (strcmp(confName, confListTmp->confName) == 0) {
            int ret = strcpy_s(confValue, valueLen, confListTmp->confValue);
            if (ret != EOK) {
                SELF_LOG_WARN("strcpy_s config value failed, result=%d, strerr=%s.", ret, strerror(ToolGetErrorCode()));
                UNLOCK_WARN_LOG(&g_confMutex);
                return STR_COPY_FAILED;
            }
            UNLOCK_WARN_LOG(&g_confMutex);
            return SUCCESS;
        }
        confListTmp = confListTmp->next;
    }
    UNLOCK_WARN_LOG(&g_confMutex);
    return CONF_VALUE_NULL;
}

bool IsPathValidByConfig(const char *ppath, unsigned int pathLen)
{
    ONE_ACT_WARN_LOG(ppath == NULL, return false, "[input] file realpath is null.");
    ONE_ACT_WARN_LOG(pathLen == 0, return false,
                     "[input] filepath length is invalid, path_length=%u.", pathLen);

    const char *buffix1 = strstr(ppath, CFG_FILE_BUFFIX1);
    const char *buffix2 = strstr(ppath, CFG_FILE_BUFFIX2);
    const char *buffix3 = strstr(ppath, CFG_FILE_BUFFIX3);
    if (((buffix1 != NULL) && (strcmp(buffix1, CFG_FILE_BUFFIX1) == 0)) ||
        ((buffix2 != NULL) && (strcmp(buffix2, CFG_FILE_BUFFIX2) == 0)) ||
        ((buffix3 != NULL) && (strcmp(buffix3, CFG_FILE_BUFFIX3) == 0))) {
        return true;
    }
    return false;
}

bool IsPathValidbyLog(const char *ppath, unsigned int pathLen)
{
    const char *suffix = ".log";

    ONE_ACT_WARN_LOG(ppath == NULL, return false, "[input] file realpath is null.");
    ONE_ACT_NO_LOG((pathLen == 0) || (pathLen < (unsigned int)strlen(suffix)), return false);

    unsigned int len = pathLen - strlen(suffix);
    unsigned int i = len;
    for (; i < pathLen; i++) {
        if (ppath[i] != *suffix) {
            return false;
        }
        suffix++;
    }
    return true;
}

#if (OS_TYPE_DEF == LINUX)
int SetThreadName(const char *threadName)
{
    int ret = prctl(PR_SET_NAME, (unsigned long)(uintptr_t)threadName);
    return ret;
}
#else
int SetThreadName(const char *threadName)
{
    return 0;
}
#endif

char *RealPathCheck(const char *file, const char *homeDir, unsigned int dirLen)
{
    char *ppath = NULL;

    if (homeDir == NULL) {
        SELF_LOG_WARN("[input] home directory is null.");
        return NULL;
    }

    if ((dirLen > TOOL_MAX_PATH) || (dirLen == 0)) {
        SELF_LOG_WARN("[input] directory length is invalid, " \
                      "directory_length=%u, max_length=%d.", dirLen, TOOL_MAX_PATH);
        return NULL;
    }

    ppath = (char *)malloc(dirLen + 1);
    if (ppath == NULL) {
        SELF_LOG_WARN("malloc failed, strerr=%s.", strerror(ToolGetErrorCode()));
        return NULL;
    }
    (void)memset_s(ppath, dirLen + 1, 0, dirLen + 1);

    if (ToolRealPath((file != NULL) ? homeDir : SLOG_CONF_FILE_PATH, ppath, dirLen + 1) != SYS_OK) {
        SELF_LOG_WARN("get realpath failed, file=%s, strerr=%s.", file, strerror(ToolGetErrorCode()));
        XFREE(ppath);
        return NULL;
    }

    unsigned int cfgFileLen = strlen(ppath);
    if (IsPathValidByConfig(ppath, cfgFileLen) == false) {
        SELF_LOG_WARN("realpath is invalid, realpath=%s.", ppath);
        XFREE(ppath);
        return NULL;
    }

    return ppath;
}

// truncate string from first blank char or '#' after given position
STATIC void TrimString(char *str)
{
    ONE_ACT_NO_LOG(str == NULL, return);

    const char *head = str;
    while (*str != '\0') {
        if ((*str == '\t') || (*str == '#')) {
            *str = '\0';
            break;
        }
        str++;
    }
    if (head == str) {
        return;
    }
    str--;
    while ((str >= head) && (*str == ' ')) {
        *str = '\0';
        str--;
    }
}

// the str should not over INT_MAX, otherwise return false
bool IsNaturalNumStr(const char *str)
{
    ONE_ACT_NO_LOG((str == NULL) || (*str == '\0'), return false);

    if (*str == '0') {
        str++;
        return *str == '\0';
    }

    long long totalNum = 0;
    while (*str != '\0') {
        if ((*str > '9') || (*str < '0')) {
            return false;
        }

        // to check if str is over INT_MAX(2147483647)
        totalNum = (totalNum * BASE_NUM) + (*str - '0');
        if (totalNum > INT_MAX) {
            SELF_LOG_WARN("parse number_string to int failed, number_string=%s.", str);
            return false;
        }
        str++;
    }

    return true;
}

void InitCfgAndSelfLogPath(void)
{
    int isLibSlog = 0;
    if (InitCfg(isLibSlog) == SYS_OK) {
        if (InitFilePathForSelfLog() != SYS_OK) {
            SYSLOG_WARN("[WARNING] Init file path for self log failed and continue....\n");
        } else {
            SELF_LOG_INFO("Init conf and self log path success.");
        }
    }
}

void FreeCfgAndSelfLogPath(void)
{
    FreeConfList();
    FreeSelfLogFiles();
}
