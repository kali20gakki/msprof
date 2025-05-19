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

#include <string>
#include "analysis/csrc/domain/entities/viewer_data/basic_data.h"

namespace Analysis {
namespace Domain {
struct CpuUsageData : public BasicData {
    double usage = 0.0;
    std::string cpuNo;
};

struct MemUsageData : public BasicData {
    double usage = 0.0;
};

struct DiskUsageData : public BasicData {
    double usage = 0.0;
    double readRate = 0.0;
    double writeRate = 0.0;
};

struct NetWorkUsageData : public BasicData {
    double usage = 0.0;
    double speed = 0.0;
};

struct OSRuntimeApiData : public BasicData {
    std::string name;
    uint64_t pid = 0;
    uint64_t tid = 0;
    uint64_t endTime = 0;
};
}
}

#endif // ANALYSIS_DOMAIN_HOST_USAGE_DATA_H
