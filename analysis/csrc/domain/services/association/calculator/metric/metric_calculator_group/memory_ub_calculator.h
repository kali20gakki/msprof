/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : memory_ub_calculator.h
 * Description        : MemoryUB metric计算
 * Author             : msprof team
 * Creation Date      : 2024/5/21
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_MEMORY_UB_ITEM_H
#define ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_MEMORY_UB_ITEM_H

#include "analysis/csrc/domain/services/association/calculator/metric/metric_calculator.h"

namespace Analysis {
namespace Domain {
using namespace Analysis::Infra;
class MemoryUBCalculator : public MetricCalculator {
private:
    std::vector<double> SetAllParamsAndCalculator(CalculationElements& allParams, const DeviceContext& context,
                                                  HalPmuData& pmuData) override
    {
        std::vector<double> res;
        MAKE_SHARED_RETURN_VALUE(allParams.floatBit, DoublePtrType, res, floatBitVec);
        MAKE_SHARED_RETURN_VALUE(allParams.pipSize, DoublePtrType, res, pipeSizeVec);
        MAKE_SHARED_RETURN_VALUE(allParams.scalar, DoublePtrType, res, scalarVec);
        res = CalculatePmu(pmuData, memoryUBTable, allParams);
        return res;
    }
private:
    const std::map<MemoryUBIndex, Calculator> memoryUBTable{
        {MemoryUBIndex::UbReadBwVector, {{0x43}, Calculator::CalculatorMetricByAdditionsWithFreq}},
        {MemoryUBIndex::UbWriteBwVector, {{0x44}, Calculator::CalculatorMetricByAdditionsWithFreq}},
        {MemoryUBIndex::UbReadBwScalar, {{0x37}, Calculator::CalculatorMetricByAdditionsWithFreq}},
        {MemoryUBIndex::UbWriteBwScalar, {{0x38}, Calculator::CalculatorMetricByAdditionsWithFreq}}
    };

    const std::vector<double> floatBitVec{1.0, 1.0, 1.0, 1.0};
    const std::vector<double> pipeSizeVec{128.0, 128.0, 128.0, 128.0};
    const std::vector<double> scalarVec{2.0, 2.0, 1.0, 1.0};
};
}
}

#endif // ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_MEMORY_UB_ITEM_H
