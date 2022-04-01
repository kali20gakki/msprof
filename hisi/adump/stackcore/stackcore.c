/**
 * @file stackcore.c
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "stackcore.h"
#include <ucontext.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include "securec.h"
#ifdef STACKCORE_DEBUG
#include "slog.h"
#endif

#ifdef LLT_TEST
#define STATIC
#define CORE_DEFAULT_PATH   "llt_test_"
#else
#define STATIC              static
#ifndef CORE_DEFAULT_PATH
#define CORE_DEFAULT_PATH   "/var/log/npu/coredump/"
#endif
#endif

#define SELF_MAP_PATH       "/proc/self/maps"
#define EXEC_READ_MAP       "r"
#define EXEC_MAP_ATTR       "r-xp"
#define STACK_SECTION       "[stack]\n"
#define OS_SPLIT            '/'

#define CORE_BUFFER_LEN     512
#define CORE_MASK           0640
#define CORE_DUMP_MASK      0440
#define MIN_NAME_LEN        256 // stackcore.<exec>.<pid>.<signo>.<timestamp>
#define CONSTRUCTOR         __attribute__((constructor))
#define DESTRUCTOR          __attribute__((destructor))
#define MAX_STACK_LAYER     19 // 0 ~ 19 : 20 layer
#ifdef __x86_64__
#define FP_REGISTER         15
#define LR_REGISTER         16
#else
#define FP_REGISTER         29
#endif
#define LR_OFFSET           8
#define PC_OFFSET           4

#define MAX_ERRSTR_LEN      128 // maxlen of buffer for strerror_r

#ifdef STACKCORE_DEBUG
static inline const char *FormatErrStr(int errIdx, char *errStr, unsigned int strMaxLen)
{
    char *retStr = strerror_r(errIdx, errStr, strMaxLen);
    return retStr;
}

#define LOGI(format, ...) do {                                         \
    dlog_info(IDEDD, "[STACKCORE]>>> " format "\n", ##__VA_ARGS__);    \
} while (0)

#define LOGW(format, ...) do {                                         \
    dlog_warn(IDEDD, "[STACKCORE]>>> " format "\n", ##__VA_ARGS__);    \
} while (0)

#define LOGE(format, ...) do {                                         \
    dlog_error(IDEDD, "[STACKCORE]>>> " format "\n", ##__VA_ARGS__);   \
} while (0)

#else
#define LOGI(format, ...) do {                                         \
} while (0)

#define LOGW(format, ...) do {                                         \
} while (0)

#define LOGE(format, ...) do {                                         \
} while (0)

#endif

struct StackSigAction {
    unsigned char done;
    int signo;
    struct sigaction sigAct;
    struct sigaction oldSigAct;
};

/* core signals : 3, 5, 6, 7, 8, 10, 11, 12, 24, 25, 30, 31 */
struct StackSigAction g_sigActs[] = {
    {.done = 0, .signo = SIGQUIT, .sigAct = {{NULL}}, .oldSigAct = {{NULL}}},
    {.done = 0, .signo = SIGTRAP, .sigAct = {{NULL}}, .oldSigAct = {{NULL}}},
    {.done = 0, .signo = SIGABRT, .sigAct = {{NULL}}, .oldSigAct = {{NULL}}},
    {.done = 0, .signo = SIGBUS, .sigAct = {{NULL}}, .oldSigAct = {{NULL}}},
    {.done = 0, .signo = SIGFPE, .sigAct = {{NULL}}, .oldSigAct = {{NULL}}},
    {.done = 0, .signo = SIGILL, .sigAct = {{NULL}}, .oldSigAct = {{NULL}}},
    {.done = 0, .signo = SIGSEGV, .sigAct = {{NULL}}, .oldSigAct = {{NULL}}},
    {.done = 0, .signo = SIGXCPU, .sigAct = {{NULL}}, .oldSigAct = {{NULL}}},
    {.done = 0, .signo = SIGXFSZ, .sigAct = {{NULL}}, .oldSigAct = {{NULL}}},
    {.done = 0, .signo = SIGSYS, .sigAct = {{NULL}}, .oldSigAct = {{NULL}}}
};

