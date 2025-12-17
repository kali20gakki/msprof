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
