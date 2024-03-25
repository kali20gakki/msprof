/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : soc_processor.h
 * Description        : soc_processor，处理InterSoc表数据
 * Author             : msprof team
 * Creation Date      : 2024/3/1
 * *****************************************************************************
 */
#ifndef ANALYSIS_VIEWER_DATABASE_SOC_PROCESSOR_H
#define ANALYSIS_VIEWER_DATABASE_SOC_PROCESSOR_H

#include "analysis/csrc/viewer/database/finals/table_processor.h"

namespace Analysis {
namespace Viewer {
namespace Database {
// 该类用于生成SOC_BANDWIDTH_LEVEL表
class SocProcessor : public TableProcessor {
    // l2_buffer_bw_level, mata_bw_level, sys_time
    using OriDataFormat = std::vector<std::tuple<uint32_t, uint32_t, double>>;

    using ProcessedDataFormat = std::vector<std::tuple<uint32_t, uint32_t, uint64_t, uint16_t>>;
public:
    SocProcessor() = default;
    SocProcessor(const std::string &msprofDBPath, const std::set<std::string> &profPaths);
    virtual ~SocProcessor() = default;
    bool Run() override;
protected:
    bool Process(const std::string &fileDir) override;
private:
    static OriDataFormat GetData(const DBInfo &socBandwidthLevelDB);
    static ProcessedDataFormat FormatData(const OriDataFormat &oriData,
                                          const Utils::ProfTimeRecord &timeRecord,
                                          const uint16_t deviceId);
};

} // Database
} // Viewer
} // Analysis

#endif // ANALYSIS_VIEWER_DATABASE_SOC_PROCESSOR_H
