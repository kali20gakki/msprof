/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : llc_processor.h
 * Description        : llc_processor，处理llc表相关数据
 * Author             : msprof team
 * Creation Date      : 2024/8/20
 * *****************************************************************************
 */
#ifndef ANALYSIS_DOMAIN_LLC_PROCESSOR_H
#define ANALYSIS_DOMAIN_LLC_PROCESSOR_H

#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/llc_data.h"
#include "analysis/csrc/utils/time_utils.h"

namespace Analysis {
namespace Domain {
using namespace Analysis::Utils;

// l3tid, timestamp, hitrate, throughput
using OriLLcData = std::vector<std::tuple<uint32_t, double, double, double>>;
// 该类用于定义处理llc.db中LLCMetrics表
class LLcProcessor : public DataProcessor {
public:
    LLcProcessor() = default;
    explicit LLcProcessor(const std::string &profPath);

private:
    bool Process(DataInventory& dataInventory) override;
    bool ProcessSingleDevice(const std::string &devicePath, LocaltimeContext &localtimeContext,
                             std::vector<LLcData> &allProcessedData, std::vector<LLcSummaryData> &allSummaryData);
    std::vector<LLcData> ProcessData(const DBInfo &llcDB, LocaltimeContext &localtimeContext, const std::string &mode);
    std::vector<LLcSummaryData> ProcessSummaryData(const uint16_t &deviceId, const DBInfo &llcDB,
                                                   const std::string mode);
    OriLLcData LoadData(const DBInfo &llcDB);
    std::vector<LLcData> FormatData(const OriLLcData &oriData, const LocaltimeContext &localtimeContext,
                                    const std::string &mode);
};
} // Domain
} // Analysis

#endif // ANALYSIS_DOMAIN_LLC_PROCESSOR_H
