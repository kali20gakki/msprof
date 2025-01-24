/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : hccs_processor.h
 * Description        : hccs_processor，处理hccs表相关数据
 * Author             : msprof team
 * Creation Date      : 2024/8/9
 * *****************************************************************************
 */
#ifndef ANALYSIS_DOMAIN_HCCS_PROCESSOR_H
#define ANALYSIS_DOMAIN_HCCS_PROCESSOR_H

#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/hccs_data.h"
#include "analysis/csrc/infrastructure/utils/time_utils.h"

namespace Analysis {
namespace Domain {
using namespace Analysis::Utils;
// timestamp, txthroughput, rxthroughput
using OriHccsData = std::vector<std::tuple<double, double, double>>;

// 该类用于定义处理hccs.db中HCCSEventsData表
class HCCSProcessor : public DataProcessor {
public:
    HCCSProcessor() = default;
    explicit HCCSProcessor(const std::string &profPath);

private:
    bool Process(DataInventory& dataInventory) override;
    bool ProcessSingleDevice(const std::string &devicePath,
                             std::vector<HccsData> &allProcessedData, std::vector<HccsSummaryData> &allSummaryData);
    std::vector<HccsData> ProcessData(const DBInfo &hccsDB, LocaltimeContext &localtimeContext);
    HccsSummaryData ProcessSummaryData(const uint16_t &deviceId, const DBInfo &hccsDB);
    OriHccsData LoadData(const DBInfo &hccsDB);
    std::vector<HccsData> FormatData(const OriHccsData &oriData, const LocaltimeContext &localtimeContext);
};
} // Domain
} // Analysis

#endif // ANALYSIS_DOMAIN_HCCS_PROCESSOR_H
