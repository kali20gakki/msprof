/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : sys_io_processor.cpp
 * Description        : 处理sys io相关数据，当前仅包括 NIC和RoCE
 * Author             : msprof team
 * Creation Date      : 2024/2/21
 * *****************************************************************************
 */
#include "analysis/csrc/domain/data_process/system/sys_io_processor.h"
#include "analysis/csrc/parser/environment/context.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"

namespace Analysis {
namespace Domain {
using namespace Analysis::Parser::Environment;
using namespace Analysis::Utils;

namespace {
const std::unordered_map<std::string, std::tuple<std::string, std::string, std::string>> ORI_DB_INFO_TABLE = {
    {PROCESSOR_NAME_NIC, std::make_tuple("nic.db", "NicOriginalData", "NicReportData")},
    {PROCESSOR_NAME_ROCE, std::make_tuple("roce.db", "RoceOriginalData", "RoceReportData")},
};

const std::unordered_map<std::string, std::tuple<std::string, std::string>> RECEIVE_SEND_DB_INFO_TABLE = {
    {PROCESSOR_NAME_NIC_TIMELINE, std::make_tuple("nicreceivesend_table.db", "NicReceiveSend")},
    {PROCESSOR_NAME_ROCE_TIMELINE, std::make_tuple("rocereceivesend_table.db", "RoceReceiveSend")},
};
}

SysIOProcessor::SysIOProcessor(const std::string &profPath, const std::string &processorName)
    : DataProcessor(profPath), processorName_(processorName) {}

bool SysIOProcessor::Process(DataInventory &dataInventory)
{
    LocaltimeContext localtimeContext;
    if (!Context::GetInstance().GetProfTimeRecordInfo(localtimeContext.timeRecord, profPath_)) {
        ERROR("Failed to obtain the time in start_info and end_info, profPath is %", profPath_);
        return false;
    }
    bool flag = true;
    std::vector<SysIOOriginalData> allProcessedData;
    std::vector<SysIOReportData> allSummaryData;
    auto deviceList = File::GetFilesWithPrefix(profPath_, DEVICE_PREFIX);
    for (const auto& devicePath: deviceList) {
        localtimeContext.deviceId = GetDeviceIdByDevicePath(devicePath);
        flag = ProcessSingleDevice(devicePath, localtimeContext, allProcessedData, allSummaryData) && flag;
    }
    if (processorName_ == PROCESSOR_NAME_NIC) {
        std::vector<NicOriginalData> allNicOriginalData;
        NicOriginalData nicOriginalData;
        nicOriginalData.sysIOOriginalData = std::move(allProcessedData);
        allNicOriginalData.push_back(nicOriginalData);
        std::vector<NicReportData> allNicReportData;
        NicReportData nicReportData;
        nicReportData.sysIOReportData = std::move(allSummaryData);
        allNicReportData.push_back(nicReportData);
        if (!SaveToDataInventory<NicOriginalData>(std::move(allNicOriginalData), dataInventory, processorName_) ||
            !SaveToDataInventory<SysIOReportData>(std::move(allSummaryData), dataInventory, processorName_)) {
                flag = false;
                ERROR("Save % Data To DataInventory failed, profPath is %", processorName_, profPath_);
        }
    } else {
        std::vector<RoceOriginalData> allRoceOriginalData;
        RoceOriginalData roceOriginalData;
        roceOriginalData.sysIOOriginalData = std::move(allProcessedData);
        allRoceOriginalData.push_back(roceOriginalData);
        std::vector<RoceReportData> allRoceReportData;
        RoceReportData roceReportData;
        roceReportData.sysIOReportData = std::move(allSummaryData);
        allRoceReportData.push_back(roceReportData);
        if (!SaveToDataInventory<RoceOriginalData>(std::move(allRoceOriginalData), dataInventory, processorName_) ||
            !SaveToDataInventory<RoceReportData>(std::move(allRoceReportData), dataInventory, processorName_)) {
                flag = false;
                ERROR("Save % Data To DataInventory failed, profPath is %", processorName_, profPath_);
        }
    }
    return flag;
}

bool SysIOProcessor::ProcessSingleDevice(const std::string &devicePath, LocaltimeContext &localtimeContext,
    std::vector<SysIOOriginalData> &allProcessedData, std::vector<SysIOReportData> &allSummaryData)
{
    std::string oriDbName;
    std::string oriTableName;
    std::string reportTableName;
    std::tie(oriDbName, oriTableName, reportTableName) = ORI_DB_INFO_TABLE.find(processorName_)->second;
    DBInfo sysIODB(oriDbName, oriTableName);
    if (localtimeContext.deviceId == Parser::Environment::HOST_ID) {
        ERROR("the invalid deviceId cannot to be identified, profPath is %", profPath_);
        return false;
    }
    std::string dbPath = File::PathJoin({devicePath, SQLITE, sysIODB.dbName});
    if (!sysIODB.ConstructDBRunner(dbPath) || sysIODB.dbRunner == nullptr) {
        ERROR("Create % connection failed.", dbPath);
        return false;
    }
    // 并不是所有场景都有sys io数据
    sysIODB.tableName = oriTableName;
    auto status = CheckPathAndTable(dbPath, sysIODB);
    if (status != CHECK_SUCCESS) {
        if (status == CHECK_FAILED) {
            return false;
        }
        return true;
    }
    auto processedData = ProcessData(sysIODB, localtimeContext);
    if (processedData.empty()) {
        ERROR("Process % OriginalData error, dbPath is %", processorName_, dbPath);
        return false;
    }
    sysIODB.tableName = reportTableName;
    status = CheckPathAndTable(dbPath, sysIODB);
    if (status != CHECK_SUCCESS) {
        if (status == CHECK_FAILED) {
            return false;
        }
        return true;
    }
    auto summaryData = ProcessSummaryData(sysIODB, localtimeContext);
    if (summaryData.empty()) {
        ERROR("Process % ReportData error, dbPath is %", processorName_, dbPath);
        return false;
    }
    allProcessedData.insert(allProcessedData.end(), processedData.begin(), processedData.end());
    allSummaryData.insert(allSummaryData.end(), summaryData.begin(), summaryData.end());
    return true;
}

std::vector<SysIOOriginalData> SysIOProcessor::ProcessData(const DBInfo &sysIODB, LocaltimeContext &localtimeContext)
{
    if (!Context::GetInstance().GetClockMonotonicRaw(localtimeContext.hostMonotonic, true, localtimeContext.deviceId,
                                                     profPath_) ||
        !Context::GetInstance().GetClockMonotonicRaw(localtimeContext.deviceMonotonic, false, localtimeContext.deviceId,
                                                     profPath_)) {
        ERROR("Device MonotonicRaw is invalid in path: %., device id is %", profPath_, localtimeContext.deviceId);
        return {};
    }
    OriSysIOData oriData = LoadData(sysIODB);
    if (oriData.empty()) {
        ERROR("Get % data failed, profPath is %, device is %", sysIODB.tableName, profPath_, localtimeContext.deviceId);
        return {};
    }
    return FormatData(oriData, localtimeContext);
}

std::vector<SysIOReportData> SysIOProcessor::ProcessSummaryData(const DBInfo &sysIODB,
                                                                const LocaltimeContext &localtimeContext)
{
    SysIOReportData tempData;
    OriSysIOReportData reportData;
    std::vector<SysIOReportData> processedData;
    std::string sql{"SELECT duration, bandwidth, rxbandwidth, rxpacket, rxerrorrate, rxdroppedrate, "
                    "txbandwidth, txpacket, txerrorrate, txdroppedrate, funcid FROM " + sysIODB.tableName};
    if (!sysIODB.dbRunner->QueryData(sql, reportData) || reportData.empty()) {
        ERROR("Failed to obtain throughput data from the % table, profPath is %, deviceId is %.",
              sysIODB.tableName, profPath_, localtimeContext.deviceId);
        return {};
    }
    tempData.deviceId = localtimeContext.deviceId;
    double duration;
    for (const auto& row : reportData) {
        std::tie(duration, tempData.bandwidth, tempData.rxBandwidthEfficiency,
                 tempData.rxPacketRate, tempData.rxErrorRate, tempData.rxDroppedRate,
                 tempData.txBandwidthEfficiency, tempData.txPacketRate,
                 tempData.txErrorRate, tempData.txDroppedRate, tempData.funcId) = row;
        HPFloat timestamp = HPFloat(duration);
        tempData.localTime = GetLocalTime(timestamp, localtimeContext.timeRecord).Uint64();
        processedData.push_back(tempData);
    }
    return processedData;
}

OriSysIOData SysIOProcessor::LoadData(const DBInfo &sysIODB)
{
    OriSysIOData oriData;
    std::string sql = "SELECT timestamp, bandwidth, rxpacket, rxbyte, rxpackets, rxbytes, rxerrors, "
                      "rxdropped, txpacket, txbyte, txpackets, txbytes, txerrors, txdropped, funcid "
                      "FROM " + sysIODB.tableName;
    if (!sysIODB.dbRunner->QueryData(sql, oriData)) {
        ERROR("Failed to obtain data from the % table.", sysIODB.tableName);
    }
    return oriData;
}

std::vector<SysIOOriginalData> SysIOProcessor::FormatData(const OriSysIOData &oriData,
                                                          const LocaltimeContext &localtimeContext)
{
    std::vector<SysIOOriginalData> formatData;
    if (!Reserve(formatData, oriData.size())) {
        ERROR("Reserve for % data failed, profPath is %, deviceId is %.",
              processorName_, profPath_, localtimeContext.deviceId);
        return formatData;
    }
    SysIOOriginalData tempData;
    tempData.deviceId = localtimeContext.deviceId;
    double oriTimestamp;
    for (const auto& row : oriData) {
        std::tie(oriTimestamp, tempData.bandwidth, tempData.rxPacketRate, tempData.rxByteRate,
                 tempData.rxPackets, tempData.rxBytes, tempData.rxErrors, tempData.rxDropped,
                 tempData.txPacketRate, tempData.txByteRate, tempData.txPackets, tempData.txBytes,
                 tempData.txErrors, tempData.txDropped, tempData.funcId) = row;
        HPFloat timestamp = GetTimeBySamplingTimestamp(oriTimestamp, localtimeContext.hostMonotonic,
                                                       localtimeContext.deviceMonotonic);
        tempData.localTime = GetLocalTime(timestamp, localtimeContext.timeRecord).Uint64();
        tempData.bandwidth = tempData.bandwidth * BYTE_SIZE * BYTE_SIZE, // MB/s -> B/s
        formatData.push_back(tempData);
    }
    return formatData;
}

NicProcessor::NicProcessor(const std::string &profPath)
    : SysIOProcessor(profPath, PROCESSOR_NAME_NIC) {}

RoCEProcessor::RoCEProcessor(const std::string &profPath)
    : SysIOProcessor(profPath, PROCESSOR_NAME_ROCE) {}

SysIOTimelineProcessor::SysIOTimelineProcessor(const std::string &profPath, const std::string &processorName)
    : DataProcessor(profPath), processorName_(processorName) {}

bool SysIOTimelineProcessor::Process(DataInventory &dataInventory)
{
    LocaltimeContext localtimeContext;
    if (!Context::GetInstance().GetProfTimeRecordInfo(localtimeContext.timeRecord, profPath_)) {
        ERROR("Failed to obtain the time in start_info and end_info, profPath is %.", profPath_);
        return false;
    }
    bool flag = true;
    std::vector<SysIOReceiveSendData> allProcessedData;
    auto deviceList = File::GetFilesWithPrefix(profPath_, DEVICE_PREFIX);
    for (const auto& devicePath: deviceList) {
        localtimeContext.deviceId= GetDeviceIdByDevicePath(devicePath);
        flag = ProcessSingleDevice(devicePath, localtimeContext, allProcessedData) && flag;
    }
    if (processorName_ == PROCESSOR_NAME_NIC_TIMELINE) {
        std::vector<NicReceiveSendData> allNicReceiveSendData;
        NicReceiveSendData nicReceiveSendData;
        nicReceiveSendData.sysIOReceiveSendData = std::move(allProcessedData);
        allNicReceiveSendData.push_back(nicReceiveSendData);
        if (!SaveToDataInventory<NicReceiveSendData>(std::move(allNicReceiveSendData), dataInventory, processorName_)) {
                flag = false;
                ERROR("Save % Data To DataInventory failed, profPath is %", processorName_, profPath_);
        }
    } else {
        std::vector<RoceReceiveSendData> allRoceReceiveSendData;
        RoceReceiveSendData roceReceiveSendData;
        roceReceiveSendData.sysIOReceiveSendData = std::move(allProcessedData);
        allRoceReceiveSendData.push_back(roceReceiveSendData);
        if (!SaveToDataInventory<RoceReceiveSendData>(std::move(allRoceReceiveSendData),
                                                      dataInventory, processorName_)) {
                flag = false;
                ERROR("Save % Data To DataInventory failed, profPath is %", processorName_, profPath_);
        }
    }
    return flag;
}

bool SysIOTimelineProcessor::ProcessSingleDevice(const std::string &devicePath, LocaltimeContext &localtimeContext,
                                                 std::vector<SysIOReceiveSendData> &allProcessedData)
{
    std::string oriDbName;
    std::string oriTableName;
    std::tie(oriDbName, oriTableName) = RECEIVE_SEND_DB_INFO_TABLE.find(processorName_)->second;
    DBInfo sysIOReceiveSendDB(oriDbName, oriTableName);
    if (!Context::GetInstance().GetClockMonotonicRaw(localtimeContext.hostMonotonic, true, localtimeContext.deviceId,
                                                     profPath_) ||
        !Context::GetInstance().GetClockMonotonicRaw(localtimeContext.deviceMonotonic, false, localtimeContext.deviceId,
                                                     profPath_)) {
        ERROR("Device MonotonicRaw is invalid in path: %., device id is %", profPath_, localtimeContext.deviceId);
        return false;
    }
    std::string dbPath = File::PathJoin({devicePath, SQLITE, sysIOReceiveSendDB.dbName});
    if (!sysIOReceiveSendDB.ConstructDBRunner(dbPath) || sysIOReceiveSendDB.dbRunner == nullptr) {
        ERROR("Create % connection failed.", dbPath);
        return false;
    }
    // 并不是所有场景都有sys io数据
    auto status = CheckPathAndTable(dbPath, sysIOReceiveSendDB);
    if (status != CHECK_SUCCESS) {
        if (status == CHECK_FAILED) {
            return false;
        }
        return true;
    }
    OriSysIOReceiveSendData oriData = LoadData(sysIOReceiveSendDB);
    if (oriData.empty()) {
        ERROR("Load % ReceiveSendData error, dbPath is %.", processorName_, dbPath);
        return false;
    }
    auto processedData = FormatData(oriData, localtimeContext);
    if (processedData.empty()) {
        ERROR("Format % ReceiveSendData error, dbPath is %.", processorName_, dbPath);
        return false;
    }
    allProcessedData.insert(allProcessedData.end(), processedData.begin(), processedData.end());
    return true;
}

OriSysIOReceiveSendData SysIOTimelineProcessor::LoadData(const DBInfo &sysIOReceiveSendDB)
{
    OriSysIOReceiveSendData oriData;
    std::string sql{"SELECT timestamp, rx_bandwidth_efficiency, rx_packets, rx_error_rate, rx_dropped_rate, "
                    "tx_bandwidth_efficiency, tx_packets, tx_error_rate, tx_dropped_rate, func_id FROM " +
                    sysIOReceiveSendDB.tableName};
    if (!sysIOReceiveSendDB.dbRunner->QueryData(sql, oriData)) {
        ERROR("Failed to obtain data from the % table.", sysIOReceiveSendDB.tableName);
    }
    return oriData;
}

std::vector<SysIOReceiveSendData> SysIOTimelineProcessor::FormatData(const OriSysIOReceiveSendData &oriData,
                                                                     const LocaltimeContext &localtimeContext)
{
    std::vector<SysIOReceiveSendData> formatData;
    if (!Reserve(formatData, oriData.size())) {
        ERROR("Reserve for Hccs data failed, profPath is %, deviceId is %.", profPath_, localtimeContext.deviceId);
        return formatData;
    }
    SysIOReceiveSendData tempData;
    tempData.deviceId = localtimeContext.deviceId;
    double oriTimestamp;
    for (const auto &row: oriData) {
        std::tie(oriTimestamp, tempData.rxBandwidthEfficiency, tempData.rxPacketRate, tempData.rxErrorRate,
                 tempData.rxDroppedRate, tempData.txBandwidthEfficiency, tempData.txPacketRate, tempData.txErrorRate,
                 tempData.txDroppedRate, tempData.funcId) = row;
        HPFloat timestamp = GetTimeBySamplingTimestamp(oriTimestamp, localtimeContext.hostMonotonic,
                                                       localtimeContext.deviceMonotonic);
        tempData.localTime = GetLocalTime(timestamp, localtimeContext.timeRecord).Uint64();
        formatData.push_back(tempData);
    }
    return formatData;
}

NicTimelineProcessor::NicTimelineProcessor(const std::string &profPath)
    : SysIOTimelineProcessor(profPath, PROCESSOR_NAME_NIC_TIMELINE) {}

RoCETimelineProcessor::RoCETimelineProcessor(const std::string &profPath)
    : SysIOTimelineProcessor(profPath, PROCESSOR_NAME_ROCE_TIMELINE) {}

} // Doamin
} // Analysis
