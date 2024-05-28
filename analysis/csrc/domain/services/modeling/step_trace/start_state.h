/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : start_state.h
 * Description        : 开始状态
 * Author             : msprof team
 * Creation Date      : 2024/5/17
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_SERVICES_MODELING_STEP_TRACE_START_STATE_H
#define ANALYSIS_DOMAIN_SERVICES_MODELING_STEP_TRACE_START_STATE_H

#include "analysis/csrc/domain/services/modeling/step_trace/state.h"
#include "analysis/csrc/utils/singleton.h"

namespace Analysis {
namespace Domain {
class StartState : public State, public Utils::Singleton<StartState> {
    State& ModelStartEvent(uint32_t& indexId, const HalTrackData& step,
                           std::vector<StepTraceTasks>& baseSteps) override;
};

}
}

#endif // ANALYSIS_DOMAIN_SERVICES_MODELING_STEP_TRACE_START_STATE_H
