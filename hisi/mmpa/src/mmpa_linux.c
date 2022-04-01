/*
 * @file mmpa_linux.c
 *
 * Copyright (C) Huawei Technologies Co., Ltd. 2018-2019. All Rights Reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Description:MMPA库 Linux部分接口实现
 * Author: Huawei Technologies
 * Create:2017-12-22
 */

#include "mmpa_api.h"

#ifdef __cplusplus
#if    __cplusplus
extern "C" {
#endif /* __cpluscplus */
#endif /* __cpluscplus */

#define MMPA_CPUINFO_DEFAULT_SIZE               64
#define MMPA_CPUINFO_DOUBLE_SIZE                128
#define MMPA_CPUPROC_BUF_SIZE                   256
#define MMPA_MAX_IF_SIZE                        2048
#define MMPA_SECOND_TO_MSEC                     1000
#define MMPA_MSEC_TO_USEC                       1000
#define MMPA_MSEC_TO_NSEC                       1000000
#define MMPA_SECOND_TO_NSEC                     1000000000
#define MMPA_MAX_SLEEP_MILLSECOND_USING_USLEEP  1000
#define MMPA_MAX_SLEEP_MICROSECOND_USING_USLEEP 1000000
#define MMPA_MAX_PHYSICALCPU_COUNT              4096
#define MMPA_MIN_PHYSICALCPU_COUNT              1


struct CpuTypeTable {
    const CHAR *key;
    const CHAR *value;
};

enum {
    MMPA_MAC_ADDR_FIRST_BYTE = 0,
    MMPA_MAC_ADDR_SECOND_BYTE,
    MMPA_MAC_ADDR_THIRD_BYTE,
    MMPA_MAC_ADDR_FOURTH_BYTE,
    MMPA_MAC_ADDR_FIFTH_BYTE,
    MMPA_MAC_ADDR_SIXTH_BYTE
};

/*
 * 描述:用默认的属性创建线程
 * 参数: threadHandle -- pthread_t类型的实例
 *       funcBlock --包含函数名和参数的结构体指针
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmCreateTask(mmThread *threadHandle, mmUserBlock_t *funcBlock)
{
    if ((threadHandle == NULL) || (funcBlock == NULL) || (funcBlock->procFunc == NULL)) {
        return EN_INVALID_PARAM;
    }

    INT32 ret = pthread_create(threadHandle, NULL, funcBlock->procFunc, funcBlock->pulArg);
    if (ret != EN_OK) {
        ret = EN_ERROR;
    }
    return ret;
}

/*
 * 描述:该函数等待线程结束并返回线程退出值"value_ptr"
 *       如果线程成功结束会detach线程
 *       注意:detached的线程不能用该函数或取消
 * 参数: threadHandle-- pthread_t类型的实例
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmJoinTask(mmThread *threadHandle)
{
    if (threadHandle == NULL) {
        return EN_INVALID_PARAM;
    }

    INT32 ret = pthread_join(*threadHandle, NULL);
    if (ret != EN_OK) {
        ret = EN_ERROR;
    }
    return ret;
}

/*
 * 描述:用默认属性初始化锁
 * 参数: mutex --指向mmMutex_t 指针
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmMutexInit(mmMutex_t *mutex)
{
    if (mutex == NULL) {
        return EN_INVALID_PARAM;
    }

    INT32 ret = pthread_mutex_init(mutex, NULL);
    if (ret != EN_OK) {
        ret = EN_ERROR;
    }

    return ret;
}

/*
 * 描述:把mutex锁住
 * 参数: mutex --指向mmMutex_t 指针
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
/*lint -e454*/
INT32 mmMutexLock(mmMutex_t *mutex)
{
    if (mutex == NULL) {
        return EN_INVALID_PARAM;
    }

    INT32 ret = pthread_mutex_lock(mutex);
    if (ret != EN_OK) {
        ret = EN_ERROR;
    }
    return ret;
}

/*
 * 描述:把mutex锁住
 * 参数: mutex --指向mmMutex_t 指针
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmMutexTryLock(mmMutex_t *mutex)
{
    if (mutex == NULL) {
        return EN_INVALID_PARAM;
    }

    INT32 ret = pthread_mutex_trylock(mutex);
    if (ret != EN_OK) {
        ret = EN_ERROR;
    }
    return ret;
}

/*lint +e454*/
/*
 * 描述:把mutex锁解锁
 * 参数: mutex--指向mmMutex_t 指针
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
/*lint -e455*/
INT32 mmMutexUnLock(mmMutex_t *mutex)
{
    if (mutex == NULL) {
        return EN_INVALID_PARAM;
    }

    INT32 ret = pthread_mutex_unlock(mutex);
    if (ret != EN_OK) {
        ret = EN_ERROR;
    }
    return ret;
}
/*lint +e455*/
/*
 * 描述:把mutex锁删除
 * 参数: mutex--指向mmMutex_t 指针
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmMutexDestroy(mmMutex_t *mutex)
{
    if (mutex == NULL) {
        return EN_INVALID_PARAM;
    }

    INT32 ret = pthread_mutex_destroy(mutex);
    if (ret != EN_OK) {
        ret = EN_ERROR;
    }
    return ret;
}

/*
 * 描述:初始化条件变量，条件变量属性为NULL
 * 参数: cond -- 指向mmCond的指针
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmCondInit(mmCond *cond)
{
    if (cond == NULL) {
        return EN_INVALID_PARAM;
    }
    pthread_condattr_t condAttr;
    // 初始化cond 属性
    INT32 ret = pthread_condattr_init(&condAttr);
    if (ret != EN_OK) {
        return EN_ERROR;
    }
    // 为了使 mmCondTimedWait 使用开机时间而不是系统当前时间
    ret = pthread_condattr_setclock(&condAttr, CLOCK_MONOTONIC);
    if (ret != EN_OK) {
        (VOID)pthread_condattr_destroy(&condAttr);
        return EN_ERROR;
    }

    // 使用带属性的初始化
    ret = pthread_cond_init(cond, &condAttr);
    if (ret != EN_OK) {
        ret = EN_ERROR;
    }
    (VOID)pthread_condattr_destroy(&condAttr);

    return ret;
}

/*
 * 描述:用默认属性初始化锁
 * 参数: mutex -- 指向mmMutexFC指针
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmCondLockInit(mmMutexFC *mutex)
{
    return mmMutexInit(mutex);
}

/*
 * 描述:把mutex锁住
 * 参数: mutex--指向mmMutexFC 指针
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmCondLock(mmMutexFC *mutex)
{
    return mmMutexLock(mutex);
}

/*
 * 描述:把mutex解锁
 * 参数: mutex --指向mmMutexFC 指针
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmCondUnLock(mmMutexFC *mutex)
{
    return mmMutexUnLock(mutex);
}

 /*
 * 描述:销毁mmMutexFC
 * 参数: mutex--指向mmMutexFC 指针
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmCondLockDestroy(mmMutexFC *mutex)
{
    return mmMutexDestroy(mutex);
}

/*
 * 描述:用默认属性初始化读写锁
 * 参数: rwLock --指向mmRWLock_t的指针
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmRWLockInit(mmRWLock_t *rwLock)
{
    if (rwLock == NULL) {
        return EN_INVALID_PARAM;
    }

    INT32 ret = pthread_rwlock_init(rwLock, NULL);
    if (ret != MMPA_ZERO) {
        return EN_ERROR;
    }

    return EN_OK;
}

/*
 * 描述:把rwlock锁住,应用于读锁（阻塞）
 * 参数: rwLock --指向mmRWLock_t的指针
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmRWLockRDLock(mmRWLock_t *rwLock)
{
    if (rwLock == NULL) {
        return EN_INVALID_PARAM;
    }

    INT32 ret = pthread_rwlock_rdlock(rwLock);
    if (ret != MMPA_ZERO) {
        return EN_ERROR;
    }

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

    INT32 ret = pthread_rwlock_tryrdlock(rwLock);
    if (ret != MMPA_ZERO) {
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

    INT32 ret = pthread_rwlock_wrlock(rwLock);
    if (ret != MMPA_ZERO) {
        return EN_ERROR;
    }

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

    INT32 ret = pthread_rwlock_trywrlock(rwLock);
    if (ret != MMPA_ZERO) {
        return EN_ERROR;
    }

    return EN_OK;
}

/*
 * 描述:把rwlock解锁,应用于读锁
 * 参数: rwLock --指向mmRWLock_t的指针
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmRDLockUnLock(mmRWLock_t *rwLock)
{
    if (rwLock == NULL) {
        return EN_INVALID_PARAM;
    }

    INT32 ret = pthread_rwlock_unlock(rwLock);
    if (ret != MMPA_ZERO) {
        return EN_ERROR;
    }

    return EN_OK;
}

/*
 * 描述:把rwlock解锁,应用于写锁
 * 参数: rwLock --指向mmRWLock_t的指针
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmWRLockUnLock(mmRWLock_t *rwLock)
{
    if (rwLock == NULL) {
        return EN_INVALID_PARAM;
    }

    INT32 ret = pthread_rwlock_unlock(rwLock);
    if (ret != MMPA_ZERO) {
        return EN_ERROR;
    }

    return EN_OK;
}

/*
 * 描述:把rwlock删除，该接口windows为空实现，系统回自动清理
 * 参数: rwLock --指向mmRWLock_t的指针
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmRWLockDestroy(mmRWLock_t *rwLock)
{
    if (rwLock == NULL) {
        return EN_INVALID_PARAM;
    }

    INT32 ret = pthread_rwlock_destroy(rwLock);
    if (ret != MMPA_ZERO) {
        return EN_ERROR;
    }

    return EN_OK;
}


/*
 * 描述:用于阻塞当前线程
 * 参数: cond --  mmCond指针
 *       mutex -- mmMutexFC指针, 条件变量
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmCondWait(mmCond *cond, mmMutexFC *mutex)
{
    if ((cond == NULL) || (mutex == NULL)) {
        return EN_INVALID_PARAM;
    }

    INT32 ret = pthread_cond_wait(cond, mutex);
    if (ret != EN_OK) {
        ret = EN_ERROR;
    }
    return ret;
}

/*
 * 描述:用于阻塞当前线程一定时间，由外部指定时间
 * 参数: cond --  mmCond指针
 *       mutex -- mmMutexFC指针, 条件变量
 *       mmMilliSeconds -- 阻塞时间
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR、超时返回EN_TIMEOUT, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmCondTimedWait(mmCond *cond, mmMutexFC *mutex, UINT32 milliSecond)
{
    if ((cond == NULL) || (mutex == NULL)) {
        return EN_INVALID_PARAM;
    }

    struct timespec absoluteTime;
    (VOID)memset_s(&absoluteTime, sizeof(absoluteTime), 0, sizeof(absoluteTime)); /* unsafe_function_ignore: memset */

    // 系统开机至今的时间, 软件无法修改
    INT32 ret = clock_gettime(CLOCK_MONOTONIC, &absoluteTime);
    if (ret != EN_OK) {
        return EN_ERROR;
    }

    absoluteTime.tv_sec = absoluteTime.tv_sec + ((LONG)milliSecond / MMPA_SECOND_TO_MSEC);
    absoluteTime.tv_nsec = absoluteTime.tv_nsec +
        (((LONG)milliSecond % MMPA_SECOND_TO_MSEC) * MMPA_MSEC_TO_NSEC);

    // 判断是否进位
    if (absoluteTime.tv_nsec > MMPA_SECOND_TO_NSEC) {
        ++absoluteTime.tv_sec;
        absoluteTime.tv_nsec = absoluteTime.tv_nsec % MMPA_SECOND_TO_NSEC;
    }

    ret = pthread_cond_timedwait(cond, mutex, &absoluteTime); // 睡眠相对时间
    if (ret != EN_OK) {
        if (ret == ETIMEDOUT) {
            return EN_TIMEOUT;
        }
        ret = EN_ERROR;
    }
    return ret;
}

/*
 * 描述:唤醒mmCondWait阻塞的线程, 只给一个线程发信号
 * 参数: cond -- mmCond指针
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmCondNotify(mmCond *cond)
{
    if (cond == NULL) {
        return EN_INVALID_PARAM;
    }

    INT32 ret = pthread_cond_signal(cond);
    if (ret != EN_OK) {
        ret = EN_ERROR;
    }
    return ret;
}

/*
 * 描述:唤醒mmCondWait阻塞的线程, 可以给多个线程发信号
 * 参数: cond -- mmCond指针
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmCondNotifyAll(mmCond *cond)
{
    if (cond == NULL) {
        return EN_INVALID_PARAM;
    }

    INT32 ret = pthread_cond_broadcast(cond);
    if (ret != EN_OK) {
        ret = EN_ERROR;
    }
    return ret;
}

/*
 * 描述:销毁cond指向的条件变量
 * 参数: cond -- mmCond指针
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmCondDestroy(mmCond *cond)
{
    if (cond == NULL) {
        return EN_INVALID_PARAM;
    }

    INT32 ret = pthread_cond_destroy(cond);
    if (ret != EN_OK) {
        ret = EN_ERROR;
    }
    return ret;
}

/*
 * 描述:获取本地时间
 * 参数: sysTimePtr -- 指向mmSystemTime_t 结构的指针
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmGetLocalTime(mmSystemTime_t *sysTimePtr)
{
    if (sysTimePtr == NULL) {
        return EN_INVALID_PARAM;
    }

    struct timeval timeVal;
    (VOID)memset_s(&timeVal, sizeof(timeVal), 0, sizeof(timeVal)); /* unsafe_function_ignore: memset */

    INT32 ret = gettimeofday(&timeVal, NULL);
    if (ret != EN_OK) {
        return EN_ERROR;
    }

    struct tm nowTime;
    (VOID)memset_s(&nowTime, sizeof(nowTime), 0, sizeof(nowTime)); /* unsafe_function_ignore: memset */

    const struct tm *tmp = localtime_r(&timeVal.tv_sec, &nowTime);
    if (tmp == NULL) {
        return EN_ERROR;
    }

    sysTimePtr->wSecond = nowTime.tm_sec;
    sysTimePtr->wMinute = nowTime.tm_min;
    sysTimePtr->wHour = nowTime.tm_hour;
    sysTimePtr->wDay = nowTime.tm_mday;
    sysTimePtr->wMonth = nowTime.tm_mon + 1; // in localtime month is [0, 11], but in fact month is [1, 12]
    sysTimePtr->wYear = nowTime.tm_year + MMPA_COMPUTER_BEGIN_YEAR;
    sysTimePtr->wDayOfWeek = nowTime.tm_wday;
    sysTimePtr->tm_yday = nowTime.tm_yday;
    sysTimePtr->tm_isdst = nowTime.tm_isdst;
    sysTimePtr->wMilliseconds = timeVal.tv_usec / MMPA_MSEC_TO_USEC;

    return EN_OK;
}

