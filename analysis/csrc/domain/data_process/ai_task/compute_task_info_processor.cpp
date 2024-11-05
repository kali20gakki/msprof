/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : compute_task_info_processor.h
 * Description        : compute_task_info processor，处理taskInfo表数据
 * Author             : msprof team
 * Creation Date      : 2024/8/1
 * *****************************************************************************
 */

#include "analysis/csrc/domain/data_process/ai_task/compute_task_info_processor.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"

namespace Analysis {
namespace Domain {
ComputeTaskInfoProcessor::ComputeTaskInfoProcessor(const std::string &profPath) : DataProcessor(profPath) {}

bool ComputeTaskInfoProcessor::Process(DataInventory &dataInventory)
{
    DBInfo geInfoDB("ge_info.db", "TaskInfo");
    std::string dbPath = Utils::File::PathJoin({profPath_, HOST, SQLITE, geInfoDB.dbName});
    if (!geInfoDB.ConstructDBRunner(dbPath)) {
        return false;
    }
    // 并不是所有场景都有ge info数据
    auto status = CheckPathAndTable(dbPath, geInfoDB);
    if (status == CHECK_FAILED) {
        return false;
    } else if (status == NOT_EXIST) {
        return true;
    }
    auto hashMap = dataInventory.GetPtr<std::unordered_map<std::string, std::string>>();
    if (hashMap == nullptr) {
        ERROR("Can't get hash data.");
        return false;
    }
    auto formatData = LoadData(geInfoDB, dbPath, *hashMap);
    if (formatData.empty()) {
        ERROR("TaskInfo format data is empty. DBPath is %", dbPath);
        return false;
    }
    if (!SaveToDataInventory<TaskInfoData>(std::move(formatData), dataInventory,
            PROCESSOR_NAME_COMPUTE_TASK_INFO)) {
        ERROR("Save data failed, %.", PROCESSOR_NAME_COMPUTE_TASK_INFO);
        return false;
    }
    return true;
}

std::vector<TaskInfoData> ComputeTaskInfoProcessor::LoadData(
    const DBInfo &taskInfoDB, const std::string &dbPath, std::unordered_map<std::string, std::string>& hashMap)
{
    std::vector<TaskInfoFormat> oriData;
    std::vector<TaskInfoData> res;
    if (taskInfoDB.dbRunner == nullptr) {
        ERROR("Create % connection failed.", dbPath);
        return res;
    }
    std::string sql{"SELECT hashid, model_id, op_name, stream_id, task_id, block_dim, mix_block_dim, task_type, "
                    "op_type, op_flag, batch_id, IFNULL(input_formats, 'NULL'), IFNULL(input_data_types, 'NULL'), "
                    "IFNULL(input_shapes, 'NULL'), IFNULL(output_formats, 'NULL'), IFNULL(output_data_types, 'NULL'), "
                    "IFNULL(output_shapes, 'NULL'), device_id, context_id FROM " + taskInfoDB.tableName};
    if (!taskInfoDB.dbRunner->QueryData(sql, oriData)) {
        ERROR("Failed to obtain data from the % table.", taskInfoDB.tableName);
    }
    if (oriData.empty()) {
        ERROR("TaskInfo original data is empty, DBPath is &", dbPath);
        return res;
    }
    if (!Utils::Reserve(res, oriData.size())) {
        ERROR("Reserve for TaskInfo data failed");
        return res;
    }
    TaskInfoData data;
    std::string hashId;
    for (auto& node : oriData) {
        std::tie(hashId, data.modelId, data.opName, data.streamId, data.taskId, data.blockDim, data.mixBlockDim,
                 data.taskType, data.opType, data.opFlag, data.batchId, data.inputFormats, data.inputDataTypes,
                 data.inputShapes, data.outputFormats, data.outputDataTypes, data.outputShapes,
                 data.deviceId, data.contextId) = node;
        if (hashMap.find(hashId) != hashMap.end()) {
            data.hashId = hashMap[hashId];
        }
        res.push_back(data);
    }
    return res;
}
}
}