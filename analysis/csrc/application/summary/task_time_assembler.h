/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2025-2025
            Copyright, 2025, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : task_time_assembler.h
 * Description        : 组合task time数据
 * Author             : msprof team
 * Creation Date      : 2025/5/27
 * *****************************************************************************
 */

#ifndef ANALYSIS_APPLICATION_SUMMARY_TASK_TIME_ASSEMBLER_H
#define ANALYSIS_APPLICATION_SUMMARY_TASK_TIME_ASSEMBLER_H

#include <utility>
#include <string>
#include "analysis/csrc/application/summary/summary_assembler.h"
#include "analysis/csrc/infrastructure/data_inventory/include/data_inventory.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/task_info_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/ascend_task_data.h"


namespace Analysis {
namespace Application {
using namespace Analysis::Infra;
using namespace Analysis::Domain;


struct TaskTimeKey {
    // task time csv匹配时只需用到 stream_id, task_id, batch_id
    TaskTimeKey() = default;

    TaskTimeKey(uint16_t _streamId, uint16_t _batchId, uint16_t _taskId)
        : streamId(_streamId), batchId(_batchId), taskId(_taskId) {}

    uint16_t streamId = 0;
    uint16_t batchId = 0;
    uint16_t taskId = 0;

bool operator==(const TaskTimeKey &other) const
{
    // Compare all member variables; return true if they are equal, otherwise return false
    return taskId == other.taskId && streamId == other.streamId && batchId == other.batchId;
}

bool operator<(const TaskTimeKey &other) const
{
    // 按照taskId, streamId, batchId的顺序进行比较
    return std::tie(taskId, streamId, batchId) < std::tie(other.taskId, other.streamId, other.batchId);
}
};


class TaskTimeAssembler : public SummaryAssembler {
public:
    TaskTimeAssembler() = default;
    TaskTimeAssembler(const std::string &name, const std::string &profPath);

private:
    uint8_t AssembleData(DataInventory &dataInventory);
    void FormatTaskInfoData(const std::vector<TaskInfoData> &taskInfoData);
    void AssembleTaskTime(const std::vector<AscendTaskData> &ascendTaskData);
    void sortRes();

private:
    // pair.first is op_name, pair.second is task_type
    std::map<TaskTimeKey, std::pair<std::string, std::string>> formatedTaskInfo_;
};

}
}

#endif // ANALYSIS_APPLICATION_SUMMARY_TASK_TIME_ASSEMBLER_H
