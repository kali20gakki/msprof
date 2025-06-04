/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2025-2025
            Copyright, 2025, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : task_time_assembler.cpp
 * Description        : 组合task time数据
 * Author             : msprof team
 * Creation Date      : 2025/5/27
 * *****************************************************************************
 */

#include "analysis/csrc/application/summary/task_time_assembler.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/domain/services/device_context/device_context.h"

namespace Analysis {
namespace Application {
using namespace Analysis::Utils;
using namespace Analysis::Domain::Environment;
using namespace Analysis::Viewer::Database;


TaskTimeAssembler::TaskTimeAssembler(const std::string &name, const std::string &profPath)
    : SummaryAssembler(name, profPath)
{
    headers_ = {"Device_id", "kernel_name", "kernel_type", "stream_id", "task_id", "task_time(us)",
                "task_start(us)", "task_stop(us)"};
}

uint8_t TaskTimeAssembler::AssembleData(DataInventory &dataInventory)
{
    auto taskInfoData = dataInventory.GetPtr<std::vector<TaskInfoData>>();
    auto ascendTaskData = dataInventory.GetPtr<std::vector<AscendTaskData>>();
    if (ascendTaskData == nullptr) {
        WARN("AscendTask not exists, can't export task time data.");
        return DATA_NOT_EXIST;
    }

    if (taskInfoData == nullptr) {
        WARN("No ge data collected, maybe the TaskInfo table is not created, now try to export data with no ge data");
    } else {
        FormatTaskInfoData(*taskInfoData);
    }

    AssembleTaskTime(*ascendTaskData);

    if (res_.empty()) {
        ERROR("Can't match any task time data, failed to generate task_time_*.csv");
        return ASSEMBLE_FAILED;
    }
    // keep res in the order of device && start time
    sortRes();

    WriteToFile(File::PathJoin({profPath_, OUTPUT_PATH, TASK_TIME_SUMMARY_NAME}), {});

    return ASSEMBLE_SUCCESS;
}

void TaskTimeAssembler::FormatTaskInfoData(const std::vector<TaskInfoData> &taskInfoData)
{
    if (taskInfoData.empty()) {
        WARN("task info data is empty, no task info data for task time, check ge info please.");
        return;
    }
    for (const auto &taskInfoDatum : taskInfoData) {
        TaskTimeKey taskId{static_cast<uint16_t>(taskInfoDatum.streamId), static_cast<uint16_t>(taskInfoDatum.batchId),
                           static_cast<uint16_t>(taskInfoDatum.taskId)};
        formatedTaskInfo_.insert({taskId, {taskInfoDatum.opName, taskInfoDatum.taskType}});
    }
}

void TaskTimeAssembler::AssembleTaskTime(const std::vector<AscendTaskData> &ascendTaskData)
{
    if (ascendTaskData.empty()) {
        WARN("ascend task data is empty, no ascend task data for task time, check ascend task please.");
        return;
    }

    for (auto &ascendTaskDatum : ascendTaskData) {
        TaskTimeKey taskId{static_cast<uint16_t>(ascendTaskDatum.streamId),
                           static_cast<uint16_t>(ascendTaskDatum.batchId),
                           static_cast<uint16_t>(ascendTaskDatum.taskId)};
        auto it = formatedTaskInfo_.find(taskId);
        std::string opName;
        if (it != formatedTaskInfo_.end()) {
            opName = it->second.first;
        } else {
            opName = NA;
        }
        auto row = {std::to_string(ascendTaskDatum.deviceId),
                    opName, ascendTaskDatum.taskType, std::to_string(taskId.streamId),
                    std::to_string(taskId.taskId),
                    DivideByPowersOfTenWithPrecision(ascendTaskDatum.duration),
                    DivideByPowersOfTenWithPrecision(ascendTaskDatum.timestamp),
                    DivideByPowersOfTenWithPrecision(ascendTaskDatum.end)};
        // The original code contains a task start time filtering logic, of which is done in processor.
        res_.emplace_back(row);
    }
}

void TaskTimeAssembler::sortRes()
{
    // 业务可以保证headers_有值的情况下，device_id与start_time的下标是有效的
    const auto DEVICE_INDEX = 0;
    const auto START_TIME_INDEX = 6;
    using rowType = std::vector<std::string>;
    std::sort(res_.begin(), res_.end(), [DEVICE_INDEX, START_TIME_INDEX](const rowType& lv, const rowType& rv) {
        if (lv.at(DEVICE_INDEX) == rv.at(DEVICE_INDEX)) {
            return lv.at(START_TIME_INDEX) < rv.at(START_TIME_INDEX);
        }
        return lv.at(START_TIME_INDEX) < rv.at(START_TIME_INDEX);
    });
}

} // Application
} // Analysis