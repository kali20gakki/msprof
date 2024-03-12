/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : hbm_processor.h
 * Description        : hbm_processor，处理hbm表相关数据
 * Author             : msprof team
 * Creation Date      : 2024/2/27
 * *****************************************************************************
 */
#include "analysis/csrc/viewer/database/finals/hbm_processor.h"
#include "analysis/csrc/parser/environment/context.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"

namespace Analysis {
namespace Viewer {
namespace Database {
using namespace Analysis::Parser::Environment;
using namespace Analysis::Utils;
namespace {
struct HBMData {
    uint32_t deviceId = 0; // 这里使用uint32是因为db_runner的QueryData方法不支持存储精度低于uint32的数据
    uint32_t hbmId = 0;
    double bandwidth = 0;
    double timestamp = 0.0;
    std::string eventType;
};
}

HBMProcessor::HBMProcessor(const std::string &reportDBPath, const std::set<std::string> &profPaths)
    : TableProcessor(reportDBPath, profPaths) {}

bool HBMProcessor::Run()
{
    INFO("HBMProcessor Run.");
    bool flag = TableProcessor::Run();
    PrintProcessorResult(flag, PROCESSOR_NAME_HBM);
    return flag;
}

HBMProcessor::OriDataFormat HBMProcessor::GetData(const DBInfo &HBMDB)
{
    OriDataFormat oriData;
    std::string sql{"SELECT device_id, timestamp, bandwidth, hbmid, event_type FROM " + HBMDB.tableName};
    if (!HBMDB.dbRunner->QueryData(sql, oriData)) {
        ERROR("Failed to obtain data from the % table.", HBMDB.tableName);
    }
    return oriData;
}

HBMProcessor::ProcessedDataFormat HBMProcessor::FormatData(const OriDataFormat &oriData,
                                                           const Utils::ProfTimeRecord &timeRecord)
{
    ProcessedDataFormat processedData;
    HBMData data;
    if (!Utils::Reserve(processedData, oriData.size())) {
        ERROR("Reserve for HBM data failed.");
        return processedData;
    }
    for (auto &row: oriData) {
        std::tie(data.deviceId, data.timestamp, data.bandwidth, data.hbmId, data.eventType) = row;
        HPFloat timestamp{data.timestamp};
        uint16_t type = MEMORY_TABLE.at(data.eventType); // 枚举表MEMORY_TABLE可以保证key存在
        processedData.emplace_back(
            static_cast<uint16_t>(data.deviceId), GetLocalTime(timestamp, timeRecord).Uint64(),
            static_cast<uint64_t>(data.bandwidth * BYTE_SIZE * BYTE_SIZE), data.hbmId, type); // bandwidth MB/s -> B/s
    }
    return processedData;
}

bool HBMProcessor::Process(const std::string &fileDir)
{
    INFO("Start to process %.", fileDir);
    Utils::ProfTimeRecord timeRecord;
    DBInfo hbmDB("hbm.db", "HBMbwData");
    bool flag = true;
    MAKE_SHARED0_NO_OPERATION(hbmDB.database, HBMDB);
    auto deviceList = Utils::File::GetFilesWithPrefix(fileDir, DEVICE_PREFIX);
    bool timeFlag = Context::GetInstance().GetProfTimeRecordInfo(timeRecord, fileDir);
    for (const auto& devicePath: deviceList) {
        std::string dbPath = Utils::File::PathJoin({devicePath, SQLITE, hbmDB.dbName});
        // 并不是所有场景都有hbm数据
        auto status = CheckPath(dbPath);
        if (status != CHECK_SUCCESS) {
            if (status == CHECK_FAILED) {
                flag = false;
            }
            continue;
        }
        INFO("Start to process dbpath: %.", dbPath);
        if (!timeFlag) {
            ERROR("Failed to obtain the time in start_info and end_info.");
            flag = false;
            continue;
        }
        MAKE_SHARED_RETURN_VALUE(hbmDB.dbRunner, DBRunner, false, dbPath);
        auto oriData = GetData(hbmDB);
        if (oriData.empty()) {
            flag = false;
            ERROR("Get % data failed in %.", hbmDB.tableName, dbPath);
            continue;
        }
        auto processedData = FormatData(oriData, timeRecord);
        if (!SaveData(processedData, TABLE_NAME_HBM)) {
            flag = false;
            ERROR("Save data failed, %.", dbPath);
            continue;
        }
        INFO("process dbpath % ends.", dbPath);
    }
    INFO("process % ends.", fileDir);
    return flag;
}

}
}
}