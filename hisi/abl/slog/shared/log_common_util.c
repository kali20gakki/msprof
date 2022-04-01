/**
 * @log_common_util.c
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#include "log_common_util.h"
#include "cfg_file_parse.h"
#include "print_log.h"
#if defined(EP_DEVICE_MODE) || defined(RC_MODE)
#include "ascend_hal.h"
#endif

char g_rootLogPath[CFG_LOGAGENT_PATH_MAX_LENGTH + 1] = "\0";
char g_selfLogDir[LOG_DIR_FOR_SELF_LENGTH + 1] = "\0";
char g_workSpacePath[CFG_WORKSPACE_PATH_MAX_LENGTH + 1] = "\0";
char g_socketFilePath[WORKSPACE_PATH_MAX_LENGTH + 1] = "\0";

char *GetSocketPath(void)
{
    if (strlen(g_socketFilePath) != 0) {
        return g_socketFilePath;
    }

    if (StrcatDir(g_socketFilePath, SOCKET_FILE,
        GetWorkspacePath(), WORKSPACE_PATH_MAX_LENGTH) != SYS_OK) {
        SELF_LOG_WARN("get g_socketFilePath failed.");
    }
    return g_socketFilePath;
}

static inline int GetPermissionForAllUserFlag(void)
{
    int flag = 0;

    char val[CONF_VALUE_MAX_LEN + 1] = { 0 };
    LogRt ret = GetConfValueByList(PERMISSION_FOR_ALL, strlen(PERMISSION_FOR_ALL), val, CONF_VALUE_MAX_LEN);
    if ((ret == SUCCESS) && IsNaturalNumStr((const char *)val)) {
        int tmp = atoi(val);
        flag = (tmp == 1) ? tmp : 0;
    }
    return flag;
}

/**
 * @brief SyncGroupToOther: sync group permission to others, for example, input 0750, return 0755
 * @param [in]perm: current permission
 * @return: new permission
 */
toolMode SyncGroupToOther(toolMode perm)
{
#if (OS_TYPE_DEF == 0)
    if (GetPermissionForAllUserFlag()) {
        perm = (perm & S_IRGRP) ? (perm | S_IROTH) : perm;
        perm = (perm & S_IWGRP) ? (perm | S_IWOTH) : perm;
        perm = (perm & S_IXGRP) ? (perm | S_IXOTH) : perm;
    }
#endif
    return perm;
}

static inline int IsLeapYear(int year)
{
    return ((((year % 4) == 0) && ((year % 100) != 0)) || ((year % 400) == 0)) ? 1 : 0; // leap year
}

/**
 * @brief CalLocalTime: calculate local time
 * @param [in/out]timeInfo: local time struct
 * @param [in]sec: seconds from 1970/1/1
 * @param [in]tzone: current time zone
 * @param [in]dst: daylight time
 * @return: void
 */
