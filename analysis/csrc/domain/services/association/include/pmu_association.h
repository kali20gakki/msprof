/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : pmu_association.h
 * Description        : 将pmu数据填充到统一结构deviceTask中
 * Author             : msprof team
 * Creation Date      : 2024/5/16
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_PMU_ASSOCIATION_H
#define ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_PMU_ASSOCIATION_H

#include <set>
#include <map>
#include "analysis/csrc/infrastructure/process/include/process.h"
#include "analysis/csrc/domain/entities/hal/include/device_task.h"
#include "analysis/csrc/domain/entities/hal/include/hal_pmu.h"
#include "analysis/csrc/domain/services/association/calculator/include/metric_calculator_factory.h"

namespace Analysis {
namespace Domain {
using namespace Analysis::Infra;
class PmuAssociation : public Process {
private:
    void SplitPmu(std::vector<HalPmuData>& pmuData);
    void MergeContextPmuToDeviceTask(std::vector<HalPmuData*>& pmuVec, std::vector<DeviceTask>& deviceVec,
                                     DataInventory& dataInventory, const DeviceContext& context);
    size_t MergeBlockPmuToDeviceTask(std::vector<HalPmuData*>& pmuData, DeviceTask& deviceTask,
                                     DataInventory& dataInventory, const DeviceContext& context);
    void AssociationByPmuType(std::map<TaskId, std::vector<DeviceTask>>& deviceTask, DataInventory& dataInventory,
                              const DeviceContext& context);
    uint32_t ProcessEntry(Infra::DataInventory& dataInventory, const Infra::Context& context) override;
private:
    std::map<TaskId, std::vector<HalPmuData*>> contextPmuTask_;
    std::map<TaskId, std::vector<HalPmuData*>> blockPmuTask_;
    std::unique_ptr<MetricCalculator> aicCalculator_;
    std::unique_ptr<MetricCalculator> aivCalculator_;
};
}
}
#endif // ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_PMU_ASSOCIATION_H
