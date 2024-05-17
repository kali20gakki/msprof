/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : aicore_freq_processor.h
 * Description        : aicore_freq_processor，处理aicore freq表 低功耗变频相关数据
 * Author             : msprof team
 * Creation Date      : 2024/5/16
 * *****************************************************************************
 */
#ifndef ANALYSIS_VIEWER_DATABASE_AICORE_FREQ_PROCESSOR_H
#define ANALYSIS_VIEWER_DATABASE_AICORE_FREQ_PROCESSOR_H

#include "analysis/csrc/viewer/database/finals/table_processor.h"

namespace Analysis {
namespace Viewer {
namespace Database {
// 该类用于定义处理freq.db中FreqParse表
class AicoreFreqProcessor : public TableProcessor {
// syscnt, freq
    using OriDataFormat = std::vector<std::tuple<uint64_t, double>>;
// deviceId, timestampNs, freq
    using ProcessedDataFormat = std::vector<std::tuple<uint16_t, uint64_t, double>>;
public:
    AicoreFreqProcessor() = default;
    AicoreFreqProcessor(const std::string &msprofDBPath, const std::set<std::string> &profPaths);
    bool Run() override;
protected:
    bool Process(const std::string &fileDir) override;
private:
    OriDataFormat GetData(const std::string &dbPath, DBInfo &freqDB);
    bool FormatData(const std::string fileDir, const ThreadData &threadData,
                    const Utils::SyscntConversionParams &params,
                    const OriDataFormat &freqData, ProcessedDataFormat &processedData);
};

} // Database
} // Viewer
} // Analysis

#endif // ANALYSIS_VIEWER_DATABASE_AICORE_FREQ_PROCESSOR_H
