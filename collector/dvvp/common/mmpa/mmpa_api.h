/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: Linux interface wrapper
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2022-06-09
 */
#ifndef MMPA_API_H
#define MMPA_API_H

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <pthread.h>
#include <syslog.h>
#include <dirent.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <net/if.h>
#include <stdarg.h>
#include <limits.h>
#include <ctype.h>
#include <stddef.h>
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
#define MMPA_MAX_THREAD_PIO 99
#define MMPA_MIN_THREAD_PIO 1
#define PATH_SIZE 256
#define MMPA_MAX_PATH PATH_MAX
#define M_WAIT_NOHANG WNOHANG
#define M_WAIT_UNTRACED WUNTRACED
#define MMPA_MAX_IF_SIZE 2048
#define MMPA_MEM_MAX_LEN (0x7fffffff)
#define MMPA_COMPUTER_BEGIN_YEAR 1900
#define MMPA_MIN_OS_VERSION_SIZE 128
#define MMPA_MIN_OS_NAME_SIZE 64
#define MMPA_CPUPROC_BUF_SIZE 256
#define MMPA_CPUINFO_DEFAULT_SIZE 64
#define MMPA_CPUINFO_DOUBLE_SIZE 128
#define MMPA_MAX_PHYSICALCPU_COUNT 4096
#define MMPA_MIN_PHYSICALCPU_COUNT 1

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
#define mm_no_argument        no_argument
#define mm_required_argument  required_argument
#define mm_optional_argument  optional_argument

#define FREE_BUF(buf)                \
    do {                             \
        free(buf);                   \
        buf = nullptr;               \
    } while(0)                       \