static void CalLocalTime(struct tm *timeInfo, time_t sec, time_t tzone, int dst)
{
    const time_t oneMin = 60; // 1m: 60s
    const time_t oneHour = 3600; // 1h: 3600s
    const time_t oneDay = 86400; // 24h: 86400s
    const int oneYear = 365; // 365 days

    sec -= tzone; // Adjust for timezone
    sec += oneHour * dst; // Adjust for daylight time
    time_t days = sec / oneDay; // Days passed since epoch
    time_t seconds = sec % oneDay; // Remaining seconds

    timeInfo->tm_isdst = dst;
    timeInfo->tm_hour = seconds / oneHour;
    timeInfo->tm_min = (seconds % oneHour) / oneMin;
    timeInfo->tm_sec = (seconds % oneHour) % oneMin;

    // 1/1/1970 was a Thursday, that is, day 4 from the POV of the tm structure * where sunday = 0,
    // so to calculate the day of the week we have to add 4 * and take the modulo by 7.
    timeInfo->tm_wday = (days + 4) % 7; // start from Thursday
    // Calculate the current year
    timeInfo->tm_year = 1970; // start from 1970
    while (1) {
        // Leap years have one day more
        time_t yearDays = oneYear + IsLeapYear(timeInfo->tm_year);
        if (yearDays > days) {
            break;
        }
        days -= yearDays;
        timeInfo->tm_year++;
    }
    timeInfo->tm_yday = days; // Number of day of the current year

    // We need to calculate in which month and day of the month we are. To do *,
    // so we need to skip days according to how many days there are in each * month,
    // and adjust for the leap year that has one more day in February.
    int mDays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}; // days of month: 31, 30, 28/29
    mDays[1] += IsLeapYear(timeInfo->tm_year); // leap year

    timeInfo->tm_mon = 0;
    while (days >= mDays[timeInfo->tm_mon]) {
        days -= mDays[timeInfo->tm_mon];
        timeInfo->tm_mon++;
    }

    timeInfo->tm_mon++; // Add 1 since our 'month' is zero-based
    timeInfo->tm_mday = days + 1; // Add 1 since our 'days' is zero-based
}

/**
 * @brief GetLocaltimeR: calculate local time
 * @param [in/out]timeInfo: local time struct
 * @param [in]sec: seconds from 1970/1/1
 * @return: SYS_OK/SYS_ERROR
 */
INT32 GetLocaltimeR(struct tm *timeInfo, time_t sec)
{
#if (OS_TYPE_DEF == 0)
    ONE_ACT_NO_LOG(timeInfo == NULL, return SYS_ERROR);

    CalLocalTime(timeInfo, sec, timezone, 0);
#endif
    return SYS_OK;
}

#if (OS_TYPE_DEF == LINUX)
/**
 * @brief GetUserGroupId: get user id & group id
 * @param [out]uid: user id for specified user
 * @param [out]gid: group id for specified user
 * @return: USER_OK(0): SUCCESS, others: failed
 */
int GetUserGroupId(unsigned int *uid, unsigned int *gid)
{
    ONE_ACT_WARN_LOG(uid == NULL, return USER_ERROR, "[input] user id is null.");
    ONE_ACT_WARN_LOG(gid == NULL, return USER_ERROR, "[input] group id is null.");

    const struct passwd *secuWord = getpwuid(getuid());
    if (secuWord != NULL) {
        *uid = secuWord->pw_uid;
        *gid = secuWord->pw_gid;
        return USER_OK;
    } else {
        SELF_LOG_WARN("user not found.");
        return USER_NOT_FOUND;
    }
}

/**
 * @brief ChangePathOwner: Change file path owner
 * @param [in]path: file path to be changed
 * @return: NA
 */
void ChangePathOwner(const char *path)
{
    if ((path == NULL) || (strlen(path) == 0)) {
        SELF_LOG_WARN("[input] path is null.");
        return;
    }

    unsigned int uid = 0;
    unsigned int gid = 0;
    int ret = GetUserGroupId(&uid, &gid);
    if (ret != USER_OK) {
        return;
    }

    ret = lchown(path, uid, gid);
    if (ret != 0) {
        SELF_LOG_WARN("chown path failed, filepath=%s, result=%d, strerr=%s.", path, ret, strerror(ToolGetErrorCode()));
    }
}
#else
int GetUserGroupId(unsigned int *uid, unsigned int *gid)
{
    return USER_NOT_FOUND;
}
#endif

/**
* @brief LogAgentChangeLogPathMode: change path owner
* @param [in] path: file/folder path
* @return: LogRt
*/
LogRt LogAgentChangeLogPathMode(const char *path)
{
#if (OS_TYPE_DEF == LINUX)
    ONE_ACT_NO_LOG(path == NULL, return ARGV_NULL);

    unsigned int uid = 0;
    unsigned int gid = 0;

    int ret = GetUserGroupId(&uid, &gid);
    if ((ret != USER_OK) && (ret != USER_NOT_FOUND)) {
        return GET_USERID_FAILED;
    }

    if (ret == USER_OK) {
        ret = ToolChown(path, uid, gid);
        ONE_ACT_NO_LOG(ret != 0, return CHOWN_FAILED);
    }
#endif

    return SUCCESS;
}

