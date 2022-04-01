/**
 * @syslogd_common.c
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#include "syslogd_common.h"
#include "slog.h"
#include "cfg_file_parse.h"
#include "log_sys_package.h"
#include "print_log.h"
#include "common.h"

#ifdef APP_LOG_REPORT
#include "ide_tlv.h"
#include "log_drv.h"
#include "log_session_manage.h"
#endif

STATIC UINT8 g_writeFileThreadExit = 0;
STATIC LogBufList *g_appLogBufList = NULL;
STATIC LogBufList *g_sysLogBufList[LOG_TYPE_NUM] = { NULL, NULL, NULL, NULL };
static StLogFileList g_fileList;
STATIC toolMutex g_mutex;
STATIC int g_mutexInit = 0;
STATIC unsigned int g_writeFilePrintNum = 0;
static unsigned int g_deviceIdToHostDeviceId[MAX_DEV_NUM] = { MAX_DEV_NUM };
static unsigned int g_hostDeviceIdToDeviceId[MAX_DEV_NUM] = { MAX_DEV_NUM };
STATIC void LogToDataBuf(const char *msg, LogInfo *info);
static int g_logBufSize = DEFAULT_LOG_BUF_SIZE;

void ParseLogBufCfg(void)
{
    g_logBufSize = GetItemValueFromCfg(LOG_BUF_SIZE, MIN_LOG_BUF_SIZE, MAX_LOG_BUF_SIZE, DEFAULT_LOG_BUF_SIZE);
}

int GetItemValueFromCfg(const char *item, int minItemValue, int maxItemValue, int defaultItemValue)
{
    char val[CONF_VALUE_MAX_LEN + 1] = { 0 };
    LogRt ret = GetConfValueByList(item, strlen(item), val, CONF_VALUE_MAX_LEN);
    if ((ret == SUCCESS) && IsNaturalNumStr(val)) {
        int tmpL = atoi(val);
        if ((tmpL >= minItemValue) && (tmpL <= maxItemValue)) {
            SELF_LOG_INFO("get %s succeed, %s=%d.", item, item, tmpL);
            return tmpL;
        } else {
            SELF_LOG_WARN("%s is invalid, use default=%d.", item, defaultItemValue);
            return defaultItemValue;
        }
    } else {
        SELF_LOG_WARN("get %s failed, use default=%d.", item, defaultItemValue);
        return defaultItemValue;
    }
}

StLogFileList *GetGlobalLogFileList(void)
{
    return &g_fileList;
}

void DataBufLock(void)
{
    if (ToolMutexLock(&g_mutex) == SYS_ERROR) {
        SELF_LOG_WARN("lock fail, strerr=%s", strerror(ToolGetErrorCode()));
    }
}

void DataBufUnLock(void)
{
    if (ToolMutexUnLock(&g_mutex) == SYS_ERROR) {
        SELF_LOG_WARN("unlock fail, strerr=%s", strerror(ToolGetErrorCode()));
    }
}

STATIC LogBufList *InitLogNode(void)
{
    LogBufList *node = (LogBufList *)malloc(sizeof(LogBufList));
    if (node == NULL) {
        SELF_LOG_ERROR("malloc failed, strerr=%s.", strerror(ToolGetErrorCode()));
        return NULL;
    }
    (void)memset_s(node, sizeof(LogBufList), 0x00, sizeof(LogBufList));
    node->data = (char *)malloc(g_logBufSize + 1);
    if (node->data == NULL) {
        SELF_LOG_ERROR("malloc failed, strerr=%s.", strerror(ToolGetErrorCode()));
        XFREE(node);
        return NULL;
    }
    (void)memset_s(node->data, g_logBufSize + 1, 0x00, g_logBufSize + 1);
    return node;
}

// inner interface without lock, cannot use by other source file
STATIC LogBufList *InnerInsertAppNode(const LogInfo *info)
{
    ONE_ACT_WARN_LOG(info == NULL, return NULL, "input null");
    ONE_ACT_WARN_LOG(info->deviceId >= MAX_DEV_NUM, return NULL, "deviceId=%u invalid.", info->deviceId);
    LogBufList *node = InitLogNode();
    if (node == NULL) {
        SELF_LOG_WARN("malloc failed, strerr=%s.", strerror(ToolGetErrorCode()));
        return NULL;
    }
    node->pid = info->pid;
    node->type = info->type;
    node->deviceId = info->deviceId;
    node->writeWaitTime = 0;
    node->next = g_appLogBufList;
    g_appLogBufList = node;
    return node;
}

STATIC bool IsToDeleteNode(const LogBufList *input, const LogBufList *targetNode)
{
    if ((input == NULL) || (targetNode == NULL)) {
        return false;
    }
    if ((input->pid == targetNode->pid) && (input->deviceId == targetNode->deviceId) &&
        (input->type == targetNode->type)) {
        return true;
    } else {
        return false;
    }
}

// inner interface without lock, cannot use by other source file
STATIC LogBufList *InnerGetAppNode(const LogInfo *info)
{
    ONE_ACT_NO_LOG(info == NULL, return NULL);
    ONE_ACT_WARN_LOG(info->deviceId >= MAX_DEV_NUM, return NULL, "deviceId=%u invalid.", info->deviceId);
    LogBufList *tmp = g_appLogBufList;
    while (tmp != NULL) {
        if ((tmp->pid == info->pid) && (tmp->deviceId == info->deviceId) && (tmp->type == info->type)) {
            return tmp;
        }
        tmp = tmp->next;
    }
    return NULL;
}

// inner interface without lock, cannot use by other source file
STATIC LogBufList *InnerGetSysNode(const LogInfo *info)
{
    ONE_ACT_NO_LOG(info == NULL, return NULL);
    ONE_ACT_NO_LOG((info->type < DEBUG_LOG) && (info->type >= LOG_TYPE_NUM), return NULL);
#ifdef SORTED_LOG
    return g_sysLogBufList[info->type];
#else
    return g_sysLogBufList[0];
#endif
}

STATIC LogBufList *InnerGetNode(const LogInfo *info)
{
    if (info->processType == SYSTEM) {
        return InnerGetSysNode(info);
    }
    if (info->processType == APPLICATION) {
        LogBufList *node = InnerGetAppNode(info);
        if (node == NULL) {
            return InnerInsertAppNode(info);
        } else {
            return node;
        }
    }
    return NULL;
}

// inner interface without lock, cannot use by other source file
#if (defined APP_LOG_WATCH) || (defined APP_LOG_REPORT)
STATIC void InnerDeleteAppNode(const LogBufList *input)
{
    ONE_ACT_NO_LOG(input == NULL, return);
    LogBufList *tmp = g_appLogBufList;
    if (tmp == NULL) {
        return;
    }
    if (IsToDeleteNode((const LogBufList *)tmp, input) == true) {
        g_appLogBufList = g_appLogBufList->next;
        XFREE(tmp->data);
        XFREE(tmp);
    } else {
        while ((tmp != NULL) && (IsToDeleteNode((const LogBufList *)tmp->next, input) == false)) {
            tmp = tmp->next;
        }
        if ((tmp != NULL) && (IsToDeleteNode((const LogBufList *)tmp->next, input) == true)) {
            LogBufList *node = tmp->next;
            tmp->next = tmp->next->next;
            XFREE(node->data);
            XFREE(node);
        }
    }
    return;
}
#endif

LogRt InitBufAndFileList(void)
{
    int i;
    if (g_mutexInit == 0) {
        if (ToolMutexInit(&g_mutex) == SYS_OK) {
            g_mutexInit = 1;
        } else {
            SELF_LOG_WARN("init mutex failed, strerr=%s.", strerror(ToolGetErrorCode()));
            return MUTEX_INIT_ERR;
        }
    }
    unsigned int ret = LogAgentInitDeviceOs(&g_fileList);
    if (ret != OK) {
        SELF_LOG_ERROR("init log file info failed, result=%u, strerr=%s.", ret, strerror(ToolGetErrorCode()));
        return INIT_FILE_LIST_ERR;
    }
    for (i = 0; i < MAX_DEV_NUM; i++) {
        g_deviceIdToHostDeviceId[i] = MAX_DEV_NUM;
        g_hostDeviceIdToDeviceId[i] = MAX_DEV_NUM;
    }

#ifdef SORTED_LOG
    for (i = 0; i < LOG_TYPE_NUM; i++) {
        g_sysLogBufList[i] = InitLogNode();
        if (g_sysLogBufList[i] == NULL) {
            int j = 0;
            for (; j < i; j++) {
                XFREE(g_sysLogBufList[i]->data);
                XFREE(g_sysLogBufList[i]);
            }
            LogAgentCleanUpDeviceOs(&g_fileList);
            return MALLOC_FAILED;
        }
        g_sysLogBufList[i]->type = (LogType)((int)DEBUG_LOG + i);
    }
#else
    g_sysLogBufList[0] = InitLogNode();
    if (g_sysLogBufList[0] == NULL) {
        SELF_LOG_ERROR("malloc failed, strerr=%s.", strerror(ToolGetErrorCode()));
        LogAgentCleanUpDeviceOs(&g_fileList);
        return MALLOC_FAILED;
    }
#endif
    return SUCCESS;
}

void FreeBufAndFileList(void)
{
    DataBufLock();
    int i;
    for (i = 0; i < LOG_TYPE_NUM; i++) {
        if (g_sysLogBufList[i] != NULL) {
            XFREE(g_sysLogBufList[i]->data);
            XFREE(g_sysLogBufList[i]);
        }
    }

    LogBufList *tmp = g_appLogBufList;
    LogBufList *node = NULL;
    while (tmp != NULL) {
        node = tmp;
        tmp = tmp->next;
        XFREE(node->data);
        XFREE(node);
    }
    g_appLogBufList = NULL;
    DataBufUnLock();
    (void)ToolMutexDestroy(&g_mutex);
    g_mutexInit = 0;
    return;
}

UINT8 IsWriteFileThreadExit(void)
{
    return g_writeFileThreadExit;
}

void SetWriteFileThreadExit(void)
{
    g_writeFileThreadExit = 1;
}

void ParseSyslogdCfg(void)
{
    char val[CONF_VALUE_MAX_LEN + 1] = { 0 };
    LogRt ret = GetConfValueByList(GLOBALLEVEL_KEY, strlen(GLOBALLEVEL_KEY), val, CONF_VALUE_MAX_LEN);
    if ((ret == SUCCESS) && IsNaturalNumStr(val)) {
        int tmpL = atoi(val);
        if ((tmpL >= LOG_MIN_LEVEL) && (tmpL <= LOG_MAX_LEVEL)) {
            g_logLevel.globalLevel = tmpL;
            SELF_LOG_INFO("get global level succeed, level=%s.", GetBasicLevelNameById(tmpL));
        } else {
            SELF_LOG_WARN("global level is invalid, use default=%s.",
                          GetBasicLevelNameById(g_logLevel.globalLevel));
        }
    } else {
        SELF_LOG_WARN("get global level failed, use default=%s.",
                      GetBasicLevelNameById(g_logLevel.globalLevel));
    }
}

STATIC unsigned int UlToUInt(unsigned long input, const char *start, const char *end)
{
    // number overflow or function failed or no digit found
    if ((input > (unsigned int)(UINT_MAX)) || (start == end)) {
        return (UINT_MAX);
    }
    return (unsigned int)input;
}

STATIC unsigned int GetHostSideDeviceId(unsigned int deviceId)
{
    if (deviceId < MAX_DEV_NUM) {
        if (g_deviceIdToHostDeviceId[deviceId] == MAX_DEV_NUM) {
            unsigned int hostDevId = GetHostDeviceID(deviceId);
            g_deviceIdToHostDeviceId[deviceId] = hostDevId;
            g_hostDeviceIdToDeviceId[hostDevId] = deviceId;
        }
        return g_deviceIdToHostDeviceId[deviceId];
    }
    return deviceId;
}

STATIC void ParseMsgHead(const char **input, LogInfo *info)
{
    ONE_ACT_NO_LOG((input == NULL) || (*input == NULL) || (info == NULL), return);
    const char *p = *input;
    const char *start = NULL;
    if (*p == '[') {
        start = p + 1;
        unsigned long tmp = strtoul(start, (char **)&p, BASE_NUM);
        unsigned int pt = UlToUInt(tmp, start, p);
        info->processType = (pt != 0) ? SYSTEM : APPLICATION;
        if (*p == ',') {
            start = p + 1;
            tmp = strtoul(start, (char **)&p, BASE_NUM);
            info->pid = UlToUInt(tmp, start, p);
            if (*p == ',') {
                start = p + 1;
                tmp = strtoul(start, (char **)&p, BASE_NUM);
                info->deviceId = GetHostSideDeviceId(UlToUInt(tmp, start, p));
            }
        }
    }
    if (*p == ']') {
        p++;
    }
    *input = p;
    if ((info->processType == APPLICATION) && (info->pid == 0)) {
        info->processType = SYSTEM;
    }
}

void ProcEscapeThenLog(const char *tmpbuf, int len, LogType type)
{
    ONE_ACT_ERR_LOG(tmpbuf == NULL, return, "[input] temp buffer is null.");
    const char *p = tmpbuf;
    tmpbuf += len;
    while (p < tmpbuf) {
        char *q = g_globalsPtr->parseBuf;
        LogInfo info = { DEBUG_LOG, APPLICATION, 0, 0 };
        info.type = type;
        ParseMsgHead(&p, &info);
        char c = *p;
        while (c != '\0') {
            if (c == '\n') {
                c = ' ';
            }
            if (((c >= 0) && (c <= 0x1f)) && (c != '\t')) {
                *q = '^';
                q++;
                c += '@'; // escape special char
            }
            *q = c;
            q++;
            p++;
            c = *p;
        }
        if (q == g_globalsPtr->parseBuf) {
            continue;
        }
        *q = '\n';
        q++;
        *q = '\0';
        LogToDataBuf(g_globalsPtr->parseBuf, &info);
    }
}

#ifdef APP_LOG_REPORT
STATIC unsigned int GetDeviceSideDeviceId(unsigned int hostDevId)
{
    if (hostDevId < MAX_DEV_NUM) {
        if (g_hostDeviceIdToDeviceId[hostDevId] == MAX_DEV_NUM) {
            return hostDevId;
        }
        return g_hostDeviceIdToDeviceId[hostDevId];
    }
    return hostDevId;
}

/**
* @brief DevLogReportEnd: send hdc end buf to client to close session
* @param [in]node: session info
* @return: success: return 0; failed: return others
*/
STATIC int DevLogReportEnd(SessionNode *node)
{
    int ret;

    // log info's device id is host side
    // session node's device id is device side
    unsigned int hostDevId = GetHostSideDeviceId(node->devId);

    ret = DrvBufWrite((HDC_SESSION)node->session, HDC_END_BUF, sizeof(HDC_END_BUF));
    NO_ACT_WARN_LOG(ret != 0, "write end buf to hdc failed, ret=%d, pid=%d, devId=%d.", ret, node->pid, hostDevId);

    ret = DrvSessionRelease((HDC_SESSION)node->session);
    NO_ACT_WARN_LOG(ret != 0, "release session failed, ret=%d, pid=%d, devId=%d.", ret, node->pid, hostDevId);

    return ret;
}