/*
 * 描述:获取系统时间
 * 参数:sysTime -- 指向mmSystemTime_t 结构的指针
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmGetSystemTime(mmSystemTime_t *sysTimePtr)
{
    if (sysTimePtr == NULL) {
        return EN_INVALID_PARAM;
    }

    struct timeval timeVal;
    (VOID)memset_s(&timeVal, sizeof(timeVal), 0, sizeof(timeVal)); /* unsafe_function_ignore: memset */

    INT32 ret = gettimeofday(&timeVal, NULL);
    if (ret != EN_OK) {
        return EN_ERROR;
    }

    struct tm nowTime;
    (VOID)memset_s(&nowTime, sizeof(nowTime), 0, sizeof(nowTime)); /* unsafe_function_ignore: memset */

    const struct tm *tmp = gmtime_r(&timeVal.tv_sec, &nowTime);
    if (tmp == NULL) {
        return EN_ERROR;
    }

    sysTimePtr->wSecond = nowTime.tm_sec;
    sysTimePtr->wMinute = nowTime.tm_min;
    sysTimePtr->wHour = nowTime.tm_hour;
    sysTimePtr->wDay = nowTime.tm_mday;
    sysTimePtr->wMonth = nowTime.tm_mon + 1; // in localtime month is [0, 11], but in fact month is [1, 12]
    sysTimePtr->wYear = nowTime.tm_year + MMPA_COMPUTER_BEGIN_YEAR;
    sysTimePtr->wDayOfWeek = nowTime.tm_wday;
    sysTimePtr->tm_yday = nowTime.tm_yday;
    sysTimePtr->tm_isdst = nowTime.tm_isdst;
    sysTimePtr->wMilliseconds = timeVal.tv_usec / MMPA_MSEC_TO_USEC;

    return EN_OK;
}

/*
 * 描述:获取进程ID
 * 参数:无
 * 返回值:执行成功返回对应调用进程的id, 执行错误返回EN_ERROR
 */
INT32 mmGetPid(VOID)
{
    return (INT32)getpid();
}

/*
 * 描述:获取调用线程的线程ID
 * 参数:无
 * 返回值:执行成功返回对应调用线程的id, 执行错误返回EN_ERROR
 */
INT32 mmGetTid(VOID)
{
    INT32 ret = (INT32)syscall(SYS_gettid);
    if (ret < MMPA_ZERO) {
        return EN_ERROR;
    }

    return ret;
}
/*
 * 描述:获取进程ID
 * 参数: pstProcessHandle -- 存放进程ID的指向mmProcess类型的指针
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmGetPidHandle(mmProcess *processHandle)
{
    if (processHandle == NULL) {
        return EN_INVALID_PARAM;
    }
    *processHandle = (mmProcess)getpid();
    return EN_OK;
}


/*
 * 描述:初始化一个信号量
 * 参数: sem--指向mmSem_t的指针
 *       value--信号量的初始值
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmSemInit(mmSem_t *sem, UINT32 value)
{
    if (sem == NULL) {
        return EN_INVALID_PARAM;
    }
    INT32 ret = sem_init(sem, MMPA_ZERO, value);
    if (ret != EN_OK) {
        return EN_ERROR;
    }
    return EN_OK;
}

/*
 * 描述:用来阻塞当前线程直到信号量sem的值大于0，解除阻塞后将sem的值减一
 * 参数: sem--指向mmSem_t的指针
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmSemWait(mmSem_t *sem)
{
    if (sem == NULL) {
        return EN_INVALID_PARAM;
    }

    INT32 ret = sem_wait(sem);
    if (ret != EN_OK) {
        return EN_ERROR;
    }
    return EN_OK;
}

/*
 * 描述:用来增加信号量的值
 * 参数: sem--指向mmSem_t的指针
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmSemPost(mmSem_t *sem)
{
    if (sem == NULL) {
        return EN_INVALID_PARAM;
    }

    INT32 ret = sem_post(sem);
    if (ret != EN_OK) {
        return EN_ERROR;
    }
    return EN_OK;
}

/*
 * 描述:用完信号量对它进行清理
 * 参数: sem--指向mmSem_t的指针
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmSemDestroy(mmSem_t *sem)
{
    if (sem == NULL) {
        return EN_INVALID_PARAM;
    }

    INT32 ret = sem_destroy(sem);
    if (ret != EN_OK) {
        return EN_ERROR;
    }
    return EN_OK;
}

/*
 * 描述:打开或者创建一个文件
 * 参数: pathName--需要打开或者创建的文件路径名，由用户确保绝对路径
 *       flags--打开或者创建的文件标志位, 默认 user和group的权限
 * 返回值:执行成功返回对应打开的文件描述符, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmOpen(const CHAR *pathName, INT32 flags)
{
    return mmOpen2(pathName, flags, S_IRWXU | S_IRWXG);
}

/*
 * 描述:打开或者创建一个文件
 * 参数: pathName--需要打开或者创建的文件路径名，由用户确保绝对路径
 *       flags--打开或者创建的文件标志位
 *       mode -- 打开或者创建的权限
 * 返回值:执行成功返回对应打开的文件描述符, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmOpen2(const CHAR *pathName, INT32 flags, MODE mode)
{
    if ((pathName == NULL) || (flags < MMPA_ZERO)) {
        return EN_INVALID_PARAM;
    }
    UINT32 tmp = (UINT32)flags;

    if (((tmp & (O_TRUNC | O_WRONLY | O_RDWR | O_CREAT)) == MMPA_ZERO) && (flags != O_RDONLY)) {
        return EN_INVALID_PARAM;
    }
    if (((mode & (S_IRUSR | S_IREAD)) == MMPA_ZERO) &&
        ((mode & (S_IWUSR | S_IWRITE)) == MMPA_ZERO)) {
        return EN_INVALID_PARAM;
    }

    INT32 fd = open(pathName, flags, mode);
    if (fd < MMPA_ZERO) {
        return EN_ERROR;
    }
    return fd;
}

/*
 * 描述:内部使用
 */
static INT32 IdeCheckPopenArgs(const CHAR *type, UINT32 len)
{
    if (len < 1) {
        return EN_INVALID_PARAM;
    }

    if (((type[0] != 'r') && (type[0] != 'w'))) {
        return EN_ERROR;
    }
    if ((len > 1) && (type[1] != '\0')) {
        return EN_ERROR;
    }

    return EN_OK;
}

/*
* 描述：调用fork（）产生子进程，然后从子进程中调用shell来执行参数command的指令。
* 参数： command--参数是一个指向以 NULL 结束的 shell 命令字符串的指针。这行命令将被传到 bin/sh 并使用-c 标志，
*                shell 将执行这个命令
*        type--参数只能是读或者写中的一种，得到的返回值（标准 I/O 流）也具有和 type 相应的只读或只写类型。
* 返回值：若成功则返回文件指针，否则返回NULL，错误原因存于mmGetErrorCode中
* 说明：popen是危险函数，请使用白名单机制调用
*/
FILE *mmPopen(CHAR *command, CHAR *type)
{
    if ((command == NULL) || (type == NULL)) {
        return NULL;
    }
    if (IdeCheckPopenArgs(type, strlen(type)) != EN_OK) {
        return NULL;
    }

    return popen(command, type);
}

/*
 * 描述:关闭打开的文件
 * 参数: fd--指向打开文件的资源描述符
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmClose(INT32 fd)
{
    if (fd < MMPA_ZERO) {
        return EN_INVALID_PARAM;
    }

    INT32 ret = close(fd);
    if (ret != EN_OK) {
        return EN_ERROR;
    }
    return EN_OK;
}

/*
* 描述：pclose（）用来关闭由popen所建立的管道及文件指针。
* 参数: stream--为先前由popen（）所返回的文件指针
* 返回值：执行成功返回cmdstring的终止状态，若出错则返回EN_OK，入参检查错误返回EN_INVALID_PARAM
*/
INT32 mmPclose(FILE *stream)
{
    if (stream == NULL) {
        return EN_INVALID_PARAM;
    }
    return pclose(stream);
}

/*
 * 描述:写数据到一个资源文件中
 * 参数: fd--指向打开文件的资源描述符
 *       buf--需要写入的数据
 *       bufLen--需要写入的数据长度
 * 返回值:执行成功返回写入的长度, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
mmSsize_t mmWrite(INT32 fd, VOID *buf, UINT32 bufLen)
{
    if ((fd < MMPA_ZERO) || (buf == NULL)) {
        return EN_INVALID_PARAM;
    }

    mmSsize_t ret = write(fd, buf, (size_t)bufLen);
    if (ret < MMPA_ZERO) {
        return EN_ERROR;
    }
    return ret;
}

/*
 * 描述:从资源文件中读取数据
 * 参数: fd--指向打开文件的资源描述符
 *       buf--存放读取的数据，由用户分配缓存
 *       bufLen--需要读取的数据大小
 * 返回值:执行成功返回读取的长度, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
mmSsize_t mmRead(INT32 fd, VOID *buf, UINT32 bufLen)
{
    if ((fd < MMPA_ZERO) || (buf == NULL)) {
        return EN_INVALID_PARAM;
    }

    mmSsize_t ret = read(fd, buf, (size_t)bufLen);
    if (ret < MMPA_ZERO) {
        return EN_ERROR;
    }
    return ret;
}

/*
 * 描述:创建socket
 * 参数: sockFamily--协议域
 *       type--指定socket类型
 *       protocol--指定协议
 * 返回值:执行成功返回创建的socket id, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
mmSockHandle mmSocket(INT32 sockFamily, INT32 type, INT32 protocol)
{
    INT32 socketHandle = socket(sockFamily, type, protocol);
    if (socketHandle < MMPA_ZERO) {
        return EN_ERROR;
    }
    return socketHandle;
}

/*
 * 描述:把一个地址族中的特定地址赋给socket
 * 参数: sockFd--socket描述字，通过mmSocket函数创建
 *       addr--一个指向要绑定给sockFd的协议地址
 *       addrLen--对应地址的长度
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmBind(mmSockHandle sockFd, mmSockAddr* addr, mmSocklen_t addrLen)
{
    if ((sockFd < MMPA_ZERO) || (addr == NULL) || (addrLen <= MMPA_ZERO)) {
        return EN_INVALID_PARAM;
    }

    INT32 ret = bind(sockFd, addr, addrLen);
    if (ret != EN_OK) {
        return EN_ERROR;
    }
    return EN_OK;
}

/*
 * 描述:监听socket
 * 参数: sockFd--socket描述字，通过mmSocket函数创建
 *       backLog--相应socket可以排队的最大连接个数
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmListen(mmSockHandle sockFd, INT32 backLog)
{
    if ((sockFd < MMPA_ZERO) || (backLog <= MMPA_ZERO)) {
        return EN_INVALID_PARAM;
    }

    INT32 ret = listen(sockFd, backLog);
    if (ret != EN_OK) {
        return EN_ERROR;
    }
    return EN_OK;
}

/*
 * 描述:监听指定的socket地址
 * 参数: sockFd--socket描述字，通过mmSocket函数创建
 *       addr--用于返回客户端的协议地址, addr若为NULL, addrLen也应该为NULL
 *       addrLen--协议地址的长度
 * 返回值:执行成功返回自动生成的一个全新的socket id, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
mmSockHandle mmAccept(mmSockHandle sockFd, mmSockAddr *addr, mmSocklen_t *addrLen)
{
    if (sockFd < MMPA_ZERO) {
        return EN_INVALID_PARAM;
    }

    INT32 ret = accept(sockFd, addr, addrLen);
    if (ret < MMPA_ZERO) {
        return EN_ERROR;
    }

    return ret;
}

/*
 * 描述:发出socket连接请求
 * 参数:sockFd--socket描述字，通过mmSocket函数创建
 *      addr--服务器的socket地址
 *      addrLen--地址的长度
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmConnect(mmSockHandle sockFd, mmSockAddr* addr, mmSocklen_t addrLen)
{
    if ((sockFd < MMPA_ZERO) || (addr == NULL) || (addrLen <= MMPA_ZERO)) {
        return EN_INVALID_PARAM;
    }

    INT32 ret = connect(sockFd, addr, addrLen);
    if (ret < MMPA_ZERO) {
        return EN_ERROR;
    }
    return EN_OK;
}

/*
 * 描述:在建立连接的socket上发送数据
 * 参数: sockFd--已建立连接的socket描述字
 *       pstSendBuf--需要发送的数据缓存，有用户分配
 *       sendLen--需要发送的数据长度
 *       sendFlag--发送的方式标志位，一般置0
 * 返回值:执行成功返回实际发送的buf长度, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
mmSsize_t mmSocketSend(mmSockHandle sockFd, VOID *sendBuf, INT32 sendLen, INT32 sendFlag)
{
    if ((sockFd < MMPA_ZERO) || (sendBuf == NULL) || (sendLen <= MMPA_ZERO) || (sendFlag < MMPA_ZERO)) {
        return EN_INVALID_PARAM;
    }
    UINT32 sndLen = (UINT32)sendLen;
    mmSsize_t ret = send(sockFd, sendBuf, sndLen, sendFlag);
    if (ret <= MMPA_ZERO) {
        return EN_ERROR;
    }

    return ret;
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
    if ((sockFd < MMPA_ZERO) || (sendMsg == NULL) ||
        (sendLen <= MMPA_ZERO) || (addr == NULL) || (tolen <= MMPA_ZERO)) {
        return EN_INVALID_PARAM;
    }
    INT ret = sendto(sockFd, sendMsg, (size_t)(LONG)sendLen, (INT)sendFlag, addr, (size_t)(LONG)tolen);
    if (ret < MMPA_ZERO) {
        return EN_ERROR;
    }
    return ret;
}

/*
 * 描述:在建立连接的socket上接收数据
 * 参数: sockFd--已建立连接的socket描述字
 *       pstRecvBuf--存放接收的数据的缓存，用户分配
 *       recvLen--需要发送的数据长度
 *       recvFlag--接收的方式标志位，一般置0
 * 返回值:执行成功返回实际接收的buf长度, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
mmSsize_t mmSocketRecv(mmSockHandle sockFd, VOID *recvBuf, INT32 recvLen, INT32 recvFlag)
{
    if ((sockFd < MMPA_ZERO) || (recvBuf == NULL) || (recvLen <= MMPA_ZERO) || (recvFlag < MMPA_ZERO)) {
        return EN_INVALID_PARAM;
    }
    UINT32 rcvLen = (UINT32)recvLen;
    mmSsize_t ret = recv(sockFd, recvBuf, rcvLen, recvFlag);
    if (ret < MMPA_ZERO) {
        return EN_ERROR;
    }

    return ret;
}

/*
* 描述：在建立连接的socket上接收数据
* 参数： sockFd--已建立连接的socket描述字
*       recvBuf--存放接收的数据的缓存，用户分配
*       recvLen--需要接收的数据长度
*       recvFlag--发送的方式标志位，一般置0
*       addr--指向指定欲传送的套接字的地址
*       FromLen--addr所指地址的长度
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
    mmSsize_t ret = recvfrom(sockFd, recvBuf, recvLen, (INT)recvFlag, addr, FromLen);
    if (ret < MMPA_ZERO) {
        return EN_ERROR;
    }
    return ret;
}

/*
 * 描述:关闭建立的socket连接
 * 参数: sockFd--已打开或者建立连接的socket描述字
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmCloseSocket(mmSockHandle sockFd)
{
    if (sockFd < MMPA_ZERO) {
        return EN_INVALID_PARAM;
    }

    INT32 ret = close(sockFd);
    if (ret != EN_OK) {
        return EN_ERROR;
    }
    return EN_OK;
}

/*
 * 描述:初始化winsockDLL, windows中有效, linux为空实现
 */
