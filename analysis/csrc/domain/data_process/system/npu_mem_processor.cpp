/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : npu_mem_processor.cpp
 * Description        : npu_mem_processor，处理NpuMem表相关数据
 * Author             : msprof team
 * Creation Date      : 2024/8/8
 * *****************************************************************************
 */
#include "analysis/csrc/domain/data_process/system/npu_mem_processor.h"
#include "analysis/csrc/parser/environment/context.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"

namespace Analysis {
namespace Domain {
using namespace Analysis::Parser::Environment;

NpuMemProcessor::NpuMemProcessor(const std::string &profPath) : DataProcessor(profPath) {}

bool NpuMemProcessor::Process(DataInventory &dataInventory)
{
    LocaltimeContext localtimeContext;
    if (!Context::GetInstance().GetProfTimeRecordInfo(localtimeContext.timeRecord, profPath_)) {
        ERROR("Failed to obtain the time in start_info and end_info, profPath is %.", profPath_);
        return false;
    }
    DBInfo npuMemDB("npu_mem.db", "NpuMem");
    bool flag = true;
    auto deviceList = File::GetFilesWithPrefix(profPath_, DEVICE_PREFIX);
    std::vector<NpuMemData> allProcessedData;
    for (const auto& devicePath: deviceList) {
        localtimeContext.deviceId = GetDeviceIdByDevicePath(devicePath);
        flag = ProcessSingleDevice(devicePath, localtimeContext, allProcessedData) && flag;
    }
    if (!SaveToDataInventory<NpuMemData>(std::move(allProcessedData), dataInventory, PROCESSOR_NAME_NPU_MEM)) {
            flag = false;
            ERROR("Save NpuMem Data To DataInventory failed, profPath is %.", profPath_);
    }
    return flag;
}

bool NpuMemProcessor::ProcessSingleDevice(const std::string &devicePath, LocaltimeContext &localtimeContext,
                                          std::vector<NpuMemData> &allProcessedData)
{
    DBInfo npuMemDB("npu_mem.db", "NpuMem");
    if (localtimeContext.deviceId == Parser::Environment::HOST_ID) {
        ERROR("the invalid deviceId cannot to be identified, profPath is %", profPath_);
        return false;
    }
    if (!Context::GetInstance().GetClockMonotonicRaw(localtimeContext.hostMonotonic, true, localtimeContext.deviceId,
                                                     profPath_)) {
        ERROR("GetClockMonotonicRaw failed, profPath is %, deviceId is %.", profPath_, localtimeContext.deviceId);
        return false;
    }
    std::string dbPath = File::PathJoin({devicePath, SQLITE, npuMemDB.dbName});
    if (!npuMemDB.ConstructDBRunner(dbPath) || npuMemDB.dbRunner == nullptr) {
        ERROR("Create % connection failed.", dbPath);
        return false;
    }
    // 并不是所有场景都有NpuMem数据
    auto status = CheckPathAndTable(dbPath, npuMemDB);
    if (status != CHECK_SUCCESS) {
        if (status == CHECK_FAILED) {
            return false;
        }
        return true;
    }
    OriNpuMemData oriData = LoadData(npuMemDB);
    if (oriData.empty()) {
        ERROR("Get % data failed in %.", npuMemDB.tableName, dbPath);
        return false;
    }
    auto processedData = FormatData(oriData, localtimeContext);
    if (processedData.empty()) {
        ERROR("Format NpuMem data error, dbPath is %.", dbPath);
        return false;
    }
    allProcessedData.insert(allProcessedData.end(), processedData.begin(), processedData.end());
    return true;
}

OriNpuMemData NpuMemProcessor::LoadData(const DBInfo &npuMemDB)
{
    OriNpuMemData oriData;
    std::string sql{"SELECT event, ddr, hbm, timestamp, memory FROM " + npuMemDB.tableName};
    if (!npuMemDB.dbRunner->QueryData(sql, oriData)) {
        ERROR("Failed to obtain data from the % table.", npuMemDB.tableName);
    }
    return oriData;
}

std::vector<NpuMemData> NpuMemProcessor::FormatData(const OriNpuMemData &oriData,
                                                    const LocaltimeContext &localtimeContext)
{
    std::vector<NpuMemData> formatData;
    if (!Reserve(formatData, oriData.size())) {
        ERROR("Reserve for NpuMem data failed, profPath is %, deviceId is %.", profPath_, localtimeContext.deviceId);
        return formatData;
    }
    NpuMemData tempData;
    double oriTimestamp;
    tempData.deviceId = localtimeContext.deviceId;
    for (const auto &row: oriData) {
        std::tie(tempData.event, tempData.ddr, tempData.hbm, oriTimestamp, tempData.memory) = row;
        // 原数据结果还未加device monotonic，无需减去, 且原始时间单位为us
        HPFloat timestamp{GetTimeBySamplingTimestamp(oriTimestamp * MILLI_SECOND, localtimeContext.hostMonotonic, 0)};
        tempData.localTime = GetLocalTime(timestamp, localtimeContext.timeRecord).Uint64();
        formatData.push_back(tempData);
    }
    return formatData;
}
}
}