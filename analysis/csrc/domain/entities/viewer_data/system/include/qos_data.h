/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : qos_data.h
 * Description        : qos_processor处理QosBwData表相关数据
 * Author             : msprof team
 * Creation Date      : 2024/8/28
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_QOS_FORMAT_DATA_H
#define ANALYSIS_DOMAIN_QOS_FORMAT_DATA_H

#include <string>
#include "analysis/csrc/domain/entities/viewer_data/basic_data.h"

namespace Analysis {
namespace Domain {
struct QosData : public BasicData {
    uint16_t deviceId = UINT16_MAX;
    uint32_t bw1 = 0; // MB/s
    uint32_t bw2 = 0;
    uint32_t bw3 = 0;
    uint32_t bw4 = 0;
    uint32_t bw5 = 0;
    uint32_t bw6 = 0;
    uint32_t bw7 = 0;
    uint32_t bw8 = 0;
    uint32_t bw9 = 0;
    uint32_t bw10 = 0;
};
}
}

#endif // ANALYSIS_DOMAIN_QOS_FORMAT_DATA_H
