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

#include <cstdint>

namespace Analysis {
namespace Domain {
struct SocBandwidthData {
    uint16_t deviceId = UINT32_MAX;
    uint32_t l2_buffer_bw_level = 0;
    uint32_t mata_bw_level = 0;
    uint64_t timestamp = 0;
};
}
}
#endif // ANALYSIS_DOMAIN_SOC_DATA_H
