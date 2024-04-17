/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : process_spy.cpp
 * Description        : process test spy
 * Author             : msprof team
 * Creation Date      : 2024/4/12
 * *****************************************************************************
 */
#include "process_spy.h"

namespace Analysis {

uint32_t ProcessSpy::GetResult(const std::string& processName)
{
    auto it = GetResultMap().find(processName);
    if (it == GetResultMap().end()) {
        return 0;
    }
    return it->second;
}

void ProcessSpy::SetResult(const std::string& processName, uint32_t result)
{
    GetResultMap()[processName] = result;
}

void ProcessSpy::ClearResult()
{
    GetResultMap().clear();
}

std::map<std::string, uint32_t>& ProcessSpy::GetResultMap()
{
    static std::map<std::string, uint32_t> ins;
    return ins;
}

}
