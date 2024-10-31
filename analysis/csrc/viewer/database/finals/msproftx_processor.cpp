/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : msproftx_processor.cpp
 * Description        : msproftx_processor，处理msproftx表 msproftx打点相关数据
 * Author             : msprof team
 * Creation Date      : 2024/05/20
 * *****************************************************************************
 */

#include "analysis/csrc/viewer/database/finals/msproftx_processor.h"
#include "analysis/csrc/utils/utils.h"
#include "analysis/csrc/parser/environment/context.h"
#include "analysis/csrc/association/credential/id_pool.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"

namespace Analysis {
namespace Viewer {
namespace Database {
using IdPool = Analysis::Association::Credential::IdPool;
using Context = Parser::Environment::Context;
using namespace Analysis::Parser::Environment;
using namespace Analysis::Utils;
namespace {
struct MsprofTxData {
    uint32_t tid = UINT32_MAX;
    uint32_t category = UINT32_MAX;
    std::string eventType;
    uint64_t startTime = UINT64_MAX;
    uint64_t endTime = UINT64_MAX;
    std::string message;
};

struct MsprofTxExData {
    uint32_t tid = UINT32_MAX;
    std::string eventType;
    uint64_t startTime = UINT64_MAX;
    uint64_t endTime = UINT64_MAX;
    std::string message;
    uint64_t markId = UINT64_MAX;
};
}

MsprofTxProcessor::MsprofTxProcessor(const std::string &msprofDBPath, const std::set<std::string> &profPaths)
    : TableProcessor(msprofDBPath, profPaths) {}

bool MsprofTxProcessor::Run()
{
    INFO("MsprofTxProcessor Run.");
    bool flag = TableProcessor::Run();
    PrintProcessorResult(flag, PROCESSOR_NAME_MSTX);
    return flag;
}

bool MsprofTxProcessor::Process(const std::string &fileDir)
{
    INFO("MsprofTxProcessor Process, dir is %", fileDir);

    std::string dbPath = Utils::File::PathJoin({fileDir, HOST, SQLITE, "msproftx.db"});
    // 并不是所有场景都有msproftx数据
    auto status = CheckPath(dbPath);
    if (status != CHECK_SUCCESS) {
        return status != CHECK_FAILED;
    }
    MsprofTxDataFormat msprofTxData = GetTxData(fileDir);
    MsprofTxExDataFormat msprofTxExData = GetTxExData(fileDir);
    ProcessedDataFormat processedData;
    if (!Utils::Reserve(processedData, msprofTxData.size() + msprofTxExData.size())) {
        ERROR("Reserve for % data failed.", TABLE_NAME_MSTX);
        return false;
    }
    uint32_t profId = IdPool::GetInstance().GetUint32Id(fileDir);
    uint32_t pid = Context::GetInstance().GetPidFromInfoJson(Parser::Environment::HOST_ID, fileDir);
    Utils::SyscntConversionParams params;
    if (!Context::GetInstance().GetSyscntConversionParams(params, Parser::Environment::HOST_ID, fileDir)) {
        ERROR("GetSyscntConversionParams failed, profPath is %.", fileDir);
        return false;
    }
    Utils::ProfTimeRecord record;
    if (!Context::GetInstance().GetProfTimeRecordInfo(record, fileDir)) {
        ERROR("GetProfTimeRecordInfo failed, profPath is %.", fileDir);
        return false;
    }
    if (!FormatTxData(msprofTxData, processedData, pid, params, record) ||
        !FormatTxExData(msprofTxExData, processedData, fileDir, params, record)) {
        ERROR("FormatData failed, fileDir is %.", fileDir);
        return false;
    }
    return SaveData(processedData, TABLE_NAME_MSTX);
}

MsprofTxProcessor::MsprofTxDataFormat MsprofTxProcessor::GetTxData(const std::string &fileDir)
{
    DBInfo msprofTxDb("msproftx.db", "MsprofTx");
    std::string dbPath = Utils::File::PathJoin({fileDir, HOST, SQLITE, msprofTxDb.dbName});
    MsprofTxDataFormat data;
    MAKE_SHARED_RETURN_VALUE(msprofTxDb.dbRunner, DBRunner, data, dbPath);
    if (msprofTxDb.dbRunner == nullptr) {
        ERROR("Create % connection failed.", dbPath);
        return data;
    }
    std::string sql = "SELECT tid, category, event_type, start_time, end_time, message FROM " +
                      msprofTxDb.tableName;
    if (!msprofTxDb.dbRunner->QueryData(sql, data)) {
        ERROR("Query msproftx data failed, db path is %.", dbPath);
        return data;
    }
    return data;
}

MsprofTxProcessor::MsprofTxExDataFormat MsprofTxProcessor::GetTxExData(const std::string &fileDir)
{
    DBInfo msprofTxExDb("msproftx.db", "MsprofTxEx");
    std::string dbPath = Utils::File::PathJoin({fileDir, HOST, SQLITE, msprofTxExDb.dbName});
    MsprofTxExDataFormat data;
    MAKE_SHARED_RETURN_VALUE(msprofTxExDb.dbRunner, DBRunner, data, dbPath);
    if (msprofTxExDb.dbRunner == nullptr) {
        ERROR("Create % connection failed.", dbPath);
        return data;
    }
    std::string sql = "SELECT tid, event_type, start_time, end_time, message, mark_id FROM " +
                      msprofTxExDb.tableName;
    if (!msprofTxExDb.dbRunner->QueryData(sql, data)) {
        ERROR("Query msproftx data failed, db path is %.", dbPath);
        return data;
    }
    return data;
}

bool MsprofTxProcessor::FormatTxData(const MsprofTxDataFormat &msprofTxData, ProcessedDataFormat &processedData,
                                     uint32_t pid, Utils::SyscntConversionParams &params, Utils::ProfTimeRecord &record)
{
    INFO("MsprofTxProcessor FormatTxData");
    if (msprofTxData.empty()) {
        WARN("Msproftx original data is empty");
        return true;
    }
    MsprofTxData tmpData;
    for (const auto &data : msprofTxData) {
        std::tie(tmpData.tid, tmpData.category, tmpData.eventType, tmpData.startTime, tmpData.endTime,
                 tmpData.message) = data;
        uint64_t globalTid = Utils::Contact(pid, tmpData.tid);
        uint64_t message = IdPool::GetInstance().GetUint64Id(tmpData.message);
        Utils::HPFloat startTimestamp = Utils::GetTimeFromSyscnt(tmpData.startTime, params);
        Utils::HPFloat endTimestamp = Utils::GetTimeFromSyscnt(tmpData.endTime, params);
        uint16_t eventType = GetEnumTypeValue(tmpData.eventType,
                                              NAME_STR(MSTX_EVENT_TYPE_TABLE), MSTX_EVENT_TYPE_TABLE);
        processedData.emplace_back(Utils::GetLocalTime(startTimestamp, record).Uint64(),
                                   Utils::GetLocalTime(endTimestamp, record).Uint64(),
                                   eventType, UINT32_MAX, tmpData.category, message,
                                   globalTid, globalTid, UINT16_MAX, DEFAULT_CONNECTION_ID_MSTX);
    }
    if (processedData.empty()) {
        ERROR("MsprofTx data processing error.");
        return false;
    }
    return true;
}

bool MsprofTxProcessor::FormatTxExData(const MsprofTxExDataFormat &txExData, ProcessedDataFormat &processedData,
                                       const std::string &fileDir, Utils::SyscntConversionParams &params,
                                       Utils::ProfTimeRecord &record)
{
    INFO("MsprofTxExProcessor FormatTxData");
    if (txExData.empty()) {
        WARN("MsprofTxEx original data is empty");
        return true;
    }
    uint32_t profId = IdPool::GetInstance().GetUint32Id(fileDir);
    uint32_t pid = Context::GetInstance().GetPidFromInfoJson(Parser::Environment::HOST_ID, fileDir);
    MsprofTxExData tmpData;
    for (const auto &data : txExData) {
        std::tie(tmpData.tid, tmpData.eventType, tmpData.startTime, tmpData.endTime,
                 tmpData.message, tmpData.markId) = data;
        uint64_t globalTid = Utils::Contact(pid, tmpData.tid);
        uint64_t message = IdPool::GetInstance().GetUint64Id(tmpData.message);
        Utils::HPFloat startTimestamp = Utils::GetTimeFromSyscnt(tmpData.startTime, params);
        Utils::HPFloat endTimestamp = Utils::GetTimeFromSyscnt(tmpData.endTime, params);
        uint16_t eventType = GetEnumTypeValue(tmpData.eventType,
                                              NAME_STR(MSTX_EVENT_TYPE_TABLE), MSTX_EVENT_TYPE_TABLE);
        uint64_t connectionId = Utils::Contact(profId, tmpData.markId + START_CONNECTION_ID_MSTX);
        processedData.emplace_back(Utils::GetLocalTime(startTimestamp, record).Uint64(),
                                   Utils::GetLocalTime(endTimestamp, record).Uint64(),
                                   eventType, UINT32_MAX, UINT32_MAX, message, globalTid,
                                   globalTid, UINT16_MAX, connectionId);
    }
    if (processedData.empty()) {
        ERROR("MsprofTxEx data processing error.");
        return false;
    }
    return true;
}

}
}
}