/**
* @brief LogAgentChangeLogFdMode: change fd owner
* @param [in] fd: file discriptor
* @return: OK: succeed; NOK: failed
*/
LogRt LogAgentChangeLogFdMode(int fd)
{
#if (OS_TYPE_DEF == LINUX)
    ONE_ACT_NO_LOG(fd <= 0, return INPUT_INVALID);

    unsigned int uid = 0;
    unsigned int gid = 0;

    int ret = GetUserGroupId(&uid, &gid);
    if ((ret != USER_OK) && (ret != USER_NOT_FOUND)) {
        return GET_USERID_FAILED;
    }

    if (ret == USER_OK) {
        ret = fchown(fd, uid, gid);
        ONE_ACT_NO_LOG(ret < 0, return CHOWN_FAILED);
    }
#endif

    return SUCCESS;
}

/**
 * @brief MkdirIfNotExist: mkdir log path if not exist
 * @param [in]dirPath: log directory path
 * @return: LogRt
 */
LogRt MkdirIfNotExist(const char *dirPath)
{
    ONE_ACT_NO_LOG(dirPath == NULL, return ARGV_NULL);

    if (ToolAccess(dirPath) != SYS_OK) {
        int ret = ToolMkdir(dirPath, (toolMode)OPERATE_MODE);
        if ((ret != SYS_OK) && (ToolAccess(dirPath) != SYS_OK)) {
            return MKDIR_FAILED;
        }
        (void)ToolChmod(dirPath, (toolMode)OPERATE_MODE);
        LogRt err = LogAgentChangeLogPathMode(dirPath);
        if (err != SUCCESS) {
            return err;
        }
    }

    return SUCCESS;
}

/**
 * @brief CheckLogDirPermission: check log path permission and try to create if path not exist
 * @param [in]dirPath: log root path
 * @return: void
 */
STATIC void CheckLogDirPermission(const char *dirPath)
{
    ONE_ACT_NO_LOG(dirPath == NULL, return);

    if (ToolAccess(dirPath) == SYS_OK) {
        // check log path permission
        if (ToolAccessWithMode(dirPath, F_OK | W_OK | R_OK | X_OK) != SYS_OK) {
            SYSLOG_WARN("log path permission denied, strerr=%s.\n", strerror(ToolGetErrorCode()));
        }
    } else {
        // check log path create permission
        LogRt ret = MkdirIfNotExist(dirPath);
        if (ret != SUCCESS) {
            SYSLOG_WARN("create log path failed, ret=%d, strerr=%s.\n", ret, strerror(ToolGetErrorCode()));
        }
    }
}

/**
 * @brief LogGetHomeDir: get user's home directory
 * @param [out]homeDir: buffer to store home dir path
 * @param [in]len: max length of home dir path
 * @return: SYS_OK/SYS_ERROR
 */
int LogGetHomeDir(char *const homeDir, unsigned int len)
{
    ONE_ACT_WARN_LOG(homeDir == NULL, return SYS_ERROR, "[input] home directory path is null.");
    ONE_ACT_WARN_LOG((len == 0) || (len > (unsigned int)(TOOL_MAX_PATH + 1)), return SYS_ERROR,
                      "[input] path length is invalid, length=%u, max_length=%d.", len, TOOL_MAX_PATH);

    int ret;
#if (OS_TYPE_DEF == LINUX)
    const struct passwd *secuWord = getpwuid(getuid());
    if (secuWord != NULL) {
        ret = strcpy_s(homeDir, len, secuWord->pw_dir);
    } else {
        ret = strcpy_s(homeDir, len, "");
    }
    if (ret != EOK) {
        SELF_LOG_WARN("strcpy_s home directory failed, result=%d, strerr=%s.", ret, strerror(ToolGetErrorCode()));
        return SYS_ERROR;
    }

    SELF_LOG_INFO("home_directory=%s.", homeDir);
    return SYS_OK;
#else
    ret = strcpy_s(homeDir, len, LOG_HOME_DIR);
    if (ret != EOK) {
        SELF_LOG_WARN("strcpy_s failed, result=%d, strerr=%s.", ret, strerror(ToolGetErrorCode()));
        return SYS_ERROR;
    }
    return SYS_OK;
#endif
}


