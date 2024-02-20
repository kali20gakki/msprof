/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : time_utils.h
 * Description        : 辅助函数
 * Author             : msprof team
 * Creation Date      : 2023/11/13
 * *****************************************************************************
 */

#ifndef ANALYSIS_UTILS_TIME_UTILS_H
#define ANALYSIS_UTILS_TIME_UTILS_H

#include <string>
#include "analysis/csrc/utils/hp_float.h"

namespace Analysis {
namespace Utils {
// 2020年1月1日0时0分0秒 作为基准时间 ns
const uint64_t TIME_BASE_OFFSET_NS = 1577808000000000000;
const double DEFAULT_FREQ = 1000.0;

// SyscntConversionParams 用于记录 syscnt 转为 timestamp 所需要的三个数据
// 分为host侧和device的syscnt两种情况，不同情况下，所需要的数据来源不同
// freq: 计算使用的频率, host使用 CPU的Frequency; device使用DeviceInfo的hwts_frequency
// sysCnt: 基于开机时间的syscnt, host使用 host_start_log; device 使用 dev_start_log 中的cntvct
// hostMonotonic: 基于开机时间的monotonic, 不论host还是device，固定使用host侧下的host_start_log 中的 clock_monotonic_raw
struct SyscntConversionParams {
    double freq = DEFAULT_FREQ;
    uint64_t sysCnt = UINT64_MAX;
    uint64_t hostMonotonic = UINT64_MAX;
    SyscntConversionParams() = default;
    SyscntConversionParams(double freq, uint64_t sysCnt, uint64_t hostMonotonic)
        : freq(freq), sysCnt(sysCnt), hostMonotonic(hostMonotonic){};
};

struct ProfTimeRecord {
    uint64_t startTimeNs = UINT64_MAX;                                  // host start info 基于世界时间的任务开始时间
    uint64_t endTimeNs = 0;                                             // host end info 基于世界时间的任务结束时间
    uint64_t baseTimeNs = startTimeNs - TIME_BASE_OFFSET_NS;            // host start info 基于开机时间的任务开始时间
    ProfTimeRecord() = default;
    ProfTimeRecord(uint64_t startTimeNs, uint64_t endTimeNs, uint64_t baseTimeNs)
        : startTimeNs(startTimeNs), endTimeNs(endTimeNs), baseTimeNs(baseTimeNs){};
};

std::string GetFormatLocalTime();
// 将syscnt转为timestamp 返回结果为ns级
HPFloat GetTimeFromSyscnt(uint64_t syscnt, const SyscntConversionParams &params);
// 将采样时间戳转为host侧时间戳
HPFloat GetTimeBySamplingTimestamp(double timestamp, const SyscntConversionParams &params);
// 计算加上本地的偏移时间（再减去基准时间）, us级
HPFloat GetLocalTime(HPFloat &timestamp, const ProfTimeRecord &record);

}  // namespace Utils
}  // namespace Analysis

#endif // ANALYSIS_UTILS_TIME_UTILS_H
