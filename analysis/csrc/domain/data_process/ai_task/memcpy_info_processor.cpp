/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : memcpy_info_processor.cpp
 * Description        : memcpy_info_processor，处理MemcpyInfo表数据
 * Author             : msprof team
 * Creation Date      : 2025/01/07
 * *****************************************************************************
 */

#include "analysis/csrc/domain/data_process/ai_task/memcpy_info_processor.h"
#include "analysis/csrc/parser/environment/context.h"

namespace Analysis {
namespace Domain {
using namespace Analysis::Parser::Environment;
using namespace Analysis::Utils;
const uint32_t DEFAULT_CONTEXT_ID = UINT32_MAX;

MemcpyInfoProcessor::MemcpyInfoProcessor(const std::string &profPath) : DataProcessor(profPath) {}

bool MemcpyInfoProcessor::Process(DataInventory &dataInventory)
{
    DBInfo runtimeDB("runtime.db", "MemcpyInfo");
    std::string dbPath = Utils::File::PathJoin({profPath_, HOST, SQLITE, runtimeDB.dbName});
    if (!runtimeDB.ConstructDBRunner(dbPath)) {
        return false;
    }
    // 并不是所有场景都有memcpyInfo数据
    auto status = CheckPathAndTable(dbPath, runtimeDB, false);
    if (status == CHECK_FAILED) {
        return false;
    } else if (status == NOT_EXIST) {
        return true;
    }
    auto memcpyInfoData = LoadData(runtimeDB, dbPath);
    if (memcpyInfoData.empty()) {
        ERROR("MemcpyInfo original data is empty. DBPath is %", dbPath);
        return false;
    }
    auto processedData = FormatData(memcpyInfoData);
    if (processedData.empty()) {
        ERROR("format memcpyInfo data error");
        return false;
    }
    if (!SaveToDataInventory<MemcpyInfoData>(std::move(processedData), dataInventory,
            PROCESSOR_NAME_MEMCPY_INFO)) {
        ERROR("Save data failed, %.", PROCESSOR_NAME_MEMCPY_INFO);
        return false;
    }
    return true;
}

MemcpyInfoFormat MemcpyInfoProcessor::LoadData(const DBInfo &runtimeDB, const std::string &dbPath)
{
    MemcpyInfoFormat oriData;
    if (runtimeDB.dbRunner == nullptr) {
        ERROR("Create % connection failed.", dbPath);
        return oriData;
    }
    std::string sql = "SELECT stream_id, batch_id, task_id, context_id, device_id, data_size, memcpy_direction FROM " +
            runtimeDB.tableName;
    if (!runtimeDB.dbRunner->QueryData(sql, oriData)) {
        ERROR("Query memcpyInfo data failed, db path is %.", dbPath);
        return oriData;
    }
    return oriData;
}

std::vector<MemcpyInfoData> MemcpyInfoProcessor::FormatData(const MemcpyInfoFormat &oriData)
{
    std::vector<MemcpyInfoData> formatData;
    if (!Reserve(formatData, oriData.size())) {
        ERROR("Reserved for api data failed.");
        return formatData;
    }
    MemcpyInfoData data;
    for (const auto& row : oriData) {
        std::tie(data.taskId.streamId, data.taskId.batchId, data.taskId.taskId, data.taskId.contextId,
                 data.taskId.deviceId, data.dataSize, data.memcpyOperation) = row;
        formatData.push_back(data);
    }
    return formatData;
}
}
}