/**
* @brief DevLogReport: send log data to client on docker
* @param [in]logInfo: log info include log type, process type and pid
* @param [in]msg: log data
* @param [in]msgLen: log data length
* @return: success: return 0; failed: return others
*/
STATIC int DevLogReport(LogInfo *logInfo, const char *msg, unsigned int msgLen)
{
    int ret;

    ONE_ACT_NO_LOG(logInfo == NULL, return -1);
    ONE_ACT_NO_LOG(msg == NULL || msgLen == 0, return -1);

    // log info's device id is host side
    // session node's device id is device side
    unsigned int hostDevId = logInfo->deviceId;
    unsigned int devId = GetDeviceSideDeviceId(hostDevId);

    SessionNode *node = GetDeletedSessionNode(logInfo->pid, devId);
    if (node == NULL) {
        node = GetSessionNode(logInfo->pid, devId);
        if (node == NULL) {
            unsigned int res = LogAgentWriteDeviceAppLog(msg, msgLen, logInfo);
            SELF_LOG_WARN("get session info failed, pid=%d, devId=%d, res=%u", logInfo->pid, hostDevId, res);
            return -1;
        }
    }

    ret = DrvBufWrite((HDC_SESSION)node->session, msg, msgLen);
    if (ret != 0) {
        unsigned int res = LogAgentWriteDeviceAppLog(msg, msgLen, logInfo);
        SELF_LOG_WARN("write data to hdc failed, ret=%d, pid=%d, devId=%d, res=%u", ret, node->pid, hostDevId, res);
        return ret;
    }
    return ret;
}

