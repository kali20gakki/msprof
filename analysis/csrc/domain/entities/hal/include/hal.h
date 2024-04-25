/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : hal.h
 * Description        : task与pmu抽取公共结构
 * Author             : msprof team
 * Creation Date      : 2024/4/22
 * *****************************************************************************
 */

#ifndef MSPROF_ANALYSIS_HAL_H
#define MSPROF_ANALYSIS_HAL_H

#include <cstdint>
#include "analysis/csrc/domain/valueobject/include/task_id.h"

namespace Analysis {
namespace Domain {
#define INVALID_TIMESTAMP (-1)

struct HalUniData {
    TaskId taskId;
    uint64_t timestamp = 0; // 用于生成batchId，和flip比较，填任务开始时
};

enum AcceleratorType {
    AIC = 0,
    AIV,
    MIX_AIC,
    MIX_AIV,
    INVALID = 4
};
}
}

#endif // MSPROF_ANALYSIS_HAL_H
