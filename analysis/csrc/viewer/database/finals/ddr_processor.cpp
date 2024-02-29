/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : ddr_processor.h
 * Description        : ddr_processor，处理ddr表相关数据
 * Author             : msprof team
 * Creation Date      : 2024/2/27
 * *****************************************************************************
 */
#include "analysis/csrc/viewer/database/finals/ddr_processor.h"
#include "analysis/csrc/parser/environment/context.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"

namespace Analysis {
namespace Viewer {
namespace Database {
using namespace Analysis::Parser::Environment;
using namespace Analysis::Utils;
namespace {
struct DDRData {
    uint32_t deviceId = 0; // 这里使用uint32是因为db_runner的QueryData方法不支持存储精度低于uint32的数据
    double fluxRead = 0.0;
    double fluxWrite = 0.0;
    double timestamp = 0.0;
};
}

DDRProcessor::DDRProcessor(const std::string &reportDBPath, const std::set<std::string> &profPaths)
    : TableProcessor(reportDBPath, profPaths) {}

bool DDRProcessor::Run()
{
    INFO("DDRProcessor Run.");
    bool flag = TableProcessor::Run();
    PrintProcessorResult(flag, PROCESSOR_NAME_DDR);
    return flag;
}

DDRProcessor::OriDataFormat DDRProcessor::GetData(const DBInfo &DDRDB)
{
    OriDataFormat oriData;
    std::string sql{"SELECT device_id, timestamp, flux_read, flux_write FROM " + DDRDB.tableName};
    if (!DDRDB.dbRunner->QueryData(sql, oriData)) {
        ERROR("Failed to obtain data from the % table.", DDRDB.tableName);
    }
    return oriData;
}

DDRProcessor::ProcessedDataFormat DDRProcessor::FormatData(const OriDataFormat &oriData,
                                                           const Utils::ProfTimeRecord &timeRecord)
{
    ProcessedDataFormat processedData;
    DDRData data;
    if (!Utils::Reserve(processedData, oriData.size())) {
        ERROR("Reserve for DDR data failed.");
        return processedData;
    }
    for (auto &row: oriData) {
        std::tie(data.deviceId, data.timestamp, data.fluxRead, data.fluxWrite) = row;
        HPFloat timestamp{data.timestamp};
        processedData.emplace_back(
            static_cast<uint16_t>(data.deviceId), GetLocalTime(timestamp, timeRecord).Uint64(), data.fluxRead,
            data.fluxWrite);
    }
    return processedData;
}

bool DDRProcessor::Process(const std::string &fileDir)
{
    INFO("Start to process %.", fileDir);
    Utils::ProfTimeRecord timeRecord;
    DBInfo ddrDB("ddr.db", "DDRMetricData");
    bool flag = true;
    MAKE_SHARED0_NO_OPERATION(ddrDB.database, DDRDB);
    auto deviceList = Utils::File::GetFilesWithPrefix(fileDir, DEVICE_PREFIX);
    bool timeFlag = Context::GetInstance().GetProfTimeRecordInfo(timeRecord, fileDir);
    for (const auto& devicePath: deviceList) {
        std::string dbPath = Utils::File::PathJoin({devicePath, SQLITE, ddrDB.dbName});
        // 并不是所有场景都有ddr数据
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
        MAKE_SHARED_RETURN_VALUE(ddrDB.dbRunner, DBRunner, false, dbPath);
        auto oriData = GetData(ddrDB);
        if (oriData.empty()) {
            flag = false;
            ERROR("Get % data failed in %.", ddrDB.tableName, dbPath);
            continue;
        }
        auto processedData = FormatData(oriData, timeRecord);
        if (!SaveData(processedData, TABLE_NAME_DDR)) {
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