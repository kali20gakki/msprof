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
#ifndef ANALYSIS_VIEWER_DATABASE_NPU_OP_MEM_PROCESSOR_H
#define ANALYSIS_VIEWER_DATABASE_NPU_OP_MEM_PROCESSOR_H

#include "analysis/csrc/utils/time_utils.h"
#include "analysis/csrc/viewer/database/finals/table_processor.h"

namespace Analysis {
namespace Viewer {
namespace Database {
// 该类用于生成NPU_OP_MEM表
class NpuOpMemProcessor : public TableProcessor {
    // operator, addr, size, timestamp, thread_id, total_allocate_memory, total_reserve_memory, level, type, device_type
    using OriDataFormat = std::vector<std::tuple<std::string, std::string, int64_t, double, uint32_t, uint64_t,
                                                 uint64_t, uint32_t, uint32_t, std::string>>;
    // operatorName, addr, type, size, timestamp, globalTid, totalAllocate, totalReserve,  component, deviceId
    using ProcessedDataFormat = std::vector<std::tuple<uint64_t, uint64_t, uint32_t, uint64_t, std::string, uint64_t,
                                                       double, double, uint64_t, uint16_t>>;
public:
    NpuOpMemProcessor() = default;
    NpuOpMemProcessor(const std::string &reportDBPath, const std::set<std::string> &profPaths);
    virtual ~NpuOpMemProcessor() = default;
    bool Run() override;
protected:
    bool Process(const std::string &fileDir) override;
private:
    static OriDataFormat GetData(DBInfo &npuOpMemDB);
    static ProcessedDataFormat FormatData(const OriDataFormat &oriData, Utils::ProfTimeRecord &timeRecord,
                                          Utils::SyscntConversionParams &params, GeHashMap &hashMap, uint32_t profId);
    static uint16_t GetDeviceId(const std::string& deviceType);
};

} // Database
} // Viewer
} // Analysis

#endif // ANALYSIS_VIEWER_DATABASE_NPU_OP_MEM_PROCESSOR_H
