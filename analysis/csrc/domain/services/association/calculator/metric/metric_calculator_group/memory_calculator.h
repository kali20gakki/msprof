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

    bool CheckMetricEventValid(std::vector<uint32_t> &event) override
    {
        return CheckMetricEventBySubType(memoryTable, event);
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
        {MemoryIndex::L2ReadBw, {{0xf}, Calculator::CalculatorMetricByAdditionsWithFreq}},
        {MemoryIndex::L2WriteBw, {{0x10}, Calculator::CalculatorMetricByAdditionsWithFreq}},
    };

    const std::vector<double> floatBitVec{1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0};
    const std::vector<double> pipeSizeVec{256.0, 256.0, 256.0, 128.0, 8.0, 8.0, 256.0, 256.0};
    const std::vector<double> scalarVec{4.0, 4.0, 16.0, 8.0, 8.0, 8.0, 8.0, 8.0};
};
}
}
#endif // ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_MEMORY_ITEM_H