INT32 mmSAStartup(VOID)
{
    return EN_OK;
}

/*
 * 描述:注销winsockDLL, windows中有效, linux为空实现
 */
INT32 mmSACleanup(VOID)
{
    return EN_OK;
}

/*
 * 描述:加载一个动态库中的符号
 * 参数:fileName--动态库文件名
 *      mode--打开方式
 * 返回值:执行成功返回动态链接库的句柄, 执行错误返回NULL, 入参检查错误返回NULL
 */
VOID *mmDlopen(const CHAR *fileName, INT32 mode)
{
    if ((fileName == NULL) || (mode < MMPA_ZERO)) {
        return NULL;
    }

    return dlopen(fileName, mode);
}

/*
* 描述：取有关最近定义给定addr 的符号的信息
* 参数： addr--指定加载模块的的某一地址
*       info--是指向mmDlInfo 结构的指针。由用户分配
* 返回值：执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
*/
INT32 mmDladdr(VOID *addr, mmDlInfo *info)
{
    if ((addr == NULL) || (info == NULL)) {
        return EN_INVALID_PARAM;
    }
    INT32 ret = dladdr(addr, (Dl_info *)info);
    if (ret == MMPA_ZERO) {
        return EN_ERROR;
    }
    return EN_OK;
}

/*
 * 描述:获取mmDlopen打开的动态库中的指定符号地址
 * 参数: handle--mmDlopen 返回的指向动态链接库指针
 *       funcName--要求获取的函数的名称
 * 返回值:执行成功返回指向函数的地址, 执行错误返回NULL, 入参检查错误返回NULL
 */
VOID *mmDlsym(VOID *handle, const CHAR *funcName)
{
    if ((handle == NULL) || (funcName == NULL)) {
        return NULL;
    }

    return dlsym(handle, funcName);
}

/*
 * 描述:关闭mmDlopen加载的动态库
 * 参数: handle--mmDlopen 返回的指向动态链接库指针
 *       funcName--要求获取的函数的名称
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmDlclose(VOID *handle)
{
    if (handle == NULL) {
        return EN_INVALID_PARAM;
    }

    INT32 ret = dlclose(handle);
    if (ret != EN_OK) {
        return EN_ERROR;
    }
    return EN_OK;
}
/*
 * 描述:当mmDlopen动态链接库操作函数执行失败时，mmDlerror可以返回出错信息
 * 返回值:执行成功返回NULL
 */
CHAR *mmDlerror(VOID)
{
    return dlerror();
}

/*
 * 描述:mmCreateAndSetTimer定时器的回调函数, 仅在内部使用
 */
static VOID mmTimerCallBack(union sigval userData)
{
    const mmUserBlock_t * const tmp = (mmUserBlock_t *)userData.sival_ptr;
    tmp->procFunc(tmp->pulArg);
    return;
}

/*
 * 描述:创建特定的定时器
 * 参数: timerHandle--被创建的定时器的ID
 *       milliSecond:定时器首次timer到期的时间 单位ms
 *       period:定时器循环的周期值 单位ms
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmCreateAndSetTimer(mmTimer *timerHandle, mmUserBlock_t *timerBlock, UINT milliSecond, UINT period)
{
    if ((timerHandle == NULL) || (timerBlock == NULL) || (timerBlock->procFunc == NULL)) {
        return EN_INVALID_PARAM;
    }

    struct sigevent event;
    (VOID)memset_s(&event, sizeof(event), MMPA_ZERO, sizeof(event)); /* unsafe_function_ignore: memset */
    event.sigev_value.sival_ptr = timerBlock;
    event.sigev_notify = SIGEV_THREAD;
    event.sigev_notify_function = mmTimerCallBack;
    event.sigev_signo = SIGUSR1;

    INT32 ret = timer_create(CLOCK_MONOTONIC, &event, timerHandle);
    if (ret != EN_OK) {
        return EN_ERROR;
    }

    struct itimerspec waitTime;
    (VOID)memset_s(&waitTime, sizeof(waitTime), MMPA_ZERO, sizeof(waitTime)); /* unsafe_function_ignore: memset */

    waitTime.it_interval.tv_sec =  (LONG)period / MMPA_SECOND_TO_MSEC;
    waitTime.it_interval.tv_nsec = ((LONG)period % MMPA_SECOND_TO_MSEC) * MMPA_MSEC_TO_NSEC;

    waitTime.it_value.tv_sec = (LONG)milliSecond / MMPA_SECOND_TO_MSEC;
    waitTime.it_value.tv_nsec = ((LONG)milliSecond % MMPA_SECOND_TO_MSEC) * MMPA_MSEC_TO_NSEC;

    ret = timer_settime(*timerHandle, MMPA_ZERO, &waitTime, NULL);
    if (ret != EN_OK) {
        (VOID)timer_delete(*timerHandle);
        return EN_ERROR;
    }

    return EN_OK;
}

/*
 * 描述:删除mmCreateAndSetTimer创建的定时器
 * 参数: timerHandle--对应的timer id
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR
 */
INT32 mmDeleteTimer(mmTimer timerHandle)
{
    INT32 ret = timer_delete(timerHandle);
    if (ret != EN_OK) {
        return EN_ERROR;
    }
    return EN_OK;
}


/*
 * 描述:获取文件状态
 * 参数: path--需要获取的文件路径名
 *       buffer--获取到的状态 由用户分配缓存
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmStatGet(const CHAR *path, mmStat_t *buffer)
{
    if ((path == NULL) || (buffer == NULL)) {
        return EN_INVALID_PARAM;
    }

    INT32 ret = stat(path, buffer);
    if (ret != EN_OK) {
        return EN_ERROR;
    }
    return EN_OK;
}

/*
 * 描述:获取文件状态(文件size大于2G使用)
 * 参数: path--需要获取的文件路径名
 *       buffer--获取到的状态 由用户分配缓存
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmStat64Get(const CHAR *path, mmStat64_t *buffer)
{
    if ((path == NULL) || (buffer == NULL)) {
        return EN_INVALID_PARAM;
    }
    INT32 ret = stat64(path, buffer);
    if (ret != EN_OK) {
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
    if (buffer == NULL) {
        return EN_INVALID_PARAM;
    }
    INT32 ret = fstat(fd, buffer);
    if (ret != EN_OK) {
        return EN_ERROR;
    }
    return EN_OK;
}

/*
 * 描述:创建一个目录
 * 参数: pathName -- 需要创建的目录路径名, 比如 CHAR dirName[256]="/home/test";
 *       mode -- 新目录的权限
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmMkdir(const CHAR *pathName, mmMode_t mode)
{
    if (pathName == NULL) {
        return EN_INVALID_PARAM;
    }

    INT32 ret = mkdir(pathName, mode);
    if (ret != EN_OK) {
        return EN_ERROR;
    }
    return EN_OK;
}

/*
 * 描述:睡眠指定时间
 * 参数: milliSecond -- 睡眠时间 单位ms, linux下usleep函数microSecond入参必须小于1000000
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmSleep(UINT32 milliSecond)
{
    if (milliSecond == MMPA_ZERO) {
        return EN_INVALID_PARAM;
    }
    UINT32 microSecond;

    // 防止截断
    if (milliSecond <= MMPA_MAX_SLEEP_MILLSECOND_USING_USLEEP) {
        microSecond = milliSecond * (UINT32)MMPA_MSEC_TO_USEC;
    } else {
        microSecond = MMPA_MAX_SLEEP_MICROSECOND_USING_USLEEP;
    }

    INT32 ret = usleep(microSecond);
    if (ret != EN_OK) {
        return EN_ERROR;
    }
    return EN_OK;
}

/*
 * 描述:创建带默认调度策略属性和优先级属性的线程, 属性只在Linux下有效, 默认调度策略SCHED_FIFO 优先级1
 * 参数: threadHandle -- pthread_t类型的实例
 *       funcBlock --包含函数名和参数的结构体指针
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmCreateTaskWithAttr(mmThread *threadHandle, mmUserBlock_t *funcBlock)
{
    if ((threadHandle == NULL) || (funcBlock == NULL) ||
        (funcBlock->procFunc == NULL)) {
        return EN_INVALID_PARAM;
    }
    UINT uid = getuid();
    // 非root用户的uid不为0，非root用户不支持使用该接口
    if (uid != 0) {
        return EN_ERROR;
    }
    INT32 policy = SCHED_RR;
#ifndef __ANDROID__
    INT32 inherit = PTHREAD_EXPLICIT_SCHED;
#endif
    pthread_attr_t attr;
    struct sched_param param;
    (VOID)memset_s(&attr, sizeof(attr), 0, sizeof(attr)); /* unsafe_function_ignore: memset */
    (VOID)memset_s(&param, sizeof(param), 0, sizeof(param)); /* unsafe_function_ignore: memset */
    param.sched_priority = MMPA_MIN_THREAD_PIO;

    // 初始化线程属性
    INT32 retVal = pthread_attr_init(&attr);
    if (retVal != EN_OK) {
        return EN_ERROR;
    }
#ifndef __ANDROID__
    // 设置默认继承属性 PTHREAD_EXPLICIT_SCHED
    retVal = pthread_attr_setinheritsched(&attr, inherit);
    if (retVal != EN_OK) {
        (VOID)pthread_attr_destroy(&attr);
        return EN_ERROR;
    }
#endif
    // 设置默认调度策略 SCHED_RR
    retVal = pthread_attr_setschedpolicy(&attr, policy);
    if (retVal != EN_OK) {
        (VOID)pthread_attr_destroy(&attr);
        return EN_ERROR;
    }
    // 设置默认优先级 1
    retVal = pthread_attr_setschedparam(&attr, &param);
    if (retVal != EN_OK) {
        (VOID)pthread_attr_destroy(&attr);
        return EN_ERROR;
    }

    retVal = pthread_create(threadHandle, &attr, funcBlock->procFunc, funcBlock->pulArg);
    (VOID)pthread_attr_destroy(&attr);
    if (retVal != EN_OK) {
        retVal = EN_ERROR;
    }
    return retVal;
}
/*
 * 描述:获取指定进程优先级id
 * 参数: pid--进程ID
 * 返回值:执行成功返回优先级id, 执行错误返回MMPA_PROCESS_ERROR, 入参检查错误返回MMPA_PROCESS_ERROR
 */
INT32 mmGetProcessPrio(mmProcess pid)
{
    if (pid < MMPA_ZERO) {
        return MMPA_PROCESS_ERROR;
    }
    errno = MMPA_ZERO;
    INT32 ret = getpriority(PRIO_PROCESS, (id_t)pid);
    if ((ret == EN_ERROR) && ((INT32)errno != MMPA_ZERO)) {
        return MMPA_PROCESS_ERROR;
    }
    return ret;
}

/*
 * 描述:设置进程优先级
 * 参数: pid--进程ID
 *       processPrio: 需要设置的优先级值，参数范围取 -20 to 19
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmSetProcessPrio(mmProcess pid, INT32 processPrio)
{
    if ((pid < MMPA_ZERO) || (processPrio < MMPA_MIN_NI) || (processPrio > MMPA_MAX_NI)) {
        return EN_INVALID_PARAM;
    }
    return setpriority(PRIO_PROCESS, (id_t)pid, processPrio);
}
/*
 * 描述:获取线程优先级
 * 参数: threadHandle--线程ID
 * 返回值:执行成功返回获取到的优先级, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmGetThreadPrio(mmThread *threadHandle)
{
    if (threadHandle == NULL) {
        return EN_INVALID_PARAM;
    }
    struct sched_param param;
    INT32 policy = SCHED_RR;
    (VOID)memset_s(&param, sizeof(param), 0, sizeof(param)); /* unsafe_function_ignore: memset */

    INT32 ret = pthread_getschedparam(*threadHandle, &policy, &param);
    if (ret != EN_OK) {
        return EN_ERROR;
    }
    return param.sched_priority;
}

/*
 * 描述:设置线程优先级, Linux默认以SCHED_RR策略设置
 * 参数: threadHandle--线程ID
 *       threadPrio:设置的优先级id，参数范围取1 - 99, 1为最高优先级
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmSetThreadPrio(mmThread *threadHandle, INT32 threadPrio)
{
    if (threadHandle == NULL) {
        return EN_INVALID_PARAM;
    }
    if ((threadPrio > MMPA_MAX_THREAD_PIO) || (threadPrio < MMPA_MIN_THREAD_PIO)) {
        return EN_INVALID_PARAM ;
    }

    struct sched_param param;
    (VOID)memset_s(&param, sizeof(param), 0, sizeof(param)); /* unsafe_function_ignore: memset */
    param.sched_priority = threadPrio;

    INT32 ret = pthread_setschedparam(*threadHandle, SCHED_RR, &param);
    if (ret != EN_OK) {
        return EN_ERROR;
    }
    return ret;
}

