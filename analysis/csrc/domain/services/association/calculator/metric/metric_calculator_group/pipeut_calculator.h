/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : pipeut_calculator.h
 * Description        : PipeUtilization metric计算
 * Author             : msprof team
 * Creation Date      : 2024/5/21
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_PIPEUT_ITEM_H
#define ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_PIPEUT_ITEM_H

#include "analysis/csrc/domain/services/association/calculator/metric/metric_calculator.h"

namespace Analysis {
namespace Domain {
using namespace Analysis::Infra;
class PipeUtCalculator : public MetricCalculator {
public:
    std::vector<std::string> GetPmuHeader() override
    {
        auto res = GetPmuHeaderBySubType(pipeLineUtTable);
        return res;
    }
private:
    std::vector<double> SetAllParamsAndCalculator(CalculationElements& allParams, const DeviceContext& context,
                                                  HalPmuData& pmuData) override
    {
        std::vector<double> res;
        MAKE_SHARED_RETURN_VALUE(allParams.floatBit, DoublePtrType, res, floatBitVec);
        res = CalculatePmu(pmuData, pipeLineUtTable, allParams);
        return res;
    }
private:
    const std::map<PipeLineUtIndex, Calculator> pipeLineUtTable{
        {PipeLineUtIndex::VecRatio, {{0x8}, Calculator::CalculatorMetricByAdditions}},
        {PipeLineUtIndex::VecTime, {{0x8}, Calculator::CalculatorTimeByMultiplication}},
        {PipeLineUtIndex::MacRatio, {{0xa}, Calculator::CalculatorMetricByAdditions}},
        {PipeLineUtIndex::MacTime, {{0xa}, Calculator::CalculatorTimeByMultiplication}},
        {PipeLineUtIndex::ScalarRatio, {{0x9}, Calculator::CalculatorMetricByAdditions}},
        {PipeLineUtIndex::ScalarTime, {{0x9}, Calculator::CalculatorTimeByMultiplication}},
        {PipeLineUtIndex::Mte1Ratio, {{0xb}, Calculator::CalculatorMetricByAdditions}},
        {PipeLineUtIndex::Mte1Time, {{0xb}, Calculator::CalculatorTimeByMultiplication}},
        {PipeLineUtIndex::Mte2Ratio, {{0xc}, Calculator::CalculatorMetricByAdditions}},
        {PipeLineUtIndex::Mte2Time, {{0xc}, Calculator::CalculatorTimeByMultiplication}},
        {PipeLineUtIndex::Mte3Ratio, {{0xd}, Calculator::CalculatorMetricByAdditions}},
        {PipeLineUtIndex::Mte3Time, {{0xd}, Calculator::CalculatorTimeByMultiplication}},
        {PipeLineUtIndex::ICacheMissRate, {{0x54, 0x55}, Calculator::CalculatorMetricByAdditions}}
    };

    const std::vector<double> floatBitVec{1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0};
};
}
}

#endif // ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_PIPEUT_ITEM_H
