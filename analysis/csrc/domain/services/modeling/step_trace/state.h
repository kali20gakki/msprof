/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : state.h
 * Description        : 有限状态机各状态
 * Author             : msprof team
 * Creation Date      : 2024/5/17
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_SERVICES_MODELING_STEP_TRACE_STATE_H
#define ANALYSIS_DOMAIN_SERVICES_MODELING_STEP_TRACE_STATE_H

#include "analysis/csrc/domain/entities/hal/include/hal_track.h"
#include "analysis/csrc/domain/entities/step_trace/include/step_trace_tasks.h"

namespace Analysis {
namespace Domain {
// State状态机状态父类，定义状态机不同状态，分别有startState，PreEndState和EndState子类
class State {
public:
    virtual State& ModelStartEvent(uint32_t& indexId, const HalTrackData& step, std::vector<StepTraceTasks>& baseSteps);
    State& InnerProcessEvent(uint32_t& indexId, const HalTrackData& step, std::vector<StepTraceTasks>& baseSteps);
    virtual State& StepEndEvent(uint32_t& indexId, const HalTrackData& step, std::vector<StepTraceTasks>& baseSteps);
    virtual State& ModeLEndEvent(uint32_t& indexId, const HalTrackData& step, std::vector<StepTraceTasks>& baseSteps);
};

}
}
#endif // ANALYSIS_DOMAIN_SERVICES_MODELING_STEP_TRACE_STATE_H
