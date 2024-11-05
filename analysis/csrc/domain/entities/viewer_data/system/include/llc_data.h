/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : llc_data.h
 * Description        : hbm_processor处理hbm表相关数据
 * Author             : msprof team
 * Creation Date      : 2024/8/20
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_LLC_FORMAT_DATA_H
#define ANALYSIS_DOMAIN_LLC_FORMAT_DATA_H

#include <cstdint>
#include <string>

namespace Analysis {
namespace Domain {
struct LLcData {
    uint16_t deviceId = UINT16_MAX;
    uint8_t llcID = UINT8_MAX; // 原始数据对应的格式是I，对应uint32,但是实际来看uint8完全够了，节省内存
    uint64_t localTime = 0;
    double hitRate = 0.0; // %,中间db里面是小数，在这里统一变成百分数，比如50就是代表50%
    double throughput = 0; // MB/s
    std::string mode; // read和write,从json文件里面读
};

struct LLcSummaryData {
    uint16_t deviceId = UINT16_MAX;
    uint8_t llcId = UINT8_MAX; // llcId为UINT8_MAX，表示Average,否则表示任务ID
    double hitRate = 0;
    double throughput = 0;
    std::string mode;
};
}
}

#endif // ANALYSIS_DOMAIN_LLC_FORMAT_DATA_H
