/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : chip_id.h
 * Description        : 芯片类型资源头文件
 * Author             : msprof team
 * Creation Date      : 2024/4/12
 * *****************************************************************************
 */
#ifndef ANALYSIS_INFRASTRUCTURE_RESOURCE_CHIP_ID_H
#define ANALYSIS_INFRASTRUCTURE_RESOURCE_CHIP_ID_H

#include <cstdint>

namespace Analysis {

enum ChipId : uint32_t {
    CHIP_V1_1_0 = 0,
    CHIP_V2_1_0 = 1,
    CHIP_V3_1_0 = 2,
    CHIP_V3_2_0 = 3,
    CHIP_V3_3_0 = 4,
    CHIP_V4_1_0 = 5,

    CHIP_V1_1_1 = 7,
    CHIP_V1_1_2 = 8,
    CHIP_V5_1_0 = 9,

    CHIP_V1_1_3 = 11,

    CHIP_ID_ALL = UINT32_MAX  // 所有芯片均有此流程
};

}

#endif