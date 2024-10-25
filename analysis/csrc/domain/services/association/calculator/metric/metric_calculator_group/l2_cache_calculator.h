/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : l2_cache_calculator.h
 * Description        : L2Cache metric计算
 * Author             : msprof team
 * Creation Date      : 2024/5/21
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_L2_CACHE_ITEM_H
#define ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_L2_CACHE_ITEM_H

#include "analysis/csrc/domain/services/association/calculator/metric/metric_calculator.h"

namespace Analysis {
namespace Domain {
using namespace Analysis::Infra;
class L2CacheCalculator : public MetricCalculator {
public:
    std::vector<std::string> GetPmuHeader() override
    {
        auto res = GetPmuHeaderBySubType(l2CacheTable);
        return res;
    }

    bool CheckMetricEventValid(std::vector<uint32_t> &event) override
    {
        return CheckMetricEventBySubType(l2CacheTable, event);
    }
private:
    std::vector<double> SetAllParamsAndCalculator(CalculationElements& allParams, const DeviceContext& context,
                                                  HalPmuData& pmuData) override
    {
        std::vector<double> res;
        MAKE_SHARED_RETURN_VALUE(allParams.floatBit, DoublePtrType, res, floatBitVec);
        res = CalculatePmu(pmuData, l2CacheTable, allParams);
        return res;
    }
private:
    const std::map<L2CacheIndex, Calculator> l2CacheTable {
        {L2CacheIndex::WriteCacheHit, {{0x500}, Calculator::CalculatorMetricByNothing}},
        {L2CacheIndex::WriteCacheMissAllocate, {{0x502}, Calculator::CalculatorMetricByNothing}},
        {L2CacheIndex::R0ReadCacheHit, {{0x504}, Calculator::CalculatorMetricByNothing}},
        {L2CacheIndex::R0ReadCacheMissAllocate, {{0x506}, Calculator::CalculatorMetricByNothing}},
        {L2CacheIndex::R1ReadCacheHit, {{0x508}, Calculator::CalculatorMetricByNothing}},
        {L2CacheIndex::R1ReadCacheMissAllocate, {{0x50a}, Calculator::CalculatorMetricByNothing}},
    };
    const std::vector<double> floatBitVec{1.0, 1.0, 1.0, 1.0, 1.0, 1.0};
};
}
}
#endif // ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_L2_CACHE_ITEM_H
