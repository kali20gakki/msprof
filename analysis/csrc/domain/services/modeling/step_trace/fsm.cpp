/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : fsm.h
 * Description        : 有限状态机
 * Author             : msprof team
 * Creation Date      : 2024/5/17
 * *****************************************************************************
 */

#include "analysis/csrc/domain/services/modeling/step_trace/fsm.h"
#include "analysis/csrc/dfx/log.h"

namespace Analysis {
namespace Domain {
void Fsm::Init()
{
    state_ = &StartState::GetInstance();
    indexId_ = 1;
}

void Fsm::OnEvent(EventLabel event, const HalTrackData& step, std::vector<StepTraceTasks>& baseSteps)
{
    if (state_ == nullptr) {
        ERROR("Unknow state: %", state_);
        return;
    }
    switch (event) {
        case EventLabel::ModelStart:
            state_ = &state_->ModelStartEvent(indexId_, step, baseSteps);
            break;
        case EventLabel::InnerProcess:
            state_ = &state_->InnerProcessEvent(indexId_, step, baseSteps);
            break;
        case EventLabel::StepEnd:
            state_ = &state_->StepEndEvent(indexId_, step, baseSteps);
            break;
        case EventLabel::ModelEnd:
            state_ = &state_->ModeLEndEvent(indexId_, step, baseSteps);
            break;
        default:
            return;
    }
}

}
}
