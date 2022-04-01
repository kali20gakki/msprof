/**
 * @log_daemon.c
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#include "log_daemon.h"
#include <execinfo.h>
#include "cfg_file_parse.h"
#include "log_recv.h"
#include "log_recv_interface.h"
#include "log_common_util.h"
#include "log_monitor.h"
#include "log_zip.h"
#include "print_log.h"
#include "start_single_process.h"
#ifdef IAM
#include "iam.h"
#endif

char g_logDaemonPidFile[WORKSPACE_PATH_MAX_LENGTH] = "\0";
LogRecvAllSourceDes g_allSourceDes;

// global variables zones
STATIC volatile sig_atomic_t g_sigFlag = 1;

int GetSigFlag(void)
{
    return g_sigFlag;
}

STATIC void ParseLogdaemonArgv(int argc, const char **argv, struct Options *opt)
{
    ONE_ACT_NO_LOG(argv == NULL, return);
    ONE_ACT_NO_LOG(opt == NULL, return);
    int opts = ToolGetOpt(argc, (char **)argv, "nh:");
    while (opts != -1) {  // no use in multi-thread
        switch (opts) {
            case 'n':
                opt->n = 1;
                break;
            case 'h':
            default:
                break;
        }
        opts = ToolGetOpt(argc, (char **)argv, "nh:");
    }
}

STATIC void MainUninit(void)
{
    FreeCfgAndSelfLogPath();
}

STATIC int MainInit(int argc, const char *argv[])
{
    int nochdir = 1;
    int noclose = 1;
    struct Options opt = { 0, 0 };

#ifdef IAM
    NO_ACT_WARN_LOG(IAMResMgrReady() != SYS_OK, "IAMResMgrReady failed, strerr=%s", strerror(ToolGetErrorCode()));
    (void)signal(SIGPIPE, SIG_IGN);
#endif

    InitCfgAndSelfLogPath();
    bool isFinished = false;
    do {
        ParseLogdaemonArgv(argc, (const char **)argv, &opt);
        // create daemon
        if ((opt.n == 0) && (daemon(nochdir, noclose) < 0)) {
            SELF_LOG_ERROR("create daemon failed, strerr=%s, quit log-daemon process.", strerror(ToolGetErrorCode()));
            break;
        }
        NO_ACT_WARN_LOG(LogMonitorRegister(LOGDAEMON_MONITOR_FLAG) != 0,
                        "log monitor register failed, flag=%d", LOGDAEMON_MONITOR_FLAG);
        LogMonitorInit(LOGDAEMON_MONITOR_FLAG);
        // check if GetWorkspacePath() exist or not
#ifndef IAM
        if (LogInitWorkspacePath() != SYS_OK) {
            SYSLOG_WARN("Workspace Path=%s not exist, quit log-daemon process.\n", GetWorkspacePath());
            break;
        }
        if (StrcatDir(g_logDaemonPidFile, LOGDAEMON_PID_FILE,
            GetWorkspacePath(), WORKSPACE_PATH_MAX_LENGTH) != SYS_OK) {
            SELF_LOG_ERROR("StrcatDir g_logDaemonPidFile failed, quit log-daemon process.");
            break;
        }

        // only start one process
        if (JustStartAProcess(g_logDaemonPidFile) != 0) {
            SELF_LOG_WARN("only start a log-daemon process failed, strerr=%s.", strerror(ToolGetErrorCode()));
            break;
        }
#endif
#ifdef HARDWARE_ZIP
        GetZipCfg();
#endif
        isFinished = true;
    } while (0);

    TWO_ACT_NO_LOG(!isFinished, MainUninit(), return SYS_ERROR);
    return SYS_OK;
}

STATIC int InitRecvValue()
{
    ParentSource parentSource = (void *)0;
    ChildSource *childSources = (void *)0;
    (void)memset_s(&g_allSourceDes, sizeof(g_allSourceDes), 0, sizeof(g_allSourceDes));
    g_allSourceDes.deviceNum = MAX_DEV_NUM;

    LogRt ret = LogRecvInitParentSource(&parentSource);
    ONE_ACT_ERR_LOG(ret != SUCCESS, return SYS_ERROR,
                    "init parent source failed, result=%d, then quit log-daemon process.", ret);
    g_allSourceDes.parentSource = parentSource;
    SELF_LOG_INFO("init parent source succeed.");

    // init child source
    childSources = (ChildSource *)malloc(MAX_DEV_NUM * sizeof(ChildSource));
    if (childSources == NULL) {
        SELF_LOG_ERROR("malloc failed, strerr=%s, then quit log-daemon process.", strerror(ToolGetErrorCode()));
        return SYS_ERROR;
    }
    (void)memset_s(childSources, MAX_DEV_NUM * sizeof(ChildSource), 0, MAX_DEV_NUM * sizeof(ChildSource));
    g_allSourceDes.childSources = childSources;

    // init recv thread
    ret = LogRecvInit(parentSource, &childSources, MAX_DEV_NUM);
    ONE_ACT_ERR_LOG(ret != SUCCESS, return SYS_ERROR,
                    "init log-daemon failed, result=%d, then quit log-daemon process.", ret);
    return SYS_OK;
}

STATIC void ReleaseResource()
{
    XFREE(g_allSourceDes.childSources);
    LogRecvReleaseAllSource(&g_allSourceDes);
    // Stop bbox main thread here.
    BboxStopMainThread();
    FreeCfgAndSelfLogPath();
}

#ifndef __IDE_UT
int main(int argc, const char *argv[])
#else
int test(int argc, const char *argv[])
#endif
{
    ONE_ACT_NO_LOG(MainInit(argc, argv) != SYS_OK, return SYS_ERROR);

    // Start bbox main thread here.
    BboxStartMainThread();

    TWO_ACT_NO_LOG(InitRecvValue() != SYS_OK, ReleaseResource(), return SYS_ERROR);

    // stackcore/core file dump server thread init
#ifdef LOG_COREDUMP
    (void)AdxCoreDumpServerInit();
#endif
    // wait until exit
    while ((g_sigFlag != 0) && (IsInitError() == 0)) {
        (void)ToolSleep(FIVE_SECOND);
    }

    // wait until all device receive & write threads exit
    while ((IsAllThreadExit() == 0)) {
        (void)ToolSleep(ONE_HUNDRED_MILLISECOND);
    }
#ifndef IAM
    SingleResourceCleanup(g_logDaemonPidFile);
#endif

    // call printself log before free source, or it will write to default path
    SELF_LOG_ERROR("quit log-daemon process");
    ReleaseResource();
    return SYS_OK;
}
