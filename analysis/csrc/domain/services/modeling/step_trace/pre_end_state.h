/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : pre_end_state.h
 * Description        : 开始状态
 * Author             : msprof team
 * Creation Date      : 2024/5/17
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_SERVICES_MODELING_STEP_TRACE_PRE_END_STATE_H
#define ANALYSIS_DOMAIN_SERVICES_MODELING_STEP_TRACE_PRE_END_STATE_H

#include "analysis/csrc/domain/services/modeling/step_trace/state.h"
#include "analysis/csrc/utils/singleton.h"

namespace Analysis {
namespace Domain {
class PreEndState : public State, public Utils::Singleton<PreEndState> {
    State& ModelStartEvent(uint32_t& indexId, const HalTrackData& step,
                           std::vector<StepTraceTasks>& baseSteps) override;
    State& StepEndEvent(uint32_t& indexId, const HalTrackData& step, std::vector<StepTraceTasks>& baseSteps) override;
};

}
}

#endif // ANALYSIS_DOMAIN_SERVICES_MODELING_STEP_TRACE_PRE_END_STATE_H
