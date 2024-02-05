/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : time_utils.cpp
 * Description        : 辅助函数
 * Author             : msprof team
 * Creation Date      : 2023/11/16
 * *****************************************************************************
 */

#include "analysis/csrc/utils/time_utils.h"

#include <ctime>
#include <iomanip>

#include "analysis/csrc/dfx/log.h"
#include "analysis/csrc/utils/utils.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"

namespace Analysis {
namespace Utils {

using namespace Analysis::Viewer::Database;
const int YEAR_DISPLAY_WIDTH = 4; // 年的固定宽度是4
const int TIME_DISPLAY_WIDTH = 2; // 日期的固定宽度是2，不足的前面补零
    
std::string GetFormatLocalTime()
{
    time_t t = time(nullptr);  // 获取1970年1月1日0点0分0秒到现在经过的秒数
    struct tm localTm;
    struct tm *tm = localtime_r(&t, &localTm);

    std::stringstream sstr;

    // 将秒数转换为本地时间,年从1900算起,需要+1900,月为0-11,所以要+1
    sstr << std::setw(YEAR_DISPLAY_WIDTH) << std::setfill('0') << (tm->tm_year + 1900) <<
        std::setw(TIME_DISPLAY_WIDTH) << std::setfill('0') << (tm->tm_mon + 1) <<
        std::setw(TIME_DISPLAY_WIDTH) << std::setfill('0') << tm->tm_mday <<
        std::setw(TIME_DISPLAY_WIDTH) << std::setfill('0') << tm->tm_hour <<
        std::setw(TIME_DISPLAY_WIDTH) << std::setfill('0') << tm->tm_min <<
        std::setw(TIME_DISPLAY_WIDTH) << std::setfill('0') << tm->tm_sec;

    // 格式：20231129134509  年月日时分秒的拼接字符串
    return sstr.str();
}

double GetTimeFromSyscnt(uint64_t syscnt, const SyscntConversionParams &params)
{
    if (IsDoubleEqual(params.freq, DEFAULT_FREQ)) {
        // 当freq为默认值时,默认数据为 monotonic,直接返回即可
        return static_cast<double>(syscnt);
    }
    if (syscnt < params.sysCnt) {
        ERROR("The base syscnt's value % greater than the task syscnt's value % .", params.sysCnt, syscnt);
        return static_cast<double>(syscnt);
    }
    return static_cast<double>((syscnt - params.sysCnt) / params.freq * MILLI_SECOND + params.hostMonotonic);
}

double GetLocalTime(double timestamp, const ProfTimeRecord &record)
{
    return static_cast<double>(timestamp + (record.startTimeNs - record.baseTimeNs - TIME_BASE_OFFSET_NS));
}

}  // namespace Utils
}  // namespace Analysis
