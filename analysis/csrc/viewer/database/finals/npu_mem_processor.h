/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : npu_mem_processor.h
 * Description        : npu_mem_processor，处理NpuMem表相关数据
 * Author             : msprof team
 * Creation Date      : 2024/2/7
 * *****************************************************************************
 */
#ifndef ANALYSIS_VIEWER_DATABASE_NPU_MEM_PROCESSOR_H
#define ANALYSIS_VIEWER_DATABASE_NPU_MEM_PROCESSOR_H

#include "analysis/csrc/viewer/database/finals/table_processor.h"

namespace Analysis {
namespace Viewer {
namespace Database {
// 该类用于生成NPU_MEM表
class NpuMemProcessor : public TableProcessor {
    // event, ddr, hbm, timestamp
    using OriDataFormat = std::vector<std::tuple<std::string, uint64_t, uint64_t, double>>;
    // type, ddrUsage, hbmUsage, timestamp, deviceId
    using ProcessedDataFormat = std::vector<std::tuple<uint64_t, uint64_t, uint64_t, uint64_t, uint16_t>>;
public:
    NpuMemProcessor() = default;
    NpuMemProcessor(const std::string &msprofDBPath, const std::set<std::string> &profPaths);
    virtual ~NpuMemProcessor() = default;
    bool Run() override;
protected:
    bool Process(const std::string &fileDir) override;
private:
    static OriDataFormat GetData(DBInfo &npuMemDB);
    ProcessedDataFormat FormatData(const OriDataFormat &oriData, const uint16_t deviceId,
                                   const uint64_t hostMonotonic, const Utils::ProfTimeRecord &timeRecord) const;
    uint64_t stringAppId_;
    uint64_t stringDeviceId_;
};

} // Database
} // Viewer
} // Analysis

#endif // ANALYSIS_VIEWER_DATABASE_NPU_MEM_PROCESSOR_H
