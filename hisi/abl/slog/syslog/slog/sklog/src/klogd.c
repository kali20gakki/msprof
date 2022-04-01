/**
 * @klogd.c
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#include "klogd.h"
#include "common.h"
#include "constant.h"
#include "log_common_util.h"
#include "slog.h"
#include "start_single_process.h"
#include "cfg_file_parse.h"
#include "log_monitor.h"
#ifdef IAM
#include "iam.h"
#endif

#define BASE_HEX 16
#define BASE_DECIMAL 10
#define INDEX_UNIT 4
#define KLOG_BUFF_SIZE (16 * 1024 * 1024)
#define POSITION_UNIT 2
#define KLOG_PATH "/dev/kmsg"
static int g_fKlog = INVALID;
static char g_sklogdPidFile[WORKSPACE_PATH_MAX_LENGTH] = "\0";

struct {
    int i;
    int n;
} g_argvOpt = { 0, 0 };

/**
 * @brief IsDigit: judge input character if digit or not
 * @param [in]c: input character
 * @return: 1(is digit), 0(not digit)
 */
int IsDigit(char c)
{
    return ((c >= '0') && (c <= '9')) ? 1 : 0;
}

unsigned int GetChValue(char ch)
{
    unsigned int c = 0;
    if (IsDigit(ch) != 0) {
        // number_in_char - '0' = number
        c = ch - '0';
    } else if ((ch >= 'a') && (ch <= 'z')) {
        // hex char offset + BASE_DECIMAL = integer number
        c = ch - 'a' + BASE_DECIMAL;
    } else if ((ch >= 'A') && (ch <= 'Z')) {
        c = ch - 'A' + BASE_DECIMAL;
    } else {
        c = ch;
    }
    return c;
}

void DecodeMsg(char *msg, unsigned int length)
{
    if ((msg == NULL) || (length == 0)) {
        return;
    }
    int idx = 0;
    char ch = '\0';
    const char *ptr = msg;
    unsigned len = 0;
    while ((*ptr != '\0') && (len <= length)) {
        if ((*ptr == '\\') && (*(ptr + 1) == 'x')) {
            ptr += POSITION_UNIT;
            ch = (GetChValue(*ptr) * BASE_HEX) + GetChValue(*(ptr + 1));
            ptr += POSITION_UNIT;
            len = len + INDEX_UNIT;
        } else {
            ch = *ptr;
            ptr++;
            len++;
        }
        len++;
        msg[idx] = ch;
        idx++;
    }
    msg[idx] = '\0';
    idx++;
    return;
}

/**
 * @brief CheckProessBufParm: check param and malloc for heap, copy msg to heap
 * @param [in]msg: input msg
 * @param [in]length: malloc length for heapBuf
 * @param [in/out]heapBuf: pointer to msg copy
 * @return: EOK(0), INVALID(-1)
 */
int CheckProessBufParm(const char *msg, unsigned int length, char **heapBuf)
{
    ONE_ACT_WARN_LOG(msg == NULL, return INVALID, "[input] message_buffer is null.");
    ONE_ACT_WARN_LOG((length == 0) || (length > KLOG_BUFF_SIZE), return INVALID,
                      "[input] length is invalid, length=%u.", length);
    ONE_ACT_WARN_LOG(heapBuf == NULL, return INVALID, "[input] heap buffer array is null.");

    *heapBuf = (char *)malloc(length);
    if (*heapBuf == NULL) {
        SELF_LOG_WARN("malloc failed, strerr=%s.", strerror(ToolGetErrorCode()));
        return INVALID;
    }
    (void)memset_s(*heapBuf, length, 0x00, length);

    int ret = memcpy_s(*heapBuf, length, msg, length);
    if (ret != EOK) {
        SELF_LOG_WARN("memcpy_s failed, result=%d, strerr=%s.", ret, strerror(ToolGetErrorCode()));
        XFREE(*heapBuf);
        return INVALID;
    }
    return EOK;
}

