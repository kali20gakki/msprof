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

#ifndef ANALYSIS_DOMAIN_SERVICES_MODELING_STEP_TRACE_STEP_TRACE_CONSTANT_H
#define ANALYSIS_DOMAIN_SERVICES_MODELING_STEP_TRACE_STEP_TRACE_CONSTANT_H

#include <cstdint>

namespace Analysis {
namespace Domain {
// 原始step trace数据相关tag标签
const uint16_t MODEL_START_TAG = 0;
const uint16_t MODEL_END_TAG = 1;
const uint16_t FP_TAG = 2;
const uint16_t BP_TAG = 3;
const uint16_t ITER_END_TAG = 4;
const uint16_t MSTX_TAG = 11;
const uint16_t ALL_REDUCE_START = 10000;
const uint16_t GET_NEXT_START_TAG = 20000;
const uint16_t STEP_START_TAG = 60000;

// 状态机label标签
enum class StepLabel {
    ModelStartLabel = 0,
    ModelEndLabel,
    TrainingTraceLabel,
    GetNextLabel,
    AllReduceLabel,
    MstxLabel,
    InvalidLabel = 5
};

// 状态机状态标签
enum class EventLabel {
    ModelStart = 0,
    InnerProcess,
    StepEnd,
    ModelEnd,
    InvalidEvent
};

}
}
#endif // ANALYSIS_DOMAIN_SERVICES_MODELING_STEP_TRACE_STEP_TRACE_CONSTANT_H
