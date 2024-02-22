/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : sys_io_processor.cpp
 * Description        : 处理sys io相关数据，当前仅包括 NIC和RoCE
 * Author             : msprof team
 * Creation Date      : 2024/2/21
 * *****************************************************************************
 */
#include "analysis/csrc/viewer/database/finals/sys_io_processor.h"

#include "analysis/csrc/dfx/error_code.h"
#include "analysis/csrc/parser/environment/context.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"

namespace Analysis {
namespace Viewer {
namespace Database {
using Context = Parser::Environment::Context;

namespace {
struct SysIoData {
    uint16_t device_id = UINT16_MAX;
    uint16_t funcid = UINT16_MAX;
    double timestamp = 0.0;
    double rxpacket = 0.0;
    double rxbyte = 0.0;
    double txpacket = 0.0;
    double txbyte = 0.0;
    uint32_t bandwidth = UINT32_MAX;
    uint32_t rxpackets = UINT32_MAX;
    uint32_t rxbytes = UINT32_MAX;
    uint32_t rxerrors = UINT32_MAX;
    uint32_t rxdropped = UINT32_MAX;
    uint32_t txpackets = UINT32_MAX;
    uint32_t txbytes = UINT32_MAX;
    uint32_t txerrors = UINT32_MAX;
    uint32_t txdropped = UINT32_MAX;
};

const std::unordered_map<std::string, std::tuple<std::string, std::string, std::string>> ORI_DB_INFO_TABLE = {
    {PROCESSOR_NAME_NIC, std::make_tuple("nic.db", "NicOriginalData", TABLE_NAME_NIC)},
    {PROCESSOR_NAME_ROCE, std::make_tuple("roce.db", "RoceOriginalData", TABLE_NAME_ROCE)},
};
}

SysIoProcessor::SysIoProcessor(const std::string &reportDBPath, const std::set<std::string> &profPaths,
                               const std::string &processorName)
    : TableProcessor(reportDBPath, profPaths), processorName_(processorName) {}

bool SysIoProcessor::Run()
{
    INFO("% Run.", processorName_);
    bool flag = TableProcessor::Run();
    PrintProcessorResult(flag, processorName_);
    return flag;
}

bool SysIoProcessor::Process(const std::string &fileDir)
{
    INFO("% Process, dir is %", processorName_, fileDir);
    auto deviceList = Utils::File::GetFilesWithPrefix(fileDir, DEVICE_PREFIX);
    std::string oriDbName;
    std::string oriTableName;
    std::string targetTableName;
    std::tie(oriDbName, oriTableName, targetTableName) = ORI_DB_INFO_TABLE.find(processorName_)->second;
    DBInfo sysIoDB(oriDbName, oriTableName);
    bool flag = true;
    Utils::ProfTimeRecord timeRecord;
    bool timeFlag = Context::GetInstance().GetProfTimeRecordInfo(timeRecord, fileDir);
    for (const auto& devicePath: deviceList) {
        std::string dbPath = Utils::File::PathJoin({devicePath, SQLITE, sysIoDB.dbName});
        // 并不是所有场景都有sys io数据
        auto status = CheckPath(dbPath);
        if (status != CHECK_SUCCESS) {
            if (status == CHECK_FAILED) {
                flag = false;
            }
            continue;
        }
        if (!timeFlag) {
            ERROR("Failed to GetProfTimeRecordInfo, fileDir is %.", fileDir);
            return false;
        }
        SysIoDataFormat sysIoData = GetData(dbPath, sysIoDB);
        ProcessedDataFormat processedData;
        if (!FormatData(fileDir, timeRecord, sysIoData, processedData)) {
            ERROR("FormatData failed, fileDir is %.", fileDir);
            flag = false;
            continue;
        }
        if (!SaveData(processedData, targetTableName)) {
            flag = false;
            ERROR("Save data failed, %.", dbPath);
            continue;
        }
    }
    return flag;
}

SysIoProcessor::SysIoDataFormat SysIoProcessor::GetData(const std::string &dbPath, DBInfo &sysIoDB) const
{
    INFO("% GetData, dir is %", processorName_, dbPath);
    SysIoDataFormat sysIoData;
    MAKE_SHARED_RETURN_VALUE(sysIoDB.dbRunner, DBRunner, sysIoData, dbPath);
    if (sysIoDB.dbRunner == nullptr) {
        ERROR("Create % connection failed.", dbPath);
        return sysIoData;
    }
    std::string sql = "SELECT device_id, timestamp, bandwidth, rxpacket, rxpackets, rxbytes, rxerrors, rxdropped "
                      "txpacket, txbyte, txpackets, txbytes, txerrors, txdropped, funcid "
                      "FROM " + sysIoDB.tableName;
    if (!sysIoDB.dbRunner->QueryData(sql, sysIoData)) {
        ERROR("Query api data failed, db path is %.", dbPath);
        return sysIoData;
    }
    return sysIoData;
}

bool SysIoProcessor::FormatData(const std::string &fileDir, const Utils::ProfTimeRecord timeRecord,
                                const SysIoDataFormat &sysIoData, ProcessedDataFormat &processedData)
{
    INFO("% FormatData, dir is %", processorName_, fileDir);
    Utils::SyscntConversionParams params;
    Utils::ProfTimeRecord record;
    if (sysIoData.empty()) {
        ERROR("Sys io original data is empty, processor name is %.", processorName_);
        return false;
    }
    if (!Utils::Reserve(processedData, sysIoData.size())) {
        ERROR("Reserve for % data failed.", processorName_);
        return false;
    }
    SysIoData tempData;
    for (const auto& data : sysIoData) {
        std::tie(tempData.device_id, tempData.timestamp, tempData.bandwidth, tempData.rxpacket, tempData.rxbyte,
                 tempData.rxpackets, tempData.rxbytes, tempData.rxerrors, tempData.rxdropped,
                 tempData.txpacket, tempData.txbyte, tempData.txpackets, tempData.txbytes,
                 tempData.txerrors, tempData.txdropped, tempData.funcid) = data;
        Utils::HPFloat timestamp{tempData.timestamp};
        processedData.emplace_back(tempData.device_id, Utils::GetLocalTime(timestamp, timeRecord).Str(),
                                   tempData.bandwidth, tempData.rxpacket, tempData.rxbyte, tempData.rxpackets,
                                   tempData.rxbytes, tempData.rxerrors, tempData.rxdropped,
                                   tempData.txpacket, tempData.txbyte, tempData.txpackets, tempData.txbytes,
                                   tempData.txerrors, tempData.txdropped, tempData.funcid);
    }
    if (processedData.empty()) {
        ERROR("% data processing error.", processorName_);
        return false;
    }
    return true;
}

NicProcessor::NicProcessor(const std::string &reportDBPath, const std::set<std::string> &profPaths)
    : SysIoProcessor(reportDBPath, profPaths, PROCESSOR_NAME_NIC) {}

RoCEProcessor::RoCEProcessor(const std::string &reportDBPath, const std::set<std::string> &profPaths)
    : SysIoProcessor(reportDBPath, profPaths, PROCESSOR_NAME_ROCE) {}


} // Database
} // Viewer
} // Analysis
