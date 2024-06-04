/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : base_metric_calculator.h
 * Description        : pmu metric结构
 * Author             : msprof team
 * Creation Date      : 2024/5/9
 * *****************************************************************************
 */

#include "analysis/csrc/domain/services/association/calculator/metric/metric_calculator.h"
#include <algorithm>
#include "analysis/csrc/infrastructure/resource/chip_id.h"
#include "analysis/csrc/domain/entities/hal/include/hal_freq.h"
#include "securec.h"

namespace Analysis {
namespace Domain {
using namespace Analysis::Infra;
double Calculator::CalculatorTotalTime(uint64_t totalCycle, uint64_t blockDim, uint64_t coreNum, uint64_t freq)
{
    if (!blockDim || !coreNum || !freq) {
        return 0.0;
    }
    auto res = static_cast<double>(totalCycle) * FREQ_TO_Hz / freq / blockDim * ((blockDim + coreNum - 1) / coreNum);
    return res;
}

double Calculator::CalculatorMetricByAdditions(CalculationElements& allParams, size_t index)
{
    double res = 0.0;
    if (allParams.taskCyc != 0) {
        for (size_t i = 0; i < allParams.pmuList.size(); ++i) {
            res += ((*allParams.floatBit)[index] * allParams.pmuList[i] / allParams.taskCyc);
        }
    }
    return res;
}

double Calculator::CalculatorMetricByDivision(CalculationElements &allParams, size_t index)
{
    double res = 0.0;
    if (allParams.taskCyc == 0) {
        return res;
    }
    for (size_t i = 0; i < allParams.pmuList.size(); ++i) {
        if (i == 0) {
            res += ((*allParams.floatBit)[index] * allParams.pmuList[i] / allParams.taskCyc);
        } else {
            res /= ((*allParams.floatBit)[index] * allParams.pmuList[i] / allParams.taskCyc);
        }
    }
    return res;
}

double Calculator::CalculatorMetricByNothing(CalculationElements& allParams, size_t index)
{
    double res = 0.0;
    for (const auto& value : allParams.pmuList) {
        res += value;
    }
    return res;
}

double Calculator::CalculatorMetricByAdditionsWithFreq(CalculationElements& allParams, size_t index)
{
    double res = 0.0;
    if (allParams.taskCyc != 0 && allParams.bwFreq != 0) {
        for (size_t i = 0; i < allParams.pmuList.size(); ++i) {
            res += ((*allParams.floatBit)[index] * allParams.pmuList[i] * (*allParams.pipSize)[index] *
                    (*allParams.scalar)[index] / (allParams.taskCyc / allParams.bwFreq) / PMU_BW_OFFSET);
        }
    }
    return res;
}

double Calculator::CalculatorTimeByMultiplication(CalculationElements& allParams, size_t index)
{
    return allParams.totalTime * CalculatorMetricByAdditions(allParams, index);
}

double Calculator::CalculatorCubeFops(CalculationElements& allParams, size_t index)
{
    double res = 0.0;
    if (allParams.taskCyc != 0) {
        if (allParams.pmuList.size() > allParams.cubeParams->size()) {
            ERROR("The cube params count is small than the pmu count, can't calculator!");
            return res;
        }
        for (size_t i = 0; i < allParams.pmuList.size(); ++i) {
            res += ((*allParams.floatBit)[index] * allParams.pmuList[i] * (*allParams.cubeParams)[i]);
        }
    }
    return res;
}

double Calculator::CalculatorVectorFops(CalculationElements& allParams, size_t index)
{
    double res = 0.0;
    if (allParams.taskCyc != 0) {
        if (allParams.pmuList.size() > allParams.vectorParams->size()) {
            ERROR("The vector params count is small than the pmu count, can't calculator!");
            return res;
        }
        for (size_t i = 0; i < allParams.pmuList.size(); ++i) {
            res += ((*allParams.floatBit)[index] * allParams.pmuList[i] * (*allParams.vectorParams)[i]);
        }
    }
    return res;
}

std::unordered_map<uint32_t, uint64_t> MetricCalculator::GetValueMappingOffset(uint32_t events[8], uint64_t pmuList[8])
{
    std::unordered_map<uint32_t, uint64_t> res;
    for (size_t i = 0; i < PMU_LENGTH; ++i) {
        res.insert({events[i], pmuList[i]});
    }
    return res;
}

void SetBlockDim(DeviceTask& task, CalculationElements& allParams, HalPmuData& pmu)
{
    if ((pmu.pmu.acceleratorType == MIX_AIV && pmu.pmu.coreType == AIC_CORE_TYPE) ||
        (pmu.pmu.acceleratorType == MIX_AIC && pmu.pmu.coreType == AIV_CORE_TYPE)) {
        allParams.blockDim = task.mixBlockDim;
    } else {
        allParams.blockDim = task.blockDim;
    }
}

uint64_t GetFreq(std::vector<HalFreqLpmData>& freqData, uint64_t freq, HalPmuData& pmu)
{
    std::sort(freqData.begin(), freqData.end(), [](HalFreqLpmData& lData, HalFreqLpmData& rData) {
        return lData.sysCnt < rData.sysCnt;
    });
    bool hasZeroFreq = std::any_of(freqData.begin(), freqData.end(), [](HalFreqLpmData& data) {
        return data.freq == 0;
    });
    if (hasZeroFreq) {
        return freq;
    }
    auto res = freq;
    for (auto& data : freqData) {
        if (pmu.pmu.timeList[1] < data.sysCnt) {
            break;
        }
        res = data.freq * FREQ_TO_Hz;
    }
    return res != 0 ? res : freq;
}

std::vector<double> MetricCalculator::CalculatePmuMetric(DataInventory& dataInventory, const DeviceContext& context,
                                                         CalculationElements& params, HalPmuData& pmu, DeviceTask& task)
{
    DeviceInfo deviceInfo{};
    context.Getter(deviceInfo);
    SampleInfo sampleInfo;
    context.Getter(sampleInfo);
    uint64_t freq = deviceInfo.aicFrequency * FREQ_TO_Hz;
    params.bwFreq = freq;
    // chip4的PMU计算需要判断解析的频率信息
    if (context.GetChipID() == CHIP_V4_1_0) {
        auto freqData = dataInventory.GetPtr<std::vector<HalFreqLpmData>>();
        if (freqData != nullptr) {
            freq = GetFreq(*freqData, freq, pmu);
        }
    }
    params.timeFreq = freq;
    params.taskCyc = pmu.pmu.totalCycle;
    errno_t res;
    if (pmu.pmu.acceleratorType == MIX_AIC || pmu.pmu.acceleratorType == AIC) {
        res = memcpy_s(params.events, sizeof(params.events), sampleInfo.aiCoreProfilingEvents,
                       sizeof(sampleInfo.aiCoreProfilingEvents));
        params.coreNum = deviceInfo.aiCoreNum;
    } else {
        res = memcpy_s(params.events, sizeof(params.events), sampleInfo.aivProfilingEvents,
                       sizeof(sampleInfo.aivProfilingEvents));
        params.coreNum = deviceInfo.aivNum;
    }
    if (res != EOK) {
        ERROR("memcpy pmuEvents error taskId is %, streamId is %, contextId is %, batchId is %", pmu.hd.taskId.taskId,
              pmu.hd.taskId.streamId, pmu.hd.taskId.contextId, pmu.hd.taskId.batchId);
    }
    SetBlockDim(task, params, pmu);
    return SetAllParamsAndCalculator(params, context, pmu);
}
}
}
