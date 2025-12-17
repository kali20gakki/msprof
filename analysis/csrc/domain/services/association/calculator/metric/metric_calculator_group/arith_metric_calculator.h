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

#ifndef ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_ARITH_METRIC_ITEM_H
#define ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_ARITH_METRIC_ITEM_H

#include <algorithm>
#include "analysis/csrc/domain/services/association/calculator/metric/metric_calculator.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"

namespace Analysis {
namespace Domain {
using namespace Analysis::Infra;
class ArithMetricCalculator : public MetricCalculator {
public:
    std::vector<std::string> GetPmuHeader() override
    {
        auto res = GetPmuHeaderBySubType(arithMetricUtTable);
        return res;
    }

    bool CheckMetricEventValid(std::vector<uint32_t> &event) override
    {
        return CheckMetricEventBySubType(arithMetricUtTable, event);
    }
private:
    std::vector<double> SetAllParamsAndCalculator(CalculationElements& allParams, const DeviceContext& context,
                                                  HalPmuData& pmuData) override
    {
        std::vector<double> res;
        MAKE_SHARED_RETURN_VALUE(allParams.cubeParams, IntPtrType, res, cubeParams);
        MAKE_SHARED_RETURN_VALUE(allParams.floatBit, DoublePtrType, res, floatBitVec);
        if (std::count(chip_3.begin(), chip_3.end(), context.GetChipID()) > 0) {
            MAKE_SHARED_RETURN_VALUE(allParams.vectorParams, IntPtrType, res, vectorParamsForChip3);
        } else {
            MAKE_SHARED_RETURN_VALUE(allParams.vectorParams, IntPtrType, res, vectorParams);
        }
        res = CalculatePmu(pmuData, arithMetricUtTable, allParams);
        return res;
    }
private:
    const std::map<ArithMetricIndex, Calculator> arithMetricUtTable{
        {ArithMetricIndex::MacFp16Ratio, {{0x49}, Calculator::CalculatorMetricByAdditions}},
        {ArithMetricIndex::MacInt8Ratio, {{0x4a}, Calculator::CalculatorMetricByAdditions}},
        {ArithMetricIndex::VecFp32Ratio, {{0x4b}, Calculator::CalculatorMetricByAdditions}},
        {ArithMetricIndex::VecFp16Ratio, {{0x4c, 0x4d}, Calculator::CalculatorMetricByAdditions}},
        {ArithMetricIndex::VecInt32Ratio, {{0x4e}, Calculator::CalculatorMetricByAdditions}},
        {ArithMetricIndex::VecMiscRatio, {{0x4f}, Calculator::CalculatorMetricByAdditions}},
        {ArithMetricIndex::CubeFops, {{0x49, 0x4a}, Calculator::CalculatorCubeFops}},
        {ArithMetricIndex::VectorFops, {{0x4c, 0x4d, 0x4b, 0x4e, 0x4f}, Calculator::CalculatorVectorFops}}
    };

    const std::vector<ChipId> chip_3{CHIP_V3_1_0, CHIP_V3_2_0, CHIP_V3_3_0};
    const std::vector<double> floatBitVec{1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0};
    const std::vector<uint64_t> vectorParamsForChip3{128, 64, 64, 16, 16};
    const std::vector<uint64_t> vectorParams{128, 64, 64, 64, 32};
    const std::vector<uint64_t> cubeParams{8192, 16384};
};
}
}
#endif // ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_ARITH_METRIC_ITEM_H
