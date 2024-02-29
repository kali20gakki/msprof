/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : ddr_processor.h
 * Description        : ddr_processor，处理ddr表相关数据
 * Author             : msprof team
 * Creation Date      : 2024/2/27
 * *****************************************************************************
 */
#ifndef ANALYSIS_VIEWER_DATABASE_DDR_PROCESSOR_H
#define ANALYSIS_VIEWER_DATABASE_DDR_PROCESSOR_H

#include "analysis/csrc/utils/time_utils.h"
#include "analysis/csrc/viewer/database/finals/table_processor.h"

namespace Analysis {
namespace Viewer {
namespace Database {
// 该类用于定义处理ddr.db中DDRMetricData表
class DDRProcessor : public TableProcessor {
// device_id, timestamp, flux_read, flux_write
    using OriDataFormat = std::vector<std::tuple<uint32_t, double, double, double>>;
// deviceId, timestamp, read, write
    using ProcessedDataFormat = std::vector<std::tuple<uint16_t, uint64_t, double, double>>;
public:
    DDRProcessor() = default;
    DDRProcessor(const std::string &reportDBPath, const std::set<std::string> &profPaths);
    bool Run() override;
protected:
    bool Process(const std::string &fileDir) override;
private:
    static OriDataFormat GetData(const DBInfo &DDRDB);
    static ProcessedDataFormat FormatData(const OriDataFormat &oriData, const Utils::ProfTimeRecord &timeRecord);
};

} // Database
} // Viewer
} // Analysis

#endif // ANALYSIS_VIEWER_DATABASE_DDR_PROCESSOR_H
