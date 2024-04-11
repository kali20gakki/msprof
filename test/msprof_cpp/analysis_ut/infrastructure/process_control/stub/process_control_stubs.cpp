/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : tree_analyzer_utest.cpp
 * Description        : TreeAnalyzer UT
 * Author             : msprof team
 * Creation Date      : 2024/4/9
 * *****************************************************************************
 */
#include <thread>
#include <string>
#include "analysis/csrc/dfx/log.h"

namespace Analysis {

std::string Analysis::Log::GetFileName(std::string const&) {return {};}
void Analysis::Log::PrintMsg(std::string const&, std::string const&, std::string const&) const {}
void Log::LogMsg(const std::string& message, const std::string &level,
                 const std::string &fileName, const uint32_t &line)
{
}

namespace Utils {

void Analysis::Utils::FileWriter::Close() {}


} // namespace Utils
} // namespace Analysis

