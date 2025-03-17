/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : api_processor.h
 * Description        : api_processor，处理api_event表数据
 * Author             : msprof team
 * Creation Date      : 2024/7/31
 * *****************************************************************************
 */

#include "analysis/csrc/domain/data_process/ai_task/api_processor.h"
#include "analysis/csrc/domain/services/environment/context.h"

namespace Analysis {
namespace Domain {
using namespace Analysis::Domain::Environment;
using namespace Analysis::Utils;


bool isContainedInRange(const std::vector<ApiData> &modelLoadDatas, const ApiData &data)
{
    for (const auto& modelLoadData : modelLoadDatas) {
        if (data.threadId == modelLoadData.threadId && data.timestamp >= modelLoadData.timestamp &&
            data.end <= modelLoadData.end) {
            return true;
        }
    }
    return false;
}

void FilterData(std::vector<ApiData> &datas, uint64_t startTimeNs, const std::string &processorName)
{
    INFO("There are % records before % data filtering, filterTime is %.", datas.size(), processorName, startTimeNs);
    std::vector<ApiData> modelLoadDatas;
    for (const auto& data : datas) {
        if (data.apiName == "ModelLoad") {
            modelLoadDatas.push_back(data);
        }
    }
    datas.erase(std::remove_if(datas.begin(), datas.end(),
                               [&modelLoadDatas, startTimeNs](const ApiData& data) {
                                   bool inRange = isContainedInRange(modelLoadDatas, data);
                                   bool isFiltered = data.timestamp >= startTimeNs;
                                   return !(inRange || isFiltered);
                               }),
                datas.end());
    INFO("There are % records after % data filtering.", datas.size(), processorName);
}

ApiProcessor::ApiProcessor(const std::string &profPath) : DataProcessor(profPath) {}

bool ApiProcessor::Process(DataInventory &dataInventory)
{
    DBInfo apiDB("api_event.db", "ApiData");
    std::string dbPath = Utils::File::PathJoin({profPath_, HOST, SQLITE, apiDB.dbName});
    if (!apiDB.ConstructDBRunner(dbPath)) {
        return false;
    }
    // 并不是所有场景都有api数据
    auto status = CheckPathAndTable(dbPath, apiDB);
    if (status == CHECK_FAILED) {
        return false;
    } else if (status == NOT_EXIST) {
        return true;
    }
    auto apiData = LoadData(apiDB, dbPath);
    if (apiData.empty()) {
        ERROR("Api original data is empty. DBPath is %", dbPath);
        return false;
    }
    ProfTimeRecord record;
    if (!Context::GetInstance().GetProfTimeRecordInfo(record, profPath_)) {
        ERROR("GetProfTimeRecordInfo failed, profPath is %.", profPath_);
        return false;
    }
    SyscntConversionParams params;
    if (!Context::GetInstance().GetSyscntConversionParams(params, HOST_ID, profPath_)) {
        ERROR("GetSyscntConversionParams failed, profPath is %.", profPath_);
        return false;
    }
    auto processedData = FormatData(apiData, record, params);
    if (processedData.empty()) {
        ERROR("format api data error");
        return false;
    }
    FilterData(processedData, record.startTimeNs, PROCESSOR_NAME_API); // 保留ModelLoad
    if (!SaveToDataInventory<ApiData>(std::move(processedData), dataInventory, PROCESSOR_NAME_API)) {
        ERROR("Save data failed, %.", PROCESSOR_NAME_API);
        return false;
    }
    return true;
}

OriApiDataFormat ApiProcessor::LoadData(const DBInfo &apiDB, const std::string &dbPath)
{
    OriApiDataFormat oriData;
    if (apiDB.dbRunner == nullptr) {
        ERROR("Create % connection failed.", dbPath);
        return oriData;
    }
    std::string sql = "SELECT struct_type, id, level, thread_id, item_id, start, end, connection_id "
                      "FROM " + apiDB.tableName;
    if (!apiDB.dbRunner->QueryData(sql, oriData)) {
        ERROR("Query api data failed, db path is %.", dbPath);
        return oriData;
    }
    return oriData;
}

std::vector<ApiData> ApiProcessor::FormatData(
    const OriApiDataFormat &oriData, const ProfTimeRecord &record, const SyscntConversionParams &params)
{
    std::vector<ApiData> formatData;
    if (!Reserve(formatData, oriData.size())) {
        ERROR("Reserved for api data failed.");
        return formatData;
    }
    ApiData data;
    std::string level;
    for (const auto& row : oriData) {
        std::tie(data.structType, data.id, level, data.threadId, data.itemId,
                 data.timestamp, data.end, data.connectionId) = row;
        HPFloat startTimestamp = Utils::GetTimeFromSyscnt(data.timestamp, params);
        HPFloat endTimestamp = Utils::GetTimeFromSyscnt(data.end, params);
        data.level = GetEnumTypeValue(level, NAME_STR(API_LEVEL_TABLE), API_LEVEL_TABLE);
        data.apiName = data.structType;
        if (data.level == MSPROF_REPORT_ACL_LEVEL) {
            data.apiName = data.id;
        } else if (data.level == MSPROF_REPORT_HCCL_NODE_LEVEL) {
            data.apiName = data.itemId;
        }
        data.timestamp = GetLocalTime(startTimestamp, record).Uint64();
        data.end = GetLocalTime(endTimestamp, record).Uint64();
        formatData.push_back(data);
    }
    return formatData;
}
}
}