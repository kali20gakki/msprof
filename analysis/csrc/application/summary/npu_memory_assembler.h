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
#ifndef ANALYSIS_APPLICATION_SUMMARY_NPU_MEM_ASSEMBLER_H
#define ANALYSIS_APPLICATION_SUMMARY_NPU_MEM_ASSEMBLER_H

#include "analysis/csrc/application/summary/summary_assembler.h"

namespace Analysis {
namespace Application {

/**
 * static map
 */
const std::map<std::string, std::string> EVENT_MAP = {
    {"0", "APP"},
    {"1", "Device"}
};

class NpuMemoryAssembler : public SummaryAssembler {
public:
    NpuMemoryAssembler() = default;
    NpuMemoryAssembler(const std::string &name, const std::string &profPath);
private:
    uint8_t AssembleData(DataInventory &dataInventory);
};
}
}
#endif // ANALYSIS_APPLICATION_SUMMARY_NPU_MEM_ASSEMBLER_H
