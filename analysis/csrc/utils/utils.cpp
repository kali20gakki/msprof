/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
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
#include "analysis/csrc/utils/utils.h"

#include "analysis/csrc/dfx/error_code.h"
#include "analysis/csrc/parser/environment/context.h"

namespace Analysis {
namespace Utils {
using namespace Analysis::Parser::Environment;

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

std::vector<std::string> Split(const std::string &str, const std::string &delimiter)
{
    std::vector<std::string> res;
    if (delimiter.empty()) {
        res.emplace_back(str);
        return res;
    }
    size_t start = 0;
    size_t end;
    while ((end = str.find(delimiter, start)) != std::string::npos) {
        res.emplace_back(str.substr(start, end - start));
        start = end + delimiter.length();
    }
    res.emplace_back(str.substr(start));
    return res;
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
    uint16_t deviceId = Parser::Environment::HOST_ID;
    auto dirName = Split(tempStr, "/").back();
    if (dirName == "host") {
        return deviceId;
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
    while (it != s.end() && std::isdigit(*it)) ++it;
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

}  // namespace Utils
}  // namespace Analysis