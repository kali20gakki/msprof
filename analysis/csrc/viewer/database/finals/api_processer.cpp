/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : api_processer.cpp
 * Description        : 处理api相关数据
 * Author             : msprof team
 * Creation Date      : 2023/12/13
 * *****************************************************************************
 */
#include "analysis/csrc/viewer/database/finals/api_processer.h"

#include "analysis/csrc/parser/environment/context.h"
#include "analysis/csrc/association/credential/id_pool.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"

namespace Analysis {
namespace Viewer {
namespace Database {
using IdPool = Analysis::Association::Credential::IdPool;
using Context = Parser::Environment::Context;

namespace {
    struct ApiData {
        std::string structType = "";
        std::string id = "";
        std::string level = "";
        std::string itemId = "";
        uint64_t start = UINT64_MAX;
        uint64_t end = UINT64_MAX;
        uint32_t threadId = UINT32_MAX;
        uint32_t connectionId = UINT32_MAX;
    };
}

ApiProcesser::ApiProcesser(const std::string &reportDBPath, const std::set<std::string> &profPaths)
    : TableProcesser(reportDBPath, profPaths)
{
    apieventDB_.dbName = "api_event.db";
    apieventDB_.tableName = "ApiData";
    reportDB_.tableName = TABLE_NAME_API;
};

bool ApiProcesser::Process(const std::string &fileDir)
{
    std::string apiDBPath = Utils::File::PathJoin({fileDir, HOST, SQLITE, apieventDB_.dbName});
    ApiDataFormat apiData = GetData(apiDBPath);
    uint64_t pid = Context::GetInstance().GetPidFromInfoJson(Parser::Environment::HOST_ID, fileDir);
    if (!Context::GetInstance().GetSyscntConversionParams(params_, Parser::Environment::HOST_ID, fileDir)) {
        return false;
    }
    if (!Context::GetInstance().GetProfTimeRecordInfo(record_, fileDir)) {
        return false;
    }
    ProcessedDataFormat processedData;
    if (!FormatData(pid, apiData, processedData)) {
        return false;
    }
    return SaveData(processedData);
}

ApiProcesser::ApiDataFormat ApiProcesser::GetData(const std::string &dbPath)
{
    ApiDataFormat apiData;
    // db check 设置为10G
    if (!Utils::FileReader::Check(dbPath, MAX_DB_BYTES)) {
        WARN("Check % failed.", dbPath);
        return apiData;
    }
    MAKE_SHARED_RETURN_VALUE(apieventDB_.dbRunner, DBRunner, apiData, dbPath);
    if (apieventDB_.dbRunner == nullptr) {
        WARN("Create % connection failed.", dbPath);
        return apiData;
    }
    std::string sql = "SELECT struct_type, id, level, thread_id, item_id, start, end, connection_id "
                      "FROM " + apieventDB_.tableName;
    if (!apieventDB_.dbRunner->QueryData(sql, apiData)) {
        WARN("Query api data failed, db path is %.", dbPath);
        return apiData;
    }
    return apiData;
}

bool ApiProcesser::FormatData(const uint64_t &pid, const ApiDataFormat &apiData,
                              ProcessedDataFormat &processedData)
{
    if (apiData.empty()) {
        ERROR("Api original data is empty.");
        return false;
    }
    if (!Utils::Reserve(processedData, apiData.size())) {
        ERROR("Reserve for % data failed.", reportDB_.tableName);
        return false;
    }
    ApiData tempData;
    for (const auto& data : apiData) {
        std::tie(tempData.structType, tempData.id, tempData.level, tempData.threadId, tempData.itemId,
                 tempData.start, tempData.end, tempData.connectionId) = data;
        uint16_t level = GetLevelValue(tempData.level);
        uint64_t globalTid = (pid << 32) + tempData.threadId;
        double start = Utils::GetLocalTime(Utils::GetTimeFromSyscnt(tempData.start, params_), record_);
        double end = Utils::GetLocalTime(Utils::GetTimeFromSyscnt(tempData.end, params_), record_);
        uint64_t connectionId = (pid << 32) + tempData.connectionId;
        uint64_t name = IdPool::GetInstance().GetId(tempData.structType);
        if (level == MSPROF_REPORT_ACL_LEVEL) {
            name = IdPool::GetInstance().GetId(tempData.id);
        } else if (level == MSPROF_REPORT_HCCL_NODE_LEVEL) {
            name = IdPool::GetInstance().GetId(tempData.itemId);
        }
        processedData.emplace_back(start, end, level, globalTid, connectionId, name);
    }
    if (processedData.empty()) {
        ERROR("Api data processing is invalid.");
        return false;
    }
    return true;
}

uint16_t ApiProcesser::GetLevelValue(const std::string &key)
{
    auto it = API_LEVEL_TABLE.find(key);
    if (it == API_LEVEL_TABLE.end()) {
        ERROR("Unknown op type key: %", key);
        uint16_t res;
        if (!Utils::StrToU16(res, key)) {
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