/**
* @brief ProcessBuf: process kernel msg with timestamp, level and msg itself
* @param [in]msg: input kernel msg
* @param [in]length: length of input kernel msg
* @return: EOK(0)/INVALID(-1)(int)
*/
int ProcessBuf(char *msg, unsigned int length)
{
    unsigned long int timestamp = 0;
    unsigned short level = 0;
    char *heapBuf = NULL;

    if (CheckProessBufParm(msg, length, &heapBuf) == INVALID) {
        return INVALID;
    }

    char *buf = heapBuf;
    for (; (*buf != '\0') && (IsDigit(*buf) != 0); buf++) {
        if (level > (((USHRT_MAX) - (*buf - '0')) / BASE_DECIMAL)) {
            XFREE(heapBuf);
            return INVALID;
        }
        level = (level * BASE_DECIMAL) + (*buf - '0');
    }
    level = level & SKLOG_PRIMASK;
    buf++;

    // skip leading sequence number
    for (; (*buf != '\0') && (IsDigit(*buf) != 0); buf++) {
    }
    buf++;

    // get timestamp
    for (; (*buf != '\0') && (IsDigit(*buf) != 0); buf++) {
        if (timestamp > (((unsigned long)(ULONG_MAX) - (*buf - '0')) / BASE_DECIMAL)) {
            XFREE(heapBuf);
            return INVALID;
        }
        timestamp = (timestamp * BASE_DECIMAL) + (*buf - '0');
    }
    buf++;

    // skip everything before message body
    while ((*buf != '\0') && (*buf != ';')) {
        buf++;
    }
    buf++;

    DecodeMsg(buf, length);
    (void)memset_s(msg, length, 0x00, length);

    int ret = sprintf_s(msg, length - 1, "<%u>[%lu.%06lu] %s", level,
                        timestamp / UNIT_US_TO_S, timestamp % UNIT_US_TO_S, buf);
    if (ret == -1) {
        SELF_LOG_WARN("sprintf_s failed, result=%d, strerr=%s.", ret, strerror(ToolGetErrorCode()));
        XFREE(heapBuf);
        return INVALID;
    }
    XFREE(heapBuf);
    return EOK;
}

void OpenKernelLog(void)
{
    g_fKlog = open(KLOG_PATH, O_RDONLY, 0);
    if (g_fKlog == INVALID) {
        SELF_LOG_ERROR("open file failed, file=%s, strerr=%s.", KLOG_PATH, strerror(ToolGetErrorCode()));
        return;
    }

    off_t ret = lseek(g_fKlog, 0, SEEK_END);
    if (ret == INVALID) {
        SELF_LOG_ERROR("lseek file failed, file=%s, strerr=%s.", KLOG_PATH, strerror(ToolGetErrorCode()));
        CloseKernelLog();
    }
    return;
}

int ReadKernelLog(char *bufP, int len)
{
    if ((bufP == NULL) || (g_fKlog == INVALID)) {
        return INVALID;
    }
    return read(g_fKlog, bufP, len);
}

void CloseKernelLog(void)
{
    LOG_MMCLOSE_AND_SET_INVALID(g_fKlog);
}

void ParseArgv(int argc, const char **argv)
{
    int opts = getopt(argc, (char *const *)argv, "c:n");
    if (opts == INVALID) {
        return;
    }
    do {
        switch (opts) {
            case 'c':
                if (optarg == NULL) {
                    return;
                }
                g_argvOpt.i = atoi(optarg);
                break;
            case 'n':
                g_argvOpt.n = 1;
                break;
            default:
                break;
        }
        opts = getopt(argc, (char *const *)argv, "c:n");
    } while (opts != INVALID);
}

void KLogToSLog(unsigned int priority, const char *msg)
{
    if (msg != NULL) {
        switch (SKLOG_PRIMASK & priority) {
            case SKLOG_EMERG:
            case SKLOG_ALERT:
            case SKLOG_CRIT:
            case SKLOG_ERROR:
                dlog_error((KERNEL), "%s", msg);
                break;
            case SKLOG_WARNING:
                dlog_warn((KERNEL), "%s", msg);
                break;
            case SKLOG_NOTICE:
                dlog_event((KERNEL), "%s", msg);
                break;
            case SKLOG_INFO:
                dlog_info((KERNEL), "%s", msg);
                break;
            case SKLOG_DEBUG:
                dlog_debug((KERNEL), "%s", msg);
                break;
            default:
                dlog_info((KERNEL), "%s", msg);
                break;
        }
    }
}

void ParseKernelLog(const char *start)
{
    unsigned long priority = DLOG_INFO;
    char endptr = '\0';
    char *pendptr = &endptr;
    if (start == NULL) {
        SELF_LOG_ERROR("[input] start is null.");
        return;
    }

    const char *pos = start;
    if (*pos == '<') {
        pos++;
        if ((pos != NULL) && (*pos != '\0')) {
            priority = strtoul(pos, &pendptr, BASE_NUM);
        }

        if ((pendptr != NULL) && (*pendptr == '>')) {
            pendptr++;
        }
    }

    unsigned int level = (priority > (unsigned int)(UINT_MAX)) ? (DLOG_INFO) : ((unsigned int)priority);
    if ((pendptr != NULL) && (*pendptr != '\0')) {
        KLogToSLog(level, pendptr);
    }
}

