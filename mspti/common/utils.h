/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : utils.h
 * Description        : Common Utils.
 * Author             : msprof team
 * Creation Date      : 2024/05/07
 * *****************************************************************************
*/

#ifndef MSPTI_COMMON_UTILS_H
#define MSPTI_COMMON_UTILS_H

#include <cstdint>
#include <exception>
#include <memory>
#include <string>

constexpr uint32_t SECTONSEC = 1000000000UL;

namespace Mspti {
namespace Common {

template<typename Types, typename... Args>
inline void MsptiMakeSharedPtr(std::shared_ptr<Types> &ptr, Args&&... args)
{
    try {
        ptr = std::make_shared<Types>(std::forward<Args>(args)...);
    } catch (std::bad_alloc &e) {
        throw;
    } catch (...) {
        ptr = nullptr;
        return;
    }
}

template<typename Types, typename... Args>
inline void MsptiMakeUniquePtr(std::unique_ptr<Types> &ptr, Args&&... args)
{
    try {
        ptr = std::make_unique<Types>(std::forward<Args>(args)...);
    } catch (std::bad_alloc &e) {
        throw;
    } catch (...) {
        ptr = nullptr;
        return;
    }
}

class Utils {
public:
    static uint64_t GetClockMonotonicRawNs();
    static uint64_t GetClockRealTimeNs();
    static uint64_t GetHostSysCnt();
    static uint32_t GetPid();
    static uint32_t GetTid();
    static std::string RealPath(const std::string& path);
    static std::string RelativeToAbsPath(const std::string& path);
    static bool FileExist(const std::string &path);
};

}  // Common
}  // Mspti

#endif
