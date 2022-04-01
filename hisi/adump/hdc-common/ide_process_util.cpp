/**
 * @file ide_process_util.cpp
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#include "ide_process_util.h"
#include <vector>
#if (OS_TYPE != WIN)
#include <sys/resource.h>
#endif
#include <mutex>
#include "log/adx_log.h"
#include "common/memory_utils.h"

using std::vector;
using namespace std;
namespace Adx {
#if (OS_TYPE != WIN)
static pid_t     *g_childPid = nullptr;   /* ptr to array allocated at run-time */
static uint32_t   g_maxfd = 0;            /* from our OpenMax(), {Prog openmax} */
static std::mutex g_popenMtx;             /* mtx for Popen */
#endif

/**
 * @brief get the max file describe number
 *
 * @return
 *        0: get max file describe number failed
 *        other: the max file describe number
 */
#if (OS_TYPE != WIN)
uint32_t OpenMax()
{
    struct rlimit limit;
    (void)memset_s(&limit, sizeof(limit), 0, sizeof(limit));
    if (getrlimit(RLIMIT_NOFILE, &limit) == -1) {
        IDE_LOGE("get rlimit failed");
        return 0;
    }

    return (uint32_t)limit.rlim_cur;
}
#endif

/**
 * @brief check the Popen arguments is valid or not
 * @param type: Popen arguments
 *
 * @return
 *         0: check Popen args failed
 *         1: check Popen args succ
 */
int IdeCheckPopenArgs(IdeString type, size_t len)
{
    if (type == nullptr || len < 1) {
        return 0;
    }

    if ((type[0] != 'r' && type[0] != 'w') || type[1] != 0) {
        return 0;
    }

    return 1;
}

/**
 * @brief popen corresponding security version, execute commands through execl
 * @param command: the command to exec
 * @param type: 'r' or 'w' means read or write
 *
 * @return
 *         NULL: Popen failed
 *         other: Popen succ
 */
#if (OS_TYPE == WIN)
FILE *Popen(IdeString command, IdeString type)
{
    return _popen(command, type);
}
#else
int InitChildPid()
{
    std::lock_guard<std::mutex> lock(g_popenMtx);
    if (g_childPid == nullptr) {
        /* allocate zeroed out array for child pids */
        g_maxfd = OpenMax();
        if (g_maxfd > 0) {
            if (((uint64_t)g_maxfd) * sizeof(pid_t) > UINT32_MAX) {
                IDE_LOGE("maxfd too big for malloc memory, maxfd: %u", g_maxfd);
                return IDE_DAEMON_ERROR;
            }

            size_t size = g_maxfd * sizeof(pid_t);
            g_childPid = reinterpret_cast<pid_t *>(IdeXmalloc(size));
            if (g_childPid == nullptr) {
                IDE_LOGE("Xmalloc failed");
                return IDE_DAEMON_ERROR;
            }
        }
    }
    return IDE_DAEMON_OK;
}

int PopenSetType(int &rfd, int &wfd, IdeString type)
{
    int ret = 0;
    IDE_CTRL_VALUE_FAILED(type != nullptr, return -1, "type input error");
    if (*type == 'r') {
        close(rfd);
        rfd = -1;
        if (wfd != STDOUT_FILENO) {
            (void)dup2(wfd, STDERR_FILENO); // stderr redirect to wfd
            ret = dup2(wfd, STDOUT_FILENO);
            close(wfd);
            wfd = -1;
        }
    } else {
        close(wfd);
        wfd = -1;
        if (rfd != STDIN_FILENO) {
            ret = dup2(rfd, STDIN_FILENO);
            close(rfd);
            rfd = -1;
        }
    }
    return ret;
}

FILE *FdToFileStream(int &rfd, int &wfd, IdeString type)
{
    FILE *fp = nullptr;
    IDE_CTRL_VALUE_FAILED(type != nullptr, return fp, "type input error");
    /* parent */
    if (*type == 'r') {
        close(wfd);
        wfd = -1;
        fp = fdopen(rfd, type);
    } else {
        close(rfd);
        rfd = -1;
        fp = fdopen(wfd, type);
    }

    return fp;
}

