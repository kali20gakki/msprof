/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : step_trace_process.h
 * Description        : step trace modeling，用于补全indexId
 * Author             : msprof team
 * Creation Date      : 2024/5/9
 * *****************************************************************************
 */

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
    static std::vector<HalTrackData> PreprocessData(const std::shared_ptr<std::vector<HalTrackData>>& data);
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
