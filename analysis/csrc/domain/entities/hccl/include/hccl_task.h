/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : hccl_task.h
 * Description        : hccl算子和task结构体
 * Author             : msprof team
 * Creation Date      : 2024/5/29
 * *****************************************************************************
 */

#ifndef MSPROF_ANALYSIS_ORI_HCCL_TASK_H
#define MSPROF_ANALYSIS_ORI_HCCL_TASK_H

#include <cstdint>
#include <string>
namespace Analysis {
namespace Domain {
struct HcclOp {
    uint16_t deviceId;
    uint64_t modelId;
    int32_t indexId;
    uint32_t threadId;
    std::string opName;
    std::string taskType;
    std::string opType;
    uint64_t timestamp;
    uint64_t duration;
    std::string isDynamic;
    int64_t connectionId;
    int32_t relay;
    int32_t retry;
    std::string dataType;
    std::string algType;
    uint64_t count;
    std::string groupName;
};

struct HcclTask {
    uint64_t modelId;
    int32_t indexId;
    std::string name;
    std::string groupName;
    int32_t planeId;
    uint64_t timestamp;
    double duration;
    uint32_t streamId;
    uint16_t taskId;
    uint32_t contextId;
    uint16_t batchId;
    uint16_t deviceId;
    uint16_t isMaster;
    uint32_t localRank;
    uint32_t remoteRank;
    uint32_t threadId;
    std::string transportType;
    double size;
    std::string dataType;
    std::string linkType;
    std::string notifyId;
    std::string rdmaType;
};
}
}

#endif // MSPROF_ANALYSIS_ORI_HCCL_TASK_H