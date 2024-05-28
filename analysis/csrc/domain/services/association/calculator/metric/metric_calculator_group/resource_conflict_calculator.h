/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : resource_conflict_calculator.h
 * Description        : ResourceConflictRatio metric计算
 * Author             : msprof team
 * Creation Date      : 2024/5/21
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_RESOURCE_CONFLICT_ITEM_H
#define ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_RESOURCE_CONFLICT_ITEM_H

#include "analysis/csrc/domain/services/association/calculator/metric/metric_calculator.h"

namespace Analysis {
namespace Domain {
using namespace Analysis::Infra;
class ResourceConflictCalculator : public MetricCalculator {
private:
    std::vector<double> SetAllParamsAndCalculator(CalculationElements& allParams, const DeviceContext& context,
                                                  HalPmuData& pmuData) override
    {
        std::vector<double> res;
        MAKE_SHARED_RETURN_VALUE(allParams.floatBit, DoublePtrType, res, floatBitVec);
        res = CalculatePmu(pmuData, resourceConflictTable, allParams);
        return res;
    }
private:
    const std::map<ResourceConflictIndex, Calculator> resourceConflictTable{
        {ResourceConflictIndex::VecBankGroupCfltRatio, {{0x64}, Calculator::CalculatorMetricByAdditions}},
        {ResourceConflictIndex::VecBankCfltRatio, {{0x65}, Calculator::CalculatorMetricByAdditions}},
        {ResourceConflictIndex::VecRescCfltRatio, {{0x66}, Calculator::CalculatorMetricByAdditions}}
    };

    const std::vector<double> floatBitVec{1.0, 1.0, 1.0};
};
}
}
#endif // ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_RESOURCE_CONFLICT_ITEM_H
