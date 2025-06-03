/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2024
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : utils.cpp
 * Description        : 辅助函数
 * Author             : msprof team
 * Creation Date      : 2023/11/16
 * *****************************************************************************
 */

#include <unordered_set>
#include <iomanip>
#include "utils.h"

#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/domain/services/environment/context.h"

namespace Analysis {
namespace Utils {
using namespace Analysis::Domain::Environment;

std::string Join(const std::vector<std::string> &str, const std::string &delimiter)
{
    std::stringstream ss;
    for (size_t i = 0; i < str.size(); ++i) {
        if (i > 0) {
            ss << delimiter;
        }
        ss << str.at(i);
    }
    return ss.str();
}

std::vector<std::string> Split(const std::string &str, const std::string &delimiter, const int &splitPosition)
{
    std::vector<std::string> res;
    if (delimiter.empty()) {
        res.emplace_back(str);
        return res;
    }
    size_t start = 0;
    size_t end;
    if (splitPosition < 0) {
        while ((end = str.find(delimiter, start)) != std::string::npos) {
            res.emplace_back(str.substr(start, end - start));
            start = end + delimiter.length();
        }
        res.emplace_back(str.substr(start));
    } else {
        int count = 0;
        while (count < splitPosition && (end = str.find(delimiter, start)) != std::string::npos) {
            start = end + delimiter.length();
            count++;
        }
        if (count < splitPosition) {
            return res;
        }
        res.emplace_back(str.substr(0, start - 1));
        res.emplace_back(str.substr(start));
    }
    return res;
}

std::string Rstrip(const std::string &str1, const std::string &str2)
{
    size_t end = str1.find_last_not_of(str2);
    return (end == std::string::npos) ? "" : str1.substr(0, end + 1);
}

int StrToU16(uint16_t &dest, const std::string &numStr)
{
    if (numStr.empty()) {
        ERROR("StrToU16 failed, the input string is empty.");
        return ANALYSIS_ERROR;
    }
    size_t pos = 0;
    try {
        dest = std::stoi(numStr, &pos);
    } catch (...) {
        ERROR("StrToU16 failed, the input string is '%'.", numStr.c_str());
        return ANALYSIS_ERROR;
    }
    if (pos != numStr.size()) {
        ERROR("StrToU16 failed, the input string is '%'.", numStr.c_str());
        return ANALYSIS_ERROR;
    }
    return ANALYSIS_OK;
}

int StrToU32(uint32_t &dest, const std::string &numStr)
{
    if (numStr.empty()) {
        ERROR("StrToU32 failed, the input string is empty.");
        return ANALYSIS_ERROR;
    }
    size_t pos = 0;
    try {
        dest = std::stoul(numStr, &pos);
    } catch (...) {
        ERROR("StrToU32 failed, the input string is '%'.", numStr.c_str());
        return ANALYSIS_ERROR;
    }
    if (pos != numStr.size()) {
        ERROR("StrToU32 failed, the input string is '%'.", numStr.c_str());
        return ANALYSIS_ERROR;
    }
    return ANALYSIS_OK;
}

int StrToU64(uint64_t &dest, const std::string &numStr)
{
    if (numStr.empty()) {
        ERROR("StrToU64 failed, the input string is empty.");
        return ANALYSIS_ERROR;
    }
    size_t pos = 0;
    try {
        dest = std::stoull(numStr, &pos);
    } catch (...) {
        ERROR("StrToU64 failed, the input string is '%'.", numStr.c_str());
        return ANALYSIS_ERROR;
    }
    if (pos != numStr.size()) {
        ERROR("StrToU64 failed, the input string is '%'.", numStr.c_str());
        return ANALYSIS_ERROR;
    }
    return ANALYSIS_OK;
}

int StrToDouble(double &dest, const std::string &numStr)
{
    if (numStr.empty()) {
        ERROR("StrToDouble failed, the input string is empty.");
        return ANALYSIS_ERROR;
    }
    size_t pos = 0;
    try {
        dest = std::stod(numStr, &pos);
    } catch (...) {
        ERROR("StrToDouble failed, the input string is '%'.", numStr.c_str());
        return ANALYSIS_ERROR;
    }
    if (pos != numStr.size()) {
        ERROR("StrToDouble failed, the input string is '%'.", numStr.c_str());
        return ANALYSIS_ERROR;
    }
    return ANALYSIS_OK;
}

uint16_t GetDeviceIdByDevicePath(const std::string &filePath)
{
    auto tempStr = filePath;
    while (tempStr.back() == '/') {
        tempStr.pop_back();
    }
    // 默认host的deviceId为64
    uint16_t deviceId = INVALID_DEVICE_ID;
    auto dirName = Split(tempStr, "/").back();
    if (dirName == "host") {
        return HOST_ID;
    }
    if (StrToU16(deviceId, Split(dirName, "_").back()) != ANALYSIS_OK) {
        ERROR("DeviceId to uint16_t failed.");
        return deviceId;
    }
    return deviceId;
}

bool IsNumber(const std::string& s)
{
    std::string::const_iterator it = s.begin();
    while (it != s.end() && std::isdigit(*it)) {
        ++it;
    }
    return !s.empty() && it == s.end();
}

uint64_t Contact(uint32_t high, uint32_t low)
{
    const uint8_t shift = 32;
    return (static_cast<uint64_t>(high) << shift) | low;
}

bool IsDoubleEqual(double checkDouble, double standard)
{
    const double eps = 1e-9;
    return (std::abs(checkDouble - standard) < eps);
}

std::string AddQuotation(std::string str)
{
    return Join({"\"", str, "\""}, "");
}

std::string DivideByPowersOfTenWithPrecision(uint64_t value, int accuracy, int scale)
{
    // scale代表除以10的多少次幂，比如3就是除以10^3，accuracy代表保留位数
    std::string numStr = std::to_string(value);
    // 在位置前插入小数点
    if (static_cast<int>(numStr.size()) <= scale) {
        numStr.insert(0, scale - numStr.size() + 1, '0');
    }
    numStr.insert(numStr.size() - scale, ".");
    if (scale == accuracy) { // 精度与移位数相等，直接返回即可
        return numStr;
    } else if (accuracy > scale) { // 精度比移位数大，需要末尾补0
        numStr.insert(numStr.end(),  accuracy - scale, '0');
        return numStr;
    } else {  // 精度比移位数小，需要截取numStr.size() - (scale - accuracy)长个字符串
        return numStr.substr(0, numStr.size() + accuracy - scale);
    }
}

bool EndsWith(const std::string &str, const std::string &suffix)
{
    if (suffix.size() > str.size()) {
        return false;
    }
    return str.substr(str.size() - suffix.size()) == suffix;
}

std::string DoubleToStr(const double &value, const uint16_t &scale)
{
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(scale) << value;
    std::string result = oss.str();

    // 去掉末尾的零
    size_t pos = result.find('.');
    if (pos != std::string::npos) {
        // 找到最后一个非零的位置
        size_t lastNonZero = result.find_last_not_of('0');
        if (lastNonZero != std::string::npos) {
            result = result.substr(0, lastNonZero + 1);
            if (result.back() == '.') {
                result.pop_back();
            }
        } else {
            result = result.substr(0, pos);
        }
    }
    return result;
}

double RoundToDecimalPlaces(const double num, int decimalPlaces)
{
    double multiplier = std::pow(10.0, decimalPlaces);
    return std::round(num * multiplier) / multiplier;
}
}  // namespace Utils
}  // namespace Analysis
