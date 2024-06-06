/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : pmu_association.cpp
 * Description        : 将pmu数据填充到统一结构deviceTask中
 * Author             : msprof team
 * Creation Date      : 2024/5/16
 * *****************************************************************************
 */
#include "analysis/csrc/domain/services/association/include/pmu_association.h"
#include <map>
#include <cstdlib>
#include <algorithm>
#include "analysis/csrc/dfx/error_code.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"
#include "analysis/csrc/infrastructure/process/include/process_register.h"
#include "analysis/csrc/domain/services/device_context/load_host_data.h"
#include "analysis/csrc/domain/services/modeling/include/pmu_modeling.h"
#include "analysis/csrc/domain/entities/hal/include/hal_freq.h"

namespace Analysis {
namespace Domain {
using namespace Analysis::Utils;
void PmuAssociation::SplitPmu(std::vector<HalPmuData> &pmuData)
{
    int context_size = 0;
    int block_size = 0;
    for (auto& pmu : pmuData) {
        if (pmu.type == PMU) {
            contextPmuTask_[pmu.hd.taskId].push_back(&pmu);
            ++context_size;
        } else {
            blockPmuTask_[pmu.hd.taskId].push_back(&pmu);
            ++block_size;
        }
    }
    INFO("context pmu count is : %", context_size);
    INFO("block pmu count is : %", block_size);
}

void BlockPmuResultAccumulate(uint64_t* arr1, uint64_t* arr2, int size)
{
    for (int i = 0; i < size; ++i) {
        arr1[i] += arr2[i];
    }
}

void PmuAssociation::MergeContextPmuToDeviceTask(std::vector<HalPmuData*>& pmuVec, std::vector<DeviceTask>& deviceVec,
                                                 DataInventory& dataInventory, const DeviceContext& context)
{
    int missCount = abs(static_cast<int>(pmuVec.size() - deviceVec.size()));
    size_t matchSize = std::min(pmuVec.size(), deviceVec.size());
    for (std::size_t index = 0; index < matchSize; index++) {
        deviceVec[index].acceleratorType = pmuVec[index]->pmu.acceleratorType;
        CalculationElements params;
        if (deviceVec[index].acceleratorType == MIX_AIC || deviceVec[index].acceleratorType == MIX_AIV) {
            PmuInfoMixAccelerator pmuInfoMix;
            if (pmuVec[index]->pmu.acceleratorType == MIX_AIC) {
                pmuInfoMix.aicTotalCycles = pmuVec[index]->pmu.totalCycle;
                auto res = aicCalculator_->CalculatePmuMetric(dataInventory, context, params, *pmuVec[index],
                                                              deviceVec[index]);
                pmuInfoMix.aicPmuResult.swap(res);
                pmuInfoMix.aiCoreTime = params.totalTime;
            } else {
                pmuInfoMix.aivTotalCycles = pmuVec[index]->pmu.totalCycle;
                auto res = aivCalculator_->CalculatePmuMetric(dataInventory, context, params, *pmuVec[index],
                                                              deviceVec[index]);
                pmuInfoMix.aivPmuResult.swap(res);
                pmuInfoMix.aivTime = params.totalTime;
            }
            pmuInfoMix.mainTimestamp = pmuVec[index]->pmu.timeList[1];
            deviceVec[index].pmuInfo = MAKE_UNIQUE_PTR<PmuInfoMixAccelerator>(pmuInfoMix);
        } else {
            PmuInfoSingleAccelerator pmuInfoNormal;
            pmuInfoNormal.totalCycles = pmuVec[index]->pmu.totalCycle;
            std::vector<double> res;
            if (pmuVec[index]->pmu.acceleratorType == AIC) {
                res = aicCalculator_->CalculatePmuMetric(dataInventory, context, params, *pmuVec[index],
                                                         deviceVec[index]);
            } else {
                res = aivCalculator_->CalculatePmuMetric(dataInventory, context, params, *pmuVec[index],
                                                         deviceVec[index]);
            }
            pmuInfoNormal.totalTime = params.totalTime;
            pmuInfoNormal.pmuResult.swap(res);
            deviceVec[index].pmuInfo = MAKE_UNIQUE_PTR<PmuInfoSingleAccelerator>(pmuInfoNormal);
        }
    }
    if (missCount != 0) {
        ERROR("There has missMatch % pmu to deviceTask, taskId is %, streamId is %, contextId is %, batchId is %",
              missCount, pmuVec[0]->hd.taskId.taskId, pmuVec[0]->hd.taskId.streamId, pmuVec[0]->hd.taskId.contextId,
              pmuVec[0]->hd.taskId.batchId);
    }
}

size_t PmuAssociation::MergeBlockPmuToDeviceTask(std::vector<HalPmuData*>& pmuData, DeviceTask& deviceTask,
                                                 DataInventory& dataInventory, const DeviceContext& context)
{
    if (deviceTask.pmuInfo == nullptr) {
        ERROR("Block Pmu has no context Pmu taskId is %, streamId is %, contextId is",
              pmuData[0]->hd.taskId.taskId, pmuData[0]->hd.taskId.streamId, pmuData[0]->hd.taskId.contextId);
        return pmuData.size();
    }
    uint32_t mixCount = deviceTask.mixBlockDim;
    uint8_t core_type = AIC_CORE_TYPE;
    if (deviceTask.acceleratorType == MIX_AIC) {
        core_type = AIV_CORE_TYPE;
    }
    CalculationElements params;
    HalPmuData tempPmuData;
    tempPmuData.type = BLOCK_PMU;
    auto res = dynamic_cast<PmuInfoMixAccelerator *>(deviceTask.pmuInfo.get());
    for (auto it = pmuData.begin(); it != pmuData.end();) {
        if ((*it)->pmu.acceleratorType == deviceTask.acceleratorType && (*it)->pmu.coreType == core_type) {
            if (res->totalBlockCount >= mixCount) {
                break;
            }
            tempPmuData.hd.taskId = {(*it)->hd.taskId.streamId, (*it)->hd.taskId.batchId, (*it)->hd.taskId.taskId,
                                     (*it)->hd.taskId.contextId};
            res->totalBlockCount += 1;
            tempPmuData.pmu.timeList[1] = res->mainTimestamp;
            tempPmuData.pmu.totalCycle += (*it)->pmu.totalCycle;
            tempPmuData.pmu.acceleratorType = (*it)->pmu.acceleratorType;
            tempPmuData.pmu.coreType = (*it)->pmu.coreType;
            BlockPmuResultAccumulate(tempPmuData.pmu.pmuList, ((*it)->pmu.pmuList), PMU_LENGTH);
            it = pmuData.erase(it);
        } else {
            ++it;
        }
    }
    if (core_type) {
        auto pmuRes = aivCalculator_->CalculatePmuMetric(dataInventory, context, params, tempPmuData, deviceTask);
        res->aivTime = params.totalTime;
        res->aivPmuResult.swap(pmuRes);
        res->aivTotalCycles = tempPmuData.pmu.totalCycle;
    } else {
        auto pmuRes = aicCalculator_->CalculatePmuMetric(dataInventory, context, params, tempPmuData, deviceTask);
        res->aiCoreTime = params.totalTime;
        res->aicPmuResult.swap(pmuRes);
        res->aicTotalCycles = tempPmuData.pmu.totalCycle;
    }
    return pmuData.size();
}

void PmuAssociation::AssociationByPmuType(std::map<TaskId, std::vector<DeviceTask>>& deviceTask,
                                          DataInventory& dataInventory, const DeviceContext& context)
{
    for (auto& pmu : contextPmuTask_) {
        auto it = deviceTask.find(pmu.first);
        if (it != deviceTask.end()) {
            MergeContextPmuToDeviceTask(pmu.second, it->second, dataInventory, context);
        } else {
            ERROR("contextPmu has no matched log taskId is %, streamId is %, contextId is %, batchId is %",
                  pmu.first.taskId, pmu.first.streamId, pmu.first.contextId, pmu.first.batchId);
        }
    }
    INFO("Context PMU has been calculated!");
    for (auto& pmu : blockPmuTask_) {
        auto it = deviceTask.find(pmu.first);
        if (it == deviceTask.end()) {
            ERROR("blockPmu has no matched log taskId is %, streamId is %, contextId is %, batchId is %",
                  pmu.first.taskId, pmu.first.streamId, pmu.first.contextId, pmu.first.batchId);
            continue;
        }
        std::sort(it->second.begin(), it->second.end(), [](DeviceTask& lData, DeviceTask& rData) {
            return lData.taskStart < rData.taskStart;
        });
        size_t pmuIndex;
        for (auto& task : it->second) {
            if (task.acceleratorType == AIC || task.acceleratorType == AIV) {
                INFO("block pmu is used only by MIX, but context pmu is not MIX! No association is required.");
                continue;
            }
            pmuIndex = MergeBlockPmuToDeviceTask(pmu.second, task, dataInventory, context);
            if (pmuIndex == 0) {
                INFO("Block Pmu has been used up!");
                break;
            }
        }
    }
}

uint32_t PmuAssociation::ProcessEntry(Infra::DataInventory& dataInventory, const Infra::Context& context)
{
    auto& deviceContext = dynamic_cast<const DeviceContext&>(context);
    SampleInfo sampleInfo;
    deviceContext.Getter(sampleInfo);
    aicCalculator_ = MetricCalculatorFactory::GetAicCalculator(sampleInfo.aiCoreMetrics);
    aivCalculator_ = MetricCalculatorFactory::GetAivCalculator(sampleInfo.aivMetrics);
    auto deviceTask = dataInventory.GetPtr<std::map<TaskId, std::vector<DeviceTask>>>();
    auto pmuData = dataInventory.GetPtr<std::vector<HalPmuData>>();
    if (deviceTask->empty()) {
        ERROR("There is no deviceTask, can't associate!");
        return ANALYSIS_ERROR;
    }
    SplitPmu(*pmuData);
    if (deviceContext.GetChipID() != CHIP_V4_1_0) {
        // 非MIX算子不需要计算block级别数据
        blockPmuTask_.clear();
        INFO("The acceleratorType of PMU is not MIX");
    } else {
        INFO("The acceleratorType of PMU is MIX");
    }
    AssociationByPmuType(*deviceTask, dataInventory, deviceContext);
    return ANALYSIS_OK;
}

REGISTER_PROCESS_SEQUENCE(PmuAssociation, true, LoadHostData, PmuModeling);
REGISTER_PROCESS_DEPENDENT_DATA(PmuAssociation, std::vector<HalPmuData>, std::map<TaskId, std::vector<DeviceTask>>,
                                std::vector<HalFreqLpmData>);
REGISTER_PROCESS_SUPPORT_CHIP(PmuAssociation, CHIP_ID_ALL);
}
}
