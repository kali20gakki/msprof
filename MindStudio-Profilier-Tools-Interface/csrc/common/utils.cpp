/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : utils.cpp
 * Description        : Common Utils.
 * Author             : msprof team
 * Creation Date      : 2024/05/07
 * *****************************************************************************
*/

#include "csrc/common/utils.h"

#include <ctime>
#include <sys/prctl.h>
#include <unistd.h>
#include <unordered_map>
#include <sys/syscall.h>
#include <linux/limits.h>
#include "securec.h"
#include "csrc/common/plog_manager.h"

namespace Mspti {
namespace Common {

const std::unordered_map<std::string, std::string> INVALID_CHAR = {
    {"\n", "\\n"}, {"\f", "\\f"}, {"\r", "\\r"}, {"\b", "\\b"}, {"\t", "\\t"},
    {"\v", "\\v"}, {"\u007F", "\\u007F"}, {"\"", "\\\""}, {"'", "\'"},
    {"\\", "\\\\"}, {"%", "\\%"}, {">", "\\>"}, {"<", "\\<"}, {"|", "\\|"},
    {"&", "\\&"}, {"$", "\\$"}, {";", "\\;"}, {"`", "\\`"}
};

uint64_t Utils::GetClockMonotonicRawNs()
{
    struct timespec ts;
    if (memset_s(&ts, sizeof(timespec), 0, sizeof(timespec)) != EOK) {
        return 0;
    }
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return static_cast<uint64_t>(ts.tv_sec) * SECTONSEC + static_cast<uint64_t>(ts.tv_nsec);
}

uint64_t Utils::GetClockRealTimeNs()
{
    struct timespec ts;
    if (memset_s(&ts, sizeof(timespec), 0, sizeof(timespec)) != EOK) {
        return 0;
    }
    clock_gettime(CLOCK_REALTIME, &ts);
    return static_cast<uint64_t>(ts.tv_sec) * SECTONSEC + static_cast<uint64_t>(ts.tv_nsec);
}

uint64_t Utils::GetHostSysCnt()
{
    uint64_t cycles;
#if defined(__aarch64__)
    asm volatile("mrs %0, cntvct_el0" : "=r"(cycles));
#elif defined(__x86_64__)
    constexpr uint32_t uint32Bits = 32U;
    uint32_t hi = 0;
    uint32_t lo = 0;
    __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
    cycles = (static_cast<uint64_t>(lo)) | ((static_cast<uint64_t>(hi)) << uint32Bits);
#elif defined(__arm__)
    const uint32_t uint32Bits = 32U;
    uint32_t hi = 0;
    uint32_t lo = 0;
    asm volatile("mrrc p15, 1, %0, %1, c14" : "=r"(lo), "=r"(hi));
    cycles = (static_cast<uint64_t>(lo)) | ((static_cast<uint64_t>(hi)) << uint32Bits);
#else
    cycles = 0;
#endif
    return cycles;
}

uint32_t Utils::GetPid()
{
    static thread_local uint32_t pid = static_cast<uint32_t>(getpid());
    return pid;
}

uint32_t Utils::GetTid()
{
    static thread_local uint32_t tid = static_cast<uint32_t>(syscall(SYS_gettid));
    return tid;
}

std::string Utils::RealPath(const std::string& path)
{
    if (path.empty() || path.size() > PATH_MAX) {
        return "";
    }
    char realPath[PATH_MAX] = {0};
    if (realpath(path.c_str(), realPath) == nullptr) {
        return "";
    }
    return std::string(realPath);
}

std::string Utils::RelativeToAbsPath(const std::string& path)
{
    if (path.empty() || path.size() > PATH_MAX) {
        return "";
    }
    if (path[0] != '/') {
        char pwd_path[PATH_MAX] = {0};
        if (getcwd(pwd_path, PATH_MAX) != nullptr) {
            return std::string(pwd_path) + "/" + path;
        }
        return "";
    }
    return std::string(path);
}

bool Utils::FileExist(const std::string &path)
{
    if (path.empty() || path.size() > PATH_MAX) {
        return false;
    }
    return (access(path.c_str(), F_OK) == 0) ? true : false;
}

bool Utils::FileReadable(const std::string &path)
{
    if (path.empty() || path.size() > PATH_MAX) {
        return false;
    }
    return (access(path.c_str(), R_OK) == 0) ? true : false;
}

bool Utils::CheckCharValid(const std::string &str)
{
    for (auto &item: INVALID_CHAR) {
        if (str.find(item.first) != std::string::npos) {
            MSPTI_LOGE("The path contains invalid character: %s.", item.second.c_str());
            return false;
        }
    }
    return true;
}

}  // Common
}  // Mspti
