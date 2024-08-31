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
const int NS_US = 3; // NS到US的时间转换的数量级，3表示10^3的指数
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

HPFloat GetTimeFromSyscnt(uint64_t syscnt, const SyscntConversionParams &params)
{
    if (IsDoubleEqual(params.freq, DEFAULT_FREQ)) {
        // 当freq为默认值时,默认数据为 monotonic,直接返回即可
        return {syscnt};
    }
    if (syscnt < params.sysCnt) {
        ERROR("The base syscnt's value % greater than the task syscnt's value % .", params.sysCnt, syscnt);
        return {syscnt};
    }
    HPFloat res = (syscnt - params.sysCnt) / params.freq;
    res = res << NS_US;
    res += HPFloat(params.hostMonotonic);
    return res;
}

HPFloat GetDurTimeFromSyscnt(uint64_t syscnt, const SyscntConversionParams &params)
{
    HPFloat res = syscnt / params.freq;
    res = res << NS_US;
    return res;
}

HPFloat GetTimeBySamplingTimestamp(double timestamp, const uint64_t hostMonotonic, const uint64_t deviceMonotonic)
{
    // 时间单位均为ns
    return HPFloat(timestamp) + HPFloat(hostMonotonic) - HPFloat(deviceMonotonic);
}

HPFloat GetLocalTime(HPFloat &timestamp, const ProfTimeRecord &record)
{
    HPFloat baseTime = record.startTimeNs - record.baseTimeNs;
    HPFloat res = timestamp + baseTime;
    return res;
}

}  // namespace Utils
}  // namespace Analysis
