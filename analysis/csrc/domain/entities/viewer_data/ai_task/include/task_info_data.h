/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : task_info_data.h
 * Description        : taskInfo表format后的数据格式
 * Author             : msprof team
 * Creation Date      : 2024/8/1
 * *****************************************************************************
 */
#ifndef ANALYSIS_DOMAIN_TASK_INFO_DATA_H
#define ANALYSIS_DOMAIN_TASK_INFO_DATA_H

#include <cstdint>
#include <string>

namespace Analysis {
namespace Domain {
struct TaskInfoData {
    uint16_t deviceId = UINT16_MAX;
    uint32_t modelId = UINT32_MAX;
    uint32_t streamId = UINT32_MAX;
    uint32_t taskId = UINT32_MAX;
    uint32_t blockDim = UINT32_MAX;
    uint32_t mixBlockDim = UINT32_MAX;
    uint32_t batchId = UINT32_MAX;
    uint32_t contextId = UINT32_MAX;
    std::string hashId;
    std::string opName;
    std::string taskType;
    std::string opType;
    std::string opFlag;
    std::string inputFormats;
    std::string inputDataTypes;
    std::string inputShapes;
    std::string outputFormats;
    std::string outputDataTypes;
    std::string outputShapes;
};
}
}
#endif // ANALYSIS_DOMAIN_TASK_INFO_DATA_H
