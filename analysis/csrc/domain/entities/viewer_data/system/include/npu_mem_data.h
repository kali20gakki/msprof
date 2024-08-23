/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : npu_mem_data.h
 * Description        : npu_mem_processor处理NpuMem表后的格式化数据
 * Author             : msprof team
 * Creation Date      : 2024/8/10
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_NPU_MEM_FORMAT_DATA_H
#define ANALYSIS_DOMAIN_NPU_MEM_FORMAT_DATA_H

#include <cstdint>
#include <string>

namespace Analysis {
namespace Domain {
struct NpuMemData {
    uint16_t deviceId = UINT16_MAX;
    std::string event;
    uint64_t ddr = 0;
    uint64_t hbm = 0;
    uint64_t memory = 0; // ddr与hbm之和
    uint64_t localTime = 0;
};
}
}

#endif // ANALYSIS_DOMAIN_NPU_MEM_FORMAT_DATA_H
