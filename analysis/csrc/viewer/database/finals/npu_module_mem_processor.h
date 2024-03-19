/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : npu_module_mem_processor.h
 * Description        : npu_module_mem_processor，处理NpuModuleMem表相关数据
 * Author             : msprof team
 * Creation Date      : 2024/2/17
 * *****************************************************************************
 */
#ifndef ANALYSIS_VIEWER_DATABASE_NPU_MODULE_MEM_PROCESSOR_H
#define ANALYSIS_VIEWER_DATABASE_NPU_MODULE_MEM_PROCESSOR_H

#include "analysis/csrc/viewer/database/finals/table_processor.h"

namespace Analysis {
namespace Viewer {
namespace Database {
// 该类用于生成NPU_MODULE_MEM表
class NpuModuleMemProcessor : public TableProcessor {
    // module_id, syscnt, total_size, device_type
    using OriDataFormat = std::vector<std::tuple<uint32_t, double, uint64_t, std::string>>;
    // module_id, timestamp, totalReserved, deviceId
    using ProcessedDataFormat = std::vector<std::tuple<uint32_t, uint64_t, uint64_t, uint16_t>>;
public:
    NpuModuleMemProcessor() = default;
    NpuModuleMemProcessor(const std::string &reportDBPath, const std::set<std::string> &profPaths);
    virtual ~NpuModuleMemProcessor() = default;
    bool Run() override;
protected:
    bool Process(const std::string &fileDir) override;
private:
    static OriDataFormat GetData(DBInfo &npuModuleMemDB);
    static ProcessedDataFormat FormatData(const OriDataFormat &oriData, uint16_t deviceId,
                                          const Utils::ProfTimeRecord &timeRecord,
                                          Utils::SyscntConversionParams &params);
};

} // Database
} // Viewer
} // Analysis

#endif // ANALYSIS_VIEWER_DATABASE_NPU_MODULE_MEM_PROCESSOR_H
