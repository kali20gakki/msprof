/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : pmu_accelerator_util.h
 * Description        : pmu二进制数据判断加速器类型
 * Author             : msprof team
 * Creation Date      : 2024/4/22
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_SERVICE_PARSER_PARSER_ITEM_PMU_ACCELERATOR_UTIL_H
#define ANALYSIS_DOMAIN_SERVICE_PARSER_PARSER_ITEM_PMU_ACCELERATOR_UTIL_H

#include "analysis/csrc/domain/entities/hal/include/hal.h"

namespace Analysis {
namespace Domain {
const int FFTS_PLUS = 4;
const int TASK_AIC = 6;
const int TASK_AIV = 7;
const int NORMAL_TASK = 0;
AcceleratorType GetAcceleratorTypeByType(uint16_t subTaskType, uint16_t fftsType);
}
}
#endif // ANALYSIS_DOMAIN_SERVICE_PARSER_PARSER_ITEM_PMU_ACCELERATOR_UTIL_H