/**
 * @brief LogGetProcessPath: get current process directory
 * @param [out]processDir: buffer to stor current process directory path
 * @param [in]len: max length of process dir path
 * @return: SYS_OK/SYS_ERROR
 */
#if (OS_TYPE_DEF == 0)
int LogGetProcessPath(char *processDir, unsigned int len)
{
    if (processDir == NULL) {
        SYSLOG_WARN("[input] process directory path is null.");
        return SYS_ERROR;
    }
    const char *selfBin = "/proc/self/exe";
    int selflen = readlink(selfBin, processDir, len); // read self path of store
    if ((selflen < 0) || (selflen > TOOL_MAX_PATH)) {
        SYSLOG_WARN("[ERROR] Get self bin directory failed, selflen=%d, strerr=%s.", selflen,
                    strerror(ToolGetErrorCode()));
        return SYS_ERROR;
    }
    return SYS_OK;
}

/**
 * @brief LogGetProcessConfigPath: get config file from current process directory
 * @param [out]configPath: buffer to stor config path
 * @param [in]len: max length of process dir path
 * @return: SYS_OK/SYS_ERROR
 */
int LogGetProcessConfigPath(char *configPath, unsigned int len)
{
    if (configPath == NULL) {
        SYSLOG_WARN("[input] config path is null.\n");
        return SYS_ERROR;
    }
    char *tmp = (char *)malloc(TOOL_MAX_PATH + 1);
    if (tmp == NULL) {
        SYSLOG_WARN("tmp malloc failed, strerr=%s.\n", strerror(ToolGetErrorCode()));
        return SYS_ERROR;
    }
    (void)memset_s(tmp, TOOL_MAX_PATH + 1, '\0', TOOL_MAX_PATH + 1);
    int ret = LogGetProcessPath(tmp, TOOL_MAX_PATH);
    if (ret != SYS_OK) {
        SYSLOG_WARN("[ERROR] Get process path failed.\n");
        XFREE(tmp);
        return SYS_ERROR;
    }
    const char *pend = strrchr(tmp, OS_SPLIT);
    if (pend == NULL) {
        SYSLOG_WARN("[ERROR] Config path has no \"\\\".\n");
        XFREE(tmp);
        return SYS_ERROR;
    }
    unsigned int endlen = pend - tmp + 1;
    ret = strncpy_s(configPath, len, tmp, endlen);
    if (ret != EOK) {
        SYSLOG_WARN("[ERROR] strcpy_s failed, result=%d, strerr=%s.\n", ret, strerror(ToolGetErrorCode()));
        XFREE(tmp);
        return SYS_ERROR;
    }
    XFREE(tmp);
    if (len < (unsigned int)(strlen(configPath) + strlen(LOG_CONFIG_FILE) + 1)) {
        SYSLOG_WARN("[ERROR] Path length more than upper limit, upper_limit=%u, configPath=%s.\n", len, configPath);
        return SYS_ERROR;
    }
    ret = strcat_s(configPath, len, LOG_CONFIG_FILE);
    if (ret != EOK) {
        SYSLOG_WARN("[ERROR] strcat_s failed, configPath=%s, result=%d, strerr=%s.\n",
                    configPath, ret, strerror(ToolGetErrorCode()));
        return SYS_ERROR;
    }
    return SYS_OK;
}
#endif

