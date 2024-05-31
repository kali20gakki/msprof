/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : metric_calculator_factory.cpp
 * Description        : metric计算工厂类
 * Author             : msprof team
 * Creation Date      : 2024/5/21
 * *****************************************************************************
 */

#include "analysis/csrc/domain/services/association/calculator/include/metric_calculator_factory.h"
#include "analysis/csrc/domain/services/association/calculator/metric/metric_calculator_group/arith_metric_calculator.h"
#include "analysis/csrc/domain/services/association/calculator/metric/metric_calculator_group/l2_cache_calculator.h"
#include "analysis/csrc/domain/services/association/calculator/metric/metric_calculator_group/memory_calculator.h"
#include "analysis/csrc/domain/services/association/calculator/metric/metric_calculator_group/memory_l0_calculator.h"
#include "analysis/csrc/domain/services/association/calculator/metric/metric_calculator_group/memory_ub_calculator.h"
#include "analysis/csrc/domain/services/association/calculator/metric/metric_calculator_group/pipeut_calculator.h"
#include "analysis/csrc/domain/services/association/calculator/metric/metric_calculator_group/pipeutext_calculator.h"
#include "analysis/csrc/domain/services/association/calculator/metric/metric_calculator_group/resource_conflict_calculator.h"

namespace Analysis {
namespace Domain {
using namespace Utils;
std::unordered_map<AicMetricsEventsType, Creator> MetricCalculatorFactory::aicEvent{
    {AicMetricsEventsType::AIC_ARITHMETIC_UTILIZATION, []() {
        return MAKE_UNIQUE_PTR<ArithMetricCalculator>();
    }},
    {AicMetricsEventsType::AIC_PIPE_UTILIZATION, []() {
        return MAKE_UNIQUE_PTR<PipeUtCalculator>();
    }},
    {AicMetricsEventsType::AIC_PIPE_UTILIZATION_EXCT, []() {
        return MAKE_UNIQUE_PTR<PipeUtExtCalculator>();
    }},
    {AicMetricsEventsType::AIC_MEMORY, []() {
        return MAKE_UNIQUE_PTR<MemoryCalculator>();
    }},
    {AicMetricsEventsType::AIC_MEMORY_L0, []() {
        return MAKE_UNIQUE_PTR<MemoryL0Calculator>();
    }},
    {AicMetricsEventsType::AIC_RESOURCE_CONFLICT_RATIO, []() {
        return MAKE_UNIQUE_PTR<ResourceConflictCalculator>();
    }},
    {AicMetricsEventsType::AIC_MEMORY_UB, []() {
        return MAKE_UNIQUE_PTR<MemoryUBCalculator>();
    }},
    {AicMetricsEventsType::AIC_L2_CACHE, []() {
        return MAKE_UNIQUE_PTR<L2CacheCalculator>();
    }},
};

std::unordered_map<AivMetricsEventsType, Creator> MetricCalculatorFactory::aivEvent{
    {AivMetricsEventsType::AIV_ARITHMETIC_UTILIZATION, []() {
        return MAKE_UNIQUE_PTR<ArithMetricCalculator>();
    }},
    {AivMetricsEventsType::AIV_PIPE_UTILIZATION, []() {
        return MAKE_UNIQUE_PTR<PipeUtCalculator>();
    }},
    {AivMetricsEventsType::AIV_MEMORY, []() {
        return MAKE_UNIQUE_PTR<MemoryCalculator>();
    }},
    {AivMetricsEventsType::AIV_MEMORY_L0, []() {
        return MAKE_UNIQUE_PTR<MemoryL0Calculator>();
    }},
    {AivMetricsEventsType::AIV_RESOURCE_CONFLICT_RATIO, []() {
        return MAKE_UNIQUE_PTR<ResourceConflictCalculator>();
    }},
    {AivMetricsEventsType::AIV_MEMORY_UB, []() {
        return MAKE_UNIQUE_PTR<MemoryUBCalculator>();
    }},
    {AivMetricsEventsType::AIV_L2_CACHE, []() {
        return MAKE_UNIQUE_PTR<L2CacheCalculator>();
    }},
};
}
}