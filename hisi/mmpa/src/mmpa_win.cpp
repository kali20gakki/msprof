/*
 * @file mmpa_win.cpp
 *
 * Copyright (C) Huawei Technologies Co., Ltd. 2018-2019. All Rights Reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Description:MMPA库 Windows部分接口实现
 * Author:Huawei Technologies
 * Create:2017-12-22
 */
#include "mmpa_api.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cpluscplus */
#endif /* __cpluscplus */

const INT32 MMPA_MAX_BUF_SIZE = 4096;
const INT32 MMPA_MAX_PHYSICALCPU_COUNT = 64;
const INT32 MMPA_MAX_PROCESSOR_ARCHITECTURE_COUNT = 10;
const INT32 MMPA_SECOND_TO_MSEC = 1000;
const INT32 MMPA_MSEC_TO_USEC = 1000;
const INT32 MMPA_MSEC_TO_NSEC = 1000000;
const INT32 MMPA_MAX_WAIT_TIME = 100;
const INT32 MMPA_MIN_WAIT_TIME = 0;

static INT32 opterr = 1;    // 与linux的全局环境变量命名保持一致
static INT32 optind = 1;    // 与linux的全局环境变量命名保持一致
static INT32 optopt = '?';  // 与linux的全局环境变量命名保持一致
static CHAR *optarg = nullptr; // 与linux的全局环境变量命名保持一致
static INT32 optreset;      // 与linux的全局环境变量命名保持一致

static INT32 g_nonoptStart = -1;
static INT32 g_nonoptEnd = -1;

// 错误消息 与linux保持一致
static const CHAR *OPT_REQ_ARG_CHAR = "option requires an argument -- %c";
static const CHAR *OPT_REQ_ARG_STRING = "option requires an argument -- %s";
static const CHAR *OPT_AMBIGIOUS = "OPT_AMBIGIOUSuous option -- %.*s";
static const CHAR *OPT_NO_ARG = "option doesn't take an argument -- %.*s";
static const CHAR *OPT_ILLEGAL_CHAR = "unknown option -- %c";
static const CHAR *OPT_ILLEGAL_STRING = "unknown option -- %s";

static CHAR *g_place = const_cast<CHAR *>(MMPA_EMSG);

#define ERRA(s, c) do { \
    if (MMPA_PRINT_ERROR) { \
        (void)fprintf(stderr, s, c); \
    } \
} while (0)

#define ERRB(s, c, v) do { \
    if (MMPA_PRINT_ERROR) { \
        (void)fprintf(stderr, s, c, v); \
    } \
} while (0)

#define MMPA_FREE(var) \
    do { \
        if (var != nullptr) { \
            free(var); \
            var = nullptr; \
        } \
    } \
    while (0)

#define MMPA_CLOSE_FILE_HANDLE(var) \
    do { \
        if (var != nullptr) { \
            (void)CloseHandle(var); \
            var = nullptr; \
        } \
    } \
    while (0)

typedef enum {
    MMPA_OS_VERSION_WINDOWS_2000 = 0,
    MMPA_OS_VERSION_WINDOWS_XP,
    MMPA_OS_VERSION_WINDOWS_XP_SP1,
    MMPA_OS_VERSION_WINDOWS_XP_SP2,
    MMPA_OS_VERSION_WINDOWS_SERVER_2003,
    MMPA_OS_VERSION_WINDOWS_HOME_SERVER,
    MMPA_OS_VERSION_WINDOWS_VISTA,
    MMPA_OS_VERSION_WINDOWS_SERVER_2008,
    MMPA_OS_VERSION_WINDOWS_SERVER_2008_R2,
    MMPA_OS_VERSION_WINDOWS_7,
    MMPA_OS_VERSION_WINDOWS_SERVER_2012,
    MMPA_OS_VERSION_WINDOWS_8,
    MMPA_OS_VERSION_WINDOWS_SERVER_2012_R2,
    MMPA_OS_VERSION_WINDOWS_8_1,
    MMPA_OS_VERSION_WINDOWS_SERVER_2016,
    MMPA_OS_VERSION_WINDOWS_10
}MMPA_OS_VERSION_WINDOWS_TYPE;

// windows 操作系统列表
CHAR g_winOps[][MMPA_MIN_OS_VERSION_SIZE] = {
    "Windows 2000",
    "Windows XP",
    "Windows XP SP1",
    "Windows XP SP2",
    "Windows Server 2003",
    "Windows Home Server",
    "Windows Vista",
    "Windows Server 2008",
    "Windows Server 2008 R2",
    "Windows 7",
    "Windows Server 2012",
    "Windows 8",
    "Windows Server 2012 R2",
    "Windows 8.1",
    "Windows Server 2016",
    "Windows 10"
};

// 操作系统架构表
CHAR g_arch[MMPA_MAX_PROCESSOR_ARCHITECTURE_COUNT][MMPA_MIN_OS_VERSION_SIZE] = {
    "x86",  // 0
    "MIPS", // 1
    "Alpha ",
    "PowerPC",
    "",
    "ARM",  // 5
    "ia64", // 6
    "",
    "",
    "x64",  // 9
};

static INT32 CheckSizetAddOverflow(size_t a, size_t b)
{
    if ((b > 0) && (a > (SIZE_MAX - b))) {
        return EN_ERROR;
    }
    return EN_OK;
}

/*
 * 描述:根据创建线程时候的函数名和参数执行相关功能,内部使用
 * 参数:pstArg --包含函数名和参数的结构体指针
 * 返回值:执行成功或者失败都返回EN_OK
 */
static DWORD WINAPI LocalThreadProc(LPVOID pstArg)
{
    mmUserBlock_t *pstTemp = reinterpret_cast<mmUserBlock_t *>(pstArg);
    pstTemp->procFunc(pstTemp->pulArg);
    return EN_OK;
}

/*
 * 描述:用默认的属性创建线程
 * 参数:threadHandle -- pthread_t类型的实例
 *       funcBlock --包含函数名和参数的结构体指针
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmCreateTask(mmThread *threadHandle, mmUserBlock_t *funcBlock)
{
    if ((threadHandle == nullptr) || (funcBlock  == nullptr) || (funcBlock->procFunc  == nullptr)) {
        return EN_INVALID_PARAM;
    }

    DWORD threadId;
    *threadHandle = CreateThread(nullptr, MMPA_ZERO, LocalThreadProc, (LPVOID)funcBlock, MMPA_ZERO, &threadId);
    if (*threadHandle == nullptr) {
        return EN_ERROR;
    }
    (void)WaitForSingleObject(*threadHandle, MMPA_MAX_WAIT_TIME); // 等待时间为100ms，确保funcBlock->procFunc线程体被执行
    return EN_OK;
}

/*
 * 描述:该函数等待线程结束并返回线程退出值"value_ptr"
 *       如果线程成功结束会detach线程
 *       注意:detached的线程不能用该函数或取消
 * 参数:threadHandle-- pthread_t类型的实例
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmJoinTask(mmThread *threadHandle)
{
    if (threadHandle == nullptr) {
        return EN_INVALID_PARAM;
    }

    INT32 ret = EN_OK;
    DWORD dwWaitResult = WaitForSingleObject(*threadHandle, INFINITE);
    if (dwWaitResult == WAIT_ABANDONED) {
        ret = EN_ERROR;
    } else if (dwWaitResult == WAIT_TIMEOUT) {
        ret = EN_ERROR;
    } else if (dwWaitResult == WAIT_FAILED) {
        ret = EN_ERROR;
    }
    return ret;
}

/*
 * 描述:用默认属性初始化锁
 * 参数:mutex --指向mmMutex_t 指针
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmMutexInit(mmMutex_t *mutex)
{
    if (mutex == nullptr) {
        return EN_INVALID_PARAM;
    }

    INT32 ret = EN_OK;
    HANDLE handleMutex = CreateMutex(nullptr, FALSE, nullptr);
    if (handleMutex == nullptr) {
        ret = EN_ERROR;
    } else {
        *mutex = handleMutex;
    }
    return ret;
}

/*
 * 描述:把mutex锁住
 * 参数:mutex --指向mmMutex_t 指针
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmMutexLock(mmMutex_t *mutex)
{
    if (mutex == nullptr) {
        return EN_INVALID_PARAM;
    }
    INT32 ret = EN_OK;
    DWORD dwWaitResult = WaitForSingleObject(*mutex, INFINITE);
    if (dwWaitResult == WAIT_ABANDONED) {
        ret = EN_ERROR;
    } else if (dwWaitResult == WAIT_FAILED) {
        ret = EN_ERROR;
    }
    return ret;
}

/*
 * 描述:把mutex锁住，非阻塞
 * 参数: mutex --指向mmMutex_t 指针
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmMutexTryLock(mmMutex_t *mutex)
{
    if (mutex == nullptr) {
        return EN_INVALID_PARAM;
    }
    INT32 ret = EN_OK;
    DWORD dwWaitResult = WaitForSingleObject(*mutex, MMPA_MIN_WAIT_TIME);
    if (dwWaitResult == WAIT_ABANDONED || dwWaitResult == WAIT_FAILED) {
        ret = EN_ERROR;
    }
    return ret;
}

/*
 * 描述:把mutex锁解锁
 * 参数:mutex--指向mmMutex_t 指针
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmMutexUnLock(mmMutex_t *mutex)
{
    if (mutex == nullptr) {
        return EN_INVALID_PARAM;
    }

    INT32 ret = EN_OK;
    BOOL releaseMutex = ReleaseMutex(*mutex);
    if (!releaseMutex) {
        ret = EN_ERROR;
    }
    return ret;
}

/*
 * 描述:把mutex锁删除
 * 参数:mutex--指向mmMutex_t 指针
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmMutexDestroy(mmMutex_t *mutex)
{
    if (mutex == nullptr) {
        return EN_INVALID_PARAM;
    }

    INT32 ret = EN_OK;
    BOOL cHandle = CloseHandle(*mutex);
    if (!cHandle) {
        ret = EN_ERROR;
    }
    return ret;
}

/*
 * 描述:初始化条件变量，条件变量属性为nullptr
 * 输入 :cond -- 指向mmCond的指针
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmCondInit(mmCond *cond)
{
    if (cond == nullptr) {
        return EN_INVALID_PARAM;
    }

    InitializeConditionVariable(cond);
    return EN_OK;
}

/*
 * 描述:用默认属性初始化锁
 * 参数:cond -- 指向mmMutexFC指针
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmCondLockInit(mmMutexFC *mutex)
{
    if (mutex == nullptr) {
        return EN_INVALID_PARAM;
    }

    InitializeCriticalSection(mutex);
    return EN_OK;
}

/*
 * 描述:把mutex锁住
 * 参数:cond--指向mmMutexFC 指针
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmCondLock(mmMutexFC *mutex)
{
    if (mutex == nullptr) {
        return EN_INVALID_PARAM;
    }

    EnterCriticalSection(mutex);
    return EN_OK;
}

/*
 * 描述:把mutex解锁
 * 参数:cond --指向mmMutexFC 指针
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmCondUnLock(mmMutexFC *mutex)
{
    if (mutex == nullptr) {
        return EN_INVALID_PARAM;
    }

    LeaveCriticalSection(mutex);
    return EN_OK;
}

 /*
 * 描述:销毁mmMutexFC
 * 参数:cond--指向mmMutexFC 指针
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmCondLockDestroy(mmMutexFC *mutex)
{
    if (mutex == nullptr) {
        return EN_INVALID_PARAM;
    }

    DeleteCriticalSection(mutex);
    return EN_OK;
}

/*
 * 描述:用默认属性初始化读写锁
 * 参数: rwLock --指向mmRWLock_t的指针
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmRWLockInit(mmRWLock_t *rwLock)
{
    if (rwLock == nullptr) {
        return EN_INVALID_PARAM;
    }

    InitializeSRWLock(rwLock);
    return EN_OK;
}

/*
 * 描述:把rwlock锁住,应用于读锁
 * 参数: rwLock --指向mmRWLock_t的指针
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmRWLockRDLock(mmRWLock_t *rwLock)
{
    if (rwLock == NULL) {
        return EN_INVALID_PARAM;
    }

    AcquireSRWLockShared(rwLock);
    return EN_OK;
}

/*
 * 描述:把rwlock锁住,应用于读锁（非阻塞）
 * 参数: rwLock --指向mmRWLock_t的指针
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmRWLockTryRDLock(mmRWLock_t *rwLock)
{
    if (rwLock == NULL) {
        return EN_INVALID_PARAM;
    }

    BOOLEAN ret = TryAcquireSRWLockShared(rwLock);
    if (!ret) {
        return EN_ERROR;
    }

    return EN_OK;
}

/*
 * 描述:把rwlock锁住,应用于写锁（阻塞）
 * 参数: rwLock --指向mmRWLock_t的指针
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmRWLockWRLock(mmRWLock_t *rwLock)
{
    if (rwLock == NULL) {
        return EN_INVALID_PARAM;
    }

    AcquireSRWLockExclusive(rwLock);
    return EN_OK;
}

/*
 * 描述:把rwlock锁住,应用于写锁（非阻塞）
 * 参数: rwLock --指向mmRWLock_t的指针
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmRWLockTryWRLock(mmRWLock_t *rwLock)
{
    if (rwLock == NULL) {
        return EN_INVALID_PARAM;
    }

    BOOLEAN ret = TryAcquireSRWLockExclusive(rwLock);
    if (!ret) {
        return EN_ERROR;
    }

    return EN_OK;
}

/*
 * 描述:把rwlock解锁,应用于读锁
 * 参数: rwLock --指向mmRWLock_t的指针
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARA
 */
INT32 mmRDLockUnLock(mmRWLock_t *rwLock)
{
    if (rwLock == NULL) {
        return EN_INVALID_PARAM;
    }

    ReleaseSRWLockShared(rwLock);
    return EN_OK;
}

/*
 * 描述:把rwlock解锁,应用于写锁
 * 参数: rwLock --指向mmRWLock_t的指针
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARA
 */
INT32 mmWRLockUnLock(mmRWLock_t *rwLock)
{
    if (rwLock == NULL) {
        return EN_INVALID_PARAM;
    }

    ReleaseSRWLockExclusive(rwLock);
    return EN_OK;
}

/*
 * 描述:把rwlock删除，该接口windows为空实现，系统回自动清理
 * 参数: rwLock --指向mmRWLock_t的指针
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmRWLockDestroy(mmRWLock_t *rwLock)
{
    return EN_OK;
}

/*
 * 描述:用于阻塞当前线程
 * 参数:cond --  mmCond指针
 *       mutex -- mmMutexFC指针, 条件变量
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmCondWait(mmCond *cond, mmMutexFC *mutex)
{
    if ((cond == nullptr) || (mutex == nullptr)) {
        return EN_INVALID_PARAM;
    }

    INT32 ret = EN_OK;
    BOOL sCondition = SleepConditionVariableCS(cond, mutex, INFINITE);
    if (!sCondition) {
        ret = EN_ERROR;
    }
    return ret;
}

/*
 * 描述:用于阻塞当前线程一定时间，由外部指定时间
 * 参数:cond --  mmCond指针
 *       mutex -- mmMutexFC指针, 条件变量
 *       mmMilliSeconds -- 阻塞时间
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR、超时返回EN_TIMEOUT, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmCondTimedWait(mmCond *cond, mmMutexFC *mutex, UINT32 milliSecond)
{
    if ((cond == nullptr) || (mutex == nullptr)) {
        return EN_INVALID_PARAM;
    }

    INT32 ret = EN_OK;
    BOOL sleepCondition = SleepConditionVariableCS(cond, mutex, (DWORD)milliSecond);
    if (!sleepCondition) {
        if (GetLastError() == WAIT_TIMEOUT) {
            return EN_TIMEOUT;
        }
        ret = EN_ERROR;
    }
    return ret;
}

/*
 * 描述:唤醒mmCondWait阻塞的线程, 只给一个线程发信号
 * 参数:cond -- mmCond指针
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmCondNotify(mmCond *cond)
{
    if (cond == nullptr) {
        return EN_INVALID_PARAM;
    }

    WakeConditionVariable(cond);
    return EN_OK;
}

/*
 * 描述:唤醒mmCondWait阻塞的线程, 可以给多个线程发信号
 * 参数:cond -- mmCond指针
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmCondNotifyAll(mmCond *cond)
{
    if (cond == nullptr) {
        return EN_INVALID_PARAM;
    }

    WakeAllConditionVariable(cond);
    return EN_OK;
}

/*
 * 描述:销毁cond指向的条件变量
 * 参数:cond -- mmCond指针
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32  mmCondDestroy(mmCond *cond)
{
    if (cond == nullptr) {
        return EN_INVALID_PARAM;
    }

    return EN_OK;
}

/*
 * 描述:获取本地时间
 * 参数:sysTime -- 指向mmSystemTime_t 结构的指针
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmGetLocalTime(mmSystemTime_t *sysTimePtr)
{
    if (sysTimePtr == nullptr) {
        return EN_INVALID_PARAM;
    } else {
        GetLocalTime(sysTimePtr);
    }
    return EN_OK;
}

/*
 * 描述:获取系统时间
 * 参数:sysTime -- 指向mmSystemTime_t 结构的指针
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmGetSystemTime(mmSystemTime_t *sysTimePtr)
{
    if (sysTimePtr == nullptr) {
        return EN_INVALID_PARAM;
    } else {
        GetSystemTime(sysTimePtr);
    }
    return EN_OK;
}

/*
 * 描述:获取进程ID
 * 返回值:执行成功返回对应调用进程的id, 执行错误返回EN_ERROR
 */
INT32 mmGetPid(void)
{
    return static_cast<INT32>(GetCurrentProcessId());
}

/*
 * 描述:获取调用线程的线程ID
 * 返回值:执行成功返回对应调用线程的id, 执行错误返回EN_ERROR
 */
INT32 mmGetTid(void)
{
    return static_cast<INT32>(GetCurrentThreadId());
}

