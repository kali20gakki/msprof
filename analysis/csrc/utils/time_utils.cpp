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

#include "time_utils.h"

#include <ctime>
#include <sstream>

namespace Analysis {
namespace Utils {

std::string GetFormatLocalTime()
{
    time_t t = time(NULL);  // 获取1970年1月1日0点0分0秒到现在经过的秒数
    struct tm localTm;
    struct tm *tm = localtime_r(&t, &localTm);

    std::stringstream sstr;
    // 将秒数转换为本地时间,年从1900算起,需要+1900,月为0-11,所以要+1
    sstr << (tm->tm_year + 1900) << (tm->tm_mon + 1) << tm->tm_mday <<
            tm->tm_hour << tm->tm_min << tm->tm_sec;
    
    // 格式：20231129134509  年月日时分秒的拼接字符串
    return sstr.str();
}

}  // namespace Utils
}  // namespace Analysis
