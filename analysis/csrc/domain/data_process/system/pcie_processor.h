/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : pcie_processor.h
 * Description        : 处理pcie数据
 * Author             : msprof team
 * Creation Date      : 2024/08/13
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_PCIE_PROCESSOR_H
#define ANALYSIS_DOMAIN_PCIE_PROCESSOR_H

#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/pcie_data.h"

namespace Analysis {
namespace Domain {
// device_id, timestamp, tx_p_bandwidth_min, tx_p_bandwidth_max, tx_p_bandwidth_avg, tx_np_bandwidth_min,
// tx_np_bandwidth_max, tx_np_bandwidth_avg, tx_cpl_bandwidth_min, tx_cpl_bandwidth_max, tx_cpl_bandwidth_avg,
// tx_np_lantency_min, tx_np_lantency_max, tx_np_lantency_avg, rx_p_bandwidth_min, rx_p_bandwidth_max,
// rx_p_bandwidth_avg, rx_np_bandwidth_min, rx_np_bandwidth_max, rx_np_bandwidth_avg, rx_cpl_bandwidth_min,
// rx_cpl_bandwidth_max, rx_cpl_bandwidth_avg
using PCIeDataFormat = std::vector<std::tuple<uint16_t, uint64_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t,
    uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t,
    uint32_t, uint32_t, uint32_t, uint32_t, uint32_t>>;

// 该类用于处理pcie数据
class PCIeProcessor : public DataProcessor {
public:
    PCIeProcessor() = default;
    explicit PCIeProcessor(const std::string& profPaths);
private:
    bool Process(DataInventory& dataInventory) override;
    PCIeDataFormat LoadData(const std::string& dbPath, const DBInfo& pcieDB);
    bool FormatData(const Utils::LocaltimeContext& localtimeContext,
                    const PCIeDataFormat& pcieDB, std::vector<PCIeData>& processedData);
};
} // Domain
} // Analysis

#endif // ANALYSIS_DOMAIN_PCIE_PROCESSOR_H