/*
 * 描述:获取进程ID
 * 参数:pstProcessHandle -- 存放进程ID的指向mmProcess类型的指针
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmGetPidHandle(mmProcess *processHandle)
{
    if (processHandle == nullptr) {
        return EN_INVALID_PARAM;
    }
    *processHandle = GetCurrentProcess();
    return EN_OK;
}

/*
 * 描述:初始化一个信号量
 * 参数:sem--指向mmSem_t的指针
 *       value--信号量的初始值
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmSemInit(mmSem_t *sem, UINT32 value)
{
    if (sem == nullptr || (value > (MMPA_MEM_MAX_LEN - MMPA_ONE_THOUSAND * MMPA_ONE_THOUSAND))) {
        return EN_INVALID_PARAM;
    }
    *sem = CreateSemaphore(nullptr, static_cast<LONG>(value),
        static_cast<LONG>(value) + MMPA_ONE_THOUSAND * MMPA_ONE_THOUSAND, nullptr);
    if (*sem == nullptr) {
        return EN_ERROR;
    }
    return EN_OK;
}

/*
 * 描述:用来阻塞当前线程直到信号量sem的值大于0，解除阻塞后将sem的值减一
 * 参数:sem--指向mmSem_t的指针
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmSemWait(mmSem_t *sem)
{
    if (sem == nullptr) {
        return EN_INVALID_PARAM;
    }

    DWORD result = WaitForSingleObject(*sem, INFINITE);
    if (result == WAIT_ABANDONED || result == WAIT_FAILED) {
        return EN_ERROR;
    }
    return EN_OK;
}

/*
 * 描述:用来增加信号量的值
 * 参数:sem--指向mmSem_t的指针
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmSemPost(mmSem_t *sem)
{
    if (sem == nullptr) {
        return EN_INVALID_PARAM;
    }

    BOOL result = ReleaseSemaphore(*sem, static_cast<LONG>(MMPA_VALUE_ONE), nullptr);
    if (!result) {
        return EN_ERROR;
    }
    return EN_OK;
}

/*
 * 描述:用完信号量对它进行清理
 * 参数:sem--指向mmSem_t的指针
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmSemDestroy(mmSem_t *sem)
{
    if (sem == nullptr) {
        return EN_INVALID_PARAM;
    }

    BOOL result = CloseHandle(*sem);
    if (!result) {
        return EN_ERROR;
    }
    return EN_OK;
}

/*
 * 描述:打开或者创建一个文件
 * 参数:pathName--需要打开或者创建的文件路径名，由用户确保绝对路径
 *       flags--打开或者创建的文件标志位, 默认 user和group的权限
 * 返回值:执行成功返回对应打开的文件描述符, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmOpen(const CHAR *pathName, INT32 flags)
{
    if (pathName == nullptr) {
        return EN_INVALID_PARAM;
    }
    UINT32 tmpFlag = static_cast<UINT32>(flags);

    if ((tmpFlag & (_O_TRUNC | _O_WRONLY | _O_RDWR | _O_CREAT | _O_BINARY)) == MMPA_ZERO && flags != _O_RDONLY) {
        return EN_INVALID_PARAM;
    }

    INT32 fd = _open(pathName, flags, _S_IREAD | _S_IWRITE); // mode默认为 _S_IREAD | _S_IWRITE
    if (fd < MMPA_ZERO) {
        return EN_ERROR;
    }
    return fd;
}

/*
 * 描述:打开或者创建一个文件
 * 参数:pathName--需要打开或者创建的文件路径名，由用户确保绝对路径
 *       flags--打开或者创建的文件标志位
 *       mode -- 打开或者创建的权限
 * 返回值:执行成功返回对应打开的文件描述符, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmOpen2(const CHAR *pathName, INT32 flags, MODE mode)
{
    if (pathName == nullptr) {
        return EN_INVALID_PARAM;
    }
    UINT32 tmpFlag = static_cast<UINT32>(flags);
    UINT32 tmpMode = static_cast<UINT32>(mode);

    if ((tmpFlag & (_O_TRUNC | _O_WRONLY | _O_RDWR | _O_CREAT | _O_BINARY)) == MMPA_ZERO && flags != _O_RDONLY) {
        return EN_INVALID_PARAM;
    }
    if (((tmpMode & _S_IREAD) == MMPA_ZERO) && ((tmpMode & _S_IWRITE) == MMPA_ZERO)) {
        return EN_INVALID_PARAM;
    }

    INT32 winFd = _open(pathName, flags, mode);
    if (winFd < MMPA_ZERO) {
        return EN_ERROR;
    }
    return winFd;
}

/*
* 描述：调用fork（）产生子进程，然后从子进程中调用shell来执行参数command的指令。
* 参数： command--参数是一个指向以 NULL 结束的 shell 命令字符串的指针。这行命令将被传到 bin/sh 并使用-c 标志，shell 将执行这个命令
*        type--参数只能是读或者写中的一种，得到的返回值（标准 I/O 流）也具有和 type 相应的只读或只写类型。
* 返回值：若成功则返回文件指针，否则返回NULL, 错误原因存于mmGetErrorCode中
*/
FILE *mmPopen(CHAR *command, CHAR *type)
{
    if ((command == NULL) || (type == NULL)) {
        return NULL;
    }
    FILE *stream = _popen(command, type);
    return stream;
}

/*
 * 描述:关闭打开的文件
 * 参数:fd--指向打开文件的资源描述符
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM， 错误原因存于mmGetErrorCode中
 */
INT32 mmClose(INT32 fd)
{
    if (fd < MMPA_ZERO) {
        return EN_INVALID_PARAM;
    }

    INT32 result = _close(fd);
    if (result != EN_OK) {
        return EN_ERROR;
    }
    return EN_OK;
}

/*
* 描述：pclose（）用来关闭由popen所建立的管道及文件指针。
* 参数: stream--为先前由popen（）所返回的文件指针
* 返回值：执行成功返回cmdstring的终止状态，若出错则返回EN_ERROR，入参检查错误返回EN_INVALID_PARAM
*/
INT32 mmPclose(FILE *stream)
{
    if (stream == NULL) {
        return EN_INVALID_PARAM;
    }
    return _pclose(stream);
}

/*
 * 描述:写数据到一个资源文件中
 * 参数:fd--指向打开文件的资源描述符
 *       buf--需要写入的数据
 *       bufLen--需要写入的数据长度
 * 返回值:执行成功返回写入的长度, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
mmSsize_t mmWrite(INT32 fd, VOID *buf, UINT32 bufLen)
{
    if (fd < MMPA_ZERO || buf == nullptr || bufLen == MMPA_ZERO) {
        return EN_INVALID_PARAM;
    }

    mmSsize_t result = _write(fd, buf, bufLen);
    if (result < MMPA_ZERO) {
        return EN_ERROR;
    }
    return result;
}

/*
 * 描述:从资源文件中读取数据
 * 参数:fd--指向打开文件的资源描述符
 *       buf--存放读取的数据，由用户分配缓存
 *       bufLen--需要读取的数据大小
 * 返回值:执行成功返回读取的长度, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
mmSsize_t mmRead(INT32 fd, VOID *buf, UINT32 bufLen)
{
    if (fd < MMPA_ZERO || buf == nullptr || bufLen == MMPA_ZERO) {
        return EN_INVALID_PARAM;
    }

    mmSsize_t result = _read(fd, buf, bufLen);
    if (result < MMPA_ZERO) {
        return EN_ERROR;
    }
    return result;
}

/*
 * 描述:创建socket
 * 参数:sockFamily--协议域
 *       type--指定socket类型
 *       protocol--指定协议
 * 返回值:执行成功返回创建的socket id, 执行错误返回EN_ERROR
 */
mmSockHandle mmSocket(INT32 sockFamily, INT32 type, INT32 protocol)
{
    SOCKET socketHandle = socket(sockFamily, type, protocol);
    if (socketHandle == INVALID_SOCKET) {
        return EN_ERROR;
    }
    return socketHandle;
}

/*
 * 描述:把一个地址族中的特定地址赋给socket
 * 参数:sockFd--socket描述字，通过mmSocket函数创建
 *       addr--一个指向要绑定给sockFd的协议地址
 *       addrLen--对应地址的长度
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmBind(mmSockHandle sockFd, mmSockAddr *addr, mmSocklen_t addrLen)
{
    if ((sockFd == INVALID_SOCKET) || (addr == nullptr) || (addrLen <= MMPA_ZERO)) {
        return EN_INVALID_PARAM;
    }

    INT32 result = bind(sockFd, addr, addrLen);
    if (result != EN_OK) {
        return EN_ERROR;
    }
    return EN_OK;
}

/*
 * 描述:监听socket
 * 参数:sockFd--socket描述字，通过mmSocket函数创建
 *       backLog--相应socket可以排队的最大连接个数
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmListen(mmSockHandle sockFd, INT32 backLog)
{
    if ((sockFd == INVALID_SOCKET) || (backLog <= MMPA_ZERO)) {
        return EN_INVALID_PARAM;
    }

    INT32 result = listen(sockFd, backLog);
    if (result != EN_OK) {
        return EN_ERROR;
    }
    return EN_OK;
}

/*
 * 描述:监听指定的socket地址
 * 参数:sockFd--socket描述字，通过mmSocket函数创建
 *       addr--用于返回客户端的协议地址
 *       addrLen--协议地址的长度
 * 返回值:执行成功返回自动生成的一个全新的socket id, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
mmSockHandle mmAccept(mmSockHandle sockFd, mmSockAddr *addr, mmSocklen_t *addrLen)
{
    if (sockFd == INVALID_SOCKET) {
        return EN_INVALID_PARAM;
    }

    SOCKET socketHandle = accept(sockFd, addr, addrLen);
    if (socketHandle == INVALID_SOCKET) {
        return EN_ERROR;
    }
    return socketHandle;
}

/*
 * 描述:发出socket连接请求
 * 参数:sockFd--socket描述字，通过mmSocket函数创建
 *      addr--服务器的socket地址
 *      addrLen--地址的长度
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmConnect(mmSockHandle sockFd, mmSockAddr *addr, mmSocklen_t addrLen)
{
    if ((sockFd == INVALID_SOCKET) || (addr == nullptr) || (addrLen <= MMPA_ZERO)) {
        return EN_INVALID_PARAM;
    }

    INT32 result = connect(sockFd, addr, addrLen);
    if (result < MMPA_ZERO) {
        return EN_ERROR;
    }
    return EN_OK;
}

/*
 * 描述:在建立连接的socket上发送数据
 * 参数:sockFd--已建立连接的socket描述字
 *       pstSendBuf--需要发送的数据缓存，有用户分配
 *       sendLen--需要发送的数据长度
 *       sendFlag--发送的方式标志位，一般置0
 * 返回值:执行成功返回实际发送的buf长度, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
mmSsize_t mmSocketSend(mmSockHandle sockFd, VOID *sendBuf, INT32 sendLen, INT32 sendFlag)
{
    if ((sockFd == INVALID_SOCKET) || (sendBuf == nullptr) || (sendLen <= MMPA_ZERO) || (sendFlag < MMPA_ZERO)) {
        return EN_INVALID_PARAM;
    }

    mmSsize_t sendRet = send(sockFd, reinterpret_cast<CHAR *>(sendBuf), sendLen, sendFlag);
    if (sendRet == SOCKET_ERROR) {
        return EN_ERROR;
    }
    return sendRet;
}

/*
* 描述：适用于发送未建立连接的UDP数据包 （参数为SOCK_DGRAM）
* 参数： sockFd--socket描述字
*       sendMsg--需要发送的数据缓存，用户分配
*       sendLen--需要发送的数据长度
*       sendFlag--发送的方式标志位，一般置0
*       addr--指向目的套接字的地址
*       tolen--addr所指地址的长度
* 返回值：执行成功返回实际发送的buf长度, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
*/
INT32 mmSocketSendTo(mmSockHandle sockFd,
                     VOID *sendMsg,
                     INT32 sendLen,
                     UINT32 sendFlag,
                     const mmSockAddr* addr,
                     INT32 tolen)
{
    if ((sockFd < MMPA_ZERO) || (sendMsg == NULL) || (sendLen <= MMPA_ZERO) || (addr == NULL) || (tolen <= MMPA_ZERO)) {
        return EN_INVALID_PARAM;
    }
    INT32 flag = static_cast<INT32>(sendFlag);
    INT32 ret = sendto(sockFd, reinterpret_cast<CHAR *>(sendMsg), sendLen, flag, addr, tolen);
    if (ret < MMPA_ZERO) {
        return EN_ERROR;
    }
    return ret;
}

/*
 * 描述:在建立连接的socket上接收数据
 * 参数:sockFd--已建立连接的socket描述字
 *       pstRecvBuf--存放接收的数据的缓存，用户分配
 *       recvLen--需要发送的数据长度
 *       recvFlag--接收的方式标志位，一般置0
 * 返回值:执行成功返回实际接收的buf长度, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
mmSsize_t mmSocketRecv(mmSockHandle sockFd, VOID *recvBuf, INT32 recvLen, INT32 recvFlag)
{
    if ((sockFd == INVALID_SOCKET) || (recvBuf == nullptr) || (recvLen <= MMPA_ZERO) || (recvFlag < MMPA_ZERO)) {
        return EN_INVALID_PARAM;
    }

    mmSsize_t recvRet = recv(sockFd, reinterpret_cast<CHAR *>(recvBuf), recvLen, recvFlag);
    if (recvRet == SOCKET_ERROR) {
        return EN_ERROR;
    }
    return recvRet;
}

/*
* 描述：在建立连接的socket上接收数据
* 参数： sockFd--已建立连接的socket描述字
*       recvBuf--存放接收的数据的缓存，用户分配
*       recvLen--需要接收的数据长度
*       recvFlag--发送的方式标志位，一般置0
*       addr--指向指定欲传送的套接字的地址
*       tolen--addr所指地址的长度
* 返回值：执行成功返回实际发送的buf长度, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
*/
mmSsize_t mmSocketRecvFrom(mmSockHandle sockFd,
                           VOID *recvBuf,
                           mmSize recvLen,
                           UINT32 recvFlag,
                           mmSockAddr* addr,
                           mmSocklen_t *FromLen)
{
    if ((sockFd < MMPA_ZERO) || (recvBuf == NULL) || (recvLen <= MMPA_ZERO) || (addr == NULL) || (FromLen == NULL)) {
        return EN_INVALID_PARAM;
    }
    INT32 flag = static_cast<INT32>(recvFlag);
    mmSsize_t ret = recvfrom(sockFd, reinterpret_cast<CHAR *>(recvBuf), recvLen, flag, addr, FromLen);
    if (ret < MMPA_ZERO) {
        return EN_ERROR;
    }
    return ret;
}

/*
 * 描述:关闭建立的socket连接
 * 参数:sockFd--已打开或者建立连接的socket描述字
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmCloseSocket(mmSockHandle sockFd)
{
    if (sockFd == INVALID_SOCKET) {
        return EN_INVALID_PARAM;
    }

    INT32 result = closesocket(sockFd);
    if (result != EN_OK) {
        return EN_ERROR;
    }
    return EN_OK;
}

/*
 * 描述:初始化winsockDLL, windows中有效, linux为空实现
 * 参数:空
 * 返回值:执行成功返回EN_OK，执行失败返回EN_ERROR
 */
INT32 mmSAStartup(VOID)
{
    WSADATA data;
    WORD wVersionRequested = MAKEWORD(MMPA_SOCKET_MAIN_EDITION, MMPA_SOCKET_SECOND_EDITION);
    INT32 result = WSAStartup(wVersionRequested, &data);
    if (result != EN_OK) {
        return EN_ERROR;
    }
    return EN_OK;
}

/*
 * 描述:注销winsockDLL, windows中有效, linux为空实现
 * 参数:空
 * 返回值:执行成功返回EN_OK，执行失败返回EN_ERROR
 */
INT32 mmSACleanup(VOID)
{
    INT32 result = WSACleanup();
    if (result != EN_OK) {
        return EN_ERROR;
    }
    return EN_OK;
}

/*
 * 描述:加载一个动态库中的符号
 * 参数:fileName--动态库文件名
 *      mode--打开方式
 * 返回值:执行成功返回动态链接库的句柄, 执行错误返回nullptr, 入参检查错误返回nullptr
 */
VOID *mmDlopen(const CHAR *fileName, INT32 mode)
{
    if ((fileName == nullptr) || (mode < MMPA_ZERO)) {
        return nullptr;
    }

    return LoadLibrary(static_cast<LPCTSTR>(fileName));
}

/*
* 描述：取有关最近定义给定addr 的符号的信息
* 参数： addr--指定加载模块的的某一地址
*       info--是指向mmDlInfo 结构的指针。由用户分配
* 返回值：执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
*/
INT32 mmDladdr(VOID *addr, mmDlInfo *info)
{
    __declspec(thread) static TCHAR szPath[MAX_PATH];
    HMODULE hModule = nullptr;
    if ((addr == nullptr) || (info == nullptr)) {
        return EN_INVALID_PARAM;
    }

    BOOL bret = GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCSTR)addr, &hModule);
    if (!bret) {
        return EN_ERROR;
    }

    DWORD ret = GetModuleFileName(hModule, szPath, MAX_PATH);
    if (ret == MMPA_ZERO) {
        return EN_ERROR;
    }
    info->dli_fname = szPath;
    return EN_OK;
}

/*
 * 描述:获取mmDlopen打开的动态库中的指定符号地址
 * 参数:handle--mmDlopen 返回的指向动态链接库指针
 *       funcName--要求获取的函数的名称
 * 返回值:执行成功返回指向函数的地址, 执行错误返回nullptr, 入参检查错误返回nullptr
 */
VOID *mmDlsym(VOID *handle, const CHAR *funcName)
{
    if ((handle == nullptr) || (funcName == nullptr)) {
        return nullptr;
    }

    return GetProcAddress((HMODULE)handle, (LPCSTR)funcName);
}

/*
 * 描述:关闭mmDlopen加载的动态库
 * 参数:handle--mmDlopen 返回的指向动态链接库指针
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmDlclose(VOID *handle)
{
    if (handle == nullptr) {
        return EN_INVALID_PARAM;
    }

    BOOL result = FreeLibrary((HMODULE)handle);
    if (!result) {
        return EN_ERROR;
    }
    return EN_OK;
}

/*
 * 描述:当mmDlopen动态链接库操作函数执行失败时，mmDlerror可以返回出错信息
 * 参数:空
 * 返回值:执行成功返回nullptr
 */
CHAR *mmDlerror(void)
{
    return nullptr;
}

/*
 * 描述:mmCreateAndSetTimer定时器的回调函数, 仅在内部使用
 * 参数:空
 * 返回值:空
 */
VOID CALLBACK mmTimerCallBack(_In_ PVOID lpParameter, _In_ BOOLEAN TimerOrWaitFired)
{
    mmUserBlock_t *pstTemp = reinterpret_cast<mmUserBlock_t *>(lpParameter);

    if ((pstTemp != nullptr) && (pstTemp->procFunc != nullptr)) {
        pstTemp->procFunc(pstTemp->pulArg);
    }
    return;
}

