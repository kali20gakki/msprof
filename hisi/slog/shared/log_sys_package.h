/**
 * @log_sys_package.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef LOG_SYS_PACKAGE_H
#define LOG_SYS_PACKAGE_H

#ifndef WIN
#define WIN 1
#endif

#ifndef LINUX
#define LINUX 0
#endif

#ifndef __IDE_UT
#define STATIC static
#else
#define STATIC
#endif

#ifndef OS_TYPE_DEF
#define OS_TYPE_DEF LINUX
#endif

#if (OS_TYPE_DEF == LINUX)

#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <net/if.h>
#include <dirent.h>
#include <signal.h>
#include <limits.h>
#include <syslog.h>
#include <arpa/inet.h>
#include <string.h>
#include <time.h>

#include <sys/ipc.h>
#include <sys/inotify.h>
#include <sys/klog.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/un.h>

#include <pwd.h>
#include <stdlib.h>
#include "securec.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cpluscplus */
#endif /* __cpluscplus */

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

typedef void* ArgPtr;
typedef void Buff;
typedef void* (*ThreadFunc)(ArgPtr);

typedef unsigned char UINT8;
typedef unsigned int UINT32;
typedef signed int INT32;
typedef unsigned long long UINT64;
typedef void VOID;
typedef char CHAR;
typedef long LONG;

#define TOOL_MAX_THREAD_PIO 99
#define TOOL_MIN_THREAD_PIO 1
#define TOOL_MAX_SLEEP_MILLSECOND 4294967
#define TOOL_ONE_THOUSAND 1000
#define TOOL_COMPUTER_BEGIN_YEAR 1900
#define TOOL_THREADNAME_SIZE 16

#define TOOL_THREAD_SCHED_RR SCHED_RR
#define TOOL_THREAD_SCHED_FIFO SCHED_FIFO
#define TOOL_THREAD_SCHED_OTHER SCHED_OTHER
#define TOOL_THREAD_MIN_STACK_SIZE PTHREAD_STACK_MIN

#define TOOL_MAX_PATH PATH_MAX

#define SYS_OK 0
#define SYS_ERR 1
#define EOK 0
#define SYS_ERROR (-1)
#define SYS_INVALID_PARAM (-2)

#ifdef __ANDROID__
#define S_IREAD S_IRUSR
#define S_IWRITE S_IWUSR
#endif

#define M_FILE_RDONLY O_RDONLY
#define M_FILE_WRONLY O_WRONLY
#define M_FILE_RDWR O_RDWR
#define M_FILE_CREAT O_CREAT

#define M_RDONLY O_RDONLY
#define M_WRONLY O_WRONLY
#define M_RDWR O_RDWR
#define M_CREAT O_CREAT

#define M_IREAD S_IREAD
#define M_IRUSR S_IRUSR
#define M_IWRITE S_IWRITE
#define M_IWUSR S_IWUSR
#define M_IXUSR S_IXUSR
#define FDSIZE 64
#define M_MSG_CREAT IPC_CREAT
#define M_MSG_EXCL (IPC_CREAT | IPC_EXCL)
#define M_MSG_NOWAIT IPC_NOWAIT

// system
typedef pthread_t toolThread;
typedef pthread_mutex_t toolMutex;
typedef struct {
    INT32 detachFlag;
    INT32 priorityFlag;
    INT32 priority;
    INT32 policyFlag;
    INT32 policy;
    INT32 stackFlag;
    UINT32 stackSize;
} ToolThreadAttr;

typedef int toolKey;
typedef int toolMsgid;

typedef signed int toolProcess;
typedef mode_t toolMode;
typedef struct stat ToolStat;

typedef struct dirent ToolDirent;
typedef int (*toolFilter)(const ToolDirent *entry);
typedef int (*toolSort)(const ToolDirent **a, const ToolDirent **b);

typedef VOID *(*toolProcFunc)(VOID *pulArg);

typedef struct {
    toolProcFunc procFunc;
    VOID *pulArg;
} ToolUserBlock;

typedef int toolSockHandle;
typedef struct sockaddr ToolSockAddr;
typedef socklen_t toolSocklen;

typedef struct {
    LONG tv_sec;
    LONG tv_usec;
} ToolTimeval;

typedef struct {
    INT32 tz_minuteswest;
    INT32 tz_dsttime;
} ToolTimezone;

// multi thread interface
INT32 ToolMutexInit(toolMutex *mutex);
INT32 ToolMutexLock(toolMutex *mutex);
INT32 ToolMutexUnLock(toolMutex *mutex);
INT32 ToolMutexDestroy(toolMutex *mutex);
INT32 ToolCreateTaskWithThreadAttr(toolThread *threadHandle, const ToolUserBlock *funcBlock,
                                   const ToolThreadAttr *threadAttr);