/**
 * @brief LogReplaceDefaultByDir: replace ~ with home dir path
 * @param [in]path: path to be replaced
 * @param [in]homeDir: home dir path
 * @param [in]len: max length of path
 * @return: SYS_OK/SYS_ERROR
 */
int LogReplaceDefaultByDir(const char *path, char *homeDir, unsigned int len)
{
    ONE_ACT_WARN_LOG(path == NULL, return SYS_ERROR, "[input] path is null.");
    ONE_ACT_WARN_LOG(homeDir == NULL, return SYS_ERROR, "[input] home directory path is null.");
    ONE_ACT_WARN_LOG((len == 0) || (len > (unsigned int)(TOOL_MAX_PATH + 1)), return SYS_ERROR,
                      "[input] path length is invalid, length=%u, max_length=%d.", len, TOOL_MAX_PATH);

    if (path[0] != '~') {
        int err = strcpy_s(homeDir, len, path);
        if (err != EOK) {
            SELF_LOG_WARN("strcpy_s path failed, result=%d, strerr=%s.", err, strerror(ToolGetErrorCode()));
            return SYS_ERROR;
        }
        return SYS_OK;
    }

    int ret = LogGetHomeDir(homeDir, len);
    if (ret != SYS_OK) {
        SELF_LOG_WARN("get home directory failed.");
        return SYS_ERROR;
    }
    path++;

    if (len < (unsigned int)(strlen(homeDir) + strlen(path) + 1)) {
        SELF_LOG_WARN("path length more than upper limit, upper_limit=%u, homeDir=%s, path=%s.", len, homeDir, path);
        return SYS_ERROR;
    }
    ret = strcat_s(homeDir, len, path);
    if (ret != EOK) {
        SELF_LOG_WARN("strcat_s failed, home_directory=%s, path=%s, result=%d, strerr=%s.",
                      homeDir, path, ret, strerror(ToolGetErrorCode()));
        return SYS_ERROR;
    }

    return SYS_OK;
}

int StartsWith(const char *str, const char *pattern)
{
    if ((str == NULL) || (pattern == NULL)) {
        return 0;
    }
    return (strncmp(str, pattern, strlen(pattern)) == 0);
}

/**
 * @brief SetWorkspacePath: set workpath to global value
 * @param [in]path: workpath string
 * @return: SYS_OK/SYS_ERROR
 */
int SetWorkspacePath(const char *path)
{
    ONE_ACT_NO_LOG(path == NULL, return SYS_ERROR);

    int ret = strncpy_s(g_workSpacePath, CFG_WORKSPACE_PATH_MAX_LENGTH + 1, path, strlen(path));
    if (ret != EOK) {
        SELF_LOG_WARN("set workpath failed, ret=%d, strerr=%s, pid=%d.",
                      ret, strerror(ToolGetErrorCode()), ToolGetPid());
        return SYS_ERROR;
    }
    return SYS_OK;
}

char *GetWorkspacePath(void)
{
    return g_workSpacePath;
}

char *GetRootLogPath(void)
{
    return g_rootLogPath;
}

char *GetLogAgentPath(void)
{
    return g_selfLogDir;
}

/**
 * @brief LogInitWorkspacePath: get workspace path from config file
 * @return: SYS_OK/SYS_ERROR
 */
