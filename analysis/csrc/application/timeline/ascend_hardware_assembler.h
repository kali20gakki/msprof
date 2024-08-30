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
#include "analysis/csrc/domain/valueobject/include/task_id.h"

namespace Analysis {
namespace Application {
class AscendHardwareAssembler : public JsonAssembler {
public:
    AscendHardwareAssembler();
private:
    uint8_t AssembleData(DataInventory& dataInventory, JsonWriter &ostream, const std::string &profPath) override;
    void GenerateTaskTrace(const std::vector<AscendTaskData> &taskData, const std::string &profPath,
                           std::unordered_map<uint16_t, int> &pidMap);
    void GenerateTxTrace(const std::vector<MsprofTxDeviceData> &txData, const std::string &profPath,
                         std::unordered_map<uint16_t, int> &pidMap);
    void InitData(DataInventory &dataInventory);
    std::string GetOpName(const AscendTaskData& data);
    uint32_t GetPhysicStreamId(const AscendTaskData &data);
    void GenerateTaskConnectionTrace(const std::vector<AscendTaskData> &taskData,
                                     std::unordered_map<uint16_t, int> &pidMap);
    void GenerateTxConnectionTrace(const std::vector<MsprofTxDeviceData> &txData,
                                   std::unordered_map<uint16_t, int> &pidMap);
    void GenerateMetaData(std::unordered_map<uint16_t, int> &pidMap);
private:
    std::vector<std::shared_ptr<TraceEvent>> res_;
    std::shared_ptr<std::unordered_map<uint32_t, uint32_t>> logicStream_;
    std::map<TaskId, std::string> opName_;
    std::set<std::pair<int, int>> pidTidSet_;
};

class TaskTraceEvent : public DurationEvent {
public:
    TaskTraceEvent(int pid, int tid, double dur, const std::string &ts, const std::string &name, uint32_t modelId,
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

class DeviceTxTraceEvent : public DurationEvent {
public:
    DeviceTxTraceEvent(int pid, int tid, double dur, const std::string &ts, const std::string &name, uint32_t streamId,
                       uint32_t taskId)
        : DurationEvent(pid, tid, dur, ts, name), streamId_(streamId), taskId_(taskId) {}
private:
    void ProcessArgs(JsonWriter &ostream) override;
private:
    uint32_t streamId_;
    uint32_t taskId_;
};
}
}
#endif // ANALYSIS_APPLICATION_ASCEND_ASSEMBLER_H
