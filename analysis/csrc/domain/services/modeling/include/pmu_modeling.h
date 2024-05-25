/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : pmu_modeling.h
 * Description        : pmu modeling 处理流程类
 * Author             : msprof team
 * Creation Date      : 2024/5/11
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_SERVICES_MODELING_PMU_MODELING_H
#define ANALYSIS_DOMAIN_SERVICES_MODELING_PMU_MODELING_H

#include <unordered_map>
#include "analysis/csrc/domain/entities/hal/include/hal_pmu.h"
#include "analysis/csrc/domain/entities/hal/include/hal_track.h"
#include "analysis/csrc/infrastructure/process/include/process.h"

namespace Analysis {
namespace Domain {
class PmuModeling : public Infra::Process {
private:
    uint32_t ProcessEntry(Infra::DataInventory& dataInventory, const Infra::Context& context) override;

    template<typename T>
    static bool cmp(T* lData, T* rData)
    {
        return lData->hd.timestamp < rData->hd.timestamp;
    }
    void GenerateBatchId();
    void GroupDataByStream(std::vector<HalPmuData>& pmuData, std::vector<HalTrackData>& flipTrack);
private:
    std::unordered_map<uint16_t, std::vector<HalPmuData*>> pmu_;
    std::unordered_map<uint16_t, std::vector<HalTrackData*>> flipGroup_;
};
}
}
#endif // ANALYSIS_DOMAIN_SERVICES_MODELING_PMU_MODELING_H
