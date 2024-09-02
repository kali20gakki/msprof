/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : pcie_assembler.h
 * Description        : 组合PCIe层数据
 * Author             : msprof team
 * Creation Date      : 2024/9/2
 * *****************************************************************************
 */

#ifndef ANALYSIS_APPLICATION_PCIE_ASSEMBLER_H
#define ANALYSIS_APPLICATION_PCIE_ASSEMBLER_H

#include <unordered_map>
#include <vector>

#include "analysis/csrc/application/timeline/json_assembler.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/pcie_data.h"

namespace Analysis {
namespace Application {
class PCIeAssembler : public JsonAssembler {
public:
    PCIeAssembler();
private:
    uint8_t AssembleData(DataInventory& dataInventory, JsonWriter &ostream, const std::string &profPath) override;
    // 仅txNopostLatency单位转为us,其余为带宽 单位转为MB/s
    std::unordered_map<uint16_t, uint32_t> GeneratePCIeTrace(std::vector<PCIeData> &pcieData,
                                                             uint32_t layerInfo, const std::string &profPath);
private:
    std::vector<std::shared_ptr<TraceEvent>> res_;
};
}
}
#endif // ANALYSIS_APPLICATION_PCIE_ASSEMBLER_H