/*
 * 描述:判断文件或者目录是否存在
 * 参数: pathName -- 文件路径名
 * 参数: mode -- 权限
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmAccess2(const CHAR *pathName, INT32 mode)
{
    if (pathName == NULL) {
        return EN_INVALID_PARAM;
    }

    INT32 ret = access(pathName, mode);
    if (ret != EN_OK) {
        return EN_ERROR;
    }
    return EN_OK;
}

/*
 * 描述:判断文件或者目录是否存在
 * 参数: pathName -- 文件路径名
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmAccess(const CHAR *pathName)
{
    return mmAccess2(pathName, F_OK);
}

/*
 * 描述:删除目录下所有文件及目录, 包括子目录
 * 参数: pathName -- 目录名全路径
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmRmdir(const CHAR *pathName)
{
    INT32 ret;
    DIR *childDir = NULL;

    if (pathName == NULL) {
        return EN_INVALID_PARAM;
    }
    DIR *dir = opendir(pathName);
    if (dir == NULL) {
        return EN_INVALID_PARAM;
    }

    const struct dirent *entry = NULL;
    size_t bufSize = strlen(pathName) + (size_t)(PATH_SIZE + 2); // make sure the length is large enough
    while ((entry = readdir(dir)) != NULL) {
        if ((strcmp(".", entry->d_name) == MMPA_ZERO) || (strcmp("..", entry->d_name) == MMPA_ZERO)) {
            continue;
        }
        CHAR *buf = (CHAR *)malloc(bufSize);
        if (buf == NULL) {
            break;
        }
        ret = memset_s(buf, bufSize, 0, bufSize);
        if (ret == EN_ERROR) {
            free(buf);
            buf = NULL;
            break;
        }
        ret = snprintf_s(buf, bufSize, bufSize - 1U, "%s/%s", pathName, entry->d_name);
        if (ret == EN_ERROR) {
            free(buf);
            buf = NULL;
            break;
        }

        childDir = opendir(buf);
        if (childDir != NULL) {
            (VOID)closedir(childDir);
            (VOID)mmRmdir(buf);
            free(buf);
            buf = NULL;
            continue;
        } else {
            ret = unlink(buf);
            if (ret == EN_OK) {
                free(buf);
                continue;
            }
        }
        free(buf);
        buf = NULL;
    }
    (VOID)closedir(dir);

    ret = rmdir(pathName);
    if (ret == EN_ERROR) {
        return EN_ERROR;
    }
    return EN_OK;
}

/*
 * 描述:对设备的I/O通道进行管理
 * 参数: fd--指向设备驱动文件的文件资源描述符
 *       ioctlCode--ioctl的操作码
 *       bufPtr--指向数据的缓存, 里面包含的输入输出buf缓存由用户分配
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmIoctl(mmProcess fd, INT32 ioctlCode, mmIoctlBuf *bufPtr)
{
    if ((fd < MMPA_ZERO) || (bufPtr == NULL) || (bufPtr->inbuf == NULL)) {
        return EN_INVALID_PARAM;
    }
    UINT32 request = (UINT32)ioctlCode;
    INT32 ret = ioctl(fd, request, bufPtr->inbuf);
    if (ret < EN_OK) {
        return EN_ERROR;
    }
    return ret;
}

/*
 * 描述:获取自系统启动的调单递增的时间
 * 参数: 无
 * 返回值:自系统启动的调单递增的时间单位为毫秒
 */
static LONG GetMonnotonicTime(VOID)
{
    struct timespec curTime = {0};
    (VOID)clock_gettime(CLOCK_MONOTONIC, &curTime);
    // 以毫秒为单位
    LONG result = (LONG)((curTime.tv_sec) * MMPA_SECOND_TO_MSEC);
    result += (LONG)((curTime.tv_nsec) / MMPA_MSEC_TO_NSEC);
    return result;
}

/*
 * 描述:用来阻塞当前线程直到信号量sem的值大于0，解除阻塞后将sem的值减一, 同时指定最长阻塞时间
 * 参数: sem--指向mmSem_t的指针
 *       timeout--超时时间 单位ms
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmSemTimedWait(mmSem_t *sem, INT32 timeout)
{
    if ((sem == NULL) || (timeout < MMPA_ZERO)) {
        return EN_INVALID_PARAM;
    }
    LONG startTime = GetMonnotonicTime();
    LONG endTime = startTime + timeout;
    do {
        if (sem_trywait(sem) == EN_OK) {
            return EN_OK;
        }
        (VOID)mmSleep(1); // 延时1毫秒再检测
    } while (GetMonnotonicTime() <= endTime);
    return EN_OK;
}

/*
 * 描述:用于在一次函数调用中写多个非连续缓冲区
 * 参数: fd -- 打开的资源描述符
 *       iov -- 指向非连续缓存区的首地址的指针
 *       iovcnt -- 非连续缓冲区的个数, 最大支持MAX_IOVEC_SIZE片非连续缓冲区
 * 返回值:执行成功返回实际写入的长度, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
mmSsize_t mmWritev(mmProcess fd, mmIovSegment *iov, INT32 iovcnt)
{
    INT32 i = 0;
    if ((fd < MMPA_ZERO) || (iov == NULL) || (iovcnt < MMPA_ZERO) || (iovcnt > MAX_IOVEC_SIZE)) {
        return EN_INVALID_PARAM;
    }
    struct iovec tmpSegment[MAX_IOVEC_SIZE];
    (VOID)memset_s(tmpSegment, sizeof(tmpSegment), 0, sizeof(tmpSegment)); /* unsafe_function_ignore: memset */

    for (i = 0; i < iovcnt; i++) {
        tmpSegment[i].iov_base = iov[i].sendBuf;
        tmpSegment[i].iov_len = (UINT32)iov[i].sendLen;
    }

    mmSsize_t ret = writev(fd, tmpSegment, iovcnt);
    if (ret < EN_OK) {
        return EN_ERROR;
    }
    return ret;
}

/*
 * 描述:创建一个内存屏障
 */
VOID mmMb(VOID)
{
    __asm__ __volatile__ ("" : : : "memory");
}

/*
 * 描述:一个字符串IP地址转换为一个32位的网络序列IP地址
 * 参数: addrStr -- 待转换的字符串IP地址
 *       addr -- 转换后的网络序列IP地址
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmInetAton(const CHAR *addrStr, mmInAddr *addr)
{
    if ((addr == NULL) || (addrStr == NULL)) {
        return EN_INVALID_PARAM;
    }
    INT32 ret = inet_aton(addrStr, addr);
    if (ret > MMPA_ZERO) {
        return EN_OK;
    } else {
        return EN_ERROR;
    }
}

/*
 * 描述:打开文件或者设备驱动,linux侧暂不支持
 * 参数: 文件路径名fileName, 打开的权限access, 是否新创建标志位fileFlag
 * 返回值:执行成功返回对应打开的文件描述符，执行错误返回EN_ERROR, 入参检查错误返回EN_ERROR
 */
mmProcess mmOpenFile(const CHAR *fileName, UINT32 accessFlag, mmCreateFlag fileFlag)
{
    return EN_ERROR;
}

/*
 * 描述:关闭打开的文件或者设备驱动
 * 参数: 打开的文件描述符fileId
 * 返回值:执行成功返回EN_OK，执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmCloseFile(mmProcess fileId)
{
    if (fileId < MMPA_ZERO) {
        return EN_INVALID_PARAM;
    }

    INT32 ret = close(fileId);
    if (ret != EN_OK) {
        return EN_ERROR;
    }
    return EN_OK;
}

/*
 * 描述:往打开的文件或者设备驱动写内容
 * 参数:打开的文件描述符fileId，buffer为用户分配缓存，len为buffer对应长度
 * 返回值:执行成功返回EN_OK，执行错误返回EN_ERROR，入参检查错误返回EN_INVALID_PARAM
 */
mmSsize_t mmWriteFile(mmProcess fileId, VOID *buffer, INT32 len)
{
    if ((fileId < MMPA_ZERO) || (buffer == NULL) || (len < MMPA_ZERO)) {
        return EN_INVALID_PARAM;
    }
    UINT32 writeLen = (UINT32)len;
    mmSsize_t ret = write(fileId, buffer, writeLen);
    if (ret < MMPA_ZERO) {
        return EN_ERROR;
    }
    return ret;
}

/*
 * 描述:往打开的文件或者设备驱动读取内容
 * 参数: 打开的文件描述符fileId，buffer为用户分配缓存，len为buffer对应长度
 * 返回值:执行成功返回实际读取的长度，执行错误返回EN_ERROR，入参检查错误返回EN_INVALID_PARAM
 */
mmSsize_t mmReadFile(mmProcess fileId, VOID *buffer, INT32 len)
{
    if ((fileId < MMPA_ZERO) || (buffer == NULL) || (len < MMPA_ZERO)) {
        return EN_INVALID_PARAM;
    }
    UINT32 readLen = (UINT32)len;

    mmSsize_t ret = read(fileId, buffer, readLen);
    if (ret < MMPA_ZERO) {
        return EN_ERROR;
    }
    return ret;
}

/*
 * 描述:原子操作的设置某个值
 * 参数: ptr指针指向需要修改的值, value是需要设置的值
 * 返回值:执行成功返回设置为value后的值，入参错误返回EN_INVALID_PARAM
 */
mmAtomicType mmSetData(mmAtomicType *ptr, mmAtomicType value)
{
    if (ptr == NULL) {
        return EN_INVALID_PARAM;
    }
    mmAtomicType ret = __sync_lock_test_and_set(ptr, value);
    return ret;
}

/*
 * 描述:原子操作的加法
 * 参数: ptr指针指向需要修改的值, value是需要加的值
 * 返回值:执行成功返回加上value后的值，入参错误返回EN_INVALID_PARAM
 */
mmAtomicType mmValueInc(mmAtomicType *ptr, mmAtomicType value)
{
    if (ptr == NULL) {
        return EN_INVALID_PARAM;
    }
    mmAtomicType ret = __sync_add_and_fetch(ptr, value);
    return ret;
}

/*
 * 描述:原子操作的减法
 * 参数: ptr指针指向需要修改的值, value是需要减的值
 * 返回值:执行成功返回减去value后的值，入参错误返回EN_INVALID_PARAM
 */
mmAtomicType mmValueSub(mmAtomicType *ptr, mmAtomicType value)
{
    if (ptr == NULL) {
        return EN_INVALID_PARAM;
    }
    mmAtomicType ret = __sync_sub_and_fetch(ptr, value);
    return ret;
}

/*
 * 描述:原子操作的设置某个值
 * 参数: ptr指针指向需要修改的值, value是需要设置的值
 * 返回值:执行成功返回设置为value后的值，入参错误返回EN_INVALID_PARAM
 */
mmAtomicType64 mmSetData64(mmAtomicType64 *ptr, mmAtomicType64 value)
{
    if (ptr == NULL) {
        return EN_INVALID_PARAM;
    }
    mmAtomicType64 ret = __sync_lock_test_and_set(ptr, value);
    return ret;
}

/*
 * 描述:原子操作的加法
 * 参数: ptr指针指向需要修改的值, value是需要加的值
 * 返回值:执行成功返回加上value后的值，入参错误返回EN_INVALID_PARAM
 */
mmAtomicType64 mmValueInc64(mmAtomicType64 *ptr, mmAtomicType64 value)
{
    if (ptr == NULL) {
        return EN_INVALID_PARAM;
    }
    mmAtomicType64 ret = __sync_add_and_fetch(ptr, value);
    return ret;
}

/*
 * 描述:原子操作的减法
 * 参数: ptr指针指向需要修改的值, value是需要减的值
 * 返回值:执行成功返回减去value后的值，入参错误返回EN_INVALID_PARAM
 */
mmAtomicType64 mmValueSub64(mmAtomicType64 *ptr, mmAtomicType64 value)
{
    if (ptr == NULL) {
        return EN_INVALID_PARAM;
    }
    mmAtomicType64 ret = __sync_sub_and_fetch(ptr, value);
    return ret;
}

/*
 * 描述:创建带分离属性的线程, 不需要mmJoinTask等待线程结束
 * 参数: threadHandle -- pthread_t类型的实例
 *       funcBlock --包含函数名和参数的结构体指针
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmCreateTaskWithDetach(mmThread *threadHandle, mmUserBlock_t *funcBlock)
{
    if ((threadHandle == NULL) || (funcBlock == NULL) || (funcBlock->procFunc == NULL)) {
        return EN_INVALID_PARAM;
    }
    pthread_attr_t attr;
    (VOID)memset_s(&attr, sizeof(attr), 0, sizeof(attr)); /* unsafe_function_ignore: memset */

    // 初始化线程属性
    INT32 ret = pthread_attr_init(&attr);
    if (ret != EN_OK) {
        return EN_ERROR;
    }
    // 设置默认线程分离属性
    ret = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (ret != EN_OK) {
        (VOID)pthread_attr_destroy(&attr);
        return EN_ERROR;
    }

    ret = pthread_create(threadHandle, &attr, funcBlock->procFunc, funcBlock->pulArg);
    (VOID)pthread_attr_destroy(&attr);
    if (ret != EN_OK) {
        ret = EN_ERROR;
    }
    return ret;
}

/*
 * 描述:创建命名管道 待废弃 请使用mmCreatePipe
 * 参数:pipe管道描述符, 必须是两个, 一个读一个写
 *      pipeName-管道名, waitMode-是否阻塞创建
 * 返回值:执行成功返回EN_OK, 失败返回EN_ERROR, 入参错误返回EN_INVALID_PARAM
 */
INT32 mmCreateNamedPipe(mmPipeHandle pipeHandle[], CHAR *pipeName[], INT32 waitMode)
{
    INT32 ret = EN_ERROR;
    INT32 i = 0;

    for (i = 0; i < MAX_PIPE_COUNT; i++) {
        ret = mmAccess(pipeName[i]);
        if (ret == EN_ERROR) {
            ret = mkfifo(pipeName[i], MMPA_DEFAULT_PIPE_PERMISSION);
            if (ret != MMPA_ZERO) {
                return EN_ERROR;
            }
        }
    }

    if (!waitMode) {
        pipeHandle[0] = open(pipeName[0], O_RDONLY | O_NONBLOCK);
        pipeHandle[1] = open(pipeName[1], O_WRONLY | O_NONBLOCK);
    } else {
        pipeHandle[0] = open(pipeName[0], O_RDONLY);
        pipeHandle[1] = open(pipeName[1], O_WRONLY);
    }

    if ((pipeHandle[0] == EN_ERROR) || (pipeHandle[1] == EN_ERROR)) {
        return EN_ERROR;
    } else {
        return EN_OK;
    }
}

