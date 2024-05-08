/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : log_modeling.cpp
 * Description        : log modeling 处理流程类
 * Author             : msprof team
 * Creation Date      : 2024/4/12
 * *****************************************************************************
 */
#include "analysis/csrc/domain/services/modeling/include/log_modeling.h"
#include <algorithm>
#include <iostream>
#include "analysis/csrc/domain/services/parser/log/include/stars_soc_parser.h"
#include "analysis/csrc/infrastructure/process/include/process_register.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"
#include "analysis/csrc/dfx/error_code.h"
#include "analysis/csrc/domain/services/modeling/batch_id/batch_id.h"
#include "analysis/csrc/utils/time_logger.h"
#include "analysis/csrc/utils/utils.h"
#include "analysis/csrc/dfx/log.h"

namespace Analysis {

namespace Domain {
namespace {

constexpr uint32_t LOW_16BIT_MASK = 0xffff;

inline uint64_t MakeKey(uint64_t contextId, uint32_t taskId, uint16_t streamId)
{
    uint64_t key = (contextId << 32) | (taskId << 16) | streamId;
    return key;
}

void MergeStartAndEnd(const std::map<uint64_t, std::vector<HalLogData*>>& logStart,
                      std::map<uint64_t, std::vector<HalLogData*>>& logEnd,
                      std::map<TaskId, std::vector<DeviceTask>>& deviceTaskMap,
                      std::function<void(Domain::DeviceTask&, const HalLogData&, const HalLogData&)> mergeFunc)
{
    for (const auto& startPair : logStart) {
        auto it = logEnd.find(startPair.first);
        if (it == logEnd.end() || startPair.second.size() != it->second.size()) {
            uint64_t key = startPair.first;
            uint32_t contextId = key >> 32;
            uint16_t taskId = (key >> 16) & LOW_16BIT_MASK;
            uint16_t streamId = key & LOW_16BIT_MASK;
            ERROR("Start log size(%) not same as End log size(%); context:%, task:%, stream:%",
                  startPair.second.size(), it->second.size(), contextId, taskId, streamId);
            continue;
        }
        for (size_t i = 0; i < startPair.second.size(); ++i) {
            auto& vec = deviceTaskMap[startPair.second[i]->hd.taskId];
            vec.emplace_back();
            mergeFunc(vec.back(), *startPair.second[i], *it->second[i]);
        }
    }
}

}

std::map<uint16_t, std::vector<HalTrackData*>> LogModeling::SplitLogGroups(std::vector<HalLogData>& logData,
    std::shared_ptr<std::vector<HalTrackData>>& flipTrack)
{
    Utils::TimeLogger t{"LogModeling::SplitLogGroups "};
    std::map<uint16_t, std::vector<HalTrackData*>> flipGroups;
    for (auto& halLog : logData) {
        if (halLog.type == ACSQ_LOG) {
            if (halLog.acsq.isEndTimestamp) {
                acsqEnd_[halLog.hd.taskId.streamId].push_back(&halLog);
            } else {
                acsqStart_[halLog.hd.taskId.streamId].push_back(&halLog);
            }
        } else {
            if (halLog.ffts.isEndTimestamp) {
                fftsEnd_[halLog.hd.taskId.streamId].push_back(&halLog);
            } else {
                fftsStart_[halLog.hd.taskId.streamId].push_back(&halLog);
            }
        }
    }
    if (flipTrack) {
        for (auto &node: *flipTrack) {
            flipGroups[node.hd.taskId.streamId].push_back(&node);
        }
    }
    return flipGroups;
}

template<typename T, typename U>
size_t GetAllNodeSize(const std::map<T, std::vector<U>>& tasks)
{
    size_t total{};
    for (const auto& node : tasks) {
        total += node.second.size();
    }
    return total;
}

void LogModeling::OutputLogCounts(const std::vector<HalLogData>& logData) const
{
    INFO("HalLogData size:%", logData.size());
    INFO("acsqS stream:%, total:%", acsqStart_.size(), GetAllNodeSize(acsqStart_));
    INFO("acsqE stream:%, total:%", acsqEnd_.size(), GetAllNodeSize(acsqEnd_));
    INFO("fftsS stream:%, total:%", fftsStart_.size(), GetAllNodeSize(fftsStart_));
    INFO("fftsE stream:%, total:%", fftsEnd_.size(), GetAllNodeSize(fftsEnd_));
}

void LogModeling::AddToDeviceTask(std::map<uint16_t, std::vector<HalLogData*>>& startTask,
    std::map<uint16_t, std::vector<HalLogData*>>& endTask,
    std::map<uint16_t, std::vector<HalTrackData*>>& flipGroups,
    std::map<TaskId, std::vector<DeviceTask>>& deviceTaskMap,
    std::function<void(Domain::DeviceTask&, const HalLogData&, const HalLogData&)> mergeFunc)
{
    // 下面for循环中会:1. 给Start补Batch id;  2. 给Start和End按三元组分组;  3. 将分组后的Log写入DeviceTask
    for (auto& streamNode : startTask) {
        std::map<uint64_t, std::vector<HalLogData*>> acsqTaskS;
        std::map<uint64_t, std::vector<HalLogData*>> acsqTaskE;
        auto it = flipGroups.find(streamNode.first);
        if (it != flipGroups.end()) {
            std::sort(streamNode.second.begin(), streamNode.second.end(),
                      [](HalLogData* lhs, HalLogData* rhs) {return lhs->hd.timestamp < rhs->hd.timestamp;});
            std::sort(it->second.begin(), it->second.end(),
                      [](HalTrackData* lhs, HalTrackData* rhs) {return lhs->hd.timestamp < rhs->hd.timestamp;});
            ModelingComputeBatchIdBinary(Utils::ReinterpretConvert<HalUniData**>(streamNode.second.data()),
                                         streamNode.second.size(),
                                         Utils::ReinterpretConvert<HalUniData**>(it->second.data()), it->second.size());
        }

        uint16_t streamId = streamNode.first;
        for (auto& node : streamNode.second) {
            acsqTaskS[MakeKey(node->hd.taskId.contextId, node->hd.taskId.taskId, streamId)].push_back(node);
        }

        auto itE = endTask.find(streamId);
        if (itE == endTask.end()) {
            ERROR("start exist but end not exist, stream:%", streamId);
            continue;
        }
        for (auto& node : itE->second) {
            acsqTaskE[MakeKey(node->hd.taskId.contextId, node->hd.taskId.taskId, streamId)].push_back(node);
        }
        
        MergeStartAndEnd(acsqTaskS, acsqTaskE, deviceTaskMap, mergeFunc);
    }
}

uint32_t LogModeling::ProcessEntry(Infra::DataInventory& dataInventory, const Infra::Context&)
{
    auto logData = dataInventory.GetPtr<std::vector<HalLogData>>();
    auto flipTrack = dataInventory.GetPtr<std::vector<HalTrackData>>();
    auto deviceTaskMap = dataInventory.GetPtr<std::map<TaskId, std::vector<DeviceTask>>>();
    if (!logData || !deviceTaskMap) {
        ERROR("data null:logData:%, deviceTaskMap:%", logData != nullptr, deviceTaskMap != nullptr);
        return Analysis::ANALYSIS_ERROR;
    }

    // 为了后面补BatchId，按stream和开始结束的维度给log分组
    auto flipGroups = SplitLogGroups(*logData, flipTrack);
    OutputLogCounts(*logData);

    size_t deviceTaskNum = GetAllNodeSize(*deviceTaskMap);
    Utils::TimeLogger t{"LogModeling::AddToDeviceTask "};
    AddToDeviceTask(acsqStart_, acsqEnd_, flipGroups, *deviceTaskMap,
        [](Domain::DeviceTask& oneTask, const HalLogData& startLog, const HalLogData& endLog) {
            oneTask.taskType = startLog.acsq.taskType;
            oneTask.taskStart = startLog.acsq.timestamp;
            oneTask.taskDuration = endLog.acsq.timestamp - oneTask.taskStart;
    });
    size_t acsqMatchedCount = GetAllNodeSize(*deviceTaskMap) - deviceTaskNum;
    INFO("ACSQ matched count:%", acsqMatchedCount);

    AddToDeviceTask(fftsStart_, fftsEnd_, flipGroups, *deviceTaskMap,
        [](Domain::DeviceTask& oneTask, const HalLogData& startLog, const HalLogData& endLog) {
                oneTask.taskType = startLog.ffts.subTaskType;
                oneTask.taskStart = startLog.ffts.timestamp;
                oneTask.taskDuration = endLog.ffts.timestamp - oneTask.taskStart;
    });
    size_t fftsMatchedCount = GetAllNodeSize(*deviceTaskMap) - acsqMatchedCount - deviceTaskNum;
    INFO("FFTS matched count:%", fftsMatchedCount);

    return Analysis::ANALYSIS_OK;
}

}


REGISTER_PROCESS_SEQUENCE(Domain::LogModeling, true, Domain::StarsSocParser);
REGISTER_PROCESS_DEPENDENT_DATA(Domain::LogModeling, std::vector<Domain::HalLogData>,
    std::vector<Domain::HalTrackData>, std::map<Domain::TaskId, std::vector<Domain::DeviceTask>>);
REGISTER_PROCESS_SUPPORT_CHIP(Domain::LogModeling, CHIP_V4_1_0);

}