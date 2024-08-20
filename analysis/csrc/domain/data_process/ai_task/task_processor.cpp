/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : ascend_task_processor.cpp
 * Description        : 处理ascendTask表数据
 * Author             : msprof team
 * Creation Date      : 2024/8/2
 * *****************************************************************************
 */

#include "analysis/csrc/domain/data_process/ai_task/task_processor.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "analysis/csrc/parser/environment/context.h"

namespace Analysis {
namespace Domain {
using namespace Analysis::Parser::Environment;
using namespace Analysis::Utils;

TaskProcessor::TaskProcessor(const std::string &profPath) : DataProcessor(profPath) {}

bool TaskProcessor::Process(DataInventory &dataInventory)
{
    ProfTimeRecord record;
    if (!Context::GetInstance().GetProfTimeRecordInfo(record, profPath_)) {
        ERROR("GetProfTimeRecordInfo failed, profPath is %.", profPath_);
        return false;
    }
    bool flag = true;
    auto deviceList = Utils::File::GetFilesWithPrefix(profPath_, DEVICE_PREFIX);
    std::vector<AscendTaskData> res;
    for (const auto& devicePath: deviceList) {
        DBInfo ascendTaskDB("ascend_task.db", "AscendTask");
        std::string dbPath = Utils::File::PathJoin({devicePath, SQLITE, ascendTaskDB.dbName});
        if (!ascendTaskDB.ConstructDBRunner(dbPath)) {
            flag = false;
            continue;
        }
        auto status = CheckPathAndTable(dbPath, ascendTaskDB);
        if (status != CHECK_SUCCESS) {
            if (status == CHECK_FAILED) {
                flag = false;
            }
            continue;
        }
        uint16_t deviceId = GetDeviceIdByDevicePath(devicePath);
        if (deviceId == Parser::Environment::HOST_ID) {
            ERROR("the invalid deviceId cannot to be identified.");
            flag = false;
            continue;
        }
        auto oriData = LoadData(ascendTaskDB, dbPath);
        if (oriData.empty()) {
            flag = false;
            ERROR("AscendTask original data is empty. DBPath is %", dbPath);
            continue;
        }
        auto formatData = FormatData(oriData, record, deviceId);
        if (formatData.empty()) {
            flag = false;
            ERROR("AscendTask format data failed. DBPath is %", dbPath);
            continue;
        }
        res.insert(res.end(), formatData.begin(), formatData.end());
    }
    if (!SaveToDataInventory<AscendTaskData>(std::move(res), dataInventory, PROCESSOR_NAME_TASK)) {
        ERROR("Save data failed, %.", PROCESSOR_NAME_TASK);
        flag = false;
    }
    return flag;
}

OriAscendTaskData TaskProcessor::LoadData(const DBInfo &ascendTaskDB, const std::string &dbPath)
{
    OriAscendTaskData oriData;
    if (ascendTaskDB.dbRunner == nullptr) {
        ERROR("Create % connection failed.", dbPath);
        return oriData;
    }
    std::string sql{"SELECT a.start_time, a.duration, a.model_id, a.index_id, a.stream_id, a.task_id, a.context_id,"
                    "a.batch_id, a.connection_id, a.host_task_type, a.device_task_type FROM " + ascendTaskDB.tableName +
                    " a LEFT JOIN (SELECT DISTINCT stream_id, task_id, batch_id FROM " + ascendTaskDB.tableName +
                    " WHERE context_id <> 4294967295) b ON a.stream_id = b.stream_id AND a.task_id = b.task_id "
                    "AND a.batch_id = b.batch_id WHERE (a.context_id <> 4294967295 OR b.batch_id IS NULL) "
                    "and a.device_task_type != 'UNKNOWN'"};
    if (!ascendTaskDB.dbRunner->QueryData(sql, oriData)) {
        ERROR("Failed to obtain data from the % table.", ascendTaskDB.tableName);
    }
    return oriData;
}

std::vector<AscendTaskData> TaskProcessor::FormatData(
    const OriAscendTaskData &oriData, const ProfTimeRecord &timeRecord, const uint16_t deviceId)
{
    std::vector<AscendTaskData> processedData;
    if (!Utils::Reserve(processedData, oriData.size())) {
        ERROR("Reserve for AscendTask data failed.");
        return processedData;
    }
    AscendTaskData data;
    data.deviceId = deviceId;
    double start;
    for (const auto &row: oriData) {
        std::tie(start, data.duration, data.modelId, data.indexId, data.streamId, data.taskId, data.contextId,
                 data.batchId, data.connectionId, data.hostType, data.deviceType) = row;
        HPFloat timestamp{start};
        data.start = GetLocalTime(timestamp, timeRecord).Uint64();
        processedData.push_back(data);
    }
    return processedData;
}
}
}
