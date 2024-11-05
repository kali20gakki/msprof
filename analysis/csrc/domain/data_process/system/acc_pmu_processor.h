/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : acc_pmu_processor.h
 * Description        : acc_pmu_processor，处理AccPmu表数据
 * Author             : msprof team
 * Creation Date      : 2024/7/20
 * *****************************************************************************
 */

#ifndef ANALYSIS_APPLICATION_ACC_PMU_PROCESSOR_H
#define ANALYSIS_APPLICATION_ACC_PMU_PROCESSOR_H

#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/acc_pmu_data.h"

namespace Analysis {
namespace Domain {
// acc_id, read_bandwidth, write_bandwidth, read_ost, write_ost, timestamp
using OriAccPmuData = std::vector<std::tuple<uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, double>>;

class AccPmuProcessor : public DataProcessor {
public:
    AccPmuProcessor() = default;
    explicit AccPmuProcessor(const std::string &profPath);
private:
    bool Process(DataInventory& dataInventory) override;
    OriAccPmuData LoadData(const DBInfo &accPmuDB, const std::string &dbPath);
    std::vector<AccPmuData> FormatData(const OriAccPmuData &oriData,
                                       const Utils::ProfTimeRecord &timeRecord,
                                       const uint16_t deviceId);
};
}
}

#endif // ANALYSIS_APPLICATION_ACC_PMU_PROCESSOR_H
