/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : op_summary_assembler.h
 * Description        : 组合op_summary数据
 * Author             : msprof team
 * Creation Date      : 2024/11/29
 * *****************************************************************************
 */

#ifndef ANALYSIS_APPLICATION_OP_SUMMARY_ASSEMBLE_H
#define ANALYSIS_APPLICATION_OP_SUMMARY_ASSEMBLE_H

#include <cstdint>
#include <map>
#include "analysis/csrc/application/summary/summary_assembler.h"
#include "analysis/csrc/infrastructure/data_inventory/include/data_inventory.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/task_info_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/ascend_task_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/communication_info_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/metric_summary.h"

namespace Analysis {
namespace Application {
using namespace Analysis::Infra;
using namespace Analysis::Domain;
class OpSummaryAssembler : public SummaryAssembler {
public:
    OpSummaryAssembler() = default;
    OpSummaryAssembler(const std::string &name, const std::string &profPath);
private:
    uint8_t AssembleData(DataInventory &dataInventory);
    void GenerateOpBody(std::vector<AscendTaskData> &taskData, std::shared_ptr<MetricSummary> &pmu);
    void GenerateHcclBody(std::vector<CommunicationOpData> &opData);
    void SplitDataByTaskId(std::vector<TaskInfoData> &taskInfo);
    std::vector<std::string> GenerateOneTaskRow(const TaskInfoData &computeTask, const AscendTaskData &task);
    void MergeTaskAndPmu(std::shared_ptr<MetricSummary> &pmu, std::vector<std::string> &row, const TaskId &id,
                         std::unordered_map<std::string, int> &indexTable);
    void AddCubeUsage(std::vector<std::string> &data, std::unordered_map<std::string, int> &indexTable);
    void CalculateWaitTime();
    void WriteToFile() override;
private:
    std::map<TaskId, TaskInfoData*> computeTask_;
};
}
}
#endif // ANALYSIS_APPLICATION_OP_SUMMARY_ASSEMBLE_H
