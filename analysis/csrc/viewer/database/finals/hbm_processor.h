/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : hbm_processor.h
 * Description        : hbm_processor，处理hbm表相关数据
 * Author             : msprof team
 * Creation Date      : 2024/2/27
 * *****************************************************************************
 */
#ifndef ANALYSIS_VIEWER_DATABASE_HBM_PROCESSOR_H
#define ANALYSIS_VIEWER_DATABASE_HBM_PROCESSOR_H

#include "analysis/csrc/viewer/database/finals/table_processor.h"

namespace Analysis {
namespace Viewer {
namespace Database {
// 该类用于定义处理hbm.db中HBMbwData表
class HBMProcessor : public TableProcessor {
// device_id, timestamp, bandwidth, hbmid, event_type
    using OriDataFormat = std::vector<std::tuple<uint32_t, double, double, uint32_t, std::string>>;
// deviceId, timestamp, bandwidth, hbmId, type
    using ProcessedDataFormat = std::vector<std::tuple<uint16_t, uint64_t, uint64_t, uint8_t, uint64_t>>;
public:
    HBMProcessor() = default;
    HBMProcessor(const std::string &msprofDBPath, const std::set<std::string> &profPaths);
    bool Run() override;
protected:
    bool Process(const std::string &fileDir) override;
private:
    static OriDataFormat GetData(const DBInfo &HBMDB);
    static ProcessedDataFormat FormatData(const OriDataFormat &oriData, const ThreadData &threadData);
};

} // Database
} // Viewer
} // Analysis

#endif // ANALYSIS_VIEWER_DATABASE_HBM_PROCESSOR_H
