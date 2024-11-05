/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : step_trace_assembler.h
 * Description        : 组合StepTrace层数据
 * Author             : msprof team
 * Creation Date      : 2024/8/31
 * *****************************************************************************
 */

#ifndef ANALYSIS_APPLICATION_STEP_TRACE_ASSEMBLER_H
#define ANALYSIS_APPLICATION_STEP_TRACE_ASSEMBLER_H

#include "analysis/csrc/application/timeline/json_assembler.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/step_trace_data.h"

namespace Analysis {
namespace Application {
class StepTraceAssembler : public JsonAssembler {
public:
    StepTraceAssembler();
private:
    void GenerateTrainTrace(const std::vector<TrainTraceData> &trainData, const std::string &profPath,
                            std::unordered_map<uint16_t, uint32_t> &pidMap, const LayerInfo &layer);
    void GenerateFpBpTrace(const TrainTraceData &data, uint32_t pid, int tid);
    void GenerateRefreshTrace(const TrainTraceData &data, uint32_t pid, int tid);
    void GenerateDataAugTrace(const TrainTraceData &data, uint32_t pid, int tid);
    void GenerateMetaData(std::unordered_map<uint16_t, uint32_t> &pidMap, const LayerInfo &layer);
    void GenerateReduceTrace(const std::vector<AllReduceData> &reduceData, const std::string &profPath,
                             std::unordered_map<uint16_t, uint32_t> &pidMap, const LayerInfo &layer);
    void GenerateNextTrace(const std::vector<GetNextData> &nextData, const std::string &profPath,
                           std::unordered_map<uint16_t, uint32_t> &pidMap, const LayerInfo &layer);
    uint8_t AssembleData(DataInventory& dataInventory, JsonWriter &ostream, const std::string &profPath) override;
private:
    std::vector<std::shared_ptr<TraceEvent>> res_;
    std::set<std::pair<uint32_t, int>> pidTidSet_;
};

class StepTraceEvent : public DurationEvent {
public:
    StepTraceEvent(uint32_t pid, int tid, double dur, const std::string &ts, const std::string &name,
                   const std::string &cat, uint32_t indexId, double iterTime, const std::string fpStart,
                   const std::string bpEnd, const std::string iterEnd)
        : DurationEvent(pid, tid, dur, ts, name, cat), indexId_(indexId), iterTime_(iterTime), fpStart_(fpStart),
        bpEnd_(bpEnd), iterEnd_(iterEnd) {}
private:
    void ProcessArgs(JsonWriter &ostream) override;
private:
    uint32_t indexId_;
    double iterTime_;
    std::string fpStart_;
    std::string bpEnd_;
    std::string iterEnd_;
};

class FpBpTraceEvent : public DurationEvent {
public:
    FpBpTraceEvent(uint32_t pid, int tid, double dur, const std::string &ts, const std::string &name,
                   const std::string &cat, uint32_t indexId, double fpBpTime, const std::string fpStart,
                   const std::string bpEnd)
        : DurationEvent(pid, tid, dur, ts, name, cat), indexId_(indexId), fpBpTime_(fpBpTime), fpStart_(fpStart),
        bpEnd_(bpEnd) {}
private:
    void ProcessArgs(JsonWriter &ostream) override;
private:
    uint32_t indexId_;
    double fpBpTime_;
    std::string fpStart_;
    std::string bpEnd_;
};

class RefreshTraceEvent : public DurationEvent {
public:
    RefreshTraceEvent(uint32_t pid, int tid, double dur, const std::string &ts, const std::string &name,
                      const std::string &cat, uint32_t indexId, double refresh, const std::string bpEnd,
                      const std::string iterEnd)
        : DurationEvent(pid, tid, dur, ts, name, cat), indexId_(indexId), refresh_(refresh), bpEnd_(bpEnd),
        iterEnd_(iterEnd) {}
private:
    void ProcessArgs(JsonWriter &ostream) override;
private:
    uint32_t indexId_;
    double refresh_;
    std::string bpEnd_;
    std::string iterEnd_;
};

class DataAug1TraceEvent : public TraceEvent {
public:
    DataAug1TraceEvent(uint32_t pid, int tid, const std::string &ts, const std::string &name, const std::string &cat,
                      const std::string &id, double arg)
        : TraceEvent(pid, tid, name), ts_(ts), cat_(cat), id_(id), arg_(arg) {}
private:
    void ToJson(JsonWriter &ostream) override;
private:
    std::string ts_;
    std::string cat_;
    std::string id_;
    std::string ph_ = "s";
    double arg_;
};

class DataAug0TraceEvent : public TraceEvent {
public:
    DataAug0TraceEvent(uint32_t pid, int tid, const std::string &ts, const std::string &name, const std::string &cat,
                       const std::string &id, uint32_t arg)
        : TraceEvent(pid, tid, name), ts_(ts), cat_(cat), id_(id), arg_(arg) {}
private:
    void ToJson(JsonWriter &ostream) override;
private:
    std::string ts_;
    std::string cat_;
    std::string id_;
    std::string ph_ = "t";
    uint32_t arg_;
};

class ReduceTraceEvent : public DurationEvent {
public:
    ReduceTraceEvent(uint32_t pid, int tid, double dur, const std::string &ts, const std::string &name,
                     const std::string &cat, uint32_t indexId, std::unordered_map<std::string, std::string> &args)
        : DurationEvent(pid, tid, dur, ts, name, cat), indexId_(indexId), args_(std::move(args)) {}
private:
    void ProcessArgs(JsonWriter &ostream) override;
private:
    uint32_t indexId_;
    std::unordered_map<std::string, std::string> args_;
};

class NextTraceEvent : public DurationEvent {
public:
    NextTraceEvent(uint32_t pid, int tid, double dur, const std::string &ts, const std::string &name,
                   const std::string &cat, const std::string &start, const std::string &end, double time)
        : DurationEvent(pid, tid, dur, ts, name, cat), start_(start), end_(end), time_(time) {}
private:
    void ProcessArgs(JsonWriter &ostream) override;
private:
    std::string start_;
    std::string end_;
    double time_;
};
}
}

#endif // ANALYSIS_APPLICATION_STEP_TRACE_ASSEMBLER_H