/*
 * 描述:创建特定的定时器
 * 参数:timerHandle--被创建的定时器的ID
 *       milliSecond:定时器首次timer到期的时间 单位ms
 *       period:定时器循环的周期值 单位ms
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmCreateAndSetTimer(mmTimer *timerHandle, mmUserBlock_t *timerBlock, UINT milliSecond, UINT period)
{
    if ((timerHandle == nullptr) || (timerBlock == nullptr) || (timerBlock->procFunc == nullptr)) {
        return EN_INVALID_PARAM;
    }

    timerHandle->timerQueue = CreateTimerQueue();
    if (timerHandle->timerQueue == nullptr) {
        return EN_ERROR;
    }

    BOOL ret = CreateTimerQueueTimer(&(timerHandle->timerHandle), timerHandle->timerQueue,
        mmTimerCallBack, timerBlock, milliSecond, period, WT_EXECUTEDEFAULT);
    if (!ret) {
        (void)DeleteTimerQueue(timerHandle->timerQueue);
        return EN_ERROR;
    }
    return EN_OK;
}

/*
 * 描述:删除mmCreateAndSetTimer创建的定时器
 * 参数:timerHandle--对应的timer id
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR
 */
INT32 mmDeleteTimer(mmTimer timerHandle)
{
    BOOL ret = DeleteTimerQueueTimer(timerHandle.timerQueue, timerHandle.timerHandle, nullptr);
    if (!ret) {
        return EN_ERROR;
    }

    ret = DeleteTimerQueue(timerHandle.timerQueue);
    if (!ret) {
        return EN_ERROR;
    }

    return EN_OK;
}

/*
 * 描述:获取文件状态
 * 参数:path--需要获取的文件路径名
 *       buffer--获取到的状态 由用户分配缓存
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmStatGet(const CHAR *path, mmStat_t *buffer)
{
    if (path == nullptr || buffer == nullptr) {
        return EN_INVALID_PARAM;
    }

    INT32 ret = stat(path, buffer);
    if (ret == EN_ERROR) {
        return EN_ERROR;
    }
    return EN_OK;
}

/*
 * 描述:获取文件状态(文件size大于2G使用)
 * 参数:path--需要获取的文件路径名
 *       buffer--获取到的状态 由用户分配缓存
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmStat64Get(const CHAR *path, mmStat64_t *buffer)
{
    if (path == nullptr || buffer == nullptr) {
        return EN_INVALID_PARAM;
    }

    INT32 ret = _stat64(path, buffer);
    if (ret == EN_ERROR) {
        return EN_ERROR;
    }
    return EN_OK;
}

/*
 * 描述:获取文件状态
 * 参数: fd--需要获取的文件描述符
 *       buffer--获取到的状态 由用户分配缓存
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmFStatGet(INT32 fd, mmStat_t *buffer)
{
    if (buffer == nullptr) {
        return EN_INVALID_PARAM;
    }

    INT32 ret = _fstat(fd, reinterpret_cast<struct _stat64i32 *>(buffer));
    if (ret == EN_ERROR) {
        return EN_ERROR;
    }
    return EN_OK;
}

/*
 * 描述:创建一个目录
 * 参数:pathName -- 需要创建的目录路径名, 比如 CHAR dirName[256]="/home/test";
 *       mode -- 新目录的权限
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmMkdir(const CHAR *pathName, mmMode_t mode)
{
    if (pathName == nullptr) {
        return EN_INVALID_PARAM;
    }

    BOOL ret = CreateDirectory((LPCSTR)pathName, nullptr);
    if (!ret) {
        return EN_ERROR;
    }
    return EN_OK;
}

/*
 * 描述:睡眠指定时间
 * 参数:milliSecond -- 睡眠时间 单位ms
 * 返回值:执行成功返回EN_OK, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmSleep(UINT32 milliSecond)
{
    if (milliSecond == MMPA_ZERO) {
        return EN_INVALID_PARAM;
    }

    Sleep((DWORD)milliSecond);
    return EN_OK;
}

/*
 * 描述:用默认的属性创建线程，属性linux下有效
 * 参数:threadHandle -- pthread_t类型的实例
 *       funcBlock --包含函数名和参数的结构体指针
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmCreateTaskWithAttr(mmThread *threadHandle, mmUserBlock_t *funcBlock)
{
    return mmCreateTask(threadHandle, funcBlock);
}

/*
 * 描述:获取指定进程优先级id
 * 参数:pid--进程ID
 * 返回值:执行成功返回优先级id, 执行错误返回MMPA_PROCESS_ERROR, 入参检查错误返回MMPA_PROCESS_ERROR
 */
INT32 mmGetProcessPrio(mmProcess pid)
{
    if (pid == nullptr) {
        return MMPA_PROCESS_ERROR ;
    }

    INT32 priority;
    INT32 ret = GetPriorityClass(pid);
    if (ret == MMPA_ZERO) {
        return MMPA_PROCESS_ERROR;
    }
    switch (ret) {
        case REALTIME_PRIORITY_CLASS:   // those three process priority in windows
        case HIGH_PRIORITY_CLASS:       // compare to MMPA_MIN_NI in linux
        case ABOVE_NORMAL_PRIORITY_CLASS:
            priority = MMPA_MIN_NI;
            break;
        case NORMAL_PRIORITY_CLASS:
            priority = MMPA_LOW_NI;
            break;
        case BELOW_NORMAL_PRIORITY_CLASS: // those two process priority in windows
        case IDLE_PRIORITY_CLASS:         // compare to MMPA_MAX_NI in linux
            priority = MMPA_MAX_NI;
            break;
        default:
            priority = MMPA_PROCESS_ERROR;
    }
    return priority;
}

/*
 * 描述:设置进程优先级
 * 参数:pid--进程ID
 *       processPrio:需要设置的优先级值，参数范围取 -20 to 19
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmSetProcessPrio(mmProcess pid, INT32 processPrio)
{
    if (pid == nullptr) {
        return EN_INVALID_PARAM;
    }

    DWORD priority;
    if (processPrio >= MMPA_MIDDLE_NI) {
        priority = BELOW_NORMAL_PRIORITY_CLASS;
    } else if (processPrio < MMPA_MIDDLE_NI && processPrio >= MMPA_LOW_NI) {
        priority = NORMAL_PRIORITY_CLASS;
    } else {
        priority = ABOVE_NORMAL_PRIORITY_CLASS;
    }

    INT32 ret = SetPriorityClass(pid, priority);
    if (ret == MMPA_ZERO) {
        return EN_ERROR;
    } else {
        return EN_OK;
    }
}

/*
 * 描述:获取线程优先级
 * 参数:threadHandle--线程ID
 * 返回值:执行成功返回获取到的优先级, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmGetThreadPrio(mmThread *threadHandle)
{
    if (threadHandle == nullptr) {
        return EN_INVALID_PARAM;
    }

    INT32 priority;
    INT32 ret = GetThreadPriority(*threadHandle);
    if (ret == THREAD_PRIORITY_ERROR_RETURN) {
        return EN_ERROR;
    }
    switch (ret) {
        case THREAD_PRIORITY_TIME_CRITICAL: // those three thread priority in windows
        case THREAD_PRIORITY_HIGHEST:       // compare to  MMPA_MAX_THREAD_PIO in linux
        case THREAD_PRIORITY_ABOVE_NORMAL:
            priority = MMPA_MAX_THREAD_PIO;
            break;
        case THREAD_PRIORITY_NORMAL:
            priority = MMPA_LOW_THREAD_PIO;
            break;
        case THREAD_PRIORITY_BELOW_NORMAL: // those three thread priority in windows
        case THREAD_PRIORITY_LOWEST:       // compare to  MMPA_MIN_THREAD_PIO in linux
        case THREAD_PRIORITY_IDLE:
            priority = MMPA_MIN_THREAD_PIO;
            break;
        default:
            return EN_ERROR;
    }
    return priority;
}

/*
 * 描述:设置线程优先级, Linux默认以SCHED_RR策略设置
 * 参数:threadHandle--线程ID
 *       threadPrio:设置的优先级id，参数范围取1 - 99, 1为最高优先级
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmSetThreadPrio(mmThread *threadHandle, INT32 threadPrio)
{
    if (threadHandle == nullptr) {
        return EN_INVALID_PARAM;
    }

    INT32 priority;
    if (threadPrio <= MMPA_MAX_THREAD_PIO && threadPrio >= MMPA_MIDDLE_THREAD_PIO) {
        priority = THREAD_PRIORITY_ABOVE_NORMAL;
    } else if (threadPrio < MMPA_MIDDLE_THREAD_PIO && threadPrio >= MMPA_LOW_THREAD_PIO) {
        priority = THREAD_PRIORITY_NORMAL;
    } else if (threadPrio < MMPA_LOW_THREAD_PIO && threadPrio >= MMPA_MIN_THREAD_PIO) {
        priority = THREAD_PRIORITY_BELOW_NORMAL;
    } else {
        return EN_INVALID_PARAM;
    }

    INT32 ret = SetThreadPriority(*threadHandle, priority);
    if (ret == MMPA_ZERO) {
        return EN_ERROR;
    } else {
        return EN_OK;
    }
}

/*
 * 描述:判断文件或者目录是否存在
 * 参数:pathName -- 文件路径名
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmAccess(const CHAR *pathName)
{
    if (pathName == nullptr) {
        return EN_INVALID_PARAM;
    }
    DWORD ret = GetFileAttributes((LPCSTR)pathName);
    if (ret == INVALID_FILE_ATTRIBUTES) {
        return EN_ERROR;
    }
    return EN_OK;
}

INT32 mmAccess2(const CHAR *pathName, INT32 mode)
{
    return mmAccess(pathName);
}

/*
 * 描述:删除目录下所有文件及目录, 包括子目录
 * 参数:pathName -- 目录名全路径
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmRmdir(const CHAR *pathName)
{
    DWORD dRet;
    if (pathName == nullptr) {
        return EN_INVALID_PARAM;
    }
    CHAR szCurPath[MAX_PATH] = {0};
    INT32 ret = _snprintf_s(szCurPath, MAX_PATH - 1, "%hs\\*.*", pathName);
    if (ret < MMPA_ZERO) {
        return EN_INVALID_PARAM;
    }
    WIN32_FIND_DATA FindFileData;
    SecureZeroMemory(&FindFileData, sizeof(WIN32_FIND_DATAA));
    HANDLE hFile = FindFirstFile((LPCSTR)szCurPath, &FindFileData);
    BOOL FOUND = FALSE;
    do {
        FOUND = FindNextFile(hFile, &FindFileData);
        if (strcmp(reinterpret_cast<CHAR *>(FindFileData.cFileName), ".") == MMPA_ZERO ||
            strcmp(reinterpret_cast<CHAR *>(FindFileData.cFileName), "..") == MMPA_ZERO) {
            continue;
        }
        CHAR buf[MAX_PATH];
        ret = _snprintf_s(buf, MAX_PATH - 1, "%s\\%s", pathName, FindFileData.cFileName);
        if (ret < MMPA_ZERO) {
            (void)FindClose(hFile);
            return EN_INVALID_PARAM;
        }
        dRet = GetFileAttributes(buf); // whether dir or file,
        if (dRet == FILE_ATTRIBUTE_DIRECTORY) {
            dRet = mmRmdir(buf);
            if (dRet != EN_OK) {
                (void)FindClose(hFile);
                return EN_ERROR;
            }
        } else {
            if (!DeleteFile(buf)) {
                (void)FindClose(hFile);
                return EN_ERROR;
            }
        }
    } while (FOUND);
    (void)FindClose(hFile);

    BOOL bRet = RemoveDirectory((LPCSTR)pathName);
    if (!bRet) {
        return EN_ERROR;
    }
    return EN_OK;
}

/*
 * 描述:对设备的I/O通道进行管理
 * 参数:fd--指向设备驱动文件的文件资源描述符
 *       ioctlCode--ioctl的操作码
 *       bufPtr--指向数据的缓存, 里面包含的输入输出buf缓存由用户分配
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmIoctl(mmProcess fd, INT32 ioctlCode, mmIoctlBuf *bufPtr)
{
    if ((fd < MMPA_ZERO) || bufPtr == nullptr || bufPtr->inbuf == nullptr || bufPtr->outbuf == nullptr) {
        return EN_INVALID_PARAM;
    }
    DWORD bytesReturned = 0;
    DWORD result = DeviceIoControl(fd, static_cast<DWORD>(ioctlCode), static_cast<LPVOID>(bufPtr->inbuf),
                                   static_cast<DWORD>(bufPtr->inbufLen), static_cast<LPVOID>(bufPtr->outbuf),
                                   static_cast<DWORD>(bufPtr->outbufLen), &bytesReturned, bufPtr->oa);
    if (result == MMPA_ZERO) {
        return EN_ERROR;
    } else {
        return EN_OK;
    }
}

/*
 * 描述:用来阻塞当前线程直到信号量sem的值大于0，解除阻塞后将sem的值减一, 同时指定最长阻塞时间
 * 参数:sem--指向mmSem_t的指针
 *       timeout--超时时间 单位ms
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmSemTimedWait(mmSem_t *sem, INT32 timeout)
{
    if (sem == nullptr || timeout < MMPA_ZERO) {
        return EN_INVALID_PARAM;
    }

    DWORD result = WaitForSingleObject(*sem, (DWORD)timeout);
    if (result == WAIT_ABANDONED || result == WAIT_FAILED) {
        return EN_ERROR;
    }
    return EN_OK;
}

/*
 * 描述:用于在一次函数调用中写多个非连续缓冲区
 * 参数:fd -- 打开的资源描述符
 *       iov -- 指向非连续缓存区的首地址的指针
 *       iovcnt -- 非连续缓冲区的个数, 最大支持MAX_IOVEC_SIZE片非连续缓冲区
 * 返回值:执行成功返回实际写入的长度, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
mmSsize_t mmWritev(mmSockHandle fd, mmIovSegment *iov, INT32 iovcnt)
{
    if (fd < MMPA_ZERO || iovcnt < MMPA_ZERO || iov == nullptr || iovcnt > MAX_IOVEC_SIZE) {
        return EN_INVALID_PARAM;
    }

    INT32 i = 0;
    for (i = 0; i < iovcnt; i++) {
        INT32 sendRet = send(fd, reinterpret_cast<CHAR *>(iov[i].sendBuf), iov[i].sendLen, MSG_DONTROUTE);
        if (sendRet == SOCKET_ERROR) {
            return EN_ERROR;
        }
    }
    return EN_OK;
}

/*
 * 描述:创建一个内存屏障
 */
VOID mmMb()
{
    MemoryBarrier();
}

/*
 * 描述:一个字符串IP地址转换为一个32位的网络序列IP地址
 * 参数:addrStr -- 待转换的字符串IP地址
 *       addr -- 转换后的网络序列IP地址
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmInetAton(const CHAR *addrStr, mmInAddr *addr)
{
    if (addrStr == nullptr || addr == nullptr) {
        return EN_INVALID_PARAM;
    }
    DWORD result = inet_pton(AF_INET, (PCSTR)addrStr, addr);
    if (result == MMPA_ZERO) {
        return EN_INVALID_PARAM;
    } else if (result == 1) { // inet_pton没有错误返回1
        return EN_OK;
    } else {
        return EN_ERROR;
    }
}

/*
 * 描述:打开文件或者设备驱动
 * 参数:文件路径名fileName, 打开的权限access, 是否新创建标志位fileFlag
 * 返回值:执行成功返回对应打开的文件描述符，执行错误返回EN_ERROR, 入参检查错误返回EN_ERROR
 */
mmProcess mmOpenFile(const CHAR *fileName, UINT32 accessFlag, mmCreateFlag fileFlag)
{
    if (fileName == nullptr) {
        return (mmProcess)EN_ERROR;
    }

    if ((accessFlag & (GENERIC_READ | GENERIC_WRITE)) == MMPA_ZERO) {
        return (mmProcess)EN_ERROR;
    }
    DWORD dwCreationDisposition;
    if (fileFlag.createFlag == OPEN_ALWAYS) {
        dwCreationDisposition = OPEN_ALWAYS; // if no file , create file
    } else {
        dwCreationDisposition = OPEN_EXISTING; // if no file , return false
    }
    DWORD dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL;
    if (fileFlag.oaFlag != MMPA_ZERO) {
        dwFlagsAndAttributes |= FILE_FLAG_OVERLAPPED;
    }

    mmProcess fd = CreateFile((LPCSTR)fileName, accessFlag,
        FILE_SHARE_READ | FILE_SHARE_WRITE, // 默认文件对于多进程都是共享读写的
        nullptr, (DWORD)dwCreationDisposition, dwFlagsAndAttributes, nullptr);
    if (fd == INVALID_HANDLE_VALUE) {
        return (mmProcess)EN_ERROR;
    }
    return fd;
}

/*
 * 描述:关闭打开的文件或者设备驱动
 * 参数:打开的文件描述符fileId
 * 返回值:执行成功返回EN_OK，执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmCloseFile(mmProcess fileId)
{
    if (fileId < MMPA_ZERO) {
        return EN_INVALID_PARAM;
    }

    BOOL result = CloseHandle(fileId);
    if (!result) {
        return EN_ERROR;
    }
    return EN_OK;
}

/*
 * 描述:往打开的文件或者设备驱动写内容
 * 参数:打开的文件描述符fileId，buffer为用户分配缓存，len为buffer对应长度
 * 返回值:执行成功返回写入的字节数，执行错误返回EN_ERROR，入参检查错误返回EN_INVALID_PARAM
 */
mmSsize_t mmWriteFile(mmProcess fileId, VOID *buffer, INT32 len)
{
    DWORD dwWritten = MMPA_ZERO;
    if ((fileId < MMPA_ZERO) || (buffer == nullptr) || len < MMPA_ZERO) {
        return EN_INVALID_PARAM;
    }

    INT32 ret = WriteFile(fileId, buffer, len, &dwWritten, nullptr);
    if (ret < MMPA_ZERO) {
        return EN_ERROR;
    }
    return (INT32)dwWritten;
}

/*
 * 描述:往打开的文件或者设备驱动读取内容
 * 参数:打开的文件描述符fileId，buffer为用户分配缓存，len为buffer对应长度
 * 返回值:执行成功返回实际读取的长度，执行错误返回EN_ERROR，入参检查错误返回EN_INVALID_PARAM
 */
mmSsize_t mmReadFile(mmProcess fileId, VOID *buffer, INT32 len)
{
    DWORD dwRead = MMPA_ZERO;
    if ((fileId < MMPA_ZERO) || (buffer == nullptr) || len == MMPA_ZERO) {
        return EN_INVALID_PARAM;
    }

    mmSsize_t ret = ReadFile(fileId, buffer, len, &dwRead, nullptr);
    if (ret < MMPA_ZERO) {
        return EN_ERROR;
    }
    return (INT32)dwRead;
}

/*
 * 描述:原子操作的设置某个值
 * 参数:ptr指针指向需要修改的值, value是需要设置的值
 * 返回值:执行成功返回设置为value后的值，入参错误返回EN_INVALID_PARAM
 */
