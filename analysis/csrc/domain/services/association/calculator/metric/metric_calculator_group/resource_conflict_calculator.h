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

#ifndef ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_RESOURCE_CONFLICT_ITEM_H
#define ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_RESOURCE_CONFLICT_ITEM_H

#include "analysis/csrc/domain/services/association/calculator/metric/metric_calculator.h"

namespace Analysis {
namespace Domain {
using namespace Analysis::Infra;
class ResourceConflictCalculator : public MetricCalculator {
public:
    std::vector<std::string> GetPmuHeader() override
    {
        auto res = GetPmuHeaderBySubType(resourceConflictTable);
        return res;
    }

    bool CheckMetricEventValid(std::vector<uint32_t> &event) override
    {
        return CheckMetricEventBySubType(resourceConflictTable, event);
    }
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
