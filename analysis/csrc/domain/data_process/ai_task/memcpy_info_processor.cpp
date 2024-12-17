/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : memcpy_info_processor.cpp
 * Description        : memcpy_info_processor，处理MemcpyInfo表数据
 * Author             : msprof team
 * Creation Date      : 2024/12/12
 * *****************************************************************************
 */

#include "analysis/csrc/domain/data_process/ai_task/memcpy_info_processor.h"
#include "analysis/csrc/parser/environment/context.h"

namespace Analysis {
namespace Domain {
using namespace Analysis::Parser::Environment;
using namespace Analysis::Utils;
namespace {
const std::vector<std::string> MEMCPY_DIRECTIONS {
    "host to host",
    "host to device",
    "device to host",
    "device to device",
    "managed memory",
    "addr device to device",
    "host to device ex",
    "device to host ex"
};
const std::string OTHER_DIRECTION = "other";
}


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

OriMemcpyInfoFormat MemcpyInfoProcessor::LoadData(const DBInfo &runtimeDB, const std::string &dbPath)
{
    OriMemcpyInfoFormat oriData;
    if (runtimeDB.dbRunner == nullptr) {
        ERROR("Create % connection failed.", dbPath);
        return oriData;
    }
    std::string sql = "SELECT data_size, max_size, memcpy_direction, connection_id FROM " + runtimeDB.tableName;
    if (!runtimeDB.dbRunner->QueryData(sql, oriData)) {
        ERROR("Query memcpyInfo data failed, db path is %.", dbPath);
        return oriData;
    }
    return oriData;
}

std::vector<MemcpyInfoData> MemcpyInfoProcessor::FormatData(const OriMemcpyInfoFormat &oriData)
{
    std::vector<MemcpyInfoData> formatData;
    if (!Reserve(formatData, oriData.size())) {
        ERROR("Reserved for api data failed.");
        return formatData;
    }
    MemcpyInfoData data;
    uint16_t memcpyDirection;
    uint16_t tmpSize = static_cast<uint16_t>(MEMCPY_DIRECTIONS.size());
    for (const auto& row : oriData) {
        std::tie(data.dataSize, data.maxSize, memcpyDirection, data.connectionId) = row;
        if (memcpyDirection >= tmpSize) {
            data.memcpyDirection = OTHER_DIRECTION;
        } else {
            data.memcpyDirection = MEMCPY_DIRECTIONS[memcpyDirection];
        }
        formatData.push_back(data);
    }
    return formatData;
}
}
}