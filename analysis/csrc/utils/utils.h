/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : utils.h
 * Description        : 辅助函数
 * Author             : msprof team
 * Creation Date      : 2023/11/13
 * *****************************************************************************
 */

#ifndef ANALYSIS_UTILS_UTILS_H
#define ANALYSIS_UTILS_UTILS_H

#include <string>
#include <vector>
#include <memory>

#include "log.h"

namespace Analysis {
namespace Utils {
using CHAR_PTR = char *;
std::string Join(const std::vector<std::string> &str, const std::string &delimiter);
std::vector<std::string> Split(const std::string &str, const std::string &delimiter);
int StrToU16(uint16_t &dest, const std::string &numStr);
int StrToU64(uint64_t &dest, const std::string &numStr);
int StrToDouble(double &dest, const std::string &numStr);
// 根据所给的device路径获取对应的deviceId test/PROF_1234/device_x 返回x对应的数值
// 对于传入的 test/PROF_1234/host, 也返回host对应的id（64）
uint16_t GetDeviceIdByDevicePath(const std::string &filePath);

template<class T, class ...Args>
std::shared_ptr<T> MakeShared(const Args &...args)
{
    std::shared_ptr<T> sp;
    try {
        sp = std::make_shared<T>(args...);
    } catch (...) {
        ERROR("make_shared failed");
        return nullptr;
    }
    return sp;
}

template<class T>
bool Reserve(std::vector<T> &vec, size_t s)
{
    try {
        vec.reserve(s);
    } catch (...) {
        ERROR("Reserve vector failed");
        return false;
    }
    return true;
}

template<typename T, typename U, U T::*element>
void Sort(std::vector<std::shared_ptr<T>> &items)
{
    std::stable_sort(items.begin(), items.end(),
                     [&](const std::shared_ptr<T> &lhs, const std::shared_ptr<T> &rhs) {
                         return lhs && rhs && (*lhs).*element < (*rhs).*element;
                     });
}

template<typename T, typename V>
T ReinterpretConvert(V ptr)
{
    return reinterpret_cast<T>(ptr);
}

template<typename T, typename U>
inline std::shared_ptr<T> ReinterpretPointerCast(const std::shared_ptr<U> &r) noexcept
{
    return std::shared_ptr<T>(r, reinterpret_cast<typename std::shared_ptr<T>::element_type *>(r.get()));
}

// 元素数量等于1时，模板特例
template<typename T>
void ConvertToString(std::ostringstream &oss, T t)
{
    oss << t;
}

// 元素数量大于1时，递归展开可变参数模板
template<typename T, typename... Args>
void ConvertToString(std::ostringstream &oss, T t, Args... args)
{
    oss << t << "_";
    ConvertToString(oss, args...);
}

// 调用可变参数模板，返回字符串
template<typename... Args>
std::string ConvertToString(Args... args)
{
    std::ostringstream oss;
    ConvertToString(oss, args...);
    return oss.str();
}
}  // namespace Utils
}  // namespace Analysis

#endif // ANALYSIS_UTILS_UTILS_H