INT32 ToolCreateTaskWithDetach(toolThread *threadHandle, const ToolUserBlock *funcBlock);

// message queue interface
toolMsgid ToolMsgCreate(toolKey key, INT32 msgFlag);
toolMsgid ToolMsgOpen(toolKey key, INT32 msgFlag);
INT32 ToolMsgSnd(toolMsgid msqid, const VOID *buf, INT32 bufLen, INT32 msgFlag);
INT32 ToolMsgRcv(toolMsgid msqid, VOID *buf, INT32 bufLen, INT32 msgFlag, LONG msgType);
INT32 ToolMsgClose(toolMsgid msqid);

// I/O interface
INT32 ToolOpen(const CHAR *pathName, INT32 flags);
INT32 ToolOpenWithMode(const CHAR *pathName, INT32 flags, toolMode mode);
INT32 ToolClose(INT32 fd);
INT32 ToolWrite(INT32 fd, const VOID *buf, UINT32 bufLen);
INT32 ToolRead(INT32 fd, VOID *buf, UINT32 bufLen);
INT32 ToolMkdir(const CHAR *pathName, toolMode mode);
INT32 ToolAccess(const CHAR *pathName);
INT32 ToolAccessWithMode(const CHAR *pathName, INT32 mode);
INT32 ToolRealPath(const CHAR *path, CHAR *realPath, INT32 realPathLen);
INT32 ToolUnlink(const CHAR *filename);
INT32 ToolChmod(const CHAR *filename, INT32 mode);
INT32 ToolChown(const char *filename, uid_t owner, gid_t group);
INT32 ToolScandir(const CHAR *path, ToolDirent ***entryList, toolFilter filterFunc, toolSort sort);
VOID ToolScandirFree(ToolDirent **entryList, INT32 count);
INT32 ToolFtruncate(toolProcess fd, UINT32 length);
INT32 ToolStatGet(const CHAR *path,  ToolStat *buffer);
INT32 ToolFsync(toolProcess fd);
INT32 ToolFileno(FILE *stream);

// socket interface
toolSockHandle ToolSocket(INT32 sockFamily, INT32 type, INT32 protocol);
INT32 ToolBind(toolSockHandle sockFd, const ToolSockAddr *addr, toolSocklen addrLen);
toolSockHandle ToolAccept(toolSockHandle sockFd, ToolSockAddr *addr, toolSocklen *addrLen);
INT32 ToolConnect(toolSockHandle sockFd, const ToolSockAddr *addr, toolSocklen addrLen);
INT32 ToolCloseSocket(toolSockHandle sockFd);
INT32 ToolSAStartup(void);
INT32 ToolSACleanup(void);

// shared memory interface
INT32 ToolShmGet(key_t key, size_t size, INT32 shmflg);
VOID *ToolShMat(INT32 shmid, const VOID *shmaddr, INT32 shmflg);
INT32 ToolShmDt(const VOID *shmaddr);
INT32 ToolShmCtl(INT32 shmid, INT32 cmd, struct shmid_ds *buf);

// others
INT32 ToolGetPid(void);
INT32 ToolSleep(UINT32 milliSecond);
VOID ToolMemBarrier(void);
INT32 ToolGetErrorCode(void);
INT32 ToolGetTimeOfDay(ToolTimeval *timeVal, ToolTimezone *timeZone);
INT32 ToolLocalTimeR(const time_t *timep, struct tm *result);
INT32 ToolGetOpt(INT32 argc, char * const *argv, const char *opts);
INT32 ToolJoinTask(const toolThread *threadHandle);

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cpluscplus */
#endif /* __cpluscplus */

#else

#include "mmpa_api.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cpluscplus */
#endif /* __cpluscplus */

#define SYS_OK 0
#define SYS_ERR 1
#define SYS_ERROR (-1)
#define SYS_INVALID_PARAM (-2)

#define TOOL_MAX_PATH MAX_PATH

#ifndef EOK
#define EOK 0
#endif // !EOK

typedef void* ArgPtr;
typedef void Buff;
typedef void* (*ThreadFunc)(ArgPtr);

typedef HANDLE toolMutex;
typedef HANDLE toolThread;
typedef HANDLE toolProcess;

typedef VOID *(*toolProcFunc)(VOID *pulArg);
typedef struct {
    toolProcFunc procFunc;
    VOID *pulArg;
} ToolUserBlock;

typedef DWORD toolThreadKey;

typedef SOCKET toolSockHandle;
typedef struct sockaddr ToolSockAddr;
typedef int toolSocklen;
typedef int toolKey;
typedef HANDLE toolMsgid;