mmAtomicType mmSetData(mmAtomicType *ptr, mmAtomicType value)
{
    if (ptr == nullptr) {
        return EN_INVALID_PARAM;
    }

    return InterlockedExchange(ptr, value);
}

/*
 * 描述:原子操作的加法
 * 参数:ptr指针指向需要修改的值, value是需要加的值
 * 返回值:执行成功返回加上value后的值，入参错误返回EN_INVALID_PARAM
 */
mmAtomicType mmValueInc(mmAtomicType *ptr, mmAtomicType value)
{
    if (ptr == nullptr) {
        return EN_INVALID_PARAM;
    }

    (void)InterlockedExchangeAdd(ptr, value);
    return *ptr;
}

/*
 * 描述:原子操作的减法
 * 参数:ptr指针指向需要修改的值, value是需要减的值
 * 返回值:执行成功返回减去value后的值，入参错误返回EN_INVALID_PARAM
 */
mmAtomicType mmValueSub(mmAtomicType *ptr, mmAtomicType value)
{
    if (ptr == nullptr) {
        return EN_INVALID_PARAM;
    }

    (void)InterlockedExchangeAdd(ptr, (value) * (-1));
    return *ptr;
}

/*
 * 描述:原子操作的设置某个值(64位)
 * 参数:ptr指针指向需要修改的值, value是需要设置的值
 * 返回值:执行成功返回设置为value后的值，入参错误返回EN_INVALID_PARAM
 */
mmAtomicType64 mmSetData64(mmAtomicType64 *ptr, mmAtomicType64 value)
{
    if (ptr == nullptr) {
        return EN_INVALID_PARAM;
    }

    return InterlockedExchange64(ptr, value);
}

/*
 * 描述:原子操作的加法(64位)
 * 参数:ptr指针指向需要修改的值, value是需要加的值
 * 返回值:执行成功返回加上value后的值，入参错误返回EN_INVALID_PARAM
 */
mmAtomicType64 mmValueInc64(mmAtomicType64 *ptr, mmAtomicType64 value)
{
    if (ptr == nullptr) {
        return EN_INVALID_PARAM;
    }

    (void)InterlockedExchangeAdd64(ptr, value);
    return *ptr;
}

/*
 * 描述:原子操作的减法(64位)
 * 参数:ptr指针指向需要修改的值, value是需要减的值
 * 返回值:执行成功返回减去value后的值，入参错误返回EN_INVALID_PARAM
 */
mmAtomicType64 mmValueSub64(mmAtomicType64 *ptr, mmAtomicType64 value)
{
    if (ptr == nullptr) {
        return EN_INVALID_PARAM;
    }

    (void)InterlockedExchangeAdd64(ptr, (value) * (-1));
    return *ptr;
}

/*
 * 描述:创建带分离属性的线程, 不需要mmJoinTask等待线程结束
 * 参数:threadHandle -- pthread_t类型的实例
 *       funcBlock --包含函数名和参数的结构体指针
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmCreateTaskWithDetach(mmThread *threadHandle, mmUserBlock_t *funcBlock)
{
    INT32 ret = EN_OK;
    DWORD threadId;

    if ((threadHandle == nullptr) || (funcBlock == nullptr) || (funcBlock->procFunc == nullptr)) {
        return EN_INVALID_PARAM;
    }

    *threadHandle = CreateThread(nullptr, MMPA_ZERO, LocalThreadProc, (LPVOID)funcBlock, MMPA_ZERO, &threadId);
    (void)CloseHandle(*threadHandle);
    if (*threadHandle == nullptr) {
        ret = EN_ERROR;
    }
    return ret;
}

/*
 * 描述:创建命名管道 待废弃 请使用mmCreatePipe
 * 参数:pipeHandle管道描述符, 必须是两个, 一个读一个写
 *      pipeName-管道名, waitMode-是否阻塞创建
 * 返回值:执行成功返回EN_OK, 失败返回EN_ERROR, 入参错误返回EN_INVALID_PARAM
 */
INT32 mmCreateNamedPipe(mmPipeHandle pipeHandle[], CHAR *pipeName[], INT32 waitMode)
{
    HANDLE namedPipe = CreateNamedPipe((LPCSTR)pipeName[0], PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
        0, 1, MMPA_PIPE_BUF_SIZE, MMPA_PIPE_BUF_SIZE, 0, nullptr);
    if (namedPipe == INVALID_HANDLE_VALUE) {
        namedPipe = nullptr;
        return EN_ERROR;
    }
    if (waitMode) {
        HANDLE hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
        if (hEvent == nullptr) {
            (void)CloseHandle(namedPipe);
            return EN_ERROR;
        }

        OVERLAPPED overlapped;
        SecureZeroMemory(&overlapped, sizeof(overlapped));
        overlapped.hEvent = hEvent;

        if (!ConnectNamedPipe(namedPipe, &overlapped)) {
            if (ERROR_IO_PENDING != GetLastError()) {
                (void)CloseHandle(namedPipe);
                (void)CloseHandle(hEvent);
                return EN_ERROR;
            }
        }

        if (WaitForSingleObject(hEvent, INFINITE) == WAIT_FAILED) {
            (void)CloseHandle(namedPipe);
            (void)CloseHandle(hEvent);
            return EN_ERROR;
        }

        (void)CloseHandle(hEvent);
    }
    pipeHandle[0] = namedPipe;
    pipeHandle[1] = namedPipe;
    return EN_OK;
}

/*
 * 描述:打开创建的命名管道 待废弃 请使用mmOpenPipe
 * 参数:pipeHandle管道描述符, 必须是两个, 一个读一个写
 *      pipeName-管道名, waitMode-是否阻塞打开
 * 返回值:执行成功返回EN_OK, 失败返回EN_ERROR
 */
INT32 mmOpenNamePipe(mmPipeHandle pipeHandle[], CHAR *pipeName[], INT32 waitMode)
{
    DWORD  nTimeOut = 0;
    if (waitMode) {
        nTimeOut = NMPWAIT_WAIT_FOREVER;
    }
    (void)WaitNamedPipe((LPCSTR)pipeName[0], nTimeOut);

    pipeHandle[0] = CreateFile((LPCSTR)pipeName[0], GENERIC_READ | GENERIC_WRITE, 0, nullptr,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (pipeHandle[0] == INVALID_HANDLE_VALUE) {
        return EN_ERROR;
    }
    pipeHandle[1] = pipeHandle[0];
    return EN_OK;
}

/*
 * 描述:关闭命名管道 待废弃 请使用mmClosePipe
 * 参数:namedPipe管道描述符, 必须是两个, 一个读一个写
 */
void mmCloseNamedPipe(mmPipeHandle namedPipe[])
{
    if (namedPipe[0] && namedPipe[1]) {
        (void)CloseHandle(namedPipe[0]);
    }
}

/*
 * 描述:创建管道, 类型(命名管道)
 * 参数:pipeHandle管道描述符, 必须是两个, 一个读一个写
 *      pipeName-管道名, waitMode-是否阻塞创建
 *      pipeCount-管道个数，必须是MMPA_PIPE_COUNT
 *      waitMode非0表示阻塞调用
 * 返回值:执行成功返回EN_OK, 失败返回EN_ERROR, 入参错误返回EN_INVALID_PARAM
 */
INT32 mmCreatePipe(mmPipeHandle pipeHandle[], CHAR *pipeName[], UINT32 pipeCount, INT32 waitMode)
{
    if (pipeCount != MMPA_PIPE_COUNT || pipeHandle == nullptr || pipeName == nullptr ||
        pipeName[0] == nullptr || pipeName[1] == nullptr) {
        return EN_INVALID_PARAM;
    }

    HANDLE namedPipe = CreateNamedPipe((LPCSTR)pipeName[0], PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED, 0, 1,
                                       MMPA_PIPE_BUF_SIZE, MMPA_PIPE_BUF_SIZE, 0, nullptr);
    if (namedPipe == INVALID_HANDLE_VALUE) {
        namedPipe = nullptr;
        return EN_ERROR;
    }
    if (waitMode != MMPA_ZERO) {
        HANDLE event = CreateEvent(nullptr, TRUE, FALSE, nullptr);
        if (event == nullptr) {
            (void)CloseHandle(namedPipe);
            return EN_ERROR;
        }

        OVERLAPPED overlapped;
        SecureZeroMemory(&overlapped, sizeof(overlapped));
        overlapped.hEvent = event;

        if (ConnectNamedPipe(namedPipe, &overlapped) == MMPA_ZERO) {
            if (GetLastError() != ERROR_IO_PENDING) {
                (void)CloseHandle(namedPipe);
                (void)CloseHandle(event);
                return EN_ERROR;
            }
        }

        if (WaitForSingleObject(event, INFINITE) == WAIT_FAILED) {
            (void)CloseHandle(namedPipe);
            (void)CloseHandle(event);
            return EN_ERROR;
        }

        (void)CloseHandle(event);
    }

    pipeHandle[0] = namedPipe;
    pipeHandle[1] = namedPipe;

    return 0;
}

/*
 * 描述:打开创建的命名管道
 * 参数:pipeHandle管道描述符, 必须是两个, 一个读一个写
 *      pipeName-管道名, waitMode-是否阻塞打开
 *      pipeCount-管道个数，必须是MMPA_PIPE_COUNT
 * 返回值:执行成功返回EN_OK, 失败返回EN_ERROR, 入参错误返回EN_INVALID_PARAM
 */
INT32 mmOpenPipe(mmPipeHandle pipeHandle[], CHAR *pipeName[], UINT32 pipeCount, INT32 waitMode)
{
    if (pipeCount != MMPA_PIPE_COUNT || pipeHandle == nullptr || pipeName == nullptr ||
        pipeName[0] == nullptr || pipeName[1] == nullptr) {
        return EN_INVALID_PARAM;
    }
    INT32 ret = mmOpenNamePipe(pipeHandle, pipeName, waitMode);

    return ret;
}

/*
 * 描述:关闭命名管道
 * 参数:pipeHandle管道描述符, 必须是两个, 一个读一个写
 *      pipeCount-管道个数，必须是MMPA_PIPE_COUNT
 */
void mmClosePipe(mmPipeHandle pipeHandle[], UINT32 pipeCount)
{
    if (pipeCount != MMPA_PIPE_COUNT || pipeHandle == nullptr) {
        return;
    }
    if ((pipeHandle[0] != MMPA_ZERO) && (pipeHandle[1] != MMPA_ZERO)) {
        (void)CloseHandle(pipeHandle[0]);
    }
}

/*
 * 描述:创建完成端口, Linux内部为空实现, 主要用于windows系统
 */
mmCompletionHandle mmCreateCompletionPort()
{
    mmCompletionHandle handleComplete = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
    return handleComplete;
}

/*
 * 描述:关闭完成端口, Linux内部为空实现, 主要用于windows系统
 */
VOID mmCloseCompletionPort(mmCompletionHandle handle)
{
    if (handle != INVALID_HANDLE_VALUE) {
        (void)CloseHandle(handle);
    }
}

/*
 * 描述:mmPoll内部使用，用来接收数据
 * 参数:fd--需要poll的资源描述符
 *      polledData--读取/接收到的数据
 */
VOID LocalReceiveAction(mmPollfd fd, VOID *buf, UINT32 bufLen, LPOVERLAPPED poa)
{
    PPRE_IO_DATA pPreIoData = nullptr;
    mmPollType overlapType = fd.pollType;
    HANDLE handle = fd.handle;
    if (overlapType == pollTypeRead) { // read
        DWORD dwRead = 0;
        ReadFile(handle, buf, bufLen, &dwRead, poa);
    } else if (overlapType == pollTypeRecv) { // wsarecv
        DWORD dwRecv = 0;
        DWORD dwFlags = 0;
        PRE_IO_DATA ioData;
        pPreIoData = &ioData;
        SecureZeroMemory(pPreIoData, sizeof(PRE_IO_DATA));
        pPreIoData->DataBuf.len = bufLen;
        pPreIoData->DataBuf.buf = reinterpret_cast<CHAR *>(buf);
        pPreIoData->completionHandle = handle;
        WSARecv((SOCKET)handle, (LPWSABUF)&pPreIoData->DataBuf, 1, &dwRecv, &dwFlags, poa, nullptr);
    } else if (overlapType == pollTypeIoctl) { // ioctl
        DWORD dwRead = 0;
        DeviceIoControl(handle,           // device to be queried
                        fd.ioctlCode,     // operation to perform
                        nullptr,
                        0,                // no input buffer
                        buf,              // output buffer
                        bufLen,
                        &dwRead,          // bytes returned
                        poa);             // synchronous I/O
    }
}

/*
 * 描述:等待IO数据是否可读可接收
 * 参数:指向需要等待的fd的集合的指针和个数，timeout -- 超时等待时间,
         handleIOCP -- 对应的完成端口key,
         polledData -- 由用户分配的缓存,
         pollBack --用户注册的回调函数,
 *       polledData -- 若读取到会填充进缓存，回调出来，缓存大小由用户分配
 * 返回值:超时返回EN_ERR, 读取成功返回EN_OK, 失败返回EN_ERROR, 入参错误返回EN_INVALID_PARAM
 */
INT32 mmPoll(mmPollfd *fds, INT32 fdCount, INT32 timeout, mmCompletionHandle handleIOCP,
    pmmPollData polledData, mmPollBack pollBack)
{
    if (fds == nullptr || polledData == nullptr || polledData->buf == nullptr || fdCount < 0) {
        return EN_INVALID_PARAM;
    }
    DWORD tout = timeout;
    if (timeout == EN_ERROR) {
        tout = INFINITE;
    }
    for (INT32 i = 0; i < fdCount; i++) {
        pmmComPletionKey pCompletionKey = &fds[i].completionKey;
        OVERLAPPED  overlapped;
        SecureZeroMemory(&overlapped, sizeof(overlapped));
        if (fds[i].handle == INVALID_HANDLE_VALUE) {
            return EN_INVALID_PARAM;
        }
        pCompletionKey->completionHandle = fds[i].handle;
        pCompletionKey->overlapType = fds[i].pollType;
        pCompletionKey->oa = overlapped;

        CreateIoCompletionPort(fds[i].handle, handleIOCP, (ULONG_PTR)pCompletionKey, 0);
        LocalReceiveAction(fds[i], polledData->buf, polledData->bufLen, &pCompletionKey->oa);
    }
    DWORD dwTransCount = 0;
    pmmComPletionKey outCompletionKey = nullptr;
    pmmPollData outPolledData = nullptr;
    if (GetQueuedCompletionStatus(handleIOCP, &dwTransCount,
        reinterpret_cast<PULONG_PTR>(&outCompletionKey),
        reinterpret_cast<LPOVERLAPPED *>(&outPolledData), tout)) {
        polledData->bufHandle = outCompletionKey->completionHandle;
        polledData->bufRes = dwTransCount;
        polledData->bufType = outCompletionKey->overlapType;
        if (dwTransCount == MMPA_ZERO) {
            return EN_ERROR;
        }
        pollBack(polledData);
        return EN_OK;
    } else if (GetLastError() == WAIT_TIMEOUT) { // The wait operation timed out.
        return EN_ERR;
    } else {
        polledData->bufRes = dwTransCount;
        return EN_ERROR;
    }
}

/*
 * 描述:获取错误码
 * 返回值:error code
 */
INT32 mmGetErrorCode()
{
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        errno = EEXIST;
    }
    return GetLastError();
}

/*
* 描述：将mmGetErrorCode函数得到的错误信息转化成字符串信息
* 参数： errnum--错误码，即mmGetErrorCode的返回值
*       buf--收错误信息描述的缓冲区指针
*       size--缓冲区的大小
* 返回值:成功返回错误信息的字符串，失败返回nullptr
*/
CHAR *mmGetErrorFormatMessage(mmErrorMsg errnum, CHAR *buf, mmSize size)
{
    if (buf == nullptr || size <= 0) {
        return nullptr;
    }

    mmErrorMsg ret = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                                   nullptr,
                                   errnum,
                                   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                   reinterpret_cast<LPTSTR>(buf),
                                   size,
                                   nullptr);
    if (ret == MMPA_ZERO) {
        return nullptr;
    }
    return buf;
}

/*
 * 描述:获取当前系统时间和时区信息, windows不支持时区获取
 * 参数:timeVal--当前系统时间, 不能为nullptr
        timeZone--当前系统设置的时区信息, 可以为nullptr, 表示不需要获取时区信息
 * 返回值:执行成功返回EN_OK, 失败返回EN_ERROR，入参错误返回EN_INVALID_PARAM
 */
INT32 mmGetTimeOfDay(mmTimeval *timeVal, mmTimezone *timeZone)
{
    struct tm nowTime;
    SYSTEMTIME sysTime;
    if (timeVal == nullptr) {
        return EN_INVALID_PARAM;
    }
    GetLocalTime(&sysTime);
    nowTime.tm_year = sysTime.wYear - MMPA_COMPUTER_BEGIN_YEAR;
    // tm.tm_mon is [0, 11], wtm.wMonth is [1, 12]
    nowTime.tm_mon = sysTime.wMonth - 1;
    nowTime.tm_mday = sysTime.wDay;
    nowTime.tm_hour = sysTime.wHour;
    nowTime.tm_min = sysTime.wMinute;
    nowTime.tm_sec = sysTime.wSecond;

    nowTime.tm_isdst = SUMMER_TIME_OR_NOT;
    // 将时间转换为自1970年1月1日以来逝去时间的秒数
    time_t seconds = mktime(&nowTime);
    if (seconds == EN_ERROR) {
        return EN_ERROR;
    }
    timeVal->tv_sec = seconds;
    timeVal->tv_usec = sysTime.wMilliseconds * MMPA_MSEC_TO_USEC;

    return EN_OK;
}

/*
 * 描述:获取系统开机到现在经过的时间
 * 返回值:执行成功返回类型mmTimespec结构的时间
 */
mmTimespec mmGetTickCount()
{
    mmTimespec rts;
    ULONGLONG milliSecond = GetTickCount64();
    rts.tv_sec = (MM_LONG)milliSecond / MMPA_SECOND_TO_MSEC;
    rts.tv_nsec = (MM_LONG)(milliSecond % MMPA_SECOND_TO_MSEC) * MMPA_MSEC_TO_NSEC;
    return rts;
}

