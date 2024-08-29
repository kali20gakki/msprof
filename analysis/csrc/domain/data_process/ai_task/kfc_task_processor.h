/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : task_processor.h
 * Description        : 处理kfcinfo表数据
 * Author             : msprof team
 * Creation Date      : 2024/8/26
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_KFC_TASK_PROCESSOR_H
#define ANALYSIS_DOMAIN_KFC_TASK_PROCESSOR_H

#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/kfc_turn_data.h"

namespace Analysis {
namespace Domain {
// "device_id", "stream_id", "task_id", "comm_turn", "current_turn", "wait_notify_start_time", "kfc_alg_exe_start_time",
// "send_task_start_time", "wait_active_start_time",
// "active_start_time", "wait_exe_end_start_time", "rtsq_exe_end_time"
using OriKfcCommData = std::vector<std::tuple<uint16_t, uint16_t, uint32_t, uint16_t, uint16_t, uint64_t, uint64_t,
uint64_t, uint64_t, uint64_t, uint64_t, uint64_t>>;
// "device_id", "stream_id", "task_id", "compute_turn", "current_turn", "wait_compute_start_time",
// "compute_start_time", "compute_exe_end_time"
using OriKfcComputeData = std::vector<std::tuple<uint16_t, uint16_t, uint32_t, uint16_t, uint16_t, uint64_t, uint64_t,
uint64_t>>;

struct KfcCommTurn {
    uint16_t deviceId = UINT16_MAX;
    uint16_t streamId = UINT16_MAX;
    uint32_t taskId = UINT32_MAX;
    uint16_t commTurn = UINT16_MAX;
    uint16_t currentTurn = UINT16_MAX;
    uint64_t waitNotifyStartTime = UINT64_MAX;
    uint64_t kfcAlgExeStartTime = UINT64_MAX;
    uint64_t sendTaskStartTime = UINT64_MAX;
    uint64_t waitActiveStartTime = UINT64_MAX;
    uint64_t activeStartTime = UINT64_MAX;
    uint64_t waitExeEndStartTime = UINT64_MAX;
    uint64_t rtsqExeEndTime = UINT64_MAX;
};

struct KfcComputeTurn {
    uint16_t deviceId = UINT16_MAX;
    uint16_t streamId = UINT16_MAX;
    uint32_t taskId = UINT32_MAX;
    uint16_t computeTurn = UINT16_MAX;
    uint16_t currentTurn = UINT16_MAX;
    uint64_t waitComputeStartTime = UINT64_MAX;
    uint64_t computeStartTime = UINT64_MAX;
    uint64_t computeExeEndTime = UINT64_MAX;
};

class KfcTaskProcessor : public DataProcessor {
public:
    KfcTaskProcessor() = default;
    explicit KfcTaskProcessor(const std::string &profPath);
private:
    bool Process(DataInventory& dataInventory) override;
    std::vector<KfcCommTurn> LoadCommData(const DBInfo &kfcDB, const std::string &dbPath);
    std::vector<KfcComputeTurn> LoadComputeData(const DBInfo &kfcDB, const std::string &dbPath);
    std::vector<KfcTurnData> FormatCommData(const std::vector<KfcCommTurn> &oriCommData, uint16_t deviceId);
    std::vector<KfcTurnData> FormatComputeData(const std::vector<KfcComputeTurn> &oriComputeData, uint16_t deviceId);
};
}
}

#endif // ANALYSIS_DOMAIN_KFC_TASK_PROCESSOR_H
