/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2021. All rights reserved.
 * Description: common util interface
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2018-06-13
 */
#include "utils.h"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <locale>
#include <iomanip>
#include <string>
#include <ctime>
#include "config/config.h"
#include "config/config_manager.h"
#include "errno/error_code.h"
#include "msprof_dlog.h"
#include "securec.h"

#if (defined(linux) || defined(__linux__))
#include <sys/file.h>
#endif

namespace analysis {
namespace dvvp {
namespace common {
namespace utils {
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::config;
using namespace Analysis::Dvvp::Common::Config;
using namespace Collector::Dvvp::Plugin;
using namespace Collector::Dvvp::Mmpa;
using FuncIntPtr = int(*)(int);
std::mutex g_envMtx;
const unsigned long long CHANGE_FROM_S_TO_NS = 1000000000;

unsigned long long Utils::GetClockRealtime()
{
#if (defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER))
    MmTimespec now;
    (void)memset_s(&now, sizeof(now), 0, sizeof(now));
    now = MmGetTickCount();
    return ((unsigned long long)(now.tvSec) * CHANGE_FROM_S_TO_NS) + (unsigned long long)(now.tvNsec);
#else
    struct timespec now;
    (void)memset_s(&now, sizeof(now), 0, sizeof(now));
    (void)clock_gettime(CLOCK_REALTIME, &now);
    return (static_cast<unsigned long long>(now.tv_sec) * CHANGE_FROM_S_TO_NS) +
        static_cast<unsigned long long>(now.tv_nsec);
#endif
}

unsigned long long Utils::GetClockMonotonicRaw()
{
    MmTimespec now;
    (void)memset_s(&now, sizeof(now), 0, sizeof(now));
    now = MmGetTickCount();
    return (static_cast<unsigned long long>(now.tvSec) * CHANGE_FROM_S_TO_NS) +
        static_cast<unsigned long long>(now.tvNsec);
}

unsigned long long Utils::GetCPUCycleCounter()
{
    unsigned long long cycles;
#if defined(__aarch64__)
    asm volatile("mrs %0, cntvct_el0" : "=r" (cycles));
#elif defined(__x86_64__)
    const int uint32Bits = 32;  // 32 is uint bit count
    unsigned int hi = 0;
    unsigned int lo = 0;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    cycles = (static_cast<unsigned long long>(lo)) | ((static_cast<unsigned long long>(hi)) << uint32Bits);
#else
    cycles = 0;
#endif
    return cycles;
}

double Utils::StatCpuRealFreq()
{
    const int sampleTimes = 500;    // sample 500 copies of data
    const int sampleIntv = 1000;    // sample a copy of data every 1000us

    std::vector<unsigned long long> cycleCnts;
    std::vector<unsigned long long> monotonicRaws;
    for (int i = 0; i <= sampleTimes; i++) {
        cycleCnts.push_back(Utils::GetCPUCycleCounter());
        monotonicRaws.push_back(Utils::GetClockMonotonicRaw());
        usleep(sampleIntv);
    }

    double freqSum = 0;
    for (int i = 0; i < sampleTimes; i++) {
        freqSum += static_cast<double>(cycleCnts[i + 1] - cycleCnts[i]) / (monotonicRaws[i + 1] - monotonicRaws[i]);
    }
    return freqSum / sampleTimes;
}

void Utils::GetTime(unsigned long long &startRealtime, unsigned long long &startMono, unsigned long long &cntvct)
{
    cntvct = analysis::dvvp::common::utils::Utils::GetCPUCycleCounter();
    startMono = analysis::dvvp::common::utils::Utils::GetClockMonotonicRaw();
    startRealtime = analysis::dvvp::common::utils::Utils::GetClockRealtime();
}

std::string Utils::GenerateStartTime(const unsigned long long startRealtime, const unsigned long long startMono,
    const unsigned long long cntvct)
{
    std::stringstream devStartData;
    devStartData << CLOCK_REALTIME_KEY << ": " << startRealtime << std::endl;
    devStartData << CLOCK_MONOTONIC_RAW_KEY << ": " << startMono << std::endl;
    devStartData << CLOCK_CNTVCT_KEY << ": " << cntvct << std::endl;
    return devStartData.str();
}

long long Utils::GetFileSize(const std::string &path)
{
    unsigned long long size = 0;

    int ret = MmGetFileSize(path, &size);
    if (ret < 0) {
        MSPROF_LOGW("MsprofMmGetFileSize fail, ret=%d, errno=%d.", ret, errno);
        return -1;
    }

    return static_cast<long long>(size);
}

/**
 * @ingroup Utils
 * @brief 获取path挂载节点的存储空间.
 *
 * @param  path [IN]  文件或者目录路径.
 * @param  size [IN] 用于获取内存大小的外部引用.
 * @param  sizeType [IN] 获取内存的类型.
 *  TOTAL_SIZE为总的磁盘空间,AVAIL_SIZE为非特权用户的可用磁盘空间,FREE_SIZE为总的空闲磁盘空间.
 *
 * @warning 获取的内存大小以字节为单位;
 *
 * @retval 执行成功返回PROFILING_SUCCESS.
 * @retval 执行失败返回PROFILING_FAILED.
 *
 */
int Utils::GetVolumeSize(const std::string &path, unsigned long long &size, VolumeSize sizeType)
{
    MmDiskSize diskSize;

    int ret = MmGetDiskFreeSpace(path, &diskSize);
    if (ret < 0) {
        return PROFILING_FAILED;
    }
    switch (sizeType) {
        case VolumeSize::AVAIL_SIZE:
            size = diskSize.availSize;
            break;
        case VolumeSize::FREE_SIZE:
            size = diskSize.freeSize;
            break;
        case VolumeSize::TOTAL_SIZE:
            size = diskSize.totalSize;
            break;
        default:
            ret = PROFILING_FAILED;
            break;
    }
    return ret;
}

bool Utils::IsDir(const std::string &path)
{
    if (path.length() == 0) {
        return false;
    }

    if (MmIsDir(path) != PROFILING_SUCCESS) {
        return false;
    }

    return true;
}

bool Utils::IsAllDigit(const std::string &digitStr)
{
    if (digitStr.empty()) {
        return false;
    }
    for (std::string::size_type i = 0; i < digitStr.size(); i++) {
        if (isdigit(digitStr[i]) == 0) {
            return false;
        }
    }
    return true;
}

bool Utils::IsDirAccessible(const std::string &path)
{
    if (!IsDir(path)) {
        MSPROF_LOGE("Path %s is not a dir", BaseName(path).c_str());
        return false;
    }
    if (MmAccess2(path, M_W_OK) == PROFILING_SUCCESS) {
        return true;
    }
    MSPROF_LOGE("No access to dir %s", BaseName(path).c_str());
    return false;
}

bool Utils::IsFileExist(const std::string &path)
{
    if (path.empty()) {
        return false;
    }

    if (MmAccess(path) == PROFILING_SUCCESS) {
        return true;
    }

    return false;
}

std::vector<std::string> Utils::Split(const std::string &input_str,
                                      bool filter_out_enabled,
                                      const std::string &filter_out,
                                      const std::string &pattern)
{
    std::string::size_type pos;
    std::vector<std::string> res;
    if (input_str.empty()) {
        return res;
    }
    std::string str = input_str + pattern;
    std::string::size_type size = str.size();
    std::string::size_type i = 0;
    while (i < size) {
        pos = str.find(pattern, i);
        if (pos < size) {
            bool isOk = true;

            std::string ss = str.substr(i, pos - i);
            if ((filter_out_enabled && (ss.compare(filter_out) == 0))) {
                isOk = false;
            }

            if (isOk) {
                res.push_back(ss);
            }

            i = pos + pattern.size() - 1;
        }
        i++;
    }

    return res;
}

int Utils::RelativePath(const std::string &path,
                        const std::string &dir,
                        std::string &relativePath)
{
    size_t pos = path.find(dir);
    if (pos == std::string::npos) {
        MSPROF_LOGE("Failed to find \"%s\" from \"%s\"",
                    BaseName(dir).c_str(), BaseName(path).c_str());
        return PROFILING_FAILED;
    }

    relativePath = LeftTrim(path.substr(pos + dir.size()), "/\\");

    return PROFILING_SUCCESS;
}

#if (defined(linux) || defined(__linux__))
std::string Utils::DirName(const std::string &path)
{
    std::string result;
    char *pathc = MSVP_STRDUP(path.c_str());
    if (pathc != nullptr) {
        char *dirc = MmDirName(pathc);
        if (dirc != nullptr) {
            result = dirc;
        }
        free(pathc);
        pathc = nullptr;
    }
    return result;
}
#else
std::string Utils::DirName(const std::string &path)
{
    std::string result;
    std::string filePath(path);
    const size_t last_slash_idx = filePath.rfind('\\');
    if (std::string::npos != last_slash_idx) {
        result = filePath.substr(0, last_slash_idx);
    }
    return result;
}
#endif

#if (defined(linux) || defined(__linux__))
std::string Utils::BaseName(const std::string &path)
{
    std::string result;
    char *pathc = MSVP_STRDUP(path.c_str());
    if (pathc != nullptr) {
        char *basec = MmBaseName(pathc);
        if (basec != nullptr) {
            result = basec;
        }
        free(pathc);
        pathc = nullptr;
    }
    return result;
}
#else
std::string Utils::BaseName(const std::string &path)
{
    std::string result;
    std::string filePath(path);
    const size_t last_slash_idx = filePath.rfind('\\');
    if (std::string::npos != last_slash_idx) {
        result = filePath.substr(last_slash_idx + 1);
    }
    return result;
}
#endif

int Utils::SplitPath(const std::string &path, std::string &dir, std::string &base)
{
    int ret = PROFILING_FAILED;

    char *dirc = nullptr;
    char *basec = nullptr;
    char *bname = nullptr;
    char *dname = nullptr;

    dirc = MSVP_STRDUP(path.c_str());
    basec = MSVP_STRDUP(path.c_str());
    if (dirc != nullptr && basec != nullptr) {
        dname = MmDirName(dirc);
        bname = MmBaseName(basec);
        if (dname != nullptr && bname != nullptr) {
            dir = std::string(dname);
            base = std::string(bname);
            ret = PROFILING_SUCCESS;
        } else {
            MSPROF_LOGE("mmDirName or mmBaseName failed");
        }
    } else {
        MSPROF_LOGE("MSVP_STRDUP failed");
    }

    if (dirc != nullptr) {
        free(dirc);
        dirc = nullptr;
    }

    if (basec != nullptr) {
        free(basec);
        basec = nullptr;
    }

    return ret;
}

#if (defined(linux) || defined(__linux__))
int Utils::CreateDir(const std::string &path)
{
    std::string curr = path;

    if (curr.empty()) {
        return PROFILING_FAILED;
    }
    if (IsFileExist(curr)) {
        MSPROF_LOGD("The file already exists, %s", BaseName(path).c_str());
        return PROFILING_SUCCESS;
    } else {
        std::string dir;
        std::string base;

        int ret = SplitPath(curr, dir, base);
        if (ret != PROFILING_SUCCESS) {
            MSPROF_LOGE("Split path failed");
            return ret;
        }
        ret = CreateDir(dir);
        if (ret != PROFILING_SUCCESS) {
            return ret;
        }
    }

    const MmMode_t defaultFileMode = (MmMode_t)0750;  // 0750 means xwrx-r
    MSPROF_LOGI("CreateDir dir %s with 750", BaseName(path).c_str());
    if ((MmMkdir(path, defaultFileMode) != PROFILING_SUCCESS) && (errno != EEXIST)) {
        return PROFILING_FAILED;
    }

    if (MmChmod(path, defaultFileMode) != PROFILING_SUCCESS) {
        MSPROF_LOGW("Chmod : %s unsuccessfully", BaseName(path).c_str());
    }
    MSPROF_LOGI("Success to mkdir, FilePath : %s, FileMode : %o",
                BaseName(path).c_str(), static_cast<int>(defaultFileMode));
    return PROFILING_SUCCESS;
}

#else
int Utils::CreateDir(const std::string &path)
{
    std::string curr = path;
    if (curr.empty()) {
        return PROFILING_FAILED;
    }
    if (IsFileExist(curr)) {
        MSPROF_LOGD("The file already exists, %s", BaseName(path).c_str());
        return PROFILING_SUCCESS;
    }
    const MmMode_t defaultFileMode = (MmMode_t)0750;  // 0750 means xwrx-r
    if ((MmMkdir(path, defaultFileMode) != PROFILING_SUCCESS) && (errno != EEXIST)) {
        char errBuf[MAX_ERR_STRING_LEN + 1] = {0};
        MSPROF_LOGE("Failed to mkdir, FilePath : %s, FileMode : %o, ErrorCode : %d, ERRORInfo : %s",
            BaseName(path).c_str(), static_cast<int>(defaultFileMode), MmGetErrorCode(),
            MmGetErrorFormatMessage(MmGetErrorCode(),
                errBuf, MAX_ERR_STRING_LEN));
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}
#endif

void Utils::RemoveDir(const std::string &dir, bool rmTopDir)
{
    if ((dir.empty()) || (dir.compare("/")) == 0) {
        MSPROF_LOGE("empty compare failed.");
        return;
    }
    MSPROF_LOGI("RemoveDir dir %s", BaseName(dir).c_str());

    if (!rmTopDir) {
        MmDirent **nameList = nullptr;
        int count = MmScandir(dir, &nameList, nullptr, nullptr);
        if (count == PROFILING_FAILED || count == PROFILING_INVALID_PARAM) {
            MSPROF_LOGW("mmScandir failed %s. ErrorCode : %d", BaseName(dir).c_str(),
                MmGetErrorCode());
            return;
        }

        for (int i = 0; i < count; i++) {
            std::string fileName = nameList[i]->d_name;
            std::string childPath = dir + MSVP_SLASH + fileName;

            if ((fileName.compare(".") == 0) || (fileName.compare("..") == 0)) {
                continue;
            }

            if (IsDir(childPath)) {
                MmRmdir(childPath);
            } else {
                MmUnlink(childPath);
            }
        }

        MmScandirFree(nameList, count);
    } else {
        if (MmRmdir(dir) != PROFILING_SUCCESS) {
            MSPROF_LOGW("mmRmdir failed %s. ErrorCode : %d", BaseName(dir).c_str(),
                MmGetErrorCode());
        }
    }
}

bool Utils::IsSoftLink(const std::string &path)
{
#if (defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER))
    return false;   // not soft-link
#else
    struct stat buf1;
    int ret = stat(path.c_str(), &buf1);
    if (ret != 0) {
        MSPROF_LOGE("stat %s fail, ret=%d errno=%u", BaseName(path).c_str(), ret, errno);
        return true;
    }

    struct stat buf2;
    ret = lstat(path.c_str(), &buf2);
    if (ret != 0) {
        MSPROF_LOGE("lstat %s fail, ret=%d errno=%u", BaseName(path).c_str(), ret, errno);
        return true;
    }

    if (buf1.st_ino != buf2.st_ino) {
        return true;     // soft-link
    }

    return false;   // not soft-link
#endif
}

bool Utils::IsSocketFile(const std::string &path)
{
#if (defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER))
    return false;
#else
    struct stat statBuf;
    int ret = stat(path.c_str(), &statBuf);
    if (ret != 0) {
        MSPROF_LOGE("stat %s fail, ret=%d errno=%u", path.c_str(), ret, errno);
        return true;
    }
    return S_ISSOCK(statBuf.st_mode);
#endif
}

std::string Utils::CanonicalizePath(const std::string &path)
{
    std::string resolvedPath;
    std::string tmpPath = IdeReplaceWaveWithHomedir(path);
    if ((tmpPath.length() == 0) || (tmpPath.length() >= MMPA_MAX_PATH)) {
        return "";
    }

    char realPath[MMPA_MAX_PATH] = { 0 };
    int ret = MmRealPath(tmpPath.c_str(), realPath, MMPA_MAX_PATH);
    if (ret == PROFILING_SUCCESS) {
        MSPROF_LOGI("mmRealPath ret=%d.", ret);
        resolvedPath = realPath;
    }
    MSPROF_LOGI("Input path:%s, canonicalized path:%s",
                BaseName(tmpPath).c_str(), BaseName(resolvedPath).c_str());

    return resolvedPath;
}

int Utils::ExecCmd(const ExecCmdParams &execCmdParams,
    const std::vector<std::string> &argv,
    const std::vector<std::string> &envp,
    int &exitCodeP,
    MmProcess &childProcess)
{
    int ret = PROFILING_FAILED;
    const std::string &cmd = execCmdParams.cmd;
    bool async = execCmdParams.async;
    std::string stdoutRedirectFile = execCmdParams.stdoutRedirectFile;
    const int maxArgvEnvSize = (1024 * 1024);  // 1024 * 1024 means 1mb
    if (argv.size() > maxArgvEnvSize || envp.size() > maxArgvEnvSize) {
        MSPROF_LOGE("invalid argv or envp size");
        return ret;
    }
    UtilsStringBuilder<std::string> builder;
    std::string argsStr = builder.Join(argv, " ");
    MSPROF_LOGI("Execute cmd:\"%s %s\", stdoutRedirectFile=%s",
                BaseName(cmd).c_str(), BaseName(argsStr).c_str(), BaseName(stdoutRedirectFile).c_str());

    do {
        uint32_t ii = 0;
        const int reserveArgvLen = 2;
        SHARED_PTR_ALIA<CHAR_PTR> argvArray(new(std::nothrow) CHAR_PTR[argv.size() + reserveArgvLen],
                                            std::default_delete<CHAR_PTR[]>());
        if (argvArray == nullptr) {
            MSPROF_LOGE("argvArray malloc memory failed.");
            return PROFILING_FAILED;
        }
        argvArray.get()[0] = const_cast<CHAR_PTR>(cmd.c_str());
        for (ii = 0; ii < static_cast<uint32_t>(argv.size()); ++ii) {
            argvArray.get()[ii + 1] = const_cast<CHAR_PTR>(argv[ii].c_str());
        }
        argvArray.get()[ii + 1] = nullptr;

        SHARED_PTR_ALIA<CHAR_PTR> envpArray(new(std::nothrow) CHAR_PTR[envp.size() + 1],
                                            std::default_delete<CHAR_PTR[]>());
        if (envpArray == nullptr) {
            MSPROF_LOGE("envpArray malloc memory failed.");
            return PROFILING_FAILED;
        }
        for (ii = 0; ii < static_cast<uint32_t>(envp.size()); ++ii) {
            envpArray.get()[ii] = const_cast<CHAR_PTR>(envp[ii].c_str());
        }
        envpArray.get()[ii] = nullptr;
        ExecCmdArgv execCmdArgv(argvArray.get(), static_cast<uint32_t>(argv.size() + reserveArgvLen),
            envpArray.get(), static_cast<uint32_t>(envp.size()));
        if (async) {
            ret = ExecCmdCAsync(execCmdArgv, execCmdParams, childProcess);
        } else {
            ret = ExecCmdC(execCmdArgv, execCmdParams, exitCodeP);
        }
    } while (0);
    return ret;
}

int Utils::ChangeWorkDir(const std::string &fileName)
{
    if (fileName.empty()) {
        return PROFILING_FAILED;
    }
    // change work dir
    char *dName = nullptr;
    char *dirc = nullptr;
    dirc = MSVP_STRDUP(fileName.c_str());
    if (dirc == nullptr) {
        return PROFILING_FAILED;
    }
    dName = MmDirName(dirc);
    if (dName == nullptr || MmChdir(std::string(dName)) != PROFILING_SUCCESS) {
        MSPROF_LOGW("chdir(%s) failed.", dirc);
    }
    if (dirc != nullptr) {
        free(dirc);
        dirc = nullptr;
    }
    return PROFILING_SUCCESS;
}

void Utils::SetArgEnv(CHAR_PTR_CONST argv[],
                      const int argvCount,
                      CHAR_PTR_CONST envp[],
                      const int envCount,
                      MmArgvEnv &argvEnv)
{
    if (argv != nullptr && envp != nullptr) {
        (void)memset_s(&argvEnv, sizeof(MmArgvEnv), 0, sizeof(MmArgvEnv));

        argvEnv.argv = const_cast<CHAR_PTR_PTR>(argv);
        argvEnv.argvCount = argvCount;
        argvEnv.envp = const_cast<CHAR_PTR_PTR>(envp);
        argvEnv.envpCount = envCount;
    }
}

int Utils::DoCreateCmdProcess(const std::string &stdoutRedirectFile,
                              const std::string &fileName,
                              MmArgvEnv &argvEnv,
                              MmProcess &tid)
{
    int ret = MmCreateProcess(fileName, &argvEnv, stdoutRedirectFile, &tid);
    if (ret != PROFILING_SUCCESS) {
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

int Utils::ExecCmdC(const ExecCmdArgv &execCmdArgv, const ExecCmdParams &execCmdParams, int &exitCodeP)
{
    std::string filename = execCmdParams.cmd;
    CHAR_PTR_CONST *argv = execCmdArgv.argv;
    int argvCount = execCmdArgv.argvCount;
    CHAR_PTR_CONST *envp = execCmdArgv.envp;
    int envCount = execCmdArgv.envCount;
    std::string stdoutRedirectFile = execCmdParams.stdoutRedirectFile;
    constexpr int invalidExitCode = -1;
    if (filename.empty() || argv == nullptr || envp == nullptr || exitCodeP == invalidExitCode) {
        return PROFILING_FAILED;
    }

    MmArgvEnv argvEnv;
    MmProcess tid = 0;
    SetArgEnv(argv, argvCount, envp, envCount, argvEnv);

    if (ChangeWorkDir(filename) == PROFILING_FAILED) {
        return PROFILING_FAILED;
    }
    int ret = DoCreateCmdProcess(stdoutRedirectFile, filename, argvEnv, tid);
    if (ret == PROFILING_FAILED) {
        return PROFILING_FAILED;
    }

    bool exited = false;
    int exitedCode = 0;
    ret = WaitProcess(tid, exited, exitedCode, true);
    if (ret == PROFILING_SUCCESS) {
        if (exitCodeP != invalidExitCode) {
            exitCodeP = exitedCode;
        }
    }

    return ret;
}

int Utils::ExecCmdCAsync(const ExecCmdArgv &execCmdArgv, const ExecCmdParams &execCmdParams,
                         MmProcess &childProcess)
{
    std::string filename = execCmdParams.cmd;
    CHAR_PTR_CONST *argv = execCmdArgv.argv;
    int argvCount = execCmdArgv.argvCount;
    CHAR_PTR_CONST *envp = execCmdArgv.envp;
    int envCount = execCmdArgv.envCount;
    std::string stdoutRedirectFile = execCmdParams.stdoutRedirectFile;

    if (filename.empty() || argv == nullptr || envp == nullptr) {
        return PROFILING_FAILED;
    }
    MmProcess tid = 0;
    MmArgvEnv argvEnv;
    SetArgEnv(argv, argvCount, envp, envCount, argvEnv);
    int ret = DoCreateCmdProcess(stdoutRedirectFile, filename, argvEnv, tid);
    if (ret == PROFILING_FAILED) {
        return PROFILING_FAILED;
    }
    childProcess = tid;

    return PROFILING_SUCCESS;
}

int Utils::WaitProcess(MmProcess process, bool &isExited, int &exitCode, bool hang /* = true */)
{
    isExited = false;
    int waitStatus = 0;
    uint32_t options = 0;
    uint32_t flags1 = M_WAIT_NOHANG;
    if (!hang) {
        options |= flags1;
    }

    uint32_t flags2 = options & flags1;
    int ret = MmWaitPid(process, &waitStatus, options);
    if (ret != PROFILING_INVALID_PARAM && ret != PROFILING_FAILED) {
        if ((flags2 != 0) && (ret == PROFILING_SUCCESS)) {
            return PROFILING_SUCCESS;
        }
        if (ret == PROFILING_ERROR) {
            isExited = true;
            exitCode = waitStatus;
            return PROFILING_SUCCESS;
        }
    }
    return PROFILING_FAILED;
}

bool Utils::ProcessIsRuning(MmProcess process)
{
    int waitStatus = 0;

    int ret = MmWaitPid(process, &waitStatus, M_WAIT_NOHANG);
    if (ret == PROFILING_SUCCESS) {
        return true;
    }
    return false;
}

std::string Utils::LeftTrim(const std::string &str, const std::string &trims)
{
    if (str.length() > 0) {
        size_t posStr = str.find_first_not_of(trims);
        if (posStr != std::string::npos) {
            return str.substr(posStr);
        } else {
            return "";
        }
    }

    return str;
}

std::string Utils::JoinPath(const std::vector<std::string> &paths)
{
    std::string path;

    for (size_t ii = 0; ii < paths.size(); ++ii) {
        if (ii == 0) {
            path = paths[ii];
        } else {
            path += MSVP_SLASH + paths[ii];
        }
    }

    return path;
}

std::string Utils::ToUpper(const std::string &value)
{
    std::string result = value;
    std::transform (result.begin(), result.end(), result.begin(), static_cast<FuncIntPtr>(std::toupper));
    return result;
}

std::string Utils::ToLower(const std::string &value)
{
    std::string result = value;
    std::transform (result.begin(), result.end(), result.begin(), static_cast<FuncIntPtr>(std::tolower));
    return result;
}

std::string Utils::Trim(const std::string &value)
{
    std::string result = value;
    size_t pos = result.find_first_not_of(" ");
    if (pos == std::string::npos) {
        return "";
    }
    result.erase(0, pos);
    result.erase(result.find_last_not_of(" ") + 1);
    return result;
}

int Utils::UsleepInterupt(unsigned long usec)
{
    // here we don't need accurate sleep time, so we don't check error code
    const int changeFormUsToMs = 1000;
    (void)MmSleep(usec / changeFormUsToMs);

    return PROFILING_SUCCESS;
}

int Utils::MsleepInterruptible(unsigned long msec)
{
    const unsigned long onecSleepTime = 50;
    unsigned long sleepTimes = msec / onecSleepTime;
    while (sleepTimes-- > 0) {
        (void)MmSleep(onecSleepTime);
    }
    (void)MmSleep(msec % onecSleepTime);
    return PROFILING_SUCCESS;
}

void Utils::GetFiles(const std::string &dir, bool isRecur, std::vector<std::string> &files)
{
    if (dir.empty()) {
        return;
    }

    MmDirent **dirNameList = nullptr;
    int count = MmScandir(dir, &dirNameList, nullptr, nullptr);
    if (count == PROFILING_FAILED || count == PROFILING_INVALID_PARAM) {
        return;
    }

    for (int j = 0; j < count; j++) {
        std::string fileName = dirNameList[j]->d_name;
        std::string childPath = dir + MSVP_SLASH + fileName;

        if (IsDir(childPath)) {
            if (!isRecur) {
                continue;
            }

            if ((fileName.compare(".") == 0) || (fileName.compare("..") == 0)) {
                continue;
            }

            GetFiles(childPath, isRecur, files);
        } else {
            files.push_back(childPath);
        }
    }

    MmScandirFree(dirNameList, count);
}

void Utils::GetChildDirs(const std::string &dir, bool isRecur, std::vector<std::string> &childDirs)
{
    if (dir.empty()) {
        return;
    }

    MmDirent **nameList = nullptr;
    int count = MmScandir(dir, &nameList, nullptr, nullptr);
    if (count == PROFILING_FAILED || count == PROFILING_INVALID_PARAM) {
        return;
    }

    for (int i = 0; i < count; i++) {
        std::string fileName = nameList[i]->d_name;
        std::string childPath = dir + MSVP_SLASH + fileName;

        if (IsDir(childPath)) {
            if ((fileName.compare(".") == 0) || (fileName.compare("..") == 0)) {
                continue;
            }
            childDirs.push_back(childPath);
            if (!isRecur) {
                continue;
            }
            GetChildDirs(childPath, isRecur, childDirs);
        }
    }

    MmScandirFree(nameList, count);
}

void Utils::GetChildFilenames(const std::string &dir, std::vector<std::string> &files)
{
    if (dir.empty()) {
        return;
    }
    MmDirent **dirNameList = nullptr;
    int count = MmScandir(dir, &dirNameList, nullptr, nullptr);
    if (count == PROFILING_INVALID_PARAM || count == PROFILING_FAILED) {
        return;
    }
    for (int j = 0; j < count; j++) {
        std::string fileName = dirNameList[j]->d_name;
        if ((fileName.compare(".") == 0) || (fileName.compare("..") == 0)) {
            continue;
        }
        files.push_back(fileName);
    }

    MmScandirFree(dirNameList, count);
}

std::vector<int> Utils::GetChildPid(int pid)
{
    std::vector<int> allChildPids;

    std::vector<std::string> threadDirs;
    std::string taskDir = "/proc/" + std::to_string(pid) + "/task/";
    Utils::GetChildDirs(taskDir, false, threadDirs);

    CHAR_PTR end = nullptr;
    constexpr int base = 10;
    std::ifstream ifs;
    for (auto &tdir : threadDirs) {
        std::string lineBuf;
        ifs.open(tdir + "/children", std::ifstream::in);
        std::getline(ifs, lineBuf);
        ifs.close();

        std::vector<std::string> childPids = Utils::Split(lineBuf, true, "", " ");
        for (auto &childPid : childPids) {
            allChildPids.push_back(std::strtol(childPid.c_str(), &end, base));
        }
    }

    return allChildPids;
}

std::vector<int> Utils::GetChildPidRecursive(int pid, unsigned int recursiveLevel)
{
    std::vector<int> allPids;
    const unsigned int maxRecursiveLevel = 4;
    if (recursiveLevel >= maxRecursiveLevel) {
        return allPids;
    }
    allPids = GetChildPid(pid);
    for (auto childPid : allPids) {
        std::vector<int> childPids = GetChildPidRecursive(childPid, recursiveLevel + 1);
        allPids.insert(allPids.end(), childPids.begin(), childPids.end());
    }
    return allPids;
}

std::string Utils::TimestampToTime(const std::string &timestamp, int unit /* = 1 */)
{
    if (timestamp.empty() || (timestamp.find_first_not_of("1234567890") != std::string::npos) ||
        (unit == 0)) {
        return "0";
    }
    time_t secTime;
    uint32_t microTime;
    try {
        secTime = std::stoll(timestamp) / unit;
        microTime = static_cast<uint32_t>(std::stoll(timestamp) % unit);
    } catch (...) {
        return "0";
    }

    std::string timeString("0-0-0 0:0:0.0");
    struct tm ttime;
    (void)memset_s(&ttime, sizeof(tm), 0, sizeof(ttime));
#if (defined(linux) || defined(__linux__))
    if (localtime_r(&secTime, &ttime) == nullptr) {
        return timeString;
    }
#else
    if (localtime_s(&ttime, &secTime) != 0) {
        return timeString;
    }
#endif
    const int timeLen = 32;
    char timeStr[timeLen] = { 0 };
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &ttime);
    timeString = std::string(timeStr);

    (void)memset_s(timeStr, sizeof(timeStr), 0, sizeof(timeStr));
    int ret = sprintf_s(timeStr, sizeof(timeStr), "%06u", microTime);
    if (ret == -1) {
        return "0";
    }
    std::string tmp = timeStr;
    timeString.append(".").append(tmp);

    return timeString;
}

int Utils::GetMac(std::string &macAddress)
{
    MmMacInfo *macInfo = nullptr;
    int32_t count = 0;

    int ret = MmGetMac(&macInfo, &count);
    if (ret != PROFILING_SUCCESS || count == 0) {
        return PROFILING_FAILED;
    }

    macAddress = macInfo[0].addr;
    (void)MmGetMacFree(macInfo, count);

    return PROFILING_SUCCESS;
}

std::string Utils::GetEnvString(const std::string &name)
{
    std::lock_guard<std::mutex> lock(g_envMtx);

    std::string curEnv;
    constexpr int envValMaxLen = 1024 * 8; // 1024 * 8 : 8k
    char val[envValMaxLen + 1] = { 0 };
    int ret = MmGetEnv(name, val, envValMaxLen);
    if (ret == PROFILING_SUCCESS) {
        return std::string(val);
    }
    return curEnv;
}

std::string Utils::RelativePathToAbsolutePath(const std::string &path)
{
    if (path.empty()) {
        return "";
    }

    if (path[0] != '/') {
        std::string absolutePath = GetCwdString() + MSVP_SLASH + path;
        return absolutePath;
    } else {
        return std::string(path);
    }
}

std::string Utils::GetCwdString(void)
{
    constexpr int cwdValMaxLen = 1024 * 8; // 1024 * 8 : 8k
    char val[cwdValMaxLen + 1] = { 0 };
    (void)memset_s(val, cwdValMaxLen + 1, 0, cwdValMaxLen + 1);
    int ret = MmGetCwd(val, cwdValMaxLen);
    if (ret == PROFILING_SUCCESS) {
        return std::string(val);
    }
    return "";
}

bool Utils::CheckStringIsNonNegativeIntNum(const std::string &numberStr)
{
    if (numberStr.empty()) {
        return false;
    }

    const std::string maxIntValStr = std::to_string(INT32_MAX);  // max int string
    if (numberStr.length() > maxIntValStr.length()) {  // over max int value
        MSPROF_LOGE("[Utils::CheckStringIsNonNegativeIntNum]numberStr(%s) is too long", numberStr.c_str());
        return false;
    }

    for (unsigned int i = 0; i < numberStr.length(); ++i) {
        if (numberStr[i] < '0' || numberStr[i] > '9') {  // numberStr must be pure number
            MSPROF_LOGE("[Utils::CheckStringIsNonNegativeIntNum]numberStr(%s) is not a number", numberStr.c_str());
            return false;
        }
    }

    if (numberStr.length() == maxIntValStr.length()) {  // numberStr len equal max int val length
        for (unsigned int i = 0; i < numberStr.length(); ++i) {
            if (numberStr[i] > maxIntValStr[i]) {
                MSPROF_LOGE("[Utils::CheckStringIsNonNegativeIntNum]numberStr(%s) is over than %s",
                    numberStr.c_str(), maxIntValStr.c_str());
                return false;
            } else if (numberStr[i] == maxIntValStr[i]) {
                continue;
            } else {
                break;
            }
        }
    }
    return true;
}

bool Utils::CheckStringIsValidNatureNum(const std::string &numberStr)
{
    // length of nature number [1, 10]
    if (numberStr.empty() || numberStr.length() > std::to_string(UINT32_MAX).length()) {
        MSPROF_LOGE("[Utils::CheckStringIsValidNatureNum]numberStr(%s) is too empty or too long", numberStr.c_str());
        return false;
    }

    for (unsigned int i = 0; i < numberStr.length(); ++i) {
        if (numberStr[i] < '0' || numberStr[i] > '9') {  // numberStr must be pure number
            MSPROF_LOGE("[Utils::CheckStringIsValidNatureNum]numberStr(%s) is not a number", numberStr.c_str());
            return false;
        }
    }
    return true;
}

std::string Utils::IdeGetHomedir()
{
    std::lock_guard<std::mutex> lock(g_envMtx);
#if (defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER))
    std::string homedir;
#else
    struct passwd *pw = getpwuid(getuid());
    std::string homedir(pw != nullptr ? pw->pw_dir : "");
#endif
    if (!homedir.empty() && homedir[homedir.size() - 1] == '/' && homedir.size() > 1) {
        homedir.pop_back();
    }
    return homedir;
}

std::string Utils::IdeReplaceWaveWithHomedir(const std::string &path)
{
    if (path.size() < 2) { // 2 : path begin with ~/, so size >= 2
        return std::string(path);
    }

    if (path[0] == '~' && path[1] == '/') {
        std::string homedir = IdeGetHomedir();
        homedir.append(path.substr(1));
        return homedir;
    } else {
        return std::string(path);
    }
}

void Utils::EnsureEndsInSlash(std::string& path)
{
#if (defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER))
    char slash = '\\';
#else
    char slash = '/';
#endif
    if (!path.empty() && path[path.size() - 1] != slash) {
        path += slash;
    }
}

/**
 * @brief malloc memory and memset memory
 * @param size: the size of memory to malloc
 *
 * @return
 *        NULL: malloc memory failed
 *        not NULL: malloc memory succ
 */
void* Utils::ProfMalloc(size_t size)
{
    void *val = nullptr;
    errno_t err;

    if (size == 0) {
        return nullptr;
    }

    val = malloc(size);
    if (val == nullptr) {
        return nullptr;
    }

    err = memset_s(val, size, 0, size);
    if (err != EOK) {
        free(val);
        val = nullptr;
        return nullptr;
    }

    return val;
}

/**
 * @brief free memory
 * @param ptr: the memory to free
 * @return
 */
void Utils::ProfFree(VOID_PTR &ptr)
{
    if (ptr != nullptr) {
        free(ptr);
        ptr = nullptr;
    }
}

bool Utils::IsDeviceMapping()
{
    return false;
}

std::string Utils::GetCoresStr(const std::vector<int> &cores, const std::string &separator)
{
    analysis::dvvp::common::utils::UtilsStringBuilder<int> builder;
    return builder.Join(cores, separator);
}

std::string Utils::GetEventsStr(const std::vector<std::string> &events, const std::string &separator)
{
    analysis::dvvp::common::utils::UtilsStringBuilder<std::string> builder;
    return builder.Join(events, separator);
}

void Utils::PrintSysErrorMsg()
{
    char errBuf[MAX_ERR_STRING_LEN + 1] = {0};
    int32_t errorNo = MmGetErrorCode();
    MSPROF_LOGE("ErrorCode:%d, errinfo:%s", errorNo, MmGetErrorFormatMessage(errorNo,
        errBuf, MAX_ERR_STRING_LEN));
}

std::string Utils::GetSelfPath()
{
#if (defined(linux) || defined(__linux__))
    const std::string selfPath = "/proc/self/exe";
    uint32_t pathSize = MMPA_MAX_PATH + 1;
    SHARED_PTR_ALIA<char> curPath;
    MSVP_MAKE_SHARED_ARRAY_RET(curPath, char, pathSize, "");
    int len = readlink(selfPath.c_str(), curPath.get(), MMPA_MAX_PATH); // read self path of store
    if (len < 0 || len > MMPA_MAX_PATH) {
        MSPROF_LOGW("Can't Get self bin directory");
        return "";
    }
    curPath.get()[len] = '\0';
    return std::string(curPath.get());
#else
    TCHAR path[MAX_PATH] = { 0 };
    GetModuleFileName(nullptr, path, MAX_PATH);
    std::string result;
#ifdef UNICODE
    int iLen = WideCharToMultiByte(CP_ACP, 0, path, -1, nullptr, 0, nullptr, nullptr);
    if (iLen == 0) {
        MSPROF_LOGE("WideCharToMultiByte failed.");
        PrintSysErrorMsg();
        return result;
    }
    SHARED_PTR_ALIA<char> chRtn;
    MSVP_MAKE_SHARED_ARRAY_RET(chRtn, char, iLen * sizeof(char), "");
    WideCharToMultiByte(CP_ACP, 0, path, -1, chRtn.get(), iLen, nullptr, nullptr);
    result = chRtn.get();
#else
    result = path;
#endif
    MSPROF_LOGI("GetSelfPath is %s", BaseName(result).c_str());
    return result;
#endif
}

std::string Utils::CreateResultPath(const std::string &devId)
{
    std::string result;
    int deviceIdForHost = DEFAULT_HOST_ID;
    if (devId == std::to_string(deviceIdForHost)) {
        result.append("host");
    } else {
        result.append("device_").append(devId);
    }

    MSPROF_LOGI("[MSPROF][ProfStart] created task id %s of device %s", BaseName(result).c_str(), devId.c_str());

    return result;
}

std::string Utils::CreateTaskId(uint64_t index)
{
    std::stringstream taskId;
    taskId.fill('0');
    const int maxLen = 999999;
    const int fillLen = 6;
    taskId << std::setw(fillLen) << ((index % maxLen) + 1);
    std::string result = "PROF_" + taskId.str() + "_";

    MmSystemTimeT sysTime;
    int ret = MmGetLocalTime(&sysTime);
    if (ret == -1) {
        MSPROF_LOGW("Get time failed");
    }
    const int timeStrLen = 32;
    char timeStr[timeStrLen] = {0};
    ret = snprintf_s(timeStr, timeStrLen, timeStrLen - 1, "%04d%02d%02d%02d%02d%02d%03d", sysTime.wYear,
        sysTime.wMonth, sysTime.wDay, sysTime.wHour, sysTime.wMinute, sysTime.wSecond, sysTime.wMilliseconds);
    if (ret == -1) {
        MSPROF_LOGW("Format time failed");
    }
    result = result + timeStr + "_";

    const int letterNum = 26; // A - Z
    int taskIdLen = 32; // 32 : the lenght of the task id
    srand(time(nullptr));
    taskId.str("");
    for (int idx = 0; idx < taskIdLen; idx++) {
        taskId << static_cast<char>('A' + rand() % letterNum);
    }
    auto timeSinceEpoch = std::chrono::steady_clock::now().time_since_epoch();
    size_t hashId = std::hash<std::string>()(
        taskId.str() +
        std::to_string(std::chrono::duration_cast<std::chrono::nanoseconds>(timeSinceEpoch).count()) +
        std::to_string(analysis::dvvp::common::utils::Utils::GetClockMonotonicRaw()));
    taskId.str("");
    taskIdLen = 16; // 16 : the lenght of the task id
    const int hashMod = 18; // change hashId to [A-R]
    for (int idx = 0; idx < taskIdLen; idx++) {
        taskId << static_cast<char>('A' + hashId % hashMod);
        hashId = hashId / hashMod;
    }
    result = result + taskId.str();
    MSPROF_LOGI("Created output dir name %s", BaseName(result).c_str());
    return result;
}

std::string Utils::ProfCreateId(uint64_t addr)
{
    auto timeSinceEpoch = std::chrono::steady_clock::now().time_since_epoch();
    size_t hashId = std::hash<std::string>()(
        std::to_string(addr) +
        std::to_string(std::chrono::duration_cast<std::chrono::nanoseconds>(timeSinceEpoch).count()) +
        std::to_string(analysis::dvvp::common::utils::Utils::GetClockMonotonicRaw()));
    std::stringstream jobStrSteam;
    jobStrSteam << "trace" << std::setw(PROF_MGR_TRACE_ID_DEFAULT_LEN) << std::setfill('0') << hashId;
    return jobStrSteam.str();
}

uint32_t Utils::GenerateSignature(CONST_UINT8_PTR data, uint64_t len)
{
    uint32_t signature = ~0U;

    if (data == nullptr) {
        return signature;
    }

    static const int tableSize = 256;
    static const uint32_t signatureTable[tableSize] = {
        0x00000000, 0xF26B8303, 0xE13B70F7, 0x1350F3F4, 0xC79A971F, 0x35F1141C, 0x26A1E7E8, 0xD4CA64EB,
        0x8AD958CF, 0x78B2DBCC, 0x6BE22838, 0x9989AB3B, 0x4D43CFD0, 0xBF284CD3, 0xAC78BF27, 0x5E133C24,
        0x105EC76F, 0xE235446C, 0xF165B798, 0x030E349B, 0xD7C45070, 0x25AFD373, 0x36FF2087, 0xC494A384,
        0x9A879FA0, 0x68EC1CA3, 0x7BBCEF57, 0x89D76C54, 0x5D1D08BF, 0xAF768BBC, 0xBC267848, 0x4E4DFB4B,
        0x20BD8EDE, 0xD2D60DDD, 0xC186FE29, 0x33ED7D2A, 0xE72719C1, 0x154C9AC2, 0x061C6936, 0xF477EA35,
        0xAA64D611, 0x580F5512, 0x4B5FA6E6, 0xB93425E5, 0x6DFE410E, 0x9F95C20D, 0x8CC531F9, 0x7EAEB2FA,
        0x30E349B1, 0xC288CAB2, 0xD1D83946, 0x23B3BA45, 0xF779DEAE, 0x05125DAD, 0x1642AE59, 0xE4292D5A,
        0xBA3A117E, 0x4851927D, 0x5B016189, 0xA96AE28A, 0x7DA08661, 0x8FCB0562, 0x9C9BF696, 0x6EF07595,
        0x417B1DBC, 0xB3109EBF, 0xA0406D4B, 0x522BEE48, 0x86E18AA3, 0x748A09A0, 0x67DAFA54, 0x95B17957,
        0xCBA24573, 0x39C9C670, 0x2A993584, 0xD8F2B687, 0x0C38D26C, 0xFE53516F, 0xED03A29B, 0x1F682198,
        0x5125DAD3, 0xA34E59D0, 0xB01EAA24, 0x42752927, 0x96BF4DCC, 0x64D4CECF, 0x77843D3B, 0x85EFBE38,
        0xDBFC821C, 0x2997011F, 0x3AC7F2EB, 0xC8AC71E8, 0x1C661503, 0xEE0D9600, 0xFD5D65F4, 0x0F36E6F7,
        0x61C69362, 0x93AD1061, 0x80FDE395, 0x72966096, 0xA65C047D, 0x5437877E, 0x4767748A, 0xB50CF789,
        0xEB1FCBAD, 0x197448AE, 0x0A24BB5A, 0xF84F3859, 0x2C855CB2, 0xDEEEDFB1, 0xCDBE2C45, 0x3FD5AF46,
        0x7198540D, 0x83F3D70E, 0x90A324FA, 0x62C8A7F9, 0xB602C312, 0x44694011, 0x5739B3E5, 0xA55230E6,
        0xFB410CC2, 0x092A8FC1, 0x1A7A7C35, 0xE811FF36, 0x3CDB9BDD, 0xCEB018DE, 0xDDE0EB2A, 0x2F8B6829,
        0x82F63B78, 0x709DB87B, 0x63CD4B8F, 0x91A6C88C, 0x456CAC67, 0xB7072F64, 0xA457DC90, 0x563C5F93,
        0x082F63B7, 0xFA44E0B4, 0xE9141340, 0x1B7F9043, 0xCFB5F4A8, 0x3DDE77AB, 0x2E8E845F, 0xDCE5075C,
        0x92A8FC17, 0x60C37F14, 0x73938CE0, 0x81F80FE3, 0x55326B08, 0xA759E80B, 0xB4091BFF, 0x466298FC,
        0x1871A4D8, 0xEA1A27DB, 0xF94AD42F, 0x0B21572C, 0xDFEB33C7, 0x2D80B0C4, 0x3ED04330, 0xCCBBC033,
        0xA24BB5A6, 0x502036A5, 0x4370C551, 0xB11B4652, 0x65D122B9, 0x97BAA1BA, 0x84EA524E, 0x7681D14D,
        0x2892ED69, 0xDAF96E6A, 0xC9A99D9E, 0x3BC21E9D, 0xEF087A76, 0x1D63F975, 0x0E330A81, 0xFC588982,
        0xB21572C9, 0x407EF1CA, 0x532E023E, 0xA145813D, 0x758FE5D6, 0x87E466D5, 0x94B49521, 0x66DF1622,
        0x38CC2A06, 0xCAA7A905, 0xD9F75AF1, 0x2B9CD9F2, 0xFF56BD19, 0x0D3D3E1A, 0x1E6DCDEE, 0xEC064EED,
        0xC38D26C4, 0x31E6A5C7, 0x22B65633, 0xD0DDD530, 0x0417B1DB, 0xF67C32D8, 0xE52CC12C, 0x1747422F,
        0x49547E0B, 0xBB3FFD08, 0xA86F0EFC, 0x5A048DFF, 0x8ECEE914, 0x7CA56A17, 0x6FF599E3, 0x9D9E1AE0,
        0xD3D3E1AB, 0x21B862A8, 0x32E8915C, 0xC083125F, 0x144976B4, 0xE622F5B7, 0xF5720643, 0x07198540,
        0x590AB964, 0xAB613A67, 0xB831C993, 0x4A5A4A90, 0x9E902E7B, 0x6CFBAD78, 0x7FAB5E8C, 0x8DC0DD8F,
        0xE330A81A, 0x115B2B19, 0x020BD8ED, 0xF0605BEE, 0x24AA3F05, 0xD6C1BC06, 0xC5914FF2, 0x37FACCF1,
        0x69E9F0D5, 0x9B8273D6, 0x88D28022, 0x7AB90321, 0xAE7367CA, 0x5C18E4C9, 0x4F48173D, 0xBD23943E,
        0xF36E6F75, 0x0105EC76, 0x12551F82, 0xE03E9C81, 0x34F4F86A, 0xC69F7B69, 0xD5CF889D, 0x27A40B9E,
        0x79B737BA, 0x8BDCB4B9, 0x988C474D, 0x6AE7C44E, 0xBE2DA0A5, 0x4C4623A6, 0x5F16D052, 0xAD7D5351
    };

    static const int offset = 8;
    for (uint32_t i = 0; i < len; ++i) {
        signature = signatureTable[(signature ^ *data++) & 0xff] ^ (signature >> offset);
    }
    return signature;
}

std::string Utils::ConvertIntToStr(const int interval)
{
    std::stringstream result;
    result << std::setw(4) << std::setfill('0') << interval << std::endl; // 4 : Fill to 4 characters
    return Trim(result.str());
}

int32_t Utils::GetPid()
{
    return MmGetPid();
}

void Utils::RemoveEndCharacter(std::string &input, const char end)
{
    if (input.empty() || input[input.size() - 1] != end) {
        return;
    }
    input.resize(input.size() - 1);
}

bool Utils::IsAppName(const std::string paramsName)
{
    std::string paramBaseName = BaseName(paramsName);
    std::string pythonName = "python";
    if (paramBaseName.compare("bash") == 0 || paramBaseName.compare("sh") == 0 ||
        paramBaseName.substr(0, pythonName.size()) == pythonName) {
        return false;
    }
    return true;
}

bool Utils::IsDynProfMode()
{
    if (ConfigManager::instance()->GetPlatformType() != PlatformType::CLOUD_TYPE) {
        return false;
    }
    if (GetEnvString(PROFILING_MODE_ENV) != DAYNAMIC_PROFILING_VALUE) {
        return false;
    }
    return true;
}

bool Utils::IsClusterRunEnv()
{
    std::string rankTableFilePath = Utils::GetEnvString(RANK_TABLE_FILE_ENV);
    MSPROF_LOGI("Environment variable RANK_TABLE_FILE = %s", rankTableFilePath.c_str());
    if (rankTableFilePath.empty()) {
        return false;
    }
    if (MmAccess(rankTableFilePath.c_str()) != PROFILING_SUCCESS) {
        return false;
    }
    if (MmIsDir(rankTableFilePath.c_str()) == PROFILING_SUCCESS) {
        return false;
    }
    return true;
}

int32_t Utils::GetRankId()
{
    constexpr int32_t invalidRankId = -1;
    if (!IsClusterRunEnv()) {
        return invalidRankId;
    }
    std::string rankIdStr = Utils::GetEnvString(RANK_ID_ENV);
    MSPROF_LOGI("Environment variable RANK_ID = %s", rankIdStr.c_str());
    if (!CheckStringIsValidNatureNum(rankIdStr)) {
        return invalidRankId;
    }
    return std::stoi(rankIdStr);
}

std::vector<std::string> Utils::GenEnvPairVec(const std::vector<std::string> &envVec)
{
    std::vector<std::string> envPairVec;
    for (auto env : envVec) {
        std::string envStr = GetEnvString(env);
        if (envStr.empty()) {
            MSPROF_LOGW("Failed to get value of %s.", env.c_str());
            continue;
        }
        envStr = env + "=" + envStr;
        envPairVec.push_back(envStr);
    }
    return envPairVec;
}

bool Utils::PythonEnvReady()
{
    std::string cmd = "python3";
    std::vector<std::string> argsVec;
    argsVec.push_back("--version");
    MmProcess taskPid = MSVP_MMPROCESS;
    ExecCmdParams execCmdParams(cmd, false, "/dev/null");
    int exitCode = VALID_EXIT_CODE;
    std::vector<std::string> varVec = {"PATH", "LD_LIBRARY_PATH"};
    std::vector<std::string> envsVec = GenEnvPairVec(varVec);
    int ret = ExecCmd(execCmdParams, argsVec, envsVec, exitCode, taskPid);
    return (ret == PROFILING_SUCCESS) ? true : false;
}

bool Utils::AnalysisEnvReady(std::string &msprofPyPath)
{
    Dl_info dlInfo;
    dladdr((void *)AnalysisEnvReady, &dlInfo);
    std::string selfPath(dlInfo.dli_fname);
    char realPathStr[MMPA_MAX_PATH] = { 0 };
    if (BaseName(selfPath).compare(PROF_MSPROF_BIN_NAME) == 0) {
        selfPath = GetSelfPath();
        if (selfPath.empty()) {
            return false;
        }
    }
    int ret = MmRealPath(selfPath.c_str(), realPathStr, MMPA_MAX_PATH);
    if (ret != PROFILING_SUCCESS) {
        return false;
    }
    std::string realPath(realPathStr);
    std::string baseDir = realPath;
    const int dirDiff = 3;
    for (int i = 0; i < dirDiff; i++) {
        baseDir = DirName(baseDir);
    }
    if (BaseName(realPath).compare(PROF_MSPROF_SO_NAME) != 0 &&
        BaseName(realPath).compare(PROF_MSPROF_BIN_NAME) != 0) {
        return false;
    }
    if (BaseName(realPath).compare(PROF_MSPROF_SO_NAME) == 0) {
        baseDir = baseDir + MSVP_SLASH + "tools";
    }

    msprofPyPath = baseDir + MSVP_SLASH + PROF_MSPROF_PY_PATH;
    return true;
}

int Utils::CloudAnalyze(const std::string &jobDir)
{
    MSPROF_LOGI("Cloud Pre-analysis start.");
    if (jobDir.empty() || MmAccess2(jobDir.c_str(), M_R_OK) != PROFILING_SUCCESS) {
        MSPROF_LOGW("jobDir is not exist or jobDir is not accessable.");
        return PROFILING_FAILED;
    }
    if (!PythonEnvReady()) {
        MSPROF_LOGW("python3 env is not ready.");
        return PROFILING_FAILED;
    }
    std::string msprofPyPath;
    if (!AnalysisEnvReady(msprofPyPath) || msprofPyPath.empty()) {
        MSPROF_LOGW("msprof.py is not ready.");
        return PROFILING_FAILED;
    }
    if ((MmAccess2(msprofPyPath, M_X_OK) != PROFILING_SUCCESS) || IsSoftLink(msprofPyPath)) {
        MSPROF_LOGW("msprof.py can is not executable or it is softlink.");
        return PROFILING_FAILED;
    }

    std::string cmd = "python3";
    ExecCmdParams execCmdParams(cmd, false, "");
    int exitCode = VALID_EXIT_CODE;
    MmProcess taskPid = MSVP_MMPROCESS;
    std::vector<std::string> argsImportV = {
        msprofPyPath,
        "import", "-dir=" + jobDir
    };
    std::vector<std::string> varVec = {"PATH", "LD_LIBRARY_PATH", "PYTHONPATH"};
    std::vector<std::string> envsVec = GenEnvPairVec(varVec);
    int ret = ExecCmd(execCmdParams, argsImportV, envsVec, exitCode, taskPid);
    if (ret != PROFILING_SUCCESS) {
        return ret;
    }
    return PROFILING_SUCCESS;
}

std::string Utils::RealPath(const std::string &path)
{
    char realPath[MMPA_MAX_PATH] = { 0 };
    int ret = MmRealPath(path.c_str(), realPath, MMPA_MAX_PATH);
    if (ret != PROFILING_SUCCESS) {
        return "";
    }
    return std::string(realPath);
}

int32_t WriteFile(const std::string &absolutePath, const std::string &recordFile, const std::string &profName)
{
#if (defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER))
    return analysis::dvvp::common::error::PROFILING_SUCCESS;
#else
    FILE *file;

    if ((file = fopen(absolutePath.c_str(), "a")) == nullptr) {
        MSPROF_LOGE("Failed to open %s", recordFile.c_str());
        return analysis::dvvp::common::error::PROFILING_FAILED;
    }
    int fd = fileno(file);
    if (fd  < 0) {
        fclose(file);
        MSPROF_LOGE("Failed to get file description for %s", recordFile.c_str());
        return analysis::dvvp::common::error::PROFILING_FAILED;
    }
    if (flock(fd, LOCK_SH) != 0) {
        fclose(file);
        MmClose(fd);
        MSPROF_LOGE("Failed to get file lock");
        return analysis::dvvp::common::error::PROFILING_FAILED;
    }

    if ((fwrite(profName.c_str(), FILE_WRITE_SIZE, profName.length(), file)) != profName.length()) {
        UnFileLock(file);
        fclose(file);
        MmClose(fd);
        MSPROF_LOGE("Failed to write prof dir");
        return analysis::dvvp::common::error::PROFILING_FAILED;
    }

    if (UnFileLock(file) != PROFILING_SUCCESS) {
        fclose(file);
        MmClose(fd);
        return analysis::dvvp::common::error::PROFILING_FAILED;
    }

    fclose(file);
    MmClose(fd);
    return analysis::dvvp::common::error::PROFILING_SUCCESS;
#endif
}

int32_t UnFileLock(FILE *file)
{
#if (defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER))
    return analysis::dvvp::common::error::PROFILING_SUCCESS;
#else
    for (unsigned int i = 0; i < UNLOCK_NUM; ++i) {
        if (flock(fileno(file), LOCK_UN) == 0) {
            return analysis::dvvp::common::error::PROFILING_SUCCESS;
        }
    }
    MSPROF_LOGE("Failed to release file lock");
    return analysis::dvvp::common::error::PROFILING_FAILED;
#endif
}
}  // namespace utils
}  // namespace common
}  // namespace dvvp
}  // namespace analysis
