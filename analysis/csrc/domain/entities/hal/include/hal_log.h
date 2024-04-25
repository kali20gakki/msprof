/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : hal_log.h
 * Description        : log抽象数据结构
 * Author             : msprof team
 * Creation Date      : 2024/4/22
 * *****************************************************************************
 */

#ifndef MSPROF_ANALYSIS_HAL_LOG_H
#define MSPROF_ANALYSIS_HAL_LOG_H

#include "analysis/csrc/domain/entities/hal/include/hal.h"

namespace Analysis {
namespace Domain {

enum HalLogType {
    ACSQ_LOG = 0,
    FFTS_LOG,
    INVALID_LOG = 2
};

struct HalAcsqLog {
    bool isEndTimestamp = true;
    uint64_t timestamp = 0;
    uint16_t taskType = 0;
};

struct HalFftsPlusLog {
    bool isEndTimestamp = true;
    uint64_t timestamp = 0;
    uint16_t subTaskType = 0;
    uint16_t fftsType = 0;
    uint16_t threadId = 0;
};

struct HalLogData {
    HalUniData hd;

    HalLogType type;

    union {
        HalAcsqLog acsq;
        HalFftsPlusLog ffts;
    };
};
}
}

#endif // MSPROF_ANALYSIS_HAL_LOG_H
