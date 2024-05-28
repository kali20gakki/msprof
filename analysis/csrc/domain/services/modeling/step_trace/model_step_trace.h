/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : model_step_trace.h
 * Description        : 有限状态机封装类
 * Author             : msprof team
 * Creation Date      : 2024/5/7
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_SERVICES_MODELING_STEP_TRACE_MODEL_STEP_TRACE_H
#define ANALYSIS_DOMAIN_SERVICES_MODELING_STEP_TRACE_MODEL_STEP_TRACE_H
#include <cstdint>
#include "analysis/csrc/domain/entities/step_trace/include/step_trace_tasks.h"
#include "analysis/csrc/domain/entities/hal/include/hal_track.h"
#include "analysis/csrc/domain/services/modeling/step_trace/fsm.h"

namespace Analysis {
namespace Domain {
// ModelStepTrace为内部状态机封装类，提供初始化和OnStep接口，前者用于modelId更新后重置状态机，后者是处理迭代数据接口，
// 负责将数据原始tag转化为状态机的事件，并送入状态机封装类处理
class ModelStepTrace {
public:
    void Init();
    void OnStep(const HalTrackData& step, std::vector<StepTraceTasks>& baseSteps);
private:
    Fsm fsm_;
};

}
}

#endif // ANALYSIS_DOMAIN_SERVICES_MODELING_STEP_TRACE_MODEL_STEP_TRACE_H
