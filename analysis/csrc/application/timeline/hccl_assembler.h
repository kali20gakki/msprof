/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : hccl_assembler.h
 * Description        : 组合HCCL层数据
 * Author             : msprof team
 * Creation Date      : 2024/8/27
 * *****************************************************************************
 */

#ifndef ANALYSIS_APPLICATION_HCCL_ASSEMBLER_H
#define ANALYSIS_APPLICATION_HCCL_ASSEMBLER_H

#include "analysis/csrc/application/timeline/json_assembler.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/communication_info_data.h"

namespace Analysis {
namespace Application {
class HcclAssembler : public JsonAssembler {
public:
    HcclAssembler();
private:
    uint8_t AssembleData(DataInventory& dataInventory, JsonWriter &ostream, const std::string &profPath) override;
    void GenerateHcclTaskTrace(const std::vector<CommunicationTaskData> &task, const std::string &profPath,
                               std::unordered_map<uint16_t, uint32_t> &pidMap);
    void GenerateHcclOpTrace(const std::vector<CommunicationOpData> &opData, const std::string &profPath,
                             std::unordered_map<uint16_t, uint32_t> &pidMap);
    void GenerateConnectionTrace(const CommunicationOpData &data, uint32_t formatPid, int tid);
    void GenerateMetaDataEvent(std::unordered_map<uint16_t, uint32_t> &pidMap);
private:
    std::vector<std::shared_ptr<TraceEvent>> res_;
    std::unordered_map<std::string, int> groupIndex_;
    std::set<std::pair<uint32_t, int>> pidTidSet_;
};

class HcclOpTraceEvent : public DurationEvent {
public:
    HcclOpTraceEvent(uint32_t pid, int tid, double dur, const std::string &ts, const std::string &name,
                     uint32_t modelId, uint32_t count, uint64_t connectionId, const std::string &dataType,
                     const std::string &algType)
         : DurationEvent(pid, tid, dur, ts, name), modelId_(modelId), count_(count), connectionId_(connectionId),
         dataType_(dataType), algType_(algType) {}
private:
    void ProcessArgs(JsonWriter &ostream) override;
private:
    uint32_t modelId_;
    uint32_t count_;
    uint64_t connectionId_;
    std::string dataType_;
    std::string algType_;
};

class HcclTaskTraceEvent : public DurationEvent {
public:
    HcclTaskTraceEvent(uint32_t pid, int tid, double dur, const std::string &ts, const std::string &name, uint32_t src,
                       uint32_t dst, uint32_t streamId, uint32_t taskId, uint32_t contextId, uint32_t modelId,
                       uint64_t size, double esDur, double bw, uint64_t notifyId, const std::string &tsType,
                       const std::string &taskType, const std::string &dataType, const std::string &linkType)
        : DurationEvent(pid, tid, dur, ts, name), srcRank_(src), dstRank_(dst), streamId_(streamId), taskId_(taskId),
        contextId_(contextId), modelId_(modelId), size_(size), esDur_(esDur), bandwidth_(bw), notifyId_(notifyId),
        transportType_(tsType), taskType(taskType), dataType_(dataType), linkType_(linkType) {}
private:
    void ProcessArgs(JsonWriter &ostream) override;
private:
    uint32_t srcRank_;
    uint32_t dstRank_;
    uint32_t streamId_;
    uint32_t taskId_;
    uint32_t contextId_;
    uint32_t modelId_;
    uint64_t size_;
    double esDur_;
    double bandwidth_;
    uint64_t notifyId_;
    std::string transportType_;
    std::string taskType;
    std::string dataType_;
    std::string linkType_;
};
}
}

#endif // ANALYSIS_APPLICATION_HCCL_ASSEMBLER_H
