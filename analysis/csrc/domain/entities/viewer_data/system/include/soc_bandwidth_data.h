/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : soc_bandwidth_data.h
 * Description        : soc_bandwidth_processor，处理InterSoc表后的格式化数据
 * Author             : msprof team
 * Creation Date      : 2024/08/20
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_SOC_DATA_H
#define ANALYSIS_DOMAIN_SOC_DATA_H

#include "analysis/csrc/domain/entities/viewer_data/basic_data.h"

namespace Analysis {
namespace Domain {
struct SocBandwidthData : public BasicData {
    uint16_t deviceId = UINT16_MAX;
    uint32_t l2BufferBwLevel = 0;
    uint32_t mataBwLevel = 0;
};
}
}
#endif // ANALYSIS_DOMAIN_SOC_DATA_H
