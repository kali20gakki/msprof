/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : task_id.h
 * Description        : task唯一ID抽取公共类
 * Author             : msprof team
 * Creation Date      : 2024/4/22
 * *****************************************************************************
 */

#ifndef MSPROF_ANALYSIS_TASK_ID_H
#define MSPROF_ANALYSIS_TASK_ID_H

#include <cstdint>
#include <tuple>

constexpr uint32_t INVALID_BATCH_ID = 0;
constexpr uint32_t INVALID_CONTEXT_ID = UINT32_MAX;

namespace Analysis {
namespace Domain {

struct TaskId {
    TaskId() = default;

    TaskId(uint16_t _streamId, uint16_t _batchId, uint16_t _taskId, uint32_t _contextId, uint16_t _deviceId = 0)
        : streamId(_streamId), batchId(_batchId), taskId(_taskId), contextId(_contextId), deviceId(_deviceId) {}

    uint16_t deviceId = 0;
    uint16_t streamId = 0;
    uint16_t batchId = 0;
    uint16_t taskId = 0;
    uint32_t contextId = INVALID_CONTEXT_ID;    // 有效值：0~65535，无效值：INVALID_CONTEXT_ID/UINT32_MAX/全f/-1

    bool operator==(const TaskId &other) const
    {
        // Compare all four member variables; return true if they are equal, otherwise return false
        return deviceId == other.deviceId && taskId == other.taskId && streamId == other.streamId &&
            contextId == other.contextId && batchId == other.batchId;
    }

    bool operator<(const TaskId &other) const
    {
        // 按照taskId,streamId,contextId,batchId的顺序进行比较
        return std::tie(deviceId, taskId, streamId, contextId, batchId) <
               std::tie(other.deviceId, other.taskId, other.streamId, other.contextId, other.batchId);
    }
};
}
}
#endif // MSPROF_ANALYSIS_TASK_ID_H
