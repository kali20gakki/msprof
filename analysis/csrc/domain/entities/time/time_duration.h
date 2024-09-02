/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : time_duration.h
 * Description        : 定义时间片的类
 * Author             : MSPROF TEAM
 * Creation Date      : 2024/8/30
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_TIME_DURATION_H
#define ANALYSIS_DOMAIN_TIME_DURATION_H
namespace Analysis {
namespace Domain {
class TimeDuration {
public:
    TimeDuration(uint64_t start, uint64_t end) : start(start), end(end)
    {}

    bool operator<(const TimeDuration &other) const
    {
        return start < other.start || (start == other.start && end < other.end);
    }
public:
    uint64_t start{};
    uint64_t end{};
};
}
}
#endif // ANALYSIS_DOMAIN_TIME_DURATION_H
