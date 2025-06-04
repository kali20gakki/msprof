/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2025-2025
            Copyright, 2025, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : op_statistic_data.h
 * Description        : op_statistic处理OpReport表的格式化数据
 * Author             : msprof team
 * Creation Date      : 2025/5/29
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_OP_STATISTICS_DATA_H
#define ANALYSIS_DOMAIN_OP_STATISTICS_DATA_H

#include <limits>
#include <string>

namespace Analysis {
namespace Domain {
struct OpStatisticData {
    std::string opType;
    std::string coreType;
    std::string count;
    uint16_t deviceId;
    double totalTime = 0.0;
    double min = std::numeric_limits<double>::infinity();
    double max = 0.0;
    double avg = 0.0;
    double ratio = 0.0;
};
}
}

#endif // ANALYSIS_DOMAIN_OP_STATISTICS_DATA_H