/*
 * 描述:打开创建的命名管道 待废弃 请使用mmOpenPipe
 * 参数:pipe管道描述符, 必须是两个, 一个读一个写
 *      pipeName-管道名, waitMode-是否阻塞打开
 * 返回值:执行成功返回EN_OK, 失败返回EN_ERROR, 入参错误返回EN_INVALID_PARAM
 */
INT32 mmOpenNamePipe(mmPipeHandle pipeHandle[], CHAR *pipeName[], INT32 waitMode)
{
    if (!waitMode) {
        pipeHandle[0] = open(pipeName[0], O_WRONLY | O_NONBLOCK);
        pipeHandle[1] = open(pipeName[1], O_RDONLY | O_NONBLOCK);
    } else {
        pipeHandle[0] = open(pipeName[0], O_WRONLY);
        pipeHandle[1] = open(pipeName[1], O_RDONLY);
    }

    if ((pipeHandle[0] == EN_ERROR) || (pipeHandle[1] == EN_ERROR)) {
        return EN_ERROR;
    } else {
        return EN_OK;
    }
}

/*
 * 描述:关闭命名管道 待废弃 请使用mmClosePipe
 * 参数:namedPipe管道描述符, 必须是两个, 一个读一个写
 * 返回值:无
 */
VOID mmCloseNamedPipe(mmPipeHandle namedPipe[])
{
    if (namedPipe[0] != 0) {
        (VOID)close(namedPipe[0]);
    }
    if (namedPipe[1] != 0) {
        (VOID)close(namedPipe[1]);
    }
    return;
}

/*
 * 描述:创建管道, 类型(命名管道)
 * 参数:pipe管道描述符, 必须是两个, 一个读一个写
 *      pipeName-管道名, waitMode-是否阻塞创建
 *      pipeCount-管道个数，必须是MMPA_PIPE_COUNT
 *      waitMode非0表示阻塞调用
 * 返回值:执行成功返回EN_OK, 失败返回EN_ERROR, 入参错误返回EN_INVALID_PARAM
 */
INT32 mmCreatePipe(mmPipeHandle pipeHandle[], CHAR *pipeName[], UINT32 pipeCount, INT32 waitMode)
{
    if ((pipeCount != MMPA_PIPE_COUNT) || (pipeHandle == NULL) || (pipeName == NULL) ||
        (pipeName[0] == NULL) || (pipeName[1] == NULL)) {
        return EN_INVALID_PARAM;
    }
    INT32 writePipe = EN_ERROR;
    INT32 ret = EN_ERROR;
    INT32 i = 0;

    for (i = 0; i < MMPA_PIPE_COUNT; i++) {
        ret = mmAccess(pipeName[i]);
        if (ret == EN_ERROR) {
            ret = mkfifo(pipeName[i], MMPA_DEFAULT_PIPE_PERMISSION);
            if (ret != MMPA_ZERO) {
                return EN_ERROR;
            }
        }
    }

    if (waitMode == MMPA_ZERO) {
        pipeHandle[0] = open(pipeName[0], O_RDONLY | O_NONBLOCK);

        // 为了pipe1使得O_WRONLY open成功
        writePipe = open(pipeName[1], O_RDONLY | O_NONBLOCK);

        // 如果没有其他进程以读的方式打开，O_WRONLY open会失败
        pipeHandle[1] = open(pipeName[1], O_WRONLY | O_NONBLOCK);
    } else {
        pipeHandle[0] = open(pipeName[0], O_RDONLY);
        pipeHandle[1] = open(pipeName[1], O_WRONLY);
    }

    if ((pipeHandle[0] == EN_ERROR) || (pipeHandle[1] == EN_ERROR)) {
        (VOID)mmClose(pipeHandle[0]);
        (VOID)mmClose(pipeHandle[1]);
        (VOID)mmClose(writePipe);
        return EN_ERROR;
    } else {
        (VOID)mmClose(writePipe);
        return EN_OK;
    }
}

/*
 * 描述:打开创建的命名管道
 * 参数:pipe管道描述符, 必须是两个, 一个读一个写
 *      pipeName-管道名, waitMode-是否阻塞打开 非0表示阻塞打开 0表示非阻塞打开
 *      pipeCount-管道个数，必须是MMPA_PIPE_COUNT
 * 返回值:执行成功返回EN_OK, 失败返回EN_ERROR, 入参错误返回EN_INVALID_PARAM
 */
INT32 mmOpenPipe(mmPipeHandle pipeHandle[], CHAR *pipeName[], UINT32 pipeCount, INT32 waitMode)
{
    if ((pipeCount != MMPA_PIPE_COUNT) || (pipeHandle == NULL) || (pipeName == NULL) ||
        (pipeName[0] == NULL) || (pipeName[1] == NULL)) {
        return EN_INVALID_PARAM;
    }
    if (waitMode == MMPA_ZERO) {
        pipeHandle[0] = open(pipeName[0], O_WRONLY | O_NONBLOCK);
        pipeHandle[1] = open(pipeName[1], O_RDONLY | O_NONBLOCK);
    } else {
        pipeHandle[0] = open(pipeName[0], O_WRONLY);
        pipeHandle[1] = open(pipeName[1], O_RDONLY);
    }

    if ((pipeHandle[0] == EN_ERROR) || (pipeHandle[1] == EN_ERROR)) {
        (VOID)mmClose(pipeHandle[0]);
        (VOID)mmClose(pipeHandle[1]);
        return EN_ERROR;
    } else {
        return EN_OK;
    }
}

/*
 * 描述:关闭命名管道
 * 参数:pipe管道描述符, 必须是两个, 一个读一个写
 *      pipeCount-管道个数，必须是MMPA_PIPE_COUNT
 * 返回值:无
 */
VOID mmClosePipe(mmPipeHandle pipeHandle[], UINT32 pipeCount)
{
    if ((pipeCount != MMPA_PIPE_COUNT) || (pipeHandle == NULL)) {
        return;
    }
    (VOID)mmClose(pipeHandle[0]);
    (VOID)mmClose(pipeHandle[1]);
    return;
}

/*
 * 描述:创建完成端口, Linux内部为空实现, 主要用于windows系统
 */
mmCompletionHandle mmCreateCompletionPort(VOID)
{
    return EN_OK;
}

/*
 * 描述:关闭完成端口, Linux内部为空实现, 主要用于windows系统
 */
VOID mmCloseCompletionPort(mmCompletionHandle handle)
{
    return;
}

/*
 * 描述:mmPoll内部使用，用来接收数据
 * 参数:fd--需要poll的资源描述符
 *        polledData--读取/接收到的数据
 */
static INT32 LocalGetData(mmPollfd fd, pmmPollData polledData)
{
    INT32 ret = 0;
    switch (fd.pollType) {
        case pollTypeIoctl: // ioctl获取数据
            ret = ioctl(fd.handle, (ULONG)(LONG)fd.ioctlCode, polledData->buf);
            if (ret < MMPA_ZERO) {
                polledData->bufRes = 0;
                return EN_ERROR;
            }
            break;
        case pollTypeRead: // read读取数据
            ret = (INT32)read(fd.handle, polledData->buf, (size_t)polledData->bufLen);
            if (ret <= MMPA_ZERO) {
                polledData->bufRes = 0;
                return EN_ERROR;
            }
            break;
        case pollTypeRecv: // recv接收数据
            ret = (INT32)recv(fd.handle, polledData->buf, (size_t)polledData->bufLen, 0);
            if (ret <= MMPA_ZERO) {
                polledData->bufRes = 0;
                return EN_ERROR;
            }
            break;
        default:
            break;
    }
    polledData->bufHandle = fd.handle;
    polledData->bufType = fd.pollType;
    polledData->bufRes = (UINT)ret;
    return EN_OK;
}

static INT32 CheckPollParam(const mmPollfd *fds, INT32 fdCount, const pmmPollData polledData, mmPollBack pollBack)
{
    if ((fds == NULL) || (fdCount == MMPA_ZERO) || (fdCount > FDSIZE) ||
        (polledData == NULL) || (polledData->buf == NULL) || (pollBack == NULL)) {
        return EN_INVALID_PARAM;
    }
    return EN_OK;
}

/*
 * 描述:等待IO数据是否可读可接收
 * 参数: 指向需要等待的fd的集合的指针和个数，timeout -- 超时等待时间,
         handleIOCP -- 对应的完成端口key,
         polledData -- 由用户分配的缓存,
         pollBack --用户注册的回调函数,
 *       polledData -- 若读取到会填充进缓存，回调出来，缓存大小由用户分配
 * 返回值:超时返回EN_ERR, 读取成功返回EN_OK, 失败返回EN_ERROR, 入参错误返回EN_INVALID_PARAM
 */
INT32 mmPoll(mmPollfd *fds, INT32 fdCount, INT32 timeout,
    mmCompletionHandle handleIOCP, pmmPollData polledData, mmPollBack pollBack)
{
    INT32 checkParamRet = CheckPollParam(fds, fdCount, polledData, pollBack);
    if (checkParamRet != EN_OK) {
        return checkParamRet;
    }
    INT32 i;
    UINT16 pollRevent;
    struct pollfd polledFd[FDSIZE];
    (VOID)memset_s(polledFd, sizeof(polledFd), 0, sizeof(polledFd)); /* unsafe_function_ignore: memset */

    for (i = 0; i < fdCount; i++) {
        polledFd[i].fd = fds[i].handle;
        polledFd[i].events = POLLIN;
    }
    UINT32 count = (UINT32)fdCount;
    INT32 ret = poll(polledFd, count, timeout);
    // poll调用返回-1表示失败，返回0表示超时，大于0正常
    if (ret == -1) {
        return EN_ERROR;
    }
    if (ret == MMPA_ZERO) {
        return EN_ERR;
    }
    for (i = 0; i < fdCount; i++) {
        pollRevent = (UINT16)polledFd[i].revents;
        if (((pollRevent & POLLIN) != MMPA_ZERO) || (fds[i].pollType == pollTypeIoctl)) {
            ret = LocalGetData(fds[i], polledData);
            if (ret == EN_OK) {
                pollBack(polledData);
                return EN_OK;
            } else {
                return EN_ERROR;
            }
        } else {
            continue;
        }
    }
    return EN_ERROR;
}

/*
 * 描述:获取错误码
 * 返回值:error code
 */
INT32 mmGetErrorCode(VOID)
{
    INT32 ret = (INT32)errno;
    return ret;
}

/*
* 描述：将mmGetErrorCode函数得到的错误信息转化成字符串信息
* 参数： errnum--错误码，即mmGetErrorCode的返回值
*       buf--收错误信息描述的缓冲区指针
*       size--缓冲区的大小
* 返回值:成功返回错误信息的字符串，失败返回NULL
*/
CHAR *mmGetErrorFormatMessage(mmErrorMsg errnum, CHAR *buf, mmSize size)
{
    if ((buf == NULL) || (size <= 0)) {
        return NULL;
    }
    return strerror_r(errnum, buf, size);
}

/*
 * 描述:获取当前系统时间和时区信息, windows不支持时区获取
 * 参数:timeVal--当前系统时间, 不能为NULL
        timeZone--当前系统设置的时区信息, 可以为NULL, 表示不需要获取时区信息
 * 返回值:执行成功返回EN_OK, 失败返回EN_ERROR，入参错误返回EN_INVALID_PARAM
 */
INT32 mmGetTimeOfDay(mmTimeval *timeVal, mmTimezone *timeZone)
{
    if (timeVal == NULL) {
        return EN_INVALID_PARAM;
    }
    INT32 ret = gettimeofday((struct timeval *)timeVal, (struct timezone *)timeZone);
    if (ret != EN_OK) {
        ret = EN_ERROR;
    }
    return ret;
}

/*
 * 描述:获取系统开机到现在经过的时间
 * 返回值:执行成功返回类型mmTimespec结构的时间
 */
mmTimespec mmGetTickCount(VOID)
{
    mmTimespec rts = {0};
    struct timespec ts = {0};
    (VOID)clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    rts.tv_sec = ts.tv_sec;
    rts.tv_nsec = ts.tv_nsec;
    return rts;
}

/*
 * 描述:将参数path所指的相对路径转换成绝对路径, 接口待废弃, 请使用mmRealPath
 * 参数: path--原始路径相对路径
         realPath--规范化后的绝对路径, 由用户分配内存, 长度必须要>= MMPA_MAX_PATH
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmGetRealPath(CHAR *path, CHAR *realPath)
{
    INT32 ret = EN_OK;
    if ((realPath == NULL) || (path == NULL)) {
        return EN_INVALID_PARAM;
    }
    const CHAR *pRet = realpath(path, realPath);
    if (pRet == NULL) {
        ret = EN_ERROR;
    }
    return ret;
}

/*
 * 描述:将参数path所指的相对路径转换成绝对路径
 * 参数: path--原始路径相对路径
 *       realPath--规范化后的绝对路径, 由用户分配内存
 *       realPathLen--realPath缓存的长度, 长度必须要>= MMPA_MAX_PATH
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmRealPath(const CHAR *path, CHAR *realPath, INT32 realPathLen)
{
    INT32 ret = EN_OK;
    if ((realPath == NULL) || (path == NULL) || (realPathLen < MMPA_MAX_PATH)) {
        return EN_INVALID_PARAM;
    }
    const CHAR *ptr = realpath(path, realPath);
    if (ptr == NULL) {
        ret = EN_ERROR;
    }
    return ret;
}

/*
 * 描述:扫描目录
 * 参数:path--目录路径
 *      filterFunc--用户指定的过滤回调函数
 *      sort--用户指定的排序回调函数
 *      entryList--扫描到的目录结构指针, 用户不需要分配缓存, 内部分配, 需要调用mmScandirFree释放
 * 返回值:执行成功返回扫描到的子目录数量, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmScandir(const CHAR *path, mmDirent ***entryList, mmFilter filterFunc, mmSort sort)
{
    if ((path == NULL) || (entryList == NULL)) {
        return EN_INVALID_PARAM;
    }
    INT32 count = scandir(path, entryList, filterFunc, sort);
    if (count < MMPA_ZERO) {
        return EN_ERROR;
    }
    return count;
}

/*
 * 描述:扫描目录
 * 参数:path--目录路径
 *      filterFunc--用户指定的过滤回调函数
 *      sort--用户指定的排序回调函数
 *      entryList--扫描到的目录结构指针, 用户不需要分配缓存, 内部分配, 需要调用mmScandirFree2释放
 * 返回值:执行成功返回扫描到的子目录和文件数量, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmScandir2(const CHAR *path, mmDirent2 ***entryList, mmFilter2 filterFunc, mmSort2 sort)
{
    if ((path == NULL) || (entryList == NULL)) {
        return EN_INVALID_PARAM;
    }
    INT32 count = scandir(path, entryList, filterFunc, sort);
    if (count < MMPA_ZERO) {
        return EN_ERROR;
    }
    return count;
}

/*
 * 描述:扫描目录对应的内存释放函数
 * 参数:entryList--mmScandir扫描到的目录结构指针
 *      count--扫描到的子目录数量
 * 返回值:无
 */