int LogInitWorkspacePath(void)
{
    if (strlen(g_workSpacePath) != 0) {
        SELF_LOG_INFO("g_workSpacePath has already been initialize.");
        return SYS_OK;
    }
    char val[CONF_VALUE_MAX_LEN + 1] = { 0 };
    LogRt res = GetConfValueByList(LOG_WORKSPACE_STR, strlen(LOG_WORKSPACE_STR), val, CONF_VALUE_MAX_LEN);
    if ((res != SUCCESS)) {
        // use default path /usr/slog
        snprintf_truncated_s(g_workSpacePath, CFG_WORKSPACE_PATH_MAX_LENGTH + 1, "%s", DEFAULT_LOG_WORKSPACE);
    } else {
        char *path = (char *)malloc(TOOL_MAX_PATH + 1);
        if (path == NULL) {
            SELF_LOG_WARN("malloc failed, strerr=%s.", strerror(ToolGetErrorCode()));
            return SYS_ERROR;
        }
        (void)memset_s(path, TOOL_MAX_PATH + 1, 0, TOOL_MAX_PATH + 1);
        if (ToolRealPath((const CHAR *)val, path, TOOL_MAX_PATH + 1) != SYS_OK) {
            SELF_LOG_WARN("get realpath failed, val=%s, strerr=%s.", val, strerror(ToolGetErrorCode()));
            XFREE(path);
            return SYS_ERROR;
        }
        int ret = strncpy_s(g_workSpacePath, CFG_WORKSPACE_PATH_MAX_LENGTH + 1, path, CFG_WORKSPACE_PATH_MAX_LENGTH);
        if (ret != EOK) {
            SELF_LOG_WARN("copy workspace path failed, err=%d.", ret);
            XFREE(path);
            return SYS_ERROR;
        }
        XFREE(path);
    }
    if (ToolAccessWithMode(g_workSpacePath, F_OK | R_OK | X_OK) != SYS_OK) {
        SELF_LOG_WARN("Not access workspace path, g_workSpacePath=%s.", g_workSpacePath);
        return SYS_ERROR;
    }
    SELF_LOG_INFO("logWorkspace = %s.", g_workSpacePath);
    return SYS_OK;
}

/**
 * @brief LogInitLogAgentPath: get self log dir from config file
 * @return: SYS_OK/SYS_ERROR
 */
int LogInitLogAgentPath(void)
{
    // g_selfLogDir has already been initialize.
    ONE_ACT_NO_LOG(strlen(g_selfLogDir) != 0, return SYS_OK);

    int ret;
    char val[CONF_VALUE_MAX_LEN + 1] = "\0";
    LogRt err = GetConfValueByList(LOG_AGENT_FILE_DIR_STR, strlen(LOG_AGENT_FILE_DIR_STR),
                                   val, CONF_VALUE_MAX_LEN);
    if (err != SUCCESS) {
        ret = snprintf_s(g_rootLogPath, CFG_LOGAGENT_PATH_MAX_LENGTH + 1,
                         CFG_LOGAGENT_PATH_MAX_LENGTH, "%s", LOG_FILE_PATH);
        if (ret == -1) {
            SYSLOG_WARN("snprintf_s failed, log_path=%s, strerr=%s.\n", LOG_FILE_PATH, strerror(ToolGetErrorCode()));
            return SYS_ERROR;
        }
    } else {
        char *path = (char *)malloc(TOOL_MAX_PATH + 1);
        if (path == NULL) {
            SYSLOG_WARN("malloc failed, strerr=%s.", strerror(ToolGetErrorCode()));
            return SYS_ERROR;
        }
        (void)memset_s(path, TOOL_MAX_PATH + 1, 0, TOOL_MAX_PATH + 1);
        ret = ToolRealPath((const CHAR *)val, path, TOOL_MAX_PATH + 1);
        if ((ret != SYS_OK) && (ToolGetErrorCode() != ENOENT)) {
            SYSLOG_WARN("get realpath failed, val=%s, strerr=%s.", val, strerror(ToolGetErrorCode()));
            XFREE(path);
            return SYS_ERROR;
        }
        ret = strncpy_s(g_rootLogPath, CFG_LOGAGENT_PATH_MAX_LENGTH + 1, path, CFG_LOGAGENT_PATH_MAX_LENGTH);
        if (ret != EOK) {
            SYSLOG_WARN("get log dir realpath failed, ret=%d, strerr=%s.\n", ret, strerror(ToolGetErrorCode()));
            XFREE(path);
            return SYS_ERROR;
        }
        XFREE(path);
    }
    CheckLogDirPermission((const char *)g_rootLogPath);

    ret = snprintf_s(g_selfLogDir, LOG_DIR_FOR_SELF_LENGTH + 1, LOG_DIR_FOR_SELF_LENGTH,
                     "%s%s", g_rootLogPath, LOG_DIR_FOR_SELF_LOG);
    if (ret == -1) {
        SYSLOG_WARN("snprintf_s failed, slogd_log_path=%s, strerr=%s.\n", g_rootLogPath, strerror(ToolGetErrorCode()));
        return SYS_ERROR;
    }
    return SYS_OK;
}

