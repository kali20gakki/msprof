/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : ddr_processor.h
 * Description        : ddr_processor，处理ddr表相关数据
 * Author             : msprof team
 * Creation Date      : 2024/08/09
 * *****************************************************************************
 */
#include "analysis/csrc/domain/data_process/system/ddr_processor.h"
#include "analysis/csrc/domain/services/environment/context.h"

namespace Analysis {
namespace Domain {
using namespace Analysis::Domain::Environment;
using namespace Analysis::Utils;
DDRProcessor::DDRProcessor(const std::string& profPaths) : DataProcessor(profPaths)
{}

OriDataFormat DDRProcessor::LoadData(const Analysis::Infra::DBInfo& DDRDB)
{
    OriDataFormat oriData;
    std::string sql{"SELECT device_id, timestamp, flux_read, flux_write FROM " + DDRDB.tableName};
    if (!DDRDB.dbRunner->QueryData(sql, oriData)) {
        ERROR("Failed to obtain data from the % table.", DDRDB.tableName);
    }
    return oriData;
}

std::vector<DDRData> DDRProcessor::FormatData(const OriDataFormat& oriData, const LocaltimeContext& localtimeContext)
{
    std::vector<DDRData> processedData;
    DDRData data;
    if (!Reserve(processedData, oriData.size())) {
        ERROR("Reserve for DDR data failed.");
        return processedData;
    }
    uint32_t deviceId; // 这里使用uint32是因为db_runner的QueryData方法不支持存储精度低于uint32的数据
    double oriTimestamp;
    double fluxRead;
    double fluxWrite;
    for (auto& row : oriData) {
        std::tie(deviceId, oriTimestamp, fluxRead, fluxWrite) = row;
        HPFloat timestamp = GetTimeBySamplingTimestamp(oriTimestamp, localtimeContext.hostMonotonic,
                                                       localtimeContext.deviceMonotonic);
        data.deviceId = static_cast<uint16_t>(deviceId);
        data.timestamp = GetLocalTime(timestamp, localtimeContext.timeRecord).Uint64();
        data.fluxRead = fluxRead * BYTE_SIZE * BYTE_SIZE;
        data.fluxWrite = fluxWrite * BYTE_SIZE * BYTE_SIZE;
        processedData.push_back(data);
    }
    return processedData;
}

bool DDRProcessor::ProcessOneDevice(const std::string& devicePath, std::vector<DDRData>& res)
{
    LocaltimeContext localtimeContext;
    uint16_t deviceId = GetDeviceIdByDevicePath(devicePath);
    if (deviceId == INVALID_DEVICE_ID) {
        ERROR("The invalid deviceId cannot to be identified.");
        return false;
    }
    if (!Context::GetInstance().GetProfTimeRecordInfo(localtimeContext.timeRecord, profPath_, deviceId)) {
        ERROR("Failed to obtain the time in start_info and end_info. Path is %, device id is %.", profPath_, deviceId);
        return false;
    }
    if (!Context::GetInstance().GetClockMonotonicRaw(localtimeContext.hostMonotonic, true, deviceId, profPath_) ||
        !Context::GetInstance().GetClockMonotonicRaw(localtimeContext.deviceMonotonic, false, deviceId, profPath_)) {
        ERROR("Device MonotonicRaw is invalid in path: %., device id is %", profPath_, deviceId);
        return false;
    }
    DBInfo ddrDB("ddr.db", "DDRMetricData");
    std::string dbPath = File::PathJoin({devicePath, SQLITE, ddrDB.dbName});
    if (!ddrDB.ConstructDBRunner(dbPath)) {
        return false;
    }
    auto status = CheckPathAndTable(dbPath, ddrDB);
    if (status != CHECK_SUCCESS) {
        if (status == CHECK_FAILED) {
            return false;
        }
        return true;
    }
    auto oriData = LoadData(ddrDB);
    if (oriData.empty()) {
        ERROR("Get % data failed in %.", ddrDB.tableName, dbPath);
        return false;
    }
    auto formatData = FormatData(oriData, localtimeContext);
    if (formatData.empty()) {
        ERROR("DDR Data format data failed. DBPath is %", dbPath);
        return false;
    }
    res.insert(res.end(), formatData.begin(), formatData.end());
    return true;
}

bool DDRProcessor::Process(DataInventory& dataInventory)
{
    bool flag = true;
    std::vector<DDRData> res;
    auto deviceList = File::GetFilesWithPrefix(profPath_, DEVICE_PREFIX);
    for (const auto& devicePath : deviceList) {
        flag = ProcessOneDevice(devicePath, res) && flag;
    }
    if (!SaveToDataInventory<DDRData>(std::move(res), dataInventory, PROCESSOR_NAME_DDR)) {
        flag = false;
        ERROR("Save data failed, %.", PROCESSOR_NAME_DDR);
    }
    return flag;
}
}
}