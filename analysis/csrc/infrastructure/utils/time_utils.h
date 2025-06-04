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
#include "analysis/csrc/infrastructure/utils/hp_float.h"

namespace Analysis {
namespace Utils {
const double DEFAULT_FREQ = 1000.0;
// log中数据单位为US,换算成NS时需要*1000,直接使用UINT64_MAX存在溢出等情况,需要向下取整
const uint64_t DEFAULT_END_TIME_NS = UINT64_MAX / 1000 * 1000;

// SyscntConversionParams 用于记录 syscnt 转为 timestamp 所需要的三个数据
// 分为host侧和device的syscnt两种情况，不同情况下，所需要的数据来源不同
// freq: 计算使用的频率, host使用 CPU的Frequency; device使用DeviceInfo的hwts_frequency
// hostFreq: host中CPU的Frequency;
// sysCnt: 基于开机时间的syscnt, host使用 host_start_log; device 使用 dev_start_log 中的cntvct
// hostCnt: 无论是host还是device默认都从host_start_log中取
// hostMonotonic: 基于开机时间的monotonic, 不论host还是device，固定使用host侧下的host_start_log 中的 clock_monotonic_raw
struct SyscntConversionParams {
    double freq = DEFAULT_FREQ;
    double hostFreq = DEFAULT_FREQ;
    uint64_t sysCnt = UINT64_MAX;
    uint64_t hostCnt = UINT64_MAX;
    uint64_t hostMonotonic = UINT64_MAX;
    SyscntConversionParams() = default;
    SyscntConversionParams(double freq, uint64_t sysCnt, uint64_t hostMonotonic)
        : freq(freq), sysCnt(sysCnt), hostMonotonic(hostMonotonic){};
};

struct ProfTimeRecord {
    uint64_t startTimeNs = UINT64_MAX;                                  // host start info 基于世界时间的任务开始时间
    uint64_t endTimeNs = 0;                                             // host end info 基于世界时间的任务结束时间
    uint64_t baseTimeNs = UINT64_MAX;                                   // host start info 基于开机时间的任务开始时间
    ProfTimeRecord() = default;
    ProfTimeRecord(uint64_t startTimeNs, uint64_t endTimeNs, uint64_t baseTimeNs)
        : startTimeNs(startTimeNs), endTimeNs(endTimeNs), baseTimeNs(baseTimeNs){};
};

// 转换成本地时间所需要的上下文参数
struct LocaltimeContext {
    uint16_t deviceId = UINT16_MAX;
    uint64_t hostMonotonic = UINT64_MAX;
    uint64_t deviceMonotonic = UINT64_MAX;
    Utils::ProfTimeRecord timeRecord;
};

std::string GetFormatLocalTime();
// 将syscnt转为timestamp 返回结果为ns级
HPFloat GetTimeFromSyscnt(uint64_t syscnt, const SyscntConversionParams &params);
// 将syscnt转为timestamp 返回结果为ns级,从host侧获取
HPFloat GetTimeFromHostCnt(uint64_t syscnt, const SyscntConversionParams &params);
// 将以syscnt计算的duration转换成timestamp 返回结果为ns级
HPFloat GetDurTimeFromSyscnt(uint64_t syscnt, const SyscntConversionParams &params);
// 将采样时间戳转为host侧时间戳
HPFloat GetTimeBySamplingTimestamp(double timestamp, const uint64_t hostMonotonic, const uint64_t deviceMonotonic);
// 计算加上本地的偏移时间, ns级
HPFloat GetLocalTime(HPFloat &timestamp, const ProfTimeRecord &record);

}  // namespace Utils
}  // namespace Analysis

#endif // ANALYSIS_UTILS_TIME_UTILS_H