void ProcKernelLog(void)
{
    char *logBuffer = (char *)malloc(COMMON_BUFSIZE);
    if (logBuffer == NULL) {
        SELF_LOG_ERROR("malloc failed, strerr=%s.", strerror(ToolGetErrorCode()));
        return;
    }
    (void)memset_s(logBuffer, COMMON_BUFSIZE, 0x00, COMMON_BUFSIZE);
    while (GetSigNo() == 0) {
        int ret = ReadKernelLog(logBuffer, COMMON_BUFSIZE - 1);
        if (ret == -1) {
            ONE_ACT_NO_LOG(ToolGetErrorCode() == EINTR, continue);
            SELF_LOG_WARN("read file(%s) failed, result=%d, strerr=%s.", KLOG_PATH, ret, strerror(ToolGetErrorCode()));
            ToolSleep(ONE_SECOND);
            continue;
        }
        logBuffer[ret] = '\0';
        ret = ProcessBuf(logBuffer, COMMON_BUFSIZE);
        if (ret == INVALID) {
            continue;
        }
        ParseKernelLog(logBuffer);
        (void)memset_s(logBuffer, COMMON_BUFSIZE, 0x00, COMMON_BUFSIZE);
    }
    XFREE(logBuffer);
    return;
}

STATIC void SignalHandle(void)
{
    (void)signal(SIGHUP, SIG_IGN);
    SignalNoSaRestartEmptyMask(SIGHUP, RecordSigNo);     // Close terminal, process end
    SignalNoSaRestartEmptyMask(SIGINT, RecordSigNo);     // Interrupt by ctrl+c
    SignalNoSaRestartEmptyMask(SIGTERM, RecordSigNo);
#ifdef IAM
    (void)signal(SIGPIPE, SIG_IGN);
#else
    SignalNoSaRestartEmptyMask(SIGPIPE, RecordSigNo);    // Write to pipe with no readers
#endif
    SignalNoSaRestartEmptyMask(SIGQUIT, RecordSigNo);    // Interrupt by ctrl+"\"
    SignalNoSaRestartEmptyMask(SIGABRT, RecordSigNo);
    SignalNoSaRestartEmptyMask(SIGALRM, RecordSigNo);
    SignalNoSaRestartEmptyMask(SIGVTALRM, RecordSigNo);
    SignalNoSaRestartEmptyMask(SIGXCPU, RecordSigNo);    // Exceeds CPU time limit
    SignalNoSaRestartEmptyMask(SIGXFSZ, RecordSigNo);    // Exceeds file size limit
    SignalNoSaRestartEmptyMask(SIGUSR1, RecordSigNo);    // user custom signal, process end
    SignalNoSaRestartEmptyMask(SIGUSR2, RecordSigNo);
}

#ifdef __IDE_UT
int KlogdLltMain(int argc, char **argv)
#else
int main(int argc, char **argv)
#endif
{
#ifdef IAM
    NO_ACT_WARN_LOG(IAMResMgrReady() != SYS_OK, "IAMResMgrReady failed, strerr=%s", strerror(ToolGetErrorCode()));
#endif
    InitCfgAndSelfLogPath();
    OpenKernelLog();
    ParseArgv(argc, (const char **)argv);
    if (g_argvOpt.n == 0) {
        if (DaemonizeProc() != SYS_OK) {
            SELF_LOG_ERROR("fork failed, strerr=%s, start sklogd failed.", strerror(ToolGetErrorCode()));
            goto SKLOGD_EXIT;
        }
    }
    NO_ACT_WARN_LOG(LogMonitorRegister(SKLOGD_MONITOR_FLAG) != 0,
                    "log monitor register failed, flag=%d", SKLOGD_MONITOR_FLAG);
    LogMonitorInit(SKLOGD_MONITOR_FLAG);
#ifndef IAM
    if (LogInitWorkspacePath() != SYS_OK) {
        SELF_LOG_ERROR("Workspace Path=%s not exist and quit sklogd process.", GetWorkspacePath());
        goto SKLOGD_EXIT;
    }
    if (StrcatDir(g_sklogdPidFile, SKLOGD_PID_FILE,
        GetWorkspacePath(), WORKSPACE_PATH_MAX_LENGTH) != SYS_OK) {
        SELF_LOG_ERROR("StrcatDir g_sklogdPidFile failed and quit sklogd process.");
        goto SKLOGD_EXIT;
    }
    if (JustStartAProcess(g_sklogdPidFile) != SYS_OK) {
        SELF_LOG_ERROR("only start one sklogd failed, strerr=%s.", strerror(ToolGetErrorCode()));
        goto SKLOGD_EXIT;
    }
#endif

    SignalHandle();
    SELF_LOG_INFO("sklogd process started......");
    LogAttr logAttr;
    logAttr.type = SYSTEM;
    logAttr.pid = 0;
    logAttr.deviceId = 0;
    if (DlogSetAttr(logAttr) != SYS_OK) {
        SELF_LOG_WARN("Set log attr failed.");
    }
    ProcKernelLog();

    // call printself log before free source, or it will write to default path
    SELF_LOG_ERROR("sklogd process quit, signal=%d.", g_gotSignal);
#ifndef IAM
    SingleResourceCleanup(g_sklogdPidFile);
#endif
SKLOGD_EXIT:
    CloseKernelLog();
    FreeCfgAndSelfLogPath();
    return EXIT_FAILURE;
}