namespace Collector {
namespace Dvvp {
namespace Mmpa {
// data type define
const int MMPA_MACINFO_DEFAULT_SIZE = 18;
const int MMPA_CPUDESC_DEFAULT_SIZE = 64;
const int MMPA_MAX_SLEEP_MILLSECOND_USING_USLEEP = 1000;
const int MMPA_MSEC_TO_USEC = 1000;
const int MMPA_MAX_SLEEP_MICROSECOND_USING_USLEEP = 1000000;
const int FALSE = 0;
const int TRUE = 1;

enum MMPA_MAC_ADDR_TYPE{
    MMPA_MAC_ADDR_FIRST_BYTE = 0,
    MMPA_MAC_ADDR_SECOND_BYTE,
    MMPA_MAC_ADDR_THIRD_BYTE,
    MMPA_MAC_ADDR_FOURTH_BYTE,
    MMPA_MAC_ADDR_FIFTH_BYTE,
    MMPA_MAC_ADDR_SIXTH_BYTE
};

typedef pthread_t mmThread;
typedef pthread_mutex_t mmMutex_t;
typedef struct dirent mmDirent;
typedef signed int mmProcess;
typedef mode_t mmMode_t;
typedef struct option mmStructOption;
typedef struct stat mmStat_t;
typedef int mmErrorMsg;
typedef int mmSockHandle;
// function ptr define
typedef void *(*userProcFunc)(void *pulArg);
typedef int (*mmFilter)(const mmDirent *entry);
typedef int (*mmSort)(const mmDirent **a, const mmDirent **b);

// struct define
typedef struct {
    userProcFunc procFunc;  // Callback function pointer
    void *pulArg;           // Callback function parameters
} mmUserBlock_t;

struct CpuTypeTable {
    const char *key;
    const char *value;
};

typedef struct {
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
} mmThreadAttr;

typedef struct {
    unsigned long long totalSize;
    unsigned long long freeSize;
    unsigned long long availSize;
} mmDiskSize;

typedef struct {
    char **argv;
    int32_t argvCount;
    char **envp;
    int32_t envpCount;
} mmArgvEnv;

typedef struct {
    char addr[MMPA_MACINFO_DEFAULT_SIZE];  // ex:aa-bb-cc-dd-ee-ff\0
} mmMacInfo;

typedef struct {
    char arch[MMPA_CPUDESC_DEFAULT_SIZE];
    char manufacturer[MMPA_CPUDESC_DEFAULT_SIZE];    // vendor
    char version[MMPA_CPUDESC_DEFAULT_SIZE];         // modelname
    int32_t frequency;                               // cpu frequency
    int32_t maxFrequency;                            // max speed
    int32_t ncores;                                  // cpu cores
    int32_t nthreads;                                // cpu thread count
    int32_t ncounts;                                 // logical cpu nums
} mmCpuDesc;

typedef struct {
    int32_t wSecond;             // Seconds. [0-60] (1 leap second)
    int32_t wMinute;             // Minutes. [0-59]
    int32_t wHour;               // Hours. [0-23]
    int32_t wDay;                // Day. [1-31]
    int32_t wMonth;              // Month. [1-12]
    int32_t wYear;               // Year
    int32_t wDayOfWeek;          // Day of week. [0-6]
    int32_t tm_yday;             // Days in year.[0-365]
    int32_t tm_isdst;            // DST. [-1/0/1]
    long wMilliseconds;          // milliseconds
} mmSystemTime_t;

typedef struct {
    long tv_sec;
    long tv_usec;
} mmTimeval;

typedef struct {
    int32_t tz_minuteswest;  // How many minutes is it different from Greenwich
    int32_t tz_dsttime;      // type of DST correction
} mmTimezone;

typedef struct {
  long long tv_sec;
  long long tv_nsec;
} mmTimespec;

// function define
int32_t MmSleep(uint32_t milliSecond);
int32_t MmCreateTaskWithThreadAttr(mmThread *threadHandle, const mmUserBlock_t *funcBlock,
    const mmThreadAttr *threadAttr);
int32_t MmJoinTask(mmThread *threadHandle);
int32_t MmSetCurrentThreadName(const std::string &name);
mmTimespec MmGetTickCount();
int32_t MmGetFileSize(const std::string &fileName, unsigned long long *length);
int32_t MmGetDiskFreeSpace(const std::string &path, mmDiskSize *diskSize);
int32_t MmIsDir(const std::string &fileName);
int32_t MmAccess2(const std::string &pathName, int32_t mode);
int32_t MmAccess(const std::string &pathName);
char *MmDirName(char *path);
char *MmBaseName(char *path);
int32_t MmMkdir(const std::string &pathName, mmMode_t mode);
int32_t MmChmod(const std::string &filename, int32_t mode);
int32_t MmGetErrorCode();
char *MmGetErrorFormatMessage(int errnum, char *buf, size_t size);
int32_t MmScandir(const std::string &path, mmDirent ***entryList, mmFilter filterFunc, mmSort sort);
void MmScandirFree(mmDirent **entryList, int32_t count);
int32_t MmRmdir(const std::string &pathName);
int32_t MmUnlink(const std::string &filename);
int32_t MmRealPath(const char *path, char *realPath, int32_t realPathLen);
int32_t MmChdir(const std::string &path);
int32_t MmCreateProcess(const std::string &fileName,
                        const mmArgvEnv *env,
                        const std::string &stdoutRedirectFile,
                        mmProcess *id);
int32_t MmWaitPid(mmProcess pid, int32_t *status, int32_t options);
int32_t MmGetMac(mmMacInfo **list, int32_t *count);
int32_t MmGetMacFree(mmMacInfo *list, int32_t count);
int32_t MmGetEnv(const std::string &name, char *value, uint32_t len);
int32_t MmGetCwd(char *buffer, int32_t maxLen);
int32_t MmGetLocalTime(mmSystemTime_t *sysTimePtr);
int32_t MmGetPid();
int32_t MmGetTid();
int32_t MmStatGet(const std::string &path, mmStat_t *buffer);
int32_t MmClose(int32_t fd);
int32_t MmGetOptLong(int32_t argc,
                     char *const *argv,
                     const char *opts,
                     const mmStructOption *longOpts,
                     int32_t *longIndex);
int32_t MmGetOptInd();
char *MmGetOptArg();
int32_t MmGetTimeOfDay(mmTimeval *timeVal, mmTimezone *timeZone);
int32_t MmOpen2(const std::string &pathName, int32_t flags, mmMode_t mode);
ssize_t MmWrite(int32_t fd, void *buf, uint32_t bufLen);
int32_t MmGetOsVersion(char *versionInfo, int32_t versionLength);
int32_t MmGetOsName(char *name, int32_t nameSize);
int32_t MmGetCpuInfo(mmCpuDesc **cpuInfo, int32_t *count);
int32_t MmCpuInfoFree(mmCpuDesc *cpuInfo, int32_t count);
ssize_t MmRead(int32_t fd, void *buf, uint32_t bufLen);
ssize_t MmSocketSend(mmSockHandle sockFd, void *sendBuf, int32_t sendLen, int32_t sendFlag);
int32_t mmMutexLock(mmMutex_t *mutex);
int32_t mmMutexUnLock(mmMutex_t *mutex);
} // Mmpa
} // Dvvp
} // Collector
#endif