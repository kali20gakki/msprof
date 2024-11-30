/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : npu_mem_processor.h
 * Description        : npu_mem_processor，处理NpuMem表相关数据
 * Author             : msprof team
 * Creation Date      : 2024/8/8
 * *****************************************************************************
 */
#ifndef ANALYSIS_DOMAIN_NPU_MEM_PROCESSOR_H
#define ANALYSIS_DOMAIN_NPU_MEM_PROCESSOR_H

#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/npu_mem_data.h"
#include "analysis/csrc/utils/time_utils.h"

namespace Analysis {
namespace Domain {
using namespace Analysis::Utils;
// event, ddr, hbm, timestamp, memory
using OriNpuMemData = std::vector<std::tuple<std::string, uint64_t, uint64_t, double, uint64_t>>;

class NpuMemProcessor : public DataProcessor {
public:
    NpuMemProcessor() = default;
    explicit NpuMemProcessor(const std::string &profPath);

private:
    bool Process(DataInventory& dataInventory) override;
    bool ProcessSingleDevice(const std::string &devicePath, std::vector<NpuMemData> &allProcessedData);
    OriNpuMemData LoadData(const DBInfo &npuMemDB);
    std::vector<NpuMemData> FormatData(const OriNpuMemData &oriData, const LocaltimeContext &localtimeContext);
};
} // Domain
} // Analysis

#endif // ANALYSIS_DOMAIN_NPU_MEM_PROCESSOR_H
