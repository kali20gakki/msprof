/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : process_spy.h
 * Description        : process test spy
 * Author             : msprof team
 * Creation Date      : 2024/4/12
 * *****************************************************************************
 */

#ifndef ANALYSIS_UT_INFRASTRUCTURE_PROCESS_CONTROL_TEST_SPY_H
#define ANALYSIS_UT_INFRASTRUCTURE_PROCESS_CONTROL_TEST_SPY_H
#include <iostream>
#include <map>
#include <string>

namespace Analysis {

class ProcessSpy final {
public:
    explicit ProcessSpy(const std::string& processName) : processName_(processName)
    {
        std::cout << processName_ << " Enter!" << std::endl;
    }
    ~ProcessSpy() {std::cout << processName_ << " Out!" << std::endl;}
    static uint32_t GetResult(const std::string& processName);
    static void SetResult(const std::string& processName, uint32_t result);
    static void ClearResult();

private:
    static std::map<std::string, uint32_t>& GetResultMap();

private:
    std::string processName_;
};

#define PROCESS_TRACE(processName) \
ProcessSpy processTrace(#processName)

}
#endif