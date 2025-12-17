/* -------------------------------------------------------------------------
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/

#ifndef MSPROF_ANALYSIS_HAL_LOG_H
#define MSPROF_ANALYSIS_HAL_LOG_H

#include "analysis/csrc/domain/entities/hal/include/hal.h"

namespace Analysis {
namespace Domain {

enum HalLogType {
    ACSQ_LOG = 0,
    FFTS_LOG,
    ACC_PMU,
    INVALID_LOG = 3
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

struct HalAccPmu {
    uint64_t timestamp = 0;
    uint16_t accId = 0;
    uint64_t bandwidth[2] = {0};
    uint64_t ost[2] = {0};
};

struct HalLogData {
    HalUniData hd;

    HalLogType type;

    union {
        HalAcsqLog acsq;
        HalFftsPlusLog ffts;
        HalAccPmu accPmu;
    };
    HalLogData() {}
};
}
}

#endif // MSPROF_ANALYSIS_HAL_LOG_H
