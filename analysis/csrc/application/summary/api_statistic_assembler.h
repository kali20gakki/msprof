/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2025-2025
            Copyright, 2025, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : api_statistic_assembler.h
 * Description        : 组合api_statistic数据
 * Author             : msprof team
 * Creation Date      : 2025/5/12
 * *****************************************************************************
 */

#ifndef ANALYSIS_APPLICATION_SUMMARY_API_STATISTIC_ASSEMBLE_H
#define ANALYSIS_APPLICATION_SUMMARY_API_STATISTIC_ASSEMBLE_H

#include <cstdint>
#include <map>
#include "analysis/csrc/application/summary/summary_assembler.h"
#include "analysis/csrc/infrastructure/data_inventory/include/data_inventory.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/api_data.h"

namespace Analysis {
namespace Application {
using namespace Analysis::Infra;
using namespace Analysis::Domain;
class ApiStatisticAssembler : public SummaryAssembler {
public:
    ApiStatisticAssembler() = default;
    ApiStatisticAssembler(const std::string &name, const std::string &profPath);
private:
    uint8_t AssembleData(DataInventory &dataInventory);
    void GenerateApiStatistic(std::vector<ApiData> &apiData);
};
}
}
#endif // ANALYSIS_APPLICATION_API_STATISTIC_ASSEMBLE_H