/*
 * 描述:将参数path所指的相对路径转换成绝对路径, 接口待废弃, 请使用mmRealPath
 * 参数:path--原始路径相对路径
         realPath--规范化后的绝对路径, 由用户分配内存, 长度必须要>= MMPA_MAX_PATH
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmGetRealPath(CHAR *path, CHAR *realPath)
{
    if (realPath == nullptr || path == nullptr) {
        return EN_INVALID_PARAM;
    }

    if (_fullpath(realPath, path, MMPA_MAX_PATH) == nullptr) {
        return EN_ERROR;
    }

    return EN_OK;
}

/*
 * 描述:将参数path所指的相对路径转换成绝对路径
 * 参数:path--原始路径相对路径
         realPath--规范化后的绝对路径, 由用户分配内存
         realPathLen--realPath缓存的长度, 长度必须要>= MMPA_MAX_PATH
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmRealPath(const CHAR *path, CHAR *realPath, INT32 realPathLen)
{
    if (realPath == nullptr || path == nullptr || realPathLen < MMPA_MAX_PATH) {
        return EN_INVALID_PARAM;
    }

    if (_fullpath(realPath, path, MMPA_MAX_PATH) == nullptr) {
        return EN_ERROR;
    }

    return EN_OK;
}

/*
 * mmScandir接口内部使用，查找有效的子目录或文件
 */
