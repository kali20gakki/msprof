/**
 * @common.c
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "securec.h"
#include "common.h"
int g_gotSignal;
STATIC int SigActionSet(int sigNum, const struct sigaction *act);

void RecordSigNo(int sigNo)
{
    g_gotSignal = sigNo;
}

int GetSigNo(void)
{
    return g_gotSignal;
}

int DaemonizeProc(void)
{
#if (OS_TYPE_DEF == LINUX)
    int noChdir = 0;
    int noClose = 1;
    if (daemon(noChdir, noClose) < 0) {
        SELF_LOG_ERROR("create daemon failed, strerr=%s.", strerror(ToolGetErrorCode()));
        return SYS_ERROR;
    }
#endif
    return SYS_OK;
}

void SignalNoSaRestartEmptyMask(int sig, void (*handler)(int))
{
#if (OS_TYPE_DEF == LINUX)
    if (handler != NULL) {
        struct sigaction sa;
        (void)memset_s(&sa, sizeof(sa), 0, sizeof(sa));
        sa.sa_handler = handler;
        int ret = SigActionSet(sig, &sa);
        ONE_ACT_WARN_LOG(ret < 0, return, "SigActionSet failed, result is %d.", ret);
        SELF_LOG_INFO("examine and change a signal action, signal_type=%d.", sig);
    }
#endif
}

STATIC int SigActionSet(int sigNum, const struct sigaction *act)
{
#if (OS_TYPE_DEF == LINUX)
    ONE_ACT_WARN_LOG(act == NULL, return 0, "[input] parameter is null.");
    return sigaction(sigNum, act, NULL);
#else
    return 0;
#endif
}
