/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : hbm_processor.h
 * Description        : hbm_processor，处理hbm表相关数据
 * Author             : msprof team
 * Creation Date      : 2024/8/20
 * *****************************************************************************
 */
#ifndef ANALYSIS_DOMAIN_HBM_PROCESSOR_H
#define ANALYSIS_DOMAIN_HBM_PROCESSOR_H

#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/hbm_data.h"
#include "analysis/csrc/infrastructure/utils/time_utils.h"

namespace Analysis {
namespace Domain {
using namespace Analysis::Utils;

// timestamp, bandwidth, hbmid, event_type
using OriHbmData = std::vector<std::tuple<double, double, uint32_t, std::string>>;
// 该类用于定义处理hbm.db中HBMbwData表
class HBMProcessor : public DataProcessor {
public:
    HBMProcessor() = default;
    explicit HBMProcessor(const std::string &profPath);

private:
    bool Process(DataInventory& dataInventory) override;
    bool ProcessSingleDevice(const std::string &devicePath,
                             std::vector<HbmData> &allProcessedData, std::vector<HbmSummaryData> &allSummaryData);
    std::vector<HbmData> ProcessData(const DBInfo &hbmDB, LocaltimeContext &localtimeContext);
    std::vector<HbmSummaryData> ProcessSummaryData(const uint16_t &deviceId, const DBInfo &hbmDB);
    OriHbmData LoadData(const DBInfo &hbmDB);
    std::vector<HbmData> FormatData(const OriHbmData &oriData, const LocaltimeContext &localtimeContext);
};
} // Domain
} // Analysis

#endif // ANALYSIS_DOMAIN_HBM_PROCESSOR_H
