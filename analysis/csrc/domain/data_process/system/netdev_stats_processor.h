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
#ifndef ANALYSIS_DOMAIN_NETDEV_STATS_PROCESSOR_H
#define ANALYSIS_DOMAIN_NETDEV_STATS_PROCESSOR_H

#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/netdev_stats_data.h"
#include "analysis/csrc/infrastructure/utils/time_utils.h"

namespace Analysis {
namespace Domain {
using namespace Analysis::Utils;
// timestamp, mac_tx_pfc_pkt, mac_rx_pfc_pkt, mac_tx_total_oct, mac_rx_total_oct,
// mac_tx_bad_oct, mac_rx_bad_oct, roce_tx_all_pkt, roce_rx_all_pkt, roce_tx_err_pkt, roce_rx_err_pkt,
// roce_tx_cnp_pkt, roce_rx_cnp_pkt, roce_new_pkt_rty, nic_tx_all_oct, nic_rx_all_oct
using OriNetDevStatsData = std::tuple<uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                                      uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                                      uint64_t, uint64_t, uint64_t, uint64_t, uint64_t>;

// 该类用于处理netdev_stats.db中NetDevStatsOriginalData表
class NetDevStatsProcessor : public DataProcessor {
public:
    NetDevStatsProcessor() = default;
    explicit NetDevStatsProcessor(const std::string &profPath);

private:
    bool Process(DataInventory& dataInventory) override;
    bool ProcessSingleDevice(const std::string &devicePath, std::vector<NetDevStatsEventData> &allEventsData,
                             const ProfTimeRecord &record);
    std::vector<NetDevStatsEventData> ProcessData(const DBInfo &netDevStatsDB, uint16_t deviceId,
                                                  const ProfTimeRecord &record);
    std::vector<NetDevStatsEventData> ComputeEventData(const std::vector<OriNetDevStatsData> &oriData,
                                                       uint16_t deviceId, const ProfTimeRecord &record);
};
} // namespace Domain
} // namespace Analysis

#endif // ANALYSIS_DOMAIN_NETDEV_STATS_PROCESSOR_H
