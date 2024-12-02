/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : overlap_analysis_assembler.h
 * Description        : 组合计算、通信统计信息
 * Author             : msprof team
 * Creation Date      : 2024/8/30
 * *****************************************************************************
 */
#ifndef ANALYSIS_APPLICATION_OVERLAP_ANALYSIS_ASSEMBLER_H
#define ANALYSIS_APPLICATION_OVERLAP_ANALYSIS_ASSEMBLER_H

#include <map>
#include "analysis/csrc/application/timeline/json_assembler.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/ascend_task_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/task_info_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/communication_info_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/mc2_comm_info_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/kfc_turn_data.h"
#include "analysis/csrc/domain/valueobject/include/task_id.h"
#include "analysis/csrc/domain/entities/time/time_duration.h"

namespace Analysis {
namespace Application {
class OverlapAnalysisAssembler : public JsonAssembler {
public:
    OverlapAnalysisAssembler();
private:
    uint8_t AssembleData(DataInventory &dataInventory, JsonWriter &ostream, const std::string &profPath) override;
    void RecordCompAndCommTaskTime(const std::shared_ptr<std::vector<AscendTaskData>> &ascendTasks,
                                   const std::shared_ptr<std::vector<TaskInfoData>> &compTasks,
                                   const std::shared_ptr<std::vector<CommunicationOpData>> &commOps,
                                   const std::shared_ptr<std::vector<KfcOpData>> &kfcOps,
                                   const std::shared_ptr<std::vector<MC2CommInfoData>> &mc2CommInfos);
    void AssembleOneDevice(uint16_t deviceId, JsonWriter &ostream);
    static void SepCompTaskAndKFCCommSections(
        std::map<TaskId, std::vector<TimeDuration>> &allTaskPool,
        const std::shared_ptr<std::vector<TaskInfoData>> &compTasks,
        const std::shared_ptr<std::vector<MC2CommInfoData>> &mc2CommInfos,
        std::unordered_map<uint16_t, std::vector<TimeDuration>> &compSections);
    template<typename T>
    void GetCommTaskSections(std::unordered_map<uint16_t, std::vector<TimeDuration>> &commOpSections,
                             const std::shared_ptr<std::vector<T>> &commOps);
    std::vector<std::shared_ptr<TraceEvent>> GenerateComputeEvents(
        std::vector<TimeDuration> &compSections, uint16_t deviceId);
    std::vector<std::shared_ptr<TraceEvent>> GenerateCommEvents(
        std::vector<TimeDuration> &commSections, uint16_t deviceId);
    std::vector<std::shared_ptr<TraceEvent>> GenerateCommNotOverlapCompEvents(
        std::vector<TimeDuration> &compSections, std::vector<TimeDuration> &commSections, uint16_t deviceId);
    std::vector<std::shared_ptr<TraceEvent>> GenerateFreeEvents(
        std::vector<TimeDuration> &compSections, std::vector<TimeDuration> &commSections, uint16_t deviceId);
    std::vector<std::shared_ptr<TraceEvent>> GenerateMetaData(uint16_t deviceId);

    // 该函数要求vec以开始时间第一优先级，结束时间第二优先级的顺序排序
    static std::vector<TimeDuration> UnionOneSet(const std::vector<TimeDuration> &vecA);
    // 该函数要求两个集合均不重叠
    // 该函数要求vec以开始时间第一优先级，结束时间第二优先级的顺序排序
    static std::vector<TimeDuration> UnionTwoSet(const std::vector<TimeDuration> &vecA,
                                                 const std::vector<TimeDuration> &vecB);
    // 该函数要求两个集合均不重叠
    // 该函数要求vec以开始时间第一优先级，结束时间第二优先级的顺序排序
    static std::vector<TimeDuration> GetDifferenceSet(const std::vector<TimeDuration> &vecA,
                                                      const std::vector<TimeDuration> &vecB);

    static void ProcessSetAIsOnTheLeftOfSetB(
        const TimeDuration &durationA, uint64_t &lastRecordEnd, std::vector<TimeDuration> &res);
    static void ProcessLeftOfSetAIntersectRightOfSetBOrSetAContainsSetB(
        const TimeDuration &durationA, const TimeDuration &durationB,
        uint64_t &lastRecordEnd, std::vector<TimeDuration> &res);
private:
    std::unordered_map<uint16_t, std::vector<TimeDuration>> compTaskRecords_;
    std::unordered_map<uint16_t, std::vector<TimeDuration>> commTaskRecords_;
    std::unordered_map<uint16_t, uint64_t> begin_;
    std::unordered_map<uint16_t, uint64_t> end_;
    std::set<uint16_t> deviceIds_;
    std::unordered_map<uint16_t, uint32_t> pidMap_;
    std::string profPath_;
};
enum class OverlapType {
    COMMUNICATION = 0,
    COMM_NOT_OVERLAP_COMP,
    COMPUTE,
    FREE,
    RESERVE
};
class OverlapEvent : public DurationEvent {
public:
    OverlapEvent(int pid, int tid, double dur, const std::string &ts, const std::string &name, OverlapType type)
        : DurationEvent(pid, tid, dur, ts, name), type_(type)
    {}
private:
    OverlapType type_;
};
}
}
#endif // ANALYSIS_APPLICATION_OVERLAP_ANALYSIS_ASSEMBLER_H
