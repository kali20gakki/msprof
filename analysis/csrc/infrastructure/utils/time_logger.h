/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : time_logger.h
 * Description        : 利用RAII机制实现程序运行时间计算
 * Author             : msprof team
 * Creation Date      : 2023/12/5
 * *****************************************************************************
 */
#ifndef ANALYSIS_UTILS_TIME_LOGGER_H
#define ANALYSIS_UTILS_TIME_LOGGER_H

#include <string>
#include <chrono>

#include "analysis/csrc/infrastructure/dfx/log.h"

namespace Analysis {
namespace Utils {

class TimeLogger {
public:
    TimeLogger(const std::string tag)
        : tag_(tag), startTime_(std::chrono::high_resolution_clock::now())
    {
        INFO("% start", tag_);
    }

    ~TimeLogger()
    {
        auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now() - startTime_).count();
        INFO("% end, cost time = % ms", tag_, dur);
    }
private:
    std::chrono::time_point<std::chrono::high_resolution_clock> startTime_;
    std::string tag_;
};
}
}
#endif // ANALYSIS_UTILS_TIME_LOGGER_H
