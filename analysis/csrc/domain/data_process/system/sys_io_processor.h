/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : sys_io_processor.h
 * Description        : sys_io_processor，处理NIC, RoCE表相关数据
 * Author             : msprof team
 * Creation Date      : 2024/8/12
 * *****************************************************************************
 */
#ifndef ANALYSIS_DOMAIN_SYS_IO_PROCESSOR_H
#define ANALYSIS_DOMAIN_SYS_IO_PROCESSOR_H

#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/sys_io_data.h"
#include "analysis/csrc/utils/time_utils.h"

namespace Analysis {
namespace Domain {
using namespace Analysis::Utils;
// timestamp, bandwidth, rxpacket, rxbyte, rxpackets, rxbytes, rxerrors, rxdropped,
// txpacket, txbyte, txpackets, txbytes, txerrors, txdropped, funcid
using OriSysIOData = std::vector<std::tuple<double, uint32_t, double, double, uint32_t,
    uint32_t, uint32_t, uint32_t, double, double, uint32_t, uint32_t, uint32_t, uint32_t, uint16_t>>;

using OriSysIOReportData = std::vector<std::tuple<double, uint64_t, double, double, double,
    double, double, double, double, double, uint16_t>>;

// timestamp, rx_bandwidth_efficiency, rx_packets, rx_error_rate, rx_dropped_rate
// tx_bandwidth_efficiency, tx_packets, tx_error_rate, tx_dropped_rate, func_id
using OriSysIOReceiveSendData = std::vector<std::tuple<double, double, double, double, double,
    double, double, double, double, uint16_t>>;

// 该类用于定义处理SYS IO的相关数据：当前仅包括 NIC和RoCE
class SysIOProcessor : public DataProcessor {
public:
    SysIOProcessor() = default;
    explicit SysIOProcessor(const std::string &profPath, const std::string &processorName);

private:
    bool Process(DataInventory& dataInventory) override;
    bool ProcessSingleDevice(const std::string &devicePath,
        std::vector<SysIOOriginalData> &timelineData, std::vector<SysIOReportData> &summaryData);
    bool ProcessTimelineData(const DBInfo &sysIODB, LocaltimeContext &localtimeContext,
                             std::vector<SysIOOriginalData> &timelineData);
    bool ProcessSummaryData(const DBInfo &sysIODB, const LocaltimeContext &localtimeContext,
                            std::vector<SysIOReportData> &summaryData);
private:
    std::string processorName_;
};

// 考虑到NIC和RoCE的数据格式和处理流程在2024.2.21之前均相同,这里采用同一个父类来处理。
// 后续若有差异，则分别在各自processor中做子类自己的处理

// 处理NIC数据
class NicProcessor : public SysIOProcessor {
public:
    NicProcessor() = default;
    NicProcessor(const std::string &profPath);
};

// 处理RoCE数据
class RoCEProcessor : public SysIOProcessor {
public:
    RoCEProcessor() = default;
    RoCEProcessor(const std::string &profPath);
};

// 该类用于定义处理SYS IO的相关数据：当前仅包括 NIC和RoCE,与上面不同的是，该类处理的是rocereceivesend_table.db
class SysIOTimelineProcessor : public DataProcessor {
public:
    SysIOTimelineProcessor() = default;
    explicit SysIOTimelineProcessor(const std::string &profPath, const std::string &processorName);

private:
    bool Process(DataInventory& dataInventory) override;
    bool ProcessSingleDevice(const std::string &devicePath, std::vector<SysIOReceiveSendData> &allProcessedData);
    OriSysIOReceiveSendData LoadData(const DBInfo &sysIOReceiveSendDB);
    std::vector<SysIOReceiveSendData> FormatData(const OriSysIOReceiveSendData &oriData,
                                                 const LocaltimeContext &localtimeContext);

private:
    std::string processorName_;
};

// 处理NIC timeline数据
class NicTimelineProcessor : public SysIOTimelineProcessor {
public:
    NicTimelineProcessor() = default;
    NicTimelineProcessor(const std::string &profPath);
};

// 处理RoCE timeline数据
class RoCETimelineProcessor : public SysIOTimelineProcessor {
public:
    RoCETimelineProcessor() = default;
    RoCETimelineProcessor(const std::string &profPath);
};
} // Domain
} // Analysis

#endif // ANALYSIS_DOMAIN_SYS_IO_PROCESSOR_H
