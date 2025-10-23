/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2025-2025
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : sync_utils.cpp
 * Description        : 文件写作锁的工具类
 * Author             : msprof team
 * Creation Date      : 2025/1/02
 * *****************************************************************************
 */

#include "analysis/csrc/infrastructure/dump_tools/include/sync_utils.h"
#include <iomanip>
#include <ctime>
#include <sstream>
#include <chrono>

namespace Analysis {
namespace Infra {
const std::string& GetTimeStampStr()
{
    const static std::string TIMESTAMP_STR = []() -> std::string {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto tm = std::localtime(&time);
        if (!tm) {
            return "99999999999999"; // When localtime is abnormal, the default value is '99999999999999'
        }
        std::ostringstream oss;
        oss << std::put_time(tm, "%Y%m%d%H%M%S");
        return oss.str();
    }();
    return TIMESTAMP_STR;
}
}
}
