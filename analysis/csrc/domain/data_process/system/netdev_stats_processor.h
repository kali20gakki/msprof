/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2025-2025
            Copyright, 2025, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : netdev_stats_processor.h
 * Description        : netdev_stats_processor处理netdev_stats表相关数据
 * Author             : msprof team
 * Creation Date      : 2025/5/14
 * *****************************************************************************
 */
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
} // namepace Domain
} // namspace Analysis

#endif // ANALYSIS_DOMAIN_NETDEV_STATS_PROCESSOR_H
