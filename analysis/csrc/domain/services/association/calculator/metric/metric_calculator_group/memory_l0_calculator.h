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

#ifndef ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_MEMORY_L0_ITEM_H
#define ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_MEMORY_L0_ITEM_H

#include "analysis/csrc/domain/services/association/calculator/metric/metric_calculator.h"

namespace Analysis {
namespace Domain {
using namespace Analysis::Infra;
class MemoryL0Calculator : public MetricCalculator {
public:
    std::vector<std::string> GetPmuHeader() override
    {
        auto res = GetPmuHeaderBySubType(memoryL0Table);
        return res;
    }

    bool CheckMetricEventValid(std::vector<uint32_t> &event) override
    {
        return CheckMetricEventBySubType(memoryL0Table, event);
    }
private:
    std::vector<double> SetAllParamsAndCalculator(CalculationElements& allParams, const DeviceContext& context,
                                                  HalPmuData& pmuData) override
    {
        std::vector<double> res;
        MAKE_SHARED_RETURN_VALUE(allParams.floatBit, DoublePtrType, res, floatBitVec);
        MAKE_SHARED_RETURN_VALUE(allParams.pipSize, DoublePtrType, res, pipeSizeVec);
        MAKE_SHARED_RETURN_VALUE(allParams.scalar, DoublePtrType, res, scalarVec);
        res = CalculatePmu(pmuData, memoryL0Table, allParams);
        return res;
    }
private:
    const std::map<MemoryL0Index, Calculator> memoryL0Table{
        {MemoryL0Index::L0aReadBw, {{0x1b}, Calculator::CalculatorMetricByAdditionsWithFreq}},
        {MemoryL0Index::L0aWriteBw, {{0x1c}, Calculator::CalculatorMetricByAdditionsWithFreq}},
        {MemoryL0Index::L0bReadBw, {{0x21}, Calculator::CalculatorMetricByAdditionsWithFreq}},
        {MemoryL0Index::L0bWriteBw, {{0x22}, Calculator::CalculatorMetricByAdditionsWithFreq}},
        {MemoryL0Index::L0cReadBw, {{0x27}, Calculator::CalculatorMetricByAdditionsWithFreq}},
        {MemoryL0Index::L0cWriteBw, {{0x29}, Calculator::CalculatorMetricByAdditionsWithFreq}},
        {MemoryL0Index::L0cReadBwCube, {{0x28}, Calculator::CalculatorMetricByAdditionsWithFreq}},
        {MemoryL0Index::L0cWriteBwCube, {{0x2a}, Calculator::CalculatorMetricByAdditionsWithFreq}},
    };

    const std::vector<double> floatBitVec{1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0};
    const std::vector<double> pipeSizeVec{256.0, 256.0, 256.0, 256.0, 256.0, 256.0, 256.0, 256.0};
    const std::vector<double> scalarVec{16.0, 16.0, 16.0, 8.0, 8.0, 8.0, 32.0, 32.0};
};
}
}

#endif // ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_MEMORY_L0_ITEM_H
