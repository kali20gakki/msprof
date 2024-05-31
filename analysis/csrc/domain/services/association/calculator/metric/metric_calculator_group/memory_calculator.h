/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : memory_calculator.h
 * Description        : Memory metric计算
 * Author             : msprof team
 * Creation Date      : 2024/5/21
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_MEMORY_ITEM_H
#define ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_MEMORY_ITEM_H

#include "analysis/csrc/domain/services/association/calculator/metric/metric_calculator.h"

namespace Analysis {
namespace Domain {
using namespace Analysis::Infra;
class MemoryCalculator : public MetricCalculator {
public:
    std::vector<std::string> GetPmuHeader() override
    {
        auto res = GetPmuHeaderBySubType(memoryTable);
        return res;
    }
private:
    std::vector<double> SetAllParamsAndCalculator(CalculationElements& allParams, const DeviceContext& context,
                                                  HalPmuData& pmuData) override
    {
        std::vector<double> res;
        MAKE_SHARED_RETURN_VALUE(allParams.floatBit, DoublePtrType, res, floatBitVec);
        MAKE_SHARED_RETURN_VALUE(allParams.pipSize, DoublePtrType, res, pipeSizeVec);
        MAKE_SHARED_RETURN_VALUE(allParams.scalar, DoublePtrType, res, scalarVec);
        res = CalculatePmu(pmuData, memoryTable, allParams);
        return res;
    }
private:
    const std::map<MemoryIndex, Calculator> memoryTable{
        {MemoryIndex::UBReadBw, {{0x15}, Calculator::CalculatorMetricByAdditionsWithFreq}},
        {MemoryIndex::UBWriteBw, {{0x16}, Calculator::CalculatorMetricByAdditionsWithFreq}},
        {MemoryIndex::L1ReadBw, {{0x31}, Calculator::CalculatorMetricByAdditionsWithFreq}},
        {MemoryIndex::L1WriteBw, {{0x32}, Calculator::CalculatorMetricByAdditionsWithFreq}},
        {MemoryIndex::MainMemReadBw, {{0x12}, Calculator::CalculatorMetricByAdditionsWithFreq}},
        {MemoryIndex::MainMemWriteBw, {{0x13}, Calculator::CalculatorMetricByAdditionsWithFreq}},
    };

    const std::vector<double> floatBitVec{1.0, 1.0, 1.0, 1.0, 1.0, 1.0};
    const std::vector<double> pipeSizeVec{256.0, 256.0, 256.0, 128.0, 8.0, 8.0};
    const std::vector<double> scalarVec{4.0, 4.0, 16.0, 8.0, 8.0, 8.0};
};
}
}
#endif // ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_MEMORY_ITEM_H
