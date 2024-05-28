/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : pipeutext_calculator.h
 * Description        : PipeUtilizationExct metric计算
 * Author             : msprof team
 * Creation Date      : 2024/5/21
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_PIPEUTEXT_ITEM_H
#define ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_PIPEUTEXT_ITEM_H

#include "analysis/csrc/domain/services/association/calculator/metric/metric_calculator.h"

namespace Analysis {
namespace Domain {
using namespace Analysis::Infra;
class PipeUtExtCalculator : public MetricCalculator {
private:
    std::vector<double> SetAllParamsAndCalculator(CalculationElements& allParams, const DeviceContext& context,
                                                  HalPmuData& pmuData) override
    {
        std::vector<double> res;
        MAKE_SHARED_RETURN_VALUE(allParams.floatBit, DoublePtrType, res, floatBitVec);
        res = CalculatePmu(pmuData, pipeUtExTable, allParams);
        return res;
    }
private:
    const std::map<PipeUtilizationExctIndex, Calculator> pipeUtExTable{
        {PipeUtilizationExctIndex::MacRatioExtra, {{0x416, 0x417}, Calculator::CalculatorMetricByAdditions}},
        {PipeUtilizationExctIndex::ScalarRatio, {{0x9}, Calculator::CalculatorMetricByAdditions}},
        {PipeUtilizationExctIndex::Mte1RatioExtra, {{0x302}, Calculator::CalculatorMetricByAdditions}},
        {PipeUtilizationExctIndex::Mte2Ratio, {{0xc}, Calculator::CalculatorMetricByAdditions}},
        {PipeUtilizationExctIndex::FixPipeRatio, {{0x303}, Calculator::CalculatorMetricByAdditions}},
        {PipeUtilizationExctIndex::ICacheMissRate, {{0x54, 0x55}, Calculator::CalculatorMetricByAdditions}},
        {PipeUtilizationExctIndex::MacTime, {{0x416, 0x417}, Calculator::CalculatorTimeByMultiplication}},
        {PipeUtilizationExctIndex::ScalarTime, {{0x9}, Calculator::CalculatorTimeByMultiplication}},
        {PipeUtilizationExctIndex::Mte1Time, {{0x302}, Calculator::CalculatorTimeByMultiplication}},
        {PipeUtilizationExctIndex::Mte2Time, {{0xc}, Calculator::CalculatorTimeByMultiplication}},
        {PipeUtilizationExctIndex::FixPipeTime, {{0x303}, Calculator::CalculatorTimeByMultiplication}}
    };

    const std::vector<double> floatBitVec{1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0};
};
}
}

#endif // ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_PIPEUTEXT_ITEM_H
