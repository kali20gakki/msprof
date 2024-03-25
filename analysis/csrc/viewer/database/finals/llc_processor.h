/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : llc_processor.h
 * Description        : llc_processor，处理LLCMetrics表相关数据
 * Author             : msprof team
 * Creation Date      : 2024/2/23
 * *****************************************************************************
 */

#ifndef ANALYSIS_VIEWER_DATABASE_LLC_PROCESSOR_H
#define ANALYSIS_VIEWER_DATABASE_LLC_PROCESSOR_H

#include "analysis/csrc/viewer/database/finals/table_processor.h"

namespace Analysis {
namespace Viewer {
namespace Database {
// 该类用于生成LLC表
class LLCProcessor : public TableProcessor {
public:
    // device_id, l3tid, timestamp, hitrate, throughput
    using OriDataFormat = std::vector<std::tuple<uint32_t, uint32_t, double, double, double>>;
    // deviceId, llcID, timestamp, hitRate, throughput, mode
    using ProcessedDataFormat = std::vector<std::tuple<uint16_t, uint32_t, uint64_t, double, uint64_t, uint64_t>>;

    LLCProcessor() = default;
    LLCProcessor(const std::string &msprofDBPath, const std::set<std::string> &profPaths);
    virtual ~LLCProcessor() = default;
    bool Run() override;

protected:
    bool Process(const std::string &fileDir) override;

private:
    static OriDataFormat GetData(const std::string &dbPath, DBInfo &dbInfo);
    static ProcessedDataFormat FormatData(const OriDataFormat &oriData, const ThreadData &threadData,
                                          const uint64_t mode);
    bool ProcessData(const std::string &fileDir);
};
} // Database
} // Viewer
} // Analysis

#endif // ANALYSIS_VIEWER_DATABASE_LLC_PROCESSOR_H