/**
* @brief HandleDeletedSessionNode: Pop all session nodes which will be deleted from list,
*    traverse each node and send log.
*    if session node is timeout, then step into the way that free session node and send hdc end buf to client.
*    if session node is not timeout, then save it to the next, and at last, push all remaining nodes to the list again.
* @return: NA
*/
STATIC void HandleDeletedSessionNode(void)
{
    int ret;
    SessionNode *pre = NULL;
    // pop all session nodes which will be deleted
    SessionNode *sessionNode = PopDeletedSessionNode();
    SessionNode *head = sessionNode;
    while (sessionNode != NULL) {
        // traverse each node
        // log-info's device id is host side
        // session-node's device id is device side
        unsigned int devId = sessionNode->devId;
        unsigned int hostDevId = GetHostSideDeviceId(devId);
        LogInfo info = { DEBUG_LOG, APPLICATION, sessionNode->pid, hostDevId };
        LogBufList *logNode = InnerGetNode((const LogInfo *)(&info));
        if (logNode != NULL && logNode->data != NULL && logNode->len > 0) {
            // send log node to client
            ret = DrvBufWrite((HDC_SESSION)sessionNode->session, logNode->data, logNode->len);
            if (ret != 0) {
                // get dev id about host side
                unsigned int res = LogAgentWriteDeviceAppLog((const char *)logNode->data, logNode->len, &info);
                SELF_LOG_WARN("write log to hdc failed, ret=%d, pid=%d, devId=%d, res=%u",
                              ret, sessionNode->pid, hostDevId, res);
            }
            logNode->len = 0;
        }
        sessionNode->timeout -= ONE_SECOND;
        if (sessionNode->timeout <= 0) {
            // session node is timeout, the timeout value is notified by client (libalog.so)
            // delete node from list
            SessionNode *tmp = sessionNode->next;
            if (pre == NULL) {
                head = sessionNode->next;
            } else {
                pre->next = sessionNode->next;
            }
            // send end buf to client
            (void)DevLogReportEnd(sessionNode);
            // free log node
            InnerDeleteAppNode(logNode);
            XFREE(sessionNode);
            sessionNode = tmp;
        } else {
            pre = sessionNode;
            sessionNode = sessionNode->next;
        }
    }
    // push none timeout nodes to the list again
    PushDeletedSessionNode(head);
}

