/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : acc_pmu_processor.h
 * Description        : acc_pmu_processor，处理AccPmu表数据
 * Author             : msprof team
 * Creation Date      : 2024/2/29
 * *****************************************************************************
 */
#ifndef ANALYSIS_VIEWER_DATABASE_ACC_PMU_PROCESSOR_H
#define ANALYSIS_VIEWER_DATABASE_ACC_PMU_PROCESSOR_H

#include "analysis/csrc/viewer/database/finals/table_processor.h"

namespace Analysis {
namespace Viewer {
namespace Database {
// 该类用于生成ACC_PMU表
class AccPmuProcessor : public TableProcessor {
    // acc_id, read_bandwidth, write_bandwidth, read_ost, write_ost, timestamp
    using OriDataFormat = std::vector<std::tuple<uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, double>>;

    using ProcessedDataFormat = std::vector<std::tuple<uint16_t, uint32_t, uint32_t,
                                                       uint32_t, uint32_t, uint64_t, uint16_t>>;
public:
    AccPmuProcessor() = default;
    AccPmuProcessor(const std::string &reportDBPath, const std::set<std::string> &profPaths);
    virtual ~AccPmuProcessor() = default;
    bool Run() override;
protected:
    bool Process(const std::string &fileDir) override;
private:
    static OriDataFormat GetData(const DBInfo &accPmuDB);
    static ProcessedDataFormat FormatData(const OriDataFormat &oriData,
                                          const Utils::ProfTimeRecord &timeRecord,
                                          const uint16_t deviceId);
};

} // Database
} // Viewer
} // Analysis

#endif // ANALYSIS_VIEWER_DATABASE_ACC_PMU_PROCESSOR_H
