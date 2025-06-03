/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2025-2025
            Copyright, 2025, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : fusion_op_assembler.h
 * Description        : 组合fusion_op数据
 * Author             : msprof team
 * Creation Date      : 2025/5/12
 * *****************************************************************************
 */

#ifndef ANALYSIS_APPLICATION_SUMMARY_FUSION_OP_ASSEMBLE_H
#define ANALYSIS_APPLICATION_SUMMARY_FUSION_OP_ASSEMBLE_H

#include <cstdint>
#include <unordered_map>
#include "analysis/csrc/application/summary/summary_assembler.h"
#include "analysis/csrc/infrastructure/data_inventory/include/data_inventory.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/fusion_op_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/model_name_data.h"

namespace Analysis {
namespace Application {
using namespace Analysis::Infra;
using namespace Analysis::Domain;
class FusionOpAssembler : public SummaryAssembler {
public:
    FusionOpAssembler() = default;
    FusionOpAssembler(const std::string &name, const std::string &profPath);
private:
    uint8_t AssembleData(DataInventory &dataInventory);
    void GenerateFusionOp(std::vector<FusionOpInfo> &fusionOpData,
                        std::vector<ModelName> &modelNameData, std::unordered_map<std::string, std::string> &hashMap);
};
}
}
#endif // ANALYSIS_APPLICATION_SUMMARY_FUSION_OP_ASSEMBLE_H