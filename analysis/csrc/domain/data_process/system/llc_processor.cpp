/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : llc_processor.cpp
 * Description        : llc_processor，处理llc表相关数据
 * Author             : msprof team
 * Creation Date      : 2024/8/20
 * *****************************************************************************
 */
#include "analysis/csrc/domain/data_process/system/llc_processor.h"
#include <unordered_set>
#include "analysis/csrc/parser/environment/context.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "collector/dvvp/common/config/config.h"

namespace Analysis {
namespace Domain {
using namespace Analysis::Parser::Environment;
using namespace Analysis::Utils;
using namespace analysis::dvvp::common::config;

namespace {
std::unordered_set<std::string> LLC_MODES = {LLC_PROFILING_READ, LLC_PROFILING_WRITE};
}

LLcProcessor::LLcProcessor(const std::string &profPath) : DataProcessor(profPath) {}

bool LLcProcessor::Process(DataInventory &dataInventory)
{
    auto platformVersion = Context::GetInstance().GetPlatformVersion(DEFAULT_DEVICE_ID, profPath_);
    if (Context::IsFirstChipV1(platformVersion)) {
        WARN("Skip llc processing in path: %.", profPath_);
        return true;
    }
    bool flag = true;
    std::vector<LLcData> allProcessedData;
    std::vector<LLcSummaryData> allSumaryData;
    auto deviceList = File::GetFilesWithPrefix(profPath_, DEVICE_PREFIX);
    for (const auto& devicePath: deviceList) {
        flag = ProcessSingleDevice(devicePath, allProcessedData, allSumaryData) && flag;
    }
    if (!SaveToDataInventory<LLcData>(std::move(allProcessedData), dataInventory, PROCESSOR_NAME_LLC) ||
        !SaveToDataInventory<LLcSummaryData>(std::move(allSumaryData), dataInventory, PROCESSOR_NAME_LLC)) {
        flag = false;
        ERROR("Save LLC Data To DataInventory failed, profPath is %.", profPath_);
    }
    return flag;
}

bool LLcProcessor::ProcessSingleDevice(const std::string &devicePath,
    std::vector<LLcData> &allProcessedData, std::vector<LLcSummaryData> &allSummaryData)
{
    LocaltimeContext localtimeContext;
    localtimeContext.deviceId = GetDeviceIdByDevicePath(devicePath);
    if (localtimeContext.deviceId == Parser::Environment::INVALID_DEVICE_ID) {
        ERROR("the invalid deviceId cannot to be identified, profPath is %.", profPath_);
        return false;
    }
    if (!Context::GetInstance().GetProfTimeRecordInfo(localtimeContext.timeRecord, profPath_,
                                                      localtimeContext.deviceId)) {
        ERROR("Failed to obtain the time in start_info and end_info, "
              "profPath is %, device id is %.", profPath_, localtimeContext.deviceId);
        return false;
    }
    DBInfo llcDB("llc.db", "LLCMetrics");
    std::string dbPath = File::PathJoin({devicePath, SQLITE, llcDB.dbName});
    if (!llcDB.ConstructDBRunner(dbPath) || llcDB.dbRunner == nullptr) {
        ERROR("Create % connection failed.", dbPath);
        return false;
    }
    // 并不是所有场景都有LLC数据
    auto status = CheckPathAndTable(dbPath, llcDB);
    if (status != CHECK_SUCCESS) {
        if (status == CHECK_FAILED) {
            return false;
        }
        return true;
    }
    std::string mode = Context::GetInstance().GetLLCProfiling(localtimeContext.deviceId, profPath_);
    if (LLC_MODES.find(mode) == LLC_MODES.end()) {
        ERROR("GetLLCProfiling failed in path: %, device id is %", profPath_, localtimeContext.deviceId);
        return false;
    }
    auto processedData = ProcessData(llcDB, localtimeContext, mode);
    if (processedData.empty()) {
        ERROR("Format LLC data error, dbPath is %.", dbPath);
        return false;
    }
    auto summaryData = ProcessSummaryData(localtimeContext.deviceId, llcDB, mode);
    if (summaryData.empty()) {
        ERROR("Process LLC summary data error, dbPath is %.", dbPath);
        return false;
    }
    allProcessedData.insert(allProcessedData.end(), processedData.begin(), processedData.end());
    allSummaryData.insert(allSummaryData.end(), summaryData.begin(), summaryData.end());
    return true;
}

std::vector<LLcData> LLcProcessor::ProcessData(const DBInfo &llcDB, LocaltimeContext &localtimeContext,
                                               const std::string &mode)
{
    if (!Context::GetInstance().GetClockMonotonicRaw(localtimeContext.hostMonotonic, true, localtimeContext.deviceId,
                                                     profPath_) ||
        !Context::GetInstance().GetClockMonotonicRaw(localtimeContext.deviceMonotonic, false, localtimeContext.deviceId,
                                                     profPath_)) {
        ERROR("Device MonotonicRaw is invalid in path: %., device id is %", profPath_, localtimeContext.deviceId);
        return {};
    }
    OriLLcData oriData = LoadData(llcDB);
    if (oriData.empty()) {
        ERROR("Get % data failed, profPath is %, device is %", llcDB.tableName, profPath_, localtimeContext.deviceId);
        return {};
    }
    return FormatData(oriData, localtimeContext, mode);
}

std::vector<LLcSummaryData> LLcProcessor::ProcessSummaryData(const uint16_t &deviceId, const DBInfo &llcDB,
                                                             const std::string mode)
{
    std::vector<LLcSummaryData> processedData;
    std::vector<std::tuple<double, double>> avgData;
    std::vector<std::tuple<uint32_t, double, double>> avgLLcIdData;
    // csv表里面的第一行，所有数据的hitrate和throughput的平均值
    std::string sql{"SELECT AVG(hitrate), AVG(throughput) FROM "+ llcDB.tableName};
    if (!llcDB.dbRunner->QueryData(sql, avgData) || avgData.empty()) {
        ERROR("Failed to obtain avg data from the % table, profPath is %, deviceId is %",
              llcDB.tableName, profPath_, deviceId);
        return processedData;
    }
    // 按照llcid分组的hitrate和throughput的平均值
    sql = "SELECT l3tid, AVG(hitrate), AVG(throughput) FROM "+ llcDB.tableName + " GROUP BY l3tid";
    if (!llcDB.dbRunner->QueryData(sql, avgLLcIdData) || avgLLcIdData.empty()) {
        ERROR("Failed to obtain avg llcid bandwidth from the % table, profPath is %, deviceId is %",
              llcDB.tableName, profPath_, deviceId);
        return processedData;
    }
    LLcSummaryData tempData;
    // 在这里赋值可以在外面根据deviceId判断是否查询失败
    tempData.deviceId = deviceId;
    tempData.mode = mode;
    // 这里vector已经判断过是否为空，所以可以取固定的索引
    std::tie(tempData.hitRate, tempData.throughput) = avgData.at(0);
    // 先将所有数据的平均值放进去，然后一一放每个llcid的平均值
    processedData.push_back(tempData);
    for (const auto &row : avgLLcIdData) {
        std::tie(tempData.llcId, tempData.hitRate, tempData.throughput) = row;
        tempData.hitRate = tempData.hitRate * PERCENTAGE;
        processedData.push_back(tempData);
    }
    return processedData;
}

OriLLcData LLcProcessor::LoadData(const DBInfo &llcDB)
{
    OriLLcData oriData;
    std::string sql{"SELECT l3tid, timestamp, hitrate, throughput FROM " + llcDB.tableName};
    if (!llcDB.dbRunner->QueryData(sql, oriData) || oriData.empty()) {
        ERROR("Failed to obtain data from the % table.", llcDB.tableName);
    }
    return oriData;
}

std::vector<LLcData> LLcProcessor::FormatData(const OriLLcData &oriData, const LocaltimeContext &localtimeContext,
                                              const std::string &mode)
{
    std::vector<LLcData> formatData;
    if (!Reserve(formatData, oriData.size())) {
        ERROR("Reserve for LLC data failed, profPath is %, deviceId is %.", profPath_, localtimeContext.deviceId);
        return formatData;
    }
    LLcData tempData;
    tempData.deviceId = localtimeContext.deviceId;
    tempData.mode = mode;
    double oriTimestamp;
    for (const auto &row: oriData) {
        std::tie(tempData.llcID, oriTimestamp, tempData.hitRate, tempData.throughput) = row;
        HPFloat timestamp = GetTimeBySamplingTimestamp(oriTimestamp, localtimeContext.hostMonotonic,
                                                       localtimeContext.deviceMonotonic);
        tempData.localTime = GetLocalTime(timestamp, localtimeContext.timeRecord).Uint64();
        tempData.hitRate = tempData.hitRate * PERCENTAGE;
        formatData.push_back(tempData);
    }
    return formatData;
}
}
}