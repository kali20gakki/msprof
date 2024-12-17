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

#ifndef ANALYSIS_DOMAIN_SERVICES_MEMORY_ACCESS_CALCULATOR_H
#define ANALYSIS_DOMAIN_SERVICES_MEMORY_ACCESS_CALCULATOR_H

#include "analysis/csrc/domain/services/association/calculator/metric/metric_calculator.h"

namespace Analysis {
namespace Domain {
using namespace Analysis::Infra;
class MemoryAccessCalculator : public MetricCalculator {
public:
    std::vector<std::string> GetPmuHeader() override
    {
        auto res = GetPmuHeaderBySubType(memoryAccess);
        return res;
    }

    bool CheckMetricEventValid(std::vector<uint32_t> &event) override
    {
        return CheckMetricEventBySubType(memoryAccess, event);
    }
private:
    std::vector<double> SetAllParamsAndCalculator(CalculationElements& allParams, const DeviceContext& context,
                                                  HalPmuData& pmuData) override
    {
        std::vector<double> res;
        MAKE_SHARED_RETURN_VALUE(allParams.floatBit, DoublePtrType, res, floatBitVec);
        res = CalculatePmu(pmuData, memoryAccess, allParams);
        return res;
    }
private:
    const std::map<MemoryAccessIndex, Calculator> memoryAccess{
        {MemoryAccessIndex::ReadMainMemoryData, {{0x50d, 0x50e}, Calculator::CalculateMetricWithoutCycByAdd}},
        {MemoryAccessIndex::WriteMainMemoryData, {{0x50c}, Calculator::CalculateMetricWithoutCycByAdd}},
        {MemoryAccessIndex::GmToL1Data, {{0x32, 0x206}, Calculator::CalculateMetricWithoutCycBySub}},
        {MemoryAccessIndex::L0CToL1Data, {{0x206}, Calculator::CalculateMetricWithoutCycByAdd}},
        {MemoryAccessIndex::L0CToGmData, {{0x20c, 0x206}, Calculator::CalculateMetricWithoutCycBySub}},
        {MemoryAccessIndex::GmToUbData, {{0x3e}, Calculator::CalculateMetricWithoutCycByAdd}},
        {MemoryAccessIndex::UbToGmData, {{0x3d}, Calculator::CalculateMetricWithoutCycByAdd}},
    };
    const std::vector<double> floatBitVec{0.125, 0.125, 0.25, 0.125, 0.125, 0.125, 0.125};
};
}
}
#endif // ANALYSIS_DOMAIN_SERVICES_MEMORY_ACCESS_CALCULATOR_H