/**
 * @brief CONSTRUCTOR init when link stackcore
 */
STATIC CONSTRUCTOR void Init(void)
{
    (void)StackInit();
}

/**
 * @brief get stackcore file name
 * @param [out] name : name buffer
 * @param [in] len : name buffer length
 * @param [in] signo : exception signal number
 * @return
 *       0 : success
 *     -1 : failed
 */
STATIC int StackCoreName(char *name, unsigned int len, int signo)
{
    if (name == NULL || len < MIN_NAME_LEN) {
        return -1;
    }

    time_t ts = time(NULL);
    if (ts == (time_t)-1) {
        LOGE("get timestamp failed");
        return -1;
    }
    /* stackcore.<execname>.<pid>.<signo>.<timestamp> */
    int ret = snprintf_s(name, len, len - 1, "%sstackcore.%s.%d.%d.%ld",
        CORE_DEFAULT_PATH, program_invocation_short_name, getpid(), signo, ts);
    if (ret == -1) {
        LOGE("printf core name failed");
        return -1;
    }

    return 0;
}

/**
 * @brief create a file
 * @param [in] fileName : file name
 * @return
 *       0 : success
 *     -1 : failed
 */
STATIC int StackOpen(const char *fileName)
{
    if (fileName == NULL) {
        return -1;
    }
    return open(fileName, O_CREAT | O_RDWR, CORE_MASK);
}

/**
 * @brief read a line data from file
 * @param [in] fd : file descriptor
 * @param [out] data : read data
 * @param [in] len : data buffer length
 * @return
 *       0 : success
 *     -1 : failed
 */
STATIC ssize_t StackReadLine(int fd, char *data, unsigned int len)
{
    if (fd < 0 || data == NULL || len == 0) {
        return -1;
    }

    ssize_t n = 0;
    char c = EOF;
    char *ptr = data;
    for (n = 1; n < len; n++) {
        ssize_t rc = 0;
        if ((rc = read(fd, &c, 1)) == 1) {
            *ptr++ = c;
            if (c == '\n') {
                break;
            }
        } else if (rc == 0) {
            *ptr = '\0';
            return (n - 1);
        } else {
            if (errno == EINTR) {
                n--;
                continue;
            }
            return -1;
        }
    }
    *ptr = '\0';
    return n;
}

/**
 * @brief write data to file
 * @param [in] fd : file descriptor
 * @param [in] data : write data
 * @param [in] len : data buffer length
 * @return
 *       0 : success
 *     -1 : failed
 */
STATIC ssize_t StackWriteData(int fd, const char *data, unsigned int len)
{
    if (fd < 0 || data == NULL || len == 0) {
        return -1;
    }

    unsigned int reserved = len;
    do {
        ssize_t wrLen = write(fd, data + (len - reserved), reserved);
        if (wrLen < 0 && errno == EINTR) {
            wrLen = 0;
        } else if (wrLen < 0) {
            return -1;
        }
        reserved -= wrLen;
    } while (reserved != 0);
    return len;
}

/**
 * @brief close file descriptor
 * @param [in] fd : file descriptor
 * @return
 *       0 : success
 *     other : failed
 */
STATIC int StackClose(int fd)
{
    if (fd < 0) {
        return 0;
    }

    return close(fd);
}

/**
 * @brief get current pc self map
 * @param [in] pc : exception PC register
 * @param [out] data :  exception map info
 * @param [in] len :  data buffer len
 * @return None
 */
STATIC void GetSelfMap(uintptr_t pc, char *data, unsigned int len)
{
    char *vmPath = NULL;
    uintptr_t vmStart = 0;
    uintptr_t vmEnd = 0;
    if (data == NULL || len == 0) {
        return;
    }

    int fd = open(SELF_MAP_PATH, O_RDONLY);
    if (fd >= 0) {
        while (StackReadLine(fd, data, len) > 0) {
            vmPath = strchr(data, OS_SPLIT);
            if (vmPath == NULL || strstr(data, EXEC_MAP_ATTR) == NULL) {
                continue;
            }

            if (sscanf_s(data, "%lx-%lx", &vmStart, &vmEnd) <= 0) {
                continue;
            }

            if (pc < vmStart || pc > vmEnd) {
                continue;
            }

            int ret = snprintf_s(data, len, len - 1, "%018p %s", (void *)vmStart, vmPath);
            if (ret == -1) {
                LOGE("snprintf_s map info failed");
            }
            break;
        }
        close(fd);
        fd = -1;
    } else {
        LOGW("self map fopen null");
    }
}

