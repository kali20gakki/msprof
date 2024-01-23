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
#include "analysis/csrc/viewer/database/finals/api_processor.h"

#include "analysis/csrc/association/credential/id_pool.h"
#include "analysis/csrc/dfx/error_code.h"
#include "analysis/csrc/parser/environment/context.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"

namespace Analysis {
namespace Viewer {
namespace Database {
using IdPool = Analysis::Association::Credential::IdPool;
using Context = Parser::Environment::Context;

namespace {
struct ApiData {
    std::string structType;
    std::string id;
    std::string level;
    std::string itemId;
    uint64_t start = UINT64_MAX;
    uint64_t end = UINT64_MAX;
    uint32_t threadId = UINT32_MAX;
    uint32_t connectionId = UINT32_MAX;
};
}

ApiProcessor::ApiProcessor(const std::string &reportDBPath, const std::set<std::string> &profPaths)
    : TableProcessor(reportDBPath, profPaths) {}

bool ApiProcessor::Run()
{
    INFO("ApiProcessor Run.");
    bool flag = TableProcessor::Run();
    PrintProcessorResult(flag, TABLE_NAME_API);
    return flag;
}

bool ApiProcessor::Process(const std::string &fileDir)
{
    INFO("ApiProcessor Process, dir is %", fileDir);
    DBInfo apieventDB("api_event.db", "ApiData");
    std::string dbPath = Utils::File::PathJoin({fileDir, HOST, SQLITE, apieventDB.dbName});
    // db check 设置为10G
    if (!Utils::FileReader::Check(dbPath, MAX_DB_BYTES)) {
        ERROR("Check % failed.", dbPath);
        return false;
    }
    ApiDataFormat apiData = GetData(dbPath, apieventDB);
    ProcessedDataFormat processedData;
    if (!FormatData(fileDir, apiData, processedData)) {
        ERROR("FormatData failed, fileDir is %.", fileDir);
        return false;
    }
    return SaveData(processedData, TABLE_NAME_API);
}

ApiProcessor::ApiDataFormat ApiProcessor::GetData(const std::string &dbPath, DBInfo &apieventDB)
{
    INFO("ApiProcessor GetData, dir is %", dbPath);
    ApiDataFormat apiData;
    MAKE_SHARED_RETURN_VALUE(apieventDB.dbRunner, DBRunner, apiData, dbPath);
    if (apieventDB.dbRunner == nullptr) {
        ERROR("Create % connection failed.", dbPath);
        return apiData;
    }
    std::string sql = "SELECT struct_type, id, level, thread_id, item_id, start, end, connection_id "
                      "FROM " + apieventDB.tableName;
    if (!apieventDB.dbRunner->QueryData(sql, apiData)) {
        ERROR("Query api data failed, db path is %.", dbPath);
        return apiData;
    }
    return apiData;
}

bool ApiProcessor::FormatData(const std::string &fileDir, const ApiDataFormat &apiData,
                              ProcessedDataFormat &processedData)
{
    INFO("ApiProcessor FormatData, dir is %", fileDir);
    uint32_t pid = IdPool::GetInstance().GetUint32Id(fileDir);
    Utils::SyscntConversionParams params;
    Utils::ProfTimeRecord record;
    if (!Context::GetInstance().GetSyscntConversionParams(params, Parser::Environment::HOST_ID, fileDir)) {
        ERROR("GetSyscntConversionParams failed, profPath is %.", fileDir);
        return false;
    }
    if (!Context::GetInstance().GetProfTimeRecordInfo(record, fileDir)) {
        ERROR("GetProfTimeRecordInfo failed, profPath is %.", fileDir);
        return false;
    }
    if (apiData.empty()) {
        ERROR("Api original data is empty.");
        return false;
    }
    if (!Utils::Reserve(processedData, apiData.size())) {
        ERROR("Reserve for % data failed.", TABLE_NAME_API);
        return false;
    }
    ApiData tempData;
    for (const auto& data : apiData) {
        std::tie(tempData.structType, tempData.id, tempData.level, tempData.threadId, tempData.itemId,
                 tempData.start, tempData.end, tempData.connectionId) = data;
        if (tempData.start == 0 || tempData.end == 0) {
            continue;
        }
        uint16_t level = GetLevelValue(tempData.level);
        uint64_t globalTid = Utils::Contact(pid, tempData.threadId);
        std::string start = std::to_string(
            Utils::GetLocalTime(Utils::GetTimeFromSyscnt(tempData.start, params), record));
        std::string end = std::to_string(
            Utils::GetLocalTime(Utils::GetTimeFromSyscnt(tempData.end, params), record));
        uint64_t connectionId = Utils::Contact(pid, tempData.connectionId);
        uint64_t name = IdPool::GetInstance().GetUint64Id(tempData.structType);
        if (level == MSPROF_REPORT_ACL_LEVEL) {
            name = IdPool::GetInstance().GetUint64Id(tempData.id);
        } else if (level == MSPROF_REPORT_HCCL_NODE_LEVEL) {
            name = IdPool::GetInstance().GetUint64Id(tempData.itemId);
        }
        processedData.emplace_back(start, end, level, globalTid, connectionId, name);
    }
    if (processedData.empty()) {
        ERROR("Api data processing error.");
        return false;
    }
    return true;
}

uint16_t ApiProcessor::GetLevelValue(const std::string &key)
{
    auto it = API_LEVEL_TABLE.find(key);
    if (it == API_LEVEL_TABLE.end()) {
        ERROR("Unknown op type key: %", key);
        uint16_t res;
        if (Utils::StrToU16(res, key) != ANALYSIS_OK) {
            ERROR("Unknown op type key: %, it will be set %.", key, UINT16_MAX);
            return UINT16_MAX;
        }
        return res;
    }
    return it->second;
}

} // Database
} // Viewer
} // Analysis
