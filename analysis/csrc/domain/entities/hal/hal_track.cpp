/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : hal_track.cpp
 * Description        : ts_track数据分组
 * Author             : msprof team
 * Creation Date      : 2024/4/22
 * *****************************************************************************
 */

#include "analysis/csrc/domain/entities/hal/include/hal_track.h"
#include <algorithm>
#include <map>

namespace Analysis {
namespace Domain {
namespace {
const uint16_t STEP_START_TAG = 60000;
const uint16_t STEP_END_TAG = 60001;
const uint16_t PAIR = 2;
}

std::unordered_map<uint16_t, std::vector<HalTrackData*>> GetFlipData(std::vector<HalTrackData>& trackData)
{
    std::unordered_map<uint16_t, std::vector<HalTrackData*>> trackDataMap;
    for (auto& data : trackData) {
        if (data.type == TS_TASK_FLIP) {
            trackDataMap[data.hd.taskId.streamId].push_back(&data);
        }
    }
    return trackDataMap;
}

std::vector<HalTrackData> GetTrackDataByType(std::vector<HalTrackData>& trackData, HalTrackType type)
{
    std::vector<HalTrackData> trackDataRefer;
    for (auto& data : trackData) {
        if (data.type == type) {
            trackDataRefer.push_back(data);
        }
    }
    return trackDataRefer;
}

StepTraceDataVectorFormat GenerateStepTime(std::vector<HalTrackData>& halTraceTasks)
{
    StepTraceDataVectorFormat processedData;
    std::map<uint32_t, std::vector<HalTrackData>> stepTime;
    std::vector<HalTrackData> halTraceTasksBackup = halTraceTasks;
    std::sort(halTraceTasksBackup.begin(), halTraceTasksBackup.end(),
              [](const HalTrackData& t1, const HalTrackData& t2) {
        return t1.stepTrace.timestamp < t2.stepTrace.timestamp;
    });
    for (auto trackData : halTraceTasksBackup) {
        if (trackData.stepTrace.tagId == STEP_START_TAG || trackData.stepTrace.tagId == STEP_END_TAG) {
            stepTime[trackData.stepTrace.indexId].emplace_back(trackData);
        }
    }
    for (auto &it : stepTime) {
        if (it.second.size() != PAIR) {
            continue;
        }
        processedData.emplace_back(it.first, it.second[0].stepTrace.modelId, it.second[0].stepTrace.timestamp,
                                   it.second[1].stepTrace.timestamp, it.first);
    }
    return processedData;
}
}
}