/**
 * @brief reason thre stack frame by FP
 * @param [in] layer : exception layer
 * @param [in] nfp : exception FP register
 * @param [out] data :  exception info
 * @param [in] len :  data buffer len
 * @return
 *         0 : reason end
 *      other : continue to reason
 */
STATIC uintptr_t StackFrame(int layer, uintptr_t fp, char *data, unsigned int len)
{
    uintptr_t nfp = 0;
    uintptr_t pc = 0;
    char info[CORE_BUFFER_LEN] = {'\0'};
    if (fp == 0 || layer > MAX_STACK_LAYER || data == NULL || len == 0) {
        return 0;
    }

    if (layer == 0) { // current pc frame
        pc = fp;
        nfp = pc;
    } else { // fp inference pc frame
        uintptr_t lr = fp + LR_OFFSET;
        if (lr < fp) {
            return 0;
        }
        pc = *(uintptr_t *)lr;
        nfp = *(uintptr_t *)fp;
    }
    GetSelfMap(pc, info, sizeof(info));
    int ret = snprintf_s(data, len, len - 1, "#%d %018p %s", layer, (void *)pc, info);
    if (ret == -1) {
        LOGE("snprintf_s core info failed");
        return 0;
    }
    return nfp;
}

/**
 * @brief reason thre stack frame by FP
 * @param [in] layer : exception layer
 * @param [in] pc : current pc
 * @param [out] data :  exception info
 * @param [in] len :  data buffer len
 * @return
 *         0 : reason end
 *      other : continue to reason
 */
STATIC void StackPcFrame(int layer, uintptr_t pc, char *data, unsigned int len)
{
    if (layer > MAX_STACK_LAYER || data == NULL || len == 0) {
        return;
    }

    if (pc == 0) { // current pc frame
        int ret = snprintf_s(data, len, len - 1, "#%d 0x%016lx 0x%016lx %s\n",
            layer, pc, pc, program_invocation_name);
        if (ret == -1) {
            LOGE("snprintf_s core info failed");
        }
    }
    (void)StackFrame(layer, pc, data, len);
}

/**
 * @brief create exception stackcore file
 * @param [in] nfp : exception FP register
 * @param [in] pc :  exception PC register
 * @param [in] signo :  exception signal number
 * @return
 *         -1 : failed
 *          0 : success
 */
STATIC ssize_t CreateStackCore(const uintptr_t nfp, uintptr_t pc, int signo)
{
    int layer = 0;
    char info[CORE_BUFFER_LEN] = {'\0'};
#ifdef STACKCORE_DEBUG
    char errBuf[MAX_ERRSTR_LEN + 1] = {0};
#endif
    uintptr_t fp = nfp;
    ssize_t ret = StackCoreName(info, sizeof(info), signo);
    if (ret != 0) {
        return ret;
    }
    int fd = StackOpen(info);
    if (fd < 0) {
        LOGE("create core file %s failed, info : %s", info, FormatErrStr(errno, errBuf, MAX_ERRSTR_LEN));
        return -1;
    }
    ret = StackWriteData(fd, STACK_SECTION, strlen(STACK_SECTION));
    if (ret == -1) {
        LOGE("write core file failed, info : %s", FormatErrStr(errno, errBuf, MAX_ERRSTR_LEN));
        goto ERROR;
    }

    // print pc frame
    StackPcFrame(layer, pc, info, sizeof(info));
    ret = StackWriteData(fd, info, strlen(info));
    if (ret == -1) {
        LOGE("write core file failed, info : %s", FormatErrStr(errno, errBuf, MAX_ERRSTR_LEN));
        goto ERROR;
    }

    // write stack frame info
    while (fp != 0 && fp >= nfp && layer < MAX_STACK_LAYER) {
        layer++;
        fp = StackFrame(layer, fp, info, sizeof(info));
        ret = StackWriteData(fd, info, strlen(info));
        if (ret < 0) {
            LOGE("write core file failed, info : %s", FormatErrStr(errno, errBuf, MAX_ERRSTR_LEN));
            goto ERROR;
        }
    }
    ret = fchmod(fd, CORE_DUMP_MASK); // change the file mode for dump(440)
    if (ret < 0) {
        LOGE("chmod mode failed, info : %s", FormatErrStr(errno, errBuf, MAX_ERRSTR_LEN));
    }
ERROR:
    (void)StackClose(fd);
    fd = -1;
    return ret;
}

