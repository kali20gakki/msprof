/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : api_processor.cpp
 * Description        : 处理api相关数据
 * Author             : msprof team
 * Creation Date      : 2023/12/13
 * *****************************************************************************
 */
#include "analysis/csrc/viewer/database/finals/memcpy_info_processor.h"

#include "analysis/csrc/association/credential/id_pool.h"
#include "analysis/csrc/parser/environment/context.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"

namespace Analysis {
namespace Viewer {
namespace Database {
using IdPool = Analysis::Association::Credential::IdPool;
using Context = Parser::Environment::Context;

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

MemcpyInfoProcessor::MemcpyInfoProcessor(const std::string &msprofDBPath, const std::set<std::string> &profPaths)
    : TableProcessor(msprofDBPath, profPaths) {}

bool MemcpyInfoProcessor::Run()
{
    INFO("MemcpyInfoProcessor Run.");
    bool flag = TableProcessor::Run();
    PrintProcessorResult(flag, PROCESSOR_NAME_MEMCPY_INFO);
    return flag;
}

bool MemcpyInfoProcessor::Process(const std::string &fileDir)
{
    INFO("MemcpyInfoProcessor Process, dir is %", fileDir);
    DBInfo runtimeDB("runtime.db", "MemcpyInfo");
    std::string dbPath = Utils::File::PathJoin({fileDir, HOST, SQLITE, runtimeDB.dbName});
    MAKE_SHARED_RETURN_VALUE(runtimeDB.dbRunner, DBRunner, false, dbPath);
    // 并不是所有场景都有memcpyInfo数据
    auto status = CheckPathAndTable(dbPath, runtimeDB);
    if (status != CHECK_SUCCESS) {
        INFO("Check % data exit in %.", runtimeDB.tableName, dbPath);
        return status != CHECK_FAILED;
    }
    MemcpyInfoFormat memcpyInfoData = GetData(dbPath, runtimeDB);
    ProcessedDataFormat processedData;
    if (!FormatData(fileDir, memcpyInfoData, processedData)) {
        ERROR("FormatData failed, fileDir is %.", fileDir);
        return false;
    }
    return SaveData(processedData, TABLE_NAME_MEMCPY_INFO);
}

MemcpyInfoProcessor::MemcpyInfoFormat MemcpyInfoProcessor::GetData(const std::string &dbPath, DBInfo &runtimeDB)
{
    INFO("MemcpyInfoProcessor GetData, dir is %", dbPath);
    MemcpyInfoFormat memcpyInfoData;
    MAKE_SHARED_RETURN_VALUE(runtimeDB.dbRunner, DBRunner, memcpyInfoData, dbPath);
    if (runtimeDB.dbRunner == nullptr) {
        ERROR("Create % connection failed.", dbPath);
        return memcpyInfoData;
    }
    std::string sql = "SELECT connection_id, data_size, memcpy_direction, max_size FROM " + runtimeDB.tableName;
    if (!runtimeDB.dbRunner->QueryData(sql, memcpyInfoData)) {
        ERROR("Query memcpyInfo data failed, db path is %.", dbPath);
        return memcpyInfoData;
    }
    return memcpyInfoData;
}

bool MemcpyInfoProcessor::FormatData(const std::string &fileDir, const MemcpyInfoFormat &memcpyInfoData,
                                     ProcessedDataFormat &processedData)
{
    if (memcpyInfoData.empty()) {
        ERROR("MemcpyInfo original data is empty.");
        return false;
    }
    if (!Utils::Reserve(processedData, memcpyInfoData.size())) {
        ERROR("Reserve for % data failed.", TABLE_NAME_MEMCPY_INFO);
        return false;
    }
    INFO("MemcpyInfoProcessor FormatData, dir is %", fileDir);
    uint64_t connectionId;
    uint64_t dataSize;
    uint16_t memcpyDirection;
    std::string direction;
    uint64_t maxSize;
    uint16_t tmpSize = static_cast<uint16_t>(MEMCPY_DIRECTIONS.size());
    for (const auto& data : memcpyInfoData) {
        std::tie(connectionId, dataSize, memcpyDirection, maxSize) = data;
        if (memcpyDirection >= tmpSize) {
            direction = OTHER_DIRECTION;
        } else {
            direction = MEMCPY_DIRECTIONS[memcpyDirection];
        }
        processedData.emplace_back(connectionId, dataSize, IdPool::GetInstance().GetUint64Id(direction), maxSize);
    }
    if (processedData.empty()) {
        ERROR("MemcpyInfo data processing error.");
        return false;
    }
    return true;
}

} // Database
} // Viewer
} // Analysis
