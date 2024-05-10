/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : pmu_accelerator_util.cpp
 * Description        : pmu二进制数据判断加速器类型
 * Author             : msprof team
 * Creation Date      : 2024/4/22
 * *****************************************************************************
 */

#include "analysis/csrc/domain/services/parser/pmu/pmu_accelerator_utils.h"

namespace Analysis {
namespace Domain {
AcceleratorType GetAcceleratorTypeByType(uint16_t subTaskType, uint16_t fftsType)
{
    if (fftsType == FFTS_PLUS) {
        if (subTaskType == TASK_AIC) {
            return AcceleratorType::MIX_AIC;
        } else if (subTaskType == TASK_AIV) {
            return AcceleratorType::MIX_AIV;
        }
    }
    if (fftsType == NORMAL_TASK || (fftsType == FFTS_PLUS && subTaskType == NORMAL_TASK)) {
        return AcceleratorType::AIC;
    }
    return AcceleratorType::AIV;
}
}
}