VOID mmScandirFree(mmDirent **entryList, INT32 count)
{
    if (entryList == NULL) {
        return;
    }
    INT32 j;
    for (j = 0; j < count; j++) {
        if (entryList[j] != NULL) {
            free(entryList[j]);
            entryList[j] = NULL;
        }
    }
    free(entryList);
}

/*
 * 描述:扫描目录对应的内存释放函数
 * 参数:entryList--mmScandir2扫描到的目录结构指针
 *      count--扫描到的子目录数量
 * 返回值:无
 */
VOID mmScandirFree2(mmDirent2 **entryList, INT32 count)
{
    if (entryList == NULL) {
        return;
    }
    INT32 j;
    for (j = 0; j < count; j++) {
        if (entryList[j] != NULL) {
            free(entryList[j]);
            entryList[j] = NULL;
        }
    }
    free(entryList);
}

/*
 * 描述:创建一个消息队列用于进程间通信
 * 参数:key--消息队列的KEY键值,
 *      msgFlag--取消息队列标识符
 * 返回值:执行成功则返回打开的消息队列ID, 执行错误返回EN_ERROR
 */
mmMsgid mmMsgCreate(mmKey_t key, INT32 msgFlag)
{
    return (mmMsgid)msgget(key, msgFlag);
}

/*
 * 描述:扫描目录对应的内存释放函数
 * 参数:key--消息队列的KEY键值,
 *      msgFlag--取消息队列标识符
 * 返回值:执行成功则返回打开的消息队列ID, 执行错误返回EN_ERROR
 */
mmMsgid mmMsgOpen(mmKey_t key, INT32 msgFlag)
{
    return (mmMsgid)msgget(key, msgFlag);
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
    if ((buf == NULL) || (bufLen <= MMPA_ZERO)) {
        return EN_INVALID_PARAM;
    }
    UINT32 sndLen = (UINT32)bufLen;
    return (INT32)msgsnd(msqid, buf, sndLen, msgFlag);
}

/*
 * 描述:从消息队列接收消息
 * 参数:msqid--消息队列ID
 *      bufLen--缓存长度
 *      msgFlag--消息标志位
 *      buf--由用户分配内存
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmMsgRcv(mmMsgid msqid, VOID *buf, INT32 bufLen, INT32 msgFlag)
{
    if ((buf == NULL) || (bufLen <= MMPA_ZERO)) {
        return EN_INVALID_PARAM;
    }
    UINT32 rcvLen = (UINT32)bufLen;
    return (INT32)msgrcv(msqid, buf, rcvLen, MMPA_DEFAULT_MSG_TYPE, msgFlag);
}

/*
 * 描述:关闭创建的消息队列
 * 参数:msqid--消息队列ID
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR
 */
INT32 mmMsgClose(mmMsgid msqid)
{
    return (INT32)msgctl(msqid, IPC_RMID, NULL);
}

/*
 * 描述:转换时间为本地时间格式
 * 参数:timep--待转换的time_t类型的时间
 *      result--struct tm格式类型的时间
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmLocalTimeR(const time_t *timep, struct tm *result)
{
    if ((timep == NULL) || (result == NULL)) {
        return EN_INVALID_PARAM;
    } else {
        time_t ts = *timep;
        struct tm nowTime = {0};
        const struct tm *tmp = localtime_r(&ts, &nowTime);
        if (tmp == NULL) {
            return EN_ERROR;
        }

        result->tm_year = nowTime.tm_year + MMPA_COMPUTER_BEGIN_YEAR;
        result->tm_mon = nowTime.tm_mon + 1;
        result->tm_mday = nowTime.tm_mday;
        result->tm_hour = nowTime.tm_hour;
        result->tm_min = nowTime.tm_min;
        result->tm_sec = nowTime.tm_sec;
    }
    return EN_OK;
}

/*
 * 描述:获取变量opterr的值
 * 返回值：获取到opterr的值
 */
INT32 mmGetOptErr(VOID)
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
INT32 mmGetOptInd(VOID)
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
INT32 mmGetOptOpt(VOID)
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
CHAR *mmGetOptArg(VOID)
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
INT32 mmGetOpt(INT32 argc, CHAR * const * argv, const CHAR *opts)
{
    return getopt(argc, argv, opts);
}

/*
 * 描述:分析命令行参数-长参数
 * 参数:argc--传递的参数个数
 *      argv--传递的参数内容
 *      opts--用来指定可以处理哪些选项
 *      longOpts--指向一个mmStructOption的数组
 *      longIndex--表示长选项在longopts中的位置
 * 返回值:执行错误, 找不到选项元素, 返回EN_ERROR
 */
INT32 mmGetOptLong(INT32 argc, CHAR * const * argv, const CHAR *opts, const mmStructOption *longOpts, INT32 *longIndex)
{
    return getopt_long(argc, argv, opts, longOpts, longIndex);
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
    off_t pos = lseek(fd, (LONG)offset, seekFlag);
    return (LONG)pos;
}

/*
 * 描述:参数fd指定的文件大小改为参数length指定的大小
 * 参数:文件描述符fd
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmFtruncate(mmProcess fd, UINT32 length)
{
    if (fd <= MMPA_ZERO) {
        return EN_INVALID_PARAM;
    }
    return ftruncate(fd, (off_t)(ULONG)length);
}

/*
 * 描述:复制一个文件的描述符
 * 参数:oldFd -- 需要复制的fd
 *      newFd -- 新的生成的fd
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmDup2(INT32 oldFd, INT32 newFd)
{
    if ((oldFd <= MMPA_ZERO) || (newFd < MMPA_ZERO)) {
        return EN_INVALID_PARAM;
    }
    INT32 ret = dup2(oldFd, newFd);
    if (ret == EN_ERROR) {
        return EN_ERROR;
    }
    return EN_OK;
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
    return dup(fd);
}

/*
 * 描述:返回指定文件流的文件描述符
 * 参数:stream--FILE类型文件流
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmFileno(FILE *stream)
{
    if (stream == NULL) {
        return EN_INVALID_PARAM;
    }

    return fileno(stream);
}

/*
 * 描述:删除一个文件
 * 参数:filename--文件路径
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmUnlink(const CHAR *filename)
{
    if (filename == NULL) {
        return EN_INVALID_PARAM;
    }

    return unlink(filename);
}

/*
 * 描述:修改文件读写权限，目前仅支持读写权限修改
 * 参数:filename--文件路径
 *      mode--需要修改的权限
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmChmod(const CHAR *filename, INT32 mode)
{
    if (filename == NULL) {
        return EN_INVALID_PARAM;
    }

    return chmod(filename, (UINT32)mode);
}

/*
 * 描述:线程局部变量存储区创建
 * 参数:destructor--清理函数-linux有效
 *      key-线程变量存储区索引
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmTlsCreate(mmThreadKey *key, VOID(*destructor)(VOID*))
{
    if (key == NULL) {
        return EN_INVALID_PARAM;
    }
    INT32 ret = pthread_key_create(key, destructor);
    if (ret != EN_OK) {
        return EN_ERROR;
    }
    return ret;
}

/*
 * 描述:线程局部变量设置
 * 参数:key--线程变量存储区索引
 *      value--需要设置的局部变量的值
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmTlsSet(mmThreadKey key, const VOID *value)
{
    if (value == NULL) {
        return EN_INVALID_PARAM;
    }
    INT32 ret = pthread_setspecific(key, value);
    if (ret != EN_OK) {
        return EN_ERROR;
    }
    return ret;
}

/*
 * 描述:线程局部变量获取
 * 参数:key --线程变量存储区索引
 * 返回值:执行成功返回指向局部存储变量的指针, 执行错误返回NULL
 */
VOID *mmTlsGet(mmThreadKey key)
{
    return pthread_getspecific(key);
}

/*
 * 描述:线程局部变量清除
 * 参数:key--线程变量存储区索引
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR
 */
INT32 mmTlsDelete(mmThreadKey key)
{
    INT32 ret = pthread_key_delete(key);
    if (ret != EN_OK) {
        return EN_ERROR;
    }
    return ret;
}

/*
 * 描述:获取当前操作系统类型
 * 返回值:执行成功返回:LINUX--当前系统是Linux系统
 *              WIN--当前系统是Windows系统
 */
INT32 mmGetOsType(VOID)
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
    if (fd == MMPA_ZERO) {
        return EN_INVALID_PARAM;
    }
    INT32 ret = fsync(fd);
    if (ret != EN_OK) {
        return EN_ERROR;
    }
    return EN_OK;
}

/*
 * 描述:将对应文件描述符在内存中的内容写入磁盘
 * 参数:fd--文件句柄
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmFsync2(INT32 fd)
{
    if (fd == MMPA_ZERO) {
        return EN_INVALID_PARAM;
    }
    INT32 ret = fsync(fd);
    if (ret != EN_OK) {
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
    if (path == NULL) {
        return EN_INVALID_PARAM;
    }

    return chdir(path);
}

/*
 * 描述:为每一个进程设置文件模式创建屏蔽字，并返回之前的值
 * 参数:pmode--需要修改的权限值
 * 返回值:执行成功返回先前的pmode的值
 */
INT32 mmUmask(INT32 pmode)
{
    mode_t mode = (mode_t)pmode;
    return (INT32)umask(mode);
}

/*
 * 描述:等待子进程结束并返回对应结束码
 * 参数:pid--欲等待的子进程ID
 *      status--保存子进程的状态信息
 *      options--options提供了一些另外的选项来控制waitpid()函数的行为
 *               M_WAIT_NOHANG--如果pid指定的子进程没有结束，则立即返回;如果结束了, 则返回该子进程的进程号
 *               M_WAIT_UNTRACED--如果子进程进入暂停状态，则马上返回
 * 返回值:子进程未结束返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 *        进程已经结束返回EN_ERR
 */
INT32 mmWaitPid(mmProcess pid, INT32 *status, INT32 options)
{
    if ((options != MMPA_ZERO) && (options != M_WAIT_NOHANG) && (options != M_WAIT_UNTRACED)) {
        return EN_INVALID_PARAM;
    }

    INT32 ret = waitpid(pid, status, options);
    if (ret == EN_ERROR) {
        return EN_ERROR;                 // 调用异常
    }
    if ((ret > MMPA_ZERO) && (ret == pid)) { // 返回了子进程ID
        if (status != NULL) {
            if (WIFEXITED(*status)) {
                *status = WEXITSTATUS(*status); // 正常退出码
            }
            if (WIFSIGNALED(*status)) {
                *status = WTERMSIG(*status); // 信号中止退出码
            }
        }
        return EN_ERR;                  // 进程结束
    }
    return EN_OK; // 进程未结束
}

/*
 * 描述:获取当前工作目录路径
 * 参数:buffer--由用户分配用来存放工作目录路径的缓存
 *      maxLen--缓存长度
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmGetCwd(CHAR *buffer, INT32 maxLen)
{
    if ((buffer == NULL) || (maxLen < MMPA_ZERO)) {
        return EN_INVALID_PARAM;
    }
    const CHAR *ptr = getcwd(buffer, (UINT32)maxLen);
    if (ptr != NULL) {
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
 * 返回值:执行成功返回截取后的字符串指针, 执行失败返回NULL
 */
