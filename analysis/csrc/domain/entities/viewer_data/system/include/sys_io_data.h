/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : sys_io_data.h
 * Description        : sys_io_data，处理NIC, RoCE表相关数据
 * Author             : msprof team
 * Creation Date      : 2024/8/10
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_SYS_IO_FORMAT_PROCESSOR_H
#define ANALYSIS_DOMAIN_SYS_IO_FORMAT_PROCESSOR_H

#include <string>
#include <vector>
#include "analysis/csrc/domain/entities/viewer_data/basic_data.h"

namespace Analysis {
namespace Domain {
struct SysIOOriginalData : public BasicData {
    uint16_t deviceId = UINT16_MAX;
    uint16_t funcId = UINT16_MAX; // 功能号，又名func_id（如果一个device有多个端口的时候，区分不同端口使用）
    uint32_t rxPackets = 0; // 累计收包数量(Packet)
    uint32_t rxBytes = 0; // 累计接收字节量(Byte)
    uint32_t rxErrors = 0; // 接收错误数量(Packet)
    uint32_t rxDropped = 0; // 接收丢包量(Packet)
    uint32_t txPackets = 0; // 累计发包数量(Packet)
    uint32_t txBytes = 0; // 累计发包字节量(Byte)
    uint32_t txErrors = 0; // 发送错误量(Packet)
    uint32_t txDropped = 0; // 发送丢包量(Packet)
    double txPacketRate = 0.0; // 发包速率(Packet/s)
    double txByteRate = 0.0; // 发送字节速率(Byte/s)
    double rxPacketRate = 0.0; // 收包速率(Packet/s)
    double rxByteRate = 0.0; // 接收字节速率(Byte/s)
    uint64_t bandwidth = 0; // 带宽(B/s)
};

struct SysIOReportData : public BasicData {
    uint16_t deviceId = UINT16_MAX;
    uint16_t funcId = UINT16_MAX;
    double rxBandwidthEfficiency = 0.0; // 接收包带宽利用率(%)
    double rxPacketRate = 0.0; // 每秒接收包速率(Packet/s)
    double rxErrorRate = 0.0; // 接收包错误率(%)
    double rxDroppedRate = 0.0; // 接收包丢包率(%)
    double txBandwidthEfficiency = 0.0; // 接收包带宽利用率(%)
    double txPacketRate = 0.0; // 每秒接收包速率(Packet/s)
    double txErrorRate = 0.0; // 接收包错误率(%)
    double txDroppedRate = 0.0; // 接收包丢包率(%)
    uint64_t bandwidth = 0; // 带宽(MB/s)
};

struct SysIOReceiveSendData : public BasicData {
    uint16_t deviceId = UINT16_MAX;
    uint16_t funcId = UINT16_MAX;
    double rxBandwidthEfficiency = 0.0; // 接收包带宽利用率(%)
    double rxPacketRate = 0.0; // 每秒接收包速率(Packet/s)
    double rxErrorRate = 0.0; // 接收包错误率(%)
    double rxDroppedRate = 0.0; // 接收包丢包率(%)
    double txBandwidthEfficiency = 0.0; // 接收包带宽利用率(%)
    double txPacketRate = 0.0; // 每秒接收包速率(Packet/s)
    double txErrorRate = 0.0; // 接收包错误率(%)
    double txDroppedRate = 0.0; // 接收包丢包率(%)
};

struct NicOriginalData {
    std::vector<SysIOOriginalData> sysIOOriginalData;
};

struct NicReportData {
    std::vector<SysIOReportData> sysIOReportData;
};

struct NicReceiveSendData {
    std::vector<SysIOReceiveSendData> sysIOReceiveSendData;
};

struct RoceOriginalData {
    std::vector<SysIOOriginalData> sysIOOriginalData;
};

struct RoceReportData {
    std::vector<SysIOReportData> sysIOReportData;
};

struct RoceReceiveSendData {
    std::vector<SysIOReceiveSendData> sysIOReceiveSendData;
};
}
}

#endif // ANALYSIS_DOMAIN_SYS_IO_FORMAT_PROCESSOR_H
