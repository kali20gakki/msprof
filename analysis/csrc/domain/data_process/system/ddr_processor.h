/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
******************************************************************************
*/
/* ******************************************************************************
 * File Name          : ddr_processor.h
 * Description        : ddr_processor，处理ddr表相关数据
 * Author             : msprof team
 * Creation Date      : 2024/08/09
 * *****************************************************************************
 */
#ifndef ANALYSIS_DOMAIN_DDR_PROCESSOR_H
#define ANALYSIS_DOMAIN_DDR_PROCESSOR_H

#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/ddr_data.h"

namespace Analysis {
namespace Domain {
using namespace Analysis::Utils;
// device_id, timestamp, flux_read, flux_write
using OriDataFormat = std::vector<std::tuple<uint32_t, double, double, double>>;

// 该类用于定义处理ddr.db中DDRMetricData表
class DDRProcessor : public DataProcessor {
public:
    DDRProcessor() = default;
    explicit DDRProcessor(const std::string& profPaths);

private:
    bool Process(DataInventory& dataInventory) override;
    bool ProcessOneDevice(const std::string& devicePath, std::vector<DDRData>& res);
    OriDataFormat LoadData(const DBInfo& DDRDB);
    std::vector<DDRData> FormatData(const OriDataFormat& oriData, const LocaltimeContext& localtimeContext);
};
} // namespace Domain
} // namespace Analysis

#endif // ANALYSIS_DOMAIN_DDR_PROCESSOR_H