CHAR *mmStrTokR(CHAR *str, const CHAR *delim, CHAR **saveptr)
{
    if (delim == NULL) {
        return NULL;
    }

    return strtok_r(str, delim, saveptr);
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
    INT32 ret;
    UINT32 envLen = 0;
    if ((name == NULL) || (value == NULL) || (len == MMPA_ZERO)) {
        return EN_INVALID_PARAM;
    }
    const CHAR *envPtr = getenv(name);
    if (envPtr == NULL) {
        return EN_ERROR;
    }

    UINT32 lenOfRet = (UINT32)strlen(envPtr);
    if (lenOfRet < (UINT32)(MMPA_MEM_MAX_LEN - 1)) {
        envLen = lenOfRet + 1U;
    }

    if ((envLen != MMPA_ZERO) && (len < envLen)) {
        return EN_INVALID_PARAM;
    } else {
        ret = memcpy_s(value, len, envPtr, envLen); //lint !e613
        if (ret != EN_OK) {
            return EN_ERROR;
        }
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
    if ((name == NULL) || (value == NULL)) {
        return EN_INVALID_PARAM;
    }

    return setenv(name, value, overwrite);
}

/*
 * 描述:截取目录, 比如/usr/bin/test, 截取后为 /usr/bin
 * 参数:path--路径，函数内部会修改path的值
 * 返回值:执行成功返回指向截取到的目录部分指针，执行失败返回NULL
 */
CHAR *mmDirName(CHAR *path)
{
    if (path == NULL) {
        return NULL;
    }
    return dirname(path);
}
/*
 * 描述:截取目录后面的部分, 比如/usr/bin/test, 截取后为 test
 * 参数:path--路径，函数内部会修改path的值(行尾有\\会去掉)
 * 返回值:执行成功返回指向截取到的目录部分指针，执行失败返回NULL
 */
CHAR *mmBaseName(CHAR *path)
{
    if (path == NULL) {
        return NULL;
    }
    return basename(path);
}

/*
 * 描述:获取当前指定路径下磁盘容量及可用空间
 * 参数:path--路径名
 *      diskSize--mmDiskSize结构内容
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmGetDiskFreeSpace(const CHAR *path, mmDiskSize *diskSize)
{
    if ((path == NULL) || (diskSize == NULL)) {
        return EN_INVALID_PARAM;
    }

    // 把文件系统信息读入 struct statvfs buf 中
    struct statvfs buf;
    (VOID)memset_s(&buf, sizeof(buf), 0, sizeof(buf)); /* unsafe_function_ignore: memset */

    INT32 ret = statvfs(path, &buf);
    if (ret == MMPA_ZERO) {
        diskSize->totalSize = (ULONGLONG)(buf.f_blocks) * (ULONGLONG)(buf.f_bsize);
        diskSize->availSize = (ULONGLONG)(buf.f_bavail) * (ULONGLONG)(buf.f_bsize);
        diskSize->freeSize = (ULONGLONG)(buf.f_bfree) * (ULONGLONG)(buf.f_bsize);
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
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-function-declaration"
    INT32 ret = pthread_setname_np(*threadHandle, name);
#pragma GCC diagnostic pop
    if (ret != EN_OK) {
        return EN_ERROR;
    }
    return EN_OK;
}

/*
 * 描述:获取线程名-线程体外调用, 仅在Linux下生效, Windows下不支持为空实现
 * 参数:threadHandle--线程ID
 *      size--存放线程名的缓存长度
 *      name--线程名由用户分配缓存, 缓存长度必须>=MMPA_THREADNAME_SIZE
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmGetThreadName(mmThread *threadHandle, CHAR* name, INT32 size)
{
#ifndef __GLIBC__
    return EN_ERROR;
#else
    if ((threadHandle == NULL) || (name == NULL) || (size < MMPA_THREADNAME_SIZE)) {
        return EN_INVALID_PARAM;
    }
    UINT32 len = (UINT32)size;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-function-declaration"
    INT32 ret = pthread_getname_np(*threadHandle, name, len);
#pragma GCC diagnostic pop
    if (ret != EN_OK) {
        return EN_ERROR;
    }
    return EN_OK;
#endif
}

/*
 * 描述:设置当前执行的线程的线程名-线程体内调用, 仅在Linux下生效, Windows下不支持为空实现
 * 参数:name--需要设置的线程名
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmSetCurrentThreadName(const CHAR* name)
{
    if (name == NULL) {
        return EN_INVALID_PARAM;
    }
    INT32 ret = prctl(PR_SET_NAME, name);
    if (ret != EN_OK) {
        return EN_ERROR;
    }
    return EN_OK;
}

/*
 * 描述:获取当前执行的线程的线程名-线程体内调用, 仅在Linux下生效, Windows下不支持为空实现
 * 参数:name--获取到的线程名, 缓存空间由用户分配
 *      size--缓存长度必须>=MMPA_THREADNAME_SIZE
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmGetCurrentThreadName(CHAR* name, INT32 size)
{
    if ((name == NULL) || (size < MMPA_THREADNAME_SIZE)) {
        return EN_INVALID_PARAM;
    }
    INT32 ret = prctl(PR_GET_NAME, name);
    if (ret != EN_OK) {
        return EN_ERROR;
    }
    return EN_OK;
}

/*
 * 描述:获取当前指定路径下文件大小
 * 参数:fileName--文件路径名
 *      length--获取到的文件大小
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmGetFileSize(const CHAR *fileName, ULONGLONG *length)
{
    if ((fileName == NULL) || (length == NULL)) {
        return EN_INVALID_PARAM;
    }
    struct stat fileStat;
    (VOID)memset_s(&fileStat, sizeof(fileStat), 0, sizeof(fileStat)); /* unsafe_function_ignore: memset */
    INT32 ret = lstat(fileName, &fileStat);
    if (ret < MMPA_ZERO) {
        return EN_ERROR;
    }
    *length = (ULONGLONG)fileStat.st_size;
    return EN_OK;
}

/*
 * 描述:判断是否是目录
 * 参数: fileName -- 文件路径名
 * 返回值:执行成功返回EN_OK(是目录), 执行错误返回EN_ERROR(不是目录), 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmIsDir(const CHAR *fileName)
{
    if (fileName == NULL) {
        return EN_INVALID_PARAM;
    }
    struct stat fileStat;
    (VOID)memset_s(&fileStat, sizeof(fileStat), 0, sizeof(fileStat)); /* unsafe_function_ignore: memset */
    INT32 ret = lstat(fileName, &fileStat);
    if (ret < MMPA_ZERO) {
        return EN_ERROR;
    }

    if (S_ISDIR(fileStat.st_mode) == 0) {
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
INT32 mmGetOsName(CHAR* name, INT32 nameSize)
{
    if ((name == NULL) || (nameSize < MMPA_MIN_OS_NAME_SIZE)) {
        return EN_INVALID_PARAM;
    }
    UINT32 length = (UINT32)nameSize;
    INT32 ret = gethostname(name, length);
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
INT32 mmGetOsVersion(CHAR* versionInfo, INT32 versionLength)
{
    if ((versionInfo == NULL) || (versionLength < MMPA_MIN_OS_VERSION_SIZE)) {
        return EN_INVALID_PARAM;
    }
    INT32 ret = 0;
    struct utsname sysInfo;
    (VOID)memset_s(&sysInfo, sizeof(sysInfo), 0, sizeof(sysInfo)); /* unsafe_function_ignore: memset */
    UINT32 length = (UINT32)versionLength;
    size_t len = (size_t)length;
    INT32 fb = uname(&sysInfo);
    if (fb < MMPA_ZERO) {
        return EN_ERROR;
    } else {
        ret = snprintf_s(versionInfo, len, (len - 1U), "%s-%s-%s",
            sysInfo.sysname, sysInfo.release, sysInfo.version);
        if (ret == EN_ERROR) {
            return EN_ERROR;
        }
        return EN_OK;
    }
}

/*
 * 描述:获取当前系统下所有网卡的mac地址列表(不包括lo网卡)
 * 参数:list--获取到的mac地址列表指针数组, 缓存由函数内部分配, 需要调用mmGetMacFree释放
 *      count--获取到的mac地址的个数
 * 返回值:执行成功返回扫描到的EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmGetMac(mmMacInfo **list, INT32 *count)
{
    if ((list == NULL) || (count == NULL)) {
        return EN_INVALID_PARAM;
    }
    struct ifreq ifr;
    CHAR buf[MMPA_MAX_IF_SIZE] = {0};
    struct ifconf ifc;
    mmMacInfo *macInfo = NULL;
    INT32 sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock == EN_ERROR) {
        return EN_ERROR;
    }
    ifc.ifc_buf = buf;
    ifc.ifc_len = (INT)sizeof(buf);
    INT32 ret = ioctl(sock, SIOCGIFCONF, &ifc);
    if (ret == EN_ERROR) {
        (VOID)close(sock);
        return EN_ERROR;
    }

    INT32 len = (INT32)sizeof(struct ifreq);
    const struct ifreq* it = ifc.ifc_req;
    *count = (ifc.ifc_len / len);
    UINT32 cnt = (UINT32)(*count);
    size_t needSize = (size_t)(cnt) * sizeof(mmMacInfo);

    macInfo = (mmMacInfo*)malloc(needSize);
    if (macInfo == NULL) {
        *count = MMPA_ZERO;
        (VOID)close(sock);
        return EN_ERROR;
    }

    (VOID)memset_s(macInfo, needSize, 0, needSize); /* unsafe_function_ignore: memset */
    const struct ifreq* const end = it + *count;
    INT32 i = 0;
    for (; it != end; ++it) {
        ret = strcpy_s(ifr.ifr_name, sizeof(ifr.ifr_name), it->ifr_name);
        if (ret != EOK) {
            *count = MMPA_ZERO;
            (VOID)close(sock);
            (VOID)free(macInfo);
            return EN_ERROR;
        }
        if (ioctl(sock, SIOCGIFFLAGS, &ifr) != MMPA_ZERO) {
            continue;
        }
        UINT16 ifrFlag = (UINT16)ifr.ifr_flags;
        if ((ifrFlag & IFF_LOOPBACK) == MMPA_ZERO) { // ignore loopback
            ret = ioctl(sock, SIOCGIFHWADDR, &ifr);
            if (ret != MMPA_ZERO) {
                continue;
            }
            const UCHAR *ptr = (UCHAR *)&ifr.ifr_ifru.ifru_hwaddr.sa_data[0];
            ret = snprintf_s(macInfo[i].addr,
                sizeof(macInfo[i].addr), sizeof(macInfo[i].addr) - 1U,
                "%02X-%02X-%02X-%02X-%02X-%02X",
                *(ptr + MMPA_MAC_ADDR_FIRST_BYTE),
                *(ptr + MMPA_MAC_ADDR_SECOND_BYTE),
                *(ptr + MMPA_MAC_ADDR_THIRD_BYTE),
                *(ptr + MMPA_MAC_ADDR_FOURTH_BYTE),
                *(ptr + MMPA_MAC_ADDR_FIFTH_BYTE),
                *(ptr + MMPA_MAC_ADDR_SIXTH_BYTE));
            if (ret == EN_ERROR) {
                *count = MMPA_ZERO;
                (VOID)close(sock);
                (VOID)free(macInfo);
                return EN_ERROR;
            }
            i++;
        } else {
            *count = *count - 1;
        }
    }
    (VOID)close(sock);
    if (*count <= 0) {
        (VOID)free(macInfo);
        return EN_ERROR;
    } else {
        *list = macInfo;
        return EN_OK;
    }
}

/*
 * 描述:释放由mmGetMac产生的动态内存
 * 参数:list--获取到的mac地址列表指针数组
 *      count--获取到的mac地址的个数
 * 返回值:执行成功返回扫描到的EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmGetMacFree(mmMacInfo *list, INT32 count)
{
    if ((list == NULL) || (count < MMPA_ZERO)) {
        return EN_INVALID_PARAM;
    }
    (VOID)free(list);
    return EN_OK;
}

/*
 * 描述:内部使用, 查找是否存在关键字的信息,buf为 xxx   :   xxx 形式
 * 参数:buf--需要查找的当前行数的字符串
 *      bufLen--字符串长度
 *      pattern--关键字子串
 *      value--查找到后存放关键信息的缓存
 *      valueLen--缓存长度
 * 返回值:执行成功返回EN_OK, 执行失败返回EN_ERROR
 */
static INT32 LocalLookup(CHAR *buf, UINT32 bufLen, const CHAR *pattern, CHAR *value, UINT32 valueLen)
{
    const CHAR *pValue = NULL;
    CHAR *pBuf = NULL;
    UINT32 len = strlen(pattern); //lint !e712

    // 空白字符过滤
    for (pBuf = buf; isspace(*pBuf) != 0; pBuf++) {}

    // 关键字匹配
    INT32 ret = strncmp(pBuf, pattern, len);
    if (ret != MMPA_ZERO) {
        return EN_ERROR;
    }
    // :之前空白字符过滤
    for (pBuf = pBuf + len; isspace(*pBuf) != 0; pBuf++) {}

    if (*pBuf == '\0') {
        return EN_ERROR;
    }

    // :之后空白字符过滤
    for (pBuf = pBuf + 1; isspace(*pBuf) != 0; pBuf++) {}

    pValue = pBuf;
    // 截取所需信息
    for (pBuf = buf + bufLen; isspace(*(pBuf - 1)) != 0; pBuf--) {}

    *pBuf = '\0';

    ret = memcpy_s(value, valueLen, pValue, strlen(pValue) + 1U);
    if (ret != EN_OK) {
        return EN_ERROR;
    }
    return EN_OK;
}

/*
 * 描述:内部使用, miniRC环境下通过拼接获取arm信息
 * 参数:cpuImplememter--存放cpuImplememter信息指针
 *      implememterLen--指针长度
 *      cpuPart--存放cpuPart信息指针
 *      partLen--指针长度
 * 返回值:无
 */
static const CHAR* LocalGetArmVersion(CHAR *cpuImplememter, CHAR *cpuPart)
{
    static struct CpuTypeTable g_paramatersTable[] = {
        { "0x410xd03", "ARMv8_Cortex_A53"},
        { "0x410xd05", "ARMv8_Cortex_A55"},
        { "0x410xd07", "ARMv8_Cortex_A57"},
        { "0x410xd08", "ARMv8_Cortex_A72"},
        { "0x410xd09", "ARMv8_Cortex_A73"},
        { "0x480xd01", "TaishanV110"}
    };
    CHAR cpuArmVersion[MMPA_CPUINFO_DOUBLE_SIZE] = {0};
    INT32 ret = snprintf_s(cpuArmVersion, sizeof(cpuArmVersion), sizeof(cpuArmVersion) - 1U,
                           "%s%s", cpuImplememter, cpuPart);
    if (ret == EN_ERROR) {
        return NULL;
    }
    INT32 i = 0;
    for (i = (INT32)(sizeof(g_paramatersTable) / sizeof(g_paramatersTable[0])) - 1; i >= 0; i--) {
        ret = strcasecmp(cpuArmVersion, g_paramatersTable[i].key);
        if (ret == 0) {
            return g_paramatersTable[i].value;
        }
    }
    return NULL;
}

/*
 * 描述:内部使用, miniRC环境下通过cpuImplememter获取manufacturer信息
 * 参数:cpuImplememter--存放cpuImplememter信息指针
 *      cpuInfo--存放获取的物理CPU信息
 * 返回值:无
 */
static VOID LocalGetArmManufacturer(CHAR *cpuImplememter, mmCpuDesc *cpuInfo)
{
    size_t len = strlen(cpuInfo->manufacturer);
    if (len != 0U) {
        return;
    }
    INT32 ret = EN_ERROR;
    static struct CpuTypeTable g_manufacturerTable[] = {
        { "0x41", "ARM"},
        { "0x42", "Broadcom"},
        { "0x43", "Cavium"},
        { "0x44", "DigitalEquipment"},
        { "0x48", "HiSilicon"},
        { "0x49", "Infineon"},
        { "0x4D", "Freescale"},
        { "0x4E", "NVIDIA"},
        { "0x50", "APM"},
        { "0x51", "Qualcomm"},
        { "0x56", "Marvell"},
        { "0x69", "Intel"}
    };

    INT32 i = 0;
    for (i = (INT32)(sizeof(g_manufacturerTable) / sizeof(g_manufacturerTable[0])) - 1; i >= 0; --i) {
        ret = strcasecmp(cpuImplememter, g_manufacturerTable[i].key);
        if (ret == 0) {
            (VOID)memcpy_s(cpuInfo->manufacturer, sizeof(cpuInfo->manufacturer),
                g_manufacturerTable[i].value, (strlen(g_manufacturerTable[i].value) + 1U));
            return;
        }
    }
    return;
}

/*
 * 描述:内部使用, 读取/proc/cpuinfo信息中的部分信息
 * 参数:cpuInfo--存放获取的物理CPU信息
 *      physicalCount--物理CPU个数
 * 返回值:无
 */
static VOID LocalGetCpuProc(mmCpuDesc *cpuInfo, INT32 *physicalCount)
{
    CHAR buf[MMPA_CPUPROC_BUF_SIZE]                 = {0};
    CHAR physicalID[MMPA_CPUINFO_DEFAULT_SIZE]      = {0};
    CHAR cpuMhz[MMPA_CPUINFO_DEFAULT_SIZE]          = {0};
    CHAR cpuCores[MMPA_CPUINFO_DEFAULT_SIZE]        = {0};
    CHAR cpuCnt[MMPA_CPUINFO_DEFAULT_SIZE]          = {0};
    CHAR cpuImplememter[MMPA_CPUINFO_DEFAULT_SIZE]  = {0};
    CHAR cpuPart[MMPA_CPUINFO_DEFAULT_SIZE]         = {0};
    CHAR cpuThreads[MMPA_CPUINFO_DEFAULT_SIZE]      = {0};
    CHAR maxSpeed[MMPA_CPUINFO_DEFAULT_SIZE]        = {0};
    FILE *fp = fopen("/proc/cpuinfo", "r");
    if (fp == NULL) {
        return;
    }
    UINT32 length = 0U;
    while (fgets(buf, (INT)sizeof(buf), fp) != NULL) {
        length = (UINT32)strlen(buf);
        if (LocalLookup(buf, length, "manufacturer", cpuInfo->manufacturer, sizeof(cpuInfo->manufacturer)) == EN_OK) {
            continue;
        }
        if (LocalLookup(buf, length, "vendor_id", cpuInfo->manufacturer, sizeof(cpuInfo->manufacturer)) == EN_OK) {
            continue;
        }
        if (LocalLookup(buf, length, "CPU implementer", cpuImplememter, sizeof(cpuImplememter)) == EN_OK) {
            continue; /* ARM and aarch64 */
        }
        if (LocalLookup(buf, length, "CPU part", cpuPart, sizeof(cpuPart)) == EN_OK) {
            continue; /* ARM and aarch64 */
        }
        if (LocalLookup(buf, length, "model name", cpuInfo->version, sizeof(cpuInfo->version)) == EN_OK) {
            continue;
        }
        if (LocalLookup(buf, length, "cpu MHz", cpuMhz, sizeof(cpuMhz)) == EN_OK) {
            continue;
        }
        if (LocalLookup(buf, length, "cpu cores", cpuCores, sizeof(cpuCores)) == EN_OK) {
            continue;
        }
        if (LocalLookup(buf, length, "processor", cpuCnt, sizeof(cpuCnt)) == EN_OK) {
            continue; // processor index + 1
        }
        if (LocalLookup(buf, length, "physical id", physicalID, sizeof(physicalID)) == EN_OK) {
            continue;
        }
        if (LocalLookup(buf, length, "Thread Count", cpuThreads, sizeof(cpuThreads)) == EN_OK) {
            continue;
        }
        if (LocalLookup(buf, length, "Max Speed", maxSpeed, sizeof(maxSpeed)) == EN_OK) {
            ;
        }
    }
    (VOID)fclose(fp);
    fp = NULL;
    cpuInfo->frequency = atoi(cpuMhz);
    cpuInfo->ncores = atoi(cpuCores);
    cpuInfo->ncounts = atoi(cpuCnt) + 1;
    *physicalCount += atoi(physicalID);
    cpuInfo->nthreads = atoi(cpuThreads);
    cpuInfo->maxFrequency = atoi(maxSpeed);
    const CHAR* tmp = LocalGetArmVersion(cpuImplememter, cpuPart);
    if (tmp != NULL) {
        (VOID)memcpy_s(cpuInfo->version, sizeof(cpuInfo->version), tmp, strlen(tmp) + 1U);
    }
    LocalGetArmManufacturer(cpuImplememter, cpuInfo);
    return;
}

/*
 * 描述:获取当前系统cpu的部分物理硬件信息
 * 参数:cpuInfo--包含需要获取信息的结构体, 函数内部分配, 需要调用mmCpuInfoFree释放
 *      count--读取到的物理cpu个数
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmGetCpuInfo(mmCpuDesc **cpuInfo, INT32 *count)
{
    if (count == NULL) {
        return EN_INVALID_PARAM;
    }
    if (cpuInfo == NULL) {
        return EN_INVALID_PARAM;
    }
    INT32 i = 0;
    INT32 ret = 0;
    mmCpuDesc cpuDest = {};
    // 默认一个CPU
    INT32 physicalCount = 1;
    mmCpuDesc *pCpuDesc = NULL;
    struct utsname sysInfo = {};

    LocalGetCpuProc(&cpuDest, &physicalCount);

    if ((physicalCount < MMPA_MIN_PHYSICALCPU_COUNT) || (physicalCount > MMPA_MAX_PHYSICALCPU_COUNT)) {
        return EN_ERROR;
    }
    UINT32 needSize = (UINT32)(physicalCount) * (UINT32)(sizeof(mmCpuDesc));

    pCpuDesc = (mmCpuDesc*)malloc(needSize);
    if (pCpuDesc == NULL) {
        return EN_ERROR;
    }

    (VOID)memset_s(pCpuDesc, needSize, 0, needSize); /* unsafe_function_ignore: memset */

    if (uname(&sysInfo) == EN_OK) {
        ret = memcpy_s(cpuDest.arch, sizeof(cpuDest.arch), sysInfo.machine, strlen(sysInfo.machine) + 1U);
        if (ret != EN_OK) {
            free(pCpuDesc);
            return EN_ERROR;
        }
    }

    INT32 cpuCnt = physicalCount;
    for (i = 0; i < cpuCnt; i++) {
        pCpuDesc[i] = cpuDest;
        // 平均逻辑CPU个数
        pCpuDesc[i].ncounts = pCpuDesc[i].ncounts / cpuCnt;
    }

    *cpuInfo = pCpuDesc;
    *count = cpuCnt;
    return EN_OK;
}

