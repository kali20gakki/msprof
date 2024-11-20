/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : ascend_hardware_assembler.h
 * Description        : 组合Ascend Hardware层数据
 * Author             : msprof team
 * Creation Date      : 2024/8/27
 * *****************************************************************************
 */

#ifndef ANALYSIS_APPLICATION_ASCEND_ASSEMBLER_H
#define ANALYSIS_APPLICATION_ASCEND_ASSEMBLER_H

#include <map>
#include "analysis/csrc/application/timeline/json_assembler.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/ascend_task_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/task_info_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/kfc_turn_data.h"
#include "analysis/csrc/domain/valueobject/include/task_id.h"

namespace Analysis {
namespace Application {

class TaskTraceEvent : public DurationEvent {
public:
    TaskTraceEvent(uint32_t pid, int tid, double dur, const std::string &ts, const std::string &name, uint32_t modelId,
                   uint32_t streamId, uint32_t taskId, uint32_t batchId, uint32_t contextId, uint64_t connectionId,
                   const std::string taskType)
        : DurationEvent(pid, tid, dur, ts, name), modelId_(modelId), streamId_(streamId), taskId_(taskId),
        batchId_(batchId), contextId_(contextId), connectionId_(connectionId), taskType_(taskType) {}
private:
    void ProcessArgs(JsonWriter &ostream) override;
private:
    uint32_t modelId_;
    uint32_t streamId_;
    uint32_t taskId_;
    uint32_t batchId_;
    uint32_t contextId_;
    uint64_t connectionId_;
    std::string taskType_;
};

class KfcTurnTraceEvent : public DurationEvent {
public:
    KfcTurnTraceEvent(int pid, int tid, double dur, const std::string &ts, const std::string &name, uint32_t streamId,
                      uint32_t taskId)
        : DurationEvent(pid, tid, dur, ts, name), streamId_(streamId), taskId_(taskId) {}
private:
    void ProcessArgs(JsonWriter &ostream) override;
private:
    uint32_t streamId_;
    uint32_t taskId_;
};

class AscendHardwareAssembler : public JsonAssembler {
public:
    AscendHardwareAssembler();
private:
    uint8_t AssembleData(DataInventory& dataInventory, JsonWriter &ostream, const std::string &profPath) override;
    void GenerateTaskTrace(const std::vector<AscendTaskData> &taskData, const std::string &profPath,
                           const LayerInfo &layer, std::unordered_map<uint16_t, uint32_t> &pidMap);
    void InitData(DataInventory &dataInventory, std::vector<AscendTaskData> &taskData);
    std::string GetOpName(const AscendTaskData& data);
    uint32_t GetPhysicStreamId(const uint32_t streamId);
    void GenerateTaskConnectionTrace(const AscendTaskData &data, uint32_t formatPid, TaskId &id);
    void GenerateKfcTrace(const std::vector<KfcTurnData>& kfcData, const std::string &profPath,
                          const LayerInfo &layer, std::unordered_map<uint16_t, uint32_t> &pidMap);
    std::string ConvertNs2UsWithDouble(uint64_t value);
private:
    std::vector<std::shared_ptr<TraceEvent>> res_;
    std::shared_ptr<std::unordered_map<uint32_t, uint32_t>> logicStream_;
    std::map<TaskId, std::string> opName_;
    std::set<std::pair<uint32_t, int>> pidTidSet_;
    std::set<TaskId> ffts_;
    std::set<uint64_t> recordEvent_;
};
}
}
#endif // ANALYSIS_APPLICATION_ASCEND_ASSEMBLER_H
