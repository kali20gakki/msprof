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

#ifndef ANALYSIS_DOMAIN_SERVICES_MODELING_STEP_TRACE_STATE_H
#define ANALYSIS_DOMAIN_SERVICES_MODELING_STEP_TRACE_STATE_H

#include "analysis/csrc/domain/entities/hal/include/hal_track.h"
#include "analysis/csrc/domain/entities/step_trace/include/step_trace_tasks.h"
#include "analysis/csrc/infrastructure/utils/singleton.h"

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

class StartState : public State, public Utils::Singleton<StartState> {
    State& ModelStartEvent(uint32_t& indexId, const HalTrackData& step,
                           std::vector<StepTraceTasks>& baseSteps) override;
};

class PreEndState : public State, public Utils::Singleton<PreEndState> {
    State& ModelStartEvent(uint32_t& indexId, const HalTrackData& step,
                           std::vector<StepTraceTasks>& baseSteps) override;
    State& StepEndEvent(uint32_t& indexId, const HalTrackData& step, std::vector<StepTraceTasks>& baseSteps) override;
};

class EndState : public State, public Utils::Singleton<EndState> {
    State& StepEndEvent(uint32_t& indexId, const HalTrackData& step, std::vector<StepTraceTasks>& baseSteps) override;
    State& ModeLEndEvent(uint32_t& indexId, const HalTrackData& step, std::vector<StepTraceTasks>& baseSteps) override;
};

}
}
#endif // ANALYSIS_DOMAIN_SERVICES_MODELING_STEP_TRACE_STATE_H