typedef struct {
    unsigned char d_type;
    char d_name[MAX_PATH]; // file name
} ToolDirent;

typedef int(*toolFilter)(const ToolDirent *entry);
typedef int(*toolSort)(const ToolDirent **a, const ToolDirent **b);

typedef struct {
    LONG  tv_sec;
    LONG  tv_usec;
} ToolTimeval;

typedef struct {
    INT32 tz_minuteswest;
    INT32 tz_dsttime;
} ToolTimezone;

typedef struct {
    LONG  tv_sec;
    LONG  tv_nsec;
} ToolTimespec;

typedef struct stat ToolStat;
typedef int toolMode;

// Windows only support detachFlag
typedef struct {
    INT32 detachFlag;
    INT32 priorityFlag;
    INT32 priority;
    INT32 policyFlag;
    INT32 policy;
    INT32 stackFlag;
    UINT32 stackSize;
} ToolThreadAttr;

// multi thread interface
INT32 ToolMutexInit (toolMutex *mutex);
INT32 ToolMutexLock(toolMutex *mutex);
INT32 ToolMutexUnLock(toolMutex *mutex);
INT32 ToolMutexDestroy(toolMutex *mutex);
INT32 ToolCreateTaskWithThreadAttr(toolThread *threadHandle, const ToolUserBlock *funcBlock,
                                   const ToolThreadAttr *threadAttr);
INT32 ToolCreateTaskWithDetach(toolThread *threadHandle, const ToolUserBlock *funcBlock);

// message queue interface
toolMsgid ToolMsgCreate(toolKey key, INT32 msgFlag);
toolMsgid ToolMsgOpen(toolKey key, INT32 msgFlag);
INT32 ToolMsgSnd(toolMsgid msqid, const VOID *buf, INT32 bufLen, INT32 msgFlag);
INT32 ToolMsgRcv(toolMsgid msqid, VOID *buf, INT32 bufLen, INT32 msgFlag, LONG msgType);
INT32 ToolMsgClose(toolMsgid msqid);

// I/O interface
INT32 ToolOpen(const CHAR *pathName, INT32 flags);
INT32 ToolOpenWithMode(const CHAR *pathName, INT32 flags, toolMode mode);
INT32 ToolClose(INT32 fd);
INT32 ToolWrite(INT32 fd, const VOID *buf, UINT32 bufLen);
INT32 ToolRead(INT32 fd, VOID *buf, UINT32 bufLen);
INT32 ToolMkdir(const CHAR *pathName, toolMode mode);
INT32 ToolAccess(const CHAR *pathName);
INT32 ToolAccessWithMode(const CHAR *pathName, INT32 mode);
INT32 ToolRealPath(const CHAR *path, CHAR *realPath, INT32 realPathLen);
INT32 ToolUnlink(const CHAR *filename);
INT32 ToolChmod(const CHAR *filename, INT32 mode);
INT32 ToolScandir(const CHAR *path, ToolDirent ***entryList, toolFilter filterFunc, toolSort sort);
VOID ToolScandirFree(ToolDirent **entryList, INT32 count);
INT32 ToolFtruncate(toolProcess fd, UINT32 length);
INT32 ToolStatGet(const CHAR *path,  ToolStat *buffer);
INT32 ToolFsync(toolProcess fd);
INT32 ToolFileno(FILE *stream);

// socket interface
toolSockHandle ToolSocket(INT32 sockFamily, INT32 type, INT32 protocol);
INT32 ToolBind(toolSockHandle sockFd, const ToolSockAddr *addr, toolSocklen addrLen);
toolSockHandle ToolAccept(toolSockHandle sockFd, ToolSockAddr *addr, toolSocklen *addrLen);
INT32 ToolConnect(toolSockHandle sockFd, const ToolSockAddr *addr, toolSocklen addrLen);
INT32 ToolCloseSocket(toolSockHandle sockFd);
INT32 ToolSAStartup(void);
INT32 ToolSACleanup(void);

// others
INT32 ToolGetPid(void);
INT32 ToolSleep(UINT32 milliSecond);
VOID ToolMemBarrier(void);
INT32 ToolGetErrorCode(void);
INT32 ToolGetTimeOfDay(ToolTimeval *timeVal, ToolTimezone *timeZone);
INT32 ToolLocalTimeR(const time_t *timep, struct tm *result);
INT32 ToolGetOpt(INT32 argc, char * const *argv, const char *opts);
INT32 ToolJoinTask(const toolThread *tid);

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cpluscplus */
#endif /* __cpluscplus */

#endif /* OS_TYPE_DEF */

#endif
