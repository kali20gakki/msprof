/**
 * @syslogd.c
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
#include "log_session_manage.h"

#define FIFTY_MILLI_SECOND 50
#ifdef LHISI_
#define WRITE_DEFAULT_THREAD_ATTR {1, 0, 0, 0, 0, 1, 128 * 1024}
#endif
STATIC ToolUserBlock g_writeFileThread;
struct Globals *g_globalsPtr = NULL;
LogLevel g_logLevel;

int InitGlobals(void)
{
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

void InitModuleDefault(const struct Options *opt)
{
    if ((opt != NULL) && (opt->l != 0)) {
        g_logLevel.globalLevel = opt->l;
    }
}

STATIC void SyslogCleanUp(void)
{
    FreeBufAndFileList();
#ifdef APP_LOG_REPORT
    FreeSessionList();
#endif
}

STATIC int SyslogInit(void)
{
    if (InitBufAndFileList() != SUCCESS) {
        SELF_LOG_ERROR("init LOG buffer failed, strerr=%s.", strerror(ToolGetErrorCode()));
        return SYS_ERROR;
    }
#ifdef APP_LOG_REPORT
    if (InitSessionList() != SUCCESS) {
        SELF_LOG_ERROR("init session pid map failed");
        return SYS_ERROR;
    }
#endif
    return SYS_OK;
}

STATIC void *WriteLogBufToFile(ArgPtr arg)
{
    (void)arg;
    if (SetThreadName("writeLogToFile") != 0) {
        SELF_LOG_WARN("set thread_name(writeLogToFile) failed.");
    }

#ifndef _LOG_UT_
    while (g_gotSignal == 0) {
        FflushLogDataBuf();
        (void)ToolSleep(ONE_SECOND);
    }
#endif
    FflushLogDataBuf();
    SELF_LOG_ERROR("Thread(writeLogToFile) quit, signal=%d.", g_gotSignal);
    SetWriteFileThreadExit();
    return (void *)NULL;
}

#if (OS_TYPE_DEF == LINUX)
// Create socket for dlog interface
STATIC toolSockHandle CreateSocket(void)
{
    struct sockaddr_un sunx;
    (void)memset_s(&sunx, sizeof(sunx), 0, sizeof(sunx));
    sunx.sun_family = AF_UNIX;
    int ret = strcpy_s(sunx.sun_path, sizeof(sunx.sun_path), GetSocketPath());
    ONE_ACT_ERR_LOG(ret != EOK, return SYS_ERROR, "strcpy_s failed, result=%d, strerr=%s.",
                    ret, strerror(ToolGetErrorCode()));

    ret = ToolUnlink(sunx.sun_path);
    if (ret != EOK) {
        SELF_LOG_WARN("unlink failed, file=%s, strerr=%s.", sunx.sun_path, strerror(ToolGetErrorCode()));
    }
    toolSockHandle sockFd = ToolSocket(AF_UNIX, SOCK_DGRAM, 0);
    ONE_ACT_ERR_LOG(sockFd == SYS_ERROR, return SYS_ERROR, "create socket failed, strerr=%s.",
                    strerror(ToolGetErrorCode()));

    int nSendBuf = SIZE_SIXTEEN_MB;
    ret = setsockopt(sockFd, SOL_SOCKET, SO_RCVBUF, (const char *)&nSendBuf, sizeof(int));
    if (ret < 0) {
        SELF_LOG_WARN("set socket option failed, strerr=%s.", strerror(ToolGetErrorCode()));
    }

    ret = ToolBind(sockFd, (ToolSockAddr *)&sunx, sizeof(sunx));
    TWO_ACT_ERR_LOG(ret != SYS_OK, LOG_CLOSE_SOCK_S(sockFd), return SYS_ERROR,
                    "bind socket failed, strerr=%s.", strerror(ToolGetErrorCode()));

    ChangePathOwner(GetSocketPath());
    ret = ToolChmod(GetSocketPath(), SyncGroupToOther(S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)); // 0660
    if (ret != 0) {
        SELF_LOG_ERROR("chmod %s failed , strerr=%s.", GetSocketPath(), strerror(ToolGetErrorCode()));
    }

    SELF_LOG_INFO("create socket succeed.");
    return sockFd;
}

void ProcSyslogd(void)
{
    // Set up signal handlers (so that they interrupt read())
    SignalNoSaRestartEmptyMask(SIGTERM, RecordSigNo);
    SignalNoSaRestartEmptyMask(SIGINT, RecordSigNo);
    (void)signal(SIGHUP, SIG_IGN);

    toolSockHandle sock = CreateSocket();
    if (sock == SYS_ERROR) {
        SELF_LOG_ERROR("[FATAL] create socket failed, quit slogd process...");
        return;
    }

    int ret = SyslogInit();
    if (ret == SYS_ERROR) {
        LOG_CLOSE_SOCK_S(sock);
        SELF_LOG_ERROR("[FATAL] init slogd failed, quit slogd process...");
        return;
    }

    // threadstack no optimize, use default (thread in device Performance priority)
    toolThread tid = 0;
    g_writeFileThread.pulArg = NULL;
    g_writeFileThread.procFunc = WriteLogBufToFile;
#ifdef LHISI_
    ToolThreadAttr threadAttr = WRITE_DEFAULT_THREAD_ATTR;
    if (ToolCreateTaskWithThreadAttr(&tid, &g_writeFileThread, &threadAttr) != SYS_OK) {
#else
    if (ToolCreateTaskWithDetach(&tid, &g_writeFileThread) != SYS_OK) {
#endif
        SELF_LOG_ERROR("[FATAL] create thread failed, strerr=%s, quit slogd process...", strerror(ToolGetErrorCode()));
        SyslogCleanUp();
        LOG_CLOSE_SOCK_S(sock);
        return;
    }

    char *recvBuf = g_globalsPtr->recvBuf;
    while (g_gotSignal == 0) {
        int sz = ToolRead(sock, recvBuf, MAX_READ - 1);
        if (ProcSyslogBuf(recvBuf, &sz) != SYS_OK) {
            continue;
        }
        recvBuf[sz] = '\0';
        ProcEscapeThenLog(recvBuf, sz, DEBUG_LOG);
    }

    // wait until thread exit
    while (IsWriteFileThreadExit() == 0) {
        (void)ToolSleep(FIFTY_MILLI_SECOND);
    }

    // main process exit and resource free
    SyslogCleanUp();
    LOG_CLOSE_SOCK_S(sock);
    SELF_LOG_ERROR("signal=%d, quit slogd process...", g_gotSignal);
}

#else
// AF_UNIX Unsupported in windows
#define SLOGD_SOCKET_PORT 8008
#define SLOGD_SOCKET_ADDR "127.0.0.1"
STATIC toolSockHandle CreateSocket(void)
{
    toolSockHandle sockFd = ToolSocket(AF_INET, SOCK_DGRAM, 0);
    if (sockFd == SYS_ERROR) {
        SELF_LOG_ERROR("create socket failed, strerr=%s.", strerror(ToolGetErrorCode()));
        return SYS_ERROR;
    }

    struct sockaddr_in sunx;
    (void)memset_s(&sunx, sizeof(sunx), 0, sizeof(sunx));
    sunx.sin_family = AF_INET;
    sunx.sin_port = htons(SLOGD_SOCKET_PORT); // host port
    if ((inet_pton(AF_INET, SLOGD_SOCKET_ADDR, &sunx.sin_addr)) <= 0) {
        SELF_LOG_ERROR("convert address failed, strerr=%s.", strerror(ToolGetErrorCode()));
        LOG_CLOSE_SOCK_S(sockFd);
        return SYS_ERROR;
    }
    if (SYS_OK != ToolBind(sockFd, (ToolSockAddr *)&sunx, sizeof(sunx))) {
        SELF_LOG_ERROR("bind socket failed, strerr=%s.", strerror(ToolGetErrorCode()));
        LOG_CLOSE_SOCK_S(sockFd);
        return SYS_ERROR;
    }

    SELF_LOG_INFO("create socket succeed.");
    return sockFd;
}

void ProcSyslogd(void)
{
    (void)signal(SIGTERM, RecordSigNo);
    (void)signal(SIGINT, RecordSigNo);

    int ret = SyslogInit();
    if (ret == SYS_ERROR) {
        SELF_LOG_ERROR("[FATAL] init slogd failed, quit slogd process...");
        return;
    }

    toolThread tid = (void *)NULL;
    g_writeFileThread.pulArg = NULL;
    g_writeFileThread.procFunc = WriteLogBufToFile;
    if (ToolCreateTaskWithDetach(&tid, &g_writeFileThread) != 0) {
        SELF_LOG_ERROR("[FATAL] create task failed, strerr=%s, quit slogd process...", strerror(ToolGetErrorCode()));
        SyslogCleanUp();
        return;
    }

    toolSockHandle sockhandle = CreateSocket();
    if (sockhandle == SYS_ERROR) {
        SELF_LOG_ERROR("[FATAL] create socket failed, quit slogd process...");
        SyslogCleanUp();
        return;
    }

    int readFlag = 0;
    struct sockaddr_in clientAddr;
    int len = sizeof(clientAddr);
    char *recvBuf = g_globalsPtr->recvBuf;
    while (!g_gotSignal) {
        int sz = recvfrom((int)sockhandle, recvBuf, MAX_READ - 1, 0, (struct sockaddr *)&clientAddr, &len);
        if (ProcSyslogBuf(recvBuf, &sz) != SYS_OK) {
            continue;
        }

        recvBuf[sz] = '\0';
        ProcEscapeThenLog(recvBuf, sz, DEBUG_LOG);
    }

    // wait until thread exit
    while (!IsWriteFileThreadExit()) {
        (void)ToolSleep(FIFTY_MILLI_SECOND);
    }

    SyslogCleanUp();
    LOG_CLOSE_SOCK_S(sockhandle);
    SELF_LOG_ERROR("signal=%d, quit slogd process...", g_gotSignal);
}
#endif
