/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : top_down_task.h
 * Description        : ascendTask抽象数据结构
 * Author             : msprof team
 * Creation Date      : 2024/4/23
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_ENTITIES_HAL_TOP_DOWN_TASK_H
#define ANALYSIS_DOMAIN_ENTITIES_HAL_TOP_DOWN_TASK_H

#include <cstdint>

struct TopDownTask {
    uint16_t taskId = 0;
    uint16_t batchId = 0;
    uint32_t streamId = 0;
    uint32_t contextId = 0;
    int32_t indexId = 0;
    uint32_t deviceTaskType = 0;
    uint64_t hostTaskType = 0;
    uint64_t modelId = 0;
    int64_t connectionId = 0;
    uint64_t startTime = 0;
    uint64_t duration = 0;

    TopDownTask() = default;
    TopDownTask(uint16_t taskId, uint16_t batchId, uint32_t streamId, uint32_t contextId, int32_t indexId,
                uint32_t deviceType, uint64_t hostType, uint64_t modelId, int64_t connectionId, uint64_t startTime,
                uint64_t duration)
        : taskId(taskId), batchId(batchId), streamId(streamId), contextId(contextId), indexId(indexId),
          deviceTaskType(deviceType), hostTaskType(hostType), modelId(modelId), connectionId(connectionId),
          startTime(startTime), duration(duration) {}
};
#endif // ANALYSIS_DOMAIN_ENTITIES_HAL_TOP_DOWN_TASK_H
