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
#ifndef ANALYSIS_DOMAIN_SERVICES_MODELING_STEP_TRACE_FSM_H
#define ANALYSIS_DOMAIN_SERVICES_MODELING_STEP_TRACE_FSM_H

#include "analysis/csrc/domain/services/modeling/step_trace/state.h"
#include "analysis/csrc/domain/services/modeling/step_trace/step_trace_constant.h"

namespace Analysis {
namespace Domain {
// Fsm为状态机类，依据当前事件和状态机状态调用不同方法处理数据，其依赖State状态机状态类
class Fsm {
public:
    void Init();
    void OnEvent(EventLabel event, const HalTrackData& step, std::vector<StepTraceTasks>& baseSteps);
private:
    State* state_; // 状态机当前状态
    uint32_t indexId_; // 状态机当前indexId
};

}
}

#endif // ANALYSIS_DOMAIN_SERVICES_MODELING_STEP_TRACE_FSM_H
