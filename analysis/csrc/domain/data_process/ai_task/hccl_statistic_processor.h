/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : hccl_statistic_processor.h
 * Description        : hccl_statistic_processor，处理HcclOpReport表数据
 * Author             : msprof team
 * Creation Date      : 2024/8/17
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_HCCL_STATISTIC_PROCESSOR_H
#define ANALYSIS_DOMAIN_HCCL_STATISTIC_PROCESSOR_H

#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/hccl_statistic_data.h"


namespace Analysis {
namespace Domain {
// op_type, occurrences, total_time, min, avg, max, ratio
using OriHcclDataFormat = std::vector<std::tuple<std::string, std::string, double, double, double, double, double>>;

class HcclStatisticProcessor : public DataProcessor {
public:
    HcclStatisticProcessor() = default;
    explicit HcclStatisticProcessor(const std::string& profPaths);
private:
    bool Process(DataInventory& dataInventory) override;
    OriHcclDataFormat LoadData(const DBInfo& dbInfo, const std::string& dbPath);
    std::vector<HcclStatisticData> FormatData(const OriHcclDataFormat& oriData);
};
} // namespace Domain
} // namespace Analysis
#endif // ANALYSIS_DOMAIN_HCCL_STATISTIC_PROCESSOR_H
