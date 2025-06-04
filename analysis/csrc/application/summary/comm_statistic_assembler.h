/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2025-2025
            Copyright, 2025, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : communication_statistic_assembler.h
 * Description        : 组合communication_statistic数据
 * Author             : msprof team
 * Creation Date      : 2025/5/24
 * *****************************************************************************
 */

#ifndef ANALYSIS_APPLICATION_SUMMARY_COMMUNICATION_STATISTIC_ASSEMBLE_H
#define ANALYSIS_APPLICATION_SUMMARY_COMMUNICATION_STATISTIC_ASSEMBLE_H

#include "analysis/csrc/application/summary/summary_assembler.h"
#include "analysis/csrc/infrastructure/data_inventory/include/data_inventory.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/hccl_statistic_data.h"

namespace Analysis {
namespace Application {
using namespace Analysis::Infra;
using namespace Analysis::Domain;

class CommStatisticAssembler : public SummaryAssembler {
public:
    CommStatisticAssembler() = default;
    CommStatisticAssembler(const std::string& name, const std::string& profPath);
private:
    uint8_t AssembleData(DataInventory& dataInventory);
    void GenerateCommStatistic(std::vector<HcclStatisticData>& commStatisticData);
};
}
}


#endif // ANALYSIS_APPLICATION_SUMMARY_COMMUNICATION_STATISTIC_ASSEMBLE_H
