/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : kfc_turn_data.h
 * Description        : kfc_task_processor format后数据
 * Author             : msprof team
 * Creation Date      : 2024/8/10
 * *****************************************************************************
 */
#ifndef ANALYSIS_DOMAIN_KFC_TURN_DATA_H
#define ANALYSIS_DOMAIN_KFC_TURN_DATA_H

#include <cstdint>
#include <string>
#include <utility>

namespace Analysis {
namespace Domain {
struct KfcTurnData {
    std::string opName;
    uint16_t deviceId = UINT16_MAX;
    uint16_t streamId = UINT16_MAX;
    uint16_t taskId = UINT16_MAX;
    uint64_t startTime;
    double duration = 0.0;
public:
    KfcTurnData() = default;
    KfcTurnData(std::string _opName, uint16_t _deviceId, uint16_t _streamId, uint16_t _taskId, uint64_t _startTime,
                double _duration)
        : opName(std::move(_opName)), deviceId(_deviceId), streamId(_streamId), taskId(_taskId),
        startTime(_startTime), duration(_duration) {}
};

struct KfcTaskData {
    uint16_t deviceId;
    uint32_t modelId;
    std::string hcclName;
    uint64_t timestamp;
    double duration;
    uint64_t notifyId;
    double durationEstimated;
    uint32_t streamId = UINT32_MAX;
    uint32_t taskId = UINT32_MAX;
    uint32_t contextId = UINT32_MAX;
    std::string taskType;
    uint32_t srcRank = UINT32_MAX;
    uint32_t dstRank = UINT32_MAX;
    std::string transportType;
    uint64_t size = UINT64_MAX; // Byte
    std::string linkType;
    double bandwidth; // GB/S
    std::string groupName;
    int32_t planeId;
    std::string dataType;
    KfcTaskData() = default;
    KfcTaskData(uint16_t _deviceId, uint32_t _modelId, std::string _hcclName, uint64_t _timestamp, double _duration,
                uint64_t _notifyId, double _durationEstimated, uint32_t _streamId, uint32_t _taskId,
                uint32_t _contextId, std::string _taskType, uint32_t _srcRank, uint32_t _dstRank,
                std::string _transportType, uint64_t _size, std::string _linkType, double _bandwidth,
                std::string _groupName, int _planeId, std::string _dataType) :
                deviceId(_deviceId), modelId(_modelId), hcclName(_hcclName), timestamp(_timestamp), duration(_duration),
                notifyId(_notifyId), durationEstimated(_durationEstimated), streamId(_streamId), taskId(_taskId),
                contextId(_contextId), taskType(_taskType), srcRank(_srcRank), dstRank(_dstRank),
                transportType(_transportType), size(_size), linkType(_linkType), bandwidth(_bandwidth),
                groupName(_groupName), planeId(_planeId), dataType(_dataType) {};
};

struct KfcOpData {
    uint16_t deviceId;
    std::string opName;
    uint64_t timestamp;
    double duration;
    std::string groupName;
    uint64_t connectionId;
    uint32_t modelId;
    KfcOpData(uint16_t _deviceId, std::string _opName, uint64_t _timestamp, double _duration, std::string _groupName,
              uint64_t _connectionId, uint32_t _modelId)
        : deviceId(_deviceId), opName(_opName), timestamp(_timestamp), duration(_duration), groupName(_groupName),
        connectionId(_connectionId), modelId(_modelId) {};
};
}
}

#endif // ANALYSIS_DOMAIN_KFC_TURN_DATA_H
