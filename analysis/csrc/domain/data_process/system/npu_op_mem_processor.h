/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : npu_op_mem_processor.h
 * Description        : npu_op_mem_processor，处理NpuOpMemRaw表相关数据
 * Author             : msprof team
 * Creation Date      : 2024/2/21
 * *****************************************************************************
 */
#ifndef ANALYSIS_DOMAIN_NPU_OP_MEM_PROCESSOR_H
#define ANALYSIS_DOMAIN_NPU_OP_MEM_PROCESSOR_H

#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/npu_op_mem_data.h"
#include "analysis/csrc/utils/time_utils.h"

namespace Analysis {
namespace Domain {
using GeHashMap = std::unordered_map<std::string, std::string>;
class NpuOpMemProcessor : public DataProcessor {
    // operator, addr, size, timestamp, thread_id, total_allocate_memory, total_reserve_memory, device_type
    using OriDataFormat = std::vector<std::tuple<std::string, std::string, int64_t, double, uint32_t, uint64_t,
                                                 uint64_t, std::string>>;
public:
    NpuOpMemProcessor() = default;
    explicit NpuOpMemProcessor(const std::string &profPath);

private:
    bool Process(DataInventory& dataInventory) override;
    static OriDataFormat LoadData(const DBInfo &npuOpMemDB);
    std::vector<NpuOpMemData> FormatData(const OriDataFormat &oriData, const Utils::ProfTimeRecord &timeRecord,
                                         Utils::SyscntConversionParams &params, GeHashMap &hashMap) const;
    static uint16_t GetDeviceId(const std::string& deviceType);

private:
    uint64_t stringReleaseId_ = 0;
    uint64_t stringAllocateId_ = 0;
};

} // Domain
} // Analysis

#endif // ANALYSIS_DOMAIN_NPU_OP_MEM_PROCESSOR_H
