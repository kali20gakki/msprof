/* -------------------------------------------------------------------------
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/

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

    bool CheckMetricEventValid(std::vector<uint32_t> &event) override
    {
        return CheckMetricEventBySubType(pipeLineUtTable, event);
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
        {PipeLineUtIndex::ICacheMissRate, {{0x55, 0x54}, Calculator::CalculatorMetricByDivision}}
    };

    const std::vector<double> floatBitVec{1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0};
};
}
}

#endif // ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_PIPEUT_ITEM_H
