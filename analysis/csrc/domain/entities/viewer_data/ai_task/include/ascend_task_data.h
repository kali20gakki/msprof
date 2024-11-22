/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : ascendTask_data.h
 * Description        : ascendTask与msprofTx数据格式化后的数据结构
 * Author             : msprof team
 * Creation Date      : 2024/8/2
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_ASCENDTASK_DATA_H
#define ANALYSIS_DOMAIN_ASCENDTASK_DATA_H

#include <cstdint>
#include <string>

namespace Analysis {
namespace Domain {
const std::string TASK_TYPE_MSTX = "MsTx";
struct AscendTaskData {
    uint16_t deviceId = UINT16_MAX;
    uint32_t modelId = UINT32_MAX;
    int32_t indexId = INT32_MAX;
    uint32_t streamId = UINT32_MAX;
    uint32_t taskId = UINT32_MAX;
    uint32_t contextId = UINT32_MAX;
    uint32_t batchId = UINT32_MAX;
    uint64_t connectionId = UINT64_MAX;
    uint64_t start = UINT64_MAX;
    uint64_t end = UINT64_MAX;
    double duration = 0.0;
    std::string hostType;
    std::string deviceType;
    std::string taskType;
};

struct MsprofTxDeviceData {
    uint16_t deviceId = UINT16_MAX;
    uint32_t modelId = UINT32_MAX;
    uint32_t indexId = UINT32_MAX;
    uint32_t streamId = UINT32_MAX;
    uint32_t taskId = UINT32_MAX;
    uint32_t contextId = UINT32_MAX;
    uint32_t batchId = 0;
    uint64_t connectionId = UINT64_MAX;
    uint64_t start = UINT64_MAX;
    double duration = 0.0;
    std::string taskType = TASK_TYPE_MSTX;
};
}
}
#endif // ANALYSIS_DOMAIN_ASCENDTASK_DATA_H
