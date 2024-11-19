/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : msprof_tx_assembler.h
 * Description        : 组合msprof_tx层数据
 * Author             : msprof team
 * Creation Date      : 2024/8/30
 * *****************************************************************************
 */

#ifndef ANALYSIS_APPLICATION_DEVICE_TX_ASSEMBLER_H
#define ANALYSIS_APPLICATION_DEVICE_TX_ASSEMBLER_H

#include "analysis/csrc/application/timeline/json_assembler.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/ascend_task_data.h"

namespace Analysis {
namespace Application {
class DeviceTxAssembler : public JsonAssembler {
public:
    DeviceTxAssembler();
private:
    uint8_t AssembleData(DataInventory& dataInventory, JsonWriter &ostream, const std::string &profPath) override;
    void GenerateDeviceTxTrace(const std::vector<MsprofTxDeviceData> &txData, const std::string &profPath,
                               const LayerInfo &layer, std::unordered_map<uint16_t, uint32_t> &pidMap);
    void GenerateTxConnectionTrace(const MsprofTxDeviceData &data, uint32_t formatPid);
private:
    std::set<std::pair<uint32_t, int>> dPidTidSet_;
    std::unordered_map<uint64_t, std::string> h2dMessage_;
    std::vector<std::shared_ptr<TraceEvent>> res_;
};

class DeviceTxTraceEvent : public DurationEvent {
public:
    DeviceTxTraceEvent(uint32_t pid, int tid, double dur, const std::string &ts, const std::string &name,
                       uint32_t streamId, uint32_t taskId)
        : DurationEvent(pid, tid, dur, ts, name), streamId_(streamId), taskId_(taskId) {}
private:
    void ProcessArgs(JsonWriter &ostream) override;
private:
    uint32_t streamId_;
    uint32_t taskId_;
};
}
}
#endif // ANALYSIS_APPLICATION_DEVICE_TX_ASSEMBLER_H
