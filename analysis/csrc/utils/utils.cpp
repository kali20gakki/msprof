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

#include "utils.h"

#include <sstream>

namespace Analysis {
namespace Utils {
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
}  // namespace Utils
}  // namespace Analysis
