/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: Linux interface wrapper
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2022-06-09
 */
#ifndef MMPA_API_H
#define MMPA_API_H

#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cerrno>
#include <ctime>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <pthread.h>
#include <syslog.h>
#include <dirent.h>
#include <arpa/inet.h>
#include <cstdlib>
#include <poll.h>
#include <net/if.h>
#include <cstdarg>
#include <climits>
#include <cctype>
#include <cstddef>
#include <dirent.h>
#include <getopt.h>
#include <libgen.h>
#include <malloc.h>

#include <linux/types.h>
#include <linux/hdreg.h>
#include <linux/fs.h>
#include <linux/limits.h>

#include <sys/stat.h>
#include <sys/time.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <sys/uio.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/shm.h>
#include <sys/un.h>
#include <sys/utsname.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <sys/statvfs.h>
#include <sys/prctl.h>
#include <sys/inotify.h>

#include <string>

#include "securec.h"

#define MMPA_THREAD_SCHED_RR SCHED_RR
#define MMPA_THREAD_SCHED_FIFO SCHED_FIFO
#define MMPA_THREAD_SCHED_OTHER SCHED_OTHER
#define MMPA_THREAD_MIN_STACK_SIZE PTHREAD_STACK_MIN
#define MMPA_MAX_PATH PATH_MAX
#define M_WAIT_NOHANG WNOHANG
#define M_WAIT_UNTRACED WUNTRACED

#define M_F_OK F_OK
#define M_X_OK X_OK
#define M_W_OK W_OK
#define M_R_OK R_OK

#define M_RDONLY O_RDONLY
#define M_WRONLY O_WRONLY
#define M_RDWR O_RDWR
#define M_CREAT O_CREAT
#define M_BINARY O_RDONLY
#define M_TRUNC O_TRUNC
#define M_IRWXU S_IRWXU
#define M_APPEND O_APPEND

#define M_IREAD S_IREAD
#define M_IRUSR S_IRUSR
#define M_IWRITE S_IWRITE
#define M_IWUSR S_IWUSR
#define M_IXUSR S_IXUSR
#define FDSIZE 64
#define MM_NO_ARGUMENT        no_argument
#define MM_REQUIRED_ARGUMENT  required_argument
#define MM_OPTIONAL_ARGUMENT  optional_argument

#define FREE_BUF(buf)                \
    do {                             \
        free(buf);                   \
        buf = nullptr;               \
    } while (0)

