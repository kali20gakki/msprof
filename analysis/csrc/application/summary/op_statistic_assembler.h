/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2025-2025
            Copyright, 2025, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : op_statistic_assembler.h
 * Description        : 组合op_statistic数据
 * Author             : msprof team
 * Creation Date      : 2025/5/30
 * *****************************************************************************
 */

#ifndef ANALYSIS_APPLICATION_SUMMARY_OP_STATISTIC_ASSEMBLE_H
#define ANALYSIS_APPLICATION_SUMMARY_OP_STATISTIC_ASSEMBLE_H

#include "analysis/csrc/application/summary/summary_assembler.h"
#include "analysis/csrc/infrastructure/data_inventory/include/data_inventory.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/op_statistic_data.h"

namespace Analysis {
namespace Application {
using namespace Analysis::Infra;
using namespace Analysis::Domain;

class OpStatisticAssembler : public SummaryAssembler {
public:
    OpStatisticAssembler() = default;
    OpStatisticAssembler(const std::string& name, const std::string& profPath);
private:
    uint8_t AssembleData(DataInventory& dataInventory);
    void GenerateOpStatistic(std::vector<OpStatisticData>& opStatisticData);
};
}
}

#endif // ANALYSIS_APPLICATION_SUMMARY_OP_STATISTIC_ASSEMBLE_H