/*
 * 描述:释放mmGetCpuInfo生成的动态内存
 * 参数:cpuInfo--mmGetCpuInfo获取到的信息的结构体指针
 *      count--mmGetCpuInfo获取到的物理cpu个数
 * 返回值:执行成功返回EN_OK, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmCpuInfoFree(mmCpuDesc *cpuInfo, INT32 count)
{
    if ((cpuInfo == NULL) || (count == MMPA_ZERO)) {
        return EN_INVALID_PARAM;
    }
    (VOID)free(cpuInfo);
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
INT32 mmCreateProcess(const CHAR *fileName, const mmArgvEnv *env, const CHAR* stdoutRedirectFile, mmProcess *id)
{
    if ((id == NULL) || (fileName == NULL)) {
        return EN_INVALID_PARAM;
    }
    pid_t child = fork();
    if (child == EN_ERROR) {
        return EN_ERROR;
    }

    if (child == MMPA_ZERO) {
        if (stdoutRedirectFile != NULL) {
            INT32 fd = open(stdoutRedirectFile, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
            if (fd != EN_ERROR) {
                (VOID)mmDup2(fd, 1); // 1: standard output
                (VOID)mmDup2(fd, 2); // 2: error output
                (VOID)mmClose(fd);
            }
        }
        CHAR * const *  argv = NULL;
        CHAR * const *  envp = NULL;
        if (env != NULL) {
            if (env->argv != NULL) {
                argv = env->argv;
            }
            if (env->envp != NULL) {
                envp = env->envp;
            }
        }
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-function-declaration"
        if (execvpe(fileName, argv, envp) < MMPA_ZERO) {
#pragma GCC diagnostic pop
            exit(1);
        }
        // 退出,不会运行至此
    }
    *id = child;
    return EN_OK;
}

/*
 * 描述:mmCreateTaskWithThreadAttr内部使用,设置线程调度相关属性
 * 参数: attr -- 线程属性结构体
 *       threadAttr -- 包含需要设置的线程属性类别和值
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
static INT32 LocalSetSchedAttr(pthread_attr_t *attr, const mmThreadAttr *threadAttr)
{
#ifndef __ANDROID__
    // 设置默认继承属性 PTHREAD_EXPLICIT_SCHED 使得调度属性生效
    if ((threadAttr->policyFlag == TRUE) || (threadAttr->priorityFlag == TRUE)) {
        if (pthread_attr_setinheritsched(attr, PTHREAD_EXPLICIT_SCHED) != EN_OK) {
            return EN_ERROR;
        }
    }
#endif

        // 设置调度策略
        if (threadAttr->policyFlag == TRUE) {
            if ((threadAttr->policy != MMPA_THREAD_SCHED_FIFO) && (threadAttr->policy != MMPA_THREAD_SCHED_OTHER) &&
                (threadAttr->policy != MMPA_THREAD_SCHED_RR)) {
                return EN_INVALID_PARAM;
            }
            if (pthread_attr_setschedpolicy(attr, threadAttr->policy) != EN_OK) {
                return EN_ERROR;
            }
        }

    // 设置优先级
    if (threadAttr->priorityFlag == TRUE) {
        if ((threadAttr->priority < MMPA_MIN_THREAD_PIO) || (threadAttr->priority > MMPA_MAX_THREAD_PIO)) {
            return EN_INVALID_PARAM;
        }
        struct sched_param param;
        (VOID)memset_s(&param, sizeof(param), 0, sizeof(param)); /* unsafe_function_ignore: memset */
        param.sched_priority = threadAttr->priority;
        if (pthread_attr_setschedparam(attr, &param) != EN_OK) {
            return EN_ERROR;
        }
    }
    return EN_OK;
}

/*
 * 描述:mmCreateTaskWithThreadAttr内部使用,创建带属性的线程, 支持调度策略、优先级、线程栈大小属性设置
 * 参数: attr -- 线程属性结构体
 *       threadAttr -- 包含需要设置的线程属性类别和值
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
static INT32 LocalSetThreadAttr(pthread_attr_t *attr, const mmThreadAttr *threadAttr)
{
    // 设置调度相关属性
    INT32 ret = LocalSetSchedAttr(attr, threadAttr);
    if (ret != EN_OK) {
        return ret;
    }

    // 设置堆栈
    if (threadAttr->stackFlag == TRUE) {
        if (threadAttr->stackSize < MMPA_THREAD_MIN_STACK_SIZE) {
            return EN_INVALID_PARAM;
        }
        if (pthread_attr_setstacksize(attr, threadAttr->stackSize) != EN_OK) {
            return EN_ERROR;
        }
    }
    if (threadAttr->detachFlag == TRUE) {
        // 设置默认线程分离属性
        if (pthread_attr_setdetachstate(attr, PTHREAD_CREATE_DETACHED) != EN_OK) {
            return EN_ERROR;
        }
    }
    return EN_OK;
}


/*
 * 描述:创建带属性的线程, 支持调度策略、优先级、线程栈大小、分离属性设置,除了分离属性,其他只在Linux下有效
 * 参数: threadHandle -- pthread_t类型的实例
 *       funcBlock --包含函数名和参数的结构体指针
 *       threadAttr -- 包含需要设置的线程属性类别和值
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmCreateTaskWithThreadAttr(mmThread *threadHandle, const mmUserBlock_t *funcBlock,
    const mmThreadAttr *threadAttr)
{
    if ((threadHandle == NULL) || (funcBlock == NULL) ||
        (funcBlock->procFunc == NULL) || (threadAttr == NULL)) {
        return EN_INVALID_PARAM;
    }

    pthread_attr_t attr;
    (VOID)memset_s(&attr, sizeof(attr), 0, sizeof(attr)); /* unsafe_function_ignore: memset */

    // 初始化线程属性
    INT32 ret = pthread_attr_init(&attr);
    if (ret != EN_OK) {
        return EN_ERROR;
    }

    ret = LocalSetThreadAttr(&attr, threadAttr);
    if (ret != EN_OK) {
        (VOID)pthread_attr_destroy(&attr);
        return ret;
    }

    ret = pthread_create(threadHandle, &attr, funcBlock->procFunc, funcBlock->pulArg);
    (VOID)pthread_attr_destroy(&attr);
    if (ret != EN_OK) {
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
    (VOID)name;
    (VOID)oflag;
    (VOID)mode;
    return EN_ERROR;
}

/*
 * 描述:删除mmShmOpen创建的文件
 * 参数:name--文件路径
 * 返回值:执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
 */
INT32 mmShmUnlink(const CHAR *name)
{
    (VOID)name;
    return EN_ERROR;
}

/*
* 描述： 将一个文件或者其它对象映射进内存(系统决定映射区的起始地址)
* 参数： fd--有效的文件句柄
*        size--映射区的长度
*        offeet--被映射对象内容的起点
*        extra--(出参)CreateFileMapping()返回的文件映像对象句柄，此参数linux不使用，windows为mmMumMap入参，释放资源
*        prot--期望的内存保护标志，不能与文件的打开模式冲突，此参数linux使用，windows不使用
*        flags--指定映射对象的类型，映射选项和映射页是否可以共享，此参数linux使用，windows不使用
* 返回值：成功执行时，返回被映射区的指针；失败返回NULL
*/
VOID *mmMmap(mmFd_t fd, mmSize_t size, mmOfft_t offset, mmFd_t *extra, INT32 prot, INT32 flags)
{
    if (size == 0) {
        return NULL;
    }
    VOID *data = mmap(NULL, size, prot, flags, fd, offset);
    if (data == MAP_FAILED) {
        return NULL;
    }
    return data;
}

/*
* 描述：取消已映射内存的地址(系统决定映射区的起始地址)
* 参数： data--需要取消映射的内存地址
*        size--欲取消的内存大小
*        extra--CreateFileMapping()返回的文件映像对象句柄，此参数linux不使用，windows为释放资源参数
* 返回值：执行成功返回EN_OK, 执行错误返回EN_ERROR, 入参检查错误返回EN_INVALID_PARAM
*/
INT32 mmMunMap(VOID *data, mmSize_t size, mmFd_t *extra)
{
    if ((data == NULL) || (size == 0)) {
        return EN_INVALID_PARAM;
    }
    INT32 ret = munmap(data, size);
    if (ret < MMPA_ZERO) {
        return EN_ERROR;
    }
    return EN_OK;
}

/*
* 描述： 获取系统内存页大小
* 返回值：返系统回内存页大小
*/
mmSize mmGetPageSize(VOID)
{
    return (mmSize)(LONG)getpagesize();
}

/*
* 描述： 申请和内存页对齐的内存
* 参数： mallocSize--申请内存大小
         alignSize--内存对齐的大小
* 返回值：成功执行时，返回内存地址；失败返回NULL
*/
VOID *mmAlignMalloc(mmSize mallocSize, mmSize alignSize)
{
    return memalign(alignSize, mallocSize);
}

/*
* 描述： 释放内存页对齐的内存
* 参数： addr--内存地址
*/
VOID mmAlignFree(VOID *addr)
{
    if (addr != NULL) {
        free(addr);
    }
}

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cpluscplus */
#endif /* __cpluscplus */

