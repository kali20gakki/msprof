/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : acc_pmu_data.h
 * Description        : 处理acc_pmu表后的格式化数据
 * Author             : msprof team
 * Creation Date      : 2024/8/12
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_ACC_PMU_DATA_H
#define ANALYSIS_DOMAIN_ACC_PMU_DATA_H

#include <cstdint>

namespace Analysis {
namespace Domain {
struct AccPmuData {
    uint16_t deviceId = UINT16_MAX;
    uint16_t accId = UINT16_MAX;
    uint32_t readBwLevel = UINT32_MAX;
    uint32_t writeBwLevel = UINT32_MAX;
    uint32_t readOstLevel = UINT32_MAX;
    uint32_t writeOstLevel = UINT32_MAX;
    uint64_t timestampNs = UINT64_MAX;
};
}
}
#endif // ANALYSIS_DOMAIN_ACC_PMU_DATA_H
