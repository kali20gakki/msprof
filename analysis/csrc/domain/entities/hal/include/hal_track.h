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
#include <map>
#include "analysis/csrc/domain/entities/hal/include/hal.h"

namespace Analysis {
namespace Domain {

enum HalTrackType {
    TS_TASK_FLIP = 0,
    INVALID_TYPE = 1
};

struct HalTaskFlip {
    uint64_t timestamp = 0;
    uint16_t flipNum = 0;
};

struct HalTrackData {
    HalUniData hd;
    HalTrackType type{INVALID_TYPE};
    union {
        HalTaskFlip flip;
    };
    HalTrackData() {};
};

std::map<HalTrackType, std::vector<HalTrackData*>> ClassifyTrackData(std::vector<HalTrackData>& trackData);

/**
 * 引用类型跟踪数据，获取指针存于数组中
 * @param trackData 数据数组
 * @param type 类型
 * @return 返回指定类型的数据的指针数组
 */
std::vector<HalTrackData*> GetTrackDataByType(std::vector<HalTrackData>& trackData, HalTrackType type);

}
}
#endif // MSPROF_ANALYSIS_HAL_TRACK_H