FILE *PopenCommon(IdeString command, IdeStringBuffer env[], IdeString type, IdePidPtr processId)
{
    uint32_t i = 0;
    const uint8_t popenPipeSize = 2;
    const uint8_t popenExitCode = 127;
    int pfd[popenPipeSize] = {0};
    FILE *fp = nullptr;
    IdeString exe = "/bin/sh";
    IdeString sh = "sh";
    IdeString shArg = "-c";
    pid_t pid = 0;
    IDE_CTRL_VALUE_FAILED(command != nullptr, return nullptr, "command input error");
    IDE_CTRL_VALUE_FAILED(type != nullptr, return nullptr, "type input error");
    /* only allow "r" or "w" */
    IDE_CTRL_VALUE_FAILED(IdeCheckPopenArgs(type, strlen(type)) != 0, return nullptr, "type check error");
    IDE_CTRL_VALUE_FAILED(InitChildPid() == IDE_DAEMON_OK, return nullptr, "Iinit child pid failed");
    /* first time through */
    IDE_CTRL_VALUE_FAILED(pipe(pfd) >= 0, return nullptr, "pipe failed");
    if ((pid = fork()) < 0) {
        IDE_LOGE("fork failed");
        return nullptr;            /* errno set by fork() */
    } else if (pid == 0) {         /* child */
        if (PopenSetType(pfd[0], pfd[1], type) == -1) {
            IDE_LOGW("popen pipe type set invalid");
        }
        /* close all descriptors in childpid[] */
        for (i = 0; i < g_maxfd; i++) {
            if (g_childPid[i] > 0) {
                close(i);
            }
        }
        // After the parent process dies, the child process will receive the SIGHUP signal,
        // and the child process will also exit.
        prctl(PR_SET_PDEATHSIG, SIGHUP);
        IdeStringBuffer argv[] = {const_cast<IdeStringBuffer>(sh), const_cast<IdeStringBuffer>(shArg),
            const_cast<IdeStringBuffer>(command), nullptr};
        if (env == NULL) {
            execvp(exe, argv);
        } else {
            execvpe(exe, argv, env);
        }
        IDE_EXIT(popenExitCode);
    }
    /* parent */
    fp = FdToFileStream(pfd[0], pfd[1], type);
    if (fp != nullptr) {
        g_childPid[fileno(fp)] = pid;     /* remember child pid for this fd */
        *processId = pid;
    }
    return fp;
}

FILE *Popen(IdeString command, IdeString type)
{
    pid_t pid = 0;
    return PopenCommon(command, nullptr, type, &pid);
}

#endif
/**
 * @brief close the handle of the open execution command
 * @param fp: the handle to close
 *
 * @return
 *         -1: close file point failed
 *         other: close succ
 */
#if (OS_TYPE == WIN)
int Pclose(FILE* fp)
{
    return _pclose(fp);
}
#else
int Pclose(FILE* fp)
{
    int fd = 0;
    int stat = 0;
    pid_t pid;

    if (fp != nullptr) {
        fd = fileno(fp);
        fclose(fp);
    } else {
        return -1;
    }

    if (g_childPid == nullptr) {
        return -1;      /* popen() has never been called */
    }

    pid = g_childPid[fd];
    if (pid == 0) {
        return -1;      /* fp wasn't opened by popen() */
    }

    g_childPid[fd] = 0;

    while (waitpid(pid, &stat, 0) < 0) {
        if (errno != EINTR) {
            return -1;  /* error other than EINTR from waitpid() */
        }
    }

    return(stat);       /* return child's termination status */
}
#endif
/**
 * @brief Release childpid memory
 *
 * @return
 */
#if (OS_TYPE != WIN)
void PopenClean()
{
    IDE_XFREE_AND_SET_NULL(g_childPid);
    g_maxfd = 0;
}
#endif
}
