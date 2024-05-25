/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : device_task.h
 * Description        : deviceTask抽象数据结构
 * Author             : msprof team
 * Creation Date      : 2024/4/23
 * *****************************************************************************
 */

#ifndef MSPROF_ANALYSIS_DEVICE_TASK_H
#define MSPROF_ANALYSIS_DEVICE_TASK_H

#include <memory>
#include "analysis/csrc/domain/entities/hal/include/hal.h"
#include "analysis/csrc/domain/entities/pmu/include/pmu_info.h"

namespace Analysis {
namespace Domain {

struct DeviceTask {
    uint32_t taskType = 0;
    uint64_t taskStart = 0;
    uint64_t taskDuration = 0;
    uint32_t blockDim = 0;
    uint32_t mixBlockDim = 0;
    AcceleratorType acceleratorType = INVALID;
    std::unique_ptr<PmuBaseInfo> pmuInfo = nullptr;
};
}
}
#endif // MSPROF_ANALYSIS_DEVICE_TASK_H