BOOL LocalFindFile(WIN32_FIND_DATA *findData)
{
    BOOL findFlag = FALSE;
    if (!(findData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
        findFlag = TRUE;
    } else if (findData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        if (findData->cFileName[0] == '.' && (findData->cFileName[1] == MMPA_ZERO || findData->cFileName[1] == '.')) {
            return findFlag;
        }
        findFlag = TRUE;
    }
    return findFlag;
}

/*
 * 描述:扫描目录,最多支持扫描MMPA_MAX_SCANDIR_COUNT个数目录
 * 参数:path--目录路径
        filterFunc--用户指定的过滤回调函数
        sort--用户指定的排序回调函数
        entryList--扫描到的目录结构指针, 用户不需要分配缓存, 内部分配, 需要调用mmScandirFree释放
 * 返回值:执行成功返回扫描到的子目录数量, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmScandir(const CHAR *path, mmDirent ***entryList, mmFilter filterFunc, mmSort sort)
{
    if (path == nullptr) {
        return EN_INVALID_PARAM;
    }

    CHAR findPath[_MAX_PATH] = {0};
    WIN32_FIND_DATA findData;
    mmDirent **nameList = nullptr;
    mmDirent *newList = nullptr;
    size_t pos = 0;

    BOOL value = PathCanonicalize(findPath, path);
    if (!value) {
        return EN_INVALID_PARAM;
    }
    // 保证有6个字节的空余
    if (strlen(findPath) >= (_MAX_PATH - strlen("\\*.*") - 1)) {
        return EN_INVALID_PARAM;
    }
    INT32 ret = strcat_s(findPath, _MAX_PATH, "\\*.*");
    if (ret != MMPA_ZERO) {
        return EN_INVALID_PARAM;
    }
    HANDLE fh = FindFirstFile(findPath, &findData);
    if (fh == INVALID_HANDLE_VALUE) {
        *entryList = nameList;
        return pos;
    }
    // 支持扫描最多1024个目录
    nameList = reinterpret_cast<mmDirent **>(malloc(MMPA_MAX_SCANDIR_COUNT * sizeof(mmDirent *)));
    if (nameList == nullptr) {
        FindClose(fh);
        return EN_ERROR;
    }
    SecureZeroMemory(nameList, MMPA_MAX_SCANDIR_COUNT * sizeof(mmDirent *));
    do {
        if (LocalFindFile(&findData)) {
            mmDirent tmpDir;
            (void)memcpy_s(tmpDir.d_name, sizeof(tmpDir.d_name), findData.cFileName, MAX_PATH);
            if (filterFunc != nullptr && filterFunc(&tmpDir) == 0) {
                continue;
            }
            newList = reinterpret_cast<mmDirent *>(malloc(sizeof(mmDirent)));
            if (newList == nullptr) {
                break;
            }
            SecureZeroMemory(newList, sizeof(mmDirent));
            newList->d_type = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
            (void)memcpy_s(newList->d_name, sizeof(newList->d_name), findData.cFileName, MAX_PATH);
            nameList[pos++] = newList;
        }
    } while (FindNextFile(fh, &findData) && pos < MMPA_MAX_SCANDIR_COUNT);
    FindClose(fh);

    if (sort != nullptr && nameList != nullptr) {
        qsort(nameList, pos, sizeof(mmDirent *), (_CoreCrtNonSecureSearchSortCompareFunction)sort);
    }
    *entryList = nameList;
    return pos;
}

/*
 * 描述:扫描目录,最多支持扫描MMPA_MAX_SCANDIR_COUNT个数目录
 * 参数:path--目录路径
        filterFunc--用户指定的过滤回调函数
        sort--用户指定的排序回调函数
        entryList--扫描到的目录结构指针, 用户不需要分配缓存, 内部分配, 需要调用mmScandirFree2释放
 * 返回值:执行成功返回扫描到的子目录和文件数量, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmScandir2(const CHAR *path, mmDirent2 ***entryList, mmFilter2 filterFunc, mmSort2 sort)
{
    if (path == nullptr) {
        return EN_INVALID_PARAM;
    }

    CHAR findPath[_MAX_PATH] = {0};
    WIN32_FIND_DATA findData;
    mmDirent2 **nameList = nullptr;
    mmDirent2 *newList = nullptr;
    size_t pos = 0;

    BOOL value = PathCanonicalize(findPath, path);
    if (!value) {
        return EN_INVALID_PARAM;
    }
    // 保证有6个字节的空余
    if (strlen(findPath) >= (_MAX_PATH - strlen("\\*.*") - 1)) {
        return EN_INVALID_PARAM;
    }
    INT32 ret = strcat_s(findPath, _MAX_PATH - 1, "\\*.*");
    if (ret != MMPA_ZERO) {
        return EN_INVALID_PARAM;
    }
    HANDLE fh = FindFirstFile(findPath, &findData);
    if (fh == INVALID_HANDLE_VALUE) {
        *entryList = nameList;
        return pos;
    }
    // 支持扫描最多1024个目录
    nameList = reinterpret_cast<mmDirent2 **>(malloc(MMPA_MAX_SCANDIR_COUNT * sizeof(mmDirent2 *)));
    if (nameList == nullptr) {
        FindClose(fh);
        return EN_ERROR;
    }
    SecureZeroMemory(nameList, MMPA_MAX_SCANDIR_COUNT * sizeof(mmDirent2 *));
    do {
        if (LocalFindFile(&findData)) {
            mmDirent2 tmpDir;
            tmpDir.d_type = findData.dwFileAttributes;
            (void)memcpy_s(tmpDir.d_name, sizeof(tmpDir.d_name), findData.cFileName, MAX_PATH);
            if (filterFunc != nullptr && filterFunc(&tmpDir) == 0) {
                continue;
            }
            newList = reinterpret_cast<mmDirent2 *>(malloc(sizeof(mmDirent2)));
            if (newList == nullptr) {
                break;
            }
            SecureZeroMemory(newList, sizeof(mmDirent2));
            newList->d_type = findData.dwFileAttributes;
            (void)memcpy_s(newList->d_name, sizeof(newList->d_name), findData.cFileName, MAX_PATH);
            nameList[pos++] = newList;
        }
    } while (FindNextFile(fh, &findData) && pos < MMPA_MAX_SCANDIR_COUNT);
    FindClose(fh);

    if (sort != nullptr && nameList != nullptr) {
        qsort(nameList, pos, sizeof(mmDirent2 *), (_CoreCrtNonSecureSearchSortCompareFunction)sort);
    }
    *entryList = nameList;
    return pos;
}

/*
 * 描述:扫描目录对应的内存释放函数
 * 参数:entryList--mmScandir扫描到的目录结构指针
 *      count--扫描到的子目录数量
 * 返回值:无
 */
void mmScandirFree(mmDirent **entryList, INT32 count)
{
    if (entryList == nullptr || count < MMPA_ZERO || count > MMPA_MAX_SCANDIR_COUNT) {
        return;
    }
    INT32 i;
    for (i = 0; i < count; i++) {
        if (entryList[i] != nullptr) {
            free(entryList[i]);
            entryList[i] = nullptr;
        }
    }
    free(entryList);
    entryList = nullptr;
}

/*
 * 描述:扫描目录对应的内存释放函数
 * 参数:entryList--mmScandir2扫描到的目录结构指针
 *      count--扫描到的子目录数量
 * 返回值:无
 */
void mmScandirFree2(mmDirent2 **entryList, INT32 count)
{
    if (entryList == nullptr || count < MMPA_ZERO || count > MMPA_MAX_SCANDIR_COUNT) {
        return;
    }
    INT32 i;
    for (i = 0; i < count; i++) {
        if (entryList[i] != nullptr) {
            free(entryList[i]);
            entryList[i] = nullptr;
        }
    }
    free(entryList);
    entryList = nullptr;
}

/*
 * 描述:创建一个消息队列用于进程间通信
 * 参数:key--消息队列的KEY键值,
 *      msgFlag--取消息队列标识符
 * 返回值:执行成功则返回打开的消息队列ID, 执行错误返回EN_ERROR
 */
mmMsgid mmMsgOpen(mmKey_t key, INT32 msgFlag)
{
    CHAR pipeName[MAX_PATH] = {0};
    INT32 ret = _snprintf_s(pipeName, sizeof(pipeName) - 1, "\\\\.\\pipe\\%d", key);
    if (ret < MMPA_ZERO) {
        return (mmMsgid)EN_ERROR;
    }
    // 打开命名管道
    mmMsgid namedPipeId = CreateFile((LPCSTR)pipeName, GENERIC_READ | GENERIC_WRITE, 0, nullptr,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (namedPipeId == INVALID_HANDLE_VALUE) {
        if (GetLastError() == ERROR_FILE_NOT_FOUND) { // 打开失败，说明还没有创建命名管道
            namedPipeId = CreateNamedPipe((LPCSTR)pipeName, PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED, 0, 1,
                MMPA_PIPE_BUF_SIZE, MMPA_PIPE_BUF_SIZE, 0, nullptr);
            if (namedPipeId == INVALID_HANDLE_VALUE) {
                namedPipeId = nullptr;
                return (mmMsgid)EN_ERROR;
            }
        } else {
            namedPipeId = nullptr;
            return (mmMsgid)EN_ERROR;
        }
    }
    return namedPipeId;
}

/*
 * 描述:扫描目录对应的内存释放函数
 * 参数:key--消息队列的KEY键值,
 *      msgFlag--取消息队列标识符
 * 返回值:执行成功则返回打开的消息队列ID, 执行错误返回EN_ERROR
 */
mmMsgid mmMsgCreate(mmKey_t key, INT32 msgFlag)
{
    CHAR pipeName[MAX_PATH] = {0};
    INT32 ret = _snprintf_s(pipeName, sizeof(pipeName) - 1, "\\\\.\\pipe\\%d", key);
    if (ret < MMPA_ZERO) {
        return (mmMsgid)EN_ERROR;
    }
    // 这里创建的是双向模式且使用重叠模式的命名管道
    mmMsgid namedPipe = CreateNamedPipe((LPCSTR)pipeName, PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED, 0, 1,
                                        MMPA_PIPE_BUF_SIZE, MMPA_PIPE_BUF_SIZE, 0, nullptr);
    if (namedPipe == INVALID_HANDLE_VALUE) {
        if (GetLastError() == ERROR_PIPE_BUSY) { // 说明管道已经创建，打开就可以
            namedPipe = CreateFile((LPCSTR)pipeName, GENERIC_READ | GENERIC_WRITE, 0, nullptr,
                                   OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (namedPipe == INVALID_HANDLE_VALUE) {
                namedPipe = nullptr;
                return (mmMsgid)EN_ERROR;
            }
        } else {
            namedPipe = nullptr;
            return (mmMsgid)EN_ERROR;
        }
    }
    return namedPipe;
}

/*
 * 描述:往消息队列发送消息
 * 参数:msqid--消息队列ID
 *      buf--由用户分配内存
 *      bufLen--缓存长度
 *      msgFlag--消息标志位
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmMsgSnd(mmMsgid msqid, VOID *buf, INT32 bufLen, INT32 msgFlag)
{
    if (buf == nullptr || bufLen <= MMPA_ZERO || msqid == INVALID_HANDLE_VALUE) {
        return EN_INVALID_PARAM;
    }

    DWORD dwWrite = 0;
    ConnectNamedPipe(msqid, nullptr); // 阻塞等待，直到对端管道打开

    if (WriteFile(msqid, buf, bufLen, &dwWrite, nullptr) == MMPA_ZERO) {
        return EN_ERROR;
    }
    return EN_OK;
}
/*
 * 描述:从消息队列接收消息
 * 参数:msqid--消息队列ID
 *      bufLen--缓存长度
 *      msgFlag--消息标志位
 *      buf--由用户分配内存
 * 返回值:执行成功返回接收的长度, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmMsgRcv(mmMsgid msqid, VOID *buf, INT32 bufLen, INT32 msgFlag)
{
    if (buf == nullptr || bufLen <= MMPA_ZERO || msqid == INVALID_HANDLE_VALUE) {
        return EN_INVALID_PARAM;
    }

    DWORD dwRead = 0;
    ConnectNamedPipe(msqid, nullptr); // 阻塞等待，直到对端管道打开

    if (ReadFile(msqid, buf, bufLen, &dwRead, nullptr) == MMPA_ZERO) {
        return EN_ERROR;
    }
    return (INT32)dwRead;
}

/*
 * 描述:关闭创建的消息队列
 * 参数:msqid--消息队列ID
 * 返回值:执行成功返回EN_OK
 */
INT32 mmMsgClose(mmMsgid msqid)
{
    if (msqid != INVALID_HANDLE_VALUE) {
        (void)CloseHandle(msqid);
    }
    return EN_OK;
}

/*
 * 描述:转换时间为本地时间格式
 * 参数:timep--待转换的time_t类型的时间
 *      result--struct tm格式类型的时间
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmLocalTimeR(const time_t *timep, struct tm *result)
{
    INT32 ret = EN_OK;
    if (timep == nullptr || result == nullptr) {
        ret = EN_INVALID_PARAM;
    } else {
        time_t time = *timep;
        struct tm nowtTime;
        if (localtime_s(&nowtTime, &time) != MMPA_ZERO) {
            return EN_ERROR;
        }
        result->tm_year = nowtTime.tm_year + MMPA_COMPUTER_BEGIN_YEAR;
        result->tm_mon = nowtTime.tm_mon + 1;
        result->tm_mday = nowtTime.tm_mday;
        result->tm_hour = nowtTime.tm_hour;
        result->tm_min = nowtTime.tm_min;
        result->tm_sec = nowtTime.tm_sec;
    }
    return ret;
}

/*
 * 描述:内部使用,计算a和b的最大公约数
 */
static INT32 localGcd(INT32 a, INT32 b)
{
    INT32 c;
    if (b == MMPA_ZERO) {
        return MMPA_ZERO;
    }
    c = a % b;
    while (c != MMPA_ZERO) {
        a = b;
        b = c;
        c = a % b;
    }
    return (b);
}

/*
 * 描述:内部使用, 交换内容块
 */
static void LocalPermuteArgs(INT32 panonoptStart, INT32 panonoptEnd, INT32 optEnd, CHAR * const *nargv)
{
    INT32 start;
    INT32 i;
    INT32 j;
    INT32 pos;
    CHAR *swap = nullptr;

    INT32 nonOpts = panonoptEnd - panonoptStart;
    INT32 opts = optEnd - panonoptEnd;
    INT32 cycle = localGcd(nonOpts, opts);
    if (cycle == MMPA_ZERO) {
        return;
    }
    INT32 cycleLen = (optEnd - panonoptStart) / cycle;

    for (i = 0; i < cycle; i++) {
        start = panonoptEnd + i;
        pos = start;
        for (j = 0; j < cycleLen; j++) {
            if (pos >= panonoptEnd) {
                pos -= nonOpts;
            } else {
                pos += opts;
            }
            swap = nargv[pos];
            /* LINTED const cast */
            (const_cast<CHAR **>(nargv))[pos] = nargv[start];
            /* LINTED const cast */
            (const_cast<CHAR **>(nargv))[start] = swap;
        }
    }
}

/*
 * 描述:内部使用
 */
static INT32 LocalOptionProcess(CHAR * const *nargv, const CHAR *options, const mmStructOption *longOption,
    INT32 match, CHAR *hasEqual)
{
    if (longOption->has_arg == MMPA_NO_ARGUMENT && hasEqual) {
        ERRB(OPT_NO_ARG, (INT32)strlen(longOption->name), longOption->name);
        if (longOption->flag == nullptr) {
            optopt = longOption->val;
        } else {
            optopt = 0;
        }
        return (MMPA_BADARG);
    }
    if (longOption->has_arg == MMPA_REQUIRED_ARGUMENT || longOption->has_arg == MMPA_OPTIONAL_ARGUMENT) {
        if (hasEqual != nullptr) {
            optarg = hasEqual;
        } else if (longOption->has_arg == MMPA_REQUIRED_ARGUMENT) {
            optarg = nargv[optind++];
        }
    }
    if ((longOption->has_arg == MMPA_REQUIRED_ARGUMENT) && (optarg == nullptr)) {
        ERRA(OPT_REQ_ARG_STRING, longOption->name);
        if (longOption->flag == nullptr) {
            optopt = longOption->val;
        } else {
            optopt = 0;
        }
        --optind;
        return (MMPA_BADARG);
    }
    return 0;
}

/*
 * 描述:内部使用,
 */
static INT32 LocalGetMatch(const CHAR *options, const mmStructOption *longOptions,
    INT32 shortToo, size_t currentArgvLen, INT32 *match)
{
    INT32 i;
    CHAR *currentArgv = g_place;
    for (i = 0; longOptions[i].name; i++) {
        /* find matching long option */
        if (strncmp(currentArgv, longOptions[i].name, currentArgvLen)) {
            continue;
        }
        if (strlen(longOptions[i].name) == currentArgvLen) {
            *match = i;
            break;
        }
        if (shortToo && currentArgvLen == 1) {
            continue;
        }
        if (*match == -1) {
            *match = i;
        } else {
            ERRB(OPT_AMBIGIOUS, (INT32)currentArgvLen, currentArgv);
            optopt = 0;
            return (MMPA_BADCH);
        }
    }
    return 0;
}

/*
 * 描述:内部使用, 解析长选项参数
 * 返回值:如果short_too设置了并且选项不匹配返回-1
 */
static INT32 LocalParseLongOptions(CHAR * const *nargv, const CHAR *options,
                                   const mmStructOption *longOptions, INT32 *idx, INT32 shortToo)
{
    CHAR *currentArgv = nullptr;
    CHAR *hasEqual = nullptr;
    size_t currentArgvLen;
    INT32 match = -1;
    currentArgv = g_place;
    optind++;

    hasEqual = strchr(currentArgv, '=');
    if (hasEqual != nullptr) {
        currentArgvLen = hasEqual - currentArgv;
        hasEqual++;
    } else {
        currentArgvLen = strlen(currentArgv);
    }
    INT32 ret = LocalGetMatch(options, longOptions, shortToo, currentArgvLen, &match);
    if (ret != 0) {
        return ret;
    }

    if (match != -1) {
        ret = LocalOptionProcess(nargv, options, &longOptions[match], match, hasEqual);
        if (ret != 0) {
            return ret;
        }
        if (idx != nullptr) {
            *idx = match;
        }
        if (longOptions[match].flag) {
            *longOptions[match].flag = longOptions[match].val;
            return (0);
        } else {
            return (longOptions[match].val);
        }
    } else  {
        if (shortToo) {
            --optind;
            return (-1);
        }
        ERRA(OPT_ILLEGAL_STRING, currentArgv);
        optopt = 0;
        return (MMPA_BADCH);
    }
}

static INT32 LocalNonOption(CHAR * const *nargv, INT32 flags)
{
    g_place = const_cast<CHAR *>(MMPA_EMSG);   /* found non-option */
    if (flags & MMPA_FLAG_ALLARGS) {
        optarg = nargv[optind++];
        return (MMPA_INORDER);
    }
    if (!(flags & MMPA_FLAG_PERMUTE)) {
        return (-1);
    }
    if (g_nonoptStart == -1) {
        g_nonoptStart = optind;
    } else if (g_nonoptEnd != -1) {
        LocalPermuteArgs(g_nonoptStart, g_nonoptEnd, optind, nargv);
        g_nonoptStart = optind - g_nonoptEnd + g_nonoptStart;
        g_nonoptEnd = -1;
    }
    optind++;
    return EN_INVALID_PARAM;
}

static INT32 LocalUpdateScanPoint(INT32 nargc, CHAR * const *nargv, const CHAR *options, UINT32 flags)
{
    /* update scanning pointer */
    optreset = 0;
    INT32 ret;
    if (optind >= nargc) {
    /* end of argument vector */
        g_place = const_cast<CHAR *>(MMPA_EMSG);
        if (g_nonoptEnd != -1) {
            LocalPermuteArgs(g_nonoptStart, g_nonoptEnd, optind, nargv);
            optind -= g_nonoptEnd - g_nonoptStart;
        } else if (g_nonoptStart != -1) {
            optind = g_nonoptStart;
        }
        g_nonoptStart = -1;
        g_nonoptEnd = -1;
        return (-1);
    }
    g_place = nargv[optind];
    if (*g_place != '-' || (g_place[1] == '\0' && strchr(options, '-') == nullptr)) {
        ret = LocalNonOption(nargv, flags);
        return ret;
    }
    if (g_nonoptStart != -1 && g_nonoptEnd == -1) {
        g_nonoptEnd = optind;
    }

    if (g_place[1] != '\0' && *++g_place == '-' && g_place[1] == '\0') {
        optind++;
        g_place = const_cast<CHAR *>(MMPA_EMSG);
        if (g_nonoptEnd != -1) {
            LocalPermuteArgs(g_nonoptStart, g_nonoptEnd, optind, nargv);
            optind -= g_nonoptEnd - g_nonoptStart;
        }
        g_nonoptStart = -1;
        g_nonoptEnd = -1;
        return (-1);
    }
    return 0;
}

static INT32 LocalTakeArg(INT32 nargc, CHAR * const *nargv, const CHAR *options,
                          CHAR *oli, UINT32 flags, INT32 *optchar)
{
    if (*++oli != ':') {
        if (!*g_place) {
            ++optind;
        }
    } else {
        optarg = nullptr;
        if (*g_place) {
            optarg = g_place;
        } else if (oli[1] != ':') { /* arg not optional */
            if (++optind >= nargc) {    /* no arg */
                g_place = const_cast<CHAR *>(MMPA_EMSG);
                ERRA(OPT_REQ_ARG_CHAR, *optchar);
                optopt = *optchar;
                return (MMPA_BADARG);
            } else {
                optarg = nargv[optind];
            }
        } else if (!(flags & MMPA_FLAG_PERMUTE)) {
            if (optind + 1 < nargc && *nargv[optind + 1] != '-') {
                optarg = nargv[++optind];
            }
        }
        g_place = const_cast<CHAR *>(MMPA_EMSG);
        ++optind;
    }
    return 0;
}

static void LocalDisableExten(const CHAR *options, UINT32 flags)
{
    static INT32 posixly_correct = -1;
    if (posixly_correct == -1) {
        posixly_correct = (getenv("POSIXLY_CORRECT") != nullptr);
    }
    if (posixly_correct || *options == '+') {
        flags &= ~MMPA_FLAG_PERMUTE;
    } else if (*options == '-') {
        flags |= MMPA_FLAG_ALLARGS;
    }
    if (*options == '+' || *options == '-') {
        options++;
    }
    if (optind == MMPA_ZERO) {
        optind = optreset = 1;
    }
    optarg = nullptr;
    if (optreset) {
        g_nonoptStart = g_nonoptEnd = -1;
    }
}

static INT32 LocalJudgeMinus(const CHAR *options, INT32 *optchar, CHAR **oli)
{
    if (((*optchar = (INT32)*g_place++) == (INT32)':') || (*optchar == (INT32)'-' && *g_place != '\0') ||
        ((*oli = const_cast<CHAR *>(strchr(options, *optchar))) == nullptr)) {
        if (*optchar == (INT32)'-' && *g_place == '\0') {
            return (-1);
        }
        if (!*g_place) {
            ++optind;
        }
        ERRA(OPT_ILLEGAL_CHAR, *optchar);
        optopt = *optchar;
        return (MMPA_BADCH);
    }
    return 0;
}

static INT32 LocalJudgeW(INT32 nargc, CHAR * const *nargv, const CHAR *options,
                         const mmStructOption *longOptions, INT32 *idx)
{
    INT32 optchar = 'W';

    if (*g_place == MMPA_ZERO) {
        if (++optind >= nargc) {    /* no arg */
            g_place = const_cast<CHAR *>(MMPA_EMSG);
            ERRA(OPT_REQ_ARG_CHAR, optchar);
            optopt = optchar;
            return -1;
        } else {    /* white space */
            g_place = nargv[optind];
        }
    }
    optchar = LocalParseLongOptions(nargv, options, longOptions, idx, 0);
    g_place = const_cast<CHAR *>(MMPA_EMSG);
    return (optchar);
}

static INT32 LocalJudgeM(CHAR * const *nargv, const CHAR *options,
                         const mmStructOption *longOptions, INT32 *idx, INT32 *optchar)
{
    INT32 shortToo = 0;
    if (*g_place == '-') {
        g_place++;    /* --foo long option */
    } else if (*g_place != ':' && strchr(options, *g_place) != nullptr) {
        shortToo = 1;   /* could be short option too */
    }
    *optchar = LocalParseLongOptions(nargv, options, longOptions, idx, shortToo);
    if (*optchar != EN_ERROR) {
        g_place = const_cast<CHAR *>(MMPA_EMSG);
        return (*optchar);
    }
    return 0;
}

/*
 * 描述:内部使用, 解析命令行参数
 */
static INT32 LocalGetOptInternal(INT32 nargc, CHAR * const *nargv, const CHAR *options,
                                 const mmStructOption *longOptions, INT32 *idx, UINT32 flags)
{
    CHAR *optList = nullptr;
    INT32 optChar = 0;
    INT32 ret;
START:
    if (optreset || !*g_place) {
        ret = LocalUpdateScanPoint(nargc, nargv, options, flags);
        if (ret == EN_INVALID_PARAM) {
            goto START;
        } else if (ret != MMPA_ZERO) {
            return ret;
        }
    }

    if (longOptions != nullptr && g_place != nargv[optind] && (*g_place == '-' || (flags & MMPA_FLAG_LONGONLY))) {
        ret = LocalJudgeM(nargv, options, longOptions, idx, &optChar);
        if (ret != MMPA_ZERO) {
            return optChar;
        }
    }

    ret = LocalJudgeMinus(options, &optChar, &optList);
    if (ret != MMPA_ZERO) {
        return ret;
    }

    if (longOptions != nullptr && optChar == 'W' && optList[1] == ';') {
        ret = LocalJudgeW(nargc, nargv, options, longOptions, idx);
        return ret;
    }

    ret = LocalTakeArg(nargc, nargv, options, optList, flags, &optChar);
    if (ret != MMPA_ZERO) {
        return ret;
    }
    /* dump back option letter */
    return (optChar);
}

/*
 * 描述:获取变量opterr的值
 * 返回值：获取到opterr的值
 */
INT32 mmGetOptErr()
{
    return opterr;
}

/*
 * 描述:设置变量opterr的值
 * mmOptErr：设置的值
 */
VOID mmSetOptErr(INT32 mmOptErr)
{
    opterr = mmOptErr;
}

/*
 * 描述:获取变量optind的值
 * 返回值：获取到optind的值
 */
INT32 mmGetOptInd()
{
    return optind;
}

/*
 * 描述:设置变量optind的值
 * mmOptInd：设置的值
 */
VOID mmSetOptInd(INT32 mmOptInd)
{
    optind = mmOptInd;
}

/*
 * 描述:获取变量optopt的值
 * 返回值：获取到optopt的值
 */
INT32 mmGetOptOpt()
{
    return optopt;
}

/*
 * 描述:设置变量optopt的值
 * mmOptOpt：设置的值
 */
VOID mmSetOpOpt(INT32 mmOptOpt)
{
    optopt = mmOptOpt;
}

/*
* 描述:获取变量optarg的值
* 返回值：获取到optarg的指针
*/
CHAR *mmGetOptArg()
{
    return optarg;
}

/*
 * 描述:设置变量optarg的值
 * mmmOptArg：要设置的指针
 */
VOID mmSetOptArg(CHAR *mmOptArg)
{
    optarg = mmOptArg;
}

/*
 * 描述:分析命令行参数
 * 参数:argc--传递的参数个数
 *      argv--传递的参数内容
 *      opts--用来指定可以处理哪些选项
 * 返回值:执行错误, 找不到选项元素, 返回EN_ERROR
 */
INT32 mmGetOpt(INT32 argc, CHAR * const *argv, const CHAR *opts)
{
    if (opts == nullptr) {
        return EN_ERROR;
    } else {
        UINT32 flags = 0;
        LocalDisableExten(opts, flags);
        return (LocalGetOptInternal(argc, argv, opts, nullptr, nullptr, flags));
    }
}

/*
 * 描述:分析命令行参数-长参数
 * 参数:argc--传递的参数个数
 *      argv--传递的参数内容
 *      opts--用来指定可以处理哪些选项
 *      longopts--指向一个mmStructOption的数组
 *      longindex--表示长选项在longopts中的位置
 * 返回值:执行错误, 找不到选项元素, 返回EN_ERROR
 */
INT32 mmGetOptLong(INT32 argc, CHAR * const *argv, const CHAR *opts,
                   const mmStructOption *longopts, INT32 *longindex)
{
    if (opts == nullptr) {
        return EN_ERROR;
    } else {
        UINT32 flags = MMPA_FLAG_PERMUTE;
        LocalDisableExten(opts, flags);
        return (LocalGetOptInternal(argc, argv, opts, longopts, longindex, flags));
    }
}

/*
 * 描述:控制文件的读写位置
 * 参数:fd打开的文件描述符, 参数offset 为根据参数seekFlag来移动读写位置的位移数
 * 返回值:执行成功返回返回目前的读写位置, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
LONG mmLseek(INT32 fd, INT64 offset, INT32 seekFlag)
{
    if (fd <= MMPA_ZERO) {
        return EN_INVALID_PARAM;
    }
    LONG  pos = _lseek(fd, offset, seekFlag); // 出错返回-1L
    if (pos == -1L) {
        return EN_ERROR;
    }
    return pos;
}

/*
 * 描述:参数fd指定的文件大小改为参数length指定的大小
 * 参数:文件描述符fd
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmFtruncate(mmProcess fd, UINT32 length)
{
    if (fd == nullptr) {
        return EN_INVALID_PARAM;
    }

    DWORD ret = SetFilePointer(fd, length, nullptr, FILE_BEGIN);
    if (ret == INVALID_SET_FILE_POINTER) {
        return EN_ERROR;
    }
    BOOL result = SetEndOfFile(fd);
    if (!result) {
        return EN_ERROR;
    } else {
        return EN_OK;
    }
}

/*
 * 描述:复制一个文件的描述符
 * 参数:oldFd -- 需要复制的fd
 *      newFd -- 新的生成的fd
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmDup2(INT32 oldFd, INT32 newFd)
{
    if (oldFd <= MMPA_ZERO) {
        return EN_INVALID_PARAM;
    }
    return _dup2(oldFd, newFd);
}

/*
 * @brief Creates a second file descriptor for an open file
 * @param [in] fd, an open file handle
 * @returns a new file descripto, failed return EN_ERROR
 */
INT32 mmDup(INT32 fd)
{
    if (fd < MMPA_ZERO) {
        return EN_INVALID_PARAM;
    }
    return _dup(fd);
}

/*
 * 描述:返回指定文件流的文件描述符
 * 参数:stream--FILE类型文件流
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmFileno(FILE *stream)
{
    if (stream == nullptr) {
        return EN_INVALID_PARAM;
    }

    return _fileno(stream);
}

/*
 * 描述:删除一个文件
 * 参数:filename--文件路径
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmUnlink(const CHAR *filename)
{
    if (filename == nullptr) {
        return EN_INVALID_PARAM;
    }

    return _unlink(filename);
}

/*
 * 描述:修改文件读写权限，目前仅支持读写权限修改
 * 参数:filename--文件路径
        mode--需要修改的权限
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmChmod(const CHAR *filename, INT32 mode)
{
    if (filename == nullptr) {
        return EN_INVALID_PARAM;
    }

    return _chmod(filename, mode);
}

/*
 * 描述:线程局部变量存储区创建
 * 参数:destructor--清理函数-linux有效
 *      key-线程变量存储区索引
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmTlsCreate(mmThreadKey *key, void(*destructor)(void*))
{
    if (key == nullptr) {
        return EN_INVALID_PARAM;
    }
    DWORD index = TlsAlloc();
    if (index == TLS_OUT_OF_INDEXES) {
        return EN_ERROR;
    }
    *key = index;
    return EN_OK;
}

/*
 * 描述:线程局部变量设置
 * 参数:key--线程变量存储区索引
        value--需要设置的局部变量的值
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmTlsSet(mmThreadKey key, const VOID *value)
{
    if (value == nullptr) {
        return EN_INVALID_PARAM;
    }
    BOOL ret = TlsSetValue(key, (LPVOID)value);
    if (!ret) {
        return EN_ERROR;
    }
    return EN_OK;
}

/*
 * 描述:线程局部变量获取
 * 参数:key --线程变量存储区索引
 * 返回值:执行成功返回指向局部存储变量的指针, 执行错误返回nullptr
 */
VOID *mmTlsGet(mmThreadKey key)
{
    return TlsGetValue(key);
}

/*
 * 描述:线程局部变量清除
 * 参数:key--线程变量存储区索引
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR
 */
INT32 mmTlsDelete(mmThreadKey key)
{
    BOOL ret = TlsFree(key);
    if (!ret) {
        return EN_ERROR;
    }
    return EN_OK;
}

/*
 * 描述:获取当前操作系统类型
 * 返回值:执行成功返回:LINUX--当前系统是Linux系统
 *                     WIN--当前系统是Windows系统
 */
INT32 mmGetOsType()
{
    return OS_TYPE;
}

/*
 * 描述:将对应文件描述符在内存中的内容写入磁盘
 * 参数:fd--文件句柄
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmFsync(mmProcess fd)
{
    if (fd == INVALID_HANDLE_VALUE) {
        return EN_INVALID_PARAM;
    }
    INT32 ret = FlushFileBuffers(fd);
    if (ret == MMPA_ZERO) {
        return EN_ERROR;
    }
    return EN_OK;
}

/*
 * 描述:将对应文件描述符在内存中的内容写入磁盘
 * 参数:fd--文件描述符
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmFsync2(INT32 fd)
{
    HANDLE handle = (mmProcess)_get_osfhandle(fd);
    INT32 ret = FlushFileBuffers(handle);
    if (ret == MMPA_ZERO) {
        return EN_ERROR;
    }
    return EN_OK;
}

/*
 * 描述:改变当前工作目录
 * 参数:path--需要切换到的工作目录
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmChdir(const CHAR *path)
{
    if (path == nullptr) {
        return EN_INVALID_PARAM;
    }

    return _chdir(path);
}

/*
 * 描述:为每一个进程设置文件模式创建屏蔽字，并返回之前的值
 * 参数:pmode--需要修改的权限值
 * 返回值:执行成功返回先前的pmode的值
 */
INT32 mmUmask(INT32 pmode)
{
    return _umask(pmode);
}

/*
 * 描述:等待子进程结束并返回对应结束码
 * 参数:pid--欲等待的子进程ID
 *      status--保存子进程的状态信息
 *      options--options提供了一些另外的选项来控制waitpid()函数的行为
 *               M_WAIT_NOHANG--如果pid指定的子进程没有结束，则立即返回;如果结束了, 则返回该子进程的进程号
 *               M_WAIT_UNTRACED--如果子进程进入暂停状态，则马上返回
 * 返回值:子进程未结束返回EN_OK, 系统调用执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 *        进程已经结束返回EN_ERR
 */
INT32 mmWaitPid(mmProcess pid, INT32 *status, INT32 options)
{
    if (options != MMPA_ZERO && options != M_WAIT_NOHANG && options != M_WAIT_UNTRACED) {
        return EN_INVALID_PARAM;
    }

    DWORD  dwMilliseconds;
    if (options == M_WAIT_NOHANG || options == M_WAIT_UNTRACED) {
        dwMilliseconds = 0; // 不等待立即返回
    } else {
        dwMilliseconds = INFINITE;
    }

    DWORD ret = WaitForSingleObject(pid, dwMilliseconds);
    if (ret == WAIT_OBJECT_0) {
        if (status != nullptr) {
            GetExitCodeProcess(pid, (LPDWORD)status); // 按需获取退出码
        }
        return EN_ERR; // 进程已结束
    } else if (ret == WAIT_FAILED) {
        return EN_ERROR; // 调用异常
    } else {
        return EN_OK; // 进程未结束
    }
}

/*
 * 描述:获取当前工作目录路径
 * 参数:buffer--由用户分配用来存放工作目录路径的缓存
 *      maxLen--缓存长度
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmGetCwd(CHAR *buffer, INT32 maxLen)
{
    if (buffer == nullptr || maxLen <= MMPA_ZERO) {
        return EN_INVALID_PARAM;
    }
    CHAR *ptr = _getcwd(buffer, maxLen);
    if (ptr != nullptr) {
        return EN_OK;
    } else {
        return EN_ERROR;
    }
}

/*
 * 描述:字符串截取
 * 参数:str--待截取字符串
 *      delim --分隔符
 *      saveptr -- 一个供内部使用的指针，用于保存上次分割剩下的字串
 * 返回值:执行成功返回截取后的字符串指针, 执行失败返回nullptr
 */
CHAR *mmStrTokR(CHAR *str, const CHAR *delim, CHAR **saveptr)
{
    if (delim == nullptr) {
        return nullptr;
    }
    return strtok_s(str, delim, saveptr);
}

/*
 * 描述:获取环境变量
 * 参数:name --需要获取的环境变量名
 *      len --缓存长度,
 *      value -- 由用户分配用来存放环境变量的缓存
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmGetEnv(const CHAR *name, CHAR *value, UINT32 len)
{
    if (name == nullptr || value == nullptr || len == MMPA_ZERO) {
        return EN_INVALID_PARAM;
    }
    DWORD ret = GetEnvironmentVariable(name, value, len);
    if (ret == MMPA_ZERO || ret > (len - 1)) {
        return EN_ERROR;
    }

    return EN_OK;
}

/*
 * 描述:设置环境变量
 * 参数:name --需要获取的环境变量名
 *      value -- 由用户分配用来存放环境变量的缓存
 *      overwrite -- 是否覆盖标志位 0 表示不覆盖
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmSetEnv(const CHAR *name, const CHAR *value, INT32 overwrite)
{
    if (name == nullptr || value == nullptr) {
        return EN_INVALID_PARAM;
    }
    BOOL ret = FALSE;
    if (overwrite != MMPA_ZERO) {
        ret = SetEnvironmentVariable(name, value);
        if (!ret) {
            return EN_ERROR;
        }
    } else {
        // 不覆盖设置,先获取是否存在该name的环境变量，已存在就忽略，不存在就设置下去
        CHAR env[MMPA_MAX_BUF_SIZE] = { 0 };
        DWORD result = GetEnvironmentVariable(name, env, MMPA_MAX_BUF_SIZE);
        if (result == MMPA_ZERO && GetLastError() == ERROR_ENVVAR_NOT_FOUND) {
            ret = SetEnvironmentVariable(name, value);
            if (!ret) {
                return EN_ERROR;
            }
        }
    }
    return EN_OK;
}

/*
 * 描述:内部使用，去除行尾反斜杠符及斜杠符
 * 参数:profilePath--路径，会修改profilePath的值
 * 返回值:执行成功返回指向去除行尾后的源指针字符串
 */
static LPTSTR LocalDeleteBlackslash(LPTSTR profilePath)
{
    DWORD pathLength = strlen(profilePath);
    INT32 i = 0;
    for (i = pathLength - 1; i > 0; i--) {
        if (profilePath[i] == TEXT('\\') || profilePath[i] == TEXT('/')) {
            profilePath[i] = (TCHAR)0;
        } else {
            break;
        }
    }
    return profilePath;
}


/*
 * 描述:截取目录, windows路径格式为 d:\\usr\\bin\\test, 截取后为 d:\\usr\\bin
 * 参数:path--路径，函数内部会修改path的值
 * 返回值:执行成功返回指向截取到的目录部分指针，执行失败返回nullptr
 */
CHAR *mmDirName(CHAR *path)
{
    if (path == nullptr) {
        return nullptr;
    }

    CHAR dirName[MAX_PATH] = {};
    CHAR drive[MAX_PATH] = {};
    INT32 pathLength = strlen(path);

    // 去除行尾反斜杠符及斜杠符
    LocalDeleteBlackslash(path);
    // 获取目录后在获取盘符
    _splitpath(path, nullptr, dirName, nullptr, nullptr);
    _splitpath(path, drive, nullptr, nullptr, nullptr);

    // 将目录拼接到盘符下
    INT32 ret = strcat_s(drive, MAX_PATH, dirName);
    if (ret != MMPA_ZERO) {
        return nullptr;
    }
    // 将包含结束符的获取的路径赋值给path
    ret = memcpy_s(path, pathLength, drive, strlen(drive) + 1);
    if (ret != MMPA_ZERO) {
        return nullptr;
    }
    // 去除行尾反斜杠符及斜杠符
    LocalDeleteBlackslash(path);
    return path;
}

/*
 * 描述:截取目录后面的部分, 比如d:\\usr\\bin\\test, 截取后为test
 * 参数:path--路径，函数内部会修改path的值(行尾有\\会去掉)
 * 返回值:执行成功返回指向截取到的目录部分指针，执行失败返回nullptr
 */
CHAR *mmBaseName(CHAR *path)
{
    if (path == nullptr) {
        return nullptr;
    }
    // 去除行尾反斜杠符及斜杠符
    LocalDeleteBlackslash(path);
    CHAR fileName[MAX_PATH] = {};
    CHAR *tmp = path;
    INT32 i = 0;
    _splitpath(path, nullptr, nullptr, fileName, nullptr);
    INT32 fileNameLength = strlen(fileName);
    if (fileNameLength == MMPA_ZERO) {
        return path;
    }
    INT32 pathLength = strlen(path);
    // 防止有目录与文件名有重名部分,逆序匹配
    tmp += pathLength - fileNameLength;
    for (i = 0; i < pathLength - fileNameLength; i++) {
        if (strncmp(tmp, fileName, fileNameLength) == MMPA_ZERO) {
            return tmp;
        } else {
            tmp--;
        }
    }

    return path;
}

/*
 * 描述:获取当前指定路径下磁盘容量及可用空间
 * 参数:path--路径名
 *      diskSize--mmDiskSize结构内容
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmGetDiskFreeSpace(const CHAR *path, mmDiskSize *diskSize)
{
    ULONGLONG freeBytes;
    ULONGLONG freeBytesToCaller;
    ULONGLONG totalBytes;
    if (path == nullptr || diskSize == nullptr) {
        return EN_INVALID_PARAM;
    }
    BOOL ret = GetDiskFreeSpaceEx(path, (PULARGE_INTEGER)&freeBytesToCaller,
        (PULARGE_INTEGER)&totalBytes, (PULARGE_INTEGER)&freeBytes);
    if (ret) {
        diskSize->availSize = (ULONGLONG)freeBytesToCaller;
        diskSize->totalSize = (ULONGLONG)totalBytes;
        diskSize->freeSize = (ULONGLONG)freeBytes;
        return EN_OK;
    }
    return EN_ERROR;
}

/*
 * 描述:设置mmCreateTask创建的线程名-线程体外调用
 * 参数:threadHandle--线程ID
 *      name--线程名, name的实际长度必须<MMPA_THREADNAME_SIZE
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmSetThreadName(mmThread *threadHandle, const CHAR *name)
{
    if ((threadHandle == NULL) || (name == NULL)) {
        return EN_INVALID_PARAM;
    }
    PCWSTR threadName = reinterpret_cast<PCWSTR>(const_cast<CHAR *>(name));
    HRESULT ret = SetThreadDescription(*threadHandle, threadName);
    if (FAILED(ret)) {
        return EN_ERROR;
    }
    return EN_OK;
}

/*
 * 描述:获取线程名-线程体外调用, 仅在Linux下生效, Windows下不支持为空实现
 * 参数:threadHandle--线程ID
 *      size--存放线程名的缓存长度
 *      name--线程名由用户分配缓存, 缓存长度必须>=MMPA_THREADNAME_SIZE
 * 返回值:windows下默认返回EN_ERROR
 */
INT32 mmGetThreadName(mmThread *threadHandle, CHAR *name, INT32 size)
{
    return EN_ERROR;
}

/*
 * 描述:设置当前执行的线程的线程名-线程体内调用, 仅在Linux下生效, Windows下不支持为空实现
 * 参数:name--需要设置的线程名
 * 返回值:windows下默认返回EN_ERROR
 */
INT32 mmSetCurrentThreadName(const CHAR *name)
{
    return EN_ERROR;
}

/*
 * 描述:获取当前执行的线程的线程名-线程体内调用, 仅在Linux下生效, Windows下不支持为空实现
 * 参数:name--获取到的线程名, 缓存空间由用户分配
 *      size--缓存长度必须>=MMPA_THREADNAME_SIZE
 * 返回值:windows下默认返回EN_ERROR
 */
INT32 mmGetCurrentThreadName(CHAR *name, INT32 size)
{
    return EN_ERROR;
}

/*
 * 描述:获取当前指定路径下文件大小
 * 参数:fileName--文件路径名
 *      length--获取到的文件大小
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmGetFileSize(const CHAR *fileName, ULONGLONG *length)
{
    if (fileName == nullptr || length == nullptr) {
        return EN_INVALID_PARAM;
    }

    HANDLE hFile = CreateFile((LPCSTR)fileName, GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE, // 默认文件对于多进程都是共享读写的
        nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
        return EN_ERROR;
    }

    BOOL ret = GetFileSizeEx(hFile, (PLARGE_INTEGER)length);
    (void)CloseHandle(hFile);
    if (!ret) {
        return EN_ERROR;
    }
    return EN_OK;
}

/*
 * 描述:判断是否是目录
 * 参数:fileName -- 文件路径名
 * 返回值:执行成功返回EN_OK(是目录), 执行错误返回EN_ERROR(不是目录), 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmIsDir(const CHAR *fileName)
{
    if (fileName == nullptr) {
        return EN_INVALID_PARAM;
    }
    DWORD ret = GetFileAttributes((LPCSTR)fileName);
    if (ret != FILE_ATTRIBUTE_DIRECTORY) {
        return EN_ERROR;
    }
    return EN_OK;
}

/*
 * 描述:获取系统名字
 * 参数:nameSize--存放系统名的缓存长度
 *      name--由用户分配缓存, 缓存长度必须>=MMPA_MIN_OS_NAME_SIZE
 * 返回值:入参错误返回EN_INVALID_PARAM, 返回值:执行成功返回EN_OK, 执行失败返回EN_ERROR
 */
INT32 mmGetOsName(CHAR *name, INT32 nameSize)
{
    if ((name == nullptr) || (nameSize < MMPA_MIN_OS_NAME_SIZE)) {
        return EN_INVALID_PARAM;
    }
    DWORD nameLength = nameSize;
    BOOL ret = GetComputerName(name, &nameLength);
    if (!ret) {
        return EN_ERROR;
    }
    return EN_OK;
}

/*
 * 描述:内部调用 -根据versionIndex获取系统版本信息
 * 参数:versionLength--存放操作系统信息的缓存长度
 *      versionInfo--由用户分配缓存, 缓存长度必须>=MMPA_MIN_OS_VERSION_SIZE
 *      versionIndex--操作系统内部编号
 * 返回值:执行成功返回EN_OK, 执行失败返回EN_ERROR
 */
INT32 LocalGetVersionInfo(CHAR *versionInfo, INT32 versionLength, INT32 versionIndex)
{
    INT32 ret = _snprintf_s(versionInfo, versionLength - 1, MMPA_MIN_OS_VERSION_SIZE, "%s", g_winOps[versionIndex]);
    if (ret < MMPA_ZERO) {
        return EN_ERROR;
    }
    return EN_OK;
}

/*
 * 描述:获取当前操作系统版本信息
 * 参数:versionLength--存放操作系统信息的缓存长度
 *      versionInfo--由用户分配缓存, 缓存长度必须>=MMPA_MIN_OS_VERSION_SIZE
 * 返回值:入参错误返回EN_INVALID_PARAM, 返回值:执行成功返回EN_OK, 执行失败返回EN_ERROR
 */
INT32 mmGetOsVersion(CHAR *versionInfo, INT32 versionLength)
{
    if (versionInfo == nullptr || versionLength < MMPA_MIN_OS_VERSION_SIZE) {
        return EN_INVALID_PARAM;
    }
    INT32 ret = 0;
    if (!IsWindowsXPOrGreater()) {
        return LocalGetVersionInfo(versionInfo, versionLength, MMPA_OS_VERSION_WINDOWS_2000);
    }
    if (!IsWindowsXPSP1OrGreater()) {
        return LocalGetVersionInfo(versionInfo, versionLength, MMPA_OS_VERSION_WINDOWS_XP);
    }
    if (!IsWindowsXPSP2OrGreater()) {
        if (IsWindowsServer) {
            return LocalGetVersionInfo(versionInfo, versionLength, MMPA_OS_VERSION_WINDOWS_SERVER_2003);
        }
        return LocalGetVersionInfo(versionInfo, versionLength, MMPA_OS_VERSION_WINDOWS_XP_SP1);
    }
    if (!IsWindowsXPSP3OrGreater()) {
        return LocalGetVersionInfo(versionInfo, versionLength, MMPA_OS_VERSION_WINDOWS_HOME_SERVER);
    }
    if (!IsWindowsVistaOrGreater()) {
        return LocalGetVersionInfo(versionInfo, versionLength, MMPA_OS_VERSION_WINDOWS_XP_SP1);
    }
    if (!IsWindows7SP1OrGreater()) {
        return LocalGetVersionInfo(versionInfo, versionLength, MMPA_OS_VERSION_WINDOWS_VISTA);
    }
    if (!IsWindows8OrGreater()) {
        return LocalGetVersionInfo(versionInfo, versionLength, MMPA_OS_VERSION_WINDOWS_7);
    }
    if (!IsWindows8Point1OrGreater()) {
        return LocalGetVersionInfo(versionInfo, versionLength, MMPA_OS_VERSION_WINDOWS_8);
    }
    if (!IsWindows10OrGreater()) {
        return LocalGetVersionInfo(versionInfo, versionLength, MMPA_OS_VERSION_WINDOWS_8_1);
    }
    return LocalGetVersionInfo(versionInfo, versionLength, MMPA_OS_VERSION_WINDOWS_10);
}

/*
 * 描述:获取当前系统下所有网卡的mac地址列表(不包括lo网卡)
 * 参数:list--获取到的mac地址列表指针数组, 缓存由函数内部分配, 需要调用mmGetMacFree释放
 *      count--获取到的mac地址的个数
 * 返回值:执行成功返回扫描到的EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmGetMac(mmMacInfo **list, INT32 *count)
{
    if (list == nullptr || count == nullptr) {
        return EN_INVALID_PARAM;
    }

    mmMacInfo *pMacInfo = nullptr;
    PIP_ADAPTER_INFO pAdapter = nullptr;
    UINT i = 0;

    ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);
    PIP_ADAPTER_INFO pAdapterInfo = reinterpret_cast<IP_ADAPTER_INFO *>(HeapAlloc(GetProcessHeap(), \
        0, (sizeof(IP_ADAPTER_INFO))));
    if (pAdapterInfo == nullptr) {
        return EN_ERROR;
    }

    // 初始调用确保分配足够空间
    if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) {
        (void)HeapFree(GetProcessHeap(), 0, pAdapterInfo);
        pAdapterInfo = reinterpret_cast<IP_ADAPTER_INFO *>(HeapAlloc(GetProcessHeap(), 0, ulOutBufLen));
        if (pAdapterInfo == nullptr) {
            return EN_ERROR;
        }
    }
    DWORD dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen);
    if (dwRetVal == NO_ERROR) {
        pAdapter = pAdapterInfo;
        *count = ulOutBufLen / sizeof(IP_ADAPTER_INFO);
        pMacInfo = reinterpret_cast<mmMacInfo*>(HeapAlloc(GetProcessHeap(), 0, (*count * sizeof(mmMacInfo))));
        if (pMacInfo == nullptr) {
            (void)HeapFree(GetProcessHeap(), 0, pAdapterInfo);
            return EN_ERROR;
        }
        INT32 cycle = *count;
        while (cycle--) {
            // 0, 1, 2, 3, 4, 5是固定的MAC地址索引
            dwRetVal = _snprintf_s(pMacInfo[i].addr, sizeof(pMacInfo[i].addr)-1, "%02X-%02X-%02X-%02X-%02X-%02X", \
                pAdapter->Address[0], pAdapter->Address[1], pAdapter->Address[2], \
                pAdapter->Address[3], pAdapter->Address[4], pAdapter->Address[5]);
            if (dwRetVal < MMPA_ZERO) {
                (void)HeapFree(GetProcessHeap(), 0, pMacInfo);
                (void)HeapFree(GetProcessHeap(), 0, pAdapterInfo);
                return EN_ERROR;
            }
            pAdapter = pAdapter->Next;
            i++;
        }
    } else {
        (void)HeapFree(GetProcessHeap(), 0, pAdapterInfo);
        return EN_ERROR;
    }
    if (pAdapterInfo) {
        (void)HeapFree(GetProcessHeap(), 0, pAdapterInfo);
    }
    *list = pMacInfo;
    return EN_OK;
}

/*
 * 描述:释放由mmGetMac产生的动态内存
 * 参数:list--获取到的mac地址列表指针数组
 *      count--获取到的mac地址的个数
 * 返回值:执行成功返回扫描到的EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmGetMacFree(mmMacInfo *list, INT32 count)
{
    if (count <= 0 || list == nullptr) {
        return EN_INVALID_PARAM;
    }

    BOOL ret = HeapFree(GetProcessHeap(), 0, list);
    if (!ret) {
        return EN_ERROR;
    }
    list = nullptr;
    return EN_OK;
}

/*
 * 描述:内部使用, 获取当前系统cpu的部分物理硬件信息
 * 参数:pclsObj--对应的WMI服务指针
 *      cpuDesc--需要获取信息的结构体
 * 返回值:执行成功返回EN_OK，执行失败返回EN_ERROR
 */
static INT32 LocalGetProcessorInfo(IWbemClassObject *pclsObj, mmCpuDesc *cpuDesc)
{
    VARIANT vtProp;
    // 制造商
    pclsObj->Get(L"Manufacturer", 0, &vtProp, 0, 0);
    INT32 ret = _snprintf_s(cpuDesc->manufacturer, sizeof(cpuDesc->manufacturer) - 1, "%ws", vtProp.bstrVal);
    if (ret < MMPA_ZERO) {
        return EN_ERROR;
    }

    // 架构 目前最多支持识别10种
    pclsObj->Get(L"Architecture", 0, &vtProp, 0, 0);
    if (vtProp.uiVal < MMPA_MAX_PROCESSOR_ARCHITECTURE_COUNT) {
        ret = _snprintf_s(cpuDesc->arch, sizeof(cpuDesc->arch) - 1, "%s", g_arch[vtProp.uiVal]);
        if (ret < MMPA_ZERO) {
            return EN_ERROR;
        }
    }

    // 版本
    pclsObj->Get(L"Name", 0, &vtProp, 0, 0);
    ret = _snprintf_s(cpuDesc->version, sizeof(cpuDesc->version) - 1, "%ws", vtProp.bstrVal);
    if (ret < MMPA_ZERO) {
        return EN_ERROR;
    }
    // 工作频率
    pclsObj->Get(L"CurrentClockSpeed", 0, &vtProp, 0, 0);
    cpuDesc->frequency = vtProp.uintVal;
    // 最大超频频率
    pclsObj->Get(L"MaxClockSpeed", 0, &vtProp, 0, 0);
    cpuDesc->maxFrequency = vtProp.uintVal;
    // cpu核个数
    pclsObj->Get(L"NumberOfCores", 0, &vtProp, 0, 0);
    cpuDesc->ncores = vtProp.uintVal;
    // 线程个数
    pclsObj->Get(L"ThreadCount", 0, &vtProp, 0, 0);
    cpuDesc->nthreads = vtProp.uintVal;
    // 逻辑cpu个数
    pclsObj->Get(L"NumberOfLogicalProcessors", 0, &vtProp, 0, 0);
    cpuDesc->ncounts = vtProp.uintVal;

    VariantClear(&vtProp);
    pclsObj->Release();
    return EN_OK;
}

/*
* 描述:内部使用, 释放WMI服务申请的资源
*/
static void LocalWMIServiceRelease(IWbemServices *pSvc, IWbemLocator *pLoc, IEnumWbemClassObject *pEnumerator)
{
    if (pSvc != nullptr) {
        pSvc->Release();
        pSvc = nullptr;
    }
    if (pLoc != nullptr) {
        pLoc->Release();
        pLoc = nullptr;
    }
    if (pEnumerator != nullptr) {
        pEnumerator->Release();
        pEnumerator = nullptr;
    }
    CoUninitialize();
    return;
}

/*
* 描述:内部使用, 连接WMI服务并返回Wbem类对象指针
*/
static IEnumWbemClassObject *LocalConnectWMIService(IWbemServices **pSvc, IWbemLocator **pLoc)
{
    IEnumWbemClassObject *pEnumerator = nullptr;
    // Step 1:创建COM对象
    HRESULT hres = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hres)) {
        return nullptr;
    }
    // Step 2:设置COM对象安全等级
    hres = CoInitializeSecurity(nullptr, -1, nullptr, nullptr, RPC_C_AUTHN_LEVEL_DEFAULT,
        RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE, nullptr);
    if (FAILED(hres)) {
        LocalWMIServiceRelease(*pSvc, *pLoc, pEnumerator);
        return nullptr;
    }
    // Step 3:获取WMI初始探测器
    hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER,
        IID_IWbemLocator, reinterpret_cast<LPVOID *>(pLoc));
    if (FAILED(hres)) {
        LocalWMIServiceRelease(*pSvc, *pLoc, pEnumerator);
        return nullptr;
    }
    // Step 4:通过ConnectServer连接到WMI服务 ROOT\\CIMV2
    hres = (*pLoc)->ConnectServer((BSTR)(L"ROOT\\CIMV2"), nullptr, nullptr, 0, 0, 0, 0, pSvc);
    if (FAILED(hres)) {
        LocalWMIServiceRelease(*pSvc, *pLoc, pEnumerator);
        return nullptr;
    }
    // Step 5:设置安全等级
    hres = CoSetProxyBlanket(*pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr, RPC_C_AUTHN_LEVEL_CALL,
        RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE);
    if (FAILED(hres)) {
        LocalWMIServiceRelease(*pSvc, *pLoc, pEnumerator);
        return nullptr;
    }

    hres = (*pSvc)->ExecQuery((BSTR)(L"WQL"), (BSTR)(L"SELECT * FROM Win32_Processor"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr, &pEnumerator);
    if (FAILED(hres)) {
        LocalWMIServiceRelease(*pSvc, *pLoc, pEnumerator);
        return nullptr;
    }

    return pEnumerator;
}

/*
 * 描述:获取当前系统cpu的部分物理硬件信息
 * 参数:cpuInfo--包含需要获取信息的结构体, 函数内部分配, 需要调用mmCpuInfoFree释放
 *      count--读取到的物理cpu个数
 * 返回值:入参错误返回EN_INVALID_PARAM, 执行成功返回EN_OK，执行失败返回EN_ERROR
 */
INT32 mmGetCpuInfo(mmCpuDesc **cpuInfo, INT32 *count)
{
    if (cpuInfo == nullptr || count == nullptr) {
        return EN_INVALID_PARAM;
    }

    IWbemLocator *pLoc = nullptr;
    IWbemServices *pSvc = nullptr;

    IEnumWbemClassObject *pEnumerator = LocalConnectWMIService(&pSvc, &pLoc);
    if (pEnumerator == nullptr) {
        return EN_ERROR;
    }

    // 获取请求的服务的数据
    IWbemClassObject *pclsObj = nullptr;
    ULONG uReturn = 0;
    // 目前最多支持MMPA_MAX_PHYSICALCPU_COUNT
    mmCpuDesc cpuDesc[MMPA_MAX_PHYSICALCPU_COUNT] = {};
    SecureZeroMemory(cpuDesc, sizeof(cpuDesc));
    INT32 i = 0;
    while (pEnumerator != nullptr) {
        pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
        if (uReturn == MMPA_ZERO) {
            break;
        }

        if (LocalGetProcessorInfo(pclsObj, &cpuDesc[i]) == EN_ERROR) {
            *count = 0;
            LocalWMIServiceRelease(pSvc, pLoc, pEnumerator);
            return EN_ERROR;
        }
        if (i < MMPA_MAX_PHYSICALCPU_COUNT - 1) {
            i++;
        } else {
            break;
        }
    }
    if (i == MMPA_ZERO) {
        LocalWMIServiceRelease(pSvc, pLoc, pEnumerator);
        return EN_ERROR;
    }
    *count = i;
    mmCpuDesc *pCpuDesc = reinterpret_cast<mmCpuDesc *>(HeapAlloc(GetProcessHeap(), 0, i * (sizeof(mmCpuDesc))));
    if (pCpuDesc == nullptr) {
        LocalWMIServiceRelease(pSvc, pLoc, pEnumerator);
        return EN_ERROR;
    }
    for (i = 0; i < *count; i++) {
        pCpuDesc[i] = cpuDesc[i];
    }
    *cpuInfo = pCpuDesc;
    // 释放资源
    LocalWMIServiceRelease(pSvc, pLoc, pEnumerator);
    return EN_OK;
}

/*
 * 描述:释放mmGetCpuInfo生成的动态内存
 * 参数:cpuInfo--mmGetCpuInfo获取到的信息的结构体指针
 *      count--mmGetCpuInfo获取到的物理cpu个数
 * 返回值:入参错误返回EN_INVALID_PARAM, 执行成功返回EN_OK，执行失败返回EN_ERROR
 */
INT32 mmCpuInfoFree(mmCpuDesc *cpuInfo, INT32 count)
{
    if (cpuInfo == nullptr || count == MMPA_ZERO) {
        return EN_INVALID_PARAM;
    }
    BOOL ret = HeapFree(GetProcessHeap(), 0, cpuInfo);
    if (!ret) {
        return EN_ERROR;
    }
    return EN_OK;
}

/*
* 描述:内部使用
*/
static INT32 LocalGetArgsAndEnvpLen(const mmArgvEnv *env, size_t *argvLen, size_t *envpLen)
{
    if (env == nullptr) {
        return EN_OK;
    }
    INT32 i = 0;
    for (i = 0; i < env->argvCount; ++i) {
        if (env->argv[i] == nullptr) {
            continue;
        }
        if (CheckSizetAddOverflow(*argvLen, strlen(env->argv[i]) + 1) != EN_OK) {
            return EN_ERROR;
        }
        *argvLen += strlen(env->argv[i]) + 1; // an extra space is required
    }
    if (env->argvCount > 0) {
        *argvLen += 1;
    }
    // envp要求格式 name1=value1\0name2=value2\0...namex=valuex\0\0 双\0结尾
    for (i = 0; i < env->envpCount; ++i) {
        if (env->envp[i] == nullptr) {
            continue;
        }
        if (CheckSizetAddOverflow(*envpLen, strlen(env->envp[i]) + 1) != EN_OK) {
            return EN_ERROR;
        }
        *envpLen += strlen(env->envp[i]) + 1;
    }
    if (env->envpCount > 0) {
        *envpLen += 1;
    }
    return EN_OK;
}
/*
* 描述:内部使用，将传入的argvs和envps字符串数组组合成一个argv和envp
* 返回值:执行成功返回EN_OK, 执行错误返回EN_INVALID_PARAM
*/
INT32 LocalGetArgsAndEnvp(CHAR *argv, INT32 argvLen, CHAR *envp, INT32 envpLen, const mmArgvEnv *env)
{
    INT32 i = 0;
    INT32 ret = 0;
    CHAR *destEnvp = envp;
    // 将argv和envp字符串数组列表拼接成一个字符串命令
    if (env == nullptr) {
        return EN_OK;
    }

    // envp要求格式 name1=value1\0name2=value2\0...namex=valuex\0\0 双\0结尾
    if (destEnvp != nullptr) {
        for (i = 0; i < env->envpCount; ++i) {
            if (env->envp[i] == nullptr) {
                continue;
            }
            size_t len = strlen(env->envp[i]);
            ret = strncpy_s(destEnvp, envpLen, env->envp[i], len + 1);
            if (ret != MMPA_ZERO) {
                return EN_INVALID_PARAM;
            }
            destEnvp += len + 1;
            envpLen -= len + 1;
        }
        *destEnvp = '\0';
    }

    if (argv == nullptr) {
        return EN_OK;
    }

    for (i = 0; i < env->argvCount; ++i) {
        if (env->argv[i] == nullptr) {
            continue;
        }
        ret = strcat_s(argv, argvLen, env->argv[i]);
        if (ret != MMPA_ZERO) {
            return EN_INVALID_PARAM;
        }
        if (i < env->argvCount - 1) {
            ret = strcat_s(argv, argvLen, " ");
            if (ret != MMPA_ZERO) {
                return EN_INVALID_PARAM;
            }
        }
    }
    return EN_OK;
}

/*
 * 描述:新创建进程执行可执行程序或者指定命令或者重定向输出文件
 * 参数:fileName--需要执行的可执行程序所在路径名
 *      env--保存子进程的状态信息
 *      stdoutRedirectFile--重定向文件路径, 若不为空, 则启用重定向功能
 *      id--创建的子进程ID号
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmCreateProcess(const CHAR *fileName, const mmArgvEnv *env, const CHAR *stdoutRedirectFile, mmProcess *id)
{
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    SecureZeroMemory(&si, sizeof(si));
    SecureZeroMemory(&pi, sizeof(pi));
    BOOL bInheritHandles = FALSE;
    SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};
    HANDLE cmdOutput = nullptr;
    if (stdoutRedirectFile != nullptr) {
        cmdOutput = CreateFile(stdoutRedirectFile,
            GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            &sa, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (cmdOutput == INVALID_HANDLE_VALUE) {
            return EN_ERROR;
        }
        si.hStdOutput = cmdOutput;
        si.dwFlags = STARTF_USESTDHANDLES;
        bInheritHandles = true;
    }

    size_t argvLen = 0;
    size_t envpLen = 0;
    if (LocalGetArgsAndEnvpLen(env, &argvLen, &envpLen) != EN_OK) {
        MMPA_CLOSE_FILE_HANDLE(cmdOutput);
        return EN_ERROR;
    }
    CHAR *command = nullptr;
    CHAR *environment = nullptr;
    if (argvLen > 0) {
        command = reinterpret_cast<CHAR *>(malloc(argvLen));
        if (command == nullptr) {
            MMPA_CLOSE_FILE_HANDLE(cmdOutput);
            return EN_ERROR;
        }
        SecureZeroMemory(command, argvLen);
    }

    if (envpLen > 0) {
        environment = reinterpret_cast<CHAR *>(malloc(envpLen));
        if (environment == nullptr) {
            MMPA_CLOSE_FILE_HANDLE(cmdOutput);
            MMPA_FREE(command);
            return EN_ERROR;
        }
        SecureZeroMemory(environment, envpLen);
    }

    INT32 ret = LocalGetArgsAndEnvp(command, argvLen, environment, envpLen, env);
    if (ret != MMPA_ZERO) {
        MMPA_CLOSE_FILE_HANDLE(cmdOutput);
        MMPA_FREE(command);
        MMPA_FREE(environment);
        return ret;
    }
    BOOL bRet = CreateProcess(fileName, (LPTSTR)command, nullptr, nullptr,
        bInheritHandles, 0, (LPVOID)environment, nullptr, &si, &pi);
    MMPA_CLOSE_FILE_HANDLE(cmdOutput);
    MMPA_FREE(command);
    MMPA_FREE(environment);
    if (bRet) { // 创建成功
        *id = pi.hProcess;
        return EN_OK;
    } else {
        return EN_ERROR;
    }
}

/*
 * 描述:创建带属性的线程, 支持调度策略、优先级、分离属性设置,除了分离属性和线程栈大小,其他只在Linux下有效
 * 参数: threadHandle -- pthread_t类型的实例
 *       funcBlock --包含函数名和参数的结构体指针
 *       threadAttr -- 包含需要设置的线程属性类别和值
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmCreateTaskWithThreadAttr(mmThread *threadHandle,
                                 const mmUserBlock_t *funcBlock,
                                 const mmThreadAttr *threadAttr)

{
    INT32 ret = EN_OK;
    DWORD threadId;

    if ((threadHandle == nullptr) || (funcBlock == nullptr) || (funcBlock->procFunc == nullptr)) {
        return EN_INVALID_PARAM;
    }
    if (threadAttr->stackFlag) {
        if (threadAttr->stackSize < MMPA_THREAD_MIN_STACK_SIZE) {
            return EN_INVALID_PARAM;
        }
        *threadHandle = CreateThread(nullptr, threadAttr->stackSize,
            LocalThreadProc, (LPVOID)funcBlock, MMPA_ZERO, &threadId);
    } else {
        *threadHandle = CreateThread(nullptr, MMPA_ZERO, LocalThreadProc, (LPVOID)funcBlock, MMPA_ZERO, &threadId);
    }
    (void)WaitForSingleObject(*threadHandle, 100); // 等待时间为100ms，确保funcBlock->procFunc线程体被执行
    if ((threadAttr != nullptr) && (threadAttr->detachFlag == TRUE)) {
        (void)CloseHandle(*threadHandle);
    }

    if (*threadHandle == nullptr) {
        ret = EN_ERROR;
    }
    return ret;
}

/*
* 描述：创建或者打开共享内存文件
* 参数：name- 要打开或者创建的共享内存文件名，linux：打开的文件都是位于/dev/shm目录的，因此name不能带路径；windows：需要带路径
*       oflag：打开的文件操作属性
*       mode：共享模式
* 返回值：成功返回创建或者打开的文件句柄，执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
*/
mmFileHandle mmShmOpen(const CHAR *name, INT32 oflag, mmMode_t mode)
{
    if (name == NULL) {
        return (mmFileHandle)EN_INVALID_PARAM;
    }
    mmFileHandle handle = CreateFile(name,
                                     oflag,
                                     FILE_SHARE_READ | FILE_SHARE_WRITE,
                                     NULL,
                                     OPEN_ALWAYS,
                                     FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
                                     NULL);
    if (handle == INVALID_HANDLE_VALUE) {
        return (mmFileHandle)EN_ERROR;
    }
    return handle;
}

/*
 * 描述:删除mmShmOpen创建的文件
 * 参数:name--文件路径
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmShmUnlink(const CHAR *name)
{
    if (name == nullptr) {
        return EN_INVALID_PARAM;
    }

    return _unlink(name);
}


/*
* 描述： 将一个文件或者其它对象映射进内存
* 参数： fd--有效的文件句柄
*        size--映射区的长度
*        offeet--被映射对象内容的起点
*        extra--(出参)CreateFileMapping()返回的文件映像对象句柄，此参数linux不使用，windows为mmMumMap入参，释放资源
*        prot--期望的内存保护标志，不能与文件的打开模式冲突，此参数linux使用，windows不使用
*        flags--指定映射对象的类型，映射选项和映射页是否可以共享，此参数linux使用，windows不使用
* 返回值：成功执行时，返回被映射区的指针；失败返回nullptr
*/
VOID *mmMmap(mmFd_t fd, mmSize_t size, mmOfft_t offset, mmFd_t *extra, INT32 prot, INT32 flags)
{
    if ((size == 0) || (extra == nullptr)) {
        return nullptr;
    }
    // The ">> 32" is a high-order DWORD of the file offset where the view begins
    *extra = CreateFileMapping(fd, nullptr, PAGE_READWRITE,
                               (DWORD) ((UINT64) size >> 32),
                               (DWORD) (size & 0xffffffff),
                               nullptr);
    if (*extra == nullptr) {
        return nullptr;
    }

    // The ">> 32" is a high-order DWORD of the file offset where the view begins
    VOID *data = MapViewOfFile(*extra,
                               FILE_MAP_ALL_ACCESS,
                               (DWORD) ((UINT64) offset >> 32),
                               (DWORD) (offset & 0xffffffff),
                               size);
    if (data == nullptr) {
        CloseHandle(*extra);
    }

    return data;
}

/*
* 描述：取消已映射内存的地址
* 参数： data--需要取消映射的内存地址
*        size--欲取消的内存大小
*        extra--CreateFileMapping()返回的文件映像对象句柄，此参数linux不使用，windows为释放资源参数
* 返回值：执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
*/
INT32 mmMunMap(VOID *data, mmSize_t size, mmFd_t *extra)
{
    if ((data == nullptr) || (extra == nullptr)) {
        return EN_INVALID_PARAM;
    }
    if (UnmapViewOfFile(data) == 0) {
        (void)CloseHandle(*extra);
        return EN_ERROR;
    }

    if (CloseHandle(*extra) == 0) {
        return EN_ERROR;
    }
    return EN_OK;
}

/*
* 描述： 获取系统内存页大小
* 返回值：返系统回内存页大小
*/
mmSize mmGetPageSize()
{
    SYSTEM_INFO systemInfo = { 0 };
    GetSystemInfo(&systemInfo);
    return static_cast<mmSize>(systemInfo.dwPageSize);
}

/*
* 描述： 申请和内存页对齐的内存
* 参数： mallocSize--申请内存大小
         alignSize--内存对齐的大小
* 返回值：成功执行时，返回内存地址；失败返回NULL
*/
VOID *mmAlignMalloc(mmSize mallocSize, mmSize alignSize)
{
    return _aligned_malloc(mallocSize, alignSize);
}

/*
* 描述： 释放内存页对齐的内存
* 参数： addr--内存地址
*/
VOID mmAlignFree(VOID *addr)
{
    if (addr != NULL) {
        _aligned_free(addr);
        addr = NULL;
    }
}

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cpluscplus */
#endif /* __cpluscplus */