/**
 * @brief StrcatDir: stract filename to directory
 * @param [out]path: the full path
 * @param [in]filename: pointer to store file name
 * @param [in]dir:pointer to store dir
 * @param [in]maxlen:the maxlength of out path
 * @return:succeed: SYS_OK, failed:SYS_ERROR
 */
int StrcatDir(char *path, const char *filename, const char *dir, unsigned int maxlen)
{
    if ((dir == NULL) || (filename == NULL) || (path == NULL)) {
        return SYS_ERROR;
    }
    if ((unsigned int)(strlen(dir) + strlen(filename)) > maxlen) {
        return SYS_ERROR;
    }
    int ret = strcpy_s(path, maxlen, dir);
    if (ret != EOK) {
        return SYS_ERROR;
    }
    ret = strcat_s(path, maxlen, filename);
    if (ret != EOK) {
        return SYS_ERROR;
    }
    return SYS_OK;
}

LogRt GetDevNumIDs(unsigned int *deviceNum, unsigned int *deviceIdArray)
{
#if defined(EP_DEVICE_MODE) || defined(RC_MODE)
    int devNum = 0;
    int devId[MAX_DEV_NUM] = { 0 };
    if (deviceNum == NULL || deviceIdArray == NULL) {
        return FLAG_FALSE;
    }
    int ret = log_get_device_id(devId, &devNum, MAX_DEV_NUM);
    if ((ret != SYS_OK) || (devNum > MAX_DEV_NUM) || (devNum < 0)) {
        SELF_LOG_WARN("get device id failed, result=%d, device_number=%d.", ret, devNum);
        return FLAG_FALSE;
    }
    *deviceNum = (unsigned int)devNum;
    int idx = 0;
    for (; idx < devNum; idx++) {
        if (devId[idx] >= 0 && devId[idx] < MAX_DEV_NUM) {
            deviceIdArray[idx] = (unsigned int)devId[idx];
        }
    }
#elif defined(RC_HISI_MODE)
    if (deviceNum == NULL || deviceIdArray == NULL) {
        return FLAG_FALSE;
    }
    *deviceNum = 0;
#endif
    (void)deviceNum;
    (void)deviceIdArray;
    return SUCCESS;
}

unsigned int GetHostDeviceID(unsigned int deviceId)
{
#if defined(EP_DEVICE_MODE)
    int devNum = 0;
    int devId[MAX_DEV_NUM] = { 0 };
    int ret = log_get_device_id(devId, &devNum, MAX_DEV_NUM);
    if ((ret != SYS_OK) || (devNum > MAX_DEV_NUM) || (devNum < 0)) {
        SELF_LOG_WARN("get device id failed, result=%d, device_number=%d.", ret, devNum);
        return deviceId;
    }
    if (deviceId >= (unsigned int)devNum) {
        return deviceId;
    }
    unsigned int hostDeviceId = 0;
    ret = drvGetDevIDByLocalDevID(deviceId, &hostDeviceId);
    if (ret != SYS_OK) {
        SELF_LOG_WARN("get hostside device-id by device device-id=%u, result=%d", deviceId, ret);
        return deviceId;
    }
    SELF_LOG_INFO("get hostside device-id=%u by deviceside device-id=%u, result=%d", hostDeviceId, deviceId, ret);
    return hostDeviceId;
#else
    return deviceId;
#endif
}
