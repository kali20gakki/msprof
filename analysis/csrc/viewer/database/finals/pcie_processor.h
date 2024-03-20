/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : pcie_processor.h
 * Description        : 处理pcie数据
 * Author             : msprof team
 * Creation Date      : 2024/03/02
 * *****************************************************************************
 */

#ifndef ANALYSIS_VIEWER_DATABASE_PCIE_PROCESSOR_H
#define ANALYSIS_VIEWER_DATABASE_PCIE_PROCESSOR_H

#include "analysis/csrc/viewer/database/finals/table_processor.h"

namespace Analysis {
namespace Viewer {
namespace Database {
// 该类用于处理pcie数据
class PCIeProcessor : public TableProcessor {
// device_id, timestamp, tx_p_bandwidth_min, tx_p_bandwidth_max, tx_p_bandwidth_avg, tx_np_bandwidth_min,
// tx_np_bandwidth_max, tx_np_bandwidth_avg, tx_cpl_bandwidth_min, tx_cpl_bandwidth_max, tx_cpl_bandwidth_avg,
// tx_np_lantency_min, tx_np_lantency_max, tx_np_lantency_avg, rx_p_bandwidth_min, rx_p_bandwidth_max,
// rx_p_bandwidth_avg, rx_np_bandwidth_min, rx_np_bandwidth_max, rx_np_bandwidth_avg, rx_cpl_bandwidth_min,
// rx_cpl_bandwidth_max, rx_cpl_bandwidth_avg
using PCIeDataFormat = std::vector<std::tuple<uint32_t, uint64_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t,
    uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t,
    uint32_t, uint32_t, uint32_t, uint32_t, uint32_t>>;
// deviceId, timestampNs, txPostMin, txPostMax, txPostAvg, txNonpostMin, txNonpostMax, txNonpostAvg,
// txCplMin, txCplMax, txCplAvg, txNonpostLatencyMin, txNonpostLatencyMax, txNonpostLatencyAvg,
// rxPostMin, rxPostMax, rxPostAvg, rxNonpostMin, rxNonpostMax, rxNonpostAvg, rxCplMin, rxCplMax, rxCplAvg
using ProcessedDataFormat = std::vector<std::tuple<uint16_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
    uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
    uint64_t, uint64_t, uint64_t, uint64_t, uint64_t>>;

public:
    PCIeProcessor() = default;
    PCIeProcessor(const std::string &reportDBPath, const std::set<std::string> &profPaths);
    bool Run() override;
protected:
    bool Process(const std::string &fileDir) override;
private:
    PCIeDataFormat GetData(const std::string &dbPath, DBInfo &pcieDB);
    bool FormatData(const ThreadData &threadData,
                    const PCIeDataFormat &pcieDB, ProcessedDataFormat &processedData);
};


} // Database
} // Viewer
} // Analysis

#endif // ANALYSIS_VIEWER_DATABASE_PCIE_PROCESSOR_H
