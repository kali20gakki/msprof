#include "mmpa_plugin.h"
namespace Analysis {
namespace Dvvp {
namespace Plugin {

MmpaPlugin::~MmpaPlugin() {}

bool MmpaPlugin::IsFuncExist(const std::string &funcName) const
{
    return true;
}

INT32 MmpaPlugin::MsprofMmOpen2(const CHAR *pathName, INT32 flags, MODE mode)
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

mmSsize_t MmpaPlugin::MsprofMmRead(INT32 fd, VOID *buf, UINT32 bufLen)
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

mmSsize_t MmpaPlugin::MsprofMmWrite(INT32 fd, VOID *buf, UINT32 bufLen)
{
    if ((fd < MMPA_ZERO) || (buf == NULL)) {
        return EN_INVALID_PARAM;
    }

    INT32 ret = (INT32)write(fd, buf,(size_t)bufLen);
    if (ret < MMPA_ZERO) {
        return EN_ERROR;
    }
    return ret;
}

INT32 MmpaPlugin::MsprofMmClose(INT32 fd)
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

CHAR *MmpaPlugin::MsprofMmGetErrorFormatMessage(mmErrorMsg errnum, CHAR *buf, mmSize size)
{
    if ((buf == NULL) || (size <= 0)) {
        return NULL;
    }
    return strerror_r(errnum, buf, size);
}

INT32 MmpaPlugin::MsprofMmGetErrorCode()
{
    INT32 ret = (INT32)errno;
    return ret;
}

INT32 MmpaPlugin::MsprofMmAccess2(const CHAR *pathName, INT32 mode)
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

INT32 MmpaPlugin::MsprofMmGetOptLong(INT32 argc,
                   CHAR *const *argv,
                   const CHAR *opts,
                   const mmStructOption *longOpts,
                   INT32 *longIndex)
{
    // return getopt_long(argc, argv, opts, longOpts, longIndex);
    return 0;
}

INT32 MmpaPlugin::MsprofMmGetOptInd()
{
    // return optind;
    return 0;
}

CHAR *MmpaPlugin::MsprofMmGetOptArg()
{
    // return optarg;
    return 0;
}

INT32 MmpaPlugin::MsprofMmSleep(UINT32 milliSecond)
{
    if (milliSecond == MMPA_ZERO) {
        return(EN_INVALID_PARAM);
    }
    unsigned int microSecond;
    if (milliSecond <= MMPA_MAX_SLEEP_MILLSECOND) {
        microSecond = milliSecond * MMPA_ONE_THOUSAND;
    } else { // 防止截断
        microSecond = 0xffffffff;
    }
    INT32 ret = usleep(microSecond);
    if (ret != EN_OK) {
        return EN_ERROR;
    }
    return EN_OK;
}

INT32 MmpaPlugin::MsprofMmUnlink(const CHAR *filename)
{
    if (filename == NULL) {
        return EN_INVALID_PARAM;
    }

    return unlink(filename);
}

INT32 MmpaPlugin::MsprofMmGetTid()
{
    INT32 ret = (INT32)syscall(SYS_gettid);
    if (ret < MMPA_ZERO) {
        return EN_ERROR;
    }
    return ret;
}

INT32 MmpaPlugin::MsprofMmGetTimeOfDay(mmTimeval *timeVal, mmTimezone *timeZone)
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

mmTimespec MmpaPlugin::MsprofMmGetTickCount()
{
    mmTimespec rts;
    struct timespec ts = {0};
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    rts.tv_sec = ts.tv_sec;
    rts.tv_nsec = ts.tv_nsec;
    return rts;
}

INT32 MmpaPlugin::MsprofMmGetDiskFreeSpace(const CHAR *path, mmDiskSize *diskSize)
{
    // if ((path == NULL) || (diskSize == NULL)) {
    //     return EN_INVALID_PARAM;
    // }

    // // 把文件系统信息读入 struct statvfs buf 中
    // struct statvfs buf;
    // (VOID)memset_s(&buf, sizeof(buf), 0, sizeof(buf)); /* unsafe_function_ignore: memset */

    // INT32 ret = statvfs(path, &buf);
    // if (ret == MMPA_ZERO) {
    //     diskSize->totalSize = (ULONGLONG)(buf.f_blocks) * (ULONGLONG)(buf.f_bsize);
    //     diskSize->availSize = (ULONGLONG)(buf.f_bavail) * (ULONGLONG)(buf.f_bsize);
    //     diskSize->freeSize = (ULONGLONG)(buf.f_bfree) * (ULONGLONG)(buf.f_bsize);
    //     return EN_OK;
    // }
    // return EN_ERROR;
    return 0;
}

INT32 MmpaPlugin::MsprofMmIsDir(const CHAR *fileName)
{
    if (fileName == NULL) {
        return EN_INVALID_PARAM;
    }
    struct stat fileStat;
    (void)memset(&fileStat, 0, sizeof(fileStat)); /* unsafe_function_ignore: memset */
    int ret = lstat(fileName, &fileStat);
    if (ret < 0) {
        return EN_ERROR;
    }

    if (!S_ISDIR(fileStat.st_mode)) {
        return EN_ERROR;
    }
    return EN_OK;
}

INT32 MmpaPlugin::MsprofMmAccess(const CHAR *pathName)
{
    return MsprofMmAccess2(pathName, F_OK);
}

CHAR *MmpaPlugin::MsprofMmDirName(CHAR *path)
{
    if (path == NULL) {
        return NULL;
    }
    return dirname(path);
}

CHAR *MmpaPlugin::MsprofMmBaseName(CHAR *path)
{
    if (path == NULL) {
        return NULL;
    }
    return basename(path);
}

INT32 MmpaPlugin::MsprofMmMkdir(const CHAR *pathName, mmMode_t mode)
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

INT32 MmpaPlugin::MsprofMmChmod(const CHAR *filename, INT32 mode)
{
    if (filename == NULL) {
        return EN_INVALID_PARAM;
    }
    return chmod(filename, (UINT32)mode);
}

INT32 MmpaPlugin::MsprofMmScandir(const CHAR *path, mmDirent ***entryList, mmFilter filterFunc, mmSort sort)
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

INT32 MmpaPlugin::MsprofMmRmdir(const CHAR *pathName)
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
            (VOID)MsprofMmRmdir(buf);
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

INT32 MmpaPlugin::MsprofMmRealPath(const CHAR *path, CHAR *realPath, INT32 realPathLen)
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

INT32 MmpaPlugin::MsprofMmCreateProcess(const CHAR *fileName, const mmArgvEnv *env, const CHAR* stdoutRedirectFile, mmProcess *id)
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
                (VOID)MsprofMmDup2(fd, 1); // 1: standard output
                (VOID)MsprofMmDup2(fd, 2); // 2: error output
                (VOID)MsprofMmClose(fd);
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

INT32 MmpaPlugin::MsprofMmDup2(INT32 oldFd, INT32 newFd)
{
    if (oldFd <= MMPA_ZERO || newFd < MMPA_ZERO) {
        return EN_INVALID_PARAM;
    }
    INT32 ret = dup2(oldFd, newFd);
    if (ret == EN_ERROR) {
        return EN_ERROR;
    }
    return EN_OK;
}

VOID MmpaPlugin::MsprofMmScandirFree(mmDirent **entryList, INT32 count)
{
    // if (entryList == NULL) {
    //     return;
    // }
    // INT32 j;
    // for (j = 0; j < count; j++) {
    //     if (entryList[j] != NULL) {
    //         free(entryList[j]);
    //         entryList[j] = NULL;
    //     }
    // }
    // free(entryList);
}

INT32 MmpaPlugin::MsprofMmChdir(const CHAR *path)
{
    // if (path == NULL) {
    //     return EN_INVALID_PARAM;
    // }
    // return chdir(path);
    return 0;
}

INT32 MmpaPlugin::MsprofMmWaitPid(mmProcess pid, INT32 *status, INT32 options)
{
    // if ((options != MMPA_ZERO) && (options != M_WAIT_NOHANG) && (options != M_WAIT_UNTRACED)) {
    //     return EN_INVALID_PARAM;
    // }

    // INT32 ret = waitpid(pid, status, options);
    // if (ret == EN_ERROR) {
    //     return EN_ERROR;                 // 调用异常
    // }
    // if ((ret > MMPA_ZERO) && (ret == pid)) { // 返回了子进程ID
    //     if (status != NULL) {
    //         if (WIFEXITED(*status)) {
    //             *status = WEXITSTATUS(*status); // 正常退出码
    //         }
    //         if (WIFSIGNALED(*status)) {
    //             *status = WTERMSIG(*status); // 信号中止退出码
    //         }
    //     }
    //     return EN_ERR;                  // 进程结束
    // }
    // return EN_OK; // 进程未结束
    return 0;
}

INT32 MmpaPlugin::MsprofMmGetMac(mmMacInfo **list, INT32 *count)
{
    // if ((list == NULL) || (count == NULL)) {
    //     return EN_INVALID_PARAM;
    // }
    // struct ifreq ifr;
    // CHAR buf[MMPA_MAX_IF_SIZE] = {0};
    // struct ifconf ifc;
    // mmMacInfo *macInfo = NULL;
    // INT32 sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    // if (sock == EN_ERROR) {
    //     return EN_ERROR;
    // }
    // ifc.ifc_buf = buf;
    // ifc.ifc_len = (INT)sizeof(buf);
    // INT32 ret = ioctl(sock, SIOCGIFCONF, &ifc);
    // if (ret == EN_ERROR) {
    //     (VOID)close(sock);
    //     return EN_ERROR;
    // }

    // INT32 len = (INT32)sizeof(struct ifreq);
    // const struct ifreq* it = ifc.ifc_req;
    // *count = (ifc.ifc_len / len);
    // UINT32 cnt = (UINT32)(*count);
    // size_t needSize = (size_t)(cnt) * sizeof(mmMacInfo);

    // macInfo = (mmMacInfo*)malloc(needSize);
    // if (macInfo == NULL) {
    //     *count = MMPA_ZERO;
    //     (VOID)close(sock);
    //     return EN_ERROR;
    // }

    // (VOID)memset_s(macInfo, needSize, 0, needSize); /* unsafe_function_ignore: memset */
    // const struct ifreq* const end = it + *count;
    // INT32 i = 0;
    // for (; it != end; ++it) {
    //     ret = strcpy_s(ifr.ifr_name, sizeof(ifr.ifr_name), it->ifr_name);
    //     if (ret != EOK) {
    //         *count = MMPA_ZERO;
    //         (VOID)close(sock);
    //         (VOID)free(macInfo);
    //         return EN_ERROR;
    //     }
    //     if (ioctl(sock, SIOCGIFFLAGS, &ifr) != MMPA_ZERO) {
    //         continue;
    //     }
    //     UINT16 ifrFlag = (UINT16)ifr.ifr_flags;
    //     if ((ifrFlag & IFF_LOOPBACK) == MMPA_ZERO) { // ignore loopback
    //         ret = ioctl(sock, SIOCGIFHWADDR, &ifr);
    //         if (ret != MMPA_ZERO) {
    //             continue;
    //         }
    //         const UCHAR *ptr = (UCHAR *)&ifr.ifr_ifru.ifru_hwaddr.sa_data[0];
    //         ret = snprintf_s(macInfo[i].addr,
    //             sizeof(macInfo[i].addr), sizeof(macInfo[i].addr) - 1U,
    //             "%02X-%02X-%02X-%02X-%02X-%02X",
    //             *(ptr + MMPA_MAC_ADDR_FIRST_BYTE),
    //             *(ptr + MMPA_MAC_ADDR_SECOND_BYTE),
    //             *(ptr + MMPA_MAC_ADDR_THIRD_BYTE),
    //             *(ptr + MMPA_MAC_ADDR_FOURTH_BYTE),
    //             *(ptr + MMPA_MAC_ADDR_FIFTH_BYTE),
    //             *(ptr + MMPA_MAC_ADDR_SIXTH_BYTE));
    //         if (ret == EN_ERROR) {
    //             *count = MMPA_ZERO;
    //             (VOID)close(sock);
    //             (VOID)free(macInfo);
    //             return EN_ERROR;
    //         }
    //         i++;
    //     } else {
    //         *count = *count - 1;
    //     }
    // }
    // (VOID)close(sock);
    // if (*count <= 0) {
    //     (VOID)free(macInfo);
    //     return EN_ERROR;
    // } else {
    //     *list = macInfo;
    //     return EN_OK;
    // }
    return 0;
}

INT32 MmpaPlugin::MsprofMmGetMacFree(mmMacInfo *list, INT32 count)
{
    // if ((list == NULL) || (count < MMPA_ZERO)) {
    //     return EN_INVALID_PARAM;
    // }
    // (VOID)free(list);
    // return EN_OK;
    return 0;
}

INT32 MmpaPlugin::MsprofMmGetLocalTime(mmSystemTime_t *sysTimePtr)
{
    // if (sysTimePtr == NULL) {
    //     return EN_INVALID_PARAM;
    // }

    // struct timeval timeVal;
    // (VOID)memset_s(&timeVal, sizeof(timeVal), 0, sizeof(timeVal)); /* unsafe_function_ignore: memset */

    // INT32 ret = gettimeofday(&timeVal, NULL);
    // if (ret != EN_OK) {
    //     return EN_ERROR;
    // }

    // struct tm nowTime;
    // (VOID)memset_s(&nowTime, sizeof(nowTime), 0, sizeof(nowTime)); /* unsafe_function_ignore: memset */

    // const struct tm *tmp = localtime_r(&timeVal.tv_sec, &nowTime);
    // if (tmp == NULL) {
    //     return EN_ERROR;
    // }

    // sysTimePtr->wSecond = nowTime.tm_sec;
    // sysTimePtr->wMinute = nowTime.tm_min;
    // sysTimePtr->wHour = nowTime.tm_hour;
    // sysTimePtr->wDay = nowTime.tm_mday;
    // sysTimePtr->wMonth = nowTime.tm_mon + 1; // in localtime month is [0, 11], but in fact month is [1, 12]
    // sysTimePtr->wYear = nowTime.tm_year + MMPA_COMPUTER_BEGIN_YEAR;
    // sysTimePtr->wDayOfWeek = nowTime.tm_wday;
    // sysTimePtr->tm_yday = nowTime.tm_yday;
    // sysTimePtr->tm_isdst = nowTime.tm_isdst;
    // sysTimePtr->wMilliseconds = timeVal.tv_usec / MMPA_MSEC_TO_USEC;

    // return EN_OK;
    return 0;
}

INT32 MmpaPlugin::MsprofMmGetPid()
{
    // return (INT32)getpid();
    return 0;
}

INT32 MmpaPlugin::MsprofMmGetCwd(CHAR *buffer, INT32 maxLen)
{
    // if ((buffer == NULL) || (maxLen < MMPA_ZERO)) {
    //     return EN_INVALID_PARAM;
    // }
    // const CHAR *ptr = getcwd(buffer, (UINT32)maxLen);
    // if (ptr != NULL) {
    //     return EN_OK;
    // } else {
    //     return EN_ERROR;
    // }
    return 0;
}

INT32 MmpaPlugin::MsprofMmGetEnv(const CHAR *name, CHAR *value, UINT32 len)
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

INT32 LocalSetThreadAttr(pthread_attr_t *attr,const mmThreadAttr *threadAttr)
{
#ifndef __ANDROID__
        // 设置默认继承属性 PTHREAD_EXPLICIT_SCHED 使得调度属性生效
        if(threadAttr->policyFlag == true || threadAttr->priorityFlag == true) {
            if (pthread_attr_setinheritsched(attr, PTHREAD_EXPLICIT_SCHED) != EN_OK) {
                return EN_ERROR;
            }
        }
#endif

    // 设置调度策略
    if (threadAttr->policyFlag == true) {
        if (threadAttr->policy != MMPA_THREAD_SCHED_FIFO && threadAttr->policy != MMPA_THREAD_SCHED_OTHER &&
            threadAttr->policy != MMPA_THREAD_SCHED_RR) {
            return EN_INVALID_PARAM;
        }
        if (pthread_attr_setschedpolicy(attr, threadAttr->policy) != EN_OK) {
            return EN_ERROR;
        }
    }

    // 设置优先级
    if (threadAttr->priorityFlag == true) {
        if (threadAttr->priority < MMPA_MIN_THREAD_PIO || threadAttr->priority > MMPA_MAX_THREAD_PIO) {
            return EN_INVALID_PARAM;
        }
        struct sched_param param;
        (void)memset_s(&param, sizeof(param), 0, sizeof(param));
        param.sched_priority = threadAttr->priority;
        if (pthread_attr_setschedparam(attr, &param) != EN_OK) {
            return EN_ERROR;
        }
    }

    // 设置堆栈
    if (threadAttr->stackFlag == true) {
        if (threadAttr->stackSize < MMPA_THREAD_MIN_STACK_SIZE) {
            return EN_INVALID_PARAM;
        }
        if (pthread_attr_setstacksize(attr, threadAttr->stackSize) != EN_OK) {
            return EN_ERROR;
        }
    }
    if (threadAttr->detachFlag == true) {
        // 设置默认线程分离属性
        if (pthread_attr_setdetachstate(attr, PTHREAD_CREATE_DETACHED) != EN_OK) {
            return EN_ERROR;
        }
    }
    return EN_OK;
}

INT32 MmpaPlugin::MsprofMmCreateTaskWithThreadAttr(mmThread *threadHandle, const mmUserBlock_t *funcBlock,
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

INT32 MmpaPlugin::MsprofMmJoinTask(mmThread *threadHandle)
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

INT32 MmpaPlugin::MsprofMmSetCurrentThreadName(const CHAR* name)
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

INT32 MmpaPlugin::MsprofMmStatGet(const CHAR *path, mmStat_t *buffer)
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

INT32 MmpaPlugin::MsprofMmGetOsVersion(CHAR* versionInfo, INT32 versionLength)
{
    // if ((versionInfo == NULL) || (versionLength < MMPA_MIN_OS_VERSION_SIZE)) {
    //     return EN_INVALID_PARAM;
    // }
    // INT32 ret = 0;
    // struct utsname sysInfo;
    // (VOID)memset_s(&sysInfo, sizeof(sysInfo), 0, sizeof(sysInfo)); /* unsafe_function_ignore: memset */
    // UINT32 length = (UINT32)versionLength;
    // size_t len = (size_t)length;
    // INT32 fb = uname(&sysInfo);
    // if (fb < MMPA_ZERO) {
    //     return EN_ERROR;
    // } else {
    //     ret = snprintf_s(versionInfo, len, (len - 1U), "%s-%s-%s",
    //         sysInfo.sysname, sysInfo.release, sysInfo.version);
    //     if (ret == EN_ERROR) {
    //         return EN_ERROR;
    //     }
    //     return EN_OK;
    // }
    return 0;
}

INT32 MmpaPlugin::MsprofMmGetOsName(CHAR* name, INT32 nameSize)
{
    // if ((name == NULL) || (nameSize < MMPA_MIN_OS_NAME_SIZE)) {
    //     return EN_INVALID_PARAM;
    // }
    // UINT32 length = (UINT32)nameSize;
    // INT32 ret = gethostname(name, length);
    // if (ret < MMPA_ZERO) {
    //     return EN_ERROR;
    // }
    // return EN_OK;
    return 0;
}

INT32 MmpaPlugin::MsprofMmGetCpuInfo(mmCpuDesc **cpuInfo, INT32 *count)
{
    // if (count == NULL) {
    //     return EN_INVALID_PARAM;
    // }
    // if (cpuInfo == NULL) {
    //     return EN_INVALID_PARAM;
    // }
    // INT32 i = 0;
    // INT32 ret = 0;
    // mmCpuDesc cpuDest = {};
    // // 默认一个CPU
    // INT32 physicalCount = 1;
    // mmCpuDesc *pCpuDesc = NULL;
    // struct utsname sysInfo = {};

    // LocalGetCpuProc(&cpuDest, &physicalCount);

    // if ((physicalCount < MMPA_MIN_PHYSICALCPU_COUNT) || (physicalCount > MMPA_MAX_PHYSICALCPU_COUNT)) {
    //     return EN_ERROR;
    // }
    // UINT32 needSize = (UINT32)(physicalCount) * (UINT32)(sizeof(mmCpuDesc));

    // pCpuDesc = (mmCpuDesc*)malloc(needSize);
    // if (pCpuDesc == NULL) {
    //     return EN_ERROR;
    // }

    // (VOID)memset_s(pCpuDesc, needSize, 0, needSize); /* unsafe_function_ignore: memset */

    // if (uname(&sysInfo) == EN_OK) {
    //     ret = memcpy_s(cpuDest.arch, sizeof(cpuDest.arch), sysInfo.machine, strlen(sysInfo.machine) + 1U);
    //     if (ret != EN_OK) {
    //         free(pCpuDesc);
    //         return EN_ERROR;
    //     }
    // }

    // INT32 cpuCnt = physicalCount;
    // for (i = 0; i < cpuCnt; i++) {
    //     pCpuDesc[i] = cpuDest;
    //     // 平均逻辑CPU个数
    //     pCpuDesc[i].ncounts = pCpuDesc[i].ncounts / cpuCnt;
    // }

    // *cpuInfo = pCpuDesc;
    // *count = cpuCnt;
    // return EN_OK;
    return 0;
}

INT32 MmpaPlugin::MsprofMmCpuInfoFree(mmCpuDesc *cpuInfo, INT32 count)
{
    // if ((cpuInfo == NULL) || (count == MMPA_ZERO)) {
    //     return EN_INVALID_PARAM;
    // }
    // (VOID)free(cpuInfo);
    // return EN_OK;
    return 0;
}

INT32 MmpaPlugin::MsprofMmGetFileSize(const CHAR *fileName, ULONGLONG *length)
{
    if(fileName == NULL || length == NULL){
        return EN_INVALID_PARAM;
    }
    struct stat fileStat;
    (void)memset(&fileStat, 0, sizeof(fileStat)); /* unsafe_function_ignore: memset */
    int ret = lstat(fileName, &fileStat);
    if (ret < 0) {
        return EN_ERROR;
    }
    *length = (ULONGLONG)fileStat.st_size;
    return EN_OK;
}
} // Plugin
} // Dvvp
} // Analysis