/**
* @brief FlushLogBufToHost: send log data to host
* @return: NA
*/
STATIC void FlushLogBufToHost(void)
{
    unsigned int ret;
    LogBufList *node = NULL;
    LogBufList *tmp = g_appLogBufList;
    while (tmp != NULL) {
        node = tmp;
        tmp = tmp->next;
        if (node->len > 0) {
            LogInfo info = { node->type, APPLICATION, node->pid, node->deviceId };
            if (node->writeWaitTime < MAX_WRITE_WAIT_TIME) {
                node->writeWaitTime++;
                continue;
            }
            node->writeWaitTime = 0;
            ret = DevLogReport(&info, node->data, node->len);
            if (ret != OK) {
                SELF_LOG_ERROR_N(&g_writeFilePrintNum, GENERAL_PRINT_NUM,
                                 "report log to process failed, result=%u, strerr=%s, print once every %d times.",
                                 ret, strerror(ToolGetErrorCode()), GENERAL_PRINT_NUM);
            }
            node->len = 0;
            continue;
        }
        if (node->len == 0) {
            node->noAppDataCount++;
            if (node->noAppDataCount >= NO_APP_DATA_MAX_COUNT) {
                InnerDeleteAppNode(node);
            }
        }
    }
}
#endif

STATIC int LogToDataBufInputCheck(const char *msg, const LogInfo *info)
{
    ONE_ACT_WARN_LOG((msg == NULL) || (info == NULL), return SYS_ERROR, "[input] null message or null info");
#ifdef SORTED_LOG
    ONE_ACT_WARN_LOG((info->type < DEBUG_LOG) || (info->type >= LOG_TYPE_NUM),
                     return SYS_ERROR, "[input] wrong log type=%d", info->type);
#else
    ONE_ACT_WARN_LOG(info->type != DEBUG_LOG, return SYS_ERROR, "[input] wrong log type=%d", info->type);
#endif
    return SYS_OK;
}