/**
 * @brief analysis exception context
 * @param [in] mcontext : exception context
 * @param [in] signo :  exception signal number
 * @return None
 */
STATIC void AnalysisContext(const mcontext_t *mcontext, int signo)
{
    uintptr_t nfp = 0;
    uintptr_t pc = 0;
    if (mcontext == NULL) {
        return;
    }

#ifdef __x86_64__
    nfp = mcontext->gregs[FP_REGISTER];
    pc = mcontext->gregs[LR_REGISTER];
#else
    nfp = mcontext->regs[FP_REGISTER];
    pc = mcontext->pc;
#endif
    if (nfp == 0) {
        LOGE("fp register exception");
        return;
    }

    ssize_t ret = CreateStackCore(nfp, pc, signo);
    if (ret < 0) {
        LOGE("create stack core exception");
    }
}

/**
 * @brief process the exception signal
 * @param [in] sigNum : signal number
 * @param [in] info :  signal infomation
 * @param [in] data : signal context
 * @return None
 */
STATIC void StackSigHandler(int sigNum, siginfo_t *info, void *data)
{
    if (info == NULL || data == NULL) {
        return;
    }

    ucontext_t *utext = (ucontext_t *)data;
    mcontext_t *mcontext = (mcontext_t *)&(utext->uc_mcontext);
    LOGI("stack core handler [%d] entry", sigNum);
    AnalysisContext(mcontext, sigNum);
    LOGI("stack core handler [%d] exit", sigNum);
    // recover thre signal handler
    for (unsigned int i = 0; i < sizeof(g_sigActs) / sizeof(struct StackSigAction); i++) {
        if (info->si_signo != g_sigActs[i].signo || g_sigActs[i].done != 1) {
            continue;
        }
        if (sigaction(g_sigActs[i].signo, &g_sigActs[i].oldSigAct, NULL) < 0) {
            g_sigActs[i].done = 0;
            #ifdef STACKCORE_DEBUG
            char errBuf[MAX_ERRSTR_LEN + 1] = {0};
            #endif
            LOGE("recover sigaction signo:[%d] failed, info : %s", info->si_signo,
                    FormatErrStr(errno, errBuf, MAX_ERRSTR_LEN));
            return;
        }
        int ret = raise(info->si_signo);
        if (ret != 0) {
            LOGE("raise signal %d failed!", info->si_signo);
        }
    }
}

/**
 * @brief register the exception signal handler
 * @param None
 * @return
 *             0 : success
 *        others : failed
 */
int StackInit(void)
{
    static unsigned char stackInitFlag = 0;
    LOGI("stack init [%u] entry", stackInitFlag);
    if (stackInitFlag == 0) {
        stackInitFlag = 1; // have been inited
        for (unsigned int i = 0; i < sizeof(g_sigActs) / sizeof(struct StackSigAction); i++) {
            (void)sigemptyset(&(g_sigActs[i].sigAct.sa_mask));
            g_sigActs[i].sigAct.sa_sigaction = StackSigHandler;
            g_sigActs[i].sigAct.sa_flags |= SA_SIGINFO;
            if (sigaction(g_sigActs[i].signo, &g_sigActs[i].sigAct, &g_sigActs[i].oldSigAct) < 0) {
                LOGE("sigaction signo :[%d] error", g_sigActs[i].signo);
            } else {
                g_sigActs[i].done = 1;
            }
        }
    }
    LOGI("stack init [%u] exit", stackInitFlag);
    return 0;
}
