/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : host_usage_data.h
 * Description        : CpuUsage、DiskUsage、MemUsage、NetworkUsage、Syscall表处理后数据
 * Author             : msprof team
 * Creation Date      : 2024/8/20
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_HOST_USAGE_DATA_H
#define ANALYSIS_DOMAIN_HOST_USAGE_DATA_H

#include <cstdint>
#include <string>

struct CpuUsageData {
    uint64_t start = UINT64_MAX;
    double usage = 0.0;
    std::string cpuNo;
};

struct MemUsageData {
    uint64_t start = UINT64_MAX;
    double usage = 0.0;
};

struct DiskUsageData {
    uint64_t start = UINT64_MAX;
    double usage = 0.0;
};

struct NetWorkUsageData {
    uint64_t start = UINT64_MAX;
    double usage = 0.0;
};

#endif // ANALYSIS_DOMAIN_HOST_USAGE_DATA_H