STATIC void LogToDataBuf(const char *msg, LogInfo *info)
{
    ONE_ACT_NO_LOG(LogToDataBufInputCheck(msg, (const LogInfo *)info) != SYS_OK, return);
    DataBufLock();
    LogBufList *node = InnerGetNode((const LogInfo *)info);
    TWO_ACT_WARN_LOG(node == NULL, DataBufUnLock(), return, "device log node null, type=%d", info->processType);
    if (info->processType == SYSTEM) {
        if ((node->len > (unsigned int)g_logBufSize) ||
            (((unsigned int)g_logBufSize - node->len) < (unsigned int)strlen(msg))) {
            unsigned int ret = LogAgentWriteDeviceOsLog(info->type, &g_fileList, node->data, node->len);
            if (ret != OK) {
                SELF_LOG_ERROR_N(&g_writeFilePrintNum, GENERAL_PRINT_NUM,
                                 "write device system log failed, result=%u, strerr=%s, print once every %d times.",
                                 ret, strerror(ToolGetErrorCode()), GENERAL_PRINT_NUM);
            }
            node->len = 0;
        }
        int res = memcpy_s(node->data + node->len, (unsigned int)g_logBufSize - node->len, msg, strlen(msg));
        TWO_ACT_ERR_LOG(res != EOK, DataBufUnLock(), return, "memcpy_s log data failed, result=%d, strerr=%s.",
                        res, strerror(ToolGetErrorCode()));
        node->len += strlen(msg);
    } else {
        node->noAppDataCount = 0;
        if ((node->len > (unsigned int)g_logBufSize) ||
            (((unsigned int)g_logBufSize - node->len) < (unsigned int)strlen(msg))) {
#ifdef APP_LOG_WATCH
            unsigned int ret = LogAgentWriteDeviceAppLog(node->data, node->len, info);
            if (ret != OK) {
                SELF_LOG_ERROR_N(&g_writeFilePrintNum, GENERAL_PRINT_NUM,
                                 "write device app log failed, result=%u, strerr=%s, print once every %d times.",
                                 ret, strerror(ToolGetErrorCode()), GENERAL_PRINT_NUM);
            }
#elif defined APP_LOG_REPORT
            unsigned int ret = DevLogReport(info, node->data, node->len);
            if (ret != OK) {
                SELF_LOG_ERROR_N(&g_writeFilePrintNum, GENERAL_PRINT_NUM,
                                 "report log to process failed, result=%u, strerr=%s, print once every %d times.",
                                 ret, strerror(ToolGetErrorCode()), GENERAL_PRINT_NUM);
            }
#endif
            node->len = 0;
        }
        int res = memcpy_s(node->data + node->len, (unsigned int)g_logBufSize - node->len, msg, strlen(msg));
        TWO_ACT_ERR_LOG(res != EOK, DataBufUnLock(), return, "memcpy_s log data failed, result=%d, strerr=%s.",
                        res, strerror(ToolGetErrorCode()));
        node->len += strlen(msg);
    }
    DataBufUnLock();
}

