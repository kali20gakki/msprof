/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : hccs_processor.h
 * Description        : hccs_processor，处理hccs表相关数据
 * Author             : msprof team
 * Creation Date      : 2024/2/29
 * *****************************************************************************
 */
#ifndef ANALYSIS_VIEWER_DATABASE_HCCS_PROCESSOR_H
#define ANALYSIS_VIEWER_DATABASE_HCCS_PROCESSOR_H

#include "analysis/csrc/viewer/database/finals/table_processor.h"

namespace Analysis {
namespace Viewer {
namespace Database {
// 该类用于定义处理hccs.db中HCCSEventsData表
class HCCSProcessor : public TableProcessor {
// device_id, timestamp, txthroughput, rxthroughput
using HccsDataFormat = std::vector<std::tuple<uint32_t, double, double, double>>;
// deviceId, timestampNs, txThroughput, rxThroughput
using ProcessedDataFormat = std::vector<std::tuple<uint16_t, uint64_t, uint64_t, uint64_t>>;

public:
    HCCSProcessor() = default;
    HCCSProcessor(const std::string &msprofDBPath, const std::set<std::string> &profPaths);
    bool Run() override;
protected:
    bool Process(const std::string &fileDir) override;
private:
    HccsDataFormat GetData(const std::string &dbPath, DBInfo &hccsDB);
    bool FormatData(const ThreadData &threadData,
                    const HccsDataFormat &hccsData, ProcessedDataFormat &processedData);
};

} // Database
} // Viewer
} // Analysis

#endif // ANALYSIS_VIEWER_DATABASE_HCCS_PROCESSOR_H
