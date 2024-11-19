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

#ifndef ANALYSIS_APPLICATION_MSPROF_TX_ASSEMBLER_H
#define ANALYSIS_APPLICATION_MSPROF_TX_ASSEMBLER_H

#include "analysis/csrc/application/timeline/json_assembler.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/msprof_tx_host_data.h"

namespace Analysis {
namespace Application {
class MsprofTxAssembler : public JsonAssembler {
public:
    MsprofTxAssembler();
private:
    uint8_t AssembleData(DataInventory& dataInventory, JsonWriter &ostream, const std::string &profPath) override;
    void GenerateTxTrace(const std::vector<MsprofTxHostData> &txData, uint32_t pid);
    void GenerateTxExConnectionTrace(const MsprofTxHostData &data, uint32_t pid);
    void GenerateHMetaDataEvent(const LayerInfo &layer, uint32_t pid);

private:
    std::vector<std::shared_ptr<TraceEvent>> res_;
    std::set<std::pair<uint32_t, int>> hPidTidSet_;
};

class MsprofTxTraceEvent : public DurationEvent {
public:
    MsprofTxTraceEvent(int pid, int tid, double dur, const std::string &ts, const std::string &name, uint32_t category,
                       uint32_t payloadType, uint32_t messageType, uint64_t payloadValue, const std::string &eventType)
        : DurationEvent(pid, tid, dur, ts, name), category_(category), payloadType_(payloadType),
        messageType_(messageType), payloadValue_(payloadValue), eventType_(eventType) {}
private:
    void ProcessArgs(JsonWriter &ostream) override;
private:
    uint32_t category_;
    uint32_t payloadType_;
    uint32_t messageType_;
    uint64_t payloadValue_;
    std::string eventType_;
};

class MsprofTxExTraceEvent : public DurationEvent {
public:
    MsprofTxExTraceEvent(int pid, int tid, double dur, const std::string &ts, const std::string &name,
                         const std::string &eventType)
        : DurationEvent(pid, tid, dur, ts, name), eventType_(eventType) {}
private:
    void ProcessArgs(JsonWriter &ostream) override;
private:
    std::string eventType_;
};
}
}
#endif // ANALYSIS_APPLICATION_MSPROF_TX_ASSEMBLER_H
