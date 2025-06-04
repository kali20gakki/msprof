/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2025-2025
            Copyright, 2025, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : op_statistic_processor.h
 * Description        : op_statistic_processor，处理OpReport表数据
 * Author             : msprof team
 * Creation Date      : 2025/5/29
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_OP_STATISTIC_PROCESSOR_H
#define ANALYSIS_DOMAIN_OP_STATISTIC_PROCESSOR_H

#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/op_statistic_data.h"


namespace Analysis {
namespace Domain {
// op_type, core_type, occurrences, total_time, min, avg, max, ratio
using OriOpCountDataFormat = std::vector<std::tuple<std::string, std::string, std::string,
    double, double, double, double, double>>;

class OpStatisticProcessor : public DataProcessor {
public:
    OpStatisticProcessor() = default;
    explicit OpStatisticProcessor(const std::string& profPaths);
private:
    bool Process(DataInventory& dataInventory) override;
    OriOpCountDataFormat LoadData(const DBInfo& dbInfo, const std::string& dbPath);
    std::vector<OpStatisticData> FormatData(const OriOpCountDataFormat& oriData, const uint16_t deviceId);
};
} // namespace Domain
} // namespace Analysis

#endif // ANALYSIS_DOMAIN_OP_STATISTIC_PROCESSOR_H
