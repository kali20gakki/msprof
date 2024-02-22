/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : sys_io_processor.h
 * Description        : sys_io_processor，处理NIC, RoCE表相关数据
 * Author             : msprof team
 * Creation Date      : 2024/2/21
 * *****************************************************************************
 */
#ifndef ANALYSIS_VIEWER_DATABASE_SYS_IO_PROCESSOR_H
#define ANALYSIS_VIEWER_DATABASE_SYS_IO_PROCESSOR_H

#include "analysis/csrc/utils/time_utils.h"
#include "analysis/csrc/viewer/database/finals/table_processor.h"

namespace Analysis {
namespace Viewer {
namespace Database {
// 该类用于定义处理SYS IO的相关数据：当前仅包括 NIC和RoCE
class SysIoProcessor : public TableProcessor {
// device_id, timestamp, bandwidth, rxpacket, rxbyte, rxpackets, rxbytes, rxerrors, rxdropped,
// txpacket, txbyte, txpackets, txbytes, txerrors, txdropped, funcid
using SysIoDataFormat = std::vector<std::tuple<uint32_t, double, uint32_t, double, double, uint32_t,
        uint32_t, uint32_t, uint32_t, double, double, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t>>;
// deviceId, timestamp, bandwidth, rxPacketRate, rxByteRate, rxPackets, rxBytes, rxErrors, rxDropped
// txPacketRate, txByteRate, txPackets, txBytes, txErrors, txDropped, funcId
using ProcessedDataFormat = std::vector<std::tuple<uint16_t, std::string, uint32_t, double, double, uint32_t,
        uint32_t, uint32_t, uint32_t, double, double, uint32_t, uint32_t, uint32_t, uint32_t, uint16_t>>;
public:
    SysIoProcessor() = default;
    SysIoProcessor(const std::string &reportDBPath, const std::set<std::string> &profPaths,
                   const std::string &processorName);
    bool Run() override;
protected:
    bool Process(const std::string &fileDir) override;
private:
    SysIoDataFormat GetData(const std::string &dbPath, DBInfo &sysIoDB) const;
    bool FormatData(const std::string &fileDir, const Utils::ProfTimeRecord timeRecord,
                    const SysIoDataFormat &sysIoData, ProcessedDataFormat &processedData);
    std::string processorName_;
};

// 考虑到NIC和RoCE的数据格式和处理流程在2024.2.21之前均相同,这里采用同一个父类来处理。
// 后续若有差异，则分别在各自processor中做子类自己的处理

// 处理NIC数据
class NicProcessor : public SysIoProcessor {
public:
    NicProcessor() = default;
    NicProcessor(const std::string &reportDBPath, const std::set<std::string> &profPaths);
};

// 处理RoCE数据
class RoCEProcessor : public SysIoProcessor {
public:
    RoCEProcessor() = default;
    RoCEProcessor(const std::string &reportDBPath, const std::set<std::string> &profPaths);
};


} // Database
} // Viewer
} // Analysis

#endif // ANALYSIS_VIEWER_DATABASE_SYS_IO_PROCESSOR_H
