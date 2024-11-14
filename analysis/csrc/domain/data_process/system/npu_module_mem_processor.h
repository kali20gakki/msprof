/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : npu_module_mem_processor.h
 * Description        : npu_module_mem_processor，处理NpuModuleMem表相关数据
 * Author             : msprof team
 * Creation Date      : 2024/11/09
 * *****************************************************************************
 */
#ifndef ANALYSIS_DOMAIN_NPU_MODULE_MEM_PROCESSOR_H
#define ANALYSIS_DOMAIN_NPU_MODULE_MEM_PROCESSOR_H

#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/npu_module_mem_data.h"
#include "analysis/csrc/utils/time_utils.h"

namespace Analysis {
namespace Domain {
using namespace Analysis::Utils;
// module_id, syscnt, total_size, device_type
using OriNpuModuleFormat = std::vector<std::tuple<uint32_t, double, uint64_t, std::string>>;

class NpuModuleMemProcessor : public DataProcessor {
public:
    NpuModuleMemProcessor() = default;
    explicit NpuModuleMemProcessor(const std::string& profPaths);

private:
    bool Process(DataInventory& dataInventory) override;
    bool ProcessSingleDevice(const std::string& devicePath, LocaltimeContext& localtimeContext,
                             std::vector<NpuModuleMemData>& allProcessedData, SyscntConversionParams& params);
    OriNpuModuleFormat LoadData(const DBInfo& npuModuleMemDB);
    std::vector<NpuModuleMemData> FormatData(const OriNpuModuleFormat& oriData,
                                             const LocaltimeContext& localtimeContext,
                                             SyscntConversionParams& params);
};

} // Domain
} // Analysis

#endif // ANALYSIS_DOMAIN_NPU_MODULE_MEM_PROCESSOR_H
