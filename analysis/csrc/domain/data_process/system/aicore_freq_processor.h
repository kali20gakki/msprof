/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : aicore_freq_processor.h
 * Description        : aicore_freq_processor，处理aicore freq表 低功耗变频相关数据
 * Author             : msprof team
 * Creation Date      : 2024/7/30
 * *****************************************************************************
 */

#ifndef ANALYSIS_APPLICATION_AICORE_FREQ_PROCESSOR_H
#define ANALYSIS_APPLICATION_AICORE_FREQ_PROCESSOR_H

#include <vector>
#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/aicore_freq_data.h"

namespace Analysis {
namespace Domain {
// syscnt, freq
using OriDataFormat = std::vector<std::tuple<uint64_t, double>>;

// 该类用于定义处理freq.db中FreqParse表
class AicoreFreqProcessor : public DataProcessor {
public:
    AicoreFreqProcessor() = default;
    explicit AicoreFreqProcessor(const std::string& profPath);
private:
    bool Process(DataInventory& dataInventory);
    bool ProcessData(const std::string& devicePath, OriDataFormat& oriData);
    OriDataFormat LoadData(const std::string& dbPath, DBInfo& freqDB);
    bool FormatData(const uint16_t& deviceId, const Utils::ProfTimeRecord& timeRecord,
                    const Utils::SyscntConversionParams& params, const OriDataFormat& freqData,
                    std::vector<AicoreFreqData>& processedData);
};
} // Viewer
} // Analysis


#endif // ANALYSIS_APPLICATION_AICORE_FREQ_PROCESSOR_H
