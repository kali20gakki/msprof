/* -------------------------------------------------------------------------
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/

#ifndef ANALYSIS_DOMAIN_NETDEV_STATS_DATA_H
#define ANALYSIS_DOMAIN_NETDEV_STATS_DATA_H

#include "analysis/csrc/domain/entities/viewer_data/basic_data.h"

namespace Analysis {
namespace Domain {
struct NetDevStatsData : public BasicData {
    uint16_t deviceId = UINT16_MAX;
    uint64_t macTxPfcPkt = 0;       // MAC发送PFC帧数
    uint64_t macRxPfcPkt = 0;       // MAC接收PFC帧数
    uint64_t macTxTotalOct = 0;     // MAC发送字节数
    uint64_t macRxTotalOct = 0;     // MAC接收字节数
    uint64_t macTxBadOct = 0;       // MAC发送错误字节数
    uint64_t macRxBadOct = 0;       // MAC接收错误字节数
    uint64_t roceTxAllPkt = 0;      // RoCE发送总包数
    uint64_t roceRxAllPkt = 0;      // RoCE接收总包数
    uint64_t roceTxErrPkt = 0;      // RoCE发送错误包数
    uint64_t roceRxErrPkt = 0;      // RoCE接收错误包数
    uint64_t roceTxCnpPkt = 0;      // RoCE发送Cnp包数
    uint64_t roceRxCnpPkt = 0;      // RoCE接收Cnp包数
    uint64_t roceNewPktRty = 0;     // RoCE发送的超次重传数
    uint64_t nicTxAllOct = 0;       // NIC发送字节数
    uint64_t nicRxAllOct = 0;       // NIC接收字节数
};

struct NetDevStatsEventData {
    uint16_t deviceId = UINT16_MAX;
    uint64_t timestamp = 0;         // 时间戳
    uint64_t macTxPfcPkt = 0;       // MAC发送PFC帧数
    uint64_t macRxPfcPkt = 0;       // MAC接收PFC帧数
    uint64_t macTxByte = 0;         // MAC发送字节数
    double macTxBandwidth = 0;      // MAC发送数据带宽
    uint64_t macRxByte = 0;         // MAC接收字节数
    double macRxBandwidth = 0;      // MAC接收数据带宽
    uint64_t macTxBadByte = 0;      // MAC发送错误字节数
    uint64_t macRxBadByte = 0;      // MAC接收错误字节数
    uint64_t roceTxPkt = 0;         // RoCE发送总报文数
    uint64_t roceRxPkt = 0;         // RoCE接收总报文数
    uint64_t roceTxErrPkt = 0;      // RoCE发送错误报文数
    uint64_t roceRxErrPkt = 0;      // RoCE接收错误报文数
    uint64_t roceTxCnpPkt = 0;      // RoCE发送Cnp报文数
    uint64_t roceRxCnpPkt = 0;      // RoCE接收Cnp报文数
    uint64_t roceNewPktRty = 0;     // RoCE发送的超次重传数
    uint64_t nicTxByte = 0;         // NIC发送字节数
    double nicTxBandwidth = 0;      // NIC发送数据带宽
    uint64_t nicRxByte = 0;         // NIC接收字节数
    double nicRxBandwidth = 0;      // NIC接收数据带宽
};
} // namespace Domain
} // namespace Analysis

#endif // ANALYSIS_DOMAIN_NETDEV_STATS_DATA_H
