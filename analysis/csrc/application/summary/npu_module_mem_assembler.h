/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2025-2025
            Copyright, 2025, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : npu_module_mem_summary.h
 * Description        : npu 内存数据
 * Author             : msprof team
 * Creation Date      : 2025/05/09
 * *****************************************************************************
 */
#ifndef ANALYSIS_APPLICATION_SUMMARY_NPU_MODULE_MEM_ASSEMBLER_H
#define ANALYSIS_APPLICATION_SUMMARY_NPU_MODULE_MEM_ASSEMBLER_H

#include "analysis/csrc/application/summary/summary_assembler.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/npu_mem_data.h"

namespace Analysis {
namespace Application {

class NpuModuleMemAssembler : public SummaryAssembler {
public:
    NpuModuleMemAssembler() = default;
    NpuModuleMemAssembler(const std::string &name, const std::string &profPath);
private:
    std::unordered_map<uint16_t, std::string> moduleMap_;

    uint8_t AssembleData(DataInventory &dataInventory);
};
}
}
#endif // ANALYSIS_APPLICATION_SUMMARY_NPU_MODULE_MEM_ASSEMBLER_H