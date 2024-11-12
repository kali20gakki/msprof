/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : soc_bandwidth_processor.h
 * Description        : soc_bandwidth_processor，处理InterSoc表数据
 * Author             : msprof team
 * Creation Date      : 2024/08/20
 * *****************************************************************************
 */
#ifndef ANALYSIS_DOMAIN_SOC_BANDWIDTH_PROCESSOR_H
#define ANALYSIS_DOMAIN_SOC_BANDWIDTH_PROCESSOR_H

#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/soc_bandwidth_data.h"

namespace Analysis {
namespace Domain {
// l2_buffer_bw_level, mata_bw_level, sys_time
using OriSocDataFormat = std::vector<std::tuple<uint32_t, uint32_t, double>>;

// 该类用于生成SOC_BANDWIDTH_LEVEL表
class SocBandwidthProcessor : public DataProcessor {
public:
    SocBandwidthProcessor() = default;
    explicit SocBandwidthProcessor(const std::string& profPaths);
private:
    bool Process(DataInventory& dataInventory) override;
    bool ProcessSingleDevice(const std::string& devicePath, Utils::ProfTimeRecord& timeRecord,
                             std::vector<SocBandwidthData>& res);
    OriSocDataFormat LoadData(const DBInfo& socProfilerDB, const std::string& dbPath);
    std::vector<SocBandwidthData> FormatData(const OriSocDataFormat& oriData,
                                             const Utils::ProfTimeRecord& timeRecord,
                                             const uint16_t deviceId);
};
} // Domain
} // Analysis

#endif // ANALYSIS_DOMAIN_SOC_BANDWIDTH_PROCESSOR_H
