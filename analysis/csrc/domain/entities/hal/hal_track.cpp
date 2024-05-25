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

namespace Analysis {
namespace Domain {

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

std::vector<HalTrackData*> GetTrackDataByType(std::vector<HalTrackData>& trackData, HalTrackType type)
{
    std::vector<HalTrackData*> trackDataRefer;
    for (auto& data : trackData) {
        if (data.type == type) {
            trackDataRefer.push_back(&data);
        }
    }
    return trackDataRefer;
}
}
}
