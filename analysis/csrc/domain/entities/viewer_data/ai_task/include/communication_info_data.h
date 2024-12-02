/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : communication_info_data.h
 * Description        : HCCLTaskSingleDevice,HCCLOpSingleDevice表相关数据数据格式化后的数据结构
 * Author             : msprof team
 * Creation Date      : 2024/08/24
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_COMMUNICATION_INFO_DATA_H
#define ANALYSIS_DOMAIN_COMMUNICATION_INFO_DATA_H

#include <cstdint>
#include <string>

namespace Analysis {
namespace Domain {
struct CommunicationTaskData {
    int32_t planeId = INT32_MAX;
    uint32_t modelId = UINT32_MAX;
    uint32_t streamId = UINT32_MAX;
    uint32_t taskId = UINT32_MAX;
    uint32_t contextId = UINT32_MAX;
    uint32_t batchId = UINT32_MAX;
    uint32_t srcRank = UINT32_MAX;
    uint32_t dstRank = UINT32_MAX;
    std::string opKey;
    uint16_t deviceId = UINT16_MAX;
    std::string opName;
    std::string taskType;
    std::string groupName;
    std::string notifyId;
    uint64_t transportType = UINT64_MAX;
    uint64_t size = UINT64_MAX;
    uint64_t dataType = UINT64_MAX;
    uint64_t linkType = UINT64_MAX;
    uint64_t rdmaType = UINT64_MAX;
    uint64_t start = UINT64_MAX;
    double duration = 0.0;
    double durationEstimated = 0.0;
    double bandwidth = 0.0;
    uint16_t isMaster = 0;
};
struct CommunicationOpData {
    std::string opName;
    std::string groupName;
    uint64_t connectionId = UINT64_MAX;
    std::string opKey;
    uint64_t start = UINT64_MAX;
    uint64_t end = UINT64_MAX;
    int32_t relay = 0;
    int32_t retry = 0;
    uint64_t dataType = UINT64_MAX;
    std::string algType;
    uint64_t count = UINT64_MAX;
    std::string opType;
    uint32_t modelId = 0;
    uint16_t deviceId = UINT16_MAX;
    uint16_t rankSize = 0;
};
}
}
#endif // ANALYSIS_DOMAIN_COMMUNICATION_INFO_DATA_H
