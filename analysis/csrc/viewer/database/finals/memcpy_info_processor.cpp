/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : api_processor.cpp
 * Description        : 处理api相关数据
 * Author             : msprof team
 * Creation Date      : 2025/01/07
 * *****************************************************************************
 */
#include "analysis/csrc/viewer/database/finals/memcpy_info_processor.h"

#include "analysis/csrc/application/credential/id_pool.h"
#include "analysis/csrc/domain/services/environment/context.h"

namespace Analysis {
namespace Viewer {
namespace Database {
using IdPool = Analysis::Application::Credential::IdPool;
using Context = Domain::Environment::Context;

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
    std::string sql = "SELECT stream_id, batch_id, task_id, context_id, device_id, data_size, "
                      "memcpy_direction FROM " + runtimeDB.tableName;
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
    uint32_t streamId;
    uint16_t batchId;
    uint16_t taskId;
    uint32_t contextId;
    uint16_t deviceId;
    uint64_t dataSize;
    uint16_t operation;
    for (const auto& data : memcpyInfoData) {
        std::tie(streamId, batchId, taskId, contextId, deviceId, dataSize, operation) = data;
        uint64_t globalTaskId = IdPool::GetInstance().GetId(std::make_tuple(deviceId, streamId, taskId, contextId,
                                                                            batchId));
        processedData.emplace_back(globalTaskId, dataSize, operation);
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