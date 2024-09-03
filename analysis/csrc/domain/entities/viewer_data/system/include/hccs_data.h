/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : hccs_data.h
 * Description        : hccs_processor处理hccs表相关数据
 * Author             : msprof team
 * Creation Date      : 2024/8/10
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_HCCS_FORMAT_DATA_H
#define ANALYSIS_DOMAIN_HCCS_FORMAT_DATA_H

#include <cstdint>
#include <string>

namespace Analysis {
namespace Domain {
struct HccsData {
    uint16_t deviceId = UINT16_MAX;
    uint64_t localTime = 0; // ns
    double txThroughput = 0; // MB/s
    double rxThroughput = 0; // MB/s
};

struct HccsSummaryData {
    uint16_t deviceId = UINT16_MAX;
    double txMaxThroughput = 0; // 最大带宽（MB/s）
    double txMinThroughput = 0; // 最小带宽（MB/s）
    double txAvgThroughput = 0; // 平均带宽（MB/s）
    double rxMaxThroughput = 0;
    double rxMinThroughput = 0;
    double rxAvgThroughput = 0;
};
}
}

#endif // ANALYSIS_DOMAIN_HCCS_FORMAT_DATA_H
