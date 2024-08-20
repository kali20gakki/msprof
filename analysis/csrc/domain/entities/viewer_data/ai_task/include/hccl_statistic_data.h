/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : hccl_statistic_data.h
 * Description        : hccl_statistic处理HcclOpReport表的格式化数据
 * Author             : msprof team
 * Creation Date      : 2024/8/17
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_HCCL_STATISTICS_DATA_H
#define ANALYSIS_DOMAIN_HCCL_STATISTICS_DATA_H

#include <limits>
#include <string>

namespace Analysis {
namespace Domain {
struct HcclStatisticData {
    std::string opType;
    std::string count;
    double totalTime = 0.0;
    double min = std::numeric_limits<double>::infinity();
    double max = 0.0;
    double avg = 0.0;
    double ratio = 0.0;
};
}
}

#endif // ANALYSIS_DOMAIN_HCCL_STATISTICS_DATA_H