void FflushLogDataBuf(void)
{
    int i;
    unsigned int ret;
    LogBufList *node = NULL;
    DataBufLock();
#ifdef APP_LOG_REPORT
    HandleDeletedSessionNode();
    HandleInvalidSessionNode();
    FlushLogBufToHost();
#endif
    for (i = 0; i < (int)LOG_TYPE_NUM; i++) {
        node = g_sysLogBufList[i];
        if ((node != NULL) && (node->len > 0)) {
            ret = LogAgentWriteDeviceOsLog(node->type, &g_fileList, node->data, node->len);
            if (ret != OK) {
                SELF_LOG_ERROR_N(&g_writeFilePrintNum, GENERAL_PRINT_NUM,
                                 "write device os log failed, result=%u, strerr=%s, print once every %d times.",
                                 ret, strerror(ToolGetErrorCode()), GENERAL_PRINT_NUM);
            }
            node->len = 0;
        }
    }
#ifdef APP_LOG_WATCH
    LogBufList *tmp = g_appLogBufList;
    while (tmp != NULL) {
        node = tmp;
        tmp = tmp->next;
        if (node->len > 0) {
            LogInfo info = { node->type, APPLICATION, node->pid, node->deviceId };
            ret = LogAgentWriteDeviceAppLog(node->data, node->len, &info);
            if (ret != OK) {
                SELF_LOG_ERROR_N(&g_writeFilePrintNum, GENERAL_PRINT_NUM,
                                 "write device app log failed, result=%u, strerr=%s, print once every %d times.",
                                 ret, strerror(ToolGetErrorCode()), GENERAL_PRINT_NUM);
            }
            node->len = 0;
        } else {
            node->noAppDataCount++;
            if (node->noAppDataCount >= NO_APP_DATA_MAX_COUNT) {
                InnerDeleteAppNode(node);
            }
        }
    }
#endif

    DataBufUnLock();
}

/**
* @brief ProcSyslogBuf: proc syslog buf with space or endline
* @param [in]recvBuf: receive syslog buf
* @param [in]pSize: receive syslog buf size
* @return: SYS_OK/SYS_ERROR
*/
int ProcSyslogBuf(const char *recvBuf, int *size)
{
    if ((recvBuf == NULL) || (size == NULL)) {
        SELF_LOG_WARN("receive log failed by socket, input null.");
        return SYS_ERROR;
    }

    int sizeTmp = *size;
    if (sizeTmp < 0) {
        SELF_LOG_WARN("receive log failed by socket, size=%d, strerr=%s.", sizeTmp, strerror(ToolGetErrorCode()));
        return SYS_ERROR;
    }

    while (1) {
        if (sizeTmp == 0) {
            return SYS_ERROR;
        }
        if ((recvBuf[sizeTmp - 1] != '\0') && (recvBuf[sizeTmp - 1] != '\n')) {
            break;
        }
        sizeTmp--;
    }

    *size = sizeTmp;
    return SYS_OK;
}
