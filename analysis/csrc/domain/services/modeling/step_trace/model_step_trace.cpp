/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : model_step_trace.cpp
 * Description        : 有限状态机封装类
 * Author             : msprof team
 * Creation Date      : 2024/5/17
 * *****************************************************************************
 */

#include "analysis/csrc/domain/services/modeling/step_trace/model_step_trace.h"

namespace Analysis {
namespace Domain {

bool IsModelStart(int tag)
{
    if (tag == MODEL_START_TAG) {
        return true;
    }
    return false;
}

bool IsInnerProcess(int tag)
{
    if (tag == FP_TAG || tag == BP_TAG) {
        return true;
    } else if (tag >= ALL_REDUCE_START && tag < GET_NEXT_START_TAG) {
        return true;
    } else if (tag >= GET_NEXT_START_TAG) {
        return true;
    }
    return false;
}

bool IsStepEnd(int tag)
{
    if (tag == ITER_END_TAG) {
        return true;
    }
    return false;
}

bool IsModelEnd(int tag)
{
    if (tag == MODEL_END_TAG) {
        return true;
    }
    return false;
}

EventLabel TagToEvent(int tag)
{
    if (IsModelStart(tag)) {
        return EventLabel::ModelStart;
    } else if (IsInnerProcess(tag)) {
        return EventLabel::InnerProcess;
    } else if (IsStepEnd(tag)) {
        return EventLabel::StepEnd;
    } else if (IsModelEnd(tag)) {
        return EventLabel::ModelEnd;
    }
    return EventLabel::InvalidEvent;
}

void ModelStepTrace::Init()
{
    fsm_.Init();
}

void ModelStepTrace::OnStep(const HalTrackData& step, std::vector<StepTraceTasks>& baseSteps)
{
    EventLabel event = TagToEvent(step.stepTrace.tagId);
    fsm_.OnEvent(event, step, baseSteps);
}

}
}