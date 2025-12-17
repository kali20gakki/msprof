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

#ifndef ANALYSIS_DOMAIN_SERVICES_MODELING_TRACK_INCLUDE_STEP_TRACE_PROCESS_H
#define ANALYSIS_DOMAIN_SERVICES_MODELING_TRACK_INCLUDE_STEP_TRACE_PROCESS_H

#include "analysis/csrc/domain/entities/hal/include/hal_track.h"
#include "analysis/csrc/domain/entities/step_trace/include/step_trace_tasks.h"
#include "analysis/csrc/infrastructure/process/include/process.h"

namespace Analysis {
namespace Domain {
// StepTraceProcess为对外公开类，用于注册流程,负责补全indexId整个流程
class StepTraceProcess : public Infra::Process {
private:
    uint32_t ProcessEntry(Infra::DataInventory &dataInventory, const Infra::Context &context) override;
    // 预处理原始step数据
    static std::vector<HalTrackData> PreprocessData(std::vector<HalTrackData>& data);
    // 将当前处理完成的数据存入状态机输出数据结构中
    void SaveStepTraceTask();
private:
    using StepTraceTaskMap = std::map<uint32_t, std::vector<StepTraceTasks>>;
    uint64_t currentModeId_{UINT64_MAX};
    std::vector<StepTraceTasks> currentStepTraceTask_; // 状态机当前处理step数据队列
    StepTraceTaskMap stepTraceTasks_; // 状态机输出数据
};

}
}

#endif // ANALYSIS_DOMAIN_SERVICES_MODELING_TRACK_INCLUDE_STEP_TRACE_PROCESS_H
