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
    std::string startTime;
    double duration = 0.0;
public:
    KfcTurnData() = default;
    KfcTurnData(std::string _opName, uint16_t _deviceId, uint16_t _streamId,
                uint16_t _taskId, std::string _startTime, double _duration)
    : opName(std::move(_opName)), deviceId(_deviceId), streamId(_streamId), taskId(_taskId),
    startTime(std::move(_startTime)), duration(_duration) {}
};
}
}

#endif // ANALYSIS_DOMAIN_KFC_TURN_DATA_H
