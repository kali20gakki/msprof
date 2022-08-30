/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: Linux interface wrapper
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2022-06-09
 */
 
#include "mmpa_api.h"
#include "errno/error_code.h"

namespace Collector {
namespace Dvvp {
namespace Mmpa {
using namespace analysis::dvvp::common::error;
int32_t MmSleep(uint32_t milliSecond)
{
    if (milliSecond == 0) {
        return PROFILING_INVALID_PARAM;
    }
    uint32_t microSecond;

    if (milliSecond <= MMPA_MAX_SLEEP_MILLSECOND_USING_USLEEP) {
        microSecond = milliSecond * static_cast<uint32_t>(MMPA_MSEC_TO_USEC);
    } else {
        microSecond = MMPA_MAX_SLEEP_MICROSECOND_USING_USLEEP;
    }

    int32_t ret = usleep(microSecond);
    if (ret != PROFILING_SUCCESS) {
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

static int32_t LocalSetSchedAttr(pthread_attr_t *attr, const MmThreadAttr *threadAttr)
{
#ifndef __ANDROID__
    if ((threadAttr->policyFlag == TRUE) || (threadAttr->priorityFlag == TRUE)) {
        if (pthread_attr_setinheritsched(attr, PTHREAD_EXPLICIT_SCHED) != PROFILING_SUCCESS) {
            return PROFILING_FAILED;
        }
    }
#endif

        if (threadAttr->policyFlag == TRUE) {
            if ((threadAttr->policy != MMPA_THREAD_SCHED_FIFO) && (threadAttr->policy != MMPA_THREAD_SCHED_OTHER) &&
                (threadAttr->policy != MMPA_THREAD_SCHED_RR)) {
                return PROFILING_INVALID_PARAM;
            }
            if (pthread_attr_setschedpolicy(attr, threadAttr->policy) != PROFILING_SUCCESS) {
                return PROFILING_FAILED;
            }
        }

    if (threadAttr->priorityFlag == TRUE) {
        if ((threadAttr->priority < MMPA_MIN_THREAD_PIO) || (threadAttr->priority > MMPA_MAX_THREAD_PIO)) {
            return PROFILING_INVALID_PARAM;
        }
        struct sched_param param;
        (void)memset_s(&param, sizeof(param), 0, sizeof(param)); /* unsafe_function_ignore: memset */
        param.sched_priority = threadAttr->priority;
        if (pthread_attr_setschedparam(attr, &param) != PROFILING_SUCCESS) {
            return PROFILING_FAILED;
        }
    }
    return PROFILING_SUCCESS;
}

static int32_t LocalSetThreadAttr(pthread_attr_t *attr, const MmThreadAttr *threadAttr)
{
    int32_t ret = LocalSetSchedAttr(attr, threadAttr);
    if (ret != PROFILING_SUCCESS) {
        return ret;
    }

    if (threadAttr->stackFlag == TRUE) {
        if (threadAttr->stackSize < MMPA_THREAD_MIN_STACK_SIZE) {
            return PROFILING_INVALID_PARAM;
        }
        if (pthread_attr_setstacksize(attr, threadAttr->stackSize) != PROFILING_SUCCESS) {
            return PROFILING_FAILED;
        }
    }
    if (threadAttr->detachFlag == TRUE) {
        if (pthread_attr_setdetachstate(attr, PTHREAD_CREATE_DETACHED) != PROFILING_SUCCESS) {
            return PROFILING_FAILED;
        }
    }
    return PROFILING_SUCCESS;
}

int32_t MmCreateTaskWithThreadAttr(MmThread *threadHandle, const MmUserBlockT *funcBlock,
    const MmThreadAttr *threadAttr)
{
    if ((threadHandle == nullptr) || (funcBlock == nullptr) ||
        (funcBlock->procFunc == nullptr) || (threadAttr == nullptr)) {
        return PROFILING_INVALID_PARAM;
    }

    pthread_attr_t attr;
    (void)memset_s(&attr, sizeof(attr), 0, sizeof(attr)); /* unsafe_function_ignore: memset */

    int32_t ret = pthread_attr_init(&attr);
    if (ret != PROFILING_SUCCESS) {
        return PROFILING_FAILED;
    }

    ret = LocalSetThreadAttr(&attr, threadAttr);
    if (ret != PROFILING_SUCCESS) {
        (void)pthread_attr_destroy(&attr);
        return ret;
    }

    ret = pthread_create(threadHandle, &attr, funcBlock->procFunc, funcBlock->pulArg);
    (void)pthread_attr_destroy(&attr);
    if (ret != PROFILING_SUCCESS) {
        ret = PROFILING_FAILED;
    }
    return ret;
}
 
int32_t MmJoinTask(const MmThread *threadHandle)
{
    if (threadHandle == nullptr) {
        return PROFILING_INVALID_PARAM;
    }

    int32_t ret = pthread_join(*threadHandle, nullptr);
    if (ret != PROFILING_SUCCESS) {
        ret = PROFILING_FAILED;
    }
    return ret;
}

int32_t MmSetCurrentThreadName(const std::string &name)
{
    if (name.empty()) {
        return PROFILING_INVALID_PARAM;
    }
    int32_t ret = prctl(PR_SET_NAME, name.c_str());
    if (ret != PROFILING_SUCCESS) {
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

MmTimespec MmGetTickCount()
{
    MmTimespec rts;
    memset_s(&rts, sizeof(rts), 0, sizeof(rts));
    struct timespec ts;
    memset_s(&ts, sizeof(ts), 0, sizeof(ts));
    (void)clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    rts.tvSec = ts.tv_sec;
    rts.tvNsec = ts.tv_nsec;
    return rts;
}

int32_t MmGetFileSize(const std::string &fileName, unsigned long long *length)
{
    if (fileName.empty() || (length == nullptr)) {
        return PROFILING_INVALID_PARAM;
    }
    struct stat fileStat;
    (void)memset_s(&fileStat, sizeof(fileStat), 0, sizeof(fileStat));
    int32_t ret = lstat(fileName.c_str(), &fileStat);
    if (ret < 0) {
        return PROFILING_FAILED;
    }
    *length = static_cast<unsigned long long>(fileStat.st_size);
    return PROFILING_SUCCESS;
}

int32_t MmGetDiskFreeSpace(const std::string &path, MmDiskSize *diskSize)
{
    if (path.empty() || (diskSize == nullptr)) {
        return PROFILING_INVALID_PARAM;
    }

    struct statvfs buf;
    (void)memset_s(&buf, sizeof(buf), 0, sizeof(buf));

    int32_t ret = statvfs(path.c_str(), &buf);
    if (ret == 0) {
        diskSize->totalSize = static_cast<unsigned long long>(buf.f_blocks) *
            static_cast<unsigned long long>(buf.f_bsize);
        diskSize->availSize = static_cast<unsigned long long>(buf.f_bavail) *
            static_cast<unsigned long long>(buf.f_bsize);
        diskSize->freeSize = static_cast<unsigned long long>(buf.f_bfree) *
            static_cast<unsigned long long>(buf.f_bsize);
        return PROFILING_SUCCESS;
    }
    return PROFILING_FAILED;
}

int32_t MmIsDir(const std::string &fileName)
{
    if (fileName.empty()) {
        return PROFILING_INVALID_PARAM;
    }
    struct stat fileStat;
    (void)memset_s(&fileStat, sizeof(fileStat), 0, sizeof(fileStat));
    int32_t ret = lstat(fileName.c_str(), &fileStat);
    if (ret < 0) {
        return PROFILING_FAILED;
    }

    if (S_ISDIR(fileStat.st_mode) == 0) {
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

int32_t MmAccess2(const std::string &pathName, int32_t mode)
{
    if (pathName.empty()) {
        return PROFILING_INVALID_PARAM;
    }

    int32_t ret = access(pathName.c_str(), mode);
    if (ret != PROFILING_SUCCESS) {
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

int32_t MmAccess(const std::string &pathName)
{
    return MmAccess2(pathName, F_OK);
}

char *MmDirName(char *path)
{
    if (path == nullptr) {
        return nullptr;
    }
    return dirname(path);
}

char *MmBaseName(char *path)
{
    if (path == nullptr) {
        return nullptr;
    }
    return basename(path);
}

int32_t MmMkdir(const std::string &pathName, MmMode_t mode)
{
    if (pathName.empty()) {
        return PROFILING_INVALID_PARAM;
    }

    int32_t ret = mkdir(pathName.c_str(), mode);
    if (ret != PROFILING_SUCCESS) {
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

int32_t MmChmod(const std::string &filename, int32_t mode)
{
    if (filename.empty()) {
        return PROFILING_INVALID_PARAM;
    }

    return chmod(filename.c_str(), static_cast<uint32_t>(mode));
}

int32_t MmGetErrorCode()
{
    int32_t ret = static_cast<int32_t>(errno);
    return ret;
}

char *MmGetErrorFormatMessage(MmErrorMsg errnum, char *buf, size_t size)
{
    if ((buf == nullptr) || (size <= 0)) {
        return nullptr;
    }
    return strerror_r(errnum, buf, size);
}

int32_t MmScandir(const std::string &path, MmDirent ***entryList, MmFilter filterFunc, MmSort sort)
{
    if (path.empty() || (entryList == nullptr)) {
        return PROFILING_INVALID_PARAM;
    }
    int32_t count = scandir(path.c_str(), entryList, filterFunc, sort);
    if (count < 0) {
        return PROFILING_FAILED;
    }
    return count;
}

void MmScandirFree(MmDirent **entryList, int32_t count)
{
    if (entryList == nullptr) {
        return;
    }
    for (int32_t j = 0; j < count; j++) {
        if (entryList[j] != nullptr) {
            free(entryList[j]);
            entryList[j] = nullptr;
        }
    }
    free(entryList);
}

static int32_t LocalRmDirPreCheck(const std::string &pathName)
{
    if (pathName.empty()) {
        return PROFILING_INVALID_PARAM;
    }
    if (pathName.size() > MMPA_MAX_PATH) {
        return PROFILING_INVALID_PARAM;
    }
    return PROFILING_SUCCESS;
}

static int32_t LocalFillPathName(char *buf, size_t bufSize, const std::string &pathName, const struct dirent *entry)
{
    int32_t ret = memset_s(buf, bufSize, 0, bufSize);
    if (ret == PROFILING_FAILED) {
        return ret;
    }
    ret = snprintf_s(buf, bufSize, bufSize - 1U, "%s/%s", pathName.c_str(), entry->d_name);
    if (ret == PROFILING_FAILED) {
        return ret;
    }
    return PROFILING_SUCCESS;
}

int32_t MmRmdir(const std::string &pathName)
{
    int32_t ret = LocalRmDirPreCheck(pathName);
    if (ret != PROFILING_SUCCESS) {
        return PROFILING_INVALID_PARAM;
    }
    const int num = 2;
    size_t bufSize = pathName.size() + static_cast<size_t>(MMPA_MAX_PATH + num);

    DIR *dir = opendir(pathName.c_str());
    if (dir == nullptr) {
        return PROFILING_INVALID_PARAM;
    }

    const struct dirent *entry = nullptr;
    while ((entry = readdir(dir)) != nullptr) {
        if ((strcmp(".", entry->d_name) == 0) || (strcmp("..", entry->d_name) == 0)) {
            continue;
        }
        char *buf = (char *)malloc(bufSize);
        if (buf == nullptr) {
            break;
        }
        ret = LocalFillPathName(buf, bufSize, pathName, entry);
        if (ret != PROFILING_SUCCESS) {
            FREE_BUF(buf);
            break;
        }

        DIR *childDir = opendir(buf);
        if (childDir != nullptr) {
            (void)closedir(childDir);
            (void)MmRmdir(std::string(buf));
            FREE_BUF(buf);
            continue;
        } else {
            ret = unlink(buf);
            if (ret == PROFILING_SUCCESS) {
                free(buf);
                continue;
            }
        }
        FREE_BUF(buf);
    }
    (void)closedir(dir);

    ret = rmdir(pathName.c_str());
    if (ret == PROFILING_FAILED) {
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

int32_t MmUnlink(const std::string &filename)
{
    if (filename.empty()) {
        return PROFILING_INVALID_PARAM;
    }

    return unlink(filename.c_str());
}

int32_t MmDup2(int32_t oldFd, int32_t newFd)
{
    if ((oldFd <= 0) || (newFd < 0)) {
        return PROFILING_INVALID_PARAM;
    }
    int32_t ret = dup2(oldFd, newFd);
    if (ret == PROFILING_FAILED) {
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

int32_t MmRealPath(const char *path, char *realPath, int32_t realPathLen)
{
    int32_t ret = PROFILING_SUCCESS;
    if ((realPath == nullptr) || (path == nullptr) || (realPathLen < MMPA_MAX_PATH)) {
        return PROFILING_INVALID_PARAM;
    }
    const char *ptr = realpath(path, realPath);
    if (ptr == nullptr) {
        ret = PROFILING_FAILED;
    }
    return ret;
}

int32_t MmChdir(const std::string &path)
{
    if (path.empty()) {
        return PROFILING_INVALID_PARAM;
    }
 
    return chdir(path.c_str());
}

int32_t MmCreateProcess(const std::string &fileName, const MmArgvEnv *env,
    const std::string &stdoutRedirectFile, MmProcess *id)
{
    if ((id == nullptr) || fileName.empty()) {
        return PROFILING_INVALID_PARAM;
    }
    pid_t child = fork();
    if (child == PROFILING_FAILED) {
        return PROFILING_FAILED;
    }

    if (child == 0) {
        if (!stdoutRedirectFile.empty()) {
            int32_t fd = open(stdoutRedirectFile.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
            if (fd != PROFILING_FAILED) {
                (void)MmDup2(fd, 1); // 1: standard output
                (void)MmDup2(fd, 2); // 2: error output
                (void)MmClose(fd);
            }
        }
        char * const *  argv = nullptr;
        char * const *  envp = nullptr;
        if (env != nullptr) {
            if (env->argv != nullptr) {
                argv = env->argv;
            }
            if (env->envp != nullptr) {
                envp = env->envp;
            }
        }
        if (execvpe(fileName.c_str(), argv, envp) < 0) {
            return PROFILING_FAILED;
        }
    }
    *id = child;
    return PROFILING_SUCCESS;
}

int32_t MmWaitPid(MmProcess pid, int32_t *status, int32_t options)
{
    if ((options != 0) && (options != M_WAIT_NOHANG) && (options != M_WAIT_UNTRACED)) {
        return PROFILING_INVALID_PARAM;
    }

    int32_t ret = waitpid(pid, status, options);
    if (ret == PROFILING_FAILED) {
        return PROFILING_FAILED;
    }
    if ((ret > 0) && (ret == pid)) {
        if (status != nullptr) {
            if (WIFEXITED(*status)) {
                *status = WEXITSTATUS(*status);
            }
            if (WIFSIGNALED(*status)) {
                *status = WTERMSIG(*status);
            }
        }
        return PROFILING_ERROR;
    }
    return PROFILING_SUCCESS;
}

static int32_t LocalGetIfConf(int32_t sock, struct ifconf *ifc)
{
    char buf[MMPA_MAX_IF_SIZE] = {0};
    ifc->ifc_buf = buf;
    ifc->ifc_len = static_cast<int>(sizeof(buf));
    int32_t ret = ioctl(sock, SIOCGIFCONF, ifc);
    if (ret == PROFILING_FAILED) {
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

static int32_t LocalGetMacInfo(int32_t sock, struct ifconf ifc, MmMacInfo *macInfo, int32_t *count)
{
    int32_t i = 0;
    struct ifreq ifr;
    const struct ifreq* it = ifc.ifc_req;
    const struct ifreq* const end = it + *count;
    for (; it != end; ++it) {
        int32_t ret = strcpy_s(ifr.ifr_name, sizeof(ifr.ifr_name), it->ifr_name);
        if (ret != EOK) {
            *count = 0;
            return PROFILING_FAILED;
        }
        if (ioctl(sock, SIOCGIFFLAGS, &ifr) != 0) {
            continue;
        }
        uint16_t ifrFlag = static_cast<uint16_t>(ifr.ifr_flags);
        if ((ifrFlag & IFF_LOOPBACK) == 0) { // ignore loopback
            ret = ioctl(sock, SIOCGIFHWADDR, &ifr);
            if (ret != 0) {
                continue;
            }
            const unsigned char *ptr = reinterpret_cast<unsigned char *>(&ifr.ifr_ifru.ifru_hwaddr.sa_data[0]);
            ret = snprintf_s(macInfo[i].addr,
                sizeof(macInfo[i].addr), sizeof(macInfo[i].addr) - 1U,
                "%02X-%02X-%02X-%02X-%02X-%02X",
                *(ptr + static_cast<uint32_t>(MMPA_MAC_ADDR_TYPE::MMPA_MAC_ADDR_FIRST_BYTE)),
                *(ptr + static_cast<uint32_t>(MMPA_MAC_ADDR_TYPE::MMPA_MAC_ADDR_SECOND_BYTE)),
                *(ptr + static_cast<uint32_t>(MMPA_MAC_ADDR_TYPE::MMPA_MAC_ADDR_THIRD_BYTE)),
                *(ptr + static_cast<uint32_t>(MMPA_MAC_ADDR_TYPE::MMPA_MAC_ADDR_FOURTH_BYTE)),
                *(ptr + static_cast<uint32_t>(MMPA_MAC_ADDR_TYPE::MMPA_MAC_ADDR_FIFTH_BYTE)),
                *(ptr + static_cast<uint32_t>(MMPA_MAC_ADDR_TYPE::MMPA_MAC_ADDR_SIXTH_BYTE)));
            if (ret == PROFILING_FAILED) {
                *count = 0;
                return PROFILING_FAILED;
            }
            i++;
        } else {
            *count = *count - 1;
        }
    }
    return PROFILING_SUCCESS;
}

int32_t MmGetMac(MmMacInfo **list, int32_t *count)
{
    if ((list == nullptr) || (count == nullptr)) {
        return PROFILING_INVALID_PARAM;
    }
    int32_t sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock == PROFILING_FAILED) {
        return PROFILING_FAILED;
    }
    struct ifconf ifc;
    int32_t ret = LocalGetIfConf(sock, &ifc);
    if (ret != PROFILING_SUCCESS) {
        close(sock);
        return PROFILING_FAILED;
    }
 
    int32_t len = static_cast<int32_t>(sizeof(struct ifreq));
    *count = (ifc.ifc_len / len);
    uint32_t cnt = static_cast<uint32_t>(*count);
    size_t needSize = static_cast<size_t>(cnt) * sizeof(MmMacInfo);
 
    MmMacInfo *macInfo = (MmMacInfo*)malloc(needSize);
    if (macInfo == nullptr) {
        *count = 0;
        (void)close(sock);
        return PROFILING_FAILED;
    }
    (void)memset_s(macInfo, needSize, 0, needSize); /* unsafe_function_ignore: memset */
    ret = LocalGetMacInfo(sock, ifc, macInfo, count);
    if (ret != PROFILING_SUCCESS) {
        close(sock);
        free(macInfo);
        return PROFILING_FAILED;
    }
    (void)close(sock);
    if (*count <= 0) {
        (void)free(macInfo);
        return PROFILING_FAILED;
    } else {
        *list = macInfo;
        return PROFILING_SUCCESS;
    }
}

int32_t MmGetMacFree(MmMacInfo *list, int32_t count)
{
    if ((list == nullptr) || (count < 0)) {
        return PROFILING_INVALID_PARAM;
    }
    (void)free(list);
    return PROFILING_SUCCESS;
}

int32_t MmGetEnv(const std::string &name, char *value, uint32_t len)
{
    int32_t ret;
    uint32_t envLen = 0;
    if (name.empty() || (value == nullptr) || (len == 0)) {
        return PROFILING_INVALID_PARAM;
    }
    const char *envPtr = getenv(name.c_str());
    if (envPtr == nullptr) {
        return PROFILING_FAILED;
    }

    uint32_t lenOfRet = static_cast<uint32_t>(strlen(envPtr));
    if (lenOfRet < static_cast<uint32_t>(MMPA_MEM_MAX_LEN - 1)) {
        envLen = lenOfRet + 1U;
    }

    if ((envLen != 0) && (len < envLen)) {
        return PROFILING_INVALID_PARAM;
    } else {
        ret = memcpy_s(value, len, envPtr, envLen);
        if (ret != PROFILING_SUCCESS) {
            return PROFILING_FAILED;
        }
    }
    return PROFILING_SUCCESS;
}

int32_t MmGetCwd(char *buffer, int32_t maxLen)
{
    if ((buffer == nullptr) || (maxLen < 0)) {
        return PROFILING_INVALID_PARAM;
    }
    const char *ptr = getcwd(buffer, static_cast<uint32_t>(maxLen));
    if (ptr != nullptr) {
        return PROFILING_SUCCESS;
    } else {
        return PROFILING_FAILED;
    }
}

int32_t MmGetLocalTime(MmSystemTimeT *sysTimePtr)
{
    if (sysTimePtr == nullptr) {
        return PROFILING_INVALID_PARAM;
    }

    struct timeval timeVal;
    (void)memset_s(&timeVal, sizeof(timeVal), 0, sizeof(timeVal)); /* unsafe_function_ignore: memset */

    int32_t ret = gettimeofday(&timeVal, nullptr);
    if (ret != PROFILING_SUCCESS) {
        return PROFILING_FAILED;
    }

    struct tm nowTime;
    (void)memset_s(&nowTime, sizeof(nowTime), 0, sizeof(nowTime)); /* unsafe_function_ignore: memset */
 
    const struct tm *tmp = localtime_r(&timeVal.tv_sec, &nowTime);
    if (tmp == nullptr) {
        return PROFILING_FAILED;
    }

    sysTimePtr->wSecond = nowTime.tm_sec;
    sysTimePtr->wMinute = nowTime.tm_min;
    sysTimePtr->wHour = nowTime.tm_hour;
    sysTimePtr->wDay = nowTime.tm_mday;
    sysTimePtr->wMonth = nowTime.tm_mon + 1; // in localtime month is [0, 11], but in fact month is [1, 12]
    sysTimePtr->wYear = nowTime.tm_year + MMPA_COMPUTER_BEGIN_YEAR;
    sysTimePtr->wDayOfWeek = nowTime.tm_wday;
    sysTimePtr->tmYday = nowTime.tm_yday;
    sysTimePtr->tmIsdst = nowTime.tm_isdst;
    sysTimePtr->wMilliseconds = timeVal.tv_usec / MMPA_MSEC_TO_USEC;

    return PROFILING_SUCCESS;
}

int32_t MmGetPid()
{
    return static_cast<int32_t>(getpid());
}

int32_t MmGetTid()
{
    int32_t ret = static_cast<int32_t>(syscall(SYS_gettid));
    if (ret < 0) {
        return PROFILING_FAILED;
    }

    return ret;
}

int32_t MmStatGet(const std::string &path, MmStatT *buffer)
{
    if (path.empty() || (buffer == nullptr)) {
        return PROFILING_INVALID_PARAM;
    }

    int32_t ret = stat(path.c_str(), buffer);
    if (ret != PROFILING_SUCCESS) {
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

int32_t MmClose(int32_t fd)
{
    if (fd < 0) {
        return PROFILING_INVALID_PARAM;
    }

    int32_t ret = close(fd);
    if (ret != PROFILING_SUCCESS) {
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

int32_t MmGetOptLong(int32_t argc, char *const *argv, const char *opts, const MmStructOption *longOpts,
    int32_t *longIndex)
{
    return getopt_long(argc, argv, opts, longOpts, longIndex);
}

int32_t MmGetOptInd()
{
    return optind;
}

char *MmGetOptArg()
{
    return optarg;
}

int32_t MmGetTimeOfDay(MmTimeval *timeVal, MmTimezone *timeZone)
{
    if (timeVal == nullptr) {
        return PROFILING_INVALID_PARAM;
    }
    int32_t ret = gettimeofday(reinterpret_cast<struct timeval *>(timeVal),
        reinterpret_cast<struct timezone *>(timeZone));
    if (ret != PROFILING_SUCCESS) {
        ret = PROFILING_FAILED;
    }
    return ret;
}

int32_t MmOpen2(const std::string &pathName, int32_t flags, MmMode_t mode)
{
    if (pathName.empty() || (flags < 0)) {
        return PROFILING_INVALID_PARAM;
    }
    uint32_t tmp = static_cast<uint32_t>(flags);

    if (((tmp & (O_TRUNC | O_WRONLY | O_RDWR | O_CREAT)) == 0) && (flags != O_RDONLY)) {
        return PROFILING_INVALID_PARAM;
    }
    if (((mode & (S_IRUSR | S_IREAD)) == 0) &&
        ((mode & (S_IWUSR | S_IWRITE)) == 0)) {
        return PROFILING_INVALID_PARAM;
    }

    int32_t fd = open(pathName.c_str(), flags, mode);
    if (fd < 0) {
        return PROFILING_FAILED;
    }
    return fd;
}

ssize_t MmWrite(int32_t fd, void *buf, uint32_t bufLen)
{
    if ((fd < 0) || (buf == nullptr)) {
        return PROFILING_INVALID_PARAM;
    }

    ssize_t ret = write(fd, buf, static_cast<size_t>(bufLen));
    if (ret < 0) {
        return PROFILING_FAILED;
    }
    return ret;
}

int32_t MmGetOsVersion(char *versionInfo, int32_t versionLength)
{
    if ((versionInfo == nullptr) || (versionLength < MMPA_MIN_OS_VERSION_SIZE)) {
        return PROFILING_INVALID_PARAM;
    }
    int32_t ret = 0;
    struct utsname sysInfo;
    (void)memset_s(&sysInfo, sizeof(sysInfo), 0, sizeof(sysInfo)); /* unsafe_function_ignore: memset */
    uint32_t length = static_cast<uint32_t>(versionLength);
    size_t len = static_cast<size_t>(length);
    int32_t fb = uname(&sysInfo);
    if (fb < 0) {
        return PROFILING_FAILED;
    } else {
        ret = snprintf_s(versionInfo, len, (len - 1U), "%s-%s-%s",
            sysInfo.sysname, sysInfo.release, sysInfo.version);
        if (ret == PROFILING_FAILED) {
            return PROFILING_FAILED;
        }
        return PROFILING_SUCCESS;
    }
}

int32_t MmGetOsName(char *name, int32_t nameSize)
{
    if ((name == nullptr) || (nameSize < MMPA_MIN_OS_NAME_SIZE)) {
        return PROFILING_INVALID_PARAM;
    }
    uint32_t length = static_cast<uint32_t>(nameSize);
    int32_t ret = gethostname(name, length);
    if (ret < 0) {
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

static int32_t LocalLookup(char *buf, uint32_t bufLen, const char *pattern, char *value, uint32_t valueLen)
{
    const char *pValue = nullptr;
    char *pBuf = nullptr;
    uint32_t len = strlen(pattern);

    for (pBuf = buf; isspace(*pBuf) != 0; pBuf++) {}

    int32_t ret = strncmp(pBuf, pattern, len);
    if (ret != 0) {
        return PROFILING_FAILED;
    }

    for (pBuf = pBuf + len; isspace(*pBuf) != 0; pBuf++) {}

    if (*pBuf == '\0') {
        return PROFILING_FAILED;
    }

    for (pBuf = pBuf + 1; isspace(*pBuf) != 0; pBuf++) {}

    pValue = pBuf;
    for (pBuf = buf + bufLen; isspace(*(pBuf - 1)) != 0; pBuf--) {}

    *pBuf = '\0';

    ret = memcpy_s(value, valueLen, pValue, strlen(pValue) + 1U);
    if (ret != PROFILING_SUCCESS) {
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

static const char* LocalGetArmVersion(const char *cpuImplememter, uint32_t cpuImplememterLen,
    const char *cpuPart, uint32_t cpuPartLen)
{
    static struct CpuTypeTable gParamatersTable[] = {
        { "0x410xd03", "ARMv8_Cortex_A53"},
        { "0x410xd05", "ARMv8_Cortex_A55"},
        { "0x410xd07", "ARMv8_Cortex_A57"},
        { "0x410xd08", "ARMv8_Cortex_A72"},
        { "0x410xd09", "ARMv8_Cortex_A73"},
        { "0x480xd01", "TaishanV110"}
    };
    char cpuArmVersion[MMPA_CPUINFO_DOUBLE_SIZE] = {0};
    if (cpuImplememterLen + cpuPartLen >= sizeof(cpuArmVersion)) {
        return nullptr;
    }
    int32_t ret = snprintf_s(cpuArmVersion, sizeof(cpuArmVersion), sizeof(cpuArmVersion) - 1U,
                           "%s%s", cpuImplememter, cpuPart);
    if (ret == PROFILING_FAILED) {
        return nullptr;
    }
    int32_t limit = static_cast<int32_t>(sizeof(gParamatersTable) / sizeof(gParamatersTable[0]));
    for (int32_t i = limit - 1; i >= 0; i--) {
        ret = strcasecmp(cpuArmVersion, gParamatersTable[i].key);
        if (ret == 0) {
            return gParamatersTable[i].value;
        }
    }
    return nullptr;
}

static void LocalGetArmManufacturer(char *cpuImplememter, MmCpuDesc *cpuInfo)
{
    size_t len = strlen(cpuInfo->manufacturer);
    if (len != 0U) {
        return;
    }
    int32_t ret = PROFILING_FAILED;
    static struct CpuTypeTable gManufacturerTable[] = {
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
    int32_t limit = static_cast<int32_t>(sizeof(gManufacturerTable) / sizeof(gManufacturerTable[0]));
    for (int32_t i = limit - 1; i >= 0; --i) {
        ret = strcasecmp(cpuImplememter, gManufacturerTable[i].key);
        if (ret == 0) {
            (void)memcpy_s(cpuInfo->manufacturer, sizeof(cpuInfo->manufacturer),
                gManufacturerTable[i].value, (strlen(gManufacturerTable[i].value) + 1U));
            return;
        }
    }
    return;
}

static void LocalGetCpuProcV1(FILE *fp, MmCpuDesc *cpuInfo)
{
    char buf[MMPA_CPUPROC_BUF_SIZE]                 = {0};
    char cpuImplememter[MMPA_CPUINFO_DEFAULT_SIZE]  = {0};
    char cpuPart[MMPA_CPUINFO_DEFAULT_SIZE]         = {0};
    uint32_t length = 0U;
    while (fgets(buf, static_cast<int>(sizeof(buf)), fp) != nullptr) {
        length = static_cast<uint32_t>(strlen(buf));
        if (LocalLookup(buf, length, "manufacturer", cpuInfo->manufacturer,
            sizeof(cpuInfo->manufacturer)) == PROFILING_SUCCESS) {
            continue;
        }
        if (LocalLookup(buf, length, "vendor_id", cpuInfo->manufacturer,
            sizeof(cpuInfo->manufacturer)) == PROFILING_SUCCESS) {
            continue;
        }
        if (LocalLookup(buf, length, "CPU implementer", cpuImplememter,
            sizeof(cpuImplememter)) == PROFILING_SUCCESS) {
            continue; /* ARM and aarch64 */
        }
        if (LocalLookup(buf, length, "CPU part", cpuPart,
            sizeof(cpuPart)) == PROFILING_SUCCESS) {
            continue; /* ARM and aarch64 */
        }
        if (LocalLookup(buf, length, "model name", cpuInfo->version,
            sizeof(cpuInfo->version)) == PROFILING_SUCCESS) {
            ;
        }
    }
    const char* tmp = LocalGetArmVersion(cpuImplememter, strlen(cpuImplememter), cpuPart, strlen(cpuPart));
    if (tmp != nullptr) {
        (void)memcpy_s(cpuInfo->version, sizeof(cpuInfo->version), tmp, strlen(tmp) + 1U);
    }
    LocalGetArmManufacturer(cpuImplememter, cpuInfo);
    return;
}

static void LocalGetCpuProcV2(FILE *fp, MmCpuDesc *cpuInfo, int32_t *physicalCount)
{
    char buf[MMPA_CPUPROC_BUF_SIZE]                 = {0};
    char cpuMhz[MMPA_CPUINFO_DEFAULT_SIZE]          = {0};
    char cpuCores[MMPA_CPUINFO_DEFAULT_SIZE]        = {0};
    char cpuCnt[MMPA_CPUINFO_DEFAULT_SIZE]          = {0};
    char physicalID[MMPA_CPUINFO_DEFAULT_SIZE]      = {0};
    char cpuThreads[MMPA_CPUINFO_DEFAULT_SIZE]      = {0};
    char maxSpeed[MMPA_CPUINFO_DEFAULT_SIZE]        = {0};
    constexpr int base = 10;
    char *end = nullptr;
    uint32_t length = 0U;
    while (fgets(buf, static_cast<int>(sizeof(buf)), fp) != nullptr) {
        length = static_cast<uint32_t>(strlen(buf));
        if (LocalLookup(buf, length, "cpu MHz", cpuMhz, sizeof(cpuMhz)) == PROFILING_SUCCESS) {
            continue;
        }
        if (LocalLookup(buf, length, "cpu cores", cpuCores, sizeof(cpuCores)) == PROFILING_SUCCESS) {
            continue;
        }
        if (LocalLookup(buf, length, "processor", cpuCnt, sizeof(cpuCnt)) == PROFILING_SUCCESS) {
            continue; // processor index + 1
        }
        if (LocalLookup(buf, length, "physical id", physicalID, sizeof(physicalID)) == PROFILING_SUCCESS) {
            continue;
        }
        if (LocalLookup(buf, length, "Thread Count", cpuThreads, sizeof(cpuThreads)) == PROFILING_SUCCESS) {
            continue;
        }
        if (LocalLookup(buf, length, "Max Speed", maxSpeed, sizeof(maxSpeed)) == PROFILING_SUCCESS) {
            ;
        }
    }
    cpuInfo->frequency = std::strtol(cpuMhz, &end, base);
    cpuInfo->ncores = std::strtol(cpuCores, &end, base);
    cpuInfo->ncounts = std::strtol(cpuCnt, &end, base) + 1;
    *physicalCount += std::strtol(physicalID, &end, base);
    cpuInfo->nthreads = std::strtol(cpuThreads, &end, base);
    cpuInfo->maxFrequency = std::strtol(maxSpeed, &end, base);
    return;
}

static void LocalGetCpuProc(MmCpuDesc *cpuInfo, int32_t *physicalCount)
{
    FILE *fp = fopen("/proc/cpuinfo", "r");
    if (fp == nullptr) {
        return;
    }
    LocalGetCpuProcV1(fp, cpuInfo);
    LocalGetCpuProcV2(fp, cpuInfo, physicalCount);
    (void)fclose(fp);
    fp = nullptr;
    return;
}

int32_t MmGetCpuInfo(MmCpuDesc **cpuInfo, int32_t *count)
{
    if (count == nullptr) {
        return PROFILING_INVALID_PARAM;
    }
    if (cpuInfo == nullptr) {
        return PROFILING_INVALID_PARAM;
    }
    int32_t i = 0;
    int32_t ret = 0;
    MmCpuDesc cpuDest = {};
    int32_t physicalCount = 1;
    MmCpuDesc *pCpuDesc = nullptr;
    struct utsname sysInfo = {};

    LocalGetCpuProc(&cpuDest, &physicalCount);

    if ((physicalCount < MMPA_MIN_PHYSICALCPU_COUNT) || (physicalCount > MMPA_MAX_PHYSICALCPU_COUNT)) {
        return PROFILING_FAILED;
    }
    uint32_t needSize = static_cast<uint32_t>(physicalCount) * static_cast<uint32_t>(sizeof(MmCpuDesc));

    pCpuDesc = (MmCpuDesc*)malloc(needSize);
    if (pCpuDesc == nullptr) {
        return PROFILING_FAILED;
    }

    (void)memset_s(pCpuDesc, needSize, 0, needSize); /* unsafe_function_ignore: memset */

    if (uname(&sysInfo) == PROFILING_SUCCESS) {
        ret = memcpy_s(cpuDest.arch, sizeof(cpuDest.arch), sysInfo.machine, strlen(sysInfo.machine) + 1U);
        if (ret != PROFILING_SUCCESS) {
            free(pCpuDesc);
            return PROFILING_FAILED;
        }
    }

    int32_t cpuCnt = physicalCount;
    for (i = 0; i < cpuCnt; i++) {
        pCpuDesc[i] = cpuDest;
        pCpuDesc[i].ncounts = pCpuDesc[i].ncounts / cpuCnt;
    }

    *cpuInfo = pCpuDesc;
    *count = cpuCnt;
    return PROFILING_SUCCESS;
}

int32_t MmCpuInfoFree(MmCpuDesc *cpuInfo, int32_t count)
{
    if ((cpuInfo == nullptr) || (count == 0)) {
        return PROFILING_INVALID_PARAM;
    }
    (void)free(cpuInfo);
    return PROFILING_SUCCESS;
}

ssize_t MmRead(int32_t fd, void *buf, uint32_t bufLen)
{
    if ((fd < 0) || (buf == nullptr)) {
        return PROFILING_INVALID_PARAM;
    }

    ssize_t ret = read(fd, buf, static_cast<size_t>(bufLen));
    if (ret < 0) {
        return PROFILING_FAILED;
    }
    return ret;
}

ssize_t MmSocketSend(MmSockHandle sockFd, void *sendBuf, int32_t sendLen, int32_t sendFlag)
{
    if ((sockFd < 0) || (sendBuf == nullptr) || (sendLen <= 0) || (sendFlag < 0)) {
        return PROFILING_INVALID_PARAM;
    }
    uint32_t sndLen = static_cast<uint32_t>(sendLen);
    ssize_t ret = send(sockFd, sendBuf, sndLen, sendFlag);
    if (ret <= 0) {
        return PROFILING_FAILED;
    }
    return ret;
}

int32_t MmMutexLock(MmMutexT *mutex)
{
    if (mutex == nullptr) {
        return PROFILING_INVALID_PARAM;
    }

    int32_t ret = pthread_mutex_lock(mutex);
    if (ret != PROFILING_SUCCESS) {
        ret = PROFILING_FAILED;
    }
    return ret;
}

int32_t MmMutexUnLock(MmMutexT *mutex)
{
    if (mutex == nullptr) {
        return PROFILING_INVALID_PARAM;
    }

    int32_t ret = pthread_mutex_unlock(mutex);
    if (ret != PROFILING_SUCCESS) {
        ret = PROFILING_FAILED;
    }
    return ret;
}
} // Mmpa
} // Dvvp
} // Collector