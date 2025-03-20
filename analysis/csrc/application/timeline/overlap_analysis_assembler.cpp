/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : overlap_analysis_assembler.cpp
 * Description        : 组合计算、通信统计信息
 * Author             : msprof team
 * Creation Date      : 2024/8/30
 * *****************************************************************************
 */
#include "analysis/csrc/application/timeline/overlap_analysis_assembler.h"
#include "analysis/csrc/infrastructure/utils/time_logger.h"
#include "analysis/csrc/domain/services/environment/context.h"

namespace Analysis {
namespace Application {
using namespace Analysis::Domain;
using namespace Analysis::Viewer::Database;
using namespace Analysis::Utils;
namespace {
const std::string COMP_NAME = "Computing";
const std::string COMM_NAME = "Communication";
const std::string COMM_NOT_OVERLAP_COMP_NAME = "Communication(Not Overlapped)";
const std::string FREE_NAME = "Free";
const std::vector<std::string> THREAD_ARGS_NAMES = {COMM_NAME, COMM_NOT_OVERLAP_COMP_NAME, COMP_NAME, FREE_NAME};
const std::vector<uint32_t> TIDS = {static_cast<uint32_t>(OverlapType::COMMUNICATION),
                                    static_cast<uint32_t>(OverlapType::COMM_NOT_OVERLAP_COMP),
                                    static_cast<uint32_t>(OverlapType::COMPUTE),
                                    static_cast<uint32_t>(OverlapType::FREE)};
const std::vector<uint32_t> THREAD_INDEXES = TIDS;

void SepOneTask(
    std::vector<TimeDuration> &times, std::set<uint16_t> &mc2StreamsTable,
    TaskInfoData &task, std::unordered_map<uint16_t, std::vector<TimeDuration>> &compSections)
{
    if (mc2StreamsTable.find(task.streamId) != mc2StreamsTable.end() || EndsWith(task.opName, AICPU_KERNEL)
        || EndsWith(task.opName, AIV_KERNEL)) {
        return;
    }
    for (auto &timeDur: times) {
        compSections[task.deviceId].emplace_back(timeDur);
    }
}
}
OverlapAnalysisAssembler::OverlapAnalysisAssembler()
    : JsonAssembler(PROCESS_OVERLAP_ANALYSE, {{MSPROF_JSON_FILE, FileCategory::MSPROF}})
{}

std::vector<std::shared_ptr<TraceEvent>> OverlapAnalysisAssembler::GenerateComputeEvents(
    std::vector<TimeDuration> &compSections, uint16_t deviceId)
{
    TimeLogger logger{"Generate comp events"};
    if (compSections.empty()) {
        WARN("No compute sections found for generate comp events.");
        return {};
    }
    std::vector<std::shared_ptr<TraceEvent>> computeEvents;
    for (auto &task:compSections) {
        std::shared_ptr<OverlapEvent> event;
        auto formatPid = pidMap_[deviceId];
        MAKE_SHARED_RETURN_VALUE(event, OverlapEvent, computeEvents, formatPid, static_cast<int>(OverlapType::COMPUTE),
                                 (task.end - task.start) / NS_TO_US,
                                 DivideByPowersOfTenWithPrecision(task.start), COMP_NAME,
                                 OverlapType::COMPUTE);
        computeEvents.emplace_back(event);
        begin_[deviceId] = std::min(begin_[deviceId], task.start);
        end_[deviceId] = std::max(end_[deviceId], task.end);
    }
    return computeEvents;
}
std::vector<std::shared_ptr<TraceEvent>> OverlapAnalysisAssembler::GenerateCommEvents(
    std::vector<TimeDuration> &commSections, uint16_t deviceId)
{
    TimeLogger logger{"Generate comm events"};
    if (commSections.empty()) {
        WARN("No comm sections found for generate comm events.");
        return {};
    }
    std::vector<std::shared_ptr<TraceEvent>> commEvents;
    for (auto &task : commSections) {
        std::shared_ptr<OverlapEvent> event;
        auto formatPid = pidMap_[deviceId];
        MAKE_SHARED_RETURN_VALUE(event, OverlapEvent, commEvents,
                                 formatPid, static_cast<int>(OverlapType::COMMUNICATION),
                                 (task.end - task.start) / NS_TO_US, DivideByPowersOfTenWithPrecision(task.start),
                                 COMM_NAME, OverlapType::COMMUNICATION);
        commEvents.emplace_back(event);
        begin_[deviceId] = std::min(begin_[deviceId], task.start);
        end_[deviceId] = std::max(end_[deviceId], task.end);
    }
    return commEvents;
}
std::vector<std::shared_ptr<TraceEvent>> OverlapAnalysisAssembler::GenerateCommNotOverlapCompEvents(
    std::vector<TimeDuration> &compSections, std::vector<TimeDuration> &commSections, uint16_t deviceId)
{
    TimeLogger logger{"Generate comm not overlap comp events"};
    if (commSections.empty()) {
        WARN("No comm sections found for generate comm not overlap comp events.");
        return {};
    }
    std::vector<std::shared_ptr<TraceEvent>> commNotOverlapCompEvents;
    auto diffRecords = GetDifferenceSet(commSections, compSections);
    auto formatPid = pidMap_[deviceId];
    for (auto &task : diffRecords) {
        std::shared_ptr<OverlapEvent> event;
        MAKE_SHARED_RETURN_VALUE(event, OverlapEvent, commNotOverlapCompEvents, formatPid,
                                 static_cast<int>(OverlapType::COMM_NOT_OVERLAP_COMP),
                                 (task.end - task.start) / NS_TO_US, DivideByPowersOfTenWithPrecision(task.start),
                                 COMM_NOT_OVERLAP_COMP_NAME,
                                 OverlapType::COMM_NOT_OVERLAP_COMP);
        commNotOverlapCompEvents.emplace_back(event);
    }
    return commNotOverlapCompEvents;
}
std::vector<std::shared_ptr<TraceEvent>> OverlapAnalysisAssembler::GenerateFreeEvents(
    std::vector<TimeDuration> &compSections, std::vector<TimeDuration> &commSections, uint16_t deviceId)
{
    TimeLogger logger{"Generate free events"};
    std::vector<std::shared_ptr<TraceEvent>> freeEvents;
    auto unionRecords = UnionTwoSet(compSections, commSections);
    // 前面业务逻辑可以保证一定有key为deviceId的value
    std::vector<TimeDuration> allTimeSection{{begin_[deviceId], end_[deviceId]}};
    auto freeRecords = GetDifferenceSet(allTimeSection, unionRecords);
    auto formatPid = pidMap_[deviceId];
    for (auto &task : freeRecords) {
        std::shared_ptr<OverlapEvent> event;
        MAKE_SHARED_RETURN_VALUE(event, OverlapEvent, freeEvents, formatPid, static_cast<int>(OverlapType::FREE),
                                 (task.end - task.start) / NS_TO_US,
                                 DivideByPowersOfTenWithPrecision(task.start), FREE_NAME,
                                 OverlapType::FREE);
        freeEvents.emplace_back(event);
    }
    return freeEvents;
}
void OverlapAnalysisAssembler::RecordCompAndCommTaskTime(
    const std::shared_ptr<std::vector<AscendTaskData>> &ascendTasks,
    const std::shared_ptr<std::vector<TaskInfoData>> &compTasks,
    const std::shared_ptr<std::vector<CommunicationOpData>> &commOps,
    const std::shared_ptr<std::vector<KfcOpData>> &kfcOps,
    const std::shared_ptr<std::vector<MC2CommInfoData>> &mc2CommInfos)
{
    // 覆盖单算子和图模式场景
    std::map<TaskId, std::vector<TimeDuration>> allTaskPool;
    if (ascendTasks) {
        for (auto &task : *ascendTasks) {
            TaskId id{static_cast<uint16_t >(task.streamId), static_cast<uint16_t >(task.batchId),
                      static_cast<uint16_t >(task.taskId), task.contextId, task.deviceId};
            TimeDuration timePair{task.timestamp, task.timestamp + static_cast<uint64_t>(task.duration)};
            if (allTaskPool.find(id) != allTaskPool.end()) {
                allTaskPool[id].emplace_back(timePair);
            } else {
                allTaskPool[id] = {timePair};
            }

            // 更新标记
            deviceIds_.insert(task.deviceId);
        }
    }

    std::unordered_map<uint16_t, std::vector<TimeDuration>> compSections;
    SepCompTaskAndKFCCommSections(
        allTaskPool, compTasks, mc2CommInfos, compSections);
    std::unordered_map<uint16_t, std::vector<TimeDuration>> tradCommSections;
    GetCommTaskSections(tradCommSections, commOps);
    GetCommTaskSections(tradCommSections, kfcOps);

    for (auto &pair : tradCommSections) {
        TimeLogger logger(
            "sort and union all trad comm task in overlap analysis for device " + std::to_string(pair.first));
        std::sort(pair.second.begin(), pair.second.end());
        commTaskRecords_[pair.first] = UnionOneSet(pair.second);
    }
    for (auto &pair : compSections) {
        TimeLogger logger("sort and union all comp task in overlap analysis for device " + std::to_string(pair.first));
        std::sort(pair.second.begin(), pair.second.end());
        compTaskRecords_[pair.first] = UnionOneSet(pair.second);
    }
}
std::vector<TimeDuration> OverlapAnalysisAssembler::UnionTwoSet(const std::vector<TimeDuration> &vecA,
                                                                const std::vector<TimeDuration> &vecB)
{
    std::vector<TimeDuration> vec;
    if (!Utils::Reserve(vec, vecA.size() + vecB.size())) {
        ERROR("Reserve vec error");
        return {};
    }
    vec.insert(vec.end(), vecA.begin(), vecA.end());
    vec.insert(vec.end(), vecB.begin(), vecB.end());
    std::sort(vec.begin(), vec.end());
    return UnionOneSet(vec);
}
std::vector<TimeDuration> OverlapAnalysisAssembler::GetDifferenceSet(const std::vector<TimeDuration> &vecA,
                                                                     const std::vector<TimeDuration> &vecB)
{
    if (vecA.empty() || vecB.empty()) {
        return vecA;
    }
    std::vector<TimeDuration> res;
    uint32_t i = 0;
    uint32_t j = 0;
    // 记录上一次存储后已经处理完毕的时间段的end节点
    uint64_t lastRecordEnd = vecA.front().start;
    while (i < vecA.size() && j < vecB.size()) {
        while (j < vecB.size() && vecB[j].end <= vecA[i].start) {
            // A在B右侧
            j++;
        }
        if (vecA[i].end <= vecB[j].start) {
            // A在B左侧
            ProcessSetAIsOnTheLeftOfSetB(vecA[i], lastRecordEnd, res);
            i++;
            continue;
        } else if (vecA[i].start >= vecB[j].start && vecA[i].end <= vecB[j].end) {
            // B包含A
            i++;
            continue;
        } else if ((vecA[i].start <= vecB[j].start && vecA[i].end >= vecB[j].end) ||
            (vecA[i].end >= vecB[j].start && vecA[i].start <= vecB[j].start)) {
            ProcessLeftOfSetAIntersectRightOfSetBOrSetAContainsSetB(vecA[i], vecB[j], lastRecordEnd, res);
            j++;
            continue;
        } else if (vecA[i].start >= vecB[j].start && vecA[i].start <= vecB[j].end) {
            // A左和B右相交
            lastRecordEnd = vecB[j].end;
            j++;
            continue;
        } else {
            // 异常情况，应该是伪分支
            ERROR("Illegal section situation, secA: (%, %), secB: (%, %)",
                  vecA[i].start, vecA[i].end, vecB[j].start, vecB[j].end);
            // 保证循环正常退出
            i++;
        }
    }
    while (i < vecA.size()) {
        uint64_t startRecord = std::max(vecA[i].start, lastRecordEnd);
        if (startRecord < vecA[i].end) {
            res.emplace_back(startRecord, vecA[i].end);
        }
        lastRecordEnd = vecA[i].end;
        i++;
    }
    return res;
}
void OverlapAnalysisAssembler::ProcessSetAIsOnTheLeftOfSetB(
    const TimeDuration &durationA, uint64_t &lastRecordEnd, std::vector<TimeDuration> &res)
{
    if (durationA.end >= lastRecordEnd) {
        // 补齐i中剩下的部分
        uint64_t RecordStart = std::max(lastRecordEnd, durationA.start);
        if (RecordStart < durationA.end) {
            res.emplace_back(RecordStart, durationA.end);
        }
        lastRecordEnd = durationA.end;
    }
}
void OverlapAnalysisAssembler::ProcessLeftOfSetAIntersectRightOfSetBOrSetAContainsSetB(
    const TimeDuration &durationA, const TimeDuration &durationB,
    uint64_t &lastRecordEnd, std::vector<TimeDuration> &res)
{
    // A包含B 或 A右和B左相交
    if (durationB.start >= lastRecordEnd) {
        // 此处覆盖两种情况
        // 1. 关系刚刚成立，此时lastRecordEnd小于vecA[i].start
        // 2. j已经不是第一个关系成立的B元素，此时lastRecordEnd大于vecA[i].start
        uint64_t RecordStart = std::max(lastRecordEnd, durationA.start);
        if (RecordStart < durationB.start) {
            res.emplace_back(RecordStart, durationB.start);
        }
        lastRecordEnd = durationB.end;
    }
}
std::vector<TimeDuration> OverlapAnalysisAssembler::UnionOneSet(const std::vector<TimeDuration> &vecA)
{
    if (vecA.empty()) {
        return {};
    }
    std::vector<TimeDuration> merged;
    for (auto &i : vecA) {
        uint64_t L = i.start, R = i.end;
        if (merged.empty() || merged.back().end < L) {
            merged.emplace_back(L, R);
        } else {
            merged.back().end = std::max(merged.back().end, R);
        }
    }
    return merged;
}
void OverlapAnalysisAssembler::SepCompTaskAndKFCCommSections(
    std::map<TaskId, std::vector<TimeDuration>> &allTaskPool,
    const std::shared_ptr<std::vector<TaskInfoData>> &compTasks,
    const std::shared_ptr<std::vector<MC2CommInfoData>> &mc2CommInfos,
    std::unordered_map<uint16_t, std::vector<TimeDuration>> &compSections)
{
    if (!compTasks) {
        return;
    }
    std::set<uint16_t> mc2StreamsTable;
    if (mc2CommInfos) {
        for (auto &mc2CommInfo : *mc2CommInfos) {
            mc2StreamsTable.insert(mc2CommInfo.aiCpuKfcStreamId);
        }
    }
    uint64_t mismatchCount = 0;
    std::set<TaskId> usedTaskIds;
    for (auto &task : *compTasks) {
        TaskId id{static_cast<uint16_t >(task.streamId), static_cast<uint16_t >(task.batchId),
                  static_cast<uint16_t >(task.taskId), task.contextId, task.deviceId};
        if (usedTaskIds.find(id) != usedTaskIds.end()) {
            // 避免重复处理，图模式触发
            continue;
        }
        usedTaskIds.insert(id);
        auto it = allTaskPool.find(id);
        if (it != allTaskPool.end()) {
            SepOneTask(it->second, mc2StreamsTable, task, compSections);
        } else {
            mismatchCount++;
        }
    }
    INFO("Find % comp tasks not in all tasks.", mismatchCount);
}

template<typename T>
void OverlapAnalysisAssembler::GetCommTaskSections(
    std::unordered_map<uint16_t, std::vector<TimeDuration>> &commOpSections,
    const std::shared_ptr<std::vector<T>> &commOps)
{
    if (!commOps) {
        return;
    }
    std::set<TaskId> usedTaskIds;
    for (auto &op : *commOps) {
        // 更新标记
        deviceIds_.insert(op.deviceId);
        commOpSections[op.deviceId].emplace_back(op.timestamp, op.end);
    }
}
std::vector<std::shared_ptr<TraceEvent>> OverlapAnalysisAssembler::GenerateMetaData(uint16_t deviceId)
{
    std::vector<std::shared_ptr<TraceEvent>> metaEvents;
    // pid描述
    auto layerInfo = GetLayerInfo(PROCESS_OVERLAP_ANALYSE);
    GenerateHWMetaData(pidMap_, layerInfo, metaEvents);
    // tid描述
    uint32_t formatPid = pidMap_[deviceId];
    for (size_t i = 0; i < THREAD_ARGS_NAMES.size(); i++) {
        std::shared_ptr<MetaDataNameEvent> threadName;
        MAKE_SHARED_RETURN_VALUE(
            threadName, MetaDataNameEvent, {}, formatPid, TIDS[i], META_DATA_THREAD_NAME, THREAD_ARGS_NAMES[i]);
        metaEvents.push_back(threadName);
        std::shared_ptr<MetaDataIndexEvent> threadIndex;
        MAKE_SHARED_RETURN_VALUE(threadIndex, MetaDataIndexEvent, {}, formatPid, TIDS[i], META_DATA_THREAD_INDEX,
                                 THREAD_INDEXES[i]);
        metaEvents.push_back(threadIndex);
    }
    return metaEvents;
}
uint8_t OverlapAnalysisAssembler::AssembleData(DataInventory &dataInventory,
                                               JsonWriter &ostream,
                                               const std::string &profPath)
{
    auto taskData = dataInventory.GetPtr<std::vector<AscendTaskData>>();
    if (!taskData) {
        WARN("No any task data found");
        return DATA_NOT_EXIST;
    }
    auto computeTaskData = dataInventory.GetPtr<std::vector<TaskInfoData>>();
    if (!computeTaskData) {
        WARN("No compute task data found");
    }
    auto commOpData = dataInventory.GetPtr<std::vector<CommunicationOpData>>();
    if (!commOpData) {
        WARN("No comm task data found");
    }
    auto kfcOpData = dataInventory.GetPtr<std::vector<KfcOpData>>();
    if (!kfcOpData) {
        WARN("No kfc op data found");
    }
    auto mc2CommInfoData = dataInventory.GetPtr<std::vector<MC2CommInfoData>>();
    if (!mc2CommInfoData) {
        WARN("No kfc comm info found");
    }

    profPath_ = profPath;

    RecordCompAndCommTaskTime(taskData, computeTaskData, commOpData, kfcOpData, mc2CommInfoData);
    // init pid map
    auto layerInfo = GetLayerInfo(PROCESS_OVERLAP_ANALYSE);
    for (auto &deviceId : deviceIds_) {
        auto pid = Analysis::Domain::Environment::Context::GetInstance().GetPidFromInfoJson(deviceId, profPath_);
        uint32_t formatPid = JsonAssembler::GetFormatPid(pid, layerInfo.sortIndex, deviceId);
        pidMap_[deviceId] = formatPid;
    }

    for (auto &deviceId : deviceIds_) {
        AssembleOneDevice(deviceId, ostream);
    }
    // 为了让下一个写入的内容形成正确的JSON格式，需要补一个","
    ostream << ",";
    return ASSEMBLE_SUCCESS;
}
void OverlapAnalysisAssembler::AssembleOneDevice(uint16_t deviceId, JsonWriter &ostream)
{
    auto metaEvens = GenerateMetaData(deviceId);
    std::vector<TimeDuration> compSections =
        compTaskRecords_.find(deviceId) == compTaskRecords_.end() ? std::vector<TimeDuration>()
                                                                  : compTaskRecords_[deviceId];
    std::vector<TimeDuration> commSections =
        commTaskRecords_.find(deviceId) == commTaskRecords_.end() ? std::vector<TimeDuration>()
                                                                  : commTaskRecords_[deviceId];

    begin_[deviceId] = UINT64_MAX;
    end_[deviceId] = 0;
    auto compEvents = GenerateComputeEvents(compSections, deviceId);
    auto commEvents = GenerateCommEvents(commSections, deviceId);
    auto commNotOverlapCompEvents = GenerateCommNotOverlapCompEvents(compSections, commSections, deviceId);
    auto freeEvents = GenerateFreeEvents(compSections, commSections, deviceId);

    {
        TimeLogger logger("Dump overlap analysis events");
        for (const auto &node : metaEvens) {
            node->DumpJson(ostream);
        }
        for (const auto &node : compEvents) {
            node->DumpJson(ostream);
        }
        for (const auto &node : commEvents) {
            node->DumpJson(ostream);
        }
        for (const auto &node : commNotOverlapCompEvents) {
            node->DumpJson(ostream);
        }
        for (const auto &node : freeEvents) {
            node->DumpJson(ostream);
        }
    }
}
}
}