namespace Collector {
namespace Dvvp {
namespace Mmpa {
constexpr int32_t MMPA_MIN_THREAD_PIO = 1;
constexpr int32_t PATH_SIZE = 256;
constexpr uint32_t MMPA_CPUINFO_DEFAULT_SIZE = 64;
constexpr int32_t MMPA_MAX_THREAD_PIO = 99;
constexpr uint32_t MMPA_MAX_IF_SIZE = 2048;
constexpr uint32_t MMPA_MEM_MAX_LEN = 0x7fffffff;
constexpr int32_t MMPA_COMPUTER_BEGIN_YEAR = 1900;
constexpr int32_t MMPA_MIN_OS_VERSION_SIZE = 128;
constexpr int32_t MMPA_MIN_OS_NAME_SIZE = 64;
constexpr uint32_t MMPA_CPUPROC_BUF_SIZE = 256;
constexpr uint32_t MMPA_CPUINFO_DOUBLE_SIZE = 128;
constexpr int32_t MMPA_MAX_PHYSICALCPU_COUNT = 4096;
constexpr int32_t MMPA_MIN_PHYSICALCPU_COUNT = 1;

// data type define
const int MMPA_MACINFO_DEFAULT_SIZE = 18;
const int MMPA_CPUDESC_DEFAULT_SIZE = 64;
const int MMPA_MAX_SLEEP_MILLSECOND_USING_USLEEP = 1000;
const int MMPA_SEC_TO_MSEC = 1000;
const int MMPA_MSEC_TO_USEC = 1000;
const int MMPA_MAX_SLEEP_MICROSECOND_USING_USLEEP = 1000000;
const int FALSE = 0;
const int TRUE = 1;

enum class MMPA_MAC_ADDR_TYPE {
    MMPA_MAC_ADDR_FIRST_BYTE = 0,
    MMPA_MAC_ADDR_SECOND_BYTE,
    MMPA_MAC_ADDR_THIRD_BYTE,
    MMPA_MAC_ADDR_FOURTH_BYTE,
    MMPA_MAC_ADDR_FIFTH_BYTE,
    MMPA_MAC_ADDR_SIXTH_BYTE
};

using MmThread = pthread_t;
using MmMutexT = pthread_mutex_t;
using MmDirent = struct dirent;
using MmProcess = signed int;
using MmMode_t = mode_t;
using MmStructOption = struct option;
using MmStatT = struct stat;
using MmErrorMsg = int;
using MmSockHandle = int;
// function ptr define
using UserProcFunc = void *(*)(void *pulArg);
using MmFilter =  int (*)(const MmDirent *entry);
using MmSort =  int (*)(const MmDirent **a, const MmDirent **b);

// struct define
struct MmUserBlockType {
    UserProcFunc procFunc;  // Callback function pointer
    void *pulArg;           // Callback function parameters
};
using MmUserBlockT = MmUserBlockType;

struct CpuTypeTable {
    const char *key;
    const char *value;
};

struct MmThreadAttrType {
    int32_t detachFlag;    // Determine whether to set separation property 0, not to separate 1
    int32_t priorityFlag;  // Determine whether to set priority 0 and not set 1
    int32_t priority;      // Priority value range to be set 1-99
    int32_t policyFlag;    // Set scheduling policy or not 0 do not set 1 setting
    int32_t policy;        // Scheduling policy value value
                           //  MMPA_THREAD_SCHED_RR
                           //  MMPA_THREAD_SCHED_OTHER
                           //  MMPA_THREAD_SCHED_FIFO
    int32_t stackFlag;     // Set stack size or not: 0 does not set 1 setting
    uint32_t stackSize;    // The stack size unit bytes to be set cannot be less than MMPA_THREAD_STACK_MIN
};
using MmThreadAttr = MmThreadAttrType;

struct MmDiskSizeType {
    unsigned long long totalSize;
    unsigned long long freeSize;
    unsigned long long availSize;
};
using MmDiskSize = MmDiskSizeType;

struct MmArgvEnvType {
    char **argv;
    int32_t argvCount;
    char **envp;
    int32_t envpCount;
};
using MmArgvEnv = MmArgvEnvType;

struct MmMacInfoType {
    char addr[MMPA_MACINFO_DEFAULT_SIZE];  // ex:aa-bb-cc-dd-ee-ff\0
};
using MmMacInfo = MmMacInfoType;

struct MmCpuDescType {
    char arch[MMPA_CPUDESC_DEFAULT_SIZE];
    char manufacturer[MMPA_CPUDESC_DEFAULT_SIZE];    // vendor
    char version[MMPA_CPUDESC_DEFAULT_SIZE];         // modelname
    int32_t frequency;                               // cpu frequency
    int32_t maxFrequency;                            // max speed
    int32_t ncores;                                  // cpu cores
    int32_t nthreads;                                // cpu thread count
    int32_t ncounts;                                 // logical cpu nums
};
using MmCpuDesc = MmCpuDescType;

struct MmSystemTimeType {
    int32_t wSecond;             // Seconds. [0-60] (1 leap second)
    int32_t wMinute;             // Minutes. [0-59]
    int32_t wHour;               // Hours. [0-23]
    int32_t wDay;                // Day. [1-31]
    int32_t wMonth;              // Month. [1-12]
    int32_t wYear;               // Year
    int32_t wDayOfWeek;          // Day of week. [0-6]
    int32_t tmYday;             // Days in year.[0-365]
    int32_t tmIsdst;            // DST. [-1/0/1]
    long wMilliseconds;          // milliseconds
};
using MmSystemTimeT = MmSystemTimeType;

struct MmTimevalType {
    long tvSec;
    long tvUsec;
};
using MmTimeval = MmTimevalType;

struct MmTimezoneType {
    int32_t tzMinuteswest;  // How many minutes is it different from Greenwich
    int32_t tzDsttime;      // type of DST correction
};
using MmTimezone = MmTimezoneType;

struct MmTimespecType {
    long long tvSec;
    long long tvNsec;
};
using MmTimespec = MmTimespecType;

// function define
int32_t MmSleep(uint32_t milliSecond);
int32_t MmCreateTaskWithThreadAttr(MmThread *threadHandle, const MmUserBlockT *funcBlock,
    const MmThreadAttr *threadAttr);
int32_t MmJoinTask(const MmThread *threadHandle);
int32_t MmSetCurrentThreadName(const std::string &name);
MmTimespec MmGetTickCount();
int32_t MmGetFileSize(const std::string &fileName, unsigned long long *length);
int32_t MmGetDiskFreeSpace(const std::string &path, MmDiskSize *diskSize);
int32_t MmIsDir(const std::string &fileName);
int32_t MmAccess2(const std::string &pathName, int32_t mode);
int32_t MmAccess(const std::string &pathName);
char *MmDirName(char *path);
char *MmBaseName(char *path);
int32_t MmMkdir(const std::string &pathName, MmMode_t mode);
int32_t MmChmod(const std::string &filename, int32_t mode);
int32_t MmGetErrorCode();
char *MmGetErrorFormatMessage(MmErrorMsg errnum, char *buf, size_t size);
int32_t MmScandir(const std::string &path, MmDirent ***entryList, MmFilter filterFunc, MmSort sort);
void MmScandirFree(MmDirent **entryList, int32_t count);
int32_t MmRmdir(const std::string &pathName);
int32_t MmUnlink(const std::string &filename);
int32_t MmRealPath(const char *path, char *realPath, int32_t realPathLen);
int32_t MmChdir(const std::string &path);
int32_t MmCreateProcess(const std::string &fileName,
                        const MmArgvEnv *env,
                        const std::string &stdoutRedirectFile,
                        MmProcess *id);
int32_t MmWaitPid(MmProcess pid, int32_t *status, int32_t options);
int32_t MmGetMac(MmMacInfo **list, int32_t *count);
int32_t MmGetMacFree(MmMacInfo *list, int32_t count);
int32_t MmGetEnv(const std::string &name, char *value, uint32_t len);
int32_t MmGetCwd(char *buffer, int32_t maxLen);
int32_t MmGetLocalTime(MmSystemTimeT *sysTimePtr);
int32_t MmGetPid();
int32_t MmGetUid();
int32_t MmGetTid();
int32_t MmStatGet(const std::string &path, MmStatT *buffer);
int32_t MmClose(int32_t fd);
int32_t MmGetOptLong(int32_t argc,
                     char *const *argv,
                     const char *opts,
                     const MmStructOption *longOpts,
                     int32_t *longIndex);
int32_t MmGetOptInd();
char *MmGetOptArg();
int32_t MmGetTimeOfDay(MmTimeval *timeVal, MmTimezone *timeZone);
int32_t MmOpen2(const std::string &pathName, int32_t flags, MmMode_t mode);
ssize_t MmWrite(int32_t fd, void *buf, uint32_t bufLen);
int32_t MmGetOsVersion(char *versionInfo, int32_t versionLength);
int32_t MmGetOsName(char *name, int32_t nameSize);
int32_t MmGetCpuInfo(MmCpuDesc **cpuInfo, int32_t *count);
int32_t MmCpuInfoFree(MmCpuDesc *cpuInfo, int32_t count);
ssize_t MmRead(int32_t fd, void *buf, uint32_t bufLen);
ssize_t MmSocketSend(MmSockHandle sockFd, void *sendBuf, int32_t sendLen, int32_t sendFlag);
} // Mmpa
} // Dvvp
} // Collector
#endif