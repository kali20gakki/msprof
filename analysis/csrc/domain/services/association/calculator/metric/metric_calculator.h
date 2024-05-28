/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : base_metric_calculator.h
 * Description        : pmu metric结构计算映射关系
 * Author             : msprof team
 * Creation Date      : 2024/5/9
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_SERVICE_ASSOCIATION_METRIC_CALCULATOR_H
#define ANALYSIS_DOMAIN_SERVICE_ASSOCIATION_METRIC_CALCULATOR_H

#include <utility>
#include <vector>
#include <map>
#include <set>
#include <functional>
#include "analysis/csrc/utils/utils.h"
#include "analysis/csrc/domain/entities/metric/include/metric.h"
#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/infrastructure/process/include/process.h"
#include "analysis/csrc/domain/entities/hal/include/hal_pmu.h"
#include "analysis/csrc/domain/entities/hal/include/device_task.h"

namespace Analysis {
namespace Domain {
using namespace Analysis::Infra;
namespace {
const size_t PMU_LENGTH = 8;
const uint64_t PMU_BW_OFFSET = 1LL << 33;  // 2^33,pmu计算规则里带有BW的计算需要除以2^33
const uint64_t FREQ_TO_MHz = 1000000;
const uint8_t AIV_CORE_TYPE = 1;
const uint8_t AIC_CORE_TYPE = 0;
}
struct CalculationElements {
    uint32_t events[8];
    uint32_t blockDim{0};
    uint32_t coreNum{0};
    uint64_t taskCyc{0};
    uint64_t freq{0};
    double totalTime{0.0};
    std::vector<uint64_t> pmuList;
    std::shared_ptr<std::vector<double>> floatBit;
    std::shared_ptr<std::vector<double>> pipSize;
    std::shared_ptr<std::vector<double>> scalar;
    std::shared_ptr<std::vector<uint64_t>> cubeParams;
    std::shared_ptr<std::vector<uint64_t>> vectorParams;
};

using DoublePtrType = std::vector<double>;
using IntPtrType = std::vector<uint64_t>;

class Calculator {
public:
    Calculator(std::set<int> registers, std::function<double(CalculationElements &params, size_t index)> func)
    : registers(std::move(registers)), calculatorFunc(std::move(func)) {}
    static double CalculatorMetricByNothing(CalculationElements &allParams, size_t index);
    static double CalculatorMetricByAdditions(CalculationElements &allParams, size_t index);
    static double CalculatorMetricByAdditionsWithFreq(CalculationElements &allParams, size_t index);
    static double CalculatorTimeByMultiplication(CalculationElements &allParams, size_t index);
    static double CalculatorCubeFops(CalculationElements &allParams, size_t index);
    static double CalculatorVectorFops(CalculationElements &allParams, size_t index);
    static double CalculatorTotalTime(uint64_t totalCycle, uint64_t blockDim, uint64_t coreNum, uint64_t freq);
public:
    std::set<int> registers;
    std::function<double(CalculationElements &params, size_t index)> calculatorFunc;
};


class MetricCalculator {
public:
    template<typename T>
    std::vector<double> CalculatePmu(HalPmuData pmuData, std::map<T, Calculator> calTable, CalculationElements& params)
    {
        auto pmuTable = GetValueMappingOffset(params.events, pmuData.pmu.pmuList);
        params.totalTime = Calculator::CalculatorTotalTime(pmuData.pmu.totalCycle, params.blockDim, params.coreNum,
                                                           params.freq);
        std::vector<double> res;
        if (calTable.size() != params.floatBit->size()) {
            ERROR("pmu calculator params init error please check!");
            return res;
        }
        size_t index = 0;
        for (auto& cal : calTable) {
            for (auto reg : cal.second.registers) {
                params.pmuList.push_back(pmuTable[reg]);
            }
            res.push_back(cal.second.calculatorFunc(params, index));
            ++index;
        }
        return res;
    }

    std::vector<double> CalculatePmuMetric(DataInventory& dataInventory, const DeviceContext& context,
                                           CalculationElements& allParams, HalPmuData& pmuData, DeviceTask& task);
    virtual ~MetricCalculator() = default;
private:
    virtual std::vector<double> SetAllParamsAndCalculator(CalculationElements& allParams, const DeviceContext& context,
                                                          HalPmuData& pmuData) = 0;
    static std::unordered_map<uint32_t, uint64_t> GetValueMappingOffset(uint32_t events[8], uint64_t pmuList[8]);
};
}
}
#endif // ANALYSIS_DOMAIN_SERVICE_ASSOCIATION_METRIC_CALCULATOR_H