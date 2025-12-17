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

#ifndef MSPROF_ANALYSIS_HAL_H
#define MSPROF_ANALYSIS_HAL_H

#include <cstdint>
#include "analysis/csrc/domain/valueobject/include/task_id.h"

namespace Analysis {
namespace Domain {
#define INVALID_TIMESTAMP (-1)

struct HalUniData {
    TaskId taskId;
    uint64_t timestamp = 0; // 用于生成batchId，和flip比较，填任务开始时
};

enum AcceleratorType {
    AIC = 0,
    AIV,
    MIX_AIC,
    MIX_AIV,
    INVALID = 4
};
}
}

#endif // MSPROF_ANALYSIS_HAL_H
