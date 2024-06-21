/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : hal_track.h
 * Description        : ts_track抽象数据结构
 * Author             : msprof team
 * Creation Date      : 2024/4/22
 * *****************************************************************************
 */

#ifndef MSPROF_ANALYSIS_HAL_TRACK_H
#define MSPROF_ANALYSIS_HAL_TRACK_H


#include <vector>
#include <unordered_map>
#include "analysis/csrc/domain/entities/hal/include/hal.h"

namespace Analysis {
namespace Domain {
// index_id, model_id, step_start, step_end, iter_id
using StepTraceDataVectorFormat = std::vector<std::tuple<uint32_t, uint64_t, uint64_t, uint64_t, uint64_t>>;

enum HalTrackType {
    TS_TASK_FLIP = 0,
    STEP_TRACE = 1,
    TS_TASK_TYPE = 2,
    TS_MEMCPY = 3,
    INVALID_TYPE = 4
};

struct HalTaskFlip {
    uint64_t timestamp = 0;
    uint16_t flipNum = 0;
};

struct HalStepTrace {
    uint64_t timestamp = 0;
    uint64_t indexId = 0;
    uint64_t modelId = 0;
    uint16_t tagId = 0;
};

struct HalTaskType {
    uint64_t taskType = 0;
    uint16_t taskStatus = 0;
};

struct HalTaskMemcpy {
    uint64_t taskStatus = 0;
};

struct HalTrackData {
    HalUniData hd;
    HalTrackType type{INVALID_TYPE};
    union {
        HalTaskFlip flip;
        HalStepTrace stepTrace;
        HalTaskType taskType;
        HalTaskMemcpy taskMemcpy;
    };
    HalTrackData() {};
};

std::unordered_map<uint16_t, std::vector<HalTrackData*>> GetFlipData(std::vector<HalTrackData>& trackData);

/**
 * 引用类型跟踪数据，获取指针存于数组中
 * @param trackData 数据数组
 * @param type 类型
 * @return 返回指定类型的数据的指针数组
 */
std::vector<HalTrackData> GetTrackDataByType(std::vector<HalTrackData>& trackData, HalTrackType type);
StepTraceDataVectorFormat GenerateStepTime(std::vector<HalTrackData>& trackDatas);

}
}
#endif // MSPROF_ANALYSIS_HAL_TRACK_H
