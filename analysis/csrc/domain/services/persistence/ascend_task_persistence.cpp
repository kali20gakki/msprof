/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : ascend_task_persistence.cpp
 * Description        : Ascend_task数据落盘
 * Author             : msprof team
 * Creation Date      : 2024/5/23
 * *****************************************************************************
 */

#include "analysis/csrc/domain/services/persistence/ascend_task_persistence.h"
#include "analysis/csrc/domain/entities/hal/include/top_down_task.h"
#include "analysis/csrc/dfx/error_code.h"
#include "analysis/csrc/domain/services/constant/default_value_constant.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"
#include "analysis/csrc/domain/services/association/calculator/hccl/include/hccl_calculator.h"
#include "analysis/csrc/domain/services/persistence/persistence_utils.h"

namespace Analysis {
namespace Domain {
using namespace Viewer::Database;
// model_id, index_id, stream_id, task_id, context_id, batch_id, start_time, duration, host_task_type,
// device_task_type, connection_id
using ProcessedDataFormat = std::vector<std::tuple<uint64_t, int32_t, uint32_t, uint16_t, uint32_t, uint32_t, double,
        double, std::string, std::string, int64_t>>;

ProcessedDataFormat GenerateAscendTaskData(std::vector<TopDownTask>& ascendTask)
{
    ProcessedDataFormat processedData;
    for (auto& task : ascendTask) {
        double start_time = 0.0;
        double duration = 0.0;
        if (IsDoubleEqual(task.startTime, INVALID_TIME)) {
            start_time = INVALID_TIME;
        } else {
            start_time = task.startTime;
        }
        if (IsDoubleEqual(task.endTime, INVALID_TIME)) {
            duration = INVALID_TIME;
        } else {
            duration = task.endTime - start_time;
        }
        processedData.emplace_back(task.modelId, task.indexId, task.streamId, task.taskId, task.contextId, task.batchId,
                                   start_time, duration, task.hostTaskType, task.deviceTaskType,
                                   task.connectionId);
    }
    return processedData;
}

uint32_t AscendTaskPersistence::ProcessEntry(DataInventory& dataInventory, const Context& context)
{
    const DeviceContext& deviceContext = static_cast<const DeviceContext&>(context);
    auto ascendTask = dataInventory.GetPtr<std::vector<TopDownTask>>();
    if (!ascendTask) {
        ERROR("ascend task data is null.");
        return ANALYSIS_ERROR;
    }
    if (ascendTask->empty()) {
        WARN("no ascendTask don't persistence");
        return ANALYSIS_OK;
    }
    DBInfo ascendTaskDB("ascend_task.db", "AscendTask");
    MAKE_SHARED0_RETURN_VALUE(ascendTaskDB.database, AscendTaskDB, ANALYSIS_ERROR);
    std::string dbPath = Utils::File::PathJoin({deviceContext.GetDeviceFilePath(), SQLITE, ascendTaskDB.dbName});
    INFO("Start to process %.", dbPath);
    MAKE_SHARED_RETURN_VALUE(ascendTaskDB.dbRunner, DBRunner, ANALYSIS_ERROR, dbPath);
    auto data = GenerateAscendTaskData(*ascendTask);
    auto res = SaveData(data, ascendTaskDB, dbPath);
    if (res) {
        INFO("Process % done!", ascendTaskDB.tableName);
        return ANALYSIS_OK;
    }
    ERROR("Save % data failed: %", dbPath);
    return ANALYSIS_ERROR;
}
REGISTER_PROCESS_SEQUENCE(AscendTaskPersistence, true, HcclCalculator);
REGISTER_PROCESS_DEPENDENT_DATA(AscendTaskPersistence, std::vector<TopDownTask>);
REGISTER_PROCESS_SUPPORT_CHIP(AscendTaskPersistence, CHIP_ID_ALL);
}
}