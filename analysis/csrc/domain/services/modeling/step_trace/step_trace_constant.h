/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : step_trace_constant.h
 * Description        : 有限状态机涉及的常量
 * Author             : msprof team
 * Creation Date      : 2024/5/20
 * *****************************************************************************
 */

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
