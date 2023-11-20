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

namespace Analysis {
namespace Utils {
std::string Join(const std::vector<std::string> &str, const std::string &delimiter);
std::vector<std::string> Split(const std::string &str, const std::string &delimiter);
int StrToU16(uint16_t &dest, const std::string &numStr);
int StrToU64(uint64_t &dest, const std::string &numStr);
}  // namespace Utils
}  // namespace Analysis

#endif // ANALYSIS_UTILS_UTILS_H
