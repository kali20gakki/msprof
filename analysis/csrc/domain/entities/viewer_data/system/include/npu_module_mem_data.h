/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : npu_mem_data.h
 * Description        : npu_mem_processor处理NpuMem表后的格式化数据
 * Author             : msprof team
 * Creation Date      : 2024/11/09
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_NPU_MODULE_MEM_DATA_H
#define ANALYSIS_DOMAIN_NPU_MODULE_MEM_DATA_H

#include <cstdint>


namespace Analysis {
namespace Domain {
struct NpuModuleMemData {
    uint32_t moduleId = UINT32_MAX;
    uint64_t timestamp = UINT64_MAX;
    uint64_t totalReserved = UINT64_MAX;
    uint16_t deviceId = UINT16_MAX;
};
}
}

#endif // ANALYSIS_DOMAIN_NPU_MODULE_MEM_DATA_H
