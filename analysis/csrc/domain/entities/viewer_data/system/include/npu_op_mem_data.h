/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : npu_op_mem_data.h
 * Description        : npu_op_mem_processor处理NpuMem表后的格式化数据
 * Author             : msprof team
 * Creation Date      : 2024/8/10
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_NPU_OP_MEM_FORMAT_DATA_H
#define ANALYSIS_DOMAIN_NPU_OP_MEM_FORMAT_DATA_H

#include <cstdint>
#include <string>

namespace Analysis {
namespace Domain {
struct NpuOpMemData {
    uint16_t deviceId = UINT16_MAX;
    uint32_t threadId = 0;
    uint64_t addr = UINT64_MAX;
    int64_t size = 0;
    uint64_t localTime = 0;
    uint64_t totalAllocateMemory = 0;
    uint64_t totalReserveMemory = 0;
    uint64_t type;
    std::string operatorName;
};
}
}

#endif // ANALYSIS_DOMAIN_NPU_OP_MEM_FORMAT_DATA_H
