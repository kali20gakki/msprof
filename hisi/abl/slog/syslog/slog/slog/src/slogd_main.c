/**
 * @slogd_main.c
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "common.h"
#include "log_sys_package.h"
#include "syslogd.h"
#include "operate_loglevel.h"
#include "start_single_process.h"
#include "app_log_watch.h"
#include "cfg_file_parse.h"
#include "log_common_util.h"
#include "log_monitor.h"
#include "log_dump.h"
#include "log_zip.h"
#include "share_mem.h"

#ifdef STACKCORE
#include "stackcore.h"
#endif

#ifdef APP_LOG_REPORT
#include "adx_log_hdc_server.h"
ToolUserBlock g_stOperateloglevel;
#endif

#ifndef IAM
STATIC char g_slogdPidFile[WORKSPACE_PATH_MAX_LENGTH] = "\0";
#endif

#define SLOGD_DEFAULT_DETACH_THREAD_ATTR {1, 0, 0, 0, 0, 1, 128 * 1024} // Default ThreadSize(128KB)

STATIC void SlogdUsage(void) // no use in multi-thread
{
    (void)printf("Usage: slogd [OPTIONS]\n\n");
    (void)printf("  -n              Run in foreground\n");
    (void)printf("  -l N            Log only messages more urgent than prio N (1-4)\n");
    (void)printf("  -h              Help\n");
}

STATIC void ParseSlogdArgv(int argc, const char **argv, struct Options *opt)
{
    ONE_ACT_NO_LOG(argv == NULL, return);
    ONE_ACT_NO_LOG(opt == NULL, return);
    int opts = ToolGetOpt(argc, (char **)argv, "nhl:");
    while (opts != -1) {  // no use in multi-thread
        int matchedFlag = 1;
        switch (opts) {
            case 'n':
                opt->n = 1;
                break;
            case 'l':
                ONE_ACT_WARN_LOG(optarg == NULL, return, "option argument is null.");
                ONE_ACT_WARN_LOG(!IsNaturalNumStr(optarg), return, "not natural number.");
                opt->l = atoi(optarg);
                break;
            case 'h':
            default:
                matchedFlag = 0;
                SlogdUsage();
                break;
        }
        ONE_ACT_NO_LOG(matchedFlag == 0, return);
        opts = ToolGetOpt(argc, (char **)argv, "nhl:");
    }
}

STATIC int MainInit(int argc, char **argv)
{
    ONE_ACT_NO_LOG(argv == NULL, return SYS_ERROR);
    struct Options opt = { 0, 0 };
    ParseSlogdArgv(argc, (const char **)argv, &opt);
    if ((opt.n == 0) && (DaemonizeProc() != SYS_OK)) {
        SELF_LOG_ERROR("create daemon failed and quit slogd process.");
        return SYS_ERROR;
    }
    NO_ACT_WARN_LOG(LogMonitorRegister(SLOGD_MONITOR_FLAG) != 0,
                    "log monitor register failed, flag=%d", SLOGD_MONITOR_FLAG);
    const char DOCKERFILE[] = ".dockerenv";
    ONE_ACT_NO_LOG(access(DOCKERFILE, F_OK) != 0, LogMonitorInit(SLOGD_MONITOR_FLAG));

    ONE_ACT_ERR_LOG(InitGlobals() != SYS_OK, return SYS_ERROR, "init global failed and quit slogd process.");

    GetZipCfg();
    ParseSyslogdCfg();
    ParseLogBufCfg();
    ParseDeviceAppDirNumsCfg();
    InitModuleDefault(&opt);
    if (ThreadInit() != SYS_OK) {
        SELF_LOG_ERROR("init level change mutex failed and quit slogd process.");
        return SYS_ERROR;
    }

#ifdef APP_LOG_REPORT
    g_stOperateloglevel.procFunc = OperateLogLevel;
    g_stOperateloglevel.pulArg = &(g_logLevel);
#endif

    // check if GetWorkspacePath() exist or not
#ifndef IAM
    if (LogInitWorkspacePath() != SYS_OK) {
        SELF_LOG_ERROR("Workspace Path=%s not exist and quit slogd process.", GetWorkspacePath());
        return SYS_ERROR;
    }
    if (StrcatDir(g_slogdPidFile, SLOGD_PID_FILE,
        GetWorkspacePath(), WORKSPACE_PATH_MAX_LENGTH) != SYS_OK) {
        SELF_LOG_ERROR("StrcatDir g_slogdPidFile failed.");
        return SYS_ERROR;
    }
#endif

#ifdef STACKCORE
    (void)StackInit();
#endif

#ifdef APP_LOG_REPORT
    if (AdxLogHdcServerInit() != SYS_OK) {
        SELF_LOG_ERROR("log hdc server init failed");
    } else {
        SELF_LOG_INFO("log hdc server init success");
    }
#endif

    return SYS_OK;
}

#ifndef IAM
STATIC int CheckProcessExist(void)
{
    int ret = JustStartAProcess(g_slogdPidFile);
    if (ret == ERR_PROCESS_STARED) {
        SELF_LOG_ERROR("maybe slogd has bean started and quit current process.");
        return ERR_PROCESS_STARED;
    }
    if (ret != ERR_OK) {
        SELF_LOG_ERROR("slogd process quit.");
        return SYS_ERROR;
    }
    return SYS_OK;
}
#endif

STATIC int InitAndWriteToShm(void)
{
    if (InitShm() != SYS_OK) {
        SELF_LOG_ERROR("create shmem failed, strerr=%s.", strerror(ToolGetErrorCode()));
        return SYS_ERROR;
    }
    // write config path to shared memory for libslog.so.
    const char *confPath = GetConfigPath();
    if ((confPath != NULL) && (strlen(confPath) != 0)) {
        int ret = LogSetConfigPathToShm(confPath);
        NO_ACT_WARN_LOG(ret != SYS_OK, "Set config path to share memory failed.");
    }
    return SYS_OK;
}

STATIC void ReleaseMemory(void)
{
    RecordSigNo(1);
    FREE_GLOBALS_PTR();
    FreeCfgAndSelfLogPath();
}

STATIC void ReleaseResource(void)
{
#ifndef IAM
    SingleResourceCleanup(g_slogdPidFile);
#endif
    FreeConfigShm();
    RemoveMsgQueue();
    // release memory in process
    ReleaseMemory();
}

#ifndef __IDE_UT
int main(int argc, char **argv)
#else
int TestMain(int argc, char **argv)
#endif
{
    InitCfgAndSelfLogPath();
    SELF_LOG_INFO("slogd process start...");

    TWO_ACT_NO_LOG(MainInit(argc, argv) != SYS_OK, ReleaseMemory(), return SYS_ERROR);
#ifndef IAM
    int ret = CheckProcessExist();
    TWO_ACT_NO_LOG(ret == ERR_PROCESS_STARED, ReleaseMemory(), return SYS_ERROR);
    TWO_ACT_NO_LOG(ret != SYS_OK, ReleaseResource(), return SYS_ERROR);
#endif
    TWO_ACT_NO_LOG(InitAndWriteToShm() != SYS_OK, ReleaseResource(), return SYS_ERROR);

    // minirc/1951mdc/lhisi not support
#ifdef LOG_COREDUMP
    CreateCoreDumpThread();
#endif
    CreateAppLogWatchThread();

#ifdef APP_LOG_REPORT
    ToolThreadAttr threadAttr = SLOGD_DEFAULT_DETACH_THREAD_ATTR;
    toolThread tid = 0;
    TWO_ACT_ERR_LOG(ToolCreateTaskWithThreadAttr(&tid, &g_stOperateloglevel, &threadAttr) != SYS_OK, ReleaseResource(),
                    return SYS_ERROR, "create task(setLevel) failed, strerr=%s, then quit slogd process.",
                    strerror(ToolGetErrorCode()));
#else
    TWO_ACT_ERR_LOG(InitModuleArrToShMem() != SYS_OK, ReleaseResource(), return SYS_ERROR,
                    "init module arr to shmem failed, quit slogd process.");
    HandleLogLevelChange();
#endif

    SELF_LOG_INFO("slogd process started...");
    ProcSyslogd();
    // call printself log before free source, or it will write to default path
    SELF_LOG_ERROR("slogd process quit...");

    ReleaseResource();
    return SYS_